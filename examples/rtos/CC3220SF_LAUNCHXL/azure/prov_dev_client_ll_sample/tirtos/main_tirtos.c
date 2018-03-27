/*
 * Copyright (c) 2017, Texas Instruments Incorporated
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
 *  ======== main_tirtos.c ========
 */
#include <string.h>

#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>

#include <ti/drivers/GPIO.h>

#include <prov_dev_client_ll_sample.h>

#include "Board.h"
#include "UARTUtils.h"

#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/net/tls.h>

#include <pthread.h>

extern const char certificates[];
extern const char x509certificate[];
extern const char x509privateKey[];

#define AZURE_IOT_ROOT_CA_FILENAME "/cert/ms.der"
#define AZURE_IOT_X509_FILENAME "/cert/cert.der"
#define AZURE_IOT_X509_PRIVATE_KEY_FILENAME "/cert/key.der"

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

extern void NetWiFi_init();

/*
 *  ======== flashCerts ========
 *  Utility function to flash the contents of a buffer (PEM format) into the
 *  filename/path specified by certName (DER format)
 */
void flashCerts(uint8_t *certName, uint8_t *buffer, uint32_t bufflen)
{
    int status = 0;
    int16_t slStatus = 0;
    SlFsFileInfo_t fsFileInfo;

    /* Check if the cert file already exists */
    slStatus = sl_FsGetInfo(certName, 0, &fsFileInfo);

    /* If the cert doesn't exist, write it (or overwrite if specified to) */
    if (slStatus == SL_ERROR_FS_FILE_NOT_EXISTS || overwriteCerts == true) {

        printf("Flashing certificate file ...");

        /* Convert the cert to DER format and write to flash */
        status = TLS_writeDerFile(buffer, bufflen, TLS_CERT_FORMAT_PEM,
                (const char *)certName);

        if (status != 0) {
            printf("Error: Could not write file %s to flash (%d)\n",
                    certName, status);
            while(1);
        }
        printf("successfully wrote file %s to flash\n", certName);
    }
}

/*
 *  ======== azureThreadFxn ========
 */
void *azureThreadFxn(void *arg0)
{
    /*
     *  Add the UART device to the system.
     *  All UART peripherals must be setup and the module must be initialized
     *  before opening.  This is done by Board_initUART().  The functions used
     *  are implemented in UARTUtils.c.
     */
    add_device("UART", _MSA, UARTUtils_deviceopen,
               UARTUtils_deviceclose, UARTUtils_deviceread,
               UARTUtils_devicewrite, UARTUtils_devicelseek,
               UARTUtils_deviceunlink, UARTUtils_devicerename);

    /* Open UART0 for writing to stdout and set buffer */
    freopen("UART:0", "w", stdout);
    setvbuf(stdout, NULL, _IOLBF, 128);

    /* Open UART0 for writing to stderr and set buffer */
    freopen("UART:0", "w", stderr);
    setvbuf(stderr, NULL, _IOLBF, 128);

    /* Open UART0 for reading from stdin and set buffer */
    freopen("UART:0", "r", stdin);
    setvbuf(stdin, NULL, _IOLBF, 128);

    printf("Starting the prov_dev_client_ll_run example\n");

    /* Initialize network connection */
    NetWiFi_init();

    /* Flash Certificate Files */
    flashCerts((uint8_t *)AZURE_IOT_ROOT_CA_FILENAME, (uint8_t *)certificates,
            strlen(certificates));

    flashCerts((uint8_t *)AZURE_IOT_X509_FILENAME, (uint8_t *)x509certificate,
            strlen(x509certificate));

    flashCerts((uint8_t *)AZURE_IOT_X509_PRIVATE_KEY_FILENAME, (uint8_t *)x509privateKey,
            strlen(x509privateKey));
            
    prov_dev_client_ll_run();

    return (NULL);
}

/*
 *  ======== main ========
 */
int main(int argc, char *argv[])
{
    pthread_attr_t pthreadAttrs;
    pthread_t slThread;
    pthread_t azureThread;
    int status;

    Board_initGeneral();
    GPIO_init();
    UART_init();
    SPI_init();

    GPIO_write(Board_LED0, Board_LED_ON);

    /* Create the sl_Task thread */
    pthread_attr_init(&pthreadAttrs);

    status = pthread_attr_setstacksize(&pthreadAttrs, 2048);
    if (status != 0) {
        System_abort("main: failed to set stack size\n");
    }

    status = pthread_create(&slThread, &pthreadAttrs, sl_Task, NULL);
    if (status != 0) {
        System_abort("main: Failed to create sl_Task thread\n");
    }

    /* Create the AZURE thread */
    status = pthread_attr_setstacksize(&pthreadAttrs, 4096);
    if (status != 0) {
        System_abort("main: Error setting stack size\n");
    }

    status = pthread_create(&azureThread, &pthreadAttrs, azureThreadFxn, NULL);
    if (status != 0) {
        System_abort("main: Failed to create Azure thread!\n");
    }

    pthread_attr_destroy(&pthreadAttrs);

    BIOS_start();
}
