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
 *  @file       pga460.h
 *
 *  @brief      PGA460-Q1 Automotive Ultrasonic Signal Processor and Transducer 
 *              Driver
 *
 *  The PGA460-Q1 is Automotive Ultrasonic Signal Processor and Transducer 
 *  Driver.It is interfaced to the MCU using 
 *  UART/OWU (ONE WIRE UART)/TCI (TIME COMMAND INTERFACE).The current 
 *  implementation supports only UART mode.
 *
 *
 *  The PGA460 header file should be included in an application as
 *  follows:
 *  @code
 *  #include <ti/sail/pga460/pga460.h>
 *  @endcode
 *
 *  # Operation
 *  The PGA460 driver simplifies using a PGA460 Automotive Ultrasonic Signal
 *  Processor and Transducer Driver. A PGA460_config array should be defined by 
 *  the application. The PGA460_config array should contain pointers to a
 *  defined PGA460_HwAttrs and allocated array for the PGA460_Object structures.
 *  PGA460_init() must be called prior to using PGA460_open().
 *
 *  The APIs in this driver serve as an interface to a DPL(Driver Porting Layer)
 *  The specific implementations are responsible for creating all the RTOS
 *  specific primitives to allow for thread-safe operation.
 *
 *  This driver has no dynamic memory allocation.
 *
 *  ## Defining PGA460_Config, PGA460_Object and PGA460_HwAttrs #
 *  Each structure must be defined by the application. The following
 *  example is for a MSP432 in which an PGA460 sensor is setup.
 *  The following declarations are placed in "MSP_EXP432P401R.h"
 *  and "MSP_EXP432P401R.c" respectively.
 *
 *  "MSP_EXP432P401R.h"
 *  @code
 *  typedef enum MSP_EXP432P401R_PGA460Name {
 *      MSP_EXP432P401R_PGA460_0 = 0, // Sensor for measuring distance from obstuction.
 *      MSP_EXP432P401R_PGA460COUNT
 *  } MSP_EXP432P401R_PGA460Name;
 *
 *  @endcode
 *
 *  "MSP_EXP432P401R.c"
 *  @code
 *  #include <ti/sail/pga460/pga460.h>
 *
 *  PGA460_Object PGA460_object[MSP_EXP432P401R_PGA460COUNT];
 *
 *  const PGA460_HWAttrs PGA460_hwAttrs[MSP_EXP432P401R_PGA460COUNT] = {
 *      {
 *          .uartAddress = 0 // 0 is the address of interfaced PGA460 device
 *      }
 *  };
 *
 *  const PGA460_Config PGA460_config[] = {
 *      {
 *          .hwAttrs = &PGA460_hwAttrs[0],
 *          .objects = &PGA460_object[0],
 *      },
 *      {NULL, NULL},
 *  };
 *
 *  const uint_least8_t PGA460_count = MSP_EXP432P401R_PGA460COUNT;
 *  @endcode
 *
 *  ## Opening a UART Handle #
 *  The UART controller must be in blocking mode. This will seamlessly allow
 *  for multiple UART endpoints without conflicting callback functions. The
 *  UART_open() call requires a configured index. This index must be defined
 *  by the application in accordance to the UART driver. The default transfer
 *  mode for an UART_Params structure is UART_MODE_BLOCKING. In the example
 *  below, the transfer mode is set explicitly for clarity.The data mode 
 *  setting is binary. Additionally, the PGA460 hardware is capable of 
 *  communicating in range 2400 bps to 115200 bps
 *
 *  @code
 *  #include <ti/drivers/UART.h>
 *
 *  UART_Handle uartHandle;
 *  UART_Params uartParams;
 *
 *  UART_Params_init(&uartParams);
 *  
 *  uartParams.baudRate  = 115200;
 *  uartParams.writeMode = UART_MODE_BLOCKING;
 *  uartParams.readDataMode = UART_DATA_BINARY;
 *  uartParams.writeDataMode = UART_DATA_BINARY;
 *  uartParams.readEcho = UART_ECHO_OFF;
 *
 *  uartHandle = UART_open(Board_PGA460_UART, &uartParams);
 *  @endcode
 *
 *  ## Opening a PGA460 with default parameters #
 *  The PGA460_open() call must be made in a task context.
 *
 *  @code
 *  #include <ti/sail/pga460/pga460.h>
 *
 *  PGA460_Handle handle;
 *
 *  PGA460_init();
 *  handle = PGA460_open(Board_PGA460_PARK, uartHandle, NULL);
 *  @endcode
 *  ============================================================================
 */

#ifndef PGA460_H_
#define PGA460_H_

/* Standard includes*/
#include <stdbool.h>

/* Driver Header files */
#include <ti/drivers/UART.h>

#ifdef __cplusplus
extern "C" {
#endif

 /*********************************************************************
  * Defines
  */
 /** \defgroup grp_reg_map register map of the PGA460 device with address offset
  */
 /**@{*/
#define PGA460_USER_DATA1     0x0U
#define PGA460_USER_DATA2     0x1U
#define PGA460_USER_DATA3     0x2U
#define PGA460_USER_DATA4     0x3U
#define PGA460_USER_DATA5     0x4U
#define PGA460_USER_DATA6     0x5U
#define PGA460_USER_DATA7     0x6U
#define PGA460_USER_DATA8     0x7U
#define PGA460_USER_DATA9     0x8U
#define PGA460_USER_DATA10    0x9U
#define PGA460_USER_DATA11    0xAU
#define PGA460_USER_DATA12    0xBU
#define PGA460_USER_DATA13    0xCU
#define PGA460_USER_DATA14    0xDU
#define PGA460_USER_DATA15    0xEU
#define PGA460_USER_DATA16    0xFU
#define PGA460_USER_DATA17    0x10U
#define PGA460_USER_DATA18    0x11U
#define PGA460_USER_DATA19    0x12U
#define PGA460_USER_DATA20    0x13U
#define PGA460_TVGAIN0        0x14U
#define PGA460_TVGAIN1        0x15U
#define PGA460_TVGAIN2        0x16U
#define PGA460_TVGAIN3        0x17U
#define PGA460_TVGAIN4        0x18U
#define PGA460_TVGAIN5        0x19U
#define PGA460_TVGAIN6        0x1AU
#define PGA460_INIT_GAIN      0x1BU
#define PGA460_FREQUENCY      0x1CU
#define PGA460_DEADTIME       0x1DU
#define PGA460_PULSE_P1       0x1EU
#define PGA460_PULSE_P2       0x1FU
#define PGA460_CURR_LIM_P1    0x20U
#define PGA460_CURR_LIM_P2    0x21U
#define PGA460_REC_LENGTH     0x22U
#define PGA460_FREQ_DIAG      0x23U
#define PGA460_SAT_FDIAG_TH   0x24U
#define PGA460_FVOLT_DEC      0x25U
#define PGA460_DECPL_TEMP     0x26U
#define PGA460_DSP_SCALE      0x27U
#define PGA460_TEMP_TRIM      0x28U
#define PGA460_P1_GAIN_CTRL   0x29U
#define PGA460_P2_GAIN_CTRL   0x2AU
#define PGA460_EE_CRC         0x2BU
#define PGA460_EE_CNTRL       0x40U
#define PGA460_BPF_A2_MSB     0x41U
#define PGA460_BPF_A2_LSB     0x42U
#define PGA460_BPF_A3_MSB     0x43U
#define PGA460_BPF_A3_LSB     0x44U
#define PGA460_BPF_B1_MSB     0x45U
#define PGA460_BPF_B1_LSB     0x46U
#define PGA460_LPF_A2_MSB     0x47U
#define PGA460_LPF_A2_LSB     0x48U
#define PGA460_LPF_B1_MSB     0x49U
#define PGA460_LPF_B1_LSB     0x4AU
#define PGA460_TEST_MUX       0x4BU
#define PGA460_DEV_STAT0      0x4CU
#define PGA460_DEV_STAT1      0x4DU
#define PGA460_P1_THR_0       0x5FU
#define PGA460_P1_THR_1       0x60U
#define PGA460_P1_THR_2       0x61U
#define PGA460_P1_THR_3       0x62U
#define PGA460_P1_THR_4       0x63U
#define PGA460_P1_THR_5       0x64U
#define PGA460_P1_THR_6       0x65U
#define PGA460_P1_THR_7       0x66U
#define PGA460_P1_THR_8       0x67U
#define PGA460_P1_THR_9       0x68U
#define PGA460_P1_THR_10      0x69U
#define PGA460_P1_THR_11      0x6AU
#define PGA460_P1_THR_12      0x6BU
#define PGA460_P1_THR_13      0x6CU
#define PGA460_P1_THR_14      0x6DU
#define PGA460_P1_THR_15      0x6EU
#define PGA460_P2_THR_0       0x6FU
#define PGA460_P2_THR_1       0x70U
#define PGA460_P2_THR_2       0x71U
#define PGA460_P2_THR_3       0x72U
#define PGA460_P2_THR_4       0x73U
#define PGA460_P2_THR_5       0x74U
#define PGA460_P2_THR_6       0x75U
#define PGA460_P2_THR_7       0x76U
#define PGA460_P2_THR_8       0x77U
#define PGA460_P2_THR_9       0x78U
#define PGA460_P2_THR_10      0x79U
#define PGA460_P2_THR_11      0x7AU
#define PGA460_P2_THR_12      0x7BU
#define PGA460_P2_THR_13      0x7CU
#define PGA460_P2_THR_14      0x7DU
#define PGA460_P2_THR_15      0x7EU
#define PGA460_THR_CRC        0x7FU
 /**@}*/

 /**
  *  @defgroup error_group Error flags returned on register read or response
  *  command operation
  */

 /** @defgroup grp_uarterror_flags  UART register read, response command error
  *   flags.
  *
  *  @ingroup config_group
  *
  *  The slave (PGA460) for response and register read operations returns the
  *  diag data field (for the previous UART transaction). This field,
  *  based on the flag UART_DIAG in the PULSE_P1 register, indicates either the
  *  UART communication error status or system diagnostic error status.
  */
 /**@{*/
#define PGA460_UARTERROR_DEVICEBUSY            0x41U   /*!< Device Busy*/
 /*!
 * sync field bit rate too high or too low
 */
#define PGA460_UARTERROR_BITRATE               0x42U
 /*!
 * consecutive sync field bit widths do not match
 */
#define PGA460_UARTERROR_SYNCFIELDWIDTH        0x44U
 /*!
 * The checksum received from master for the previous UART transaction was
 * incorrect.
 */
#define PGA460_UARTERROR_INVALIDCHECKSUM       0x48U
 /*!
 * The previous command sent by master was invalid.
 */
#define PGA460_UARTERROR_INVALIDCOMMAND        0x50U
 /*!General communication error:
 ***SYNC filed stop bit too short
 ***Command filed incorrect stop bit (dominant
 when should be recessive)
 ***Command filed stop bit too short
 ***Data field incorrect stop bit (dominant when
 should be recessive)
 **Data field stop bit too short
 **Data field PGA460-Q1 transmit value
 overdriven to dominant value during stop bit
 transmission
 **Data contention
 */
#define PGA460_UARTERROR_GENCOMMUNICATION      0x60U
 /**@}*/

 /** @defgroup grp_sdiagerror_flags   Defines system diagnostic error
  * status(received in diag data field)
  *
  *  @ingroup config_group
  *
  *  The PGA460 device, for response and register read operations returns the
  *  diag data field. This field, based on the flag UART_DIAG in the PULSE_P1
  *  register, indicates either the UART communication error status or system
  *  diagnostic error status.
  */
 /**@{*/
#define PGA460_SYSDIAGERROR_DEVICEBUSY            0x41U   /*!< Device Busy*/
 /*!
  * Threshold settings CRC error
  */
#define PGA460_SYSDIAGERROR_THRSETTINGSCRC        0x42U
 /*!
 * Frequency Diagnostic Error
 */
#define PGA460_SYSDIAGERROR_FREQDIAG              0x44U
 /*!
 * Voltage Diagnostic Error
 */
#define PGA460_SYSDIAGERROR_VOLTAGEDIAT           0x48U
 /*!
 * EEPROM CRC error or TRIM CRC error
 */
#define PGA460_SYSDIAGERROR_EEPROMTRIMCRC         0x60U
 /**@}*/

 /** @defgroup grp_othererror_flags   Defines other error flags which tell about
  * checksum error and whether error type is uart error or system diagnostic
  * error.
  *
  *  @ingroup config_group
  */
 /**@{*/
 /*!
 * The error status returned on the register read or response command will be
 * a uint16. The 8th bit tells whether there was a checksum error. if the bit is
 * set, then there was a checksum error.
 */
#define PGA460_CHECKSUMERRORFLAG                  0x0100U

#define PGA460_ERROR_OK                           0x40U   /*!< No Error */
 /**@}*/

 /*********************************************************************
  * typedefs
  */

 /* !
  * @brief Error status Flags (Diagnostic data field of PGA460 response)
  *
  * The flags will be either UART diagnostic error or the system diagnostic
  * error status flags.
  */
 typedef uint16_t PGA460_respErrorStatusFlags;

 /*!
  *  @brief    A handle that is returned from a PGA460_open() call.
  */
 typedef struct PGA460_Config    *PGA460_Handle;

/*!
 *  @brief    Communication Interface used with PGA460-Q1
 *
 *  The enumeration describes about the communication interface used with
 *  the PGA460-Q1
 *
 *  The PGA460-Q1 has 3 possible communication interfaces, UART,
 *  OWU(one wire uart) and TCI(time command interface). Currently only UART
 *  is supported.
 */
typedef enum PGA460_communicationInterface
{
    PGA460_communicationInterface_UART = 0x00U,  /*!< UART interface */
    PGA460_communicationInterface_OWU  = 0x01U,  /*!< One Wire UART interface */
    PGA460_communicationInterface_TCI  = 0x02U   /*!< Time Command Interface  */
} PGA460_communicationInterface;

/*!
 *  @brief    Defines for ultrasonic sensors
 *
 *  The Enumeration is used for identifying ultrasonic sensors.
 *
 * @sa PGA460_configure
 */
typedef enum PGA460_ultrasonicSensorType
{
    PGA460_ultrasonicSensorType_MA58MF147N = 0x00U,   /*!< murata MA58MF147N */
    PGA460_ultrasonicSensorType_MA40H1SR   = 0x01U,   /*!< murata MA40H1SR */
    /*!
    * User specified settings will be used.
    */
    PGA460_ultrasonicSensorType_CUSTOM     = 0x02U
} PGA460_ultrasonicSensorType;

/*!
 *  @brief    Defines for PGA460 presets.
 *
 *  The enumeration is used for identifying PGA460 presets.
 *
 *  @sa PGA460_setThreshholds
 */
typedef enum PGA460_preset
{
    PGA460_presets_PRESET1 = 0x00,   /*!< PRESET1 of PGA460. */
    PGA460_presets_PRESET2 = 0x01,   /*!< PRESET2 of PGA460. */
} PGA460_preset;


/*!
 *  @brief    Defines for PGA460 threshold values.
 *
 *  The enumeration is used for specifying threshold values for presets.
 *
 *  @sa PGA460_setThreshholds
 */
typedef enum PGA460_threshold
{
    PGA460_threshold_25     = 0x00,   /*!< 25% threshold values. */
    PGA460_threshold_50     = 0x01,   /*!< 50% threshold values. */
    PGA460_threshold_75     = 0x02,   /*!< 75% threshold values. */
    PGA460_threshold_custom = 0x03U,   /*!< User specified threshold values. */
} PGA460_threshold;

/*!
 *  @brief    Defines for PGA460 time varying gain values.
 *
 *  The wnumeration is used for specifying time varying gain values.
 *
 *  @sa PGA460_setTimeVaryingGain
 */
typedef enum PGA460_tvggain
{
    PGA460_tvgain_25     = 0x00U,   /*!< 25% tvggain values. */
    PGA460_tvgain_50     = 0x01U,   /*!< 50% tvggain values. */
    PGA460_tvgain_75     = 0x02U,   /*!< 75% tvggain values. */
    PGA460_tvgain_custom = 0x03U,   /*!< User specified tvggain values. */
} PGA460_tvgain;

/*!
 *  @brief    Defines for PGA460 low power mode time
 *
 *  The enumeration is used for specifying low power mode enter time
 *
 *  @sa PGA460_setLowPowerMode
 */
typedef enum PGA460_lpmTime
{
   /*!
    * 250 ms low power mode enter timer.
    */
    PGA460_lpmTime_250ms    = 0x00U,
    PGA460_lpmTime_500ms    = 0x01U,   /*!< 500 ms low power mode enter timer. */
    PGA460_lpmTime_1s       = 0x02U,   /*!< 1 s low power mode enter timer. */
    PGA460_lpmTime_4s       = 0x03U,   /*!< 4 s low power mode enter timer. */
} PGA460_lpmTime;

/*!
 *  @brief    Defines for PGA460 AFE Gain.
 *
 *  The enumeration is used for specifying analog front end gain.
 *
 *  @sa PGA460_setAFEGain
 */
typedef enum PGA460_afegain
{
    PGA460_afegain_58_90     = 0x00U,   /*!< 58 to 90db AFE gain. */
    PGA460_afegain_52_84     = 0x01U,   /*!< 52 to 84db AFE gain. */
    PGA460_afegain_46_78     = 0x02U,   /*!< 52 to 84db AFE gain. */
    PGA460_afegain_32_64     = 0x03U,   /*!< 52 to 84db AFE gain. */
} PGA460_afegain;

/*!
 *  @brief    Defines for diagnostic commands.
 *
 *  The enumeration is used for specifying diagnostic command.
 *
 *  @sa PGA460_runDiagnostic
 */
typedef enum PGA460_diagnostic
{
    PGA460_diagnostic_TF  = 0x00U,   /*!< Transducer frequency. */
    PGA460_diagnostic_DPT = 0x01U,   /*!< Decay Period time. */
    PGA460_diagnostic_TLM = 0x02U,   /*!< Temperature Level measurement. */
    PGA460_diagnostic_NLM = 0x03U    /*!< Noise Level Measurement. */
} PGA460_diagnostic;

/*!
 *  @brief    Defines for broadcast enable disable
 *
 *  The enumeration is used for specifying broadcast enable/disable
 *
 */
typedef enum PGA460_broadcast
{
    PGA460_broadcast_Disable  = 0x00U,   /*!< Broadcast disable enum */
    PGA460_broadcast_Enable   = 0x01U    /*!< Broadcast enable enum */
} PGA460_broadcast;

/*!
 * @brief    [4:0] of UART command field.
 *
 * The enumeration describes field [4:0} of UART command field.
 *
 * The UART command field consists of 8 bits. The [4:0] field forms the
 * UART command field and [7:5] forms the UART address field. The
 * address field has to be appended to this field so as to create the
 * UART Command.
 */
typedef enum PGA460_commandCode
{
    PGA460_commandCode_P1BL     = 0x00U,
    PGA460_commandCode_P2BL     = 0x01U,
    PGA460_commandCode_P1LO     = 0x02U,
    PGA460_commandCode_P2LO     = 0x03U,
    PGA460_commandCode_TNLM     = 0x04U,
    PGA460_commandCode_UMR      = 0x05U,
    PGA460_commandCode_TNLR     = 0x06U,
    PGA460_commandCode_TEDD     = 0x07U,
    PGA460_commandCode_SD       = 0x08U,
    PGA460_commandCode_SRR      = 0x09U,
    PGA460_commandCode_SRW      = 0x0AU,
    PGA460_commandCode_EEBR     = 0x0BU,
    PGA460_commandCode_EEBW     = 0x0CU,
    PGA460_commandCode_TVGBR    = 0x0DU,
    PGA460_commandCode_TVGBW    = 0x0EU,
    PGA460_commandCode_THRBR    = 0x0FU,
    PGA460_commandCode_THRBW    = 0x10U,
    /* Broadcast */
    PGA460_commandCode_BC_P1BL  = 0x11U,
    PGA460_commandCode_BC_P2BL  = 0x12U,
    PGA460_commandCode_BC_P1LO  = 0x13U,
    PGA460_commandCode_BC_P2LO  = 0x14U,
    PGA460_commandCode_BC_TNLM  = 0x15U,
    PGA460_commandCode_BC_SRW   = 0x16U,
    PGA460_commandCode_BC_EEBW  = 0x17U,
    PGA460_commandCode_BC_TVGBW = 0x18U,
    PGA460_commandCode_BC_THRBW = 0x19U,
    PGA460_commandCode_none     = 0x1AU
} PGA460_commandCode;

/*!
 *  @brief    PGA460 configuration.
 *
 *  The PGA460_Config structure contains a set of pointers used to characterize
 *  the PGA460 driver implementation.
 *
 *  This structure needs to be defined and provided by the application.
 */
typedef struct PGA460_Config
{
    /*! Pointer to a driver specific hardware attributes structure. */
    void const    *hwAttrs;

    /*! Pointer to an uninitialized user defined PGA460_Object struct. */
    void          *object;
} PGA460_Config;

/*!
 *  @brief    Hardware specific settings for a PGA460(UART).
 *
 *  This structure should be defined and provided by the application.
 */
typedef struct PGA460_UARTHWAttrs
{
    /*!
    * Last 3 bits of UART command  field is UART address information.
    * The address information in command field is compared to UART_ADDR
    * parameter in EEPROM memory.
    */
     uint8_t uartAddress;
} PGA460_UARTHWAttrs;

/*!
 *  @brief    Members should not be accessed by the application.
 */
typedef struct PGA460_Object
{
    /*!
    * Handle used to communicate with the PGA460, Based on the value of param
    * comInterface this will be type casted to either SPI or UART handle.
    */
    void* comInterfaceHandle;
    /*!
    * Variable which defines what type of communication is used with PGA460.
    */
    PGA460_communicationInterface comInterface;
} PGA460_Object;

/*!
 *  @brief  PGA460 Parameters.
 *
 *  PGA460 parameters are used with the PGA460_open() call. Default values for
 *  these parameters are set using PGA460_Params_init().
 *
 *  @sa     PGA460_Params_init()
 */
typedef struct PGA460_Params
{
    /*!
    * This variable defines the communication interface used with the PGA460.
    */
    PGA460_communicationInterface comInterface;
    bool enableLPMAtStartup; //!< Enable LowPower mode at startup.
    PGA460_lpmTime lpmTime;  //!< Low power mode enter time.
} PGA460_Params;


/**
 * EEPROM bulk write command format.
 */
typedef struct PGA460_EEBW
{
    uint8_t user_data[20U];  //!< User data 20 bytes.
    uint8_t tvgain[7U];      //!< Time-varying gain map segment configuration.
    uint8_t init_gain;       //!< AFE initial gain configuration register.
    uint8_t frequency;       //!< Burst frequency configuration register.
    /*!
    * Pulse dead time and threshold deglich configuration register.
    */
    uint8_t deadtime;
    /*!
    * Preset1 pulse burst number, IO pin control,
    *  UART diagnostic configuration register.
    */
    uint8_t pulse_p1;
    /*!
    * preset2 pulse burst number and UART address configuration register.
    */
    uint8_t pulse_p2;
    /*!
    * Preset1 driver current limit configuration register.
    */
    uint8_t curr_lim_p1;
    /*!
    * Preset2 current limit and low pass filter configuration register.
    */
    uint8_t curr_lim_p2;
    uint8_t rec_length;     //!< Echo data record period configuration register.
    uint8_t freq_diag;      //!< Frequency diagnostic configuration register.
    /*!
    * Decay saturation threshold, frequency diagnostic error threshold,
    * and Preset1 non-linear enable control configuration register.
    */
    uint8_t sat_fdiag_th;
    /*!
    * Voltage thresholds and Preset2 non-linear scaling enable.
    * configuration register
    */
    uint8_t fvold_dec;
    /*!
    * De-couple temperature and AFE gain range configuration register.
    */
    uint8_t decpl_temp;
    /*!
    * DSP non-linear scaling and noise level configuration register.
    */
    uint8_t dsp_scale;
    /*!
    * Temperature sensor compensation values register.
    */
    uint8_t temp_trim;
    uint8_t p1_gain_ctrl;   //!< Preset1 digital gain configuration register.
    uint8_t p2_gain_ctrl;   //!< Preset2 digital gain configuration register.
} PGA460_EEBW;

/**
 * Threshold bulk write command format.
 */
typedef struct PGA460_THRBW
{
    /*!
    * Preset1 threshold map segment configuration register.
    */
    uint8_t p1_thr[16U];
    /*!
    * Preset2 threshold map segment configuration register.
    */
    uint8_t p2_thr[16U];
} PGA460_THRBW;

/**
 * Register write command format.
 */
typedef struct PGA460_RW
{
    uint8_t regAddr;       //!< Register address
    uint8_t regData;       //!< Register data
} PGA460_RW;

/**
 * Register read command format.
 */
typedef struct PGA460_RR
{
    uint8_t regAddr;       //!< Register address
} PGA460_RR;

/**
 * Time varying gain bulk write command format.
 */
typedef struct PGA460_TVGBW
{
    uint8_t tvgain[7U];     //!< time varying gain
} PGA460_TVGBW;

/**
 * Burst Listen Preset 1
 */
typedef struct PGA460_P1BL
{
    uint8_t numOfObjects;  //!< Number of objects to be detected.
} PGA460_P1BL;

/**
 * Burst Listen Preset 2
 */
typedef struct PGA460_P2BL
{
    uint8_t numOfObjects;  //!< Number of objects to be detected.
} PGA460_P2BL;

/**
 * Ultrasonic Measurement Results command format.
 */
typedef struct PGA460_UMR
{
    uint8_t cmd;           //!< Command byte
    uint8_t checksum;      //!< Checksum of the command
} PGA460_UMR;

/**
 * System Diagnostics command format.
 */
typedef struct PGA460_SD
{
    uint8_t cmd;           //!< Command byte
    uint8_t checksum;      //!< Checksum of the command
} PGA460_SD;

/**
 * Temperature and Noise level measurement command format
 */
typedef struct PGA460_TNLM
{
    uint8_t tempOrNoise;   //!< Checksum of the command
} PGA460_TNLM;

/**
 * Temperature and Noise Level Result command format.
 */
typedef struct PGA460_TNLR
{
    uint8_t cmd;           //!< command byte
} PGA460_TNLR;

/**
 * Ultrasonic measurement results for the each object detected
 */
typedef struct PGA460_ULMResults
{
    uint16_t distance;         //!< object distance in meter
    uint8_t  width;            //!< object width in us
    uint8_t  amplitude;        //!< amplitude
} PGA460_ULMResults;

/*!
 *  @brief  Function to close a PGA460 sensor specified by the PGA460 handle
 *
 *  The instance of the PGA460 will be closed. All the relevant data structures
 *  will be invalid after this call.
 *
 *  @pre    PGA460_open() had to be called first.
 *
 *  @param  handle    A PGA460_Handle returned from PGA460_open()
 *
 *  @return true on success or false upon failure.
 */
extern bool PGA460_close(PGA460_Handle handle);

/*!
 *  @brief  Function to initialize PGA460 driver.
 *
 *  This function will initialize the PGA460 driver. This function must also be
 *  called before any other PGA460 module APIs.
 */
extern void PGA460_init();

/*!
 *  @brief  Function to open a given PGA460 sensor
 *
 *  Function to initialize a given PGA460 sensor specified by the particular
 *  index value. This function must be called from a task context.
 *  If one intends to use the IO pin for TCI, OWU communication,
 *  then that has to be specified in the PGA460_Params structure.
 *  Additionally, a gpioIndex must be setup and specified in the
 *  PGA460_HWAttrs structure.
 *
 *  The user should ensure that each sensor has its own UART address.
 *  when UART/USART/OWU communication interface is used
 *  and PGA460Index.
 *
 *  @pre    PGA460_init() has to be called first
 *
 *  @param  PGA460Index         Logical sensor number for the PGA460 indexed
 *                              into the PGA460_config table
 *
 *  @param  comInterfaceHandle  An communication interface handler(SPI/UART)
 *
 *  @param  params              A pointer to PGA460_Params structure. If NULL,
 *                              it will use default values.
 *
 *  @return  A PGA460_Handle on success, or a NULL on failure.
 *
 *  @sa      PGA460_init()
 *  @sa      PGA460_Params_init()
 *  @sa      PGA460_close()
 */
extern PGA460_Handle PGA460_open(unsigned int index,
        void *comInterfaceHandle, PGA460_Params *params);

/*!
 *  @brief  Function to configure initial settings of the PGA460
 *
 *  Function to configure PGA460 to recommended settings for the specified
 *  ultrasonic sensor type.
 *
 *
 *  @pre    PGA460_open() has to be called first
 *
 *  @param  handle  A PGA460_Handle
 *
 *  @param  sensorType    User has to specify the sensor type. Currently three
 *                        settings are supported. If the setting is CUSTOM then
 *                        the onus is on user to provide initialized eebw
 *                        structure.
 *
 *  @param  eebw          A pointer to PGA460_EEBW structure.
 *
 *  @param broadcastEnable This can take value of PGA460_broadcast_Enable or
 *                         PGA460_broadcast_disable. If broadcast is enabled
 *                         then the broadcast command code is used.
 *
 *  @return false if execution of the API is not successful.
 *
 *  @sa      PGA460_ultrasonicSensorType
 */
bool PGA460_configure(PGA460_Handle handle,
        PGA460_ultrasonicSensorType sensorType, const PGA460_EEBW *eebw,
        PGA460_broadcast broadcastEnable);

/*!
 *  @brief  Function to set threshold levels for PGA460.
 *
 *  Function to set thresholds for PGA460. User can specify any preset
 *  value 25%, 50%, or 75% or can specify user specific value.
 *
 *  @pre    PGA460_init() has to be called first
 *
 *  @param  handle   A PGA460_Handle
 *
 *  @param  PGA460_preset  User has to specify the PGA460 preset for which
 *                         the threshold has to be set.
 *
 *  @param  threshold      User has to specify the threshold type. Currently
 *                         three types are supported. If the setting is CUSTOM
 *                         then the onus is on user to provide initialized thrbw
 *                         structure(only threshold levels).
 *
 *
 *  @param  thrbw         A pointer to PGA460_THRBW structure. This is a
 *                        optional parameter needed when the tvgain is
 *                        PGA460_tvgain_custom. User can pass NULL in other
 *                        cases.
 *
 *  @param broadcastEnable This can take value of PGA460_broadcast_Enable or
 *                         PGA460_threshold_custom. If broadcast is enabled then
 *                         the broadcast command code is used.
 *
 *  @return false if eebw structure is not null else return true.
 *
 *  @sa      PGA460_ultrasonicSensorType
 */
bool PGA460_setThreshholds(PGA460_Handle handle,
                           PGA460_threshold threshold,const PGA460_THRBW *thrbw,
                           PGA460_broadcast broadcastEnable);

/*!
 *  @brief  Function to set time varying gain values for PGA460.
 *
 *  Function to set time varying gain for PGA460. User can specify any preset
 *  value 25%, 50%, or 75% or can specify user specific value.
 *
 *  @pre    PGA460_open() has to be called first
 *
 *  @param  handle  A PGA460_Handle
 *
 *  @param  tvgain        User has to specify the time varying gain percentage
 *                        If the setting is CUSTOM then the onus is on user to
 *                        provide initialized tvgbw(only gain values)structure.
 *
 *  @param  eebw          A pointer to PGA460_TVGBW structure. This is a
 *                        optional parameter needed when the tvgain is
 *                        PGA460_tvgain_custom. User can pass NULL in other
 *                        cases.
 *
 *  @param broadcastEnable This can take value of PGA460_broadcast_Enable or
 *                         PGA460_broadcast_disable. If broadcast is enabled
 *                         then the broadcast command code is used.
 *
 *  @return false if eebw structure is null else return true.
 *
 *  @sa      PGA460_tvgain
 */
extern bool PGA460_setTimeVaryingGain(PGA460_Handle handle,
        PGA460_tvgain tvgain, const PGA460_TVGBW *tvgbw,
        PGA460_broadcast broadcastEnable);

/*!
 *  @brief  Function to set AFEGAIN for PGA460.
 *
 *  Function to set analog front end gain for PGA460.
 *
 *  @pre    PGA460_open() has to be called first
 *
 *  @param  handle  A PGA460_Handle
 *
 *  @param  afegain       User has to specify the afegain. Currently  4
 *                        presets are supported. If the setting is CUSTOM then
 *                        the onus is on user to provide initialized afegain
 *                        byte.
 *
 *  @param  afegain       A pointer to afegain byte. If NULL, it
 *                        will use default values.
 *
 *  @param broadcastEnable this can take value of PGA460_broadcast_Enable or
 *                         PGA460_broadcast_disable. If broadcast is enabled
 *                         then the broadcast command code is used.
 *
 *  @return false if afegain pointer is null else return true.
 *
 *  @sa      PGA460_afegain
 */
extern bool PGA460_setAFEGain(PGA460_Handle handle,
        PGA460_afegain afegain, PGA460_broadcast broadcastEnable);

/*!
 *  @brief  Function to start Burst and Listen command for specific preset.
 *
 *  @pre    PGA460_open() has to be called first
 *
 *  @param  handle  A PGA460_Handle
 *
 *  @param  PGA460_preset  User has to specify the PGA460 preset for which
 *                         the Burst and Listen command has to be issued.
 *
 *  @param  numOfObjects  number of objects to be detected. The same number of
 *                        of objects should be used even in the pull ultrasonic
 *                        measurement results API. Other wise there may be
 *                        failure of API.
 *                         *
 *  @param broadcastEnable This can take value of PGA460_broadcast_Enable or
 *                         PGA460_broadcast_disable. If broadcast is enabled
 *                         then the broadcast command code is used.
 *
 *  @return false if incorrect number of objects specified(max is 8) or if
 *                incorrect preset is specified.
 *
 *  @sa      PGA460_preset
 *  @sa      PGA460_pullUltrasonicMeasurmentResults
 */
extern bool PGA460_burstAndListenCmd(PGA460_Handle handle,
        PGA460_preset preset, uint8_t numOfObjects,
        PGA460_broadcast broadcastEnable);

/*!
 *  @brief  Function to start Burst and Listen command for specific preset.
 *
 *  @pre    PGA460_open() has to be called first
 *
 *  @param  handle  A PGA460_Handle
 *
 *  @param  PGA460_preset  User has to specify the PGA460 preset for which
 *                         the Burst and Listen command has to be issued.
 *
 *  @param  numOfObjects  number of objects to be detected. The same number of
 *                        of objects should be used even in the pull ultrasonic
 *                        measurement results API. Other wise there may be
 *                        failure of API.
 *
 *  @param  broadcastEnable this can take value of PGA460_broadcast_Enable or
 *                         PGA460_broadcast_disable. If broadcast is enabled
 *                         then the broadcast command code is used.
 *
 *  @return false if incorrect number of objects specified(max is 8) or if
 *                incorrect preset is specified.
 *
 *  @sa      PGA460_preset
 *  @sa      PGA460_pullUltrasonicMeasurmentResults
 */
extern bool PGA460_ListenOnlyCmd(PGA460_Handle handle,
        PGA460_preset preset, uint8_t numOfObjects,
        PGA460_broadcast broadcastEnable);

/*!
 *  @brief  Function to start Burst and Listen command for specific preset.
 *
 *  @pre    PGA460_open() has to be called first
 *
 *  @param  handle        A PGA460_Handle
 *
 *  @param  numOfObjects  number of objects the result has to be pulled for.
 *                        This should be same or less than the noOfObjects
 *                        passed when initiating the Burst and Listen or
 *                        ListenOnly commands. Otherwise the behaviour will be
 *                        incorrect.
 *
 *  @param measurementResults  A pointer to byte array of size >= 34. After the
 *                             execution of API this array will hold the
 *                             measurement results. The results returned are
 *                             raw values. The actual distance calculation
 *                             has to be done by the application by compensating
 *                             for the temperature variation and the amount
 *                             spent during bursting.
 *
 *  @param broadcastEnable this can take value of PGA460_broadcast_Enable or
 *                         PGA460_broadcast_disable. If broadcast is enabled
 *                         then the broadcast command code is used.
 *
 *  @param errFlags  The register read and response commands to the PGA460
 *                   return a diagnostic byte as the first byte. This uint16
 *                   contains the error information(it contains diagnostic data
 *                   byte for the previous transaction and contains information
 *                   related to checksum verification of received data)
 *
 *  @return false if results could not be pulled or if passed for
 *                measurement Results is NULL pointer.
 *
 *  @sa      PGA460_burstAndListenCmd
 *  @sa      PGA460_ListenOnlyCmd
 *  @sa      PGA460_respErrorStatusFlags
 */
extern bool PGA460_getMeasurements(PGA460_Handle handle,
               PGA460_ULMResults *measurementResults, uint8_t numberOfObjects,
               PGA460_respErrorStatusFlags *errFlags);

/*!
 *  @brief  Function to set echo dump bit.
 *
 *  Function to set the bit which enables capturing echo data dump.After
 *  calling this function the user has to initiate a burst and listen command
 *  this will start storing the the echo data dump in PGA460 memory.
 *
 *  @pre    PGA460_open() has to be called first
 *
 *  @param  handle   A PGA460_Handle
 *
 *  @return false if failed to enable the echo data dump
 *
 *  @param  broadcastEnable this can take value of PGA460_broadcast_Enable or
 *                         PGA460_broadcast_disable. If broadcast is enabled then
 *                         the broadcast command code is used.
 *
 *  @sa      PGA460_burstAndListenCmd
 */
extern bool PGA460_enableEchoDataDump(PGA460_Handle handle,
                                      PGA460_broadcast broadcastEnable);

/*!
 *  @brief  Function to clear echo dump enable bit.
 *
 *  Function to clear the bit which enables capturing echo data dump.This API
 *  has to be called after collecting the echo data dump. So to collect the
 *  the echo data dump user first enables the echo data dump bit, then runs
 *  a burst and listen command and after elapsing the record length interval
 *  the user disables the echo data dump enable bit by calling this API.
 *  
 *
 *  @pre    PGA460_open() has to be called first
 *
 *  @param  handle   A PGA460_Handle
 *
 *  @return false if failed to enable the echo data dump
 *
 *  @param  broadcastEnable this can take value of PGA460_broadcast_Enable or
 *                         PGA460_broadcast_disable. If broadcast is enabled then
 *                         the broadcast command code is used.
 *
 *  @sa      PGA460_burstAndListenCmd
 */
extern bool PGA460_disableEchoDataDump(PGA460_Handle handle,
                                       PGA460_broadcast broadcastEnable);
        
/*!
 *  @brief  Function to start Burst and Listen command for specific preset.
 *
 *  @pre    PGA460_open() has to be called first
 *
 *  @param  handle  A PGA460_Handle
 *
 *  @param  echoDataDump  A pointer to byte array of size >= 130 bytes. After
 *                        the execution of API this array will hold the
 *                        measurement results
 *
 *  @param errFlags  The register read and response commands to the PGA460
 *                   return a diagnostic byte as the first byte. This uint16
 *                   contains the error information(it contains diagnostic data
 *                   byte for the previous transaction and contains information
 *                   related to checksum verification of received data)
 *
 *  @return false if results could not be pulled or passed pointer
 *         echoDataDump is NULL.
 *
 */
bool PGA460_getEchoDataDump(PGA460_Handle handle,
        uint8_t *echoDataDump, PGA460_respErrorStatusFlags *errFlags);

/*!
 *  @brief  Function to run diagnostics on PGA460.
 *
 *  @pre    PGA460_open() has to be called first
 *
 *  @param  handle  A PGA460_Handle
 *
 *  @param  diagnostic    required diagnostic. Based on the required diagnostic
 *                        UART commands for system diag or for temperature or
 *                        noise measurement is issued to PGA460
 *
 *  @param  diagResult    required diagnostic result is stored in this passed
 *                        pointer
 *
 *  @param errFlags  The register read and response commands to the PGA460
 *                   return a diagnostic byte as the first byte. This uint16
 *                   contains the error information(it contains diagnostic data
 *                   byte for the previous transaction and contains information
 *                   related to checksum verification of received data)
 *
 *  @return false if diagnostics could be ran or passed pointer
 *          diagResult is NULL.
 *
 *  @sa      PGA460_diagnostic
 */
extern bool PGA460_runDiagnostic(PGA460_Handle handle,
        PGA460_diagnostic diagnostic, double *diagResult,
        PGA460_respErrorStatusFlags *errFlags);

/*!
 *  @brief  Function to write to register on PGA460.
 *
 *  @pre    PGA460_open() has to be called first.
 *
 *  @param  handle  A PGA460_Handle
 *
 *  @param  rw           pointer to structure containing register address
 *                       and register data.
 *
 *  @param broadcastEnable this can take value of PGA460_broadcast_Enable or
 *                         PGA460_broadcast_disable. If broadcast is enabled then
 *                         the broadcast command code is used.
 *
 *  @return false if register write was unsuccessful or passed pointer
 *          diagResult is NULL.
 *
 *  @sa      PGA460_RW
 */
extern bool PGA460_writeRegister(PGA460_Handle handle,
        PGA460_RW *rw, PGA460_broadcast broadcastEnable);

/*!
 *  @brief  Function to read register on PGA460. (single register read)
 *
 *  The read of a register in PGA460 can provide a response with one or more
 *  data bytes. This function is for only reading registers with 1 response
 *  byte.
 *
 *  @pre    PGA460_open() has to be called first
 *
 *  @param  handle  A PGA460_Handle
 *
 *  @param  regAddr       register address to be read.
 *
 *  @return false if diagnostics could be ran or passed pointer
 *            diagResult is NULL.
 *
 *  @param errFlags  The register read and response commands to the PGA460
 *                   return a diagnostic byte as the first byte. This uint16
 *                   contains the error information(it contains diagnostic data
 *                   byte for the previous transaction and contains information
 *                   related to checksum verification of received data)
 *
 *  @sa      PGA460_diagnostic
 */
bool PGA460_readRegister(PGA460_Handle handle,uint8_t regAddr,
                        uint8_t *regData,PGA460_respErrorStatusFlags *errFlags);

/*!
 *  @brief  Function to burn EEPROM of PGA460.
 *
 *  @pre    PGA460_open() has to be called first
 *
 *  @param  handle  A PGA460_Handle
 *
 *  @return false if EEPROM burn was not successful
 */
extern bool PGA460_burnEEPROM(PGA460_Handle handle);

/*!
 *  @brief  Function to move PGA460 to low power mode.
 *
 *  @pre    PGA460_open() has to be called first
 *
 *  @param  handle  A PGA460_Handle
 *
 *  @param broadcastEnable this can take value of PGA460_broadcast_Enable or
 *                         PGA460_broadcast_disable. If broadcast is enabled then
 *                         the broadcast command code is used.
 *  @return false if we were not able to set low power mode
 */
bool PGA460_setLowPowerMode(PGA460_Handle handle,
                            PGA460_broadcast broadcastEnable);

/*!
 *  @brief  Function to initialize a PGA460_Params struct to its defaults
 *
 *  @param  params      A pointer to PGA460_Params structure for
 *                      initialization.
 *
 *  Default values are:
 *
 *       comInterface      = PGA460_communicationInterface_UART;
 *       maxWaitTime       = 1000;
 *       lpmTime           = PGA460_lpmTime_500ms;
 */
extern void PGA460_Params_init(PGA460_Params *params);

#ifdef __cplusplus
}
#endif

#endif /* PGA460_H_ */
