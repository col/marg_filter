/**
 * @author Uwe Gartmann
 * @author Used ITG3200 library developed Aaron Berk as template
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

#ifndef ITG3200_H
#define ITG3200_H

/**
 * Includes
 */
#include "mbed.h"

/**
 * Defines
 */
#define ITG3200_I2C_ADDRESS 0x68 //7-bit address.

//-----------
// Registers
//-----------
#define WHO_AM_I_REG    0x00
#define SMPLRT_DIV_REG  0x15
#define DLPF_FS_REG     0x16
#define INT_CFG_REG     0x17
#define INT_STATUS      0x1A
#define TEMP_OUT_H_REG  0x1B
#define TEMP_OUT_L_REG  0x1C
#define GYRO_XOUT_H_REG 0x1D
#define GYRO_XOUT_L_REG 0x1E
#define GYRO_YOUT_H_REG 0x1F
#define GYRO_YOUT_L_REG 0x20
#define GYRO_ZOUT_H_REG 0x21
#define GYRO_ZOUT_L_REG 0x22
#define PWR_MGM_REG     0x3E

//----------------------------
// Low Pass Filter Bandwidths
//----------------------------
#define LPFBW_256HZ 0x00
#define LPFBW_188HZ 0x01
#define LPFBW_98HZ  0x02
#define LPFBW_42HZ  0x03
#define LPFBW_20HZ  0x04
#define LPFBW_10HZ  0x05
#define LPFBW_5HZ   0x06

/**
 * ITG-3200 triple axis digital gyroscope.
 */
class ITG3200 {

public:

    /**
     * Constructor.
     *
     * Sets FS_SEL to 0x03 for proper opertaion.
     *
     * @param sda - mbed pin to use for the SDA I2C line.
     * @param scl - mbed pin to use for the SCL I2C line.
     */
    ITG3200(PinName sda, PinName scl);

    /**
     * Get the identity of the device.
     *
     * @return The contents of the Who Am I register which contains the I2C
     *         address of the device.
     */
    char getWhoAmI(void);

    /**
     * Set the address of the device.
     *
     * @param address The I2C slave address to write to the Who Am I register
     *        on the device.
     */
    void setWhoAmI(char address);

    /**
     * Get the sample rate divider.
     *
     * @return The sample rate divider as a number from 0-255.
     */
    char getSampleRateDivider(void);

    /**
     * Set the sample rate divider.
     *
     * Fsample = Finternal / (divider + 1), where Finternal = 1kHz or 8kHz,
     * as decidied by the DLPF_FS register.
     *
     * @param The sample rate divider as a number from 0-255.
     */
    void setSampleRateDivider(char divider);

    /**
     * Get the internal sample rate.
     *
     * @return The internal sample rate in kHz - either 1 or 8.
     */
    int getInternalSampleRate(void);

    /**
     * Set the low pass filter bandwidth.
     *
     * Also used to set the internal sample rate.
     * Pass the #define bandwidth codes as a parameter.
     *
     * 256Hz -> 8kHz internal sample rate.
     * Everything else -> 1kHz internal rate.
     *
     * @param bandwidth Low pass filter bandwidth code
     */
    void setLpBandwidth(char bandwidth);

    /**
     * Get the interrupt configuration.
     *
     * See datasheet for register contents details.
     *
     *    7      6           5                 4
     * +------+------+--------------+------------------+
     * | ACTL | OPEN | LATCH_INT_EN | INT_ANYRD_2CLEAR |
     * +------+------+--------------+------------------+
     *
     *   3        2            1       0
     * +---+------------+------------+---+
     * | 0 | ITG_RDY_EN | RAW_RDY_EN | 0 |
     * +---+------------+------------+---+
     *
     * ACTL Logic level for INT output pin; 1 = active low, 0 = active high.
     * OPEN Drive type for INT output pin; 1 = open drain, 0 = push-pull.
     * LATCH_INT_EN Latch mode; 1 = latch until interrupt is cleared,
     *                          0 = 50us pulse.
     * INT_ANYRD_2CLEAR Latch clear method; 1 = any register read,
     *                                      0 = status register read only.
     * ITG_RDY_EN Enable interrupt when device is ready,
     *            (PLL ready after changing clock source).
     * RAW_RDY_EN Enable interrupt when data is available.
     * 0 Bits 1 and 3 of the INT_CFG register should be zero.
     *
     * @return the contents of the INT_CFG register.
     */
    char getInterruptConfiguration(void);

    /**
     * Set the interrupt configuration.
     *
     * See datasheet for configuration byte details.
     *
     *    7      6           5                 4
     * +------+------+--------------+------------------+
     * | ACTL | OPEN | LATCH_INT_EN | INT_ANYRD_2CLEAR |
     * +------+------+--------------+------------------+
     *
     *   3        2            1       0
     * +---+------------+------------+---+
     * | 0 | ITG_RDY_EN | RAW_RDY_EN | 0 |
     * +---+------------+------------+---+
     *
     * ACTL Logic level for INT output pin; 1 = active low, 0 = active high.
     * OPEN Drive type for INT output pin; 1 = open drain, 0 = push-pull.
     * LATCH_INT_EN Latch mode; 1 = latch until interrupt is cleared,
     *                          0 = 50us pulse.
     * INT_ANYRD_2CLEAR Latch clear method; 1 = any register read,
     *                                      0 = status register read only.
     * ITG_RDY_EN Enable interrupt when device is ready,
     *            (PLL ready after changing clock source).
     * RAW_RDY_EN Enable interrupt when data is available.
     * 0 Bits 1 and 3 of the INT_CFG register should be zero.
     *
     * @param config Configuration byte to write to INT_CFG register.
     */
    void setInterruptConfiguration(char config);

    /**
     * Check the ITG_RDY bit of the INT_STATUS register.
     *
     * @return True if the ITG_RDY bit is set, corresponding to PLL ready,
     *         false if the ITG_RDY bit is not set, corresponding to PLL not
     *         ready.
     */
    bool isPllReady(void);

    /**
     * Check the RAW_DATA_RDY bit of the INT_STATUS register.
     *
     * @return True if the RAW_DATA_RDY bit is set, corresponding to new data
     *         in the sensor registers, false if the RAW_DATA_RDY bit is not
     *         set, corresponding to no new data yet in the sensor registers.
     */
    bool isRawDataReady(void);

    /**
     * Get the temperature of the device.
     *
     * @return The temperature in degrees celsius.
     */
    float getTemperature(void);

    /**
     * Get the output for the x-axis gyroscope.
     *
     * Typical sensitivity is 14.375 LSB/(degrees/sec).
     *
     * @return The output on the x-axis in raw ADC counts.
     */
    int getGyroX(void);

    /**
     * Get the output for the y-axis gyroscope.
     *
     * Typical sensitivity is 14.375 LSB/(degrees/sec).
     *
     * @return The output on the y-axis in raw ADC counts.
     */
    int getGyroY(void);

    /**
     * Get the output on the z-axis gyroscope.
     *
     * Typical sensitivity is 14.375 LSB/(degrees/sec).
     * 
     * @return The output on the z-axis in raw ADC counts.
     */
    int getGyroZ(void);

    /**
     * Get the power management configuration.
     *
     * See the datasheet for register contents details.
     *
     *     7        6        5         4
     * +---------+-------+---------+---------+
     * | H_RESET | SLEEP | STBY_XG | STBY_YG |
     * +---------+-------+---------+---------+
     *
     *      3          2         1          0
     * +---------+----------+----------+----------+
     * | STBY_ZG | CLK_SEL2 | CLK_SEL1 | CLK_SEL0 |
     * +---------+----------+----------+----------+
     *
     * H_RESET Reset device and internal registers to the power-up-default settings.
     * SLEEP Enable low power sleep mode.
     * STBY_XG Put gyro X in standby mode (1=standby, 0=normal).
     * STBY_YG Put gyro Y in standby mode (1=standby, 0=normal).
     * STBY_ZG Put gyro Z in standby mode (1=standby, 0=normal).
     * CLK_SEL Select device clock source:
     *
     * CLK_SEL | Clock Source
     * --------+--------------
     *    0      Internal oscillator
     *    1      PLL with X Gyro reference
     *    2      PLL with Y Gyro reference
     *    3      PLL with Z Gyro reference
     *    4      PLL with external 32.768kHz reference
     *    5      PLL with external 19.2MHz reference
     *    6      Reserved
     *    7      Reserved
     *
     * @return The contents of the PWR_MGM register.
     */
    char getPowerManagement(void);

    /**
     * Set power management configuration.
     *
     * See the datasheet for configuration byte details
     *
     *      7        6        5         4
     * +---------+-------+---------+---------+
     * | H_RESET | SLEEP | STBY_XG | STBY_YG |
     * +---------+-------+---------+---------+
     *
     *      3          2         1          0
     * +---------+----------+----------+----------+
     * | STBY_ZG | CLK_SEL2 | CLK_SEL1 | CLK_SEL0 |
     * +---------+----------+----------+----------+
     *
     * H_RESET Reset device and internal registers to the power-up-default settings.
     * SLEEP Enable low power sleep mode.
     * STBY_XG Put gyro X in standby mode (1=standby, 0=normal).
     * STBY_YG Put gyro Y in standby mode (1=standby, 0=normal).
     * STBY_ZG Put gyro Z in standby mode (1=standby, 0=normal).
     * CLK_SEL Select device clock source:
     *
     * CLK_SEL | Clock Source
     * --------+--------------
     *    0      Internal oscillator
     *    1      PLL with X Gyro reference
     *    2      PLL with Y Gyro reference
     *    3      PLL with Z Gyro reference
     *    4      PLL with external 32.768kHz reference
     *    5      PLL with external 19.2MHz reference
     *    6      Reserved
     *    7      Reserved
     *
     * @param config The configuration byte to write to the PWR_MGM register.
     */
    void setPowerManagement(char config);

private:

    I2C i2c_;

};

#endif /* ITG3200_H */
