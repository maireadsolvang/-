#include <compass.h>



Compass::Compass(float dec, MPU9255& in_imu ): imu(in_imu) {
    heading = 0;
    declination = dec;
 }
void Compass::calibrate() {
    // The next call delays for 4 seconds, and then records about 15 seconds of
    // data to calculate bias and scale.
    imu.magCalMPU9255(imu.magBias, imu.magScale);
}
void Compass::update() {
    imu.readMagData(imu.magCount);  // Read the x/y/z adc values
    // Calculate the magnetometer values in milliGauss
    // Include factory calibration per data sheet and user environmental
    // corrections
    // Get actual magnetometer value, this depends on scale being set
    imu.mx = (float)imu.magCount[0] * imu.mRes* imu.factoryMagCalibration[0] - imu.magBias[0];
    imu.my = (float)imu.magCount[1] * imu.mRes* imu.factoryMagCalibration[1] - imu.magBias[1];
    imu.mz = (float)imu.magCount[2] * imu.mRes* imu.factoryMagCalibration[2] - imu.magBias[2];
   // if (readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01)
    // Get accelerometer reading
    imu.readAccelData(imu.accelCount);  // Read the x/y/z adc values
    imu.ax = (float)imu.accelCount[0] * imu.aRes - imu.accelBias[0];
    imu.ay = (float)imu.accelCount[1] * imu.aRes - imu.accelBias[1];
    imu.az = (float)imu.accelCount[2] * imu.aRes - imu.accelBias[2];
     // Calculate component of magnetism that is perpendicular to gravity 
    float gravMagnitude = 1.0*(imu.mx*imu.ax + imu.my*imu.ay + imu.mz*imu.az) / (imu.ax*imu.ax + imu.ay*imu.ay + imu.az*imu.az);
    float northx = imu.mx - gravMagnitude * imu.ax;
    float northy = imu.my - gravMagnitude * imu.ay;
    //float northz = mz - gravMagnitude * az;
    //note: northAngle is "math-y", so it's 0 on +x-axis and increases counterclockwise
    float northAngle = -1*atan2(northy, northx)*180/PI; //Heading is based on X,Y components of North
    northAngle += declination; //Account for declination
    heading = -northAngle;  // Find our heading, given northAngle
    heading -= 90; //Change axes: Now 0 degrees is straight ahead (+y-axis)
    heading = int(heading+360)%360 + (heading - int(heading)); //Hack-y mod operation for floats
}

