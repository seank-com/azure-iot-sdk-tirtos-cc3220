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

/** ============================================================================
 *  @file       led.h
 *
 *  @brief      led driver
 *
 *  The LED header file should be included in an application as
 *  follows:
 *  @code
 *  #include <led.h>
 *  @endcode
 *
 *  # Operation #
 *  An LED driver simplifies using a LED (may be GPIO or PWM controlled)
 *  available on board and supports following operations -
 *
 *      1. To Turn ON/OFF
 *      2. Blink with requested delay, stop when requested
 *      3. Vary brightness(can only be done to a PWM controlled LED)
 *      4. Toggle
 *
 *  There are also APIs to open and close an LED handle and also one to get
 *  current state of a LED. User can request to set a LED into particular state
 *  while opening itself i.e. to start blink as part of LED_open() call.
 *
 *  Information about LEDs and their instances will be available in board
 *  specific example files.
 *
 *  A LED_config array should be defined by the application. This LED_config
 *  array should contain pointers to a defined LED_HWAttrs and allocated array
 *  of LED_Object structure.
 *  LED_init() must be called before using LED_open().
 *
 *  Any GPIO pins must be configured and then initialized by using
 *  GPIO_Init(). Similarly, any PWM pins must be configured and then
 *  initialized by using PWM_init().
 *
 *  The APIs in this driver serve as an interface to a DPL(Driver Porting Layer)
 *  The specific implementations are responsible for creating all the RTOS
 *  specific primitives to allow for thread-safe operation.
 *
 *  This driver has no dynamic memory allocation.
 *
 *  ## Defining LED_Config, LED_Object and LED_HWAttrs #
 *  To use SAIL LED driver, an application has to indicate how many LEDs it
 *  wants to operate, of what type(PWM or GPIO controlled) and which GPIO
 *  to index for controlling an LED.
 *
 *  Each structure must be defined by the application. The following
 *  example is for an MSP432P401R platform in which four LEDs are available
 *  on board.
 *  The following declarations are placed in "MSP_EXP432P401R.h"
 *  and "MSP_EXP432P401R.c" respectively. How the gpioIndices are defined
 *  are detailed in the next section.
 *
 *  "MSP_EXP432P401R.h"
 *  @code
 *  typedef enum MSP_EXP432P401R_LEDName {
 *      MSP_EXP432P401R_GPIO_LED1 = 0,
 *      MSP_EXP432P401R_GPIO_RGBLED,
 *      MSP_EXP432P401R_PWM_GREENLED,
 *      MSP_EXP432P401R_PWM_BLUELED,
 *
 *      MSP_EXP432P401R_LEDCOUNT
 *  } MSP_EXP432P401R_LEDName;
 *
 *  @endcode
 *
 *  "MSP_EXP432P401R.c"
 *  @code
 *  #include <led.h>
 *
 * #define MSP_EXP432P401R_NA_GPIO_PWMLED    0xFFFF
 *
 *  LED_Object LED_object[MSP_EXP432P401R_LEDCOUNT];

 *  const LED_HWAttrs LED_hwAttrs[MSP_EXP432P401R_LEDCOUNT] = {
 *          {
 *              .gpioIndex = MSP_EXP432P401R_LED1,
 *          },
 *          {
 *              .gpioIndex = MSP_EXP432P401R_LED_RED,
 *          },
 *          {
 *              .gpioIndex = MSP_EXP432P401R_NA_GPIO_PWMLED,
 *          },
 *          {
 *              .gpioIndex = MSP_EXP432P401R_NA_GPIO_PWMLED,
 *          }
 *  };

 *  const LED_Config LED_config[] = {
 *      {
 *          .object =  &LED_object[0],
 *          .hwAttrs = &LED_hwAttrs[0],
 *      },
 *      {
 *          .object =  &LED_object[1],
 *          .hwAttrs = &LED_hwAttrs[1],
 *      },
 *      {
 *          .object =  &LED_object[2],
 *          .hwAttrs = &LED_hwAttrs[2],
 *      },
 *      {
 *          .object =  &LED_object[3],
 *          .hwAttrs = &LED_hwAttrs[3],
 *      },
 *      {NULL, NULL},
 *  };
 *  @endcode
 *  LED_config is implemented in the application with each entry being an
 *  instance of a implementation. Last instance is marked by NULL terminated
 *  LED_config structure inside config array.
 *
 *  ##Setting up GPIO configurations #
 *  The following code snippet shows how a GPIO pin controlling an LED is
 *  configured.
 *  The following definitions are in
 *  "MSP_EXP432P401R.h" and "MSP_EXP432P401R.c" respectively. This
 *  example uses GPIO pins 1.0 and 2.0 which control LED1 and RED LED on
 *  launchpad respectively.
 *
 *  "MSP_EXP432P401R.h"
 *  @code
 *  typedef enum MSP_EXP432P401R_GPIOName {
 *      MSP_EXP432P401R_S1 = 0,
 *      MSP_EXP432P401R_S2,
 *      MSP_EXP432P401R_LED1,
 *      MSP_EXP432P401R_LED_RED,
 *
 *      MSP_EXP432P401R_GPIOCOUNT
 *  } MSP_EXP432P401R_GPIOName;
 *  @endcode
 *
 *  "MSP_EXP432P401R.c"
 *  @code
 *  #include <gpio.h>
 *  GPIO_PinConfig gpioPinConfigs[] = {
 *      GPIOMSP432_P1_1 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING,
 *      GPIOMSP432_P1_4 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING,
 *      GPIOMSP432_P1_0 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
 *      GPIOMSP432_P2_0 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
 *  }
 *
 *  @endcode
 *
 *  ## Opening a PWM_Handle Handle #
 *  The PWM module instance has to be created for servicing requests to set
 *  light intensity. The pwm parameters like period, duty and polarity can be
 *  set by application before opening a pwm handle and embedding inside LED
 *  params structure before opening an handle for PWM controlled LED.
 *
 *  To let an LED controlled by PWM, GPIO settings for that particular pin has
 *  to be commented inside gpioPinConfigs[] structure. It shall also be known
 *  beforehand which PWM timer can be muxed to that pin.
 *  In "MSP_EXP432P401R.c" file
 *  @code
 *  GPIO_PinConfig gpioPinConfigs[] = {
 *
 *    GPIOMSP432_P1_1 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING,
 *
 *    GPIOMSP432_P1_4 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING,
 *
 *    GPIOMSP432_P1_0 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
 *
 *    //GPIOMSP432_P2_0 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
 *
 *    //GPIOMSP432_P2_1 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,

 *    //GPIOMSP432_P2_2 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW
 *  };
 *  @endcode
 *
 *  In application code, testled.c
 *  @code
 *  #include <ti/drivers/PWM.h>
 *
 *  PWM_Handle pwmHandle = NULL;
 *  PWM_Params params;
 *
 *  PWM_Params_init(&params);
 *  params.period = pwmPeriod;
 *  pwmHandle = PWM_open(Board_PWM0, &params);
 *  @endcode
 *
 *  ## Opening a LED with default parameters #
 *
 *  @code
 *  #include <LED.h>
 *
 *  LED_Handle LEDHandle;
 *
 *  LED_Params_init(&ledParams);
 *  ledHandle = LED_open(Board_LED0, &ledParams);
 *  @endcode
 *  ============================================================================
 */


#ifndef LED_H_
#define LED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* POSIX Header files */
#include <pthread.h>
#include <time.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/PWM.h>


#define LED_BRIGHTNESS_MAX      100U  /* Max brightness in % is 100%*/
#define LED_BRIGHTNESS_MIN      0U    /* Max brightness in % is 0%*/

#define LED_ON                  1U
#define LED_OFF                 0U

/*!
*  @brief    LED types based on control source
*
*  A LED can be controlled by GPIO or PWM. Only a PWM controlled LED can
*  be operated to show brightness variation.
*/
typedef enum LED_Type {
    LED_GPIO_CONTROLLED =  0,
    LED_PWM_CONTROLLED
} LED_Type;

/*!
*  @brief    LED State
*
*  A LED can be in OFF, ON or BLINKING state
*
*  State of particular LED may be tied with a warning/alert in system
*  which a thread/task may want to know.
*/
typedef enum LED_State {
    LED_STATE_OFF =  0,
    LED_STATE_ON,
    LED_STATE_BLINKING
} LED_State;

/*!
 *  @brief    LED configuration
 *
 *  The LED_Config structure contains a set of pointers used to characterize
 *  the LED driver implementation.
 *
 *  This structure needs to be defined and provided by the application and will
 *  be NULL terminated.
 */
typedef struct LED_Config {
    /*! Pointer to an uninitialized user defined LED_Object struct */
    void        *object;
    /*! Pointer to a driver specific hardware attributes structure */
    void const  *hwAttrs;
}LED_Config;

/*!
 *  @brief    A handle that is returned from a LED_open() call.
 */
typedef struct LED_Config    *LED_Handle;

/*!
 *  @brief    Hardware specific settings for a LED module.
 *
 *  This structure should be defined and provided by the application. The
 *  gpioIndex should be defined in accordance of the GPIO driver. The pin
 *  must be configured as GPIO_CFG_INPUT and GPIO_CFG_IN_INT_FALLING.
 */
typedef struct LED_HWAttrs {
    unsigned int    gpioIndex;       /*!< GPIO configuration index */
} LED_HWAttrs;

/*!
 *  @brief    LED Object structure
 *
*  The application must not access any member variables of this structure!
 */
typedef struct LED_Object {
    LED_Type        ledType;      /*!< GPIO or PWM controlled*/
    PWM_Handle      pwmHandle;    /*!< Use to call control APIs for PWM LED*/
    uint32_t        pwmPeriod;    /*!< pwmPeriod(us) of controlling PWM*/
    timer_t         timer;        /*!< timer used for blinking*/
    uint16_t        blinkPeriod;  /*!< Blinkperiod(ms), 0 for non-blinking LED*/
    uint16_t        brightness;   /*!< Varying min-max(0-100%) for PWM LED)*/
    LED_State       state;        /*!< Current State of LED*/
    LED_State       rawState;     /*!< rawState maintains actual state On or Off
                                       while blinking which is super state*/
} LED_Object;

/*!
 *  @brief  LED Parameters
 *
 *  LED parameters are used with the LED_open() call. Default values for
 *  these parameters are set using LED_Params_init(). It contains brightness
 *  field which will be used to control brightness of a LED and also blink
 *  period if user wants to set LED in blinking mode.
 *
 *  @sa     LED_Params_init()
 */
typedef struct LED_Params {
    LED_Type    ledType;        /*!< may be either GPIP or PWM controlled*/
    PWM_Handle  pwmHandle;      /*!< for GPIO led it is NULL*/
    uint32_t    pwmPeriod;      /*!< pwmPeriod(us) of controlling PWM*/
    uint16_t    brightness;     /*!< may vary from 0-100% for PWM LED*/
    LED_State   setState;       /*!< request to set a LED state(eg. blinking)*/
    uint16_t    blinkPeriod;    /*!< param to set blink period(in ms)*/
} LED_Params;

/*!
 *  @brief  Function to close a LED specified by the LED handle
 *
 *  The LED close API to close the LED instance specified by ledHandle.
 *
 *  This call will destruct associated timer if running and turn off LED, can
 *  be hooked up with Power Down sequence in system.
 *
 *  @pre    LED_open() had to be called first.
 *
 *  @param  ledHandle    A LED_Handle returned from LED_open()
 *
 */
extern void LED_close(LED_Handle ledHandle);

/*!
 *  @brief  Function to get LED state.
 *
 *  This function may be useful in scenarios if a LED state(ON/OFF/BLINKING) is
 *  tied with some system warning/alerts
 *
 *  @param  ledHandle    A LED_Handle returned from LED_open()
 *
 *  @return  The LED State
 */
extern LED_State LED_getState(LED_Handle ledHandle);

/*!
 *  @brief  Function to initialize LED driver.
 *
 *  This function will initialize the LED driver.
 */
extern void LED_init();

/*!
 *  @brief  Function to open an instance of LED
 *
 *  Function to initialize a given LED specified by the particular
 *  index value.LED parameters must be setup and PWM handle opened(only for
 *  PWM controlled LED type) before this call.
 *
 *  User may request to set LED into particular state, for ex: open an handle
 *  and start blinking that LED or just turn in ON or OFF
 *
 *  @pre    LED_init() has to be called first
 *
 *  @param  ledIndex       Logical led number  indexed into
 *                         the LED_config table
 *
 *
 *  @param  *params        A pointer to LED_Params structure. If NULL, it
 *                         will use default values.
 *
 *  @return  A LED_Handle on success, or a NULL on failure.
 *
 *  @sa      LED_init()
 *  @sa      LED_Params_init()
 *  @sa      LED_close()
 */
LED_Handle LED_open(unsigned int ledIndex, LED_Params *params);

/*!
 *  @brief  Function to initialize a LED_Params struct to its defaults
 *
 *  @param  params      A pointer to LED_Params structure for
 *                      initialization.
 *
 *  Default values are:
 *  ledType    = LED_GPIO_CONTROLLED
 *  brightness = LED_BRIGHTNESS_MAX
 */
extern void LED_initParams(LED_Params *params);

/*!
 *  @brief  Function to set brightness level of a LED
 *
 *  Ignores brightness settings if LED is not PWM controlled
 *
 *  @param  ledHandle    A LED handle
 *
 *  @param  level        Brightness level in terms of percentage 0-100 %
 *
 *  @return true on success or false upon failure.
 */
extern bool LED_setBrightnessLevel(LED_Handle ledHandle, uint16_t level);

/*!
 *  @brief  Function to turn off a LED
 *
 *  @param  ledHandle    A LED handle
 *
 */
extern void LED_setOff(LED_Handle ledHandle);

/*!
 *  @brief  Function to turn on a LED
 *
 *  @param  ledHandle    A LED handle
 *
 *  @param  brightness   Brightness level in terms of percentage 0-100 %
 *
 *  @return true on success or false upon failure.
 */
extern bool LED_setOn(LED_Handle ledHandle, uint16_t brightness);

/*!
 *  @brief  Function to start a LED blinking
 *
 *  @param  ledHandle    A LED handle
 *
 *  @param  blinkPeriod  blinkPeriod(in ms) with which LED will blink
 *                       maximum togglePeriod will be 65 seconds(=65535/0xFFFF)
 *
 */
extern void LED_startBlinking(LED_Handle ledHandle, uint16_t blinkPeriod);

/*!
 *  @brief  Function to stop a LED blinking
 *
 *  @param  ledHandle    A LED handle
 *
 */
extern void LED_stopBlinking(LED_Handle ledHandle);

/*!
 *  @brief  Function to toggle a LED
 *
 *  @param  ledHandle    A LED handle
 *
 */
extern void LED_toggle(LED_Handle ledHandle);

#ifdef __cplusplus
}
#endif

#endif /* LED_H_ */
