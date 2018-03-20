/*
 * Copyright (c) 2017 Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== NWP.c ========
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <ti/net/tls.h>

#include <ti/drivers/GPIO.h>

/* SimpleLink Wi-Fi Host Driver Header files */
#include <ti/drivers/net/wifi/netcfg.h>
#include <ti/drivers/net/wifi/simplelink.h>

/* Example/Board Header file */
#include "Board.h"
#include "certs.h"
#include "DBG.h"

#include "NWP.h"

#define AZURE_IOT_ROOT_CA_FILENAME "/cert/ms.der"

#include "wificonfig.h"

static volatile bool deviceConnected = false;
static volatile bool ipAcquired = false;

static _u8 NWP_macAddrBuf[SL_MAC_ADDR_LEN];

/* module object */
typedef struct {
    _u8        *macAddr;
} NWP_Object;

static NWP_Object NWP = {
    .macAddr = NWP_macAddrBuf
};

static void socketsStartUp(void);
static void socketsShutDown(void);
static int getMAC(_u16 *len, _u8 *macAddr);

/*
 * The following macro is disabled by default. This is done to prevent the
 * certificate files from being written to flash every time the program
 * is run.  If an update to the cert files are needed, just update the
 * corresponding arrays, and rebuild with this macro defined. Note
 * you must remember to disable it otherwise the files will keep being
 * overwritten each time.
 */
#ifdef OVERWRITE_CERTS
static bool overwriteCerts = true;
#else
static bool overwriteCerts = false;
#endif

/*
 *  ======== flashCerts ========
 *  Utility function to flash the contents of a buffer (PEM format) into the
 *  filename/path specified by certName (DER format)
 */
static void flashCerts(uint8_t *certName, uint8_t *buffer, uint32_t bufflen)
{
    int status = 0;
    int16_t slStatus = 0;
    SlFsFileInfo_t fsFileInfo;
    char buf[128];
    int nb;

    /* Check if the cert file already exists */
    slStatus = sl_FsGetInfo(certName, 0, &fsFileInfo);

    /* If the cert doesn't exist, write it (or overwrite if specified to) */
    if (slStatus == SL_ERROR_FS_FILE_NOT_EXISTS || overwriteCerts == true) {
        nb = snprintf(buf, sizeof(buf), "Flashing certificate file ...\r\n");
        DBG_write(buf, nb);

        /* Convert the cert to DER format and write to flash */
        status = TLS_writeDerFile(buffer, bufflen, TLS_CERT_FORMAT_PEM,
                (const char *)certName);

        if (status != 0) {
            nb = snprintf(buf, sizeof(buf),
                    "Error: Could not write file %s to flash (%d)\r\n",
                    certName, status);
            DBG_write(buf, nb);
            while(1);
        }
        nb = snprintf(buf, sizeof(buf), " successfully wrote file %s to flash\r\n", certName);
        DBG_write(buf, nb);
    }
}

/*
 *  ======== NWP_initialize ========
 */
int NWP_initialize(void)
{
    return (0);
}

/*
 *  ======== NWP_macAddr ========
 */
_u8 *NWP_macAddr(void)
{
    return (NWP.macAddr);
}

/*
 *  ======== NWP_shutdown ========
 */
int NWP_shutdown(void)
{
    int rval = 0;

    socketsShutDown();

    return (rval);
}

/*
 *  ======== NWP_startup ========
 */
int NWP_startup(void)
{
    int rval = 0;
    static bool firstTime = true;
    _u16 macAddrLen = SL_MAC_ADDR_LEN;
    int nb;
    char buf[64];

    /* initialize SimpleLink */
    socketsStartUp();

    if (firstTime) {
        /* Flash Certificate Files */
        flashCerts((uint8_t *)AZURE_IOT_ROOT_CA_FILENAME, (uint8_t *)certificates,
                strlen(certificates));
        firstTime = false;
    }

    /* use the MAC address as the unique client ID */
    rval = getMAC(&macAddrLen, NWP.macAddr);

    if (rval < 0) {
        nb = snprintf(buf, sizeof(buf),
                "failed to read MAC address\r\n");
        DBG_write(buf, nb);
        goto on_error;
    }

on_error:
    return (rval);
}

/*
 *  ======== SimpleLinkGeneralEventHandler ========
 *  SimpleLink Host Driver callback for general device errors & events.
 */
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    int nb;
    char buf[64];

    nb = snprintf(buf, sizeof(buf),
            "General event occurred, Event ID: %x\r\n", pDevEvent->Id);
    DBG_write(buf, nb);
}

/*
 *  ======== SimpleLinkHttpServerEventHandler ========
 *  SimpleLink Host Driver callback for HTTP server events.
 */
void SimpleLinkHttpServerEventHandler(
        SlNetAppHttpServerEvent_t *pHttpEvent,
        SlNetAppHttpServerResponse_t *pHttpResponse)
{
    switch (pHttpEvent->Event) {
        case SL_NETAPP_EVENT_HTTP_TOKEN_GET:
        case SL_NETAPP_EVENT_HTTP_TOKEN_POST:
        default:
            break;
    }
}

/*
 *  ======== SimpleLinkNetAppEventHandler ========
 *  SimpleLink Host Driver callback for asynchronous IP address events.
 */
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pArgs)
{
    switch (pArgs->Id) {
        case SL_NETAPP_EVENT_IPV4_ACQUIRED:
            ipAcquired = true;
            break;

        default:
            break;
    }
}

/*
 *  ======== SimpleLinkSockEventHandler ========
 *  SimpleLink Host Driver callback for socket event indication.
 */
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    switch(pSock->Event) {
        case SL_SOCKET_TX_FAILED_EVENT:
        default:
            break;
    }
}

/*
 *  ======== SimpleLinkNetAppRequestEventHandler ========
 */
void SimpleLinkNetAppRequestEventHandler(SlNetAppRequest_t *pNetAppRequest,
        SlNetAppResponse_t *pNetAppResponse)
{

}

/*
 *  ======== SimpleLinkNetAppRequestMemFreeEventHandler ========
 */
void SimpleLinkNetAppRequestMemFreeEventHandler(uint8_t *buffer)
{

}

/*
 *  ======== SimpleLinkWlanEventHandler ========
 *  SimpleLink Host Driver callback for handling WLAN connection or
 *  disconnection events.
 */
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pArgs)
{
    switch (pArgs->Id) {
        case SL_WLAN_EVENT_CONNECT:
            deviceConnected = true;
            break;

        case SL_WLAN_EVENT_DISCONNECT:
            deviceConnected = false;
            break;

        default:
            break;
    }
}

/*
 *  ======== SimpleLinkFatalErrorEventHandler ========
 *  SimpleLink Host Driver callback for handling fatal errors
 */
void SimpleLinkFatalErrorEventHandler(SlDeviceFatal_t *slFatalErrorEvent)
{
    char buf[128];
    int nb;

    switch (slFatalErrorEvent->Id)
    {
        case SL_DEVICE_EVENT_FATAL_DEVICE_ABORT:
            nb = snprintf(buf, sizeof(buf),
                    "[ERROR] - FATAL ERROR: Abort NWP event detected: "
                    "AbortType=%d, AbortData=0x%x\r\n",
                    slFatalErrorEvent->Data.DeviceAssert.Code,
                    slFatalErrorEvent->Data.DeviceAssert.Value);
            DBG_write(buf, nb);
            break;

        case SL_DEVICE_EVENT_FATAL_DRIVER_ABORT:
            nb = snprintf(buf, sizeof(buf),
                    "[ERROR] - FATAL ERROR: Driver Abort detected.\r\n");
            DBG_write(buf, nb);
            break;

        case SL_DEVICE_EVENT_FATAL_NO_CMD_ACK:
            nb = snprintf(buf, sizeof(buf),
                    "[ERROR] - FATAL ERROR: No Cmd Ack detected "
                    "[cmd opcode = 0x%x]\r\n",
                    slFatalErrorEvent->Data.NoCmdAck.Code);
            DBG_write(buf, nb);
            break;

        case SL_DEVICE_EVENT_FATAL_SYNC_LOSS:
            nb = snprintf(buf, sizeof(buf),
                    "[ERROR] - FATAL ERROR: Sync loss detected\r\n");
            DBG_write(buf, nb);
            break;

        case SL_DEVICE_EVENT_FATAL_CMD_TIMEOUT:
            nb = snprintf(buf, sizeof(buf),
                    "[ERROR] - FATAL ERROR: Async event timeout detected "
                    "[event opcode =0x%x]\r\n",
                    slFatalErrorEvent->Data.CmdTimeout.Code);
            DBG_write(buf, nb);
            break;

        default:
            nb = snprintf(buf, sizeof(buf),
                    "[ERROR] - FATAL ERROR: Unspecified error detected\r\n");
            DBG_write(buf, nb);
            break;
    }
}

/*
 *  ======== socketsShutDown ========
 *  Generic routine, defined to close down the WiFi in this case.
 */
static void socketsShutDown(void)
{
    sl_Stop(200);
}

/*
 *  ======== wlanConnect =======
 *  Secure connection parameters
 */
static int wlanConnect()
{
    SlWlanSecParams_t secParams = {0};
    int rval;

    secParams.Type = SECURITY_TYPE;
    secParams.Key = (signed char *)SECURITY_KEY;
    secParams.KeyLen = strlen((const char *)secParams.Key);

    rval = sl_WlanConnect((signed char*)SSID, strlen((const char *)SSID),
            NULL, &secParams, NULL);

    if (rval == 0) {
        sl_WlanProfileAdd((signed char *)SSID, strlen((const char *)SSID), 0,
                &secParams, 0, 6, 0);
    }

    return (rval);
}

/*
 *  ======== socketsStartUp ========
 *  Generic routine, in this case defined to open the WiFi and await a
 *  connection, using Smart Config if the appropriate button is pressed.
 */
static void socketsStartUp(void)
{
    SlNetCfgIpV4Args_t  ipV4;
    uint16_t            len = sizeof(ipV4);
    int                 rval;
    uint16_t            dhcpBits;

    int nb;
    char buf[64];

    /* start the network processor */
    rval = sl_Start(NULL, NULL, NULL);

    if (rval < 0) {
        nb = snprintf(buf, sizeof(buf),
                "Could not intialize SimpleLink Wi-Fi\r\n");
        DBG_write(buf, nb);
        while(1);
    }

    /* set to station mode if needed */
    if (rval != ROLE_STA) {
        sl_WlanSetMode(ROLE_STA);
        sl_Stop(200);

        rval = sl_Start(NULL, NULL, NULL);

        if (rval != ROLE_STA) {
            nb = snprintf(buf, sizeof(buf),
                    "Failed to set SimpleLink Wi-Fi to Station mode\r\n");
            DBG_write(buf, nb);
            while(1);
        }
    }

    /* enable DHCP client */
    rval = sl_NetCfgSet(SL_NETCFG_IPV4_STA_ADDR_MODE, SL_NETCFG_ADDR_DHCP, 0, 0);

    if (rval < 0) {
        nb = snprintf(buf, sizeof(buf),
                "Could not enable DHCP client\r\n");
        DBG_write(buf, nb);
        while(1);
    }

    /* enable auto connect and fast connect */
    rval = sl_WlanPolicySet(SL_WLAN_POLICY_CONNECTION,
            SL_WLAN_CONNECTION_POLICY(1, 1, 0, 0), NULL, 0);

    if (rval < 0) {
        nb = snprintf(buf, sizeof(buf),
                "Failed to set connection policy to auto\r\n");
        DBG_write(buf, nb);
        while(1);
    }

    sl_Stop(200);

    rval = sl_Start(NULL, NULL, NULL);
    if (rval < 0) {
        nb = snprintf(buf, sizeof(buf),
                "Could not intialize SimpleLink Wi-Fi\r\n");
        DBG_write(buf, nb);
        while(1);
    }

    /* set connection variables to initial values */
    deviceConnected = false;
    ipAcquired = false;

    if (wlanConnect() < 0) {
        nb = snprintf(buf, sizeof(buf),
                "Could not connect to WiFi AP\r\n");
        DBG_write(buf, nb);
        while(1);
    }

    /*
     * Wait for SimpleLink to connect to an AP. If it fails to connect,
     * double-check your settings in wificonfig.h.
     */
    while ((deviceConnected != true) || (ipAcquired != true)) {
        usleep(50000);
    }

    /* set normal power policy */
    sl_WlanPolicySet(SL_WLAN_POLICY_PM , SL_WLAN_NORMAL_POLICY, NULL, 0);

    /* print IP address */
    sl_NetCfgGet(SL_NETCFG_IPV4_STA_ADDR_MODE, &dhcpBits, &len,
            (unsigned char *)&ipV4);

    nb = snprintf(buf, sizeof(buf), "Connected to access point\r\n");
    DBG_write(buf, nb);

    nb = snprintf(buf, sizeof(buf), "IP Address: %d.%d.%d.%d\r\n",
            SL_IPV4_BYTE(ipV4.Ip, 3), SL_IPV4_BYTE(ipV4.Ip, 2),
            SL_IPV4_BYTE(ipV4.Ip, 1), SL_IPV4_BYTE(ipV4.Ip, 0));
    DBG_write(buf, nb);

    nb = snprintf(buf, sizeof(buf), "\r\n");
    DBG_write(buf, nb);

    return;
}

/*
 *  ======== getMAC ========
 */
static int getMAC(_u16 *len, _u8 *buf)
{
    _i32 rval;
    _u16 configOpt = 0;

    rval = sl_NetCfgGet(SL_NETCFG_MAC_ADDRESS_GET, &configOpt, len, buf);

    return (rval == 0 ? 0 : -1);
}

/*
 *  ======== NWP_wlanConnect ========
 */
void NWP_wlanConnect(void)
{
    _u8 policy;
    int response;
    int nb;
    char buf[64];

    /* set connection variables to initial values */
    deviceConnected = false;
    ipAcquired = false;

    /* enable auto connect and fast connect */
    policy = SL_WLAN_CONNECTION_POLICY(1, 1, 0, 0);
    response = sl_WlanPolicySet(SL_WLAN_POLICY_CONNECTION, policy, NULL, 0);

    if (response < 0) {
        nb = snprintf(buf, sizeof(buf),
                "Failed to set connection policy to auto\r\n");
        DBG_write(buf, nb);
        while(1);
    }

    while ((deviceConnected != true) || (ipAcquired != true)) {
        usleep(50000);
    }
}

/*
 *  ======== NWP_wlanDisconnect ========
 */
void NWP_wlanDisconnect(void)
{
    _u8 policy;

    /* disable auto connect and fast connect */
    policy = SL_WLAN_CONNECTION_POLICY(0, 0, 0, 0);
    sl_WlanPolicySet(SL_WLAN_POLICY_CONNECTION, policy, 0, 0);
    usleep(500000);

    /* disconnect from the access point */
    sl_WlanDisconnect();

    while (deviceConnected) {
        usleep(10000);
    }
}
