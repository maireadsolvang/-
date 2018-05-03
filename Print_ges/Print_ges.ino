#include <mpu9255_esp32.h>
#include <U8g2lib.h>
#include<math.h>
#include <WiFi.h>
#define SCREEN U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI

const int LOOP_SPEED = 50; //milliseconds
int primary_timer = 0;

float display_timer = 0;
String current_action;

float x,y,z; //variables for grabbing x,y,and z values

int const avg_ges_len = 12;

float corr_val2_x;
float corr_val2_y;
float corr_val2_z;
float corr_val2;

float corr_val3_x;
float corr_val3_y;
float corr_val3_z;
float corr_val3;

float running_xs[avg_ges_len];
float running_ys[avg_ges_len];
float running_zs[avg_ges_len];

float gesture_2_xs[avg_ges_len] = {-0.9754326923076926, -1.0219449300699304, -1.04583479020979, -1.0007342657342657, -0.8102403846153846, -0.5905506993006997, -0.41856206293706283, -0.36756118881118877, -0.5110708041958042, -0.688492132867133, -0.78138986013986, -0.8438942307692311};
float gesture_2_ys[avg_ges_len] = {-0.02966346153846153, -0.04720716783216783, 0.0021765734265734213, 0.06812062937062933, 0.058146853146853146, -0.04951923076923075, -0.185034965034965, -0.34396853146853146, -0.38666958041958055, -0.269506118881119, -0.1536538461538463, -0.11653846153846156};
float gesture_2_zs[avg_ges_len] =  {-0.5982692307692309, -0.8000786713286713, -0.8783435314685313, -0.7620323426573421, -0.33711975524475524, 0.17630681818181815, 0.6267045454545455, 1.0070017482517482, 0.9788898601398605, 0.6240166083916079, 0.4636756993006992, 0.43716346153846153};

float gesture_3_xs[avg_ges_len] = {-0.275, 0.11219873150105708, 0.44570295983086683, 0.7528646934460889, 1.0645031712473576, 1.311057082452432, 1.3617811839323457, 1.2419133192389007, 1.0200158562367865, 0.792394291754757, 0.6361733615221988, 0.5191279069767438};
float gesture_3_ys[avg_ges_len] = {0.7030232558139534, 0.9525528541226215, 0.9366966173361531, 0.4996035940803384, -0.33204545454545437, -1.1733298097251583, -1.6876691331923894, -1.8520560253699803, -1.8530866807611004, -1.729016913319239, -1.5487579281183943, -1.3457558139534893};
float gesture_3_zs[avg_ges_len] = {-0.5715116279069766, -0.8838530655391118, -1.1313689217758984, -1.18473044397463, -1.094587737843552, -1.0629122621564484, -1.0702801268498943, -1.0095348837209297, -0.891183932346723, -0.7573572938689217, -0.6280708245243131, -0.5412790697674422};

MPU9255 imu; //imu object called, appropriately, imu
SCREEN oled(U8G2_R0, 5, 17,16);

void setup() {
  Serial.begin(115200); //for debugging if needed.
  delay(50); //pause to make sure comms get set up
  delay(50); //pause to make sure comms get set up
  
  oled.begin();     // initialize the OLED
  oled.clearBuffer();    //clear the screen contents
  oled.setFont(u8g2_font_5x7_tf);  //set font on oled  
  oled.setCursor(0,15); //set cursor
  oled.print("Starting"); //print starting message
  oled.sendBuffer(); //send it (so it displays)
  
  setup_imu();
  primary_timer = millis();
  display_timer = millis()-5001;
}

void loop() {
  
    //get accel values from imu
    imu.readAccelData(imu.accelCount);
    x = imu.accelCount[0]*imu.aRes;
    y = imu.accelCount[1]*imu.aRes;
    z = imu.accelCount[2]*imu.aRes;
    //update your list of recent accels
    update_list(running_xs, x);
    update_list(running_ys, y);
    update_list(running_zs, z); 
    //update your correlation variables to get correlation values at current time
    set_corr_vals(corr_val2_x, corr_val2_y, corr_val2_z, corr_val2, gesture_2_xs, gesture_2_ys, gesture_2_zs);
    set_corr_vals(corr_val3_x, corr_val3_y, corr_val3_z, corr_val3, gesture_3_xs, gesture_3_ys, gesture_3_zs);


    if (corr_val2_x - corr_val2_z > 8 && corr_val2_z < -3 && corr_val2_y < 0.25){
      current_action = "Push";  
      display_timer = millis();
    }  

    else if (corr_val2 > 8.7 && millis()-display_timer > 2000){
      current_action = "Charging";
      display_timer = millis();
    }
    if (corr_val3 > 18){
      current_action = "Slash";  
      display_timer = millis();
    }        
    if (millis()-display_timer < 3000){
      oled_print(current_action);
    } else{oled_print("");}
    while (millis()-primary_timer<LOOP_SPEED); //wait for primary timer to increment
    primary_timer =millis();

}



void update_list(float running_vals[avg_ges_len], float val){
  for (int i=avg_ges_len-1; i > 0; i--){
    running_vals[i] = running_vals[i-1];
  }
  running_vals[0] = val;
}

void cross_corr(float &corr_val, float running_vals[avg_ges_len], float corr_vals[avg_ges_len]){
  corr_val = 0;
  for (int i=0; i < avg_ges_len; i++){
    corr_val += running_vals[i]*corr_vals[i];
  } 
}

void set_corr_vals(float &corr_x, float &corr_y, float &corr_z, float &corr_val, float gesture_xs[avg_ges_len],float gesture_ys[avg_ges_len],float gesture_zs[avg_ges_len]){
    cross_corr(corr_x,running_xs,gesture_xs);
    cross_corr(corr_y,running_ys,gesture_ys);
    cross_corr(corr_z,running_zs,gesture_zs);
    corr_val = corr_x + corr_y + corr_z;
}

//helper function to encapsulate basic printing actions
void oled_print(String input){
  oled.clearBuffer();
  oled.setCursor(0,15);
  oled.print(input);
  oled.sendBuffer();     // update the screen
}



void setup_imu(){
  if (imu.readByte(MPU9255_ADDRESS, WHO_AM_I_MPU9255) == 0x73){
    imu.initMPU9255();
  }else{
    while(1); //Serial.println("NOT FOUND"); // Loop forever if communication doesn't happen
  }
  imu.getAres(); //call this so the IMU internally knows its range/resolution
}



