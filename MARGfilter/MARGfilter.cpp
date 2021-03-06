/**
 * @author David X. Elrom
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
 * Adapted from IMU filter library
 * IMU orientation filter developed by Sebastian Madgwick.
 *
 * Find more details about his paper here:
 *
 * http://code.google.com/p/imumargalgorithm30042010sohm/
 */

/**
 * Includes
 */
#include "MARGfilter.h"

MARGfilter::MARGfilter(double rate, double gyroscopeMeasurementError, double gyroscopeMeasurementDrift){

    firstUpdate = 0;

    //Quaternion orientation of earth frame relative to auxiliary frame.
    AEq_1 = 1;
    AEq_2 = 0;
    AEq_3 = 0;
    AEq_4 = 0;

    //Estimated orientation quaternion elements with initial conditions.
    SEq_1 = 1;
    SEq_2 = 0;
    SEq_3 = 0;
    SEq_4 = 0;

    //Sampling period (typical value is ~0.1s).
    deltat = rate;

    //Gyroscope measurement error (in degrees per second).
    gyroMeasError = gyroscopeMeasurementError;
    gyroMeasDrift = gyroscopeMeasurementDrift;

    b_x = 1;
    b_z = 0;

    w_bx = 0;
    w_by = 0;
    w_bz = 0;

    //Compute beta.
    beta = sqrt(3.0 / 4.0) * (PI * (gyroMeasError / 180.0));
    zeta = sqrt(3.0 / 4.0) * (PI * (gyroMeasDrift / 180.0));

}

void MARGfilter::updateFilter(double w_x, double w_y, double w_z, double a_x, double a_y, double a_z, double m_x, double m_y, double m_z) {

    // local system variables
    double norm; // vector norm
    double SEqDot_omega_1, SEqDot_omega_2, SEqDot_omega_3, SEqDot_omega_4; // quaternion rate from gyroscopes elements
    double f_1, f_2, f_3, f_4, f_5, f_6; // objective function elements
    double J_11or24, J_12or23, J_13or22, J_14or21, J_32, J_33, // objective function Jacobian elements
    J_41, J_42, J_43, J_44, J_51, J_52, J_53, J_54, J_61, J_62, J_63, J_64; //
    double SEqHatDot_1, SEqHatDot_2, SEqHatDot_3, SEqHatDot_4; // estimated direction of the gyroscope error
    double w_err_x, w_err_y, w_err_z; // estimated direction of the gyroscope error (angular)
    double h_x, h_y, h_z; // computed flux in the earth frame
    // axulirary variables to avoid reapeated calcualtions
    double halfSEq_1 = 0.5 * SEq_1;
    double halfSEq_2 = 0.5 * SEq_2;
    double halfSEq_3 = 0.5 * SEq_3;
    double halfSEq_4 = 0.5 * SEq_4;
    double twoSEq_1 = 2.0 * SEq_1;
    double twoSEq_2 = 2.0 * SEq_2;
    double twoSEq_3 = 2.0 * SEq_3;
    double twoSEq_4 = 2.0 * SEq_4;
    double twob_x = 2.0 * b_x;
    double twob_z = 2.0 * b_z;
    double twob_xSEq_1 = 2.0 * b_x * SEq_1;
    double twob_xSEq_2 = 2.0 * b_x * SEq_2;
    double twob_xSEq_3 = 2.0 * b_x * SEq_3;
    double twob_xSEq_4 = 2.0 * b_x * SEq_4;
    double twob_zSEq_1 = 2.0 * b_z * SEq_1;
    double twob_zSEq_2 = 2.0* b_z * SEq_2;
    double twob_zSEq_3 = 2.0 * b_z * SEq_3;
    double twob_zSEq_4 = 2.0 * b_z * SEq_4;
    double SEq_1SEq_2;
    double SEq_1SEq_3 = SEq_1 * SEq_3;
    double SEq_1SEq_4;
    double SEq_2SEq_3;
    double SEq_2SEq_4 = SEq_2 * SEq_4;
    double SEq_3SEq_4;
    double twom_x = 2.0 * m_x;
    double twom_y = 2.0 * m_y;
    double twom_z = 2.0 * m_z;
    // normalise the accelerometer measurement
    norm = sqrt(a_x * a_x + a_y * a_y + a_z * a_z);
    a_x /= norm;
    a_y /= norm;
    a_z /= norm;
    // normalise the magnetometer measurement
    norm = sqrt(m_x * m_x + m_y * m_y + m_z * m_z);
    m_x /= norm;
    m_y /= norm;
    m_z /= norm;
    // compute the objective function and Jacobian
    f_1 = twoSEq_2 * SEq_4 - twoSEq_1 * SEq_3 - a_x;
    f_2 = twoSEq_1 * SEq_2 + twoSEq_3 * SEq_4 - a_y;
    f_3 = 1.0 - twoSEq_2 * SEq_2 - twoSEq_3 * SEq_3 - a_z;
    f_4 = twob_x * (0.5 - SEq_3 * SEq_3 - SEq_4 * SEq_4) + twob_z * (SEq_2SEq_4 - SEq_1SEq_3) - m_x;
    f_5 = twob_x * (SEq_2 * SEq_3 - SEq_1 * SEq_4) + twob_z * (SEq_1 * SEq_2 + SEq_3 * SEq_4) - m_y;
    f_6 = twob_x * (SEq_1SEq_3 + SEq_2SEq_4) + twob_z * (0.5 - SEq_2 * SEq_2 - SEq_3 * SEq_3) - m_z;
    J_11or24 = twoSEq_3; // J_11 negated in matrix multiplication
    J_12or23 = 2.0 * SEq_4;
    J_13or22 = twoSEq_1; // J_12 negated in matrix multiplication
    J_14or21 = twoSEq_2;
    J_32 = 2.0 * J_14or21; // negated in matrix multiplication
    J_33 = 2.0 * J_11or24; // negated in matrix multiplication
    J_41 = twob_zSEq_3; // negated in matrix multiplication
    J_42 = twob_zSEq_4;
    J_43 = 2.0 * twob_xSEq_3 + twob_zSEq_1; // negated in matrix multiplication
    J_44 = 2.0 * twob_xSEq_4 - twob_zSEq_2; // negated in matrix multiplication
    J_51 = twob_xSEq_4 - twob_zSEq_2; // negated in matrix multiplication
    J_52 = twob_xSEq_3 + twob_zSEq_1;
    J_53 = twob_xSEq_2 + twob_zSEq_4;
    J_54 = twob_xSEq_1 - twob_zSEq_3; // negated in matrix multiplication
    J_61 = twob_xSEq_3;
    J_62 = twob_xSEq_4 - 2.0 * twob_zSEq_2;
    J_63 = twob_xSEq_1 - 2.0 * twob_zSEq_3;
    J_64 = twob_xSEq_2;
    // compute the gradient (matrix multiplication)
    SEqHatDot_1 = J_14or21 * f_2 - J_11or24 * f_1 - J_41 * f_4 - J_51 * f_5 + J_61 * f_6;
    SEqHatDot_2 = J_12or23 * f_1 + J_13or22 * f_2 - J_32 * f_3 + J_42 * f_4 + J_52 * f_5 + J_62 * f_6;
    SEqHatDot_3 = J_12or23 * f_2 - J_33 * f_3 - J_13or22 * f_1 - J_43 * f_4 + J_53 * f_5 + J_63 * f_6;
    SEqHatDot_4 = J_14or21 * f_1 + J_11or24 * f_2 - J_44 * f_4 - J_54 * f_5 + J_64 * f_6;
    // normalise the gradient to estimate direction of the gyroscope error
    norm = sqrt(SEqHatDot_1 * SEqHatDot_1 + SEqHatDot_2 * SEqHatDot_2 + SEqHatDot_3 * SEqHatDot_3 + SEqHatDot_4 * SEqHatDot_4);
    SEqHatDot_1 = SEqHatDot_1 / norm;
    SEqHatDot_2 = SEqHatDot_2 / norm;
    SEqHatDot_3 = SEqHatDot_3 / norm;
    SEqHatDot_4 = SEqHatDot_4 / norm;
    // compute angular estimated direction of the gyroscope error
    w_err_x = twoSEq_1 * SEqHatDot_2 - twoSEq_2 * SEqHatDot_1 - twoSEq_3 * SEqHatDot_4 + twoSEq_4 * SEqHatDot_3;
    w_err_y = twoSEq_1 * SEqHatDot_3 + twoSEq_2 * SEqHatDot_4 - twoSEq_3 * SEqHatDot_1 - twoSEq_4 * SEqHatDot_2;
    w_err_z = twoSEq_1 * SEqHatDot_4 - twoSEq_2 * SEqHatDot_3 + twoSEq_3 * SEqHatDot_2 - twoSEq_4 * SEqHatDot_1;
    // compute and remove the gyroscope baises
    w_bx += w_err_x * deltat * zeta;
    w_by += w_err_y * deltat * zeta;
    w_bz += w_err_z * deltat * zeta;
    w_x -= w_bx;
    w_y -= w_by;
    w_z -= w_bz;
    // compute the quaternion rate measured by gyroscopes
    SEqDot_omega_1 = -halfSEq_2 * w_x - halfSEq_3 * w_y - halfSEq_4 * w_z;
    SEqDot_omega_2 = halfSEq_1 * w_x + halfSEq_3 * w_z - halfSEq_4 * w_y;
    SEqDot_omega_3 = halfSEq_1 * w_y - halfSEq_2 * w_z + halfSEq_4 * w_x;
    SEqDot_omega_4 = halfSEq_1 * w_z + halfSEq_2 * w_y - halfSEq_3 * w_x;
    // compute then integrate the estimated quaternion rate
    SEq_1 += (SEqDot_omega_1 - (beta * SEqHatDot_1)) * deltat;
    SEq_2 += (SEqDot_omega_2 - (beta * SEqHatDot_2)) * deltat;
    SEq_3 += (SEqDot_omega_3 - (beta * SEqHatDot_3)) * deltat;
    SEq_4 += (SEqDot_omega_4 - (beta * SEqHatDot_4)) * deltat;
    // normalise quaternion
    norm = sqrt(SEq_1 * SEq_1 + SEq_2 * SEq_2 + SEq_3 * SEq_3 + SEq_4 * SEq_4);
    SEq_1 /= norm;
    SEq_2 /= norm;
    SEq_3 /= norm;
    SEq_4 /= norm;
    // compute flux in the earth frame
    SEq_1SEq_2 = SEq_1 * SEq_2; // recompute axulirary variables
    SEq_1SEq_3 = SEq_1 * SEq_3;
    SEq_1SEq_4 = SEq_1 * SEq_4;
    SEq_3SEq_4 = SEq_3 * SEq_4;
    SEq_2SEq_3 = SEq_2 * SEq_3;
    SEq_2SEq_4 = SEq_2 * SEq_4;
    h_x = twom_x * (0.5 - SEq_3 * SEq_3 - SEq_4 * SEq_4) + twom_y * (SEq_2SEq_3 - SEq_1SEq_4) + twom_z * (SEq_2SEq_4 + SEq_1SEq_3);
    h_y = twom_x * (SEq_2SEq_3 + SEq_1SEq_4) + twom_y * (0.5 - SEq_2 * SEq_2 - SEq_4 * SEq_4) + twom_z * (SEq_3SEq_4 - SEq_1SEq_2);
    h_z = twom_x * (SEq_2SEq_4 - SEq_1SEq_3) + twom_y * (SEq_3SEq_4 + SEq_1SEq_2) + twom_z * (0.5 - SEq_2 * SEq_2 - SEq_3 * SEq_3);
    // normalise the flux vector to have only components in the x and z
    b_x = sqrt((h_x * h_x) + (h_y * h_y));
    b_z = h_z;

    if (firstUpdate == 0) {
        //Store orientation of auxiliary frame.
        AEq_1 = SEq_1;
        AEq_2 = SEq_2;
        AEq_3 = SEq_3;
        AEq_4 = SEq_4;
        firstUpdate = 1;
    }

}

void MARGfilter::computeEuler(void){

    //Quaternion describing orientation of sensor relative to earth.
    double ESq_1, ESq_2, ESq_3, ESq_4;
    //Quaternion describing orientation of sensor relative to auxiliary frame.
    double ASq_1, ASq_2, ASq_3, ASq_4;

    //Compute the quaternion conjugate.
    ESq_1 = SEq_1;
    ESq_2 = -SEq_2;
    ESq_3 = -SEq_3;
    ESq_4 = -SEq_4;

    //Compute the quaternion product.
    ASq_1 = ESq_1 * AEq_1 - ESq_2 * AEq_2 - ESq_3 * AEq_3 - ESq_4 * AEq_4;
    ASq_2 = ESq_1 * AEq_2 + ESq_2 * AEq_1 + ESq_3 * AEq_4 - ESq_4 * AEq_3;
    ASq_3 = ESq_1 * AEq_3 - ESq_2 * AEq_4 + ESq_3 * AEq_1 + ESq_4 * AEq_2;
    ASq_4 = ESq_1 * AEq_4 + ESq_2 * AEq_3 - ESq_3 * AEq_2 + ESq_4 * AEq_1;

    //Compute the Euler angles from the quaternion.
    phi = atan2(2 * ASq_3 * ASq_4 - 2 * ASq_1 * ASq_2, 2 * ASq_1 * ASq_1 + 2 * ASq_4 * ASq_4 - 1);
    theta = asin(2 * ASq_2 * ASq_3 - 2 * ASq_1 * ASq_3);
    psi = atan2(2 * ASq_2 * ASq_3 - 2 * ASq_1 * ASq_4, 2 * ASq_1 * ASq_1 + 2 * ASq_2 * ASq_2 - 1);

}

double MARGfilter::getRoll(void){

    return phi;

}

double MARGfilter::getPitch(void){

    return theta;

}

double MARGfilter::getYaw(void){

    return psi;

}

void MARGfilter::reset(void) {

    firstUpdate = 0;

    //Quaternion orientation of earth frame relative to auxiliary frame.
    AEq_1 = 1;
    AEq_2 = 0;
    AEq_3 = 0;
    AEq_4 = 0;

    //Estimated orientation quaternion elements with initial conditions.
    SEq_1 = 1;
    SEq_2 = 0;
    SEq_3 = 0;
    SEq_4 = 0;

    b_x = 1;
    b_z = 0;

    w_bx = 0;
    w_by = 0;
    w_bz = 0;

}
