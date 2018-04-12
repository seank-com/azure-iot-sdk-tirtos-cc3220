// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>

#include "iothub_client.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/string_tokenizer.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/http_proxy_io.h"

#include "iothubtransportmqtt.h"
#include "iothub_client_hsm_ll.h"
#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_security_factory.h"

#include "azure_prov_client/prov_transport_mqtt_client.h"

#include "../../../certs/certs.h"

#include "iothub_client_version.h"

DEFINE_ENUM_STRINGS(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);
DEFINE_ENUM_STRINGS(PROV_DEVICE_REG_STATUS, PROV_DEVICE_REG_STATUS_VALUES);

static const char* global_prov_uri = "global.azure-devices-provisioning.net";
static const char* id_scope = "[scopedId]";

static bool g_trace_on = true;

#define MESSAGES_TO_SEND            2
#define TIME_BETWEEN_MESSAGES       2

typedef struct CLIENT_SAMPLE_INFO_TAG
{
    unsigned int sleep_time;
    char* iothub_uri;
    char* access_key_name;
    char* device_key;
    char* device_id;
    int registration_complete;
} CLIENT_SAMPLE_INFO;

typedef struct IOTHUB_CLIENT_SAMPLE_INFO_TAG
{
    int connected;
    int stop_running;
} IOTHUB_CLIENT_SAMPLE_INFO;

static IOTHUBMESSAGE_DISPOSITION_RESULT receive_msg_callback(IOTHUB_MESSAGE_HANDLE message, void* user_context)
{
    (void)message;
    IOTHUB_CLIENT_SAMPLE_INFO* iothub_info = (IOTHUB_CLIENT_SAMPLE_INFO*)user_context;
    (void)printf("Stop message recieved from IoTHub\r\n");
    iothub_info->stop_running = 1;
    return IOTHUBMESSAGE_ACCEPTED;
}

static void registation_status_callback(PROV_DEVICE_REG_STATUS reg_status, void* user_context)
{
    if (user_context == NULL)
    {
        printf("user_context is NULL\r\n");
    }
    else
    {
        //CLIENT_SAMPLE_INFO* user_ctx = (CLIENT_SAMPLE_INFO*)user_context;
        (void)printf("Provisioning Status: %s\r\n", ENUM_TO_STRING(PROV_DEVICE_REG_STATUS, reg_status));
        if (reg_status == PROV_DEVICE_REG_STATUS_CONNECTED)
        {
        }
        else if (reg_status == PROV_DEVICE_REG_STATUS_REGISTERING)
        {
        }
        else if (reg_status == PROV_DEVICE_REG_STATUS_ASSIGNING)
        {
        }
    }
}

static void iothub_connection_status(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    (void)reason;
    if (user_context == NULL)
    {
        printf("iothub_connection_status user_context is NULL\r\n");
    }
    else
    {
        IOTHUB_CLIENT_SAMPLE_INFO* iothub_info = (IOTHUB_CLIENT_SAMPLE_INFO*)user_context;
        if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
        {
            iothub_info->connected = 1;
        }
        else
        {
            iothub_info->connected = 0;
            iothub_info->stop_running = 1;
        }
    }
}

static void register_device_callback(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context)
{
    if (user_context == NULL)
    {
        printf("user_context is NULL\r\n");
    }
    else
    {
        CLIENT_SAMPLE_INFO* user_ctx = (CLIENT_SAMPLE_INFO*)user_context;
        if (register_result == PROV_DEVICE_RESULT_OK)
        {
            (void)printf("Registration Information received from service: %s!\r\n", iothub_uri);
            (void)mallocAndStrcpy_s(&user_ctx->iothub_uri, iothub_uri);
            (void)mallocAndStrcpy_s(&user_ctx->device_id, device_id);
            user_ctx->registration_complete = 1;
        }
        else
        {
            (void)printf("Failure encountered on registration!\r\n");
            user_ctx->registration_complete = 2;
        }
    }
}

void prov_dev_client_ll_run(void)
{
    SECURE_DEVICE_TYPE hsm_type;
    hsm_type = SECURE_DEVICE_TYPE_X509;

    if (platform_init() != 0)
    {
        (void)printf("platform_init failed\r\n");
    }
    else if (prov_dev_security_init(hsm_type) != 0)
    {
        (void)printf("prov_dev_security_init failed\r\n");
    }
    else
    {
        IOTHUB_CLIENT_TRANSPORT_PROVIDER iothub_transport;
        PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION g_prov_transport;
        HTTP_PROXY_OPTIONS http_proxy;
        CLIENT_SAMPLE_INFO user_ctx;

        memset(&http_proxy, 0, sizeof(HTTP_PROXY_OPTIONS));
        memset(&user_ctx, 0, sizeof(CLIENT_SAMPLE_INFO));

        g_prov_transport = Prov_Device_MQTT_Protocol;
        iothub_transport = MQTT_Protocol;

        // Set ini
        user_ctx.registration_complete = 0;
        user_ctx.sleep_time = 10;

        printf("Provisioning API Version: foobar\r\n");
        printf("Iothub API Version: foobar\r\n");

        PROV_DEVICE_LL_HANDLE handle;
        if ((handle = Prov_Device_LL_Create(global_prov_uri, id_scope, g_prov_transport)) == NULL)
        {
            (void)printf("failed calling Prov_Device_LL_Create\r\n");
        }
        else
        {
            (void)printf("Prov_Device_LL_Create success!\r\n");

            Prov_Device_LL_SetOption(handle, "logtrace", &g_trace_on);

            if (Prov_Device_LL_Register_Device(handle, register_device_callback, &user_ctx, registation_status_callback, &user_ctx) != PROV_DEVICE_RESULT_OK)
            {
                (void)printf("failed calling Prov_Device_LL_Register_Device\r\n");
            }
            else
            {
                (void)printf("Prov_Device_LL_Register_Device success\r\n");
                do
                {
                    Prov_Device_LL_DoWork(handle);
                    ThreadAPI_Sleep(user_ctx.sleep_time);
                } while (user_ctx.registration_complete == 0);
            }
            Prov_Device_LL_Destroy(handle);
        }

        if (user_ctx.registration_complete != 1)
        {
            (void)printf("registration failed!\r\n");
        }
        else
        {
            // Sending the iothub messages
            IOTHUB_CLIENT_LL_HANDLE iothub_client;
            if ((iothub_client = IoTHubClient_LL_CreateFromDeviceAuth(user_ctx.iothub_uri, user_ctx.device_id, iothub_transport)) == NULL)
            {
                (void)printf("failed create IoTHub client from connection string %s!\r\n", user_ctx.iothub_uri);
            }
            else
            {
                (void)printf("IoTHubClient_LL_CreateFromDeviceAuth success\r\n");

                IOTHUB_CLIENT_SAMPLE_INFO iothub_info;
                TICK_COUNTER_HANDLE tick_counter_handle = tickcounter_create();
                tickcounter_ms_t current_tick;
                tickcounter_ms_t last_send_time = 0;
                size_t msg_count = 0;
                iothub_info.stop_running = 0;
                iothub_info.connected = 0;

                (void)IoTHubClient_LL_SetConnectionStatusCallback(iothub_client, iothub_connection_status, &iothub_info);

                IoTHubClient_LL_SetOption(iothub_client, "logtrace", &g_trace_on);

                if (IoTHubClient_LL_SetMessageCallback(iothub_client, receive_msg_callback, &iothub_info) != IOTHUB_CLIENT_OK)
                {
                    (void)printf("ERROR: IoTHubClient_LL_SetMessageCallback..........FAILED!\r\n");
                }

                (void)printf("Sending 1 messages to IoTHub every %d seconds for %d messages (Send any message to stop)\r\n", TIME_BETWEEN_MESSAGES, MESSAGES_TO_SEND);
                do
                {
                    if (iothub_info.connected != 0)
                    {
                        // Send a message every TIME_BETWEEN_MESSAGES seconds
                        (void)tickcounter_get_current_ms(tick_counter_handle, &current_tick);
                        if ((current_tick - last_send_time) / 1000 > TIME_BETWEEN_MESSAGES)
                        {
                            static char msgText[1024];
                            sprintf_s(msgText, sizeof(msgText), "{ \"message_index\" : \"%zu\" }", msg_count++);

                            IOTHUB_MESSAGE_HANDLE msg_handle = IoTHubMessage_CreateFromByteArray((const unsigned char*)msgText, strlen(msgText));
                            if (msg_handle == NULL)
                            {
                                (void)printf("ERROR: iotHubMessageHandle is NULL!\r\n");
                            }
                            else
                            {
                                if (IoTHubClient_LL_SendEventAsync(iothub_client, msg_handle, NULL, NULL) != IOTHUB_CLIENT_OK)
                                {
                                    (void)printf("ERROR: IoTHubClient_LL_SendEventAsync..........FAILED!\r\n");
                                }
                                else
                                {
                                    (void)tickcounter_get_current_ms(tick_counter_handle, &last_send_time);
                                    (void)printf("IoTHubClient_LL_SendEventAsync accepted message [%zu] for transmission to IoT Hub.\r\n", msg_count);

                                }
                                IoTHubMessage_Destroy(msg_handle);
                            }
                        }
                    }
                    IoTHubClient_LL_DoWork(iothub_client);
                    ThreadAPI_Sleep(1);
                } while (iothub_info.stop_running == 0 && msg_count < MESSAGES_TO_SEND);

                size_t index = 0;
                for (index = 0; index < 10; index++)
                {
                    IoTHubClient_LL_DoWork(iothub_client);
                    ThreadAPI_Sleep(1);
                }
                tickcounter_destroy(tick_counter_handle);
                IoTHubClient_LL_Destroy(iothub_client);
            }
        }
        free(user_ctx.iothub_uri);
        free(user_ctx.device_id);
        prov_dev_security_deinit();
        platform_deinit();
    }
}
