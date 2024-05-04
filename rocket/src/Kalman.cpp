// ===================================
// ========= Kalman filter ===========
// ===================================

/*
 *
* By Gunnar Edman
*/

// MIT License

// Copyright (c) 2023 Matan Pazi (Used work from "ThePoorEngineer")

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.



#include <BasicLinearAlgebra.h>

using namespace BLA;    // BLA defined as a basic linear algebra construct

// Process distrubance covariance (needs tuning)
float q = 0.0001;

BLA::Matrix<4,4> A = {1.0, 0.005, 0.0000125, 0,
                      0, 1.0, 0.005, 0,
                      0, 0, 1.0, 0,
                      0, 0, 0, 1};

BLA::Matrix<3,4> H = {1.0, 0, 0, 0,
                      0, 1.0, 0, 1,
                      0, 0, 1.0, 0};

BLA::Matrix<4,4> P = {0.039776, 0, 0, 0,
                      0, 0.012275, 0, 0,
                      0, 0, 0.018422, 0,
                      0, 0, 0, 0};

BLA::Matrix<3,3> R = {0.039776, 0, 0,
                      0, 0.012275, 0,
                      0, 0, 0.018422};

BLA::Matrix<4,4> Q = {q, 0, 0, 0,
                      0, q, 0, 0,
                      0, 0, q, 0,
                      0, 0, 0, q};

BLA::Matrix<4,4> I = {1.0, 0, 0, 0,
                      0, 1.0, 0, 0,
                      0, 0, 1.0, 0,
                      0, 0, 0, 1.0};

BLA::Matrix<4,1> x_hat = {0.0,
                          0.0,
                          0.0,
                          0.0};                        
                      
#define NUM_OF_ITERATIONS 200 
float AccX, AccY, AccZ;
float GyroX, GyroY, GyroZ;
float accAngleX, accAngleY, gyroAngleX, gyroAngleY, gyroAngleZ;
float roll, pitch_comp, yaw, pitch_kal;
float GyroErrorX, GyroErrorY, GyroErrorZ;
float elapsedTime, currentTime, previousTime;
float alpha;
float Integral = 0.0;
int c = 0;
int pos;
long counter = 0;
long GyroErrors = 0;
bool IsFirstRun = true;
long loopTime = 5000;  // Microseconds
unsigned long timer = 0;
int WhichError = 0;

void kalmanSetup() {
  timer = micros();
}

// Kalman step
void kalmanStep() {
  // === Read acceleromter data === //
  timeSync(loopTime);
  Wire.beginTransmission(MPU);
  Wire.write(0x3B); // Start with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true); // Read 6 registers total, each axis value is stored in 2 registers
  //For a range of +-2g, we need to divide the raw values by 16384, according to the datasheet
  AccX = (Wire.read() << 8 | Wire.read()) / 16384.0 - 0.02; // X-axis value
  AccY = (Wire.read() << 8 | Wire.read()) / 16384.0; // Y-axis value
  AccZ = (Wire.read() << 8 | Wire.read()) / 16384.0 - 0.02; // Z-axis value
  // Calculating Roll and Pitch from the accelerometer data
  accAngleY = (atan(-1 * AccX / sqrt(pow(AccY, 2) + pow(AccZ, 2))) * 180 / PI); // AccErrorY ~(-1.58)
  // === Read gyroscope data === //
  previousTime = currentTime;        // Previous time is stored before the actual time read
  currentTime = millis();            // Current time actual time read
  elapsedTime = (currentTime - previousTime) / 1000; // Divide by 1000 to get seconds
  Wire.beginTransmission(MPU);
  Wire.write(0x45); // Gyro data Y axis register address 0x45
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 2, true); // Read 2 registers total, each axis value is stored in 2 registers
  GyroY = (Wire.read() << 8 | Wire.read()) / 131.0;   // For a 250deg/s range we have to divide first the raw value by 131.0, according to the datasheet
  GyroY = GyroY - GyroErrorY;
//  // Initializing
  if (IsFirstRun)
  {
    gyroAngleY = accAngleY;
  }
  gyroAngleY = gyroAngleY + GyroY * elapsedTime;
//  // Complementary filter - combine acceleromter and gyro angle values
  pitch_comp = float (0.96 * gyroAngleY + 0.04 * accAngleY);

  alpha = AccZ - 1 + sin(accAngleY);

  BLA::Matrix<3,1> z = {accAngleY,
                        GyroY,
                        alpha};
  
  if (IsFirstRun) 
  {
  BLA::Matrix<4,1> x_hat = {accAngleY,
                            GyroY,
                            alpha,
                            0};
  }
  IsFirstRun = false;

  /// Kalman Filter start
  BLA::Matrix<4,1> x_hat_minus = A * x_hat;
  BLA::Matrix<4,4> P_minus = A * P * (~A) + Q;
  BLA::Matrix<4,3> K = P_minus * (~H) * ((H * P_minus * (~H) + R)).Inverse();
  x_hat = x_hat_minus + K * (z - (H * x_hat_minus));
  P = (I - K * H) * P_minus;

  /// Kalman Filter end
  
  pitch_kal = float (x_hat(0));
  
  // Command servo to go according to pitch angle
  float Pos_err = -pitch_kal;
  float Kp = 2.2;
  float Ki = 0.05;
  Integral = Integral + Pos_err * Ki;
  pos = 90.9 + Pos_err * Kp + Integral;  // Found empirically
  
  // Our hardware limits
  if (pos < 60)
  {
    pos = 60;
  }
  else if (pos > 150)
  {
    pos = 150;
  }
  
  myservo.write(pos);

  sendToPC(&pitch_kal, &pitch_comp);
}



// ============
// Sub routines
// ============
void timeSync(unsigned long deltaT)
{
  unsigned long currTime = micros();
  long timeToDelay = deltaT - (currTime - timer);
  if (timeToDelay > 5000)
  {
    delay(timeToDelay / 1000);
    delayMicroseconds(timeToDelay % 1000);
  }
  else if (timeToDelay > 0)
  {
    delayMicroseconds(timeToDelay);
  }
  else
  {
      // timeToDelay is negative so we start immediately
  }
  timer = currTime + timeToDelay;
}