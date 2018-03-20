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
 *  @file       tmp006.h
 *
 *  @brief      TMP006 driver
 *
 *  The TMP006 header file should be included in an application as
 *  follows:
 *  @code
 *  #include <ti/sail/tmp006/tmp006.h>
 *  @endcode
 *
 *  # Operation #
 *  The TMP006 driver simplifies using a TMP006 sensor to perform temperature
 *  readings. The board's I2C peripheral and pins must be configured and then
 *  initialized by using I2C_init(). Similarly, any GPIO pins must be
 *  configured and then initialized by using GPIO_init(). A TMP006_config
 *  array should be defined by the application. The TMP006_config array should
 *  contain pointers to a defined TMP006_HWAttrs and allocated array for the
 *  TMP006_Object structures. TMP006_init() must be called prior to using
 *  TMP006_open().
 *
 *  The APIs in this driver serve as an interface to a DPL(Driver Porting Layer)
 *  The specific implementations are responsible for creating all the RTOS
 *  specific primitives to allow for thread-safe operation.
 *
 *  For accurate operation, calibration may be necessary. Refer to SBOU142.pdf
 *  for calibration instructions.(this is valid even for TMP006)
 *
 *  The TMP006 out of the box does provide object temperature API.
 *  To the calculate the object temperature the algorithm specified in the 
 *  below link is used.But the API may need calibration and has not been
 *  tested for all scenarios/temperatures
 *  http://processors.wiki.ti.com/index.php/SensorTag_User_Guide
 *  http://www.ti.com/lit/ug/sbou107/sbou107.pdf
 *
 *  This driver has no dynamic memory allocation.
 *
 *  ## Defining TMP006_Config, TMP006_Object and TMP006_HWAttrs #
 *  Each structure must be defined by the application. The following
 *  example is for a MSP432 in which two TMP006 sensors are setup.
 *  The following declarations are placed in "MSP_EXP432P401R.h"
 *  and "MSP_EXP432P401R.c" respectively. How the gpioIndices are defined
 *  are detailed in the next example.
 *
 *  "MSP_EXP432P401R.h"
 *  @code
 *  typedef enum MSP_EXP432P491RLP_TMP006Name {
 *      TMP006_ROOMTEMP = 0, // Sensor measuring room temperature
 *      TMP006_OUTDOORTEMP,  // Sensor measuring outside temperature
 *      MSP_EXP432P491RLP_TMP006COUNT
 *  } MSP_EXP432P491RLP_TMP006Name;
 *
 *  @endcode
 *
 *  "MSP_EXP432P401R.c"
 *  @code
 *  #include <ti/sail/tmp006/tmp006.h>
 *
 *  TMP006_Object TMP006_object[MSP_EXP432P491RLP_TMP006COUNT];
 *
 *  const TMP006_HWAttrs TMP006_hwAttrs[MSP_EXP432P491RLP_TMP006COUNT] = {
 *  {
 *      .slaveAddress = TMP006_SA1, // 0x40
 *      .gpioIndex = MSP_EXP432P401R_TMP006_0,
 *  },
 *  {
 *      .slaveAddress = TMP006_SA2, // 0x41
 *      .gpioIndex = MSP_EXP432P401R_TMP006_1,
 *  },
 *  };
 *
 *  const TMP006_Config TMP006_config[] = {
 *  {
 *      .hwAttrs = (void *)&TMP006_hwAttrs[0],
 *      .objects = (void *)&TMP006_object[0],
 *  },
 *  {
 *      .hwAttrs = (void *)&TMP006_hwAttrs[1],
 *      .objects = (void *)&TMP006_object[1],
 *  },
 *  {NULL, NULL},
 *  };
 *  @endcode
 *
 *  ##Setting up GPIO configurations #
 *  The following example is for a MSP432 in which two TMP006 sensors
 *  each need a GPIO pin for alarming. The following definitions are in
 *  "MSP_EXP432P401R.h" and "MSP_EXP432P401R.c" respectively. This
 *  example uses GPIO pins 1.5 and 4.3. The GPIO_CallbackFxn table must
 *  contain as many indices as GPIO_CallbackFxns that will be specified by
 *  the application. For each data ready pin used, an index should be allocated
 *  as NULL.
 *
 *  "MSP_EXP432P401R.h"
 *  @code
 *  typedef enum MSP_EXP432P401R_GPIOName {
 *      MSP_EXP432P401R_TMP006_0, // ALERT pin for the room temperature
 *      MSP_EXP432P401R_TMP006_1, // ALERT pin for the outdoor temperature
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
 *  the TMP006 hardware is capable of communicating at 400kHz. The default
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
 *  ## Opening a TMP006 sensor with default parameters #
 *  The TMP006_open() call must be made in a task context.
 *
 *  @code
 *  #include <tmp006.h>
 *
 *  TMP006_Handle tmp006Handle;
 *
 *  TMP006_init();
 *  tmp006Handle = TMP006_open(TMP006_ROOMTEMP, i2cHandle, NULL);
 *  @endcode
 *
 *  ## Opening a TMP006 sensor to Data Ready #
 *  In the following example, a callback function is specified in the
 *  tmp006Params structure. This indicates to the module that
 *  the data ready pin will be used. Additionally, a user specific argument
 *  is passed in. The sensor will assert the data ready pin whenever the
 *  conversion is complete and data is ready. No data ready will be generated 
 *  until TMP006_enableDataReady() is called.
 *
 *  @code
 *  #include <tmp006.h>
 *
 *  TMP006_Handle tmp006Handle;
 *  TMP006_Params tmp006Params;
 *
 *  tmp006Params.callback = gpioCallbackFxn;
 *  tmp006Handle = TMP006_open(TMP006_ROOMTEMP, i2cHandle, &tmp006Params);
 *  TMP006_enableDataReady(tmp006Handle);
 *  @endcode
 *  ============================================================================
 */

#ifndef TMP006_H_
#define TMP006_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Driver Header files */
#include <ti/drivers/I2C.h>
#include <ti/drivers/GPIO.h>

/* TMP006 Register Addresses */
#define TMP006_VOBJ          0x0000U  /*! Sensor voltage result register      */
#define TMP006_DIE_TEMP      0x0001U  /*! DIE Temp result register            */
#define TMP006_CONFIG        0x0002U  /*! Configuration register              */

/*!
 *  @brief    A handle that is returned from a TMP006_open() call.
 */
typedef struct TMP006_Config    *TMP006_Handle;

/*!
 *  @brief    TMP006 ADC conversion settings
 *
 *  This enumeration defines the number of ADC conversions performed before an
 *  average is computed and stored in the hardware registers. More ADC
 *  conversions results in a longer overall conversion time and more accurate
 *  object temperature reading.
 */
typedef enum TMP006_Conversions 
{
    TMP006_1CONV =    0x0000U,  /*!< 4   conversions/sec  */
    TMP006_2CONV =    0x0200U,  /*!< 2   conversions/sec  */
    TMP006_4CONV =    0x0400U,  /*!< 1   conversions/sec  */
    TMP006_8CONV =    0x0600U,  /*!< .5  conversions/sec  */
    TMP006_16CONV =   0x0800U,  /*!< .25 conversions/sec  */
} TMP006_Conversions;

/*!
 *  @brief    TMP006 I2C slave addresses.
 *
 *  The TMP006 Slave Address is determined by the input to the ADR0 and ADR1
 *  input pins of the TMP006 hardware. A '1' indicates a supply voltage of
 *  up to 7.5 V while '0' indicates ground. In some cases, the ADR0 pin may
 *  be coupled with the SDA or SCL bus to achieve a particular slave address.
 *  TMP006 sensors on the same I2C bus cannot share the same slave address.
 */
typedef enum TMP006_SlaveAddress {
    TMP006_SA1 = 0x40U,  /*!< ADR1 = 0, ADR0 = 0   */
    TMP006_SA2 = 0x41U,  /*!< ADR1 = 0, ADR0 = 1   */
    TMP006_SA3 = 0x42U,  /*!< ADR1 = 0, ADR0 = SDA */
    TMP006_SA4 = 0x43U,  /*!< ADR1 = 0, ADR0 = SCL */
    TMP006_SA5 = 0x44U,  /*!< ADR1 = 1, ADR0 = 0   */
    TMP006_SA6 = 0x45U,  /*!< ADR1 = 1, ADR0 = 1   */
    TMP006_SA7 = 0x46U,  /*!< ADR1 = 1, ADR0 = SDA */
    TMP006_SA8 = 0x47U   /*!< ADR1 = 1, ADR0 = SCL */
} TMP006_SlaveAddress;

/*!
 *  @brief    TMP006 temperature settings
 *
 *  This enumeration defines the scaling for reading and writing temperature
 *  values with a TMP006 sensor.
 */
typedef enum TMP006_TempScale {
    TMP006_CELSIUS = 0U,
    TMP006_KELVIN  = 1U,
    TMP006_FAHREN  = 2U
} TMP006_TempScale;

/*!
 *  @brief    TMP006 Data ready pin 
 *
 *  The DRDY enable bit enables/disables the data ready pin.  
 */
typedef enum TMP006_DataReadyPin {
    TMP006_DataReadyDisable =0x0000U,
    TMP006_DataReadyEnable  =0x0100U
} TMP006_DataReadyPin;

/*!
 *  @brief    TMP006 Conversion Status 
 *
 *  This enumeration tells whether there is conversion ongoing or data is
 *  ready
 */
typedef enum TMP006_ConversionStatus {
    TMP006_DataNotReady     =0U,
    TMP006_DataReady        =1U
} TMP006_ConversionStatus;

/*!
 *  @brief    TMP006 configuration
 *
 *  The TMP006_Config structure contains a set of pointers used to characterize
 *  the TMP006 driver implementation.
 *
 *  This structure needs to be defined and provided by the application.
 */
typedef struct TMP006_Config {
    /*! Pointer to a driver specific hardware attributes structure */
    void const    *hwAttrs;

    /*! Pointer to an uninitialized user defined TMP006_Object struct */
    void          *object;
} TMP006_Config;

/*!
 *  @brief    Hardware specific settings for a TMP006 sensor.
 *
 *  This structure should be defined and provided by the application. The
 *  gpioIndex should be defined in accordance of the GPIO driver. The pin
 *  must be configured as GPIO_CFG_INPUT and GPIO_CFG_IN_INT_FALLING.
 */
typedef struct TMP006_HWAttrs {
    TMP006_SlaveAddress    slaveAddress;    /*!< I2C slave address */
    unsigned int           gpioIndex;       /*!< GPIO configuration index */
} TMP006_HWAttrs;

/*!
 *  @brief    Members should not be accessed by the application.
 */
typedef struct TMP006_Object {
    I2C_Handle          i2cHandle;       /*!< i2c  handle used for interfacing*/
    GPIO_CallbackFxn    callback;        /*!< Callback function for data ready*/
    double              S0;              /*!< Calibration factor */
} TMP006_Object;

/*!
 *  @brief  TMP006 Parameters
 *
 *  TMP006 parameters are used with the TMP006_open() call. Default values for
 *  these parameters are set using TMP006_Params_init(). The GPIO_CallbackFxn
 *  should be defined by the application only if the data ready functionality is
 *  desired. A gpioIndex must be defined in the TMP006_HWAttrs for the
 *  corresponding tmp006Index. The GPIO_CallbackFxn is in the context of an
 *  interrupt handler.
 *
 *  @sa     TMP006_Params_init()
 */
typedef struct TMP006_Params {
    TMP006_Conversions   conversions;       /*!< Number of ADC Conversions*/
    TMP006_DataReadyPin  dataReady;         /*!< Data ready pin enable/disable*/
    GPIO_CallbackFxn     callback;          /*!< Pointer to GPIO callback function*/
    double               S0;                /*!< Calibration factor */
} TMP006_Params;

/*!
 *  @brief  Function to disable the data ready pin and GPIO interrupt.
 *
 *  This function will disable the data ready pin on the specified TMP006
 *  sensor.Interrupts on the TMP006 specific GPIO index will be disabled. Any
 *  temperature limits set are not affected. This function is not thread
 *  safe when used in conjunction with TMP006_enableDataReady() (ie. A thread
 *  is executing TMP006_disableDataReady() but preempted by a higher priority
 *  thread that calls TMP006_enableDataReady() on the same handle).
 *
 *  @sa     TMP006_enableDataReady()
 *
 *  @pre    TMP006_enableDataReady() had to be called first in order for this
 *          function to have an effect.
 *
 *  @param  handle    A TMP006_Handle
 *
 *  @return true on success or false upon failure.
 */
extern bool TMP006_disableDataReady(TMP006_Handle handle);

/*!
 *  @brief  Function to get the data ready status 
 *
 *  This function will get the status of conversion.
 *
 *  @param  handle    A TMP006_Handle
 *
 *  @param  conversionStatus a  pointer to a TMP006_ConversionStatus where
 *                           conversion status will be stored
 *
 *  @return true on success or false upon failure.
 */
extern bool TMP006_getDataReadyStatus(TMP006_Handle handle,
                                     TMP006_ConversionStatus *conversionStatus);

/*!
 *  @brief  Function to enable the data ready pin and GPIO interrupt.
 *
 *  This function will enable the data ready pin on the specified TMP006 sensor.
 *  Interrupts on the TMP006 specific GPIO index will be enabled. This
 *  function is not thread safe when used in conjunction with
 *  TMP006_disableDataReady() (ie. A thread is executing TMP006_enableDataReady()
 *  but preempted by a higher priority thread that calls TMP006_disableDataReady()
 *  on the same handle).
 *
 *  @sa     TMP006_disableDataReady()
 *
 *  @pre    TMP006_open() or  had to be called first.
 *
 *  @param  handle    A TMP006_Handle
 *
 *  @return true on success or false upon failure.
 */
extern bool TMP006_enableDataReady(TMP006_Handle handle);

/*!
 *  @brief  Function to close a TMP006 sensor specified by the TMP006 handle
 *
 *  The TMP006 hardware will be placed in a low power state in which ADC
 *  conversions are disabled. If there is GPIO pin configured to DRDY(data
 *  ready) then GPIO pin interrupts will be disabled. The I2C handle
 *  is not affected.
 *
 *  @pre    TMP006_open() had to be called first.
 *
 *  @param  handle    A TMP006_Handle returned from TMP006_open()
 *
 *  @return true on success or false upon failure.
 */
extern bool TMP006_close(TMP006_Handle handle);

/*!
 *  @brief  Function to put TMP006 sensor is low power state.
 *
 *  This function will place the specified TMP006 sensor in a low power state.
 *  All settings are retained. Temperature and voltage readings are invalid
 *  in this state. This function is not thread safe when used in conjunction
 *  with TMP006_enableConversions().
 *
 *  @sa     TMP006_enableConversions()
 *
 *  @pre    TMP006_open() had to be called first.
 *
 *  @param  handle    A TMP006_Handle
 *
 *  @return true on success or false upon failure.
 */
extern bool TMP006_disableConversions(TMP006_Handle handle);

/*!
 *  @brief  Function to restore TMP006 to normal power operation.
 *
 *  This function will restore ADC conversions on the specified TMP006 sensor.
 *  Temperature and voltage readings may be invalid up to 4 seconds following
 *  this function call. This function is not thread safe when used in
 *  conjunction with TMP006_disableConversions().
 *
 *  @sa     TMP006_disableConversions
 *
 *  @pre    TMP006_disableConversions() had to be called first.
 *
 *  @param  handle    A TMP006_Handle
 *
 *  @return true on success or false upon failure.
 */
extern bool TMP006_enableConversions(TMP006_Handle handle);

/*!
 *  @brief  Function to read die temperature
 *
 *  This function will return the temperature of the die. Full scale allows a
 *  result of up to (+/-)256 (C).
 *
 *  @param  handle    A TMP006_Handle
 *
 *  @param  scale     A TMP006_TempScale field specifying the temperature
 *                    format.
 *
 *  @param  temp      A pointer to a float in which temperature data will be
 *                    stored in.
 *
 *  @return true on success or false upon failure.
 */
extern bool TMP006_getDieTemp(TMP006_Handle handle,
        TMP006_TempScale scale, float *temp);

/*!
 *  @brief  Function to read object temperature
 *
 *  This function will return the temperature of the object in the field
 *  of view of the specified TMP006 sensor. Full scale allows a result
 *  of up to (+/-)256 (C).
 *
 *  @param  handle    A TMP006_Handle
 *
 *  @param  scale     A TMP006_TempScale field specifying the temperature
 *                    format.
 *
 *  @param  temp      A pointer to a float in which temperature data will be
 *                    stored in.
 *
 *  @return true on success or false upon failure.
 */
extern bool TMP006_getObjTemp(TMP006_Handle handle,
        TMP006_TempScale scale, float *temp);

/*!
 *  @brief  Function to initialize TMP006 driver.
 *
 *  This function will initialize the TMP006 driver.
 */
extern void TMP006_init();

/*!
 *  @brief  Function to open a given TMP006 sensor
 *
 *  Function to initialize a given TMP006 sensor specified by the particular
 *  index value. This function must be called from a thread context.
 *  If one intends to use the data ready pin, a callBack function must be 
 *  specified in the TMP006_Params structure. Additionally, a gpioIndex must be
 *  setup and specified in the TMP006_HWAttrs structure.
 *
 *  The I2C controller must be operating in BLOCKING mode. Failure to ensure
 *  the I2C_Handle is in BLOCKING mode will result in undefined behavior.
 *
 *  The user should ensure that each sensor has its own slaveAddress,
 *  gpioIndex (if alarming) and tmp006Index.
 *
 *  @pre    TMP006_init() has to be called first
 *
 *  @param  tmp006Index    Logical sensor number for the TMP006 indexed into
 *                         the TMP006_config table
 *
 *  @param  i2cHandle      An I2C_Handle opened in BLOCKING mode
 *
 *  @param  *params        A pointer to TMP006_Params structure. If NULL, it
 *                         will use default values.
 *
 *  @return  A TMP006_Handle on success, or a NULL on failure.
 *
 *  @sa      TMP006_init()
 *  @sa      TMP006_Params_init()
 *  @sa      TMP006_close()
 */
extern TMP006_Handle TMP006_open(unsigned int tmp006Index,
        I2C_Handle i2cHandle, TMP006_Params *params);

/*!
 *  @brief  Function to initialize a TMP006_Params struct to its defaults
 *
 *  @param  params      A pointer to TMP006_Params structure for
 *                      initialization.
 *
 *  Default values are:
 *        conversions       = TMP006_4CONV
 *        intComp           = TMP006_IntMode
 *        transientCorrect  = TMP006_TCEnable
 *        callback          = NULL
 */
extern void TMP006_Params_init(TMP006_Params *params);

/*!
 *  @brief  Read function for advanced users.
 *
 *  @param  handle             A TMP006_Handle
 *
 *  @param  registerAddress    Register address
 *
 *  @param  data               A pointer to a data register in which received
 *                             data will be written to. Must be 16 bits.
 *
 *  @return true on success or false upon failure.
 */
extern bool TMP006_readRegister(TMP006_Handle handle,
        uint16_t *data, uint8_t registerAddress);
/*!
 *  @brief  Function to read a TMP006 sensor's voltage register.
 *
 *  This function will read the sensor voltage register. This value is the
 *  digitized infrared sensor voltage output. The recieved data has a
 *  resolution of 156.25 nV/LSB and should be scaled appropriately.
 *
 *  @param  handle    A TMP006_Handle
 *
 *  @param  data      A uint16_t pointer to a storage address. This
 *                    value may range from (+/-) 5.21 mV.
 *
 *  @return true on success or false upon failure.
 */
static inline bool TMP006_readVoltage(TMP006_Handle handle,
                int16_t *data)
{
    return (TMP006_readRegister(handle, (uint16_t *)data, TMP006_VOBJ));
}

/*!
 *  @brief  Write function for advanced users. This is not thread safe.
 *
 *  When writing to a handle, it is possible to overwrite data written
 *  by another thread.
 *
 *  For example: Thread A and B are writing to the configuration register.
 *  Thread A has a higher priority than THread B. Thread B is running and
 *  reads the configuration register. Thread A then preempts Thread B and reads
 *  the configuration register, performs a logical OR and writes to the
 *  configuration register. Thread B then resumes execution and performs its
 *  logical OR and writes to the configuration register--overwriting the data
 *  written by Thread A.
 *
 *  Such instances can be prevented through the use of Semaphores. Below is an
 *  example which utilizes an initialized Semaphore_handle, tmp006Lock.
 *
 *  @code
 *  if (0 == sem_wait(&tmp006Lock)) 
 *  {  
 *      //Perform read/write operations
 *      sem_post(tmp006Lock);
 *  }
 *  else
 *  {
 *      //handle error scenario
 *  }
 *  @endcode
 *
 *  @param  handle             A TMP006_Handle
 *
 *  @param  data               A uint16_t containing the 2 bytes to be written
 *                             to the TMP006 sensor.
 *
 *  @param  registerAddress    Register address.

 *  @return true on success or false upon failure.
 */
extern bool TMP006_writeRegister(TMP006_Handle handle, uint16_t data,
        uint8_t registerAddress);

#ifdef __cplusplus
}
#endif

#endif /* TMP006_H_ */
