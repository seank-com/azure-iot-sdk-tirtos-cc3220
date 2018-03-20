/*
 * Copyright (c) 2016-2017, Texas Instruments Incorporated
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
 *  ======== led.c ========
 */

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/PWM.h>

/* Module Header */
#include <ti/sail/led/led.h>

#define LED_BLINKPERIOD_ZERO    0U    /* To indicate no blinking operation*/
#define LED_PWMPERIOD_1MS       1000U /* Default PWM period is 1 ms*/

/* Default LED parameters structure */
const LED_Params LED_defaultParams = {
        LED_GPIO_CONTROLLED,    /* By default, LED is GPIO controlled*/
        NULL,                   /* As default is GPIO, PWM handle is NULL*/
        LED_PWMPERIOD_1MS,      /* Default PWM period is 1 ms*/
        LED_BRIGHTNESS_MIN,     /* To keep minimum brightness*/
        LED_STATE_OFF,          /* Set LED state to OFF*/
        LED_BLINKPERIOD_ZERO    /* Blink period zero i.e. no blinking*/
};

extern LED_Config LED_config[];

/* Used to check status and initialization */
static int LED_count = -1;

/* Static Functions followed by regular ones*/
static void timerTimeoutHandler(sigval val)
{
    LED_toggle((LED_Handle)val.sival_ptr);
}
/*
 *  ======== LED_close ========
 *  Closes an instance of a led sensor.
 */
void LED_close(LED_Handle ledHandle)
{
    LED_Object *obj = (LED_Object *)(ledHandle->object);

    LED_stopBlinking(ledHandle);
    LED_setOff(ledHandle);
    /* Delete opened timer*/
    timer_delete(obj->timer);
}

/*
 *  ======== LED_getState ========
 */
LED_State LED_getState(LED_Handle ledHandle)
{
    LED_Object *obj = (LED_Object *)(ledHandle->object);

    return (obj->state);
}

/*
 *  ======== LED_init ========
 */
void LED_init()
{
    if (LED_count == -1)
    {
        /* Call each driver's init function */
        for (LED_count = 0; LED_config[LED_count].object != NULL; LED_count++)
        {
            ((LED_Object *)(LED_config[LED_count].object))->ledType    = LED_GPIO_CONTROLLED;
            ((LED_Object *)(LED_config[LED_count].object))->pwmHandle  = NULL;
            ((LED_Object *)(LED_config[LED_count].object))->pwmPeriod  = LED_PWMPERIOD_1MS;
            ((LED_Object *)(LED_config[LED_count].object))->blinkPeriod= LED_BLINKPERIOD_ZERO;
            ((LED_Object *)(LED_config[LED_count].object))->brightness = LED_BRIGHTNESS_MIN;
            ((LED_Object *)(LED_config[LED_count].object))->state      = LED_STATE_OFF;
            ((LED_Object *)(LED_config[LED_count].object))->rawState   = LED_STATE_OFF;
        }
    }
}

/*
 *  ======== LED_open ========
 * Sets up led sensor and returns LED_Handle
 */
LED_Handle LED_open(unsigned int ledIndex, LED_Params *params)
{
    LED_Handle ledHandle;
    LED_Object *obj;
    sigevent       sev;
    int retc;

    /* ledIndex cannot be more than number of available LEDs*/
    if ((ledIndex >= LED_count))
    {
        return (NULL);
    }

    /* If params are NULL use defaults. */
    if (params == NULL)
    {
        params = (LED_Params *)(&LED_defaultParams);
    }
    else if ((params->ledType == LED_PWM_CONTROLLED) & (params->pwmHandle == NULL))
    {
        /* Need valid pwmHandle for PWM controlled LEDs*/
        return (NULL);
    }

    /* Get handle for this driver instance */
    ledHandle = (LED_Handle)(&LED_config[ledIndex]);

    obj = (LED_Object *)(LED_config[ledIndex].object);


    /* create the timer instance*/
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_value.sival_ptr = ledHandle;
    sev.sigev_notify_function = &timerTimeoutHandler;
    sev.sigev_notify_attributes = NULL;
    retc = timer_create(CLOCK_MONOTONIC,&sev, &obj->timer);
    if (0 != retc)
    {
        return (NULL);
    }

    /* Update Object fields*/
    obj->ledType    = params->ledType;
    obj->pwmHandle  = params->pwmHandle;
    obj->pwmPeriod  = params->pwmPeriod;
    obj->brightness = params->brightness;

    /* Set LED state, default is Off if setState is not modified by user*/
    switch (params->setState)
    {
        case LED_STATE_OFF:
            LED_setOff(ledHandle);
            break;

        case LED_STATE_ON:
            LED_setOn(ledHandle, obj->brightness);
            break;

        case LED_STATE_BLINKING:
            LED_startBlinking(ledHandle, params->blinkPeriod);
            break;

        default:
            /* Invalid setState value*/
            break;
    }

    return (ledHandle);
}

/*
 *  ======== LED_initParams ========
 * Initialize a LED_Params struct to default settings.
 */
void LED_initParams(LED_Params *params)
{
    *params = LED_defaultParams;
}

/*
 *  ======== LED_setBrightnessLevel ========
 * Sets brightness level as requested.
 */
bool LED_setBrightnessLevel(LED_Handle ledHandle, uint16_t level)
{
    uint32_t duty = 0;
    LED_Object *obj = (LED_Object *)(ledHandle->object);

    /* Report false if brightness request is more than maximum(100%) level */
    if (level > LED_BRIGHTNESS_MAX)
    {
        return false;
    }

    /* For PWM LED, calculate duty based on requested level and set that */
    if (obj->ledType == LED_PWM_CONTROLLED)
    {
        duty = (obj->pwmPeriod * level)/100;
        PWM_setDuty(obj->pwmHandle, duty);
        obj->brightness = level;
    }

    /* For GPIO LED, return 'false' to reflect invalid request as GPIO LED can
     * be just turned On or Off and intermediate brightness level can't be set*/
    else if (obj->ledType == LED_GPIO_CONTROLLED)
        {
            return false;
        }

    return true;
}

/*
 *  ======== LED_setOff ========
 *  Turns Off a specified a led sensor.
 */
void LED_setOff(LED_Handle ledHandle)
{
    LED_Object *obj = (LED_Object *)(ledHandle->object);
    uint16_t level;

    if (obj->ledType == LED_GPIO_CONTROLLED)
    {
        GPIO_write(((LED_HWAttrs*)ledHandle->hwAttrs)->gpioIndex, LED_OFF);
    }

    /* For PWM LED, set brightness to zero
     * Also, restoring brightness level which was there before turning it Off
     * so that Toggle APIs can set same brightness while turning it On */
    else
    {
        level = obj->brightness;
        LED_setBrightnessLevel(ledHandle, LED_BRIGHTNESS_MIN);
        obj->brightness = level;
    }

    /* Set LED state and rawState
     * If LED is blinking, which is a separate state(mix of ON + OFF), no need
     * to change state; rawState contains the actual ON or OFF state*/
    if (obj->state != LED_STATE_BLINKING)
    {
        obj->state    = LED_STATE_OFF;
    }

    obj->rawState = LED_STATE_OFF;
}

/*
 *  ======== LED_setOn ========
 *  Turns On a specified led sensor.
 */
bool LED_setOn(LED_Handle ledHandle,uint16_t brightness)
{
    bool ret = true;
    LED_Object *obj = (LED_Object *)(ledHandle->object);

    if (obj->ledType == LED_GPIO_CONTROLLED)
    {
        GPIO_write(((LED_HWAttrs*)ledHandle->hwAttrs)->gpioIndex, LED_ON);
    }

    /* For PWM LED, turn it On with requested level, also pouplate error */
    else
    {
        ret = LED_setBrightnessLevel(ledHandle, brightness);
    }

    /* Set LED state(conditional) and rawState(always)*/
    if (ret == true)
    {
        if (obj->state != LED_STATE_BLINKING)
        {
            obj->state = LED_STATE_ON;
        }

        obj->rawState  = LED_STATE_ON;
    }

    return ret;
}

/*
 *  ======== LED_startBlinking ========
 *  Starts blinking a led with specified period.
 */
void LED_startBlinking(LED_Handle ledHandle, uint16_t blinkPeriod)
{
    LED_Object *obj = (LED_Object *)(ledHandle->object);
    struct itimerspec its;

    /* If LED is starting to blink afresh*/
    if (obj->state != LED_STATE_BLINKING)
    {
        /* No need to start blinking if blinkperiod is passed as zero*/
        if (blinkPeriod == LED_BLINKPERIOD_ZERO)
        {
            /* do nothing*/
        }

        else
        {
            /* start the periodic timer with period = blinkperiod
             * in ms units*/
            its.it_interval.tv_sec  = blinkPeriod/1000;
            its.it_interval.tv_nsec = (blinkPeriod%1000) * 1000000;
            its.it_value.tv_sec     = blinkPeriod/1000;
            its.it_value.tv_nsec    = (blinkPeriod%1000) * 1000000;
            timer_settime(obj->timer, 0, &its, NULL);

            obj->blinkPeriod = blinkPeriod;
            obj->state = LED_STATE_BLINKING;
        }
    }

    else if (obj->state == LED_STATE_BLINKING)
    {
        /* If LED is already blinking and requested to change the blink period
         * Execute sequence: do a settime again*/
        if (obj->blinkPeriod != blinkPeriod)
        {
            /* start the periodic timer with period = blinkperiod
             * in ms units*/
            its.it_interval.tv_sec  = blinkPeriod/1000;
            its.it_interval.tv_nsec = (blinkPeriod%1000) * 1000000;
            its.it_value.tv_sec     = blinkPeriod/1000;
            its.it_value.tv_nsec    = (blinkPeriod%1000) * 1000000;
            timer_settime(obj->timer, 0, &its, NULL);

            obj->blinkPeriod = blinkPeriod;
            obj->state = LED_STATE_BLINKING;
        }

        else
        {
            /* If LED is already blinking with same period, do nothing*/
        }
    }
}

/*
 *  ======== LED_stopBlinking ========
 *  Stops blinking a led.
 */
void LED_stopBlinking(LED_Handle ledHandle)
{
    LED_Object *obj = (LED_Object *)(ledHandle->object);
    struct itimerspec its;

    if (obj->state == LED_STATE_BLINKING)
    {
        /* stop the periodic timer with period*/
        its.it_interval.tv_sec  = 0;
        its.it_interval.tv_nsec = 0;
        its.it_value.tv_sec     = 0;
        its.it_value.tv_nsec    = 0;
        timer_settime(obj->timer, 0, &its, NULL);;

        /* After stoping blinking, sets LED Off, a default LED state*/
        LED_setOff(ledHandle);
        obj->state = LED_STATE_OFF;
    }

    else
    {
        /* If LED is not bliking, no need to stop it, so ignore the request*/
    }
}

/*
 *  ======== LED_toggle ========
 *  Toggle a led.
 */
void LED_toggle(LED_Handle ledHandle)
{
    LED_Object *obj = (LED_Object *)(ledHandle->object);

    if (obj->rawState == LED_STATE_ON)
    {
        LED_setOff(ledHandle);
    }

    else if (obj->rawState == LED_STATE_OFF)
    {
        LED_setOn(ledHandle, obj->brightness);
    }
}
