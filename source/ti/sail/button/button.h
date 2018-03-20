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

 /** ===========================================================================
 *  @file       button.h
 *
 *  @brief      button driver
 *
 *  The BUTTON header file should be included in an application as
 *  follows:
 *  @code
 *  #include <ti/sail/button/button.h>
 *  @endcode

 *  The BUTTON driver simplifies interfacing  Push buttons.  The push buttons
 *  on the LaunchPads or push buttons present on the BoosterPacks can be easily
 *  interfaced using Button Module.  The button module provides useful
 *  callbacks on button actions to the application.
 *
 *  Buttons use GPIO for interfacing with MCU so the GPIO pins must be
 *  configured and then initialized by using GPIO_init().  A Button_config
 *  array should be defined by the application.  The Button_config array should
 *  contain pointers to a defined Button_HWAttrs and allocated array for the
 *  Button_Object structures.  Button_init() must be called prior to using
 *  Button_open().
 *
 *  The APIs in this driver serve as an interface to a DPL(Driver Porting Layer)
 *  The specific implementations are responsible for creating all the RTOS
 *  specific primitives to allow for thread-safe operation.
 *
 *  For accurate operation, correct selection of the debounce timer value
 *  will be necessary.Incorrectly selecting a debounce timer value may not
 *  debounce the push button correctly.
 *
 *  This driver has no dynamic memory allocation.
 *
 *  ## Defining Button_Config, Button_Object and Button_HWAttrs #
 *  Each structure must be defined by the application. The following
 *  example is for a MSP432 in which two Button  are setup.
 *  The following declarations are placed in "MSP_EXP432P401R.h"
 *  and "MSP_EXP432P401R.c" respectively. How the gpioIndices are defined
 *  are detailed in the next example.
 *
 *  "MSP_EXP432P401R.h"
 *  @code
 *  typedef enum MSP_EXP432P401R_ButtonName {
 *      MSP_EXP432P401R_BUTTON_0 = 0, //Button number 1 on LaunchPad
 *      MSP_EXP432P401R_BUTTON_1 = 1, //Button number 2 on LaunchPad
 *      MSP_EXP432P401R_BUTTONCOUNT   //Total number of buttons on the board
 *  } MSP_EXP432P401R_ButtonName;
 *
 *  @endcode
 *
 *  "MSP_EXP432P401R.c"
 *  @code
 *  #include <Button.h>
 *
 *  Button_Object Button_object[MSP_EXP432P401R_BUTTONCOUNT];
 *
 *  const  Button_HWAttrs Button_hwAttrs[MSP_EXP432P401R_BUTTONCOUNT] = {
 *          {
 *              .gpioIndex = MSP_EXP432P401R_S1,
 *          },
 *                  {
 *              .gpioIndex = MSP_EXP432P401R_S2,
 *          }
 *  };
 *
 *  const Button_Config Button_config[] = {
 *      {
 *          .hwAttrs = &Button_hwAttrs[0],
 *          .object =  &Button_object[0],
 *      },
 *      {
 *          .hwAttrs = &Button_hwAttrs[1],
 *          .object =  &Button_object[1],
 *      },
 *  };
 *  @endcode
 *
 *  ##Setting up GPIO configurations # The following example is for a MSP432.
 *  We are showing interfacing of two push buttons.  Each need a GPIO pin. The
 *  following definitions are in "MSP_EXP432P401R.h" and "MSP_EXP432P401R.c"
 *  respectively. This example uses GPIO pins 1.1 and 1.4.
 *
 *  "MSP_EXP432P401R.h"
 *  @code
 *  typedef enum MSP_EXP432P401R_GPIOName {
 *     MSP_EXP432P401R_S1 = 0,
 *     MSP_EXP432P401R_S2,
 *     MSP_EXP432P401R_GPIOCOUNT
 *  } MSP_EXP432P401R_GPIOName;
 *  @endcode
 *
 *  "MSP_EXP432P401R.c"
 *  @code
 *  #include <gpio.h>
 *  GPIO_PinConfig gpioPinConfigs[] = {
 *      GPIOMSP432_P1_1 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING,
 *      GPIOMSP432_P1_4 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING,
 *  }
 *
 *  @endcode
 *
 *
 *  ## Opening a Button  with default parameters #
 *  The Button_open() call must be made in a task context.
 *
 *  @code
 *  #include <ti/sail/button/button.h>
 *
 *  Button_Handle ButtonHandle;
 *
 *  Button_initParams(&buttonParams);
 *  buttonHandle = Button_open(0,&buttonParams,HandlebuttonCallback)
 *  @endcode
 *  ============================================================================
 */
#ifndef BUTTON_H_
#define BUTTON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/* POSIX Header files */
#include <pthread.h>
#include <time.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>

/*!
*  @brief    Button configuration
*
*  The Button_Config structure contains a set of pointers used to characterize
*  the Button driver implementation.
*
*  This structure needs to be defined and provided by the application.
*/
typedef    struct Button_Config
{
    /*! Pointer to an uninitialized user defined Button_Object struct */
    void *object;

    /*! Pointer to a driver specific hardware attributes structure */
    void const *hwAttrs;
}Button_Config;

/*!
 *  @brief    A handle that is returned from a Button_open() call.
 *
 *  User will use this handling for future interactions with the button
 *  instance that is opened.
 */
typedef struct Button_Config*   Button_Handle;

/*!
 *  @brief    Button State
 *  @private
 *
 * This enumeration describes whether the button is pressed or released etc.
 * This is for internal state machine handling.
 */
typedef enum  Button_State
{
    /*!
    *Edge detected, debouncing
    */
    BUTTON_PRESSING                     = 1,
    /*!
    *Press verified, not detecting longpress
    */
    BUTTON_PRESSED                      = 2,

    /*!
    *Press verified, waiting for longpress timeout.
    */
    BUTTON_LONGPRESSING                 = 3,

    /*!
    *Longpress verified, waiting for neg-edge
    */
    BUTTON_LONGPRESSED                  = 4,

    /*!
    *Neg-edge received, debouncing
    */
    BUTTON_RELEASING                    = 5,

    /*!
    *Neg-edge received after long-press, debouncing.
    */
    BUTTON_RELEASING_LONG               = 6,

    /*!
    *Button release verified.
    */
    BUTTON_RELEASED                     = 7,

    /*!
    *EDGE detected doublepress
    */
    BUTTON_DBLPRESS_DETECTION           = 8,

    /*!
    *EDGE detected doublepress
    */
    BUTTON_DBLPRESSING                  = 9,

    /*!
    *DOUBLE PRESS verified, waiting for neg edge
    */
    BUTTON_DBLPRESSED                   = 10,

    /*!
    *DOUBLE PRESS verified, waiting for neg edge
    */
    BUTTON_RELEASING_DBLPRESSED         = 11
} Button_State;


 /*!
 *  @brief Button event flags
 *
 *  The event flags are used per button to tell the driver which events should
 *  generate a callback to the application.Only events included in the event
 *   mask will be signalled to the application.
 */
typedef enum Button_Events{
    /*!
    *Button pressed down, may or may not subsequently have been released.
    */
    BUTTON_EV_PRESSED        = 0x01,

    /*!
    *Button held down for more than tLongpress (ms).
    */
    BUTTON_EV_LONGPRESSED    = 0x02,

    /*!
    *Button released after press or longpress.
    */
    BUTTON_EV_RELEASED       = 0x04,

    /*!
    * Button was pressed and released, but not held for longPressDuration (ms).
    */
    BUTTON_EV_CLICKED        = 0x08,

    /*!
    *Button was pressed and released, and held for longer than
    *longPressDuration (ms).
    */
    BUTTON_EV_LONGCLICKED    = 0x10,

    /*!
    *Button was pressed down again when double click detection was active
    */
    BUTTON_EV_DOUBLECLICKED  = 0x20,
} Button_Events;

/** \brief Event subscription and notification mask type
  */
typedef char Button_EventMask;


/*!
 *  @brief    A handler to receive button callbacks.
 */
typedef void (*Button_CallBack)(Button_Handle buttonHandle,
                                Button_EventMask buttonEvents);


/*!
 *  @brief    Button Pull settings
 *
 * This enumeration defines whether the GPIO connected to the button.
 * is PULL UP or PULL DOWN
 */
typedef enum Button_Pull
{
    PULL_DOWN   = 0,                             //!< Button is PULLED DOWN.
    PULL_UP     = 1                              //!< Button is PULLED UP.
} Button_Pull;

/*!
 *  @brief    Hardware specific settings for a Button sensor.
 *
 *  This structure should be defined and provided by the application. The
 *  gpioIndex should be defined in accordance of the GPIO driver. The pin
 *  must be configured as GPIO_CFG_INPUT and GPIO_CFG_IN_INT_FALLING.
 */
typedef struct Button_HWAttrs
{
    unsigned int           gpioIndex;            //!< GPIO configuration index.
} Button_HWAttrs;

/*!
 *  @brief  Button Button_StateVariables
 *  @private
 *
 *  Each button instance needs set of variables to monitor its state.
 *  We group these variables under the structure Button_State.
 *
 *  @sa     Button_Params_init()
 */
typedef struct Button_StateVariables
{
    Button_State        state;                   //!< Button State.

    /*!
    *Button pressed start time in milliseconds(ms).
    */
    struct timespec       pressedStartTime;

    /*!
    *Button pressed duration(it may be short press(click), long press(ms).
    */
    struct timespec       lastPressedDuration;
}Button_StateVariables;

/*!
 *  @brief    Internal to Button module.Members should not be accessed
 *            by the application.
 */
typedef struct Button_Object
{
    /*!
    * timer structure to be used for creating timer.
    */
    timer_t                timer;

    /*!
    * state variables for handling the debounce state machine.
    */
    Button_StateVariables  buttonStateVariables;

    /*!
    * Event subscription mask for the button.
    */
    Button_EventMask       buttonEventMask;

    /*!
    * Callback function for the button.
    */
    Button_CallBack        buttonCallBack;

    /*!
    * Debounce duration for the button in milliseconds(ms).
    */
    struct timespec        debounceDuration;

    /*!
    * Long press duration is milliseconds(ms).
    */
    struct timespec        longPressDuration;

    /*!
    * double press detection timeout is milliseconds(ms).
    */
    struct timespec        doublePressDetectiontimeout;

    /*!
    * Button pull(stored after reading from GPIO module).
    */
    Button_Pull            buttonPull;
} Button_Object;

/*!
 *  @brief  Button Parameters
 *
 *  Button parameters are used with the Button_open() call. Default values for
 *  these parameters are set using Button_initParams().
 *
 *  @sa     Button_initParams()
 */
typedef struct Button_Params
{
    /*!
    * Debounce duration for the button in milliseconds(ms).
    */
    unsigned int        debounceDuration;

    /*!
    * Long press duration is milliseconds(ms).
    */
    unsigned int        longPressDuration;

    /*!
    * double press detection timeout is milliseconds(ms).
    */
    unsigned int        doublePressDetectiontimeout;

    /*!
    * Event subscription mask for the button.
    */
    Button_EventMask    buttonEventMask;
} Button_Params;

/*!
 *  @brief  Function to close a Button specified by the Button handle
 *
 *  The Button close API to close button instance specified by Button handle.
 *
 *  @pre    Button_open() had to be called first.
 *
 *  @param  handle    A Button_Handle returned from Button_open() call
 *
 *  @return true on success or false upon failure.
 */
extern bool Button_close(Button_Handle handle);


/*!
 *  @brief  Function to initialize Button driver.
 *
 *  This function will initialize the Button driver.
 */
extern void Button_init();

/*!
 *  @brief  Function to open a given Button
 *
 *  Function to initialize a given button specified by the particular
 *  index value. A gpioIndex must be setup and specified in the 
 *  Button_HWAttrs structure.
 *
 *
 *  The user should ensure that each Button gpioIndex and
 *  buttonIndex.
 *
 *  @pre    Button_init() has to be called first
 *
 *  @param  buttonIndex       Logical button number  indexed into
 *                         the Button_config table
 *
 *
 *  @param  *params        A pointer to Button_Params structure. If NULL, it
 *                         will use default values.
 *
 *  @return  A Button_Handle on success, or a NULL on failure.
 *
 *  @sa      Button_init()
 *  @sa      Button_initParams()
 *  @sa      Button_close()
 */
extern Button_Handle Button_open(unsigned int buttonIndex, Button_Params *params,
                          Button_CallBack buttonCallback);

/*!
 *  @brief  Function to initialize a Button_Params struct to its defaults
 *
 *  @param  params      A pointer to Button_Params structure for
 *                      initialization.
 *
 *  Default values
 *  ------------------------------------------------------------------
 *  parameter        | value        | description              | unit
 *  -----------------|--------------|--------------------------|------------
 *  debounceDuration | 10           | debounce duration        | ms
 *  longPressDuration| 2000         | long press duration      | ms
 *  buttonEventMask  | 0xFF         | subscribed to all events | NA
 */
extern void Button_initParams(Button_Params *params);

/*!
 *  @brief  Function to returns the lastPressedDuration(valid only for short
            press, long press)
 *
 *  The API returns last pressed duration and it is valid only for shortpress,
 *  longpress. If this API is called after receiving of event click, long
 *  click then the API returns the press duration which is time duration between
 *  the press and release of the button.
 *  **Note: This API call is only valid after a click, long click.(not on double click).**
 *
 *  @param  *obj            pointer to the button object
 *
 *
 *  @return  an unsigned int which is represents the time duration in milliseconds.
 *
 */
extern unsigned int Button_getLastPressedDuration(Button_Handle handle);

/*!
 *  @brief  Function to sets callback function for the button object
 *
 *  @param  handle           A Button_Handle returned from Button_open()
 *
 *  @param  buttonCallBack   button callback function
 *
 */
extern void Button_setCallBack(Button_Handle handle,Button_CallBack buttonCallBack);

/*!
 *  @brief  This is the gpio interrupt callback function which is called on a 
            button press or release.This is internally used by button module.
 *  
 *  This API is internally used by button module for receiving the GPIO
 *  interrupt callbacks. We are exposing this API to applications also,
 *  In some of the MCUs, when in LPDS(Low power deep sleep) the GPIO interrupt
 *  is consumed for wake up, and in order to make the button module work the
 *  the application has to call this API with the index of the GPIO pin which 
 *  actually was the reason for the wake up.
 *
 *  @param  index          Index of the GPIO for which the button press has to be
 *                         Detected. This is an index in GPIO_pinConfig array.
 *
 *
 */
void Button_gpioCallBackFxn(uint_least8_t index);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_H_ */
