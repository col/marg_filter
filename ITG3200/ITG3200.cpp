/**
 * @author Aaron Berk
 *
 * @section LICENSE
 *
 * Copyright (c) 2010 ARM Limited
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * ITG-3200 triple axis, digital interface, gyroscope.
 *
 * Datasheet:
 *
 * http://invensense.com/mems/gyro/documents/PS-ITG-3200-00-01.4.pdf
 */

/**
 * Includes
 */
#include "ITG3200.h"

ITG3200::ITG3200(PinName sda, PinName scl) : i2c_(sda, scl) {

    //100kHz, mode.
    i2c_.frequency(100000);
    
    //Set FS_SEL to 0x03 for proper operation.
    //See datasheet for details.
    char tx[2];
    tx[0] = DLPF_FS_REG;
    //FS_SEL bits sit in bits 4 and 3 of DLPF_FS register.
    tx[1] = 0x03 << 3;
    
    i2c_.write((ITG3200_I2C_ADDRESS << 1) & 0xFE, tx, 2);

}

char ITG3200::getWhoAmI(void){

    //WhoAmI Register address.
    char tx = WHO_AM_I_REG;
    char rx;
    
    i2c_.write((ITG3200_I2C_ADDRESS << 1) & 0xFE, &tx, 1);
    
    i2c_.read((ITG3200_I2C_ADDRESS << 1) | 0x01, &rx, 1);
    
    return rx;

}

void ITG3200::setWhoAmI(char address){

    char tx[2];
    tx[0] = WHO_AM_I_REG;
    tx[1] = address;
    
    i2c_.write((ITG3200_I2C_ADDRESS << 1) & 0xFE, tx, 2);

}

char ITG3200::getSampleRateDivider(void){

    char tx = SMPLRT_DIV_REG;
    char rx;
    
    i2c_.write((ITG3200_I2C_ADDRESS << 1) & 0xFE, &tx, 1);
    
    i2c_.read((ITG3200_I2C_ADDRESS << 1) | 0x01, &rx, 1);

    return rx;

}

void ITG3200::setSampleRateDivider(char divider){

    char tx[2];
    tx[0] = SMPLRT_DIV_REG;
    tx[1] = divider;

    i2c_.write((ITG3200_I2C_ADDRESS << 1) & 0xFE, tx, 2);

}

int ITG3200::getInternalSampleRate(void){

    char tx = DLPF_FS_REG;
    char rx;
    
    i2c_.write((ITG3200_I2C_ADDRESS << 1) & 0xFE, &tx, 1);
    
    i2c_.read((ITG3200_I2C_ADDRESS << 1) | 0x01, &rx, 1);
    
    //DLPF_CFG == 0 -> sample rate = 8kHz.
    if(rx == 0){
        return 8;
    } 
    //DLPF_CFG = 1..7 -> sample rate = 1kHz.
    else if(rx >= 1 && rx <= 7){
        return 1;
    }
    //DLPF_CFG = anything else -> something's wrong!
    else{
        return -1;
    }
    
}

void ITG3200::setLpBandwidth(char bandwidth){

    char tx[2];
    tx[0] = DLPF_FS_REG;
    //Bits 4,3 are required to be 0x03 for proper operation.
    tx[1] = bandwidth | (0x03 << 3);
    
    i2c_.write((ITG3200_I2C_ADDRESS << 1) & 0xFE, tx, 2);

}

char ITG3200::getInterruptConfiguration(void){

    char tx = INT_CFG_REG;
    char rx;
    
    i2c_.write((ITG3200_I2C_ADDRESS << 1) & 0xFE, &tx, 1);
    
    i2c_.read((ITG3200_I2C_ADDRESS << 1) | 0x01, &rx, 1);
    
    return rx;

}

void ITG3200::setInterruptConfiguration(char config){

    char tx[2];
    tx[0] = INT_CFG_REG;
    tx[1] = config;
    
    i2c_.write((ITG3200_I2C_ADDRESS << 1) & 0xFE, tx, 2);

}

bool ITG3200::isPllReady(void){

    char tx = INT_STATUS;
    char rx;
    
    i2c_.write((ITG3200_I2C_ADDRESS << 1) & 0xFE, &tx, 1);
    
    i2c_.read((ITG3200_I2C_ADDRESS << 1) | 0x01, &rx, 1);
    
    //ITG_RDY bit is bit 4 of INT_STATUS register.
    if(rx & 0x04){
        return true;
    }
    else{
        return false;
    }
    
}

bool ITG3200::isRawDataReady(void){

    char tx = INT_STATUS;
    char rx;
    
    i2c_.write((ITG3200_I2C_ADDRESS << 1) & 0xFE, &tx, 1);
    
    i2c_.read((ITG3200_I2C_ADDRESS << 1) | 0x01, &rx, 1);
    
    //RAW_DATA_RDY bit is bit 1 of INT_STATUS register.
    if(rx & 0x01){
        return true;
    }
    else{
        return false;
    }
    
}

float ITG3200::getTemperature(void){

    char tx = TEMP_OUT_H_REG;
    char rx[2];
    
    i2c_.write((ITG3200_I2C_ADDRESS << 1) & 0xFE, &tx, 1);
    
    i2c_.read((ITG3200_I2C_ADDRESS << 1) | 0x01, rx, 2);
    
    int16_t temperature = ((int) rx[0] << 8) | ((int) rx[1]);
    //Offset = -35 degrees, 13200 counts. 280 counts/degrees C.
    return 35.0 + ((temperature + 13200)/280.0);
    
}

int ITG3200::getGyroX(void){

    char tx = GYRO_XOUT_H_REG;
    char rx[2];
    
    i2c_.write((ITG3200_I2C_ADDRESS << 1) & 0xFE, &tx, 1);
    
    i2c_.read((ITG3200_I2C_ADDRESS << 1) | 0x01, rx, 2);
    
    int16_t output = ((int) rx[0] << 8) | ((int) rx[1]);

    return output;

}

int ITG3200::getGyroY(void){

    char tx = GYRO_YOUT_H_REG;
    char rx[2];
    
    i2c_.write((ITG3200_I2C_ADDRESS << 1) & 0xFE, &tx, 1);
    
    i2c_.read((ITG3200_I2C_ADDRESS << 1) | 0x01, rx, 2);
    
    int16_t output = ((int) rx[0] << 8) | ((int) rx[1]);

    return output;

}

int ITG3200::getGyroZ(void){

    char tx = GYRO_ZOUT_H_REG;
    char rx[2];
    
    i2c_.write((ITG3200_I2C_ADDRESS << 1) & 0xFE, &tx, 1);
    
    i2c_.read((ITG3200_I2C_ADDRESS << 1) | 0x01, rx, 2);
    
    int16_t output = ((int) rx[0] << 8) | ((int) rx[1]);

    return output;
    
}

char ITG3200::getPowerManagement(void){

    char tx = PWR_MGM_REG;
    char rx;
    
    i2c_.write((ITG3200_I2C_ADDRESS << 1) & 0xFE, &tx, 1);
    
    i2c_.read((ITG3200_I2C_ADDRESS << 1) | 0x01, &rx, 1);
    
    return rx;

}

void ITG3200::setPowerManagement(char config){

    char tx[2];
    tx[0] = PWR_MGM_REG;
    tx[1] = config;

    i2c_.write((ITG3200_I2C_ADDRESS << 1) & 0xFE, tx, 2);

}
