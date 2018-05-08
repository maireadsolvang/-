#ifndef _compass_h_
#define _compass_h_


#include <Wire.h>
#include <mpu9255_esp32.h>
#include <math.h>

#define PI 3.14159


class Compass {
  public:
  float heading;
  float declination;
  MPU9255& imu;
  Compass(float dec, MPU9255& in_imu );
  void calibrate();
  void update();
};
#endif