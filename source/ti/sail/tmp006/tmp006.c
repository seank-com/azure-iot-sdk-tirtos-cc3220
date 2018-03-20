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
 *  ======== tmp006.c ========
 */
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>

/* Module Header */
#include <ti/sail/tmp006/tmp006.h>

/* POSIX Header files */
#include <unistd.h>

/* It is recommended to wait for some time(in few ms, defined by below macro)
before TMP006 module is used after issuing a reset*/
#define WAIT_POST_RESET      5U        /* Wait time in ms after reset        */

#define CELSIUS_FAHREN_CONST  1.8F     /* Multiplier for conversion          */
#define CELSIUS_FAHREN_ADD    32U      /* Celsius to Fahrenheit added factor */
#define CELSIUS_TO_KELVIN     273.15F  /* Celsius to Kelvin addition factor  */
#define CELSIUS_PER_LSB       0.03125F /* Degrees celsius per LSB            */

#define READ_BUF_SIZE        2U
#define WRITE_BUF_SIZE       3U

/* TMP006 Configuration Register Bits */
#define CFG_SW_RST           0x8000U  /* Software reset                      */
#define CFG_CONV_ON          0x7000U  /* Conversion on mode                  */
#define CFG_DATARDY          0x0100U  /* Data Ready pin enable               */
#define CFG_DATARDY_STATUS   0x0080U  /* Data Ready pin enable               */

/*
 * The target object temperature calculations consist of a series of equations 
 * that can be used to solve for the target object temperature (TOBJ) in Kelvins.
 * These equations and constants which have been defined here have been clearly
 * exlained the TMP006 users guide - SBOU107. Please refer to the user guide for
 * more information on these constants
 */
#define TMP006_CCONST_A1           1.75E-3
#define TMP006_CCONST_A2           -1.678E-5
#define TMP006_CCONST_TREF         298.15
#define TMP006_CCONST_B0           -2.94E-5
#define TMP006_CCONST_B1           -5.7E-7
#define TMP006_CCONST_B2           4.63E-9
#define TMP006_CCONST_C2           13.4
#define TMP006_CCONST_LSB_SIZE     156.25E-9

/* Default TMP006 parameters structure */
const TMP006_Params TMP006_defaultParams = {
        TMP006_4CONV,             /* 4 ADC Conversions Averaged per Sample */
        TMP006_DataReadyDisable,  /* Data ready pin disabled               */
        NULL,                     /* Callback Function                     */
        6.4E-14                   /* Calibration factor(to be calibrated)  */
};

extern TMP006_Config TMP006_config[];

/*
 *  ======== TMP006_close ========
 *  Closes an instance of a TMP006 sensor.
 */
bool TMP006_close(TMP006_Handle handle)
{
    TMP006_Object *obj = (TMP006_Object *)(handle->object);

    if (obj->callback != NULL)
    {
        TMP006_disableDataReady(handle);
    }

    if (TMP006_disableConversions(handle))
    {
        obj->i2cHandle = NULL;

        return (true);
    }

    return (false);
}

/*
 *  ======== TMP006_disableDataReady ========
 *  Disables data ready signal from a TMP006 sensor.
 */
bool TMP006_disableDataReady(TMP006_Handle handle)
{
    uint16_t reg = 0U;

    if (((TMP006_Object *)(handle->object))->callback == NULL)
    {
        return (true);
    }

    if (TMP006_readRegister(handle, &reg, TMP006_CONFIG))
    {
        reg &= ~CFG_DATARDY;

        if (TMP006_writeRegister(handle, reg, TMP006_CONFIG))
        {
            GPIO_disableInt(((TMP006_HWAttrs *) (handle->hwAttrs))->gpioIndex);

            return (true);
        }
    }

    return (false);
}

/*
 *  ======== TMP006_disableConversions ========
 *  Turns ADC conversion off on a specified TMP006 sensor.
 */
bool TMP006_disableConversions(TMP006_Handle handle)
{

    uint16_t reg = 0U; /* Prevent warning */

    if (TMP006_readRegister(handle, &reg, TMP006_CONFIG))
    {
        reg &= ~CFG_CONV_ON;

        if (TMP006_writeRegister(handle, reg, TMP006_CONFIG))
        {
            return (true);
        }
    }

    return (false);
}

/*
 *  ======== TMP006_enableDataReady ========
 *  Enables data ready interrupt from a TMP006 sensor.
 */
bool TMP006_enableDataReady(TMP006_Handle handle)
{
    uint16_t reg = 0U; /* Prevent warning */

    if (((TMP006_Object *) (handle->object))->callback != NULL)
    {
        if (TMP006_readRegister(handle, &reg, TMP006_CONFIG))
        {
            reg |= CFG_DATARDY;

            if (TMP006_writeRegister(handle, reg, TMP006_CONFIG))
            {
                GPIO_enableInt(((TMP006_HWAttrs *) (handle->hwAttrs))
                        ->gpioIndex);

                return (true);
            }
        }
    }

    return (false);
}

/*
 *  ======== TMP006_enableConversions ========
 *  Turns on ADC conversion on a specified TMP006 sensor.
 */
bool TMP006_enableConversions(TMP006_Handle handle)
{
    uint16_t reg = 0U;

    if (TMP006_readRegister(handle, &reg, TMP006_CONFIG))
    {
        reg |= CFG_CONV_ON;

        if (TMP006_writeRegister(handle, reg, TMP006_CONFIG))
        {
            return (true);
        }
    }

    return (false);
}

/*
 *  ======== TMP006_getDieTemp ========
 */
bool TMP006_getDieTemp(TMP006_Handle handle, TMP006_TempScale scale,
        float *data)
{
    int16_t temp;

    if (TMP006_readRegister(handle, (uint16_t *) &temp, TMP006_DIE_TEMP))
    {
        /* Shift out first 2 don't care bits, convert to Celsius */
        *data =(double)temp / 128.0;

        switch (scale)
        {
            case TMP006_KELVIN:
                *data += CELSIUS_TO_KELVIN;
                break;

            case TMP006_FAHREN:
                *data = *data * CELSIUS_FAHREN_CONST + CELSIUS_FAHREN_ADD;
                break;

            default:
                break;
        }

        return (true);
    }

    return (false);
}

//****************************************************************************
//
//! \brief Compute the temperature value from the sensor voltage and die temp.
//!
//! \param[in] Vobject     the sensor voltage value
//! \param[in] TDie    the local die temperature
//!
//! \return temperature value
//
//****************************************************************************
static double ComputeTemperature(TMP006_Handle handle,double Vobject,
                                 double Tdie)
{
    /*
    * This algo is obtained from
    * http://processors.wiki.ti.com/index.php/SensorTag_User_Guide
    * http://www.ti.com/lit/ug/sbou107/sbou107.pdf
    * #IR_Temperature_Sensor
    */
    double Tdie2      = Tdie + CELSIUS_TO_KELVIN;
    const double a1   = TMP006_CCONST_A1;
    const double a2   = TMP006_CCONST_A2;
    const double b0   = TMP006_CCONST_B0;
    const double b1   = TMP006_CCONST_B1;
    const double b2   = TMP006_CCONST_B2;
    const double c2   = TMP006_CCONST_C2;
    const double Tref = TMP006_CCONST_TREF;
    double S          = ((TMP006_Object *)(handle->object))->S0 * (1U+a1 *
                        (Tdie2 - Tref)+ a2 * pow((Tdie2 - Tref),2U));
    double Vos        = b0 + b1 * (Tdie2 - Tref) + b2 * pow((Tdie2 - Tref),2U);
    double fObj       = (Vobject - Vos) + c2 * pow((Vobject - Vos),2U);
    double tObj       = pow(pow(Tdie2,4U) + (fObj/S),.25);
    tObj              = (tObj - CELSIUS_TO_KELVIN);

    return tObj;
}


/*
 *  ======== TMP006_getObjTemp ========
 */
bool TMP006_getObjTemp(TMP006_Handle handle, TMP006_TempScale scale,
        float *data)
{
    int16_t temp;
    double objectVoltage,dieTemp;

    if (false == TMP006_readRegister(handle, (uint16_t *) &temp,
                                     TMP006_DIE_TEMP))
    {
        return (false);
    }
    /* Converting the integer temperature result of the TMP006 and TMP006B to
     * physical temperature is done by rightshifting the last two LSBs followed
     * by a divide-by-32 of TDIE to obtain the physical temperature result in
     * degrees Celsius.Which is equivalent to dividing by 128.
     * Please check the tmp006 data sheet for more information.
     */
    dieTemp = ((short)temp) / 128U;

    if (false == TMP006_readRegister(handle, (uint16_t *) &temp, TMP006_VOBJ))
    {
        return (false);
    }
    /* LSB is 156.25 nv*/
    objectVoltage = ((short)temp) * 156.25e-9;

    *data = ComputeTemperature(handle,objectVoltage, dieTemp);

    switch (scale)
    {
        case TMP006_KELVIN:
            *data += CELSIUS_TO_KELVIN;
            break;

        case TMP006_FAHREN:
            *data = *data * CELSIUS_FAHREN_CONST + CELSIUS_FAHREN_ADD;
            break;

        default:
            break;
    }

    return (true);
}

/*
 *  ======== TMP006_getDataReadyStatus ========
 */
bool TMP006_getDataReadyStatus(TMP006_Handle handle,
                                     TMP006_ConversionStatus *conversionStatus)
{
    int16_t data;

    if (false == TMP006_readRegister(handle, (uint16_t *) &data, TMP006_CONFIG))
    {
        return false;
    }
    if(0U == (data & CFG_DATARDY_STATUS))
    {
        *conversionStatus = TMP006_DataNotReady;
    }
    else
    {
        *conversionStatus = TMP006_DataReady;
    }

    return true;
}

/*
 *  ======== TMP006_init ========
 */
void TMP006_init()
{
    static bool initFlag = false;
    unsigned char i;

    if (initFlag == false)
    {
        for (i = 0U; TMP006_config[i].object != NULL; i++)
        {
            ((TMP006_Object *)(TMP006_config[i].object))->i2cHandle = NULL;
        }
        initFlag = true;
    }
}

/*
 *  ======== TMP006_open ========
 * Setups TMP006 sensor and returns TMP006_Handle
 */
TMP006_Handle TMP006_open(unsigned int tmp006Index, I2C_Handle i2cHandle,
        TMP006_Params *params)
{
    TMP006_Handle handle = &TMP006_config[tmp006Index];
    TMP006_Object *obj = (TMP006_Object*)(TMP006_config[tmp006Index].object);
    TMP006_HWAttrs *hw = (TMP006_HWAttrs*)(TMP006_config[tmp006Index].hwAttrs);
    uint16_t data;

    if (obj->i2cHandle != NULL)
    {
        return (NULL);
    }

    obj->i2cHandle = i2cHandle;

    if (params == NULL)
    {
       params = (TMP006_Params *) &TMP006_defaultParams;
    }
    obj->S0 = params->S0;

    /* Perform a software TMP006 Reset */
    if (TMP006_writeRegister(handle, CFG_SW_RST, TMP006_CONFIG))
    {

        usleep(WAIT_POST_RESET * 1000U);

        data = (uint16_t)params->conversions | (uint16_t)params->dataReady
                | CFG_CONV_ON;

        if (TMP006_writeRegister(handle, data, TMP006_CONFIG))
        {

            if (params->callback != NULL)
            {
                obj->callback = params->callback;
                GPIO_setCallback(hw->gpioIndex, obj->callback);
            }

            return (handle);
        }
    }

    obj->i2cHandle = NULL;

    return (NULL);
}

/*
 *  ======== TMP006_Params_init ========
 * Initialize a TMP006_Params struct to default settings.
 */
void TMP006_Params_init(TMP006_Params *params)
{
    *params = TMP006_defaultParams;
}

/*
 *  ======== TMP006_readRegister ========
 *  Reads a specified register from a TMP006 sensor.
 */
bool TMP006_readRegister(TMP006_Handle handle, uint16_t *data,
        uint8_t registerAddress)
{
    I2C_Transaction i2cTransaction;
    unsigned char writeBuffer = registerAddress;
    unsigned char readBuffer[READ_BUF_SIZE];

    i2cTransaction.writeBuf = &writeBuffer;
    i2cTransaction.writeCount = 1U;
    i2cTransaction.readBuf = readBuffer;
    i2cTransaction.readCount = READ_BUF_SIZE;
    i2cTransaction.slaveAddress = ((TMP006_HWAttrs *)(handle->hwAttrs))
            ->slaveAddress;

    if (!I2C_transfer(((TMP006_Object *)(handle->object))->i2cHandle,
            &i2cTransaction))
    {
        return (false);
    }

    *data = (readBuffer[0U] << 8U) | readBuffer[1U];

    return (true);
}

/*
 *  ======== TMP006_writeRegister ========
 *  Writes data to the specified register and TMP006 sensor.
 */
bool TMP006_writeRegister(TMP006_Handle handle, uint16_t data,
        uint8_t registerAddress)
{
    I2C_Transaction i2cTransaction;
    uint8_t writeBuffer[WRITE_BUF_SIZE] = {registerAddress, (data >> 8U), data};

    i2cTransaction.writeBuf = writeBuffer;
    i2cTransaction.writeCount = WRITE_BUF_SIZE;
    i2cTransaction.readCount = 0U;
    i2cTransaction.slaveAddress = ((TMP006_HWAttrs *)(handle->hwAttrs))
            ->slaveAddress;

    /* If transaction success */
    if (!I2C_transfer(((TMP006_Object *)(handle->object))
            ->i2cHandle, &i2cTransaction))
    {
        return (false);
    }

    return (true);
}

