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
#include <pthread.h>

#include <ti/sysbios/BIOS.h>

#include <ti/display/Display.h>
#include <ti/drivers/GPIO.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC32XX.h>

#include <ti/drivers/net/wifi/simplelink.h>

#include <ti/sail/tmp006/tmp006.h>
#include <ti/sail/button/button.h>
#include <ti/sail/led/led.h>

#include "Board.h"

#include "DBG.h"
#include "PowerHooks.h"

#define SL_TASK_STACK_SIZE      2048
#define MAIN_THREAD_STACK_SIZE  4000

extern void *mainThreadFxn(void *arg0);

extern PowerCC32XX_ModuleState PowerCC32XX_module;

/*
 *  ======== main ========
 */
int main(int argc, char *argv[])
{
    pthread_attr_t pthreadAttrs;
    pthread_t slThread;
    pthread_t mainThread;
    struct sched_param schedParam;
    int status;
    PowerCC32XX_Wakeup wakeupSettings;

    Board_initGeneral();
    I2C_init();
    GPIO_init();
    SPI_init();
    Display_init();

    /* initialize the peripheral drivers */
    Button_init();
    LED_init();
    TMP006_init();

    /* place constraint to prevent LDSP transition */
    Power_setConstraint(PowerCC32XX_DISALLOW_LPDS);

    PowerCC32XX_getWakeup(&wakeupSettings);
    wakeupSettings.wakeupGPIOFxnLPDS = PowerHooks_exitLPDS;
    wakeupSettings.wakeupGPIOFxnLPDSArg = 0;
    PowerCC32XX_configureWakeup(&wakeupSettings);

    /*
     *  When the power policy is enabled, the idle function will issue
     *  the WFI (Wait For Interrupt) instruction even when LPDS is
     *  disallowed. This cause the debugger to halt the processor.
     *  If desired, se the following line of code to prevent WFI
     *  and allow normal debug support, by disabling power management.
     */
    PowerCC32XX_module.enablePolicy = true; //false;

    /* Create the sl_Task thread */
    pthread_attr_init(&pthreadAttrs);

    status = pthread_attr_setstacksize(&pthreadAttrs, SL_TASK_STACK_SIZE);
    if (status != 0) {
        /* Error setting stack size */
        while (1);
    }

    status = pthread_create(&slThread, &pthreadAttrs, sl_Task, NULL);
    if (status != 0) {
        /* Failed to create sl_Task thread */
        while (1);
    }

    /* Create the main thread */
    status = pthread_attr_setstacksize(&pthreadAttrs, MAIN_THREAD_STACK_SIZE);
    if (status != 0) {
        /* Error setting stack size */
        while (1);
    }

    schedParam.sched_priority = 2;
    status = pthread_attr_setschedparam(&pthreadAttrs, &schedParam);
    if (status != 0) {
        /* Error setting scheduling parameter */
        while (1);
    }

    status = pthread_create(&mainThread, &pthreadAttrs, mainThreadFxn, NULL);
    if (status != 0) {
        /* Failed to create main thread! */
        while (1);
    }

    pthread_attr_destroy(&pthreadAttrs);

    BIOS_start();
}
