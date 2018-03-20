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
 *  ======== pga460.c ========
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

/* Driver Header files */
#include <ti/drivers/UART.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/dpl/HwiP.h>

/* Module Header */
#include <ti/sail/pga460/pga460.h>

/*********************************************************************
 * Constants
 */
/*EEPROM Bulk Write command length(excludes the commandCode, checksum, sync)*/
#define EEBW_CMD_SIZE               43U

/*Threshold Bulk Write command length(excludes the commandCode,
 * checksum, sync)*/
#define THRBW_CMD_SIZE              32U

/*Time Varying Gail Bulk Write command
 *  length(excludes the commandCode, checksum, sync)*/
#define TVGBW_CMD_SIZE              7U

/*Serial Register read command length*/
#define SRR_CMD_SIZE                1U

/*Serial Register write command length*/
#define SRW_CMD_SIZE                2U

/*EEPROM Bulk Write command length*/
#define BURSTLISTEN_CMD_SIZE        1U

/*ECHO DATA DUMP response length this includes the checksum and the diag
 * data*/
#define EDD_RESP_SIZE               130U

/*Ultrasonic measurement results response length this includes the checksum and
 * the diag data*/
#define ULM_RESP_SIZE               34U

/*System diagnostic response length this includes the checksum and the diag
 * data*/
#define SD_RESP_SIZE                4U

/*Temperature Noise Measurement Result  response length this includes the
 * checksum and the diag data*/
#define TNLR_RESP_SIZE              4U

/*EEPROM Bulk Write command length*/

/* Sync byte value. Used in UART communication */
#define PGA460_SYNCBYTE             0x55U

/* UART address mask cmd[7:5] is uart addresss*/
#define PGA460_UART_ADDRESS_pos     5U

/*DATADUMP Enable bit position in  EECTRL */
#define PGA460_DATADUMP_ENABLE_pos  7U

#define PGA460_PULSEP1_UARTDIAG_pos 6U


/*EEPROM unlock Enable field position in  EECTRL */
#define PGA460_EECTRL_EEUNLCK_pos   3U

/*EEPROM program bit position in  EECTRL */
#define PGA460_EECTRL_EEPRG_pos     0U

/*EEPROM  program status bit position in  EECTRL */
#define PGA460_EECTRL_EEPGST_pos    2U

/*UNLOCK EEPROM*/
#define PGA460_EEPROM_UNLOCK        0xDU

/*UNLOCK EEPROM*/
#define PGA460_MAXNOOFOBJECTS       0x8U

/*LPM_TMR field bit position in  FVOLT_DEC */
#define PGA460_FVOLTDEC_LMPTMR_pos  0x3U

/*LPM_EN field bit position in  DECPL_TEMP */
#define PGA460_DECPLTEMP_LPMEN_pos  0x5U


/*********************************************************************
 * TYPEDEFS
 */
/**
 * regular register read structure where in response frame is just having a
 * single data byte.
 */
typedef struct registerReadInfo
{
    /*!
    * dignostic field contains info on communication errors
    */
    uint8_t diagData;
    uint8_t regData;      //!< (single byte)register data
    uint8_t checksum;     //!< checksum value
} registerReadInfo;
/*********************************************************************
 * Global and Extern Variables
 */
extern PGA460_Config PGA460_config[];
extern const uint_least8_t PGA460_count;

/* Default PGA460 parameters structure */
const PGA460_Params PGA460_defaultParams = 
{
    PGA460_communicationInterface_UART,  /* Default interface is 2 wire UART*/
    true,                               /* Low power mode at startup enabled. */
    PGA460_lpmTime_250ms                /* low power mode enter time */
};

/* Two dimensional array holding commands for PGA460. Holds both broadcast and
 * normal commands. Can be addressed using the command code itself as command
 * codes start from 0 and run until 16.For example to get broadcast command for
 * burst and listen one can address like
 * PGA460_CommandList[PGA460_commandCode_P1BL][1];
 *
 */
const PGA460_commandCode PGA460_CommandList[17U][2U]=
{
    {PGA460_commandCode_P1BL,   PGA460_commandCode_BC_P1BL},
    {PGA460_commandCode_P2BL,   PGA460_commandCode_BC_P2BL},
    {PGA460_commandCode_P1LO,   PGA460_commandCode_BC_P2LO},
    {PGA460_commandCode_P2LO,   PGA460_commandCode_BC_P2LO},
    {PGA460_commandCode_TNLM,   PGA460_commandCode_BC_TNLM},
    {PGA460_commandCode_UMR,    PGA460_commandCode_none},
    {PGA460_commandCode_TNLR,   PGA460_commandCode_none},
    {PGA460_commandCode_TEDD,   PGA460_commandCode_none},
    {PGA460_commandCode_SD,     PGA460_commandCode_none},
    {PGA460_commandCode_SRR,    PGA460_commandCode_none},
    {PGA460_commandCode_SRW,    PGA460_commandCode_BC_SRW},
    {PGA460_commandCode_EEBR,   PGA460_commandCode_none},
    {PGA460_commandCode_EEBW,   PGA460_commandCode_BC_EEBW},
    {PGA460_commandCode_TVGBR,  PGA460_commandCode_none},
    {PGA460_commandCode_TVGBW,  PGA460_commandCode_BC_TVGBW},
    {PGA460_commandCode_THRBR,  PGA460_commandCode_BC_THRBW},
    {PGA460_commandCode_THRBW,  PGA460_commandCode_none}
};

/*default parameters for the sensor MA58MF147N*/
const PGA460_EEBW eepromMA58MF147N =
{
    {0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,
    0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U},{0xAAU,0xAAU,
    0xAAU,0x82U,0x08U,0x20U,0x80U},0x60U,0x8FU,0xA0U,0x04U,0x10U,0x55U,
    0x55U,0x19U,0x33U,0xEEU,0x7CU,0x4FU,0x00U,0x00U,0x09U,0x09U
};

/*default parameters for the sensor MA40H1SR*/
const PGA460_EEBW eepromMA40H1SR =
{
    {0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,
    0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U,0x00U},{0xAAU,0xAAU,
    0xAAU,0x51U,0x45U,0x14U,0x50U},0x54U,0x32U,0xA0U,0x08U,0x00U,0x40U,
    0x55U,0x19U,0x33U,0xEEU,0x7CU,0x4FU,0x00U,0x00U,0x09U,0x09U
};

/*default values for the 25% threshold*/
const PGA460_THRBW thresholdvalues25=
{
    {0x88U,0x88U,0x88U,0x88U,0x88U,0x88U,0x42U,0x10U,0x84U,0x21U,0x08U,
    0x40U,0x40U,0x40U,0x40U,0x00U},{0x88U,0x88U,0x88U,0x88U,0x88U,0x88U,
    0x42U,0x10U,0x84U,0x21U,0x08U,0x40U,0x40U,0x40U,0x40U,0x00U}
};

/*default values for the 50% threshold*/
const PGA460_THRBW thresholdvalues50=
{
    {0x88U,0x88U,0x88U,0x88U,0x88U,0x88U,0x84U,0x21U,0x42U,0x10U,0x10U,
    0x80U,0x80U,0x80U,0x80U,0x00U},{0x88U,0x88U,0x88U,0x88U,0x88U,0x88U,
    0x84U,0x21U,0x42U,0x10U,0x10U,0x80U,0x80U,0x80U,0x80U,0x00U}
};

/*default values for the 75% threshold*/
const PGA460_THRBW thresholdvalues75=
{
    {0x88U,0x88U,0x88U,0x88U,0x88U,0x88U,0xC6U,0x31U,0x8CU,0x63U,0x18U,
    0xC0U,0xC0U,0xC0U,0xC0U,0x00U},{0x88U,0x88U,0x88U,0x88U,0x88U,0x88U,
    0xC6U,0x31U,0x8CU,0x63U,0x18U,0xC0U,0xC0U,0xC0U,0xC0U,0x00U}
};

/*default values for the 25% threshold*/
const PGA460_TVGBW timeVaryinGainValues25=
{
    {0x88U,0x88U,0x88U,0x41U,0x04U,0x10U,0x40U}
};

/*default values for the 50% threshold*/
const PGA460_TVGBW timeVaryinGainValues50=
{
    {0x88U,0x88U,0x88U,0x82U,0x08U,0x20U,0x80U}
};

/*default values for the 75% threshold*/
const PGA460_TVGBW timeVaryinGainValues75=
{
    {0x88U,0x88U,0x88U,0xC3U,0x0CU,0x30U,0xC0U}
};

/* The length of biggest command in PGA460 is 46 bytes*/
static uint8_t UARTMessage[46U];
static SemaphoreP_Handle    UARTMessageSem;          /* UART read semaphore */
/* Keep track of the amount of pga460 handle instances */
static uint_fast16_t pga460Instance = 0U;

static bool isInitialized = false;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*
 *  ======== calculateChecksum ========
 * Checksum calculation routine for the message sent to the PGA460.
 * check the PGA460 data sheet for more information on this calculation.
 */

static uint8_t calculateChecksum(uint8_t *pMsg, uint8_t checksumLoops)
{
    uint16_t carry = 0U;
    uint8_t i;

    for(i=0U; i < checksumLoops; i++)
    {
        if ((pMsg[i] + carry) < carry)
        {
            carry = carry + pMsg[i] + 1U;
        }
        else
        {
            carry = carry + pMsg[i];
        }

        if (carry > 0xFF)
        {
          carry = carry - 255U;
        }
    }
    carry = (~carry & 0x00FFU);

    return ((uint8_t)carry);
}

/*
 *  ======== PGA460_sendCommand ========
 *  Sends the command over UART
 */
static bool PGA460_sendCommand(PGA460_Handle handle,
                               uint8_t commandCode, uint8_t *pMsg,
                               uint8_t commandDataLength)
{
  /*initial length of the packet is 2 including SYNC byte(1),commandCode(1)*/
  bool retVal = true;
  uint8_t len = 2U + commandDataLength;
  uint8_t checksum = 0U;
  PGA460_Object *obj = handle->object;
  UART_Handle UARTHandle = *((UART_Handle*)(obj->comInterfaceHandle));

  SemaphoreP_pend(UARTMessageSem, SemaphoreP_WAIT_FOREVER);
  UARTMessage[0U] = PGA460_SYNCBYTE;
  // Copy command code to the buffer
  UARTMessage[1U] = commandCode;
  /*copy message and checksum if data length is not 0 and if pMsg is not
   * NULL pointers
   */
  if((0U != commandDataLength) && (NULL != pMsg))
  {
      memcpy(&UARTMessage[2U], pMsg, commandDataLength);
  }
  /*If data length is 0 then no need to copy pMsg, ignore it*/
  else if(0U == commandDataLength)
  {
      /*continue nothing to be handled here and it is not an error case*/
  }
  /*if data length is not 0 and pMsg is NULL then set retVal as false*/
  else
  {
      retVal = false;
  }
  if(true == retVal)
  {
      /*copy checksum*/
      checksum = calculateChecksum((uint8_t*)&UARTMessage[1U], len-1U);
      memcpy(&UARTMessage[len], &checksum,1U);
      /*Checksum is of 2 bytes so increase th length by 2*/
      len += 1U;
      if(PGA460_communicationInterface_UART == obj->comInterface)
      {
          if(NULL != UARTHandle)
          {
              UART_write(UARTHandle, UARTMessage, len);
          }
          else
          {
              retVal = false;
          }
      }
      /* Other interface implementations can be added here. Right now the module
       * supports only the UART interface
       */
      else
      {
          retVal = false;
      }
  }
  SemaphoreP_post(UARTMessageSem);
  return (retVal);
}

/*
 *  ======== verifyChecksum ========
 *  verifies whether received diagnostic information and checksum is correct
 *  returns true in case there are no failures observed in the previous
 *  transaction.
 *
 *  This API takes the input as the data read in the previous read command or
 *  response command and also the data length and the checksum value.
 */
static bool verifyChecksum(uint8_t checksum, uint8_t* data, uint8_t dataLength,
                           PGA460_respErrorStatusFlags *errFlags)
{
    bool retVal = true;

    /*First byte of the data recieved is the Diagnostic byte*/
    *errFlags = (uint16_t)data[0];
    /*Verify Checksum of register read*/
    if(checksum == calculateChecksum(data, dataLength))
    {
        retVal = true;
    }
    else
    {
        retVal = false;
        *errFlags |= PGA460_CHECKSUMERRORFLAG;
    }

    return (retVal);
}

/*********************************************************************
 * API FUNCTIONS
 */
/*
 *  ======== PGA460_init ========
 */
void PGA460_init()
{
    uint_least8_t i;

    if (isInitialized == false) 
    {
        isInitialized = (bool) true;
        pga460Instance = 0;
        
        for (i = 0U; PGA460_config[i].object != NULL; i++) 
        {
            ((PGA460_Object *)
                    (PGA460_config[i].object))->comInterfaceHandle = NULL;
        }
    }
}

/*
 *  ======== PGA460_open ========
 *  Setups PGA460 sensor and returns PGA460_Handle
 */
PGA460_Handle PGA460_open(unsigned int index,
        void* comInterfaceHandle, PGA460_Params *params)
{
    uintptr_t                  key;
    PGA460_Handle handle = &PGA460_config[index];
    PGA460_Object *obj = (PGA460_Object*)(PGA460_config[index].object);
    SemaphoreP_Params       semParams;
    PGA460_RW rw;

    
    if(isInitialized && (index < PGA460_count))
    {
        if (obj->comInterfaceHandle != NULL) {
            return (NULL);
        }
        obj->comInterfaceHandle = comInterfaceHandle;

        if (params == NULL) {
            params = (PGA460_Params *) &PGA460_defaultParams;
        }
        /* Determine if the driver was already opened */
        key = HwiP_disable();
        /*create semaphore for the UARTMessage buffer*/
        if (pga460Instance == 0U) {
            SemaphoreP_Params_init(&semParams);
            semParams.mode = SemaphoreP_Mode_BINARY;
            UARTMessageSem = SemaphoreP_create(1U, &semParams);
            if (UARTMessageSem == NULL) {
                return (NULL);
            }
        }
        pga460Instance++;
        HwiP_restore(key);
        if(PGA460_communicationInterface_UART == params->comInterface)
        {
            obj->comInterface = PGA460_communicationInterface_UART;
            /*todo Do the initialization of the PGA460 here*/

            /* set lpm timer value */
            rw.regAddr = PGA460_FVOLT_DEC;
            /*set lpmTime specified in params structure*/
            rw.regData = params->lpmTime << PGA460_FVOLTDEC_LMPTMR_pos;
            /*the command sent here is not broadcast, For each pga460 instances
            * there will be seperate open call
            */
            if(true == 
               PGA460_writeRegister(handle, &rw, PGA460_broadcast_Disable))
            {
                return (handle);
            }
            else
            {
                return (NULL);
            }
        }
        /*other interfaces are not implemented*/
        else
        {
            return (NULL);
        }
    }
    else
    {
        return (NULL);
    }
}

bool PGA460_close(PGA460_Handle handle)
{
    PGA460_Object *obj = handle->object;
    bool retVal = true;

    obj->comInterfaceHandle = NULL;
    /*delete UARTMessageSem in case there are no open instances of the PGA460*/
    pga460Instance--;
    if (pga460Instance == 0U) {
        if (UARTMessageSem) {
            SemaphoreP_delete(UARTMessageSem);
            UARTMessageSem = NULL;
        }
    }

    /*todo set to low power or shutdown the PGA460*/

    return (retVal);
}

/*
 *  ======== PGA460_Params_init ========
 *  Initialize a PGA460_Params struct to default settings.
 */
void PGA460_Params_init(PGA460_Params *params)
{
    *params = PGA460_defaultParams;
}

/*
 *  ======== PGA460_configure ========
 *  Configure the PGA460 for particular transducer type.
 */
bool PGA460_configure(PGA460_Handle handle,
                PGA460_ultrasonicSensorType sensorType, const PGA460_EEBW *eebw,
                PGA460_broadcast broadcastEnable)
{
    bool retVal = true;
    PGA460_Object *obj = handle->object;

    if(PGA460_communicationInterface_UART == obj->comInterface)
    {
        PGA460_commandCode ultrasonicCmd =
                PGA460_CommandList[PGA460_commandCode_EEBW][broadcastEnable];
        PGA460_UARTHWAttrs *hwAttrs =
                (PGA460_UARTHWAttrs*)handle->hwAttrs;
        uint8_t uartAddress = hwAttrs->uartAddress;

        switch(sensorType)
        {
        case PGA460_ultrasonicSensorType_MA58MF147N:
            /*default parameters for the sensor MA58MF147N*/
            eebw = &eepromMA58MF147N;
            break;
        case PGA460_ultrasonicSensorType_MA40H1SR:
            /*default parameters for the sensor MA40H1SR*/
            eebw = &eepromMA40H1SR;
            break;
        case PGA460_ultrasonicSensorType_CUSTOM:
            if(NULL == eebw)
            {
                retVal = false;
            }
            break;
        default:
            retVal = false;
            break;
        }

        if (true == retVal)
        {
            if(true == PGA460_sendCommand(handle,
                       (uint8_t)(ultrasonicCmd |
                               (uartAddress << PGA460_UART_ADDRESS_pos)),
                       (uint8_t*)eebw, EEBW_CMD_SIZE))
            {
                retVal = true;
                /*wait for 50 micro second after any write register*/
                /*Wait for 50 us before issuing a read*/
                usleep(50U);
            }
            else
            {
                retVal =  false;
            }
        }
    }
    /* other interfaces not implemented*/
    else
    {
        retVal = false;
    }

    return (retVal);
}

/*
 *  ======== PGA460_setThreshholds ========
 *  Configure the PGA460 Preset 1 and 2 thresholds.
 */
bool PGA460_setThreshholds(PGA460_Handle handle,
                          PGA460_threshold threshold, const PGA460_THRBW *thrbw,
                          PGA460_broadcast broadcastEnable)
{
    bool retVal = true;
    PGA460_Object *obj = handle->object;

    if(PGA460_communicationInterface_UART == obj->comInterface)
    {
        PGA460_commandCode ultrasonicCmd =
                PGA460_CommandList[PGA460_commandCode_THRBW][broadcastEnable];
        PGA460_UARTHWAttrs *hwAttrs =
                (PGA460_UARTHWAttrs*)handle->hwAttrs;
        uint8_t uartAddress = hwAttrs->uartAddress;

        switch(threshold)
        {
        case PGA460_threshold_25:
            thrbw = &thresholdvalues25;
            break;
        case PGA460_threshold_50:
            thrbw = &thresholdvalues50;
            break;
        case PGA460_threshold_75:
            thrbw = &thresholdvalues75;
            break;
        case PGA460_threshold_custom:
            if(NULL == thrbw)
            {
                retVal = false;
            }
            break;
        default:
            retVal = false;
        }

        if(true == retVal)
        {
            if(true == PGA460_sendCommand(handle,
                       (uint8_t)(ultrasonicCmd |
                               (uartAddress << PGA460_UART_ADDRESS_pos)),
                       (uint8_t*)thrbw, THRBW_CMD_SIZE))
            {
                retVal = true;
                /*wait for 50 micro second after any write register*/
                /*Wait for 50 us before issuing a read*/
                usleep(50U);                
            }
            else
            {
                retVal =  false;
            }
        }
    }
    /* other interfaces not implemented*/
    else
    {
        retVal = false;
    }

    return (retVal);
}

/*
 *  ======== PGA460_setTimeVaryingGain ========
 *  Configure the PGA460 time varying gain
 */
bool PGA460_setTimeVaryingGain(PGA460_Handle handle,
                               PGA460_tvgain tvgain, const PGA460_TVGBW *tvgbw,
                               PGA460_broadcast broadcastEnable)
{
    bool retVal = true;
    PGA460_Object *obj = handle->object;

    if(PGA460_communicationInterface_UART == obj->comInterface)
    {
        PGA460_commandCode ultrasonicCmd =
                PGA460_CommandList[PGA460_commandCode_TVGBW][broadcastEnable];
        PGA460_UARTHWAttrs *hwAttrs =
                (PGA460_UARTHWAttrs*)handle->hwAttrs;
        uint8_t uartAddress = hwAttrs->uartAddress;

        switch(tvgain)
        {
        case PGA460_tvgain_25:
            tvgbw = &timeVaryinGainValues25;
            break;
        case PGA460_tvgain_50:
            tvgbw = &timeVaryinGainValues50;
            break;
        case PGA460_tvgain_75:
            tvgbw = &timeVaryinGainValues75;
            break;
        case PGA460_tvgain_custom:
            if(NULL == tvgbw)
            {
                retVal = false;
            }
            break;
        default:
            retVal = false;
        }

        if(true == retVal)
        {
            if(true == PGA460_sendCommand(handle,
                       (uint8_t)(ultrasonicCmd |
                               (uartAddress << PGA460_UART_ADDRESS_pos)),
                       (uint8_t*)tvgbw, TVGBW_CMD_SIZE))
            {
                retVal = true;
                /*wait for 50 micro second after any write register*/
                /*Wait for 50 us before issuing a read*/
                usleep(50U);                
            }
            else
            {
                retVal =  false;
            }
        }
    }
    /* other interfaces not implemented*/
    else
    {
        retVal = false;
    }

    return (retVal);
}

/*
 *  ======== PGA460_setAFEGain ========
 *  Configure the PGA460 analog front end gain
 */
bool PGA460_setAFEGain(PGA460_Handle handle, PGA460_afegain afegain,
                       PGA460_broadcast broadcastEnable)
{
    bool retVal = true;
    PGA460_Object *obj = handle->object;

    if(PGA460_communicationInterface_UART == obj->comInterface)
    {
        PGA460_RW registerInfo;
        PGA460_commandCode ultrasonicCmd =
                PGA460_CommandList[PGA460_commandCode_SRW][broadcastEnable];
        PGA460_UARTHWAttrs *hwAttrs =
                (PGA460_UARTHWAttrs*)handle->hwAttrs;
        uint8_t uartAddress = hwAttrs->uartAddress;

        /*register which contains bitfields for the AFE gain*/
        registerInfo.regAddr = 0x26U;
        switch(afegain)
        {
        case PGA460_afegain_32_64:
            registerInfo.regData = 0xCFU;
            break;

        case PGA460_afegain_46_78:
            registerInfo.regData = 0x8FU;
            break;

        case PGA460_afegain_52_84:
            registerInfo.regData = 0x4FU;
            break;

        case PGA460_afegain_58_90:
            registerInfo.regData = 0x0FU;
            break;

        default:
            registerInfo.regData =  0x4FU;
            break;
        }

        if(true == retVal)
        {
            if(true == PGA460_sendCommand(handle,
                       (uint8_t)(ultrasonicCmd |
                               (uartAddress << PGA460_UART_ADDRESS_pos)),
                       (uint8_t*)&registerInfo, SRW_CMD_SIZE))
            {
                retVal = true;
                /*wait for 60 micro second after any write register*/
                /*Wait for 60 us before issuing a read*/
                usleep(60U);
            }
            else
            {
                retVal =  false;
            }
        }
    }
    /* other interfaces not implemented*/
    else
    {
        retVal = false;
    }

    return (retVal);
}

/*
 *  ======== PGA460_readRegister ========
 * Initiate a read register command.Reads a particular register address
 */
bool PGA460_readRegister(PGA460_Handle handle, uint8_t regAddr,
                         uint8_t *regData,
                         PGA460_respErrorStatusFlags *errFlags)
{
    bool retVal = true;
    PGA460_Object *obj = handle->object;
    registerReadInfo regInfo;

    /*entry condition check, return in case of failure here itself*/
    if(NULL == regData || NULL == errFlags)
    {
        return (false);
    }
    if(PGA460_communicationInterface_UART == obj->comInterface)
    {
        PGA460_UARTHWAttrs *hwAttrs =
                (PGA460_UARTHWAttrs*)handle->hwAttrs;
        uint8_t uartAddress = hwAttrs->uartAddress;
        UART_Handle UARTHandle = *((UART_Handle*)(obj->comInterfaceHandle));

        if(true == retVal)
        {
            /*Read the register specified*/
            if(true == PGA460_sendCommand(handle,
                       (uint8_t)(PGA460_commandCode_SRR |
                               (uartAddress << PGA460_UART_ADDRESS_pos)),
                       (uint8_t*)&regAddr, SRR_CMD_SIZE))
            {
                retVal = true;
            }
            else
            {
                retVal =  false;
            }
            /* A register read returns 3 bytes - 1 diagnostic byte 1 data byte
             * and 1 checksum
             */
            UART_read(UARTHandle, (uint8_t*)&regInfo, 3U);

            if(true == verifyChecksum(regInfo.checksum,
                                      (uint8_t*)&regInfo, 2U,
                                      errFlags))
            {
                *regData = regInfo.regData;
                retVal = true;
            }
            else
            {
                retVal = false;
            }
        }
    }
    /* other interfaces not implemented*/
    else
    {
        retVal = false;
    }

    return (retVal);
}

/*
 *  ======== PGA460_writeRegister ========
 * Initiate write to a register on PGA460.
 */
bool PGA460_writeRegister(PGA460_Handle handle, PGA460_RW *rw,
                          PGA460_broadcast broadcastEnable)
{
    bool retVal = true;
    PGA460_Object *obj = handle->object;

    /*entry condition check*/
    if(NULL == rw)
    {
        return (false);
    }
    if(PGA460_communicationInterface_UART == obj->comInterface)
    {
        PGA460_commandCode ultrasonicCmd =
                PGA460_CommandList[PGA460_commandCode_SRW][broadcastEnable];;
        PGA460_UARTHWAttrs *hwAttrs =
                (PGA460_UARTHWAttrs*)handle->hwAttrs;
        uint8_t uartAddress = hwAttrs->uartAddress;

        if(NULL != rw)
        {
            if(true == PGA460_sendCommand(handle,
                       (uint8_t)(ultrasonicCmd |
                               (uartAddress << PGA460_UART_ADDRESS_pos)),
                       (uint8_t*)rw, SRW_CMD_SIZE))
            {
                retVal = true;
                /*wait for 60 micro second after any write register*/
                /*Wait for 60 us before issuing a read*/
                usleep(60U);
            }
            else
            {
                retVal =  false;
            }
        }
        else
        {
            retVal = false;
        }
    }
    /* other interfaces not implemented*/
    else
    {
        retVal = false;
    }
    return (retVal);
}

/*
 *  ======== PGA460_burstAndListenCmd ========
 * Initiate burst and listen command on specified preset.
 */
bool PGA460_burstAndListenCmd(PGA460_Handle handle, PGA460_preset preset,
                              uint8_t numOfObjects,
                              PGA460_broadcast broadcastEnable)
{
    bool retVal = true;
    PGA460_Object *obj = handle->object;

    if(PGA460_communicationInterface_UART == obj->comInterface)
    {
        PGA460_commandCode ultrasonicCmd = PGA460_commandCode_P1BL;
        PGA460_UARTHWAttrs *hwAttrs =
                (PGA460_UARTHWAttrs*)handle->hwAttrs;
        uint8_t uartAddress = hwAttrs->uartAddress;

        if(PGA460_presets_PRESET1 == preset)
        {
            ultrasonicCmd =
                   PGA460_CommandList[PGA460_commandCode_P1BL][broadcastEnable];
        }
        else if(PGA460_presets_PRESET2 == preset)
        {
            ultrasonicCmd =
                   PGA460_CommandList[PGA460_commandCode_P2BL][broadcastEnable];
        }
        else
        {
            retVal = false;
        }
        if(true == retVal)
        {
            if(true == PGA460_sendCommand(handle,
                       (uint8_t)(ultrasonicCmd |
                               (uartAddress << PGA460_UART_ADDRESS_pos)),
                       (uint8_t*)&numOfObjects, BURSTLISTEN_CMD_SIZE))
            {
                retVal = true;
            }
            else
            {
                retVal =  false;
            }
        }
    }
    /* other interfaces not implemented*/
    else
    {
        retVal = false;
    }
    return (retVal);
}

/*
 *  ======== PGA460_ListenOnlyCmd ========
 * Initiate listen only command on specified preset.
 */
bool PGA460_ListenOnlyCmd(PGA460_Handle handle, PGA460_preset preset,
                          uint8_t numOfObjects,
                          PGA460_broadcast broadcastEnable)
{
    bool retVal = true;
    PGA460_Object *obj = handle->object;

    if(PGA460_communicationInterface_UART == obj->comInterface)
    {
        PGA460_commandCode ultrasonicCmd = PGA460_commandCode_P1LO;
        PGA460_UARTHWAttrs *hwAttrs =
                (PGA460_UARTHWAttrs*)handle->hwAttrs;
        uint8_t uartAddress = hwAttrs->uartAddress;

        if(PGA460_presets_PRESET1 == preset)
        {
            ultrasonicCmd =
               PGA460_CommandList[PGA460_commandCode_P1LO][broadcastEnable];
        }
        else if(PGA460_presets_PRESET2 == preset)
        {
            ultrasonicCmd =
               PGA460_CommandList[PGA460_commandCode_P2LO][broadcastEnable];
        }
        else
        {
            retVal = false;
        }
        if(true == retVal)
        {
            if(true == PGA460_sendCommand(handle,
                       (uint8_t)(ultrasonicCmd |
                               (uartAddress << PGA460_UART_ADDRESS_pos)),
                       (uint8_t*)&numOfObjects, BURSTLISTEN_CMD_SIZE))
            {
                retVal = true;
            }
            else
            {
                retVal =  false;
            }
        }
    }
    /* other interfaces not implemented*/
    else
    {
        retVal = false;
    }

    return (retVal);
}

/*
 *  ======== PGA460_enableEchoDataDump ========
 * enable Echo data dump on pga460.
 */
bool PGA460_enableEchoDataDump(PGA460_Handle handle,
                               PGA460_broadcast broadcastEnable)
{
    bool retVal = true;
    PGA460_Object *obj = handle->object;

    if(PGA460_communicationInterface_UART == obj->comInterface)
    {
        PGA460_RW rw;

        /*Enable the data dump in EE_CTRL register*/
        rw.regAddr = PGA460_EE_CNTRL;
        /*set the data dump enable bit we are not considerate about other bits
         * here as they are expected to be 0. There is no need of read modify
         * write here*/
        rw.regData = 1U << PGA460_DATADUMP_ENABLE_pos;
        retVal = PGA460_writeRegister(handle, &rw, broadcastEnable);
    }
    /* other interfaces not implemented*/
    else
    {
        retVal = false;
    }

    return (retVal);
}

/*
 *  ======== PGA460_disableEchoDataDump ========
 * disable Echo data dump on pga460.
 */
bool PGA460_disableEchoDataDump(PGA460_Handle handle,
                                PGA460_broadcast broadcastEnable)
{
    bool retVal = true;
    PGA460_Object *obj = handle->object;

    if(PGA460_communicationInterface_UART == obj->comInterface)
    {
        PGA460_RW rw;

        /*Enable the data dump in EE_CTRL register*/
        rw.regAddr = PGA460_EE_CNTRL;
        /*set the data dump enable bit we are not considerate about other bits
         * here as they are expected to be 0. There is no need of read modify
         * write here*/
        rw.regData = 0U << PGA460_DATADUMP_ENABLE_pos;
        retVal = PGA460_writeRegister(handle, &rw, broadcastEnable);
    }
    /* other interfaces not implemented*/
    else
    {
        retVal = false;
    }

    return (retVal);
}

/*
 *  ======== PGA460_pullEchoDataDump ========
 *  pull the echo data dump(128 byte data).
 */
bool PGA460_getEchoDataDump(PGA460_Handle handle, uint8_t *echoDataDump,
                            PGA460_respErrorStatusFlags *errFlags)
{
    bool retVal = true;
    PGA460_Object *obj = handle->object;

    /*entry condition check*/
    if((NULL == echoDataDump) || (NULL == errFlags))
    {
        return (false);
    }
    else if(PGA460_communicationInterface_UART == obj->comInterface)
    {
        PGA460_UARTHWAttrs *hwAttrs =
                (PGA460_UARTHWAttrs*)handle->hwAttrs;
        uint8_t uartAddress = hwAttrs->uartAddress;
        UART_Handle UARTHandle = *((UART_Handle*)(obj->comInterfaceHandle));

        if(true == PGA460_sendCommand(handle,
                   (uint8_t)(PGA460_commandCode_TEDD |
                           (uartAddress << PGA460_UART_ADDRESS_pos)),
                   (uint8_t*)NULL,0U))
        {
          retVal = true;
          /* A response will be of size 128 bytes
           */
          UART_read(UARTHandle, (uint8_t*)echoDataDump, EDD_RESP_SIZE);

          /*The first byte is a diag byte which is not part of the checksum
           * calculation and the checksum byte is the 130th byte. The total
           * actual data is 120
           */
          retVal = verifyChecksum(echoDataDump[EDD_RESP_SIZE-1U],
                                  &echoDataDump[0U], EDD_RESP_SIZE-1U,
                                  errFlags);
        }
        else
        {
            retVal = false;
        }

    }
    /* other interfaces not implemented*/
    else
    {
        retVal = false;
    }

    return (retVal);
}

/*
 *  ======== PGA460_getUltrasonicMeasurementResults ========
 *  pull the ultrasonic measurement results.Maximum 8 objects
 *  can be detected.
 */
extern bool PGA460_getMeasurements(PGA460_Handle handle,
               PGA460_ULMResults *measurementResults, uint8_t numberOfObjects,
               PGA460_respErrorStatusFlags *errFlags)
{
    bool retVal = true;
    PGA460_Object *obj = handle->object;
    uint8_t ULMResults[ULM_RESP_SIZE];
    uint8_t i;

    if((NULL == measurementResults)||(NULL == errFlags)
            ||(PGA460_MAXNOOFOBJECTS < numberOfObjects))
    {
        return (false);
    }
    if(PGA460_communicationInterface_UART == obj->comInterface)
    {
        PGA460_UARTHWAttrs *hwAttrs =
                (PGA460_UARTHWAttrs*)handle->hwAttrs;
        uint8_t uartAddress = hwAttrs->uartAddress;
        UART_Handle UARTHandle = *((UART_Handle*)(obj->comInterfaceHandle));

        if(true == PGA460_sendCommand(handle,
                   (uint8_t)(PGA460_commandCode_UMR |
                           (uartAddress << PGA460_UART_ADDRESS_pos)),
                   (uint8_t*)NULL, 0U))
        {
            retVal = true;
            /* A response will be the number of objects * 4(the result of each
             * object contains 4 bytes, 2 bytes for distance, 1 byte for the width
             * and 1 byte for the Amplitude.
             * 2 bytes for the diag data and the checsum read
             */
            UART_read(UARTHandle, (uint8_t*)ULMResults, numberOfObjects*4U+2U);

            /*The first byte is a diag byte which is not part of the checksum
             * calculation and the checksum byte is the 130th byte. The total
             * actual data is 120
             */
            if(true == verifyChecksum(ULMResults[(numberOfObjects*4U+2U)-1U],
                                      &ULMResults[0U],
                                      numberOfObjects*4U+1U,
                                      errFlags))
            {
                /*ignore first byte as it is the diag byte*/
                /*Conversion has of distance to meters has to be handled
                 *by applciation.
                 */
                for(i =1U;i<=numberOfObjects*4U;)
                {
                    measurementResults->distance =
                            ((ULMResults[i]<<8U) + ULMResults[i+1U]);
                    measurementResults->width = ULMResults[i+2U];
                    measurementResults->width = ULMResults[i+3U];
                    measurementResults++;
                    i += 4U;
                }
            }
            else
            {
                retVal = false;
            }
        }
        else
        {
            retVal = false;
        }

    }
    /* other interfaces not implemented*/
    else
    {
        retVal = false;
    }

    return (retVal);
}

/*
 *  ======== PGA460_runDiagnostic ========
 * Run the system system diagnostic or the temperature and noise level
 * measurement.
 */
bool PGA460_runDiagnostic(PGA460_Handle handle, PGA460_diagnostic diagnostic,
                          double *diagResult,
                          PGA460_respErrorStatusFlags *errFlags)
{
    uint8_t diagMeasResult[SD_RESP_SIZE];
    uint8_t tempNoiseMeasResult[TNLR_RESP_SIZE];
    uint8_t tnlmParameter;
    bool retVal = true;
    PGA460_Object *obj = handle->object;

    /*entry condition check, return in case of failure here itself*/
    if(NULL == diagResult || NULL == errFlags)
    {
        return false;
    }
    if(PGA460_communicationInterface_UART == obj->comInterface)
    {
        PGA460_UARTHWAttrs *hwAttrs =
                (PGA460_UARTHWAttrs*)handle->hwAttrs;
        uint8_t uartAddress = hwAttrs->uartAddress;
        UART_Handle UARTHandle = *((UART_Handle*)(obj->comInterfaceHandle));

        switch (diagnostic)
        {
            case PGA460_diagnostic_TF: // transducer frequency
            case PGA460_diagnostic_DPT:
                if(true == PGA460_sendCommand(handle,
                           (uint8_t)(PGA460_commandCode_SD |
                                   (uartAddress << PGA460_UART_ADDRESS_pos)),
                           (uint8_t*)NULL, 0U))
                {
                    /* A response will be of size 128 bytes
                     */
                    if(UART_ERROR == UART_read(UARTHandle,
                                        (uint8_t*)diagMeasResult, SD_RESP_SIZE))
                    {
                        while(1);
                    }

                    /*The first byte is a diag byte which is not part of the
                     * checksum calculation and the checksum byte is the
                     * 130th byte. The total actual data is 120
                     */
                    retVal = verifyChecksum(diagMeasResult[SD_RESP_SIZE-1U],
                                           &diagMeasResult[0U], SD_RESP_SIZE-1U,
                                           errFlags);
                    if(retVal == true)
                    {
                        if(PGA460_diagnostic_TF == diagnostic)
                        {
                            *diagResult =
                                (1U / (diagMeasResult[1U] * 0.0000005)) / 1000U;
                        }
                        else
                        {
                            *diagResult = diagMeasResult[2U] * 16U;
                        }
                    }
                }
            break;

            case PGA460_diagnostic_TLM: //temperature
            case PGA460_diagnostic_NLM: //noise
                /*Parameter for the TNLM command. O in case of the
                 * temperature level measurement and 1 in case of Noise level
                 * measurement.
                 */
                if(PGA460_diagnostic_TLM == diagnostic)
                {
                    tnlmParameter = 0U;
                }
                else
                {
                    tnlmParameter = 1U;
                }
                retVal = PGA460_sendCommand(handle,
                                         (uint8_t)(PGA460_commandCode_TNLM |
                                     (uartAddress << PGA460_UART_ADDRESS_pos)),
                                     (uint8_t*)&tnlmParameter, 1U);
                usleep(100000U);
                retVal &= PGA460_sendCommand(handle,
                                         (uint8_t)(PGA460_commandCode_TNLR |
                                     (uartAddress << PGA460_UART_ADDRESS_pos)),
                                     (uint8_t*)NULL, 0U);
                if(true == retVal)
                {
                    /* A response will be of size 128 bytes
                     */
                    UART_read(UARTHandle,(uint8_t*)tempNoiseMeasResult,
                              TNLR_RESP_SIZE);

                    /*The first byte is a diag byte which is not part of the checksum
                     * calculation and the checksum byte is the 130th byte. The total
                     * actual data is 120
                     */
                    retVal = verifyChecksum(tempNoiseMeasResult[TNLR_RESP_SIZE-1U],
                                    &tempNoiseMeasResult[0U], TNLR_RESP_SIZE-1U,
                                      errFlags);
                    if(retVal == true)
                    {
                        if(PGA460_diagnostic_TLM == diagnostic)
                        {
                            *diagResult =
                                    (tempNoiseMeasResult[1U] - 64U) / 1.5;
                        }
                        else
                        {
                            *diagResult = tempNoiseMeasResult[2U];
                        }
                    }
                }
                break;

            default:
                break;
        }
    }
    else
    {
        retVal = false;
    }

    return (retVal);
}

/*
 *  ======== PGA460_burnEEPROM ========
 * Burn the EEPROM in PGA460 with the currently configured values.
 */
bool PGA460_burnEEPROM(PGA460_Handle handle)
{
    PGA460_RW rw;
    PGA460_respErrorStatusFlags errFlags;
    uint8_t regData;
    bool retVal = true;

    /*set UNLOCK field in EE_CTRL register to 0xD*/
    rw.regAddr = PGA460_EE_CNTRL;
    rw.regData = (PGA460_EEPROM_UNLOCK << PGA460_EECTRL_EEUNLCK_pos);
    retVal = PGA460_writeRegister(handle, &rw, PGA460_broadcast_Disable);


    /*set EEPROM PROGRAM bit in EE_CTRL register*/
    rw.regAddr = PGA460_EE_CNTRL;
    rw.regData |= (1U << PGA460_EECTRL_EEPRG_pos);
    retVal &= PGA460_writeRegister(handle, &rw, PGA460_broadcast_Disable);
    sleep(1U);

    /*read the EEPROM program status bit to check if the programming was
     * successfull*/
    retVal &= PGA460_readRegister(handle, rw.regAddr, &regData, &errFlags);
    if((0x00U != (regData & (1U<<PGA460_EECTRL_EEPGST_pos))) &&
       (true == retVal))
    {
        retVal = true;
    }
    else
    {
        retVal = false;
    }

    return (retVal);
}

/*
 *  ======== PGA460_setLowPowerMode ========
 * Set PGA460 to low power mode.
 */
bool PGA460_setLowPowerMode(PGA460_Handle handle,
                            PGA460_broadcast broadcastEnable)
{
    PGA460_RW rw;

    rw.regAddr = PGA460_DECPL_TEMP;
    /*set bit LPM_EN in the DECPL_TEMP register*/
    rw.regData = 1U << PGA460_DECPLTEMP_LPMEN_pos;
    if(true == PGA460_writeRegister(handle, &rw, broadcastEnable))
    {
        return (true);
    }
    else
    {
        return (false);
    }
}
