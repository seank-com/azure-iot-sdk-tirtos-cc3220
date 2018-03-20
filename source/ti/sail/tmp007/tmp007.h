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
 *  @file       tmp007.h
 *
 *  @brief      TMP007 driver
 *
 *  The TMP007 header file should be included in an application as
 *  follows:
 *  @code
 *  #include <tmp007.h>
 *  @endcode
 *
 *  # Operation #
 *  The TMP007 driver simplifies using a TMP007 sensor to perform temperature
 *  readings. The board's I2C peripheral and pins must be configured and then
 *  initialized by using I2C_init(). Similarly, any GPIO pins must be
 *  configured and then initialized by using GPIO_init(). A TMP007_config
 *  array should be defined by the application. The TMP007_config array should
 *  contain pointers to a defined TMP007_HWAttrs and allocated array for the
 *  TMP007_Object structures. TMP007_init() must be called prior to using
 *  TMP007_open().
 *
 *  The APIs in this driver serve as an interface to a DPL(Driver Porting Layer)
 *  The specific implementations are responsible for creating all the RTOS
 *  specific primitives to allow for thread-safe operation.
 *
 *  For accurate operation, calibration may be necessary. Refer to SBOU142.pdf
 *  for calibration instructions.
 *
 *  This driver has no dynamic memory allocation.
 *
 *  ## Defining TMP007_Config, TMP007_Object and TMP007_HWAttrs #
 *  Each structure must be defined by the application. The following
 *  example is for a MSP432 in which two TMP007 sensors are setup.
 *  The following declarations are placed in "MSP_EXP432P401R.h"
 *  and "MSP_EXP432P401R.c" respectively. How the gpioIndices are defined
 *  are detailed in the next example.
 *
 *  "MSP_EXP432P401R.h"
 *  @code
 *  typedef enum MSP_EXP432P491RLP_TMP007Name {
 *      TMP007_ROOMTEMP = 0, // Sensor measuring room temperature
 *      TMP007_OUTDOORTEMP,  // Sensor measuring outside temperature
 *      MSP_EXP432P491RLP_TMP007COUNT
 *  } MSP_EXP432P491RLP_TMP007Name;
 *
 *  @endcode
 *
 *  "MSP_EXP432P401R.c"
 *  @code
 *  #include <tmp007.h>
 *
 *  TMP007_Object TMP007_object[MSP_EXP432P491RLP_TMP007COUNT];
 *
 *  const TMP007_HWAttrs TMP007_hwAttrs[MSP_EXP432P491RLP_TMP007COUNT] = {
 *  {
 *      .slaveAddress = TMP007_SA1, // 0x40
 *      .gpioIndex = MSP_EXP432P401R_TMP007_0,
 *  },
 *  {
 *      .slaveAddress = TMP007_SA2, // 0x41
 *      .gpioIndex = MSP_EXP432P401R_TMP007_1,
 *  },
 *  };
 *
 *  const TMP007_Config TMP007_config[] = {
 *  {
 *      .hwAttrs = (void *)&TMP007_hwAttrs[0],
 *      .objects = (void *)&TMP007_object[0],
 *  },
 *  {
 *      .hwAttrs = (void *)&TMP007_hwAttrs[1],
 *      .objects = (void *)&TMP007_object[1],
 *  },
 *  {NULL, NULL},
 *  };
 *  @endcode
 *
 *  ##Setting up GPIO configurations #
 *  The following example is for a MSP432 in which two TMP007 sensors
 *  each need a GPIO pin for alarming. The following definitions are in
 *  "MSP_EXP432P401R.h" and "MSP_EXP432P401R.c" respectively. This
 *  example uses GPIO pins 1.5 and 4.3. The GPIO_CallbackFxn table must
 *  contain as many indices as GPIO_CallbackFxns that will be specified by
 *  the application. For each ALERT pin used, an index should be allocated
 *  as NULL.
 *
 *  "MSP_EXP432P401R.h"
 *  @code
 *  typedef enum MSP_EXP432P401R_GPIOName {
 *      MSP_EXP432P401R_TMP007_0, // ALERT pin for the room temperature
 *      MSP_EXP432P401R_TMP007_1, // ALERT pin for the outdoor temperature
 *      MSP_EXP432P401R_GPIOCOUNT
 *  } MSP_EXP432P401R_GPIOName;
 *  @endcode
 *
 *  "MSP_EXP432P401R.c"
 *  @code
 *  #include <gpio.h>
 *
 *  GPIO_PinConfig gpioPinConfigs[] = {
 *      GPIOMSP432_P1_5 | GPIO_CFG_INPUT | GPIO_CFG_IN_INT_FALLING,
 *      GPIOMSP432_P4_3 | GPIO_CFG_INPUT | GPIO_CFG_IN_INT_FALLING
 *  };
 *
 *  GPIO_CallbackFxn gpioCallbackFunctions[] = {
 *      NULL,
 *      NULL
 *  };
 *  @endcode
 *
 *  ## Opening a I2C Handle #
 *  The I2C controller must be in blocking mode. This will seamlessly allow
 *  for multiple I2C endpoints without conflicting callback functions. The
 *  I2C_open() call requires a configured index. This index must be defined
 *  by the application in accordance to the I2C driver. The default transfer
 *  mode for an I2C_Params structure is I2C_MODE_BLOCKING. In the example
 *  below, the transfer mode is set explicitly for clarity. Additionally,
 *  the TMP007 hardware is capable of communicating at 400kHz. The default
 *  bit rate for an I2C_Params structure is I2C_100kHz.
 *
 *  @code
 *  #include <ti/drivers/I2C.h>
 *
 *  I2C_Handle      i2cHandle;
 *  I2C_Params      i2cParams;
 *
 *  I2C_Params_init(&i2cParams);
 *  i2cParams.transferMode = I2C_MODE_BLOCKING;
 *  i2cParams.bitRate = I2C_400kHz;
 *  i2cHandle = I2C_open(someI2C_configIndexValue, &i2cParams);
 *  @endcode
 *
 *  ## Opening a TMP007 sensor with default parameters #
 *  The TMP007_open() call must be made in a thread context.
 *
 *  @code
 *  #include <tmp007.h>
 *
 *  TMP007_Handle tmp007Handle;
 *
 *  TMP007_init();
 *  tmp007Handle = TMP007_open(TMP007_ROOMTEMP, i2cHandle, NULL);
 *  @endcode
 *
 *  ## Opening a TMP007 sensor to ALERT #
 *  In the following example, a callback function is specified in the
 *  tmp007Params structure. This indicates to the module that
 *  the ALERT pin will be used. Additionally, a user specific argument
 *  is passed in. The sensor will assert the ALERT pin whenever the
 *  temperature exceeds 35 (C). The low limit is ignored. No ALERT will be
 *  generated until TMP007_enableAlert() is called.
 *
 *  @code
 *  #include <tmp007.h>
 *
 *  TMP007_Handle tmp007Handle;
 *  TMP007_Params tmp007Params;
 *
 *  tmp007Params.callback = gpioCallbackFxn;
 *  tmp007Handle = TMP007_open(Board_TMP007_ROOMTEMP, i2cHandle, &tmp007Params);
 *
 *  TMP007_setObjTempLimit(tmp007Handle, TMP007_CELSIUS, 35, TMP007_IGNORE);
 *  TMP007_enableAlert(tmp007Handle);
 *  @endcode
 *  ============================================================================
 */

#ifndef TMP007_H_
#define TMP007_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Driver Header files */
#include <ti/drivers/I2C.h>
#include <ti/drivers/GPIO.h>

/* TMP007 Register Addresses */
#define TMP007_VOBJ          0x0000  /*! Sensor voltage result register      */
#define TMP007_DIE_TEMP      0x0001  /*! DIE Temp result register            */
#define TMP007_CONFIG        0x0002  /*! Configuration register              */
#define TMP007_OBJ_TEMP      0x0003  /*! Object Temp Result Register         */
#define TMP007_STATUS        0x0004  /*! Status register                     */
#define TMP007_MASK_ENABLE   0x0005  /*! Status Mask and Enable Register     */
#define TMP007_OBJ_HI_LIM    0x0006  /*! Object Temp High limit register     */
#define TMP007_OBJ_LO_LIM    0x0007  /*! Object Temp Low limit register      */
#define TMP007_DIE_HI_LIM    0x0008  /*! DIE Temp High limit register        */
#define TMP007_DIE_LO_LIM    0x0009  /*! DIE Temp Low limit register         */
#define TMP007_S0            0x000A  /*! S0 coefficient register             */
#define TMP007_A0            0x000B  /*! A0 coefficient register             */
#define TMP007_A1            0x000C  /*! A1 coefficient register             */
#define TMP007_B0            0x000D  /*! B0 coefficient register             */
#define TMP007_B1            0x000E  /*! B1 coefficient register             */
#define TMP007_B2            0x000F  /*! B2 coefficient register             */
#define TMP007_C             0x0010  /*! C coefficient register              */
#define TMP007_TC0           0x0011  /*! TC0 coefficient register            */
#define TMP007_TC1           0x0012  /*! TC1 coefficient register            */
#define TMP007_DEVICEID      0x001F  /*! Manufacturer ID register            */
#define TMP007_MEMORY        0x002A  /*! Memory access register              */

/* Status Mask and Enable Register Bits */
#define TMP007_SMER_ALRTEN   0x8000  /*! Alert Flag Enable Bit               */
#define TMP007_SMER_CRTEN    0x4000  /*! Tem Conversion Ready Enable         */
#define TMP007_SMER_OHEN     0x2000  /*! Object Temp High Limit Enable       */
#define TMP007_SMER_OLEN     0x1000  /*! Object Temp Low Limit Enable        */
#define TMP007_SMER_LHEN     0x0800  /*! DIE Temp High Limit Enable          */
#define TMP007_SMER_LLEN     0x0400  /*! DIE Temp Low Limit Enable           */
#define TMP007_SMER_DVEN     0x0200  /*! Data Invalid Flag Enable Bit        */
#define TMP007_SMER_MEMC_EN  0x0100  /*! Memory Corrupt Enable Bit           */

/* TMP007 Status Register Bits */
#define TMP007_STAT_ALRTF    0x8000  /*! Cumulative Alert Flag               */
#define TMP007_STAT_CRTF     0x4000  /*! Conversion Ready Flag               */
#define TMP007_STAT_OHF      0x2000  /*! Object Temp High Limit Flag         */
#define TMP007_STAT_OLF      0x1000  /*! Object Temp Low Limit Flag          */
#define TMP007_STAT_LHF      0x0800  /*! Local Temp High Limit Flag          */
#define TMP007_STAT_LLF      0x0400  /*! Local Temp Low Limit Flag           */
#define TMP007_STAT_nDVF     0x0200  /*! Data Invalid Flag                   */
#define TMP007_STAT_MCRPT    0x0100  /*! Memory Corrupt Flag                 */
#define TMP007_STAT_DOF      0x0080  /*! Data Overflow                       */

/*!
 *  @brief    Ignore temperature limit define
 *
 *  TMP007_IGNORE should be used to ignore or unset a temperature limit.
 *
 *  In the following example, the object temperature limits are set and reset:
 *  @code
 *  TMP007_setObjTempLimit(handle, TMP007_CELSIUS, 40, 10);
 *  TMP007_enableAlert(handle);
 *  //Additional application code...
 *  TMP007_setObjTempLimit(handle, TMP007_CELSIUS, TMP007_IGNORE, TMP007_IGNORE);
 *  @endcode
 */
#define TMP007_IGNORE         0xFFFF

/*!
 *  @brief    A handle that is returned from a TMP007_open() call.
 */
typedef struct TMP007_Config    *TMP007_Handle;

/*!
 *  @brief    TMP007 ADC conversion settings
 *
 *  This enumeration defines the number of ADC conversions performed before an
 *  average is computed and stored in the hardware registers. More ADC
 *  conversions results in a longer overall conversion time and more accurate
 *  object temperature reading.
 */
typedef enum TMP007_Conversions {
    TMP007_1CONV =    0x0000,  /*!< 1 average per conversion   (0.26 secs)  */
    TMP007_2CONV =    0x0200,  /*!< 2 averages per conversion  (0.51 secs)  */
    TMP007_4CONV =    0x0400,  /*!< 4 averages per conversion  (1.01 secs)  */
    TMP007_8CONV =    0x0600,  /*!< 8 averages per conversion  (2.01 secs)  */
    TMP007_16CONV =   0x0800,  /*!< 16 averages per conversion (4.01 secs)  */
    TMP007_1CONVLP =  0x0A00,  /*!< 1 low power conversion     (1.00 secs)  */
    TMP007_2CONVLP =  0x0C00,  /*!< 2 low power conversions    (4.00 secs)  */
    TMP007_4CONVLP =  0x0E00   /*!< 4 low power conversions    (4.00 secs)  */
} TMP007_Conversions;

/*!
 *  @brief    TMP007 interrupt/comparator settings
 *
 *  The INT/COMP bit controls whether the limit flags are in INTERRUPT (INT)
 *  mode (0) or COMPARATOR (COMP) Mode (1). It controls the behavior of the
 *  limit flags (LH, LL, OH, OL) and the data invalid flag (nDVF) from the
 *  status register. In INT mode, the ALERT condition is maintained until
 *  the status register has been read. In COMP mode, the ALERT pin is
 *  asserted whenever the alert condition occurs, and deasserts without
 *  any external intervention when the alert condition is no longer present.
 */
typedef enum TMP007_IntComp {
    TMP007_IntMode =  0,
    TMP007_CompMode = 0x0020
} TMP007_IntComp;

/*!
 *  @brief    TMP007 I2C slave addresses.
 *
 *  The TMP007 Slave Address is determined by the input to the ADR0 and ADR1
 *  input pins of the TMP007 hardware. A '1' indicates a supply voltage of
 *  up to 7.5 V while '0' indicates ground. In some cases, the ADR0 pin may
 *  be coupled with the SDA or SCL bus to achieve a particular slave address.
 *  TMP007 sensors on the same I2C bus cannot share the same slave address.
 */
typedef enum TMP007_SlaveAddress {
    TMP007_SA1 = 0x40,  /*!< ADR1 = 0, ADR0 = 0   */
    TMP007_SA2 = 0x41,  /*!< ADR1 = 0, ADR0 = 1   */
    TMP007_SA3 = 0x42,  /*!< ADR1 = 0, ADR0 = SDA */
    TMP007_SA4 = 0x43,  /*!< ADR1 = 0, ADR0 = SCL */
    TMP007_SA5 = 0x44,  /*!< ADR1 = 1, ADR0 = 0   */
    TMP007_SA6 = 0x45,  /*!< ADR1 = 1, ADR0 = 1   */
    TMP007_SA7 = 0x46,  /*!< ADR1 = 1, ADR0 = SDA */
    TMP007_SA8 = 0x47   /*!< ADR1 = 1, ADR0 = SCL */
} TMP007_SlaveAddress;

/*!
 *  @brief    TMP007 temperature settings
 *
 *  This enumeration defines the scaling for reading and writing temperature
 *  values with a TMP007 sensor.
 */
typedef enum TMP007_TempScale {
    TMP007_CELSIUS = 0,
    TMP007_KELVIN  = 1,
    TMP007_FAHREN  = 2
} TMP007_TempScale;

/*!
 *  @brief    TMP007 Transient Correct settings
 *
 *  Setting this bit turns on the transient correction--enabling sensor voltage
 *  and object temperature output filtering. This helps reduce error due to
 *  changes in the die temperature. As a general guideline, turn on transient
 *  correction when the die temperature is changing at a rate greater than
 *  1.5 degrees (C) per minute. Transient correction corrects transients up to
 *  approximately 20 degrees (C) per minute.
 */
typedef enum TMP007_TransientCorrect {
    TMP007_TCDisable = 0,
    TMP007_TCEnable =  0x0040
} TMP007_TransientCorrect;

/*!
 *  @brief    TMP007 configuration
 *
 *  The TMP007_Config structure contains a set of pointers used to characterize
 *  the TMP007 driver implementation.
 *
 *  This structure needs to be defined and provided by the application.
 */
typedef struct TMP007_Config {
    /*! Pointer to a driver specific hardware attributes structure */
    void const    *hwAttrs;

    /*! Pointer to an uninitialized user defined TMP007_Object struct */
    void          *object;
} TMP007_Config;

/*!
 *  @brief    Hardware specific settings for a TMP007 sensor.
 *
 *  This structure should be defined and provided by the application. The
 *  gpioIndex should be defined in accordance of the GPIO driver. The pin
 *  must be configured as GPIO_CFG_INPUT and GPIO_CFG_IN_INT_FALLING.
 */
typedef struct TMP007_HWAttrs {
    TMP007_SlaveAddress    slaveAddress;    /*!< I2C slave address */
    unsigned int           gpioIndex;       /*!< GPIO configuration index */
} TMP007_HWAttrs;

/*!
 *  @brief    Members should not be accessed by the application.
 */
typedef struct TMP007_Object {
    I2C_Handle          i2cHandle;
    GPIO_CallbackFxn    callback;
} TMP007_Object;

/*!
 *  @brief  TMP007 Parameters
 *
 *  TMP007 parameters are used with the TMP007_open() call. Default values for
 *  these parameters are set using TMP007_Params_init(). The GPIO_CallbackFxn
 *  should be defined by the application only if the ALERT functionality is
 *  desired. A gpioIndex must be defined in the TMP007_HWAttrs for the
 *  corresponding tmp007Index. The GPIO_CallbackFxn is in the context of an
 *  interrupt handler.
 *
 *  @sa     TMP007_Params_init()
 */
typedef struct TMP007_Params {
    TMP007_Conversions conversions;           /*!< Number of ADC Conversions*/
    TMP007_IntComp intComp;                   /*!< Int/Comp mode            */
    TMP007_TransientCorrect transientCorrect; /*!< Transient Correct setting*/
    GPIO_CallbackFxn callback;                /*!< Pointer to GPIO callback */
} TMP007_Params;

/*!
 *  @brief  Function to close a TMP007 sensor specified by the TMP007 handle
 *
 *  The TMP007 hardware will be placed in a low power state in which ADC
 *  conversions are disabled. If the pin is configured to alarm, the ALERT pin
 *  and GPIO pin interrupts will be disabled. The I2C handle is not affected.
 *
 *  @pre    TMP007_open() had to be called first.
 *
 *  @param  handle    A TMP007_Handle returned from TMP007_open()
 *
 *  @return true on success or false upon failure.
 */
extern bool TMP007_close(TMP007_Handle handle);

/*!
 *  @brief  Function to disable the ALERT pin and GPIO interrupt.
 *
 *  This function will disable the ALERT pin on the specified TMP007 sensor.
 *  Interrupts on the TMP007 specific GPIO index will be disabled. Any
 *  temperature limits set are not affected. This function is not thread
 *  safe when used in conjunction with TMP007_enableAlert() (ie. A thread
 *  is executing TMP007_disableAlert() but preempted by a higher priority
 *  thread that calls TMP007_enableAlert() on the same handle).
 *
 *  @sa     TMP007_enableAlert()
 *
 *  @pre    TMP007_enableAlert() had to be called first in order for this
 *          function to have an effect.
 *
 *  @param  handle    A TMP007_Handle
 *
 *  @return true on success or false upon failure.
 */
extern bool TMP007_disableAlert(TMP007_Handle handle);

/*!
 *  @brief  Function to put TMP007 sensor is low power state.
 *
 *  This function will place the specified TMP007 sensor in a low power state.
 *  All settings are retained. Temperature and voltage readings are invalid
 *  in this state. This function is not thread safe when used in conjunction
 *  with TMP007_enableConversions().
 *
 *  @sa     TMP007_enableConversions()
 *
 *  @pre    TMP007_open() had to be called first.
 *
 *  @param  handle    A TMP007_Handle
 *
 *  @return true on success or false upon failure.
 */
extern bool TMP007_disableConversions(TMP007_Handle handle);

/*!
 *  @brief  Function to enable the ALERT pin and GPIO interrupt.
 *
 *  This function will enable the ALERT pin on the specified TMP007 sensor.
 *  Interrupts on the TMP007 specific GPIO index will be disabled. This
 *  function is not thread safe when used in conjunction with
 *  TMP007_disableAlert() (ie. A thread is executing TMP007_enableAlert()
 *  but preempted by a higher priority thread that calls TMP007_disableAlert()
 *  on the same handle).
 *
 *  @sa     TMP007_disableAlert()
 *  @sa     TMP007_setObjTempLimit()
 *  @sa     TMP007_setDieTempLimit()
 *
 *  @pre    TMP007_setObjTempLimit() or TMP007_setDieTempLimit() had to be
 *          called first. If not called first, the TMP007 sensor will never
 *          generate an interrupt.
 *
 *  @param  handle    A TMP007_Handle
 *
 *  @return true on success or false upon failure.
 */
extern bool TMP007_enableAlert(TMP007_Handle handle);

/*!
 *  @brief  Function to restore TMP007 to normal power operation.
 *
 *  This function will restore ADC conversions on the specified TMP007 sensor.
 *  Temperature and voltage readings may be invalid up to 4 seconds following
 *  this function call. This function is not thread safe when used in
 *  conjunction with TMP007_disableConversions().
 *
 *  @sa     TMP007_disableConversions
 *
 *  @pre    TMP007_disableConversions() had to be called first.
 *
 *  @param  handle    A TMP007_Handle
 *
 *  @return true on success or false upon failure.
 */
extern bool TMP007_enableConversions(TMP007_Handle handle);

/* This function is not intended for application use.
 */
extern bool TMP007_getTemp(TMP007_Handle, TMP007_TempScale unit,
        float *data, uint16_t registerAddress);

/* This function is not intended for application use.
 */
extern bool TMP007_getTempLimit(TMP007_Handle handle, TMP007_TempScale scale,
        float *data, uint16_t registerAddress);

/*!
 *  @brief  Function to read die temperature
 *
 *  This function will return the temperature of the die. Full scale allows a
 *  result of up to (+/-)256 (C).
 *
 *  @param  handle    A TMP007_Handle
 *
 *  @param  scale     A TMP007_TempScale field specifying the temperature
 *                    format.
 *
 *  @param  temp      A pointer to a float in which temperature data will be
 *                    stored in.
 *
 *  @return true on success or false upon failure.
 */
static inline bool TMP007_getDieTemp(TMP007_Handle handle,
        TMP007_TempScale scale, float *temp)
{
    return (TMP007_getTemp(handle, scale, temp, TMP007_DIE_TEMP));
}

/*!
 *  @brief  This function will read the die temperature limits.
 *
 *  @param  handle    A TMP007_Handle
 *
 *  @param  scale     A TMP007_TempScale field specifying the temperature
 *                    return format.
 *
 *  @param  high      A pointer to a float to store the value of the high
 *                    temperature limit.
 *
 *  @param  low       A pointer to a float to store the value of the low
 *                    temperature limit.
 *
 *  @return true on success or false upon failure.
 *
 *  @sa     TMP007_setDieTempLimit()
 */
static inline bool TMP007_getDieTempLimit(TMP007_Handle handle,
        TMP007_TempScale scale, float *high, float *low)
{
    return (TMP007_getTempLimit(handle, scale, high, TMP007_DIE_HI_LIM) &&
            TMP007_getTempLimit(handle, scale, low, TMP007_DIE_LO_LIM));
}

/*!
 *  @brief  Function to read object temperature
 *
 *  This function will return the temperature of the object in the field
 *  of view of the specified TMP007 sensor. Full scale allows a result
 *  of up to (+/-)256 (C).
 *
 *  @param  handle    A TMP007_Handle
 *
 *  @param  scale     A TMP007_TempScale field specifying the temperature
 *                    format.
 *
 *  @param  temp      A pointer to a float in which temperature data will be
 *                    stored in.
 *
 *  @return true on success or false upon failure.
 */
static inline bool TMP007_getObjTemp(TMP007_Handle handle,
        TMP007_TempScale scale, float *temp)
{
    return (TMP007_getTemp(handle, scale, temp, TMP007_OBJ_TEMP));
}

/*!
 *  @brief  This function will read the object temperature limits.
 *
 *  @param  handle    A TMP007_Handle
 *
 *  @param  scale     A TMP007_TempScale field specifying the temperature
 *                    return format.
 *
 *  @param  high      A pointer to a float to store the value of the high
 *                    temperature limit.
 *
 *  @param  low       A pointer to a float to store the value of the low
 *                    temperature limit.
 *
 *  @return true on success or false upon failure.
 *
 *  @sa     TMP007_setObjTempLimit()
 */
static inline bool TMP007_getObjTempLimit(TMP007_Handle handle,
        TMP007_TempScale scale, float *high, float *low)
{
    return (TMP007_getTempLimit(handle, scale, high, TMP007_OBJ_HI_LIM) &&
            TMP007_getTempLimit(handle, scale, low, TMP007_OBJ_LO_LIM));
}

/*!
 *  @brief  Function to initialize TMP007 driver.
 *
 *  This function will initialize the TMP007 driver.
 */
extern void TMP007_init();

/*!
 *  @brief  Function to open a given TMP007 sensor
 *
 *  Function to initialize a given TMP007 sensor specified by the particular
 *  index value. This function must be called from a thread context.
 *  If one intends to use the ALERT pin, a callBack function must be specified
 *  in the TMP007_Params structure. Additionally, a gpioIndex must be setup
 *  and specified in the TMP007_HWAttrs structure.
 *
 *  The I2C controller must be operating in BLOCKING mode. Failure to ensure
 *  the I2C_Handle is in BLOCKING mode will result in undefined behavior.
 *
 *  The user should ensure that each sensor has its own slaveAddress,
 *  gpioIndex (if alarming) and tmp007Index.
 *
 *  @pre    TMP007_init() has to be called first
 *
 *  @param  tmp007Index    Logical sensor number for the TMP007 indexed into
 *                         the TMP007_config table
 *
 *  @param  i2cHandle      An I2C_Handle opened in BLOCKING mode
 *
 *  @param  *params        A pointer to TMP007_Params structure. If NULL, it
 *                         will use default values.
 *
 *  @return  A TMP007_Handle on success, or a NULL on failure.
 *
 *  @sa      TMP007_init()
 *  @sa      TMP007_Params_init()
 *  @sa      TMP007_close()
 */
extern TMP007_Handle TMP007_open(unsigned int tmp007Index,
        I2C_Handle i2cHandle, TMP007_Params *params);

/*!
 *  @brief  Function to initialize a TMP007_Params struct to its defaults
 *
 *  @param  params      A pointer to TMP007_Params structure for
 *                      initialization.
 *
 *  Default values are:
 *        conversions       = TMP007_4CONV
 *        intComp           = TMP007_IntMode
 *        transientCorrect  = TMP007_TCEnable
 *        callback          = NULL
 */
extern void TMP007_Params_init(TMP007_Params *params);

/*!
 *  @brief  Read function for advanced users.
 *
 *  @param  handle             A TMP007_Handle
 *
 *  @param  registerAddress    Register address
 *
 *  @param  data               A pointer to a data register in which received
 *                             data will be written to. Must be 16 bits.
 *
 *  @return true on success or false upon failure.
 */
extern bool TMP007_readRegister(TMP007_Handle handle,
        uint16_t *data, uint8_t registerAddress);

/*!
 *  @brief  Function to read a TMP007 sensor's status register.
 *
 *  @param  handle    A TMP007_Handle
 *
 *  @param  data      A pointer to an uint16_t data buffer in which received
 *                    data will be written.
 *
 *  @return true on success or false upon failure.
 */
static inline bool TMP007_readStatus(TMP007_Handle handle, uint16_t *data)
{
    return (TMP007_readRegister(handle, data, TMP007_STATUS));
}

/*!
 *  @brief  Function to read a TMP007 sensor's voltage register.
 *
 *  This function will read the sensor voltage register. This value is the
 *  digitized infrared sensor voltage output. The received data has a
 *  resolution of 156.25 nV/LSB and should be scaled appropriately.
 *
 *  @param  handle    A TMP007_Handle
 *
 *  @param  data      A uint16_t pointer to a storage address. This
 *                    value may range from (+/-) 5.21 mV.
 *
 *  @return true on success or false upon failure.
 */
static inline bool TMP007_readVoltage(TMP007_Handle handle,
                int16_t *data)
{
    return (TMP007_readRegister(handle, (uint16_t *)data, TMP007_VOBJ));
}

/* This function is not intended for application use. */
extern bool TMP007_setTempLimit(TMP007_Handle handle, TMP007_TempScale scale,
        float data, uint16_t bitMask, uint8_t registerAddress);

/*!
 *  @brief  This function will set the die temperature limits.
 *
 *  This function will write the specified high and low die limits to the
 *  specified TMP007 sensor. The low limit will only flag when INT mode is
 *  enabled. In COMP mode, the low limit flag will always read 0 and therefore
 *  there will never be an ALERT generated on a low temperature limit. This
 *  function may be called multiple times to adjust or disable the limits.
 *
 *  @sa     TMP007_enableAlert()
 *
 *  @pre    TMP007_open() had to be called first.
 *
 *  @param  handle    A TMP007_Handle
 *
 *  @param  scale     A TMP007_TempScale field specifying the temperature
 *                    return format.
 *
 *  @param  high      A float that specifies the high limit. This value is
 *                    limited to (+/-) 256 (C) with 0.5 (C) precision.
 *                    Use TMP007_IGNORE as an argument if a high limit is
 *                    not desired.
 *
 *  @param  low       A float that specifies the low limit. This value is
 *                    limited to (+/-) 256 (C) with 0.5 (C) precision.
 *                    Use TMP007_IGNORE as an argument if a low limit is
 *                    not desired.
 *
 *  @return true on success or false upon failure.
 */
static inline bool TMP007_setDieTempLimit(TMP007_Handle handle,
        TMP007_TempScale scale, float high, float low)
{
    return (TMP007_setTempLimit(handle, scale, high,
                    TMP007_SMER_LHEN, TMP007_DIE_HI_LIM) &&
            TMP007_setTempLimit(handle, scale, low,
                    TMP007_SMER_LLEN, TMP007_DIE_LO_LIM));
}

/*!
 *  @brief  This function will set the object temperature limits.
 *
 *  This function will write the specified high and low object limits to the
 *  specified TMP007 sensor. The low limit will only flag when INT mode is
 *  enabled. In COMP mode, the low limit will always read 0. This function may
 *  be called multiple times to adjust or disable the limits.
 *
 *  @sa     enableAlert()
 *
 *  @pre    TMP007_open() had to be called first.
 *
 *  @param  handle    A TMP007_Handle
 *
 *  @param  scale     A TMP007_TempScale field specifying the temperature
 *                    return format.
 *
 *  @param  high      A float that specifies the high limit. This value is
 *                    limited to (+/-) 256 (C) with 0.5 (C) precision.
 *                    Use TMP007_IGNORE as an argument if a high limit is
 *                    not desired.
 *
 *  @param  low       A float that specifies the low limit. This value is
 *                    limited to (+/-) 256 (C) with 0.5 (C) precision.
 *                    Use TMP007_IGNORE as an argument if a low limit is
 *                    not desired.
 *
 *  @return true on success or false upon failure.
 */
static inline bool TMP007_setObjTempLimit(TMP007_Handle handle,
        TMP007_TempScale scale, float high, float low)
{
    return (TMP007_setTempLimit(handle, scale, high,
                    TMP007_SMER_OHEN, TMP007_OBJ_HI_LIM) &&
            TMP007_setTempLimit(handle, scale, low,
                    TMP007_SMER_OLEN, TMP007_OBJ_LO_LIM));
}

/*!
 *  @brief  Write function for advanced users. This is not thread safe.
 *
 *  When writing to a handle, it is possible to overwrite data written
 *  by another thread.
 *
 *  For example: Thread A and B are writing to the configuration register.
 *  Thread A has a higher priority than Thread B. Thread B is running and
 *  reads the configuration register. Thread A then preempts Thread B and reads
 *  the configuration register, performs a logical OR and writes to the
 *  configuration register. Thread B then resumes execution and performs its
 *  logical OR and writes to the configuration register--overwriting the data
 *  written by Thread A.
 *
 *  Such instances can be prevented through the use of Semaphores. Below is an
 *  example which utilizes an initialized Semaphore_handle, tmp007Lock.
 *
 *  @code
 *  @code
 *  if (0 == sem_wait(&tmp007Lock)) 
 *  {  
 *      //Perform read/write operations
 *      sem_post(tmp007Lock);
 *  }
 *  else
 *  {
 *      //handle error scenario
 *  }
 *  @endcode
 *  @endcode
 *
 *  @param  handle             A TMP007_Handle
 *
 *  @param  data               A uint16_t containing the 2 bytes to be written
 *                             to the TMP007 sensor.
 *
 *  @param  registerAddress    Register address.

 *  @return true on success or false upon failure.
 */
extern bool TMP007_writeRegister(TMP007_Handle handle, uint16_t data,
        uint8_t registerAddress);

#ifdef __cplusplus
}
#endif

#endif /* TMP007_H_ */
