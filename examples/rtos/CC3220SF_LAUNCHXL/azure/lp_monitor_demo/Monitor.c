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
 *  ======== Monitor.c ========
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>

#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC32XX.h>

#include <ti/sail/tmp006/tmp006.h>
#include <ti/sail/button/button.h>
#include <ti/sail/led/led.h>

#include "Board.h"
#include "DBG.h"
#include "NetMgr.h"
#include "SensorData.h"

#define SAMPLE_TIME     10       /* Sample period in sec */
#define DATA_POOL_SIZE  14

typedef struct {
    bool                running;
    sem_t               workSem;        /* tick semaphore                   */
    timer_t             timerWork;      /* tick tiemr object                */
    bool                button0;        /* button #0 isr flag               */
    bool                button1;        /* button #1 isr flag               */
    DataPacket          currPacket;     /* current packet                   */
    TMP006_Handle       tmp006Handle;   /* temperature sensor               */

    Power_NotifyObj     powerAwakeNotifyObj; /* Power notify object during wakeup */
    Button_Handle       buttonHandles[Board_BUTTONCOUNT];
    LED_Handle          ledHandles[Board_LEDCOUNT];

    /* instrumentation */
    UInt                lpdsCount;      /* Low-Power Deep-Sleep counter     */
    UInt                sampleCount;    /* main loop counter                */
    UInt                numButtonPresses;  /* Number of presses */
} Monitor_Object;

static Monitor_Object Monitor = {
    .running = TRUE,
    .workSem = NULL,
    .timerWork = NULL,
    .button0 = false,
    .button1 = false,
    .currPacket = NULL,
    .tmp006Handle = NULL,
    .lpdsCount = 0,
    .numButtonPresses = 0
};

bool Monitor_allowLPDS = false;


/*
 *  ======== tmp006Setup ========
 */
static void tmp006Setup(void)
{
    int16_t volt;
    float voltage;
    TMP006_Params tmp006Params;
    I2C_Params i2cParams;
    char buf[64];
    int nb;
    float dieTemp;
    I2C_Handle i2cHandle = NULL;

    /* setup I2C controller in BLOCKING MODE (default) */
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    i2cHandle = I2C_open(Board_I2C_TMP, &i2cParams);
    if (i2cHandle == NULL) {
        nb = snprintf(buf, sizeof(buf), "I2C Open Failed!\r\r\r\n");
        DBG_write(buf, nb);
        while(1);
    }

    /* open the tmp006 instance */
    TMP006_Params_init(&tmp006Params);
    tmp006Params.conversions = TMP006_4CONV; /* 1.01 secs / conversion */

    Monitor.tmp006Handle = TMP006_open(Board_TMP006_ROOMTEMP,
            i2cHandle, &tmp006Params);

    if (Monitor.tmp006Handle == NULL) {
        nb = snprintf(buf, sizeof(buf), "TMP006_ROOMTEMP Open Failed!\r\r\r\n");
        DBG_write(buf, nb);
        while(1);
    }

    /* Allow the sensor hardware to complete its first conversion */
    sleep(2);

    /* Get Die Temperature in Celsius */
    if (!TMP006_getDieTemp(Monitor.tmp006Handle, TMP006_CELSIUS, &dieTemp)) {
        nb = snprintf(buf, sizeof(buf), "TMP006 temperature read failed\r\r\r\n");
        DBG_write(buf, nb);
        while(1);
    }

    /* Get Sensor Voltage */
    if (!TMP006_readVoltage(Monitor.tmp006Handle, &volt)) {
        nb = snprintf(buf, sizeof(buf), "TMP006 voltage read failed\r\r\r\n");
        DBG_write(buf, nb);
        while(1);
    }

    /* Convert voltage to uV */
    voltage = volt * 0.15625;

    nb = snprintf(buf, sizeof(buf), "Room Temp: %f (C)\r\r\r\n", dieTemp);
    DBG_write(buf, nb);
    nb = snprintf(buf, sizeof(buf), "Voltage: %f (uV)\r\r\r\n", voltage);
    DBG_write(buf, nb);
    nb = snprintf(buf, sizeof(buf), "\r\r\r\n");
    DBG_write(buf, nb);
}

/*
 *  ======== collectSample ========
 */
static int collectSample(void)
{
    int nb;
    char buf[128];
    static int sampleCount = 0;
    DataPacket *packet = &Monitor.currPacket;
    TempData *tempData;
    bool rval = true;
    time_t ts;
    struct tm *tm;
    float dieTemp;

    /* increment the main loop counter */
    Monitor.sampleCount++;
    ts = time(NULL);
    tm = localtime(&ts);

    /* collect a temperature sample */
    tempData = &(packet->temp[sampleCount]);

    /* sample the die temperature */
    rval = TMP006_getDieTemp(Monitor.tmp006Handle, TMP006_CELSIUS, &dieTemp);

    if (!rval) {
        nb = snprintf(buf, sizeof(buf), "TMP006 sensor read failed\r\r\r\n");
        DBG_write(buf, nb);
        goto error_temp;
    }
    tempData->die = dieTemp;
    packet->ts[sampleCount] = ts;

    nb = snprintf(buf, sizeof(buf),
            "Monitor: sample %d temp: %f, LPDS count: %d, %02d:%02d:%02d\r\r\r\n",
            Monitor.sampleCount, dieTemp, Monitor.lpdsCount, tm->tm_hour, tm->tm_min,
            tm->tm_sec);
    DBG_write(buf, nb);

    packet->count = ++sampleCount;

    if (sampleCount == NUMSAMPLESINGROUP) {

        LED_startBlinking(Monitor.ledHandles[Board_LED0], 100);

        /* post the data packet to the wifi thread */
        NetMgr_sendData(packet);

        sampleCount = 0;

        sleep(1);
        LED_stopBlinking(Monitor.ledHandles[Board_LED0]);

        /* Restore LED state to indicate power status */
        if (Monitor_allowLPDS) {
            LED_setOff(Monitor.ledHandles[Board_LED0]);
        }
        else {
            LED_setOn(Monitor.ledHandles[Board_LED0], 100);
        }
    }

error_temp:
    return (rval ? 0 : -1);
}


/*
 *  ======== Monitor_lpdsIncr ========
 *  Increment Low-Power Deep-Sleep counter
 */
void Monitor_lpdsIncr(void)
{
    Monitor.lpdsCount++;
}

/*
 *  ======== Monitor_tick ========
 *  Generate a tick event to wake the work thread
 */
void Monitor_tick(void)
{
    /* wake the thread */
    sem_post(&Monitor.workSem);
}

/*
 *  ======== Monitor_pressBtn1 ========
 */
void Monitor_pressBtn1(void)
{
    Monitor.numButtonPresses++;
    Monitor.button1 = true;
}

static void Monitor_buttonCB(Button_Handle buttonHandle, Button_EventMask buttonEvents)
{
    if (buttonHandle == Monitor.buttonHandles[Board_BUTTON1]) {
        if (BUTTON_EV_CLICKED == (buttonEvents & BUTTON_EV_CLICKED)) {
            Monitor.button1 = true;
            Monitor.numButtonPresses++;

            /* toggle power constraint */
            Monitor_allowLPDS = Monitor_allowLPDS ? false : true;

            /* wake the monitor thread */
            Monitor_tick();
        }
    }
}

/*
 *  ======== Monitor_workTick ========
 */
static void Monitor_workTick(sigval val)
{
    sem_t sem;

    sem = *(sem_t *)(val.sival_ptr);
    sem_post(&sem);
}

void Monitor_initialize()
{
    Button_Params buttonParams;
    LED_Params  ledParams;
    char buf[64];
    int nb;
    sigevent    sev;

    /*initialize button parameters to default*/
    Button_initParams(&buttonParams);
    buttonParams.debounceDuration = 20;

    /*open button 1 */
    Monitor.buttonHandles[Board_BUTTON1] = Button_open(Board_BUTTON1,
            &buttonParams, Monitor_buttonCB);
    /* Check if the button open is successful */
    if (Monitor.buttonHandles[Board_BUTTON1]  == NULL) {
        nb = snprintf(buf, sizeof(buf), "Button Open Failed! btn1=%x",
                 Monitor.buttonHandles[Board_BUTTON1]);
        DBG_write(buf, nb);
        while(1);
    }

    /* Initialize ledParams structure to defaults */
    LED_initParams(&ledParams);
    /* Set the ledType to GPIO controlled*/
    ledParams.ledType = LED_GPIO_CONTROLLED;
    /* Open LED sensor with custom Params */
    Monitor.ledHandles[Board_LED0] = LED_open(Board_LED0, &ledParams);
    if (Monitor.ledHandles[Board_LED0] == NULL) {
        nb = snprintf(buf, sizeof(buf), "Board LED Open Failed!\r\r\r\n");
        DBG_write(buf, nb);
        while(1);
    }

    /* construct the work semaphore */
    if (0 != sem_init(&Monitor.workSem, 0, 0)) {
        nb = snprintf(buf, sizeof(buf), "sem_init failed!\r\r\r\n");
        DBG_write(buf, nb);
        while(1);
    }

    /* create periodic function to schedule the work thread */
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_value.sival_ptr = &Monitor.workSem;
    sev.sigev_notify_function = &Monitor_workTick;
    sev.sigev_notify_attributes = NULL;

    if (0 != timer_create(CLOCK_MONOTONIC, &sev, &Monitor.timerWork)) {
        nb = snprintf(buf, sizeof(buf), "timer_create failed!\r\r\r\n");
        DBG_write(buf, nb);
        while(1);
    }

    tmp006Setup();

    return;
}

/*
 *  ======== Monitor_start ========
 */
void Monitor_start(void)
{
    int nb;
    char buf[64];
    struct itimerspec its;

    /* Wait for network to be ready */
    LED_startBlinking(Monitor.ledHandles[Board_LED0], 1000);
    while (!NetMgr_ready()) {
        usleep(500000);
    }
    LED_stopBlinking(Monitor.ledHandles[Board_LED0]);
    LED_setOn(Monitor.ledHandles[Board_LED0], 100);

    /* Wake up periodically to measure temperature */
    its.it_interval.tv_sec = SAMPLE_TIME;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 100000000;

    if (0 != timer_settime(Monitor.timerWork, 0, &its, NULL)) {
        nb = snprintf(buf, sizeof(buf), "timer_settime failed!\r\r\r\n");
        DBG_write(buf, nb);
        while(1);
    }

    while (true) {
        /* Wait for next workTick */
        sem_wait(&Monitor.workSem);

        /* Find out what type of work we should do */
        if (Monitor.button1) {
            nb = snprintf(buf, sizeof(buf),
                    "Total number of button presses=%d\r\r\r\n",
                    Monitor.numButtonPresses);
            DBG_write(buf, nb);

            if (Monitor_allowLPDS) {
                nb = snprintf(buf, sizeof(buf),
                        "Btn1: allow LPDS\r\r\r\n");
                DBG_write(buf, nb);

                /* Enable LPDS */
                Power_releaseConstraint(PowerCC32XX_DISALLOW_LPDS);
                LED_setOff(Monitor.ledHandles[Board_LED0]);
            }
            else {
                nb = snprintf(buf, sizeof(buf),
                        "Btn2: disallow LPDS\r\r\r\n");
                DBG_write(buf, nb);

                /* Disable LPDS */
                Power_setConstraint(PowerCC32XX_DISALLOW_LPDS);
                LED_setOn(Monitor.ledHandles[Board_LED0], 100);
            }

            Monitor.button1 = false;
        }
        else {
            collectSample();
        }
    }
}
