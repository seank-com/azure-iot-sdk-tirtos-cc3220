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
 *  ======== button.c ========
 */
/* Module Header */
#include <ti/sail/button/button.h>


/* Default Button_Params parameters structure */
const Button_Params Button_defaultParams =
{
    5,               /* 5 ms is the debouce timer value */
    2000,            /* 2 seconds long press duration*/
    200,             /* 200 ms double key press detection timeout*/
    0xFF             /* button subscribed for all callbacks*/
};

extern Button_Config Button_config[];

/* Also used to check status for initialization */
static int Button_count = -1;


/*
 *  ======== Button_close ========
 *  Closes an instance of a Button.
 */
bool Button_close(Button_Handle handle)
{
    Button_Object *obj = (Button_Object *)(handle->object);
    /*stop any further processing by stopping clock*/
    timer_delete(obj->timer);
    return (true);
}

/*
 *  ======== Button_gpioCallBackFxn ========
 * Callback function for the button interrupts
 */
void Button_gpioCallBackFxn(uint_least8_t index)
{
    /*This is the debounce timer state,This triggers the debounce timers
     *  and disables interrupts till debounce timer is done*/
    unsigned int i;
    struct itimerspec its;

    for(i = 0; i < Button_count; i++)
    {
        Button_Object *obj = (Button_Object*) Button_config[i].object;
        Button_HWAttrs *hw = (Button_HWAttrs*) Button_config[i].hwAttrs;
        if(hw->gpioIndex == index)
        {
            its.it_interval.tv_sec = 0;
            its.it_interval.tv_nsec = 0;
            its.it_value.tv_sec = obj->debounceDuration.tv_sec;
            its.it_value.tv_nsec = obj->debounceDuration.tv_nsec;
            timer_settime(obj->timer, 0, &its, NULL);
            switch(obj->buttonStateVariables.state)
            {
                case BUTTON_PRESSED:
                case BUTTON_LONGPRESSING:
                  obj->buttonStateVariables.state = BUTTON_RELEASING;
                  break;
                case BUTTON_LONGPRESSED:
                  obj->buttonStateVariables.state = BUTTON_RELEASING_LONG;
                  break;
                case BUTTON_RELEASED:
                  obj->buttonStateVariables.state = BUTTON_PRESSING;
                  break;
                case BUTTON_DBLPRESS_DETECTION:
                  obj->buttonStateVariables.state = BUTTON_DBLPRESSING;
                  break;
                case BUTTON_DBLPRESSED:
                  obj->buttonStateVariables.state = BUTTON_RELEASING_DBLPRESSED;
                  break;
                /*Any other case(mark the button as pressed),
                  Typically we should not come here*/
                default:
                  obj->buttonStateVariables.state = BUTTON_PRESSING;
                  break;

            }
            //disble the interrupt untill the debounce timer expires
            GPIO_disableInt(index);
            break;
        }
    }
}

/*
 *  ======== Button_init ========
 */
void Button_init()
{
    /* Dont Allow multiple calls for Button_init */
    if (Button_count >= 0)
    {
        return;
    }
    for (Button_count = 0; Button_config[Button_count].object != NULL;Button_count++)
    {
        Button_Object *obj = (Button_Object *)(Button_config[Button_count].object);
        obj->buttonStateVariables.pressedStartTime.tv_nsec = 0;
        obj->buttonStateVariables.pressedStartTime.tv_sec = 0;
        obj->buttonStateVariables.state = BUTTON_RELEASED;
        //todo is clearing other object parameters necessary
    }
}

/*
 *  ======== timespecdiff ========
 * function for calculating time different between two timespec values
 */
static struct timespec timespecdiff(struct timespec start, struct timespec end)
{
    struct timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}
/*
 *  ======== Button_clockTimeoutHandler ========
 * timeout handler for the clock timeouts
 */
static void Button_clockTimeoutHandler(sigval val)
{
    Button_Object *obj;
    Button_HWAttrs *hw;
    Button_Handle buttonHandle;
    GPIO_PinConfig pinConfig;
    Button_EventMask buttonEvents = 0;
    struct itimerspec its;
    struct timespec currentclocktime;
    struct timespec timespecdifference;

    buttonHandle = (Button_Handle)val.sival_ptr;
    obj = (Button_Object *)buttonHandle->object;
    hw  = (Button_HWAttrs*)buttonHandle->hwAttrs;


    GPIO_getConfig(hw->gpioIndex,&pinConfig);
    /*Getting a debouce duration timeout callback after button has been released
     *handle state transition due to release of the key*/
    if(GPIO_read(hw->gpioIndex) == obj->buttonPull)
    {
        switch(obj->buttonStateVariables.state)
        {
            case BUTTON_RELEASING:
                 if(obj->buttonEventMask & BUTTON_EV_DOUBLECLICKED)
                 {
                     obj->buttonStateVariables.state = BUTTON_DBLPRESS_DETECTION;
                     its.it_interval.tv_sec = 0;
                     its.it_interval.tv_nsec = 0;
                     its.it_value.tv_sec = obj->doublePressDetectiontimeout.tv_sec;
                     its.it_value.tv_nsec = obj->doublePressDetectiontimeout.tv_nsec;
                     timer_settime(obj->timer, 0, &its, NULL);
                 }
                 else
                 {
                     clock_gettime(CLOCK_MONOTONIC,&currentclocktime);
                     obj->buttonStateVariables.lastPressedDuration =
                       timespecdiff(obj->buttonStateVariables.pressedStartTime,
                       currentclocktime);
                     obj->buttonStateVariables.state = BUTTON_RELEASED;
                     if(obj->buttonEventMask & BUTTON_EV_RELEASED)
                     {
                         buttonEvents |= BUTTON_EV_RELEASED;
                     }
                     if(obj->buttonEventMask & BUTTON_EV_CLICKED)
                     {
                         buttonEvents |= BUTTON_EV_CLICKED;
                     }
                 }
                 break;

            case BUTTON_DBLPRESS_DETECTION:
                clock_gettime(CLOCK_MONOTONIC,&currentclocktime);
                obj->buttonStateVariables.lastPressedDuration =
                     timespecdiff(obj->buttonStateVariables.pressedStartTime,
                     currentclocktime);
                 if(obj->buttonEventMask & BUTTON_EV_RELEASED)
                 {
                     buttonEvents |= BUTTON_EV_RELEASED;
                 }
                 if(obj->buttonEventMask & BUTTON_EV_CLICKED)
                 {
                     buttonEvents |= BUTTON_EV_CLICKED;
                 }
                 obj->buttonStateVariables.state = BUTTON_RELEASED;
                 break;

            case BUTTON_RELEASING_LONG:
                 clock_gettime(CLOCK_MONOTONIC,&currentclocktime);
                obj->buttonStateVariables.lastPressedDuration =
                     timespecdiff(obj->buttonStateVariables.pressedStartTime,
                     currentclocktime);
                 if(obj->buttonEventMask & BUTTON_EV_LONGCLICKED)
                 {
                     buttonEvents |= BUTTON_EV_LONGCLICKED;
                 }
                 obj->buttonStateVariables.state = BUTTON_RELEASED;
                 break;

            case BUTTON_RELEASING_DBLPRESSED:
                 obj->buttonStateVariables.state = BUTTON_RELEASED;
                 break;

            case BUTTON_PRESSING:
            case BUTTON_DBLPRESSING:
            case BUTTON_LONGPRESSING:
                /*Were pressing, but is released after debounce,
                  so not a press or anything.*/
                 obj->buttonStateVariables.state = BUTTON_RELEASED;
                 break;
                 
            /*Any other case(mark the button as Released),
              Typically we should not come here*/                 
            default:
                 obj->buttonStateVariables.state = BUTTON_RELEASED;
                 break;                 
       }
       if(obj->buttonPull == PULL_DOWN)
       {
           GPIO_setConfig(hw->gpioIndex,
                          ((pinConfig & (~GPIO_CFG_INT_MASK))|
                            GPIO_CFG_IN_INT_RISING));
       }
       else if(obj->buttonPull == PULL_UP)
       {
           GPIO_setConfig(hw->gpioIndex,
                          ((pinConfig & (~GPIO_CFG_INT_MASK))|
                          GPIO_CFG_IN_INT_FALLING));
       }
    }
    /*Getting a debouce duration timeout callback after button has been pressed
     *handle state transition due to release of the key*/
    else
    {
       switch(obj->buttonStateVariables.state)
       {
            case BUTTON_PRESSING:
                 // This is a debounced press
                 clock_gettime(CLOCK_MONOTONIC,&currentclocktime);
                 obj->buttonStateVariables.pressedStartTime = currentclocktime;
                 if (obj->buttonEventMask & BUTTON_EV_PRESSED)
                 {
                    buttonEvents |= BUTTON_EV_PRESSED;
                 }
                 // Start countdown if interest in long-press
                 if (obj->buttonEventMask &
                    (BUTTON_EV_LONGPRESSED | BUTTON_EV_LONGCLICKED))
                 {
                     obj->buttonStateVariables.state = BUTTON_LONGPRESSING;
                     timespecdifference =   timespecdiff(obj->debounceDuration,obj->longPressDuration);
                     its.it_interval.tv_sec = 0;
                     its.it_interval.tv_nsec = 0;
                     its.it_value.tv_sec  = timespecdifference.tv_sec;
                     its.it_value.tv_nsec = timespecdifference.tv_nsec;
                     //start the debounce timer once the key press is seen
                     timer_settime(obj->timer, 0, &its, NULL);
                 }
                 else
                 {
                    obj->buttonStateVariables.state = BUTTON_PRESSED;
                 }
                 break;

            case BUTTON_DBLPRESSING:
                 // This is a debounced press(this is considered as double click)
                 if(obj->buttonEventMask & BUTTON_EV_DOUBLECLICKED)
                 {
                     buttonEvents |= BUTTON_EV_DOUBLECLICKED;
                 }
                 obj->buttonStateVariables.state = BUTTON_DBLPRESSED;
                 break;

            case BUTTON_LONGPRESSING:
                 obj->buttonStateVariables.state = BUTTON_LONGPRESSED;
                 if (obj->buttonEventMask & BUTTON_EV_LONGPRESSED)
                 {
                    buttonEvents |= BUTTON_EV_LONGPRESSED;
                 }
                 break;

            case BUTTON_RELEASING:
            case BUTTON_RELEASING_LONG:
            case BUTTON_RELEASING_DBLPRESSED:
                 // Were releasing, but isn't released after debounce.
                 // Start countdown again if interest in long-press
                 if (obj->buttonEventMask &
                     (BUTTON_EV_LONGPRESSED | BUTTON_EV_LONGCLICKED))
                 {
                     timespecdifference =   timespecdiff(obj->debounceDuration,obj->longPressDuration);
                     its.it_interval.tv_sec = 0;
                     its.it_interval.tv_nsec = 0;
                     its.it_value.tv_sec  = timespecdifference.tv_sec;
                     its.it_value.tv_nsec = timespecdifference.tv_nsec;
                     timer_settime(obj->timer, 0, &its, NULL);

                     obj->buttonStateVariables.state = BUTTON_LONGPRESSING;
                 }
                 else
                 {
                   obj->buttonStateVariables.state = BUTTON_PRESSED;
                 }
                 
            /*Any other case(mark the button as Pressed),
              Typically we should not come here*/                 
            default:
                 obj->buttonStateVariables.state = BUTTON_PRESSED;
                 break;                      
       }
       if(obj->buttonPull == PULL_DOWN)
       {
           GPIO_setConfig(hw->gpioIndex,
                          ((pinConfig & (~GPIO_CFG_INT_MASK))
                          |GPIO_CFG_IN_INT_FALLING));
       }
       else if(obj->buttonPull == PULL_UP)
       {
           GPIO_setConfig(hw->gpioIndex,
                          ((pinConfig & (~GPIO_CFG_INT_MASK))|
                          GPIO_CFG_IN_INT_RISING));
       }
    }
    if((buttonEvents != 0) && (obj->buttonCallBack != NULL))
    {
        obj->buttonCallBack(buttonHandle,buttonEvents);
    }
    GPIO_enableInt(hw->gpioIndex);
}
/*
 *  ======== Button_open ========
 * Setups Button returns Button_Handle
 */
Button_Handle Button_open(unsigned int buttonIndex,Button_Params *params,
                    Button_CallBack buttonCallback)
{
    Button_Handle handle;
    Button_Object *obj;
    Button_HWAttrs *hw;
    GPIO_PinConfig pinConfig;
    sigevent       sev;
    int retc;


    /* This sets  the init state of the button*/
    /*buttonIndex cannot be greater than total ButtonCount*/
    if (buttonIndex >= Button_count)
    {
        return (NULL);
    }
    /*If params is null then use the default params*/
    if (params == NULL)
    {
        params = (Button_Params *) &Button_defaultParams;
    }

    handle = (Button_Handle)&Button_config[buttonIndex];

    obj = (Button_Object*)(Button_config[buttonIndex].object);
    hw  = (Button_HWAttrs*)(Button_config[buttonIndex].hwAttrs);

    /*create timer for handling debounce */
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_value.sival_ptr = handle;
    sev.sigev_notify_function = &Button_clockTimeoutHandler;
    sev.sigev_notify_attributes = NULL;
    retc = timer_create(CLOCK_MONOTONIC,&sev, &obj->timer);
    if (0 != retc)
    {
        return (NULL);
    }

    obj->debounceDuration.tv_sec             = (params->debounceDuration)/1000;
    obj->debounceDuration.tv_nsec            = ((params->debounceDuration)%1000)*1000000;
    obj->longPressDuration.tv_sec            = (params->longPressDuration)/1000;
    obj->longPressDuration.tv_nsec           = ((params->longPressDuration)%1000)*1000000;
    obj->doublePressDetectiontimeout.tv_sec  = (params->doublePressDetectiontimeout)/1000;
    obj->doublePressDetectiontimeout.tv_nsec = ((params->doublePressDetectiontimeout)%1000)*1000000;
    obj->buttonCallBack                      = buttonCallback;
    obj->buttonEventMask                     = params->buttonEventMask;


    GPIO_getConfig(hw->gpioIndex,&pinConfig);
    if((pinConfig & GPIO_CFG_IN_PD) == GPIO_CFG_IN_PD)
    {
        obj->buttonPull = PULL_DOWN;
        GPIO_setConfig(hw->gpioIndex, pinConfig|GPIO_CFG_IN_INT_RISING);
    }
    else if((pinConfig & GPIO_CFG_IN_PU) == GPIO_CFG_IN_PU)
    {
        obj->buttonPull = PULL_UP;
        GPIO_setConfig(hw->gpioIndex, pinConfig|GPIO_CFG_IN_INT_FALLING);
    }
    else
    {
        /*if button is not configured  for floating then return null as handle
        * No creation of object.
        */
        return (NULL);
    }
    GPIO_setCallback(hw->gpioIndex, &Button_gpioCallBackFxn);
    /* Enable interrupts */
    GPIO_enableInt(hw->gpioIndex);
    return handle;
}
/*
 *  ======== Button_initParams ========
 * Initialize a Button_Params struct to default settings.
 */
void Button_initParams(Button_Params *params)
{
    *params = Button_defaultParams;
}

/*
 *  ======== Button_SetCallBack ========
 * Set the callback for the buttons.
 */
void Button_setCallBack(Button_Handle handle,Button_CallBack buttonCallBack)
{
    Button_Object *obj = (Button_Object *)handle->object;

    obj->buttonCallBack = buttonCallBack;
}

/*
 *  ======== Button_getLastPressedDuration ========
 * Return the get last pressed duration
 */
extern unsigned int Button_getLastPressedDuration(Button_Handle handle)
{
    unsigned int calculateLastPressedDuration;
    Button_Object *obj = (Button_Object *)handle->object;
    calculateLastPressedDuration = obj->buttonStateVariables.lastPressedDuration.tv_sec * 1000 + obj->buttonStateVariables.lastPressedDuration.tv_nsec/1000000;

    return calculateLastPressedDuration;
}
