/**
 * MARG filter example.
 *
 * Calculate the roll, pitch and yaw angles.
 */
#include "MARGfilter.h"
#include "ADXL345.h"
#include "ITG3200.h"
#include "HMC5843.h"

//Gravity at Earth's surface in m/s/s
#define g0 9.812865328
//Number of samples to average.
#define SAMPLES 4
//Number of samples to be averaged for a null bias calculation
//during calibration.
#define CALIBRATION_SAMPLES 128
//Convert from radians to degrees.
#define toDegrees(x) (x * 57.2957795)
//Convert from degrees to radians.
#define toRadians(x) (x * 0.01745329252)
//ITG-3200 sensitivity is 14.375 LSB/(degrees/sec).
#define GYROSCOPE_GAIN (1 / 14.375)
//Full scale resolution on the ADXL345 is 4mg/LSB.
#define ACCELEROMETER_GAIN (0.004 * g0)
//Note: Not sure what the gain for the magnetometer should be. :(
#define MAGNETOMETER_GAIN 1.0
//Sampling gyroscope at 200Hz.
#define GYRO_RATE   0.005
//Sampling accelerometer at 200Hz.
#define ACC_RATE    0.005
//Sampling magnetometer at 10Hz.
#define MAG_RATE    0.1
//Updating filter at 40Hz.
#define FILTER_RATE 0.1

Serial pc(USBTX, USBRX);
//At rest the gyroscope is centred around 0 and goes between about
//-5 and 5 counts. As 1 degrees/sec is ~15 LSB, error is roughly
//5/15 = 0.3 degrees/sec.
MARGfilter margFilter(FILTER_RATE, 0.3, 0.0 /* What's the gyro drift?*/);
// p28 = sda (data pin), p27 = scl (clock pin)
ADXL345 accelerometer(p28, p27);
ITG3200 gyroscope(p28, p27);
HMC5843 magnetometer(p28, p27);
Ticker accelerometerTicker;
Ticker gyroscopeTicker;
Ticker magnetometerTicker;
Ticker filterTicker;

//Offsets for the gyroscope.
//The readings we take when the gyroscope is stationary won't be 0, so we'll
//average a set of readings we do get when the gyroscope is stationary and
//take those away from subsequent readings to ensure the gyroscope is offset
//or "biased" to 0.
double w_xBias;
double w_yBias;
double w_zBias;

//Offsets for the accelerometer.
//Same as with the gyroscope.
double a_xBias;
double a_yBias;
double a_zBias;

//Offsets for the magnetometer.
//Same as with the gyroscope.
double m_xBias;
double m_yBias;
double m_zBias;

//Accumulators used for oversampling and then averaging.
volatile double a_xAccumulator = 0;
volatile double a_yAccumulator = 0;
volatile double a_zAccumulator = 0;

volatile double w_xAccumulator = 0;
volatile double w_yAccumulator = 0;
volatile double w_zAccumulator = 0;

volatile double m_xAccumulator = 0;
volatile double m_yAccumulator = 0;
volatile double m_zAccumulator = 0;

//Accelerometer, gyroscope and magnetometer readings for x, y, z axes.
volatile double a_x;
volatile double a_y;
volatile double a_z;
volatile double w_x;
volatile double w_y;
volatile double w_z;
volatile double m_x;
volatile double m_y;
volatile double m_z;

//Buffer for accelerometer readings.
int readings[3];
//Number of accelerometer samples we're on.
int accelerometerSamples = 0;
//Number of gyroscope samples we're on.
int gyroscopeSamples = 0;
//Number of magnetometer samples we're on.
int magnetometerSamples = 0;

/**
 * Prototypes
 */
//Set up the ADXL345 appropriately.
void initializeAcceleromter(void);
//Calculate the null bias.
void calibrateAccelerometer(void);
//Take a set of samples and average them.
void sampleAccelerometer(void);

//Set up the ITG3200 appropriately.
void initializeGyroscope(void);
//Calculate the null bias.
void calibrateGyroscope(void);
//Take a set of samples and average them.
void sampleGyroscope(void);

//Set up the HMC5843 appropriately.
void initializeMagnetometer(void);
//Calculate the null bias.
void calibrateMagnetometer(void);
//Take a set of samples and average them.
void sampleMagnetometer(void);

//Update the filter and calculate the Euler angles.
void filter(void);

void initializeAccelerometer(void) {

    //Go into standby mode to configure the device.
    accelerometer.setPowerControl(0x00);
    //Full resolution, +/-16g, 4mg/LSB.
    accelerometer.setDataFormatControl(0x0B);
    //200Hz data rate.
    accelerometer.setDataRate(ADXL345_200HZ);
    //Measurement mode.
    accelerometer.setPowerControl(0x08);
    //See http://www.analog.com/static/imported-files/application_notes/AN-1077.pdf
    wait_ms(22);

}

void sampleAccelerometer(void) {

    //Have we taken enough samples?
    if (accelerometerSamples == SAMPLES) {

        //Average the samples, remove the bias, and calculate the acceleration
        //in m/s/s.
        a_x = ((a_xAccumulator / SAMPLES) - a_xBias) * ACCELEROMETER_GAIN;
        a_y = ((a_yAccumulator / SAMPLES) - a_yBias) * ACCELEROMETER_GAIN;
        a_z = ((a_zAccumulator / SAMPLES) - a_zBias) * ACCELEROMETER_GAIN;

        a_xAccumulator = 0;
        a_yAccumulator = 0;
        a_zAccumulator = 0;
        accelerometerSamples = 0;

    } else {
        //Take another sample.
        accelerometer.getOutput(readings);

        a_xAccumulator += (int16_t) readings[0];
        a_yAccumulator += (int16_t) readings[1];
        a_zAccumulator += (int16_t) readings[2];

        accelerometerSamples++;
    }

}

void calibrateAccelerometer(void) {

    a_xAccumulator = 0;
    a_yAccumulator = 0;
    a_zAccumulator = 0;

    //Take a number of readings and average them
    //to calculate the zero g offset.
    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {

        accelerometer.getOutput(readings);

        a_xAccumulator += (int16_t) readings[0];
        a_yAccumulator += (int16_t) readings[1];
        a_zAccumulator += (int16_t) readings[2];

        wait(ACC_RATE);

    }

    a_xAccumulator /= CALIBRATION_SAMPLES;
    a_yAccumulator /= CALIBRATION_SAMPLES;
    a_zAccumulator /= CALIBRATION_SAMPLES;

    //At 4mg/LSB, 250 LSBs is 1g.
    a_xBias = a_xAccumulator;
    a_yBias = a_yAccumulator;
    a_zBias = (a_zAccumulator - 250);

    a_xAccumulator = 0;
    a_yAccumulator = 0;
    a_zAccumulator = 0;

}

void initializeGyroscope(void) {

    //Low pass filter bandwidth of 42Hz.
    gyroscope.setLpBandwidth(LPFBW_42HZ);
    //Internal sample rate of 200Hz. (1kHz / 5).
    gyroscope.setSampleRateDivider(4);

}

void calibrateGyroscope(void) {

    w_xAccumulator = 0;
    w_yAccumulator = 0;
    w_zAccumulator = 0;

    //Take a number of readings and average them
    //to calculate the gyroscope bias offset.
    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {

        w_xAccumulator += gyroscope.getGyroX();
        w_yAccumulator += gyroscope.getGyroY();
        w_zAccumulator += gyroscope.getGyroZ();
        wait(GYRO_RATE);

    }

    //Average the samples.
    w_xAccumulator /= CALIBRATION_SAMPLES;
    w_yAccumulator /= CALIBRATION_SAMPLES;
    w_zAccumulator /= CALIBRATION_SAMPLES;

    w_xBias = w_xAccumulator;
    w_yBias = w_yAccumulator;
    w_zBias = w_zAccumulator;

    w_xAccumulator = 0;
    w_yAccumulator = 0;
    w_zAccumulator = 0;

}

void sampleGyroscope(void) {

    //Have we taken enough samples?
    if (gyroscopeSamples == SAMPLES) {

        //Average the samples, remove the bias, and calculate the angular
        //velocity in rad/s.
        w_x = toRadians(((w_xAccumulator / SAMPLES) - w_xBias) * GYROSCOPE_GAIN);
        w_y = toRadians(((w_yAccumulator / SAMPLES) - w_yBias) * GYROSCOPE_GAIN);
        w_z = toRadians(((w_zAccumulator / SAMPLES) - w_zBias) * GYROSCOPE_GAIN);

        w_xAccumulator = 0;
        w_yAccumulator = 0;
        w_zAccumulator = 0;
        gyroscopeSamples = 0;

    } else {
        //Take another sample.
        w_xAccumulator += gyroscope.getGyroX();
        w_yAccumulator += gyroscope.getGyroY();
        w_zAccumulator += gyroscope.getGyroZ();

        gyroscopeSamples++;

    }

}

void initializeMagnetometer(void) {
  // Continuous mode, 10Hz measurement rate, 1.0 Gain
  magnetometer.setDefault();
  // Wait at least 5ms
  wait_ms(10);
}

void calibrateMagnetometer(void) {
  m_xAccumulator = 0;
  m_yAccumulator = 0;
  m_zAccumulator = 0;

  //Take a number of readings and average them
  //to calculate the gyroscope bias offset.
  for (int i = 0; i < 20/*2 seconds at 10Hz*/; i++) {

      magnetometer.readData(readings);
      m_xAccumulator += (int16_t) readings[0];
      m_yAccumulator += (int16_t) readings[1];
      m_zAccumulator += (int16_t) readings[2];
      wait(MAG_RATE);

  }

  //Average the samples.
  m_xAccumulator /= CALIBRATION_SAMPLES;
  m_yAccumulator /= CALIBRATION_SAMPLES;
  m_zAccumulator /= CALIBRATION_SAMPLES;

  m_xBias = m_xAccumulator;
  m_yBias = m_yAccumulator;
  m_zBias = m_zAccumulator;

  m_xAccumulator = 0;
  m_yAccumulator = 0;
  m_zAccumulator = 0;
}

void sampleMagnetometer(void) {
  //Have we taken enough samples?
  if (magnetometerSamples == SAMPLES) {

      //Average the samples, remove the bias, and calculate the values
      //in m/s/s.
      m_x = ((m_xAccumulator / SAMPLES) - m_xBias) * MAGNETOMETER_GAIN;
      m_y = ((m_yAccumulator / SAMPLES) - m_yBias) * MAGNETOMETER_GAIN;
      m_z = ((m_zAccumulator / SAMPLES) - m_zBias) * MAGNETOMETER_GAIN;

      m_xAccumulator = 0;
      m_yAccumulator = 0;
      m_zAccumulator = 0;
      magnetometerSamples = 0;

  } else {
      //Take another sample.
      magnetometer.readData(readings);
      m_xAccumulator += (int16_t) readings[0];
      m_yAccumulator += (int16_t) readings[1];
      m_zAccumulator += (int16_t) readings[2];

      magnetometerSamples++;
  }
}

void filter(void) {

    //Update the filter variables.
    margFilter.updateFilter(w_y, w_x, w_z, a_y, a_x, a_z, m_y, m_x, m_z);
    //Calculate the new Euler angles.
    margFilter.computeEuler();
}

typedef union _data {
  float f;
  char  s[4];
} floatBytes;

floatBytes roll;
floatBytes pitch;
floatBytes yaw;

int main() {

    pc.printf("Starting MARG filter test...\n");

    //Initialize inertial sensors.
    initializeAccelerometer();
    calibrateAccelerometer();

    initializeGyroscope();
    calibrateGyroscope();

    initializeMagnetometer();
    calibrateMagnetometer();

    //Set up timers.
    //Accelerometer data rate is 200Hz, so we'll sample at this speed.
    accelerometerTicker.attach(&sampleAccelerometer, ACC_RATE);
    //Gyroscope data rate is 200Hz, so we'll sample at this speed.
    gyroscopeTicker.attach(&sampleGyroscope, GYRO_RATE);
    //Magnetometer data rate is 10Hz, so we'll sample at this speed.
    magnetometerTicker.attach(&sampleMagnetometer, MAG_RATE);
    //Update the filter variables at the correct rate.
    filterTicker.attach(&filter, FILTER_RATE);

    while (1) {

        wait(FILTER_RATE);

        roll.f = (float)toDegrees(margFilter.getRoll());
        pitch.f = (float)toDegrees(margFilter.getPitch());
        yaw.f = (float)toDegrees(margFilter.getYaw());

        pc.printf("R%c%c%c%c\t%c%c%c%c\t%c%c%c%c\n",
          roll.s[0],  roll.s[1],  roll.s[2],  roll.s[3],
          pitch.s[0], pitch.s[1], pitch.s[2], pitch.s[3],
          yaw.s[0],   yaw.s[1],   yaw.s[2],   yaw.s[3]
        );

    }

}
