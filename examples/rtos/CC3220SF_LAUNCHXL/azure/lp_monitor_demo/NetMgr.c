/*
 * Copyright (c) 2017 Texas Instruments Incorporated - http://www.ti.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 *
 *   Neither the name of Texas Instruments Incorporated nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== NetMgr.c ========
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <mqueue.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC32XX.h>

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/platform.h"
#include "serializer.h"
#include "iothub_client_ll.h"
#include "iothubtransporthttp.h"
#include "iothub_client_version.h"

/* Example/Board Header files */
#include "DBG.h"
#include "NWP.h"
#include "SensorData.h"

#include "NetMgr.h"

extern void startNTP(void);

/* USER STEP: Use Device Explorer to register device to get connectionString */
static const char* connectionString = "";

BEGIN_NAMESPACE(DEMO);

DECLARE_MODEL(SensorData,
WITH_DATA(ascii_char_ptr, deviceId),
WITH_DATA(ascii_char_ptr, ts),
WITH_DATA(float, temperature)
);

END_NAMESPACE(DEMO);

#define NETMGR_TSKSTKSIZE 0x2000
#define MAXMSGS 10  /* Maximum number of data packets in mq */

typedef enum NetMrg_State {
    NetMgr_State_Zero,              /* initial state, not yet started       */
    NetMgr_State_Provisioning,
    NetMgr_State_ConnectAP,
    NetMgr_State_AcquireNTS,
    NetMgr_State_ConnectCloud,
    NetMgr_State_Ready,
    NetMgr_State_Publish,
    NetMgr_State_DisconnectCloud,
    NetMgr_State_DisconnectAP,
    NetMgr_State_WaitForData,
    NetMgr_State_Error,
    NetMgr_State_ErrorConnectCloud      /* cloud connect failure           */
} NetMgr_State;

/* module object */
typedef struct {
    NetMgr_State        state;          /* current state of the FSM         */
    mqd_t               workMq;
    sem_t               workSem;
    bool                timeSync;
    bool                error;          /* global error flag                */
    bool                started;
    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
    SensorData*         myData;
    unsigned int        messageId;
} NetMgr_Object;

static NetMgr_Object NetMgr = {
    .state = NetMgr_State_Zero,
    .timeSync = true,                      /* need a time sync                 */
    .error = false,
    .started = false,
    .iotHubClientHandle = NULL,
    .myData = NULL,
    .messageId = 1
};

/* private methods */
static void *NetMgr_threadFxn(void *arg0);
static int NetMgr_executeState(void);
static int NetMgr_startNTP(void);
static int NetMgr_publish(void);
static int NetMgr_transition(void);

/* Report if mq is empty */
static bool isEmpty(mqd_t * mq)
{
    struct mq_attr attr;
    int nb;
    char buf[64];

    if (mq_getattr(*mq, &attr) == -1) {
        nb = snprintf(buf, sizeof(buf), "mq_getattr failed!");
        DBG_write(buf, nb);
        while(1);
    }

    if (attr.mq_curmsgs == 0) {
        return true;
    }
    else {
        return false;
    }
}

/* Report if mq is full */
static bool isFull(mqd_t * mq)
{
    struct mq_attr attr;
    int nb;
    char buf[64];

    if (mq_getattr(*mq, &attr) == -1) {
        nb = snprintf(buf, sizeof(buf), "mq_getattr failed!");
        DBG_write(buf, nb);
        while(1);
    }

    if (attr.mq_curmsgs == MAXMSGS) {
        return true;
    }
    else {
        return false;
    }
}

static void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    unsigned int messageTrackingId = (unsigned int)(uintptr_t)userContextCallback;
    int nb;
    char buf[128];

    if (result != IOTHUB_CLIENT_CONFIRMATION_OK) {
        nb = snprintf(buf, sizeof(buf),
                "NetMgr: Error in sendCallback! Message Id: %u, Result: %s\r\r\r\n",
                messageTrackingId,
                ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
        DBG_write(buf, nb);
    }
}

static IOTHUBMESSAGE_DISPOSITION_RESULT IoTHubMessage(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    IOTHUBMESSAGE_DISPOSITION_RESULT result;
    const unsigned char* buffer;
    size_t size;
    int nb;
    char buf[64];

    if (IoTHubMessage_GetByteArray(message, &buffer, &size) != IOTHUB_MESSAGE_OK) {
        nb = snprintf(buf, sizeof(buf), "unable to IoTHubMessage_GetByteArray");
        DBG_write(buf, nb);
        result = IOTHUBMESSAGE_ABANDONED;
    }
    else {
        /*buffer is not zero terminated*/
        char* temp = malloc(size + 1);
        if (temp == NULL) {
            nb = snprintf(buf, sizeof(buf), "failed to malloc");
            DBG_write(buf, nb);
            result = IOTHUBMESSAGE_ABANDONED;
        }
        else {
            EXECUTE_COMMAND_RESULT executeCommandResult;

            (void)memcpy(temp, buffer, size);
            temp[size] = '\0';
            executeCommandResult = EXECUTE_COMMAND(userContextCallback, temp);
            result =
                (executeCommandResult == EXECUTE_COMMAND_ERROR) ? IOTHUBMESSAGE_ABANDONED :
                (executeCommandResult == EXECUTE_COMMAND_SUCCESS) ? IOTHUBMESSAGE_ACCEPTED :
                IOTHUBMESSAGE_REJECTED;
            free(temp);
        }
    }
    return result;
}

/*
 *  ======== NetMgr_done ========
 */
bool NetMgr_done(void)
{
    bool isDone = false;

    if (isEmpty(&NetMgr.workMq)) {
        isDone = true;
    }

    return (isDone);
}

/*
 *  ======== NetMgr_transition ========
 *  Process next input event for the current state
 */
static int NetMgr_transition(void)
{
    int rval = 0;

    /* process given input for the current state */
    switch (NetMgr.state) {

        case NetMgr_State_Zero:
            if (NetMgr.error) {
                NetMgr.state = NetMgr_State_Error;
            }
            else {
                NetMgr.state = NetMgr_State_ConnectAP;
            }
            break;

        case NetMgr_State_ConnectAP:
            if (NetMgr.error) {
                NetMgr.state = NetMgr_State_Error;
            }
            else if (NetMgr.timeSync) {
                NetMgr.state = NetMgr_State_AcquireNTS;
            }
            else {
                NetMgr.state = NetMgr_State_ConnectCloud;
            }
            break;

        case NetMgr_State_AcquireNTS:
            if (NetMgr.error) {
                NetMgr.state = NetMgr_State_Error;
            }
            else {
                NetMgr.state = NetMgr_State_ConnectCloud;
            }
            break;

        case NetMgr_State_ConnectCloud:
            if (!NetMgr.error) {
                NetMgr.state = NetMgr_State_Ready;
            }
            else {
                /* Retry */
                NetMgr.state = NetMgr_State_ErrorConnectCloud;
            }
            break;

        case NetMgr_State_Ready:
            if (NetMgr_done()) {
                NetMgr.state = NetMgr_State_DisconnectCloud;
            }
            else {
                NetMgr.state = NetMgr_State_Publish;
            }
            break;

        case NetMgr_State_Publish:
            if (NetMgr.error) {
                NetMgr.state = NetMgr_State_Error;
            }
            else {
                NetMgr.state = NetMgr_State_Ready;
            }
            break;

        case NetMgr_State_DisconnectCloud:
            NetMgr.state = NetMgr_State_DisconnectAP;
            break;

        case NetMgr_State_DisconnectAP:
            NetMgr.state = NetMgr_State_WaitForData;
            break;

        case NetMgr_State_WaitForData:
            if (!NetMgr_done()) {
                NetMgr.state = NetMgr_State_ConnectAP;
            }
            break;

        case NetMgr_State_Error:
            while (1) {
                sleep(1);
            }
            //break;

        case NetMgr_State_ErrorConnectCloud:
            NetMgr.error = false;
            NetMgr.state = NetMgr_State_ConnectCloud;
            break;

    } /* switch */

    return (rval);
}

/*
 *  ======== NetMgr_executeState ========
 */
static int NetMgr_executeState(void)
{
    int rval = 0;
    int nb;
    char buf[64];
    /*
     * Because it can poll "after 9 seconds" polls will happen
     * effectively at ~10 seconds.
     * Note that for scalability, the default value of minimumPollingTime
     * is 25 minutes. For more information, see:
     * https://azure.microsoft.com/documentation/articles/iot-hub-devguide/#messaging
     */
    unsigned int minimumPollingTime = 9;

    switch (NetMgr.state) {
    case NetMgr_State_Zero:
        break;

    case NetMgr_State_ConnectAP:
        /* start network processor, connect to access point (AP) */
        rval = NWP_startup();

        if (rval < 0) {
            NetMgr.error = true;
        }
        break;

    case NetMgr_State_AcquireNTS:
        NetMgr.timeSync = false;

        /* get the current timestamp */
        rval = NetMgr_startNTP();

        if (rval < 0) {
            NetMgr.error = true;
        }
        break;

    case NetMgr_State_ConnectCloud:
        /* connect to Azure */
        if (platform_init() != 0) {
            nb = snprintf(buf, sizeof(buf),
                    "Failed to initialize the platform.\r\r\r\n");
            DBG_write(buf, nb);
            goto error_connect;
        }

        if (serializer_init(NULL) != SERIALIZER_OK) {
            nb = snprintf(buf, sizeof(buf), "Failed on serializer_init\r\r\r\n");
            DBG_write(buf, nb);
            goto error_connect;
        }

        NetMgr.iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(
                connectionString, HTTP_Protocol);

        if (NetMgr.iotHubClientHandle == NULL) {
            nb = snprintf(buf, sizeof(buf),
                    "Failed on IoTHubClient_LL_Create\r\r\r\n");
            DBG_write(buf, nb);
            goto error_connect;
        }

        if (IoTHubClient_LL_SetOption(NetMgr.iotHubClientHandle,
            "MinimumPollingTime", &minimumPollingTime) != IOTHUB_CLIENT_OK) {
            nb = snprintf(buf, sizeof(buf), "failure to set option \"MinimumPollingTime\"\r\r\r\n");
            DBG_write(buf, nb);
            goto error_connect;
        }

        NetMgr.myData = CREATE_MODEL_INSTANCE(DEMO, SensorData);
        if (NetMgr.myData == NULL) {
            nb = snprintf(buf, sizeof(buf), "Failed on CREATE_MODEL_INSTANCE\r\r\r\n");
            DBG_write(buf, nb);
            goto error_connect;
        }

        if (IoTHubClient_LL_SetMessageCallback(NetMgr.iotHubClientHandle,
                IoTHubMessage, NetMgr.myData) != IOTHUB_CLIENT_OK) {
            nb = snprintf(buf, sizeof(buf),
                    "failed to call IoTHubClient_SetMessageCallback\r\r\r\n");
            DBG_write(buf, nb);
            goto error_connect;
        }

        /* Mark ourself as started once we have connected to the cloud at least once */
        if (!NetMgr.started) {
            NetMgr.started = true;
        }

error_connect:
        if (rval < 0) {
            NetMgr.error = true;
        }

        break;

    case NetMgr_State_Ready:
        break;

    case NetMgr_State_Publish:

        /* send queued up data samples */
        rval = NetMgr_publish();
        if (rval < 0) {
            NetMgr.error = true;
        }

        break;

    case NetMgr_State_DisconnectCloud:
        if (NetMgr.myData) {
            DESTROY_MODEL_INSTANCE(NetMgr.myData);
        }
        if (NetMgr.iotHubClientHandle) {
            IoTHubClient_LL_Destroy(NetMgr.iotHubClientHandle);
        }

        serializer_deinit();
        platform_deinit();
        break;

    case NetMgr_State_DisconnectAP:
        NWP_shutdown();
        break;

    case NetMgr_State_WaitForData:
        nb = snprintf(buf, sizeof(buf), "NetMgr: waiting for work\r\r\r\n");
        DBG_write(buf, nb);

        /* Enable LPDS while waiting for work to do */
        Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);

        sem_wait(&NetMgr.workSem);

        /* Disable LPDS */
        Power_setConstraint(PowerCC32XX_DISALLOW_LPDS);
        break;

    case NetMgr_State_Error:
        /* spin here for now */
        while (true) {
            sleep(1);
        }
        //break;

    case NetMgr_State_ErrorConnectCloud:
        if (NetMgr.myData) {
            DESTROY_MODEL_INSTANCE(NetMgr.myData);
        }
        if (NetMgr.iotHubClientHandle) {
            IoTHubClient_LL_Destroy(NetMgr.iotHubClientHandle);
        }

        serializer_deinit();
        platform_deinit();
        break;

    } /* switch */

    return (rval);
}

/*
 *  ======== NetMgr_sendData ========
 */
void NetMgr_sendData(DataPacket *dpp)
{
    if (!isFull(&NetMgr.workMq)) {
        mq_send(NetMgr.workMq, (char *)dpp, sizeof(DataPacket), 0);

        sem_post(&NetMgr.workSem);
    }
    else {
        /* discard data if queue is full */
    }
}

/*
 *  ======== NetMgr_error ========
 */
bool NetMgr_error(void)
{
    return ((NetMgr.state == NetMgr_State_Error) || NetMgr.error);
}

/*
 *  ======== NetMgr_initialize ========
 */
int NetMgr_initialize(void)
{
    pthread_attr_t pthreadAttrs;
    pthread_t thread;
    struct sched_param schedParam;
    int status;
    int nb;
    char buf[64];
    struct mq_attr mqAttr;

    /*
     * Disable LPDS to minimize context save/restore in and out of sleep
     * while calling SL APIs
     */
    Power_setConstraint(PowerCC32XX_DISALLOW_LPDS);

    NWP_initialize();

    /* create the work queue */
    mqAttr.mq_flags = 0;
    mqAttr.mq_maxmsg = MAXMSGS;
    mqAttr.mq_msgsize = sizeof(DataPacket);
    NetMgr.workMq = mq_open("WorkQueue", O_CREAT, 0, &mqAttr);

    /* construct the wifi semaphore */
    if (0 != sem_init(&NetMgr.workSem, 0, 0)) {
        nb = snprintf(buf, sizeof(buf), "NetMgr: sem_init failed!\r\r\r\n");
        DBG_write(buf, nb);
        while(1);
    }

    /* construct the wifi thread */
    pthread_attr_init(&pthreadAttrs);

    status = pthread_attr_setstacksize(&pthreadAttrs, NETMGR_TSKSTKSIZE);
    if (status != 0) {
        nb = snprintf(buf, sizeof(buf),
                "NetMgr: Error setting stack size\r\r\r\n");
        DBG_write(buf, nb);
        while(1);
    }

    status = pthread_create(&thread, &pthreadAttrs, NetMgr_threadFxn, NULL);
    if (status != 0) {
        nb = snprintf(buf, sizeof(buf),
                "NetMgr: Failed to create thread\r\r\r\n");
        DBG_write(buf, nb);
        while(1);
    }

    schedParam.sched_priority = 1;
    status = pthread_attr_setschedparam(&pthreadAttrs, &schedParam);
    if (status != 0) {
        nb = snprintf(buf, sizeof(buf),
                "NetMgr: Error setting scheduling parameter\r\r\r\n");
        DBG_write(buf, nb);
        while(1);
    }

    return (0);
}

/*
 *  ======== NetMgr_publish ========
 */
static int NetMgr_publish(void)
{
    int rval = 0;
    int i;
    DataPacket packet;
    int nb;
    char buf[64];
    unsigned char *destination = NULL;
    size_t destinationSize;
    IOTHUB_CLIENT_STATUS status;
    IOTHUB_MESSAGE_HANDLE messageHandle = NULL;

    nb = snprintf(buf, sizeof(buf), "NetMgr: publish data\r\r\r\n");
    DBG_write(buf, nb);

    while (!isEmpty(&NetMgr.workMq)) {

        /* get next data message */
        mq_receive(NetMgr.workMq, (char *)&packet, sizeof(DataPacket), NULL);

        /* object temperature */
        for (i = 0; i < packet.count; i++) {
            nb = snprintf(buf, sizeof(buf), "%u", packet.ts[i]);

            /* publish the data object */
            NetMgr.myData->deviceId = "myFirstDevice";
            NetMgr.myData->ts = buf;
            NetMgr.myData->temperature = packet.temp[i].die;

            if (SERIALIZE(&destination, &destinationSize, NetMgr.myData->deviceId, NetMgr.myData->ts, NetMgr.myData->temperature) != CODEFIRST_OK) {
                nb = snprintf(buf, sizeof(buf), "Failed to serialize\r\r\r\n");
                DBG_write(buf, nb);
                rval = -1;
                goto on_error;
            }
            else {
                messageHandle = IoTHubMessage_CreateFromByteArray(destination, destinationSize);
                if (messageHandle == NULL) {
                    nb = snprintf(buf, sizeof(buf), "unable to create a new IoTHubMessage\r\r\r\n");
                    DBG_write(buf, nb);
                    rval = -1;
                    goto on_error;
                }
                else {
                    if (IoTHubClient_LL_SendEventAsync(NetMgr.iotHubClientHandle, messageHandle, sendCallback, (void*)NetMgr.messageId++) != IOTHUB_CLIENT_OK) {
                        nb = snprintf(buf, sizeof(buf), "failed to hand over the message to IoTHubClient");
                        DBG_write(buf, nb);
                        rval = -1;
                        goto on_error;
                    }

                    IoTHubMessage_Destroy(messageHandle);
                    messageHandle = NULL;
                }
                free(destination);
                destination = NULL;
            }
        }

        /* Ingress */
        while ((IoTHubClient_LL_GetSendStatus(NetMgr.iotHubClientHandle, &status)
                == IOTHUB_CLIENT_OK) && (status ==
                IOTHUB_CLIENT_SEND_STATUS_BUSY)) {
            IoTHubClient_LL_DoWork(NetMgr.iotHubClientHandle);
            sleep(1);
        }

on_error:
        if (rval != 0) {
            if (messageHandle) {
                IoTHubMessage_Destroy(messageHandle);
            }
            if (destination) {
                free(destination);
            }
            break;
        }
    }

    return (rval);
}

/*
 *  ======== NetMgr_ready ========
 */
bool NetMgr_ready(void)
{
    return (NetMgr.started);
}

/*
 *  ======== NetMgr_startNTP ========
 */
static int NetMgr_startNTP(void)
{
    int rval = 0;

    startNTP();

    return (rval);
}

/*
 *  ======== NetMgr_threadFxn ========
 */
void *NetMgr_threadFxn(void *arg0)
{
    /* main loop */
    while (true) {

        /* execute the state actions */
        NetMgr_executeState();

        /* execute the transition for the next input event */
        NetMgr_transition();
    }
}
