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
 *  ======== tmp007.c ========
 */
#include <stdint.h>
#include <stdbool.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>

/* Module Header */
#include <ti/sail/tmp007/tmp007.h>

/* POSIX Header files */
#include <unistd.h>

/* It is recommended to wait for some time(in few ms, defined by below macro)
before TMP007 module is used after issuing a reset*/
#define WAIT_POST_RESET      5        /* Wait time in ms after reset        */

#define MAX_CELSIUS          256      /* Max degrees celsius                */
#define MIN_CELSIUS          -256     /* Min degrees celsius                */
#define CELSIUS_FAHREN_CONST 1.8F     /* Multiplier for conversion          */
#define CELSIUS_FAHREN_ADD   32       /* Celsius to Fahrenheit added factor */
#define CELSIUS_TO_KELVIN    273.15F  /* Celsius to Kelvin addition factor  */
#define CELSIUS_PER_LSB      0.03125F /* Degrees celsius per LSB            */

#define READ_BUF_SIZE        2
#define WRITE_BUF_SIZE       3

/* TMP007 Configuration Register Bits */
#define CFG_SW_RST    0x8000  /* Software reset                      */
#define CFG_CONV_ON   0x1000  /* Conversion on mode                  */
#define CFG_ALRT      0x0100  /* Alert pin enable                    */

/* Default TMP007 parameters structure */
const TMP007_Params TMP007_defaultParams = {
        TMP007_4CONV,     /* 4 ADC Conversions Averaged per Sample */
        TMP007_IntMode,   /* Interrupt Mode                        */
        TMP007_TCEnable,  /* Transient Correct Enable              */
        NULL,             /* Callback Function                     */
};

extern TMP007_Config TMP007_config[];

/*
 *  ======== TMP007_close ========
 *  Closes an instance of a TMP007 sensor.
 */
bool TMP007_close(TMP007_Handle handle)
{
    TMP007_Object *obj = (TMP007_Object *)(handle->object);

    if (obj->callback != NULL) {
        TMP007_disableAlert(handle);
    }

    if (TMP007_disableConversions(handle)) {
        obj->i2cHandle = NULL;

        return (true);
    }

    return (false);
}

/*
 *  ======== TMP007_disableAlert ========
 *  Disables alerts from a TMP007 sensor.
 */
bool TMP007_disableAlert(TMP007_Handle handle)
{
    uint16_t reg = 0;

    if (((TMP007_Object *)(handle->object))->callback == NULL) {
        return (true);
    }

    if (TMP007_readRegister(handle, &reg, TMP007_CONFIG)) {
        reg &= ~CFG_ALRT;

        if (TMP007_writeRegister(handle, reg, TMP007_CONFIG)) {
            GPIO_disableInt(((TMP007_HWAttrs *) (handle->hwAttrs))->gpioIndex);

            return (true);
        }
    }

    return (false);
}

/*
 *  ======== TMP007_disableConversions ========
 *  Turns ADC conversion off on a specified TMP007 sensor.
 */
bool TMP007_disableConversions(TMP007_Handle handle)
{
    uint16_t reg = 0; /* Prevent warning */

    if (TMP007_readRegister(handle, &reg, TMP007_CONFIG)) {
        reg &= ~CFG_CONV_ON;

        if (TMP007_writeRegister(handle, reg, TMP007_CONFIG)) {
            return (true);
        }
    }

    return (false);
}

/*
 *  ======== TMP007_enableAlert ========
 *  Enables alerts from a TMP007 sensor.
 */
bool TMP007_enableAlert(TMP007_Handle handle)
{
    uint16_t reg = 0; /* Prevent warning */

    if (((TMP007_Object *) (handle->object))->callback != NULL) {
        if (TMP007_readRegister(handle, &reg, TMP007_CONFIG)) {
            reg |= CFG_ALRT;

            if (TMP007_writeRegister(handle, reg, TMP007_CONFIG)) {
                GPIO_enableInt(((TMP007_HWAttrs *) (handle->hwAttrs))
                        ->gpioIndex);

                return (true);
            }
        }
    }

    return (false);
}

/*
 *  ======== TMP007_enableConversions ========
 *  Turns on ADC conversion on a specified TMP007 sensor.
 */
bool TMP007_enableConversions(TMP007_Handle handle)
{
    uint16_t reg = 0;

    if (TMP007_readRegister(handle, &reg, TMP007_CONFIG)) {
        reg |= CFG_CONV_ON;

        if (TMP007_writeRegister(handle, reg, TMP007_CONFIG)) {
            return (true);
        }
    }

    return (false);
}

/*
 *  ======== TMP007_getTemp ========
 */
bool TMP007_getTemp(TMP007_Handle handle, TMP007_TempScale scale,
        float *data, uint16_t registerAddress)
{
    int16_t temp;

    if (TMP007_readRegister(handle, (uint16_t *) &temp, registerAddress)) {
        /* Shift out first 2 don't care bits, convert to Celsius */
        *data = ((float)(temp >> 2)) * CELSIUS_PER_LSB;

        switch (scale) {
            case TMP007_KELVIN:
                *data += CELSIUS_TO_KELVIN;
                break;

            case TMP007_FAHREN:
                *data = *data * CELSIUS_FAHREN_CONST + CELSIUS_FAHREN_ADD;
                break;

            default:
                break;
        }

        return (true);
    }

    return (false);
}

/*
 *  ======== TMP007_getTempLimit ========
 */
bool TMP007_getTempLimit(TMP007_Handle handle, TMP007_TempScale scale,
        float *data, uint16_t registerAddress)
{
    int16_t temp;

    /* Read Temperature Limit */
    if (!TMP007_readRegister(handle, (uint16_t *) &temp, registerAddress)) {
        return (false);
    }

    /* Right 6 registers are don't care bits*/
    *data = ((float)(temp >> 6)) / 2;

    switch (scale) {
        case TMP007_KELVIN:
            *data = *data - CELSIUS_TO_KELVIN;
            break;

        case TMP007_FAHREN:
            *data = (*data  * CELSIUS_FAHREN_CONST) + CELSIUS_FAHREN_ADD;
            break;

        default:
            break;
    }

    return (true);
}

/*
 *  ======== TMP007_init ========
 */
void TMP007_init()
{
    static bool initFlag = false;
    unsigned char i;

    if (initFlag == false) {
        for (i = 0; TMP007_config[i].object != NULL; i++) {
            ((TMP007_Object *)(TMP007_config[i].object))->i2cHandle = NULL;
        }
        initFlag = true;
    }
}

/*
 *  ======== TMP007_open ========
 * Setups TMP007 sensor and returns TMP007_Handle
 */
TMP007_Handle TMP007_open(unsigned int tmp007Index, I2C_Handle i2cHandle,
        TMP007_Params *params)
{
    TMP007_Handle handle = &TMP007_config[tmp007Index];
    TMP007_Object *obj = (TMP007_Object*)(TMP007_config[tmp007Index].object);
    TMP007_HWAttrs *hw = (TMP007_HWAttrs*)(TMP007_config[tmp007Index].hwAttrs);
    uint16_t data;

    if (obj->i2cHandle != NULL) {
        return (NULL);
    }

    obj->i2cHandle = i2cHandle;

    if (params == NULL) {
       params = (TMP007_Params *) &TMP007_defaultParams;
    }

    /* Perform a Hardware TMP007 Reset */
    if (TMP007_writeRegister(handle, CFG_SW_RST, TMP007_CONFIG)) {

        usleep(WAIT_POST_RESET * 1000);

        data = (uint16_t)params->conversions | (uint16_t)params->intComp | (uint16_t)params->transientCorrect
                | CFG_CONV_ON;

        if (TMP007_writeRegister(handle, data, TMP007_CONFIG)) {

            if (params->callback != NULL) {
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
 *  ======== TMP007_Params_init ========
 * Initialize a TMP007_Params struct to default settings.
 */
void TMP007_Params_init(TMP007_Params *params)
{
    *params = TMP007_defaultParams;
}

/*
 *  ======== TMP007_readRegister ========
 *  Reads a specified register from a TMP007 sensor.
 */
bool TMP007_readRegister(TMP007_Handle handle, uint16_t *data,
        uint8_t registerAddress)
{
    I2C_Transaction i2cTransaction;
    unsigned char writeBuffer = registerAddress;
    unsigned char readBuffer[READ_BUF_SIZE];

    i2cTransaction.writeBuf = &writeBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = readBuffer;
    i2cTransaction.readCount = READ_BUF_SIZE;
    i2cTransaction.slaveAddress = ((TMP007_HWAttrs *)(handle->hwAttrs))
            ->slaveAddress;

    if (!I2C_transfer(((TMP007_Object *)(handle->object))->i2cHandle,
            &i2cTransaction)) {
        return (false);
    }

    *data = (readBuffer[0] << 8) | readBuffer[1];

    return (true);
}

/*
 *  ======== TMP007_setTempLimit ========
 */
bool TMP007_setTempLimit(TMP007_Handle handle, TMP007_TempScale scale,
        float data, uint16_t bitMask, uint8_t registerAddress)
{
    uint16_t reg;
    int16_t temp;

    /* Read Status Mask and Enable Register */
    if (!TMP007_readRegister(handle, &reg, TMP007_MASK_ENABLE)) {
        return (false);
    }

    if (data == TMP007_IGNORE) {
        /* Write Lim Flag Disable */
        reg &= ~bitMask;

        if (TMP007_writeRegister(handle, reg, TMP007_MASK_ENABLE)) {
            return (true);
        }

        return (false);
    }

    /* Convert from scale to Celsius */
    switch (scale) {
        case TMP007_KELVIN:
            data = data - CELSIUS_TO_KELVIN;
            break;

        case TMP007_FAHREN:
            data = (data - CELSIUS_FAHREN_ADD) / CELSIUS_FAHREN_CONST;
            break;
            
        default:
            break;
    }

    if (data > MAX_CELSIUS/2) {
        data = MAX_CELSIUS/2;
    }

    if (data < MIN_CELSIUS/2) {
        data = MIN_CELSIUS/2;
    }


    temp = (int16_t)(data * 2); /* 0.5 (C) per LSB and truncate to integer */
    temp = temp << 6; /* Shift for TMP007 data register alignment */

    /* Write Temp Lim */
    if (!TMP007_writeRegister(handle, temp, registerAddress)) {
        return (false);
    }

    /* Write Local Lim Flag Enable */
    reg |= bitMask;

    if (!TMP007_writeRegister(handle, reg, TMP007_MASK_ENABLE)) {
        return (false);
    }

    return (true);
}

/*
 *  ======== TMP007_writeRegister ========
 *  Writes data to the specified register and TMP007 sensor.
 */
bool TMP007_writeRegister(TMP007_Handle handle, uint16_t data,
        uint8_t registerAddress)
{
    I2C_Transaction i2cTransaction;
    uint8_t writeBuffer[WRITE_BUF_SIZE] = {registerAddress, (data >> 8), data};

    i2cTransaction.writeBuf = writeBuffer;
    i2cTransaction.writeCount = WRITE_BUF_SIZE;
    i2cTransaction.readCount = 0;
    i2cTransaction.slaveAddress = ((TMP007_HWAttrs *)(handle->hwAttrs))
            ->slaveAddress;

    /* If transaction success */
    if (!I2C_transfer(((TMP007_Object *)(handle->object))
            ->i2cHandle, &i2cTransaction)) {
        return (false);
    }

    return (true);
}
