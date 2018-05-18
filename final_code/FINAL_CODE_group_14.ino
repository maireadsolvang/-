#include <mpu9255_esp32.h>
#include <adp5350.h>
#include <U8g2lib.h>
#include <compass.h> //COMPASS FOLDER UPLOADED TO GITHUB
#include<math.h>
#include <WiFi.h>
#include <SPI.h>
#include "esp32-hal-ledc.h" //MUST BE PUT IN GAME_CODE FOLDER
#include "DFRobotDFPlayerMini.h"
#include "Arduino.h"
#include <Adafruit_NeoPixel.h>
#define SCREEN U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI

//declare instance of ADP5350 class
ADP5350 adp;

/// IMU AND GESTURE VARIABLES ///
const int LOOP_SPEED = 50; //milliseconds
float x, y, z; //variables for grabbing x,y,and z values
float accelMagnitude[3] = {0.0, 0.0, 0.0};
MPU9255 imu; //imu object called, appropriately, imu
SCREEN oled(U8G2_R0, 5, 17, 16);
const int response_timeout = 6000; //ms to wait for response from host
const int post_response_timeout = 3000;
int response_timer;
bool pushedBefore = false;

/// MICROPHONE VARIABLES ///
int sound_state = 1;
int volume = 30;
HardwareSerial mySerial(2); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);

//LED VARIABLES//
#define LED_PIN 19
#define NUM_HEARTS 5
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_HEARTS, LED_PIN, NEO_GRB + NEO_KHZ800);

///MAGNETOMETER VARIABLES///
Compass compass(-14.75, imu);
#define CALIBRATE 1

/////ACTION STATE MACHINE VARIABLES////
int button_pin = 15;
int action_state = 0;
int game_over_timer;
const int g_o_time = 5000;
#define GAME_RESTING 0
#define IN_GAME 2
#define GESTURE_RECOGNITION 9
#define WAITING_TO_START 1

/// SONGS TO PLAY ///
#define SONG_FIGHT 4
#define SONG_WIN 3
#define SONG_LOSE 5
#define SONG_SLASH 9
#define SONG_PUSH 7
#define SONG_CHARGE 8
#define SONG_BLOCK 6

/////// NEW Format STATE MACHINE VAR //////
int game_ID = -1;
String player_ID = "popsifbbbbookkjlio69";
int action;
int health = -1;
String data = "";
int data_timer = 0;
int game_state = 0;
String received;


/////SLEEP VARS//////
bool Power = true;
const int power_threshold = 60000;
const float imu_sleep_threshold = .7;
long int power_timer = 0;
#define BUTTON_PIN_BITMASK 0x200000000
RTC_DATA_ATTR int bootCount = 0;
float sum =1;

//==================================================================================================================================================================
//Gesture constants
int  primary_timer;
int const avg_ges_len = 12;
int charged = 0;
String current_action = "";
bool act_happened = false;

const int get_thres = 1000;
int get_timer;

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
const int act_length = 1000 / 50;
String running_acts[act_length];
const int charge_length = 2000 / 50;
float running_charges[charge_length];

float gesture_2_xs[avg_ges_len] = { -0.9754326923076926, -1.0219449300699304, -1.04583479020979, -1.0007342657342657, -0.8102403846153846, -0.5905506993006997, -0.41856206293706283, -0.36756118881118877, -0.5110708041958042, -0.688492132867133, -0.78138986013986, -0.8438942307692311};
float gesture_2_ys[avg_ges_len] = { -0.02966346153846153, -0.04720716783216783, 0.0021765734265734213, 0.06812062937062933, 0.058146853146853146, -0.04951923076923075, -0.185034965034965, -0.34396853146853146, -0.38666958041958055, -0.269506118881119, -0.1536538461538463, -0.11653846153846156};
float gesture_2_zs[avg_ges_len] =  { -0.5982692307692309, -0.8000786713286713, -0.8783435314685313, -0.7620323426573421, -0.33711975524475524, 0.17630681818181815, 0.6267045454545455, 1.0070017482517482, 0.9788898601398605, 0.6240166083916079, 0.4636756993006992, 0.43716346153846153};

float gesture_3_xs[avg_ges_len] = { -0.275, 0.11219873150105708, 0.44570295983086683, 0.7528646934460889, 1.0645031712473576, 1.311057082452432, 1.3617811839323457, 1.2419133192389007, 1.0200158562367865, 0.792394291754757, 0.6361733615221988, 0.5191279069767438};
float gesture_3_ys[avg_ges_len] = {0.7030232558139534, 0.9525528541226215, 0.9366966173361531, 0.4996035940803384, -0.33204545454545437, -1.1733298097251583, -1.6876691331923894, -1.8520560253699803, -1.8530866807611004, -1.729016913319239, -1.5487579281183943, -1.3457558139534893};
float gesture_3_zs[avg_ges_len] = { -0.5715116279069766, -0.8838530655391118, -1.1313689217758984, -1.18473044397463, -1.094587737843552, -1.0629122621564484, -1.0702801268498943, -1.0095348837209297, -0.891183932346723, -0.7573572938689217, -0.6280708245243131, -0.5412790697674422};

//==================================================================================================================================================================


void setup() {
  fake_setup();
  Serial.println("Set up done");

}

void loop(){
  record_IMU_data();
  if(Power == true){
    //Regular loop
    loop_on();
  }
  else{
   //Go to sleep!
   update_leds(strip.Color(0,0,0));
   esp_light_sleep_start();
   Serial.begin(115200);
   Serial.println("just woke up");
   fake_setup();
  }
}

void loop_on() {
/////POWER/////
  if (sum > imu_sleep_threshold){
    power_timer = millis();
  }
  if ((millis()-power_timer)>power_threshold){
    //Go to sleep
    Power = false;
  }
  else{
    //Stay awake
    Power = true;
  }
  
  /// speaker //////
  if (myDFPlayer.available()) {
    printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }

  //Update LEDs
  update_leds(strip.Color(0,0,50));
  charged = 0;

  ///MAIN STATE MACHINE/////
  switch (action_state) {
    /////////////////////////////////////////////////////////////////////////////////////////////////
    case GAME_RESTING: {
        //Reset variables for a new game
        game_ID = -1;
        health = -1;
        data_timer = 0;
        game_state = 0;
        /////BUTTON PRESS TO START SIGNAL//////
        int button_reading = !digitalRead(button_pin);
        if (button_reading) {
          do_POST(7);
          action_state = WAITING_TO_START;
        }
      break;
      }
    /////////////////////////////////////////////////////////////////////////////////////////////////
    case WAITING_TO_START: {
        int button_reading = !digitalRead(button_pin);
        /////CONTINUALLY CHECKS IF OTHER PLAYER HAS RECEIVED SIGNAL AND GAME HAS STARTED/////

        //GAME STARTED
        if (game_state == 1) { 
          action_state = IN_GAME;
          myDFPlayer.play(SONG_FIGHT);
        }
        //PLAYER STOPPED PRESSING BUTTON BEFORE GAME STARTED
        else if (!button_reading) {
          action_state = GAME_RESTING;
        }
        //CHECKING IF GAME HAS BEGIN
        else{
          do_GET(1);
        }
        break;
      }
    /////////////////////////////////////////////////////////////////////////////////////////////////
    case IN_GAME: {
        //GET REQUEST EVERY GET_THRES
        if (millis() - get_timer > get_thres){
          do_GET(8);
          get_timer = millis(); 
        }
        //UPDATES CURRENT_ACTION VARIABLE
        gesture_stuff();
        act_happened = false;
        //CHECKS IF ACTION HAS ALREADY BEEN SENT TO SERVER
        has_happened(current_action);

        if (!act_happened) {
          //SLASH ACTION
          if (current_action == "Slash") {
            myDFPlayer.play(SONG_SLASH);
            do_POST(2);
            get_timer = millis();
          }
          //PUSH ACTION
          else if (current_action == "Push") {
            myDFPlayer.play(SONG_PUSH);
            do_POST(3);
            for (int i = 0; i < charge_length; i++) {
              running_charges[i] = 0;
            }
            get_timer = millis();
          }
          //BLOCK ACTION
          else if (current_action == "Block") {
            myDFPlayer.play(SONG_BLOCK);
            do_POST(4);
            get_timer = millis();
          }
        }

        //CHECK GAME STATE FROM RECEIVED VARIABLE
        if (game_state == 2) { //GAME OVER
          if (health == 0) {
            //YOU LOST -> PLAYER LOSER SOUND EFFECT
            myDFPlayer.play(SONG_LOSE);
            action_state = GAME_RESTING;
          }
          else {
            //YOU WON ->PLAYER WIN SOUND EFFECT
            myDFPlayer.play(SONG_WIN);
            action_state = GAME_RESTING;
          }
          current_action = "";
        }

        break;
      }
    /////////////////////////////////////////////////////////////////////////////////////////////////
    case GESTURE_RECOGNITION: {
        //USED FOR ADDING MORE DATA TO THE GESTURE DATABASE
        do_POST(2);
        action_state = WAITING_TO_START; //back to START_GAME
        break;
      }
  }
  while (millis() - primary_timer < LOOP_SPEED); //set Loop to update consistently for data
  primary_timer = millis();
}

void fake_setup(){
  Serial.begin(115200); //for debugging if needed.
  
  ////WIFI SETUP/////
  Wire.begin();
  WiFi.begin("MIT GUEST"); //attempt to connect to wifi
  //WiFi.begin("6s08","iesc6s08");
  delay(50); //pause to make sure comms get set up
  wifi_setup();

  ////ADP5350 SETUP////
  adp.setCharger(1); //Turn on charger
  adp.enableFuelGauge(1); //turn on voltage reading
  adp.enableLDO(1, 1); //Turn on LDO1
  adp.enableLDO(2, 1); //Turn on LDO2

  /////IMU SETUP/////
  setup_imu();

  ////OLED SETUP/////
  oled.begin();     // initialize the OLED
  oled_print("STARTING");

  ///LED SETUP/////
  strip.begin();
  update_leds(strip.Color(50,0,50));

  ////BUTTON SETUP/////
  pinMode(button_pin, INPUT_PULLUP);
  
  //MAGNETOMETER SETUP//
  if (CALIBRATE) { //Perform calibration routine
    Serial.println("Calibrating");
    compass.calibrate(); //Calibrate for a set # of milliseconds
  }
  
  ////SPEAKER SETUP////
  speaker_begin();

  ////MISC SETUP/////
  primary_timer = millis();
  get_timer = millis();

  ////POWER SETUP///
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_15,0);
  power_timer = millis();
  Power=true;
}

void printDetail(uint8_t type, int value) {
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void setup_imu() {
  //SETS UP IMU
  if (imu.readByte(MPU9255_ADDRESS, WHO_AM_I_MPU9255) == 0x73) {
    imu.initMPU9255();
  } else {
    while (1); //Serial.println("NOT FOUND"); // Loop forever if communication doesn't happen
  }
  imu.initMPU9255();
  imu.MPU9255SelfTest(imu.selfTest);
  imu.calibrateMPU9255(imu.gyroBias, imu.accelBias);
  Serial.println("accel calibrated");
  imu.initMPU9255();
  imu.initAK8963(imu.factoryMagCalibration);
  imu.getAres(); //call this so the IMU internally knows its range/resolution
}

void record_IMU_data() {
  //IMU ACCELEROMETER DATA
  imu.readAccelData(imu.accelCount);
  x = imu.accelCount[0] * imu.aRes;
  y = imu.accelCount[1] * imu.aRes;
  z = imu.accelCount[2] * imu.aRes;
  //ADDS TO DATA
  data += "Time:" + String(millis() - data_timer) + "Acc:" + String(x) + "," + String(y) + "," + String(z) + ";";
  //UPDATES GLOBAL VAR SUM
  sum=pow(pow(x,2)+pow(y,2)+pow(x,2),.5); 
}

bool magnetometer() { //Returns True if magnetometer > threshold and False if not
  //IMU MAGNETOMETER VALUES
  imu.readMagData(imu.magCount);  // Read the x/y/z adc values
  imu.mx = (float)imu.magCount[0] * imu.mRes * imu.factoryMagCalibration[0] - imu.magBias[0];
  imu.my = (float)imu.magCount[1] * imu.mRes * imu.factoryMagCalibration[1] - imu.magBias[1];
  imu.mz = (float)imu.magCount[2] * imu.mRes * imu.factoryMagCalibration[2] - imu.magBias[2];
  //MAGNITUDE OF X,Y,Z
  float mag = sqrt(float(imu.mx) * imu.mx + float(imu.my) * imu.my + float(imu.mz) * imu.mz);
  int threshold = 1500;
  //BLOCK IF MAG ABOVE THRESHOLD
  if (mag > threshold) {
    return true;
  }
  else {
    return false;
  }
}
void speaker_begin() {
  //SPEAKER SETUP
  mySerial.begin(9600, SERIAL_8N1, 32, 33);
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  if (!myDFPlayer.begin(mySerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while (true);
  }
  Serial.println(F("DFPlayer Mini online."));
  myDFPlayer.volume(volume);  //Set volume value. From 0 to 30

}

void do_POST(int action) {
  Serial.println("POSSTING");
  WiFiClient client; //instantiate a client object
  if (client.connect("iesc-s1.mit.edu", 80)) { //try to connect to class server
    // This will send the request to the server
    // If connected, fire off HTTP GET:
    String thing = "BODY";
    Serial.println("POSTED ACTION: " + String(action));
    client.println("POST /608dev/sandbox/smathew/final_project/server.py?action=" + String(action) + "&player_ID=" + String(player_ID) + "&game_ID=" + String(game_ID) + " HTTP/1.1");
    client.println("Host: iesc-s1.mit.edu");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Content-Length: " + String(thing.length()));
    client.print("\r\n");
    client.print(thing);
    unsigned long count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      String line = client.readStringUntil('\n');
      if (line == "\r") { //found a blank line!
        //headers have been received! (indicated by blank line)
        break;
      }
      if (millis() - count > response_timeout) break;
    }
    count = millis();
    String op; //create empty String object
    while (client.available()) { //read out remaining text (body of response)
      op += (char)client.read();
    }
    parse_received(op);
    client.stop();
  } else {
    oled_print("Connection Failed");
    Serial.println("connection failed");
    //Serial.println("wait 0.5 sec...");
    client.stop();
    delay(300);
  }
}


void do_GET(int act) {
  WiFiClient client; //instantiate a client object
  if (client.connect("iesc-s1.mit.edu", 80)) { //try to connect to numbersapi.com host
    // This will send the request to the server
    // If connected, fire off HTTP GET:
    client.println("GET /608dev/sandbox/smathew/final_project/server.py?action=" + String(act) + "&player_ID=" + player_ID + "&game_ID=" + String(game_ID) + " HTTP/1.1");
    client.println("Host: iesc-s1.mit.edu");
    client.print("\r\n");
    unsigned long count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      String line = client.readStringUntil('\n');
      if (line == "\r") { //found a blank line!
        //headers have been received! (indicated by blank line)
        break;
      }
      if (millis() - count > 6000) break;
    }
    count = millis();
    String op; //create empty String object
    while (client.available()) { //read out remaining text (body of response)
      op += (char)client.read();
    }
    parse_received(op);
    client.stop();
  } else {
    Serial.println("connection failed");
    Serial.println("wait 0.5 sec...");
    client.stop();
  }
}

void oled_print(String message) {
  //USED FOR DEBUGGING ON OLED SCREEN
  oled.clearBuffer();    //clear the screen contents
  oled.setFont(u8g2_font_5x7_tf);  //set font on oled
  oled.setCursor(0, 15); //set cursor
  oled.print(message); //print starting message
  oled.sendBuffer(); //send it (so it displays)
}


void wifi_setup() {
  //SETS UP WIFI
  int count = 0; //count used for Wifi check times
  while (WiFi.status() != WL_CONNECTED && count < 6) {
    delay(500);
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    oled_print("All Connected");
    delay(500);
  } else { //if we failed to connect just ry again.
    ESP.restart(); // restart the ESP
  }
}


void parse_received(String op) {
  // parses incoming strings from server(given as an arg to the func) and sets the glbal variables
  int first_comma =  op.indexOf(",");
  int second_comma = op.indexOf(",", first_comma + 1);
  int third_comma =  op.indexOf(",", second_comma + 1);
  //UPDATES HEALTH
  health = (op.substring(0, first_comma)).toInt();
  //UPDATES GAME_STATE
  game_state = (op.substring(first_comma + 1, second_comma)).toInt();
  //UPDATES GAME_ID
  game_ID = (op.substring(second_comma + 1)).toInt();
}

void print_wakeup_reason(){
  //SLEEP WAKEUP REASON
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason)
  {
    case 1  : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case 2  : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case 3  : Serial.println("Wakeup caused by timer"); break;
    case 4  : Serial.println("Wakeup caused by touchpad"); break;
    case 5  : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.println("Wakeup was not caused by deep sleep"); break;
  }
}


//==================================================================================================================================================================
//helper functions for gestures

void update_current_act() {
  corr_val2 = corr_val2_x + corr_val2_y + corr_val2_z;
  corr_val3 = corr_val3_x + corr_val3_y + corr_val3_z;
  Serial.println(String(corr_val2));
  update_charged();

  //BLOCK
  if (magnetometer()) {
    current_action = "Block";
  }
  //PUSH
  else if (corr_val2_x - corr_val2_z > 8 && corr_val2_z < -1 && corr_val2_y < .8
           && charged >= charge_length * 0.6) {
    current_action = "Push";
  }
  //CHARGING
  else if (corr_val2_x > 7.6) {
    current_action = "Charging";
    running_charges[0] = 1;
  }
  //SLASH
  else if (corr_val3 > 18) {
    current_action = "Slash";
  }
  //NO ACTION
  else {
    current_action = "";
  }
}

void update_list(float running_vals[avg_ges_len], float val) {
  for (int i = avg_ges_len - 1; i > 0; i--) {
    running_vals[i] = running_vals[i - 1];
  }
  running_vals[0] = val;
}

void update_charged() {
  for (int i = charge_length - 1; i > 0; i--) {
    charged += running_charges[i];
    running_charges[i] = running_charges[i - 1];
  }
  //CHARGING
  if (current_action == "Charging") {
    running_charges[0] = 1;
  } else {
    running_charges[0] = 0;
  }
}

void cross_corr(float &corr_val, float running_vals[avg_ges_len], float corr_vals[avg_ges_len]) {
  //CROSS CORRELATION
  corr_val = 0;
  for (int i = 0; i < avg_ges_len; i++) {
    corr_val += running_vals[i] * corr_vals[i];
  }
}

void set_corr_vals(float &corr_x, float &corr_y, float &corr_z, float &corr_val, float gesture_xs[avg_ges_len], float gesture_ys[avg_ges_len], float gesture_zs[avg_ges_len]) {
  //SETS THE CORRELATION VALUES
  cross_corr(corr_x, running_xs, gesture_xs);
  cross_corr(corr_y, running_ys, gesture_ys);
  cross_corr(corr_z, running_zs, gesture_zs);
  corr_val = corr_x + corr_y + corr_z;
}

void gesture_stuff() {
  //get accel values from imu
  imu.readAccelData(imu.accelCount);
  x = imu.accelCount[0] * imu.aRes;
  y = imu.accelCount[1] * imu.aRes;
  z = imu.accelCount[2] * imu.aRes;

  //update your list of recent accels
  update_list(running_xs, x);
  update_list(running_ys, y);
  update_list(running_zs, z);

  for (int i = act_length - 1; i > 0; i--) {
    running_acts[i] = running_acts[i - 1];
  }
  running_acts[0] = current_action;

  //update your correlation variables to get correlation values at current time
  set_corr_vals(corr_val2_x, corr_val2_y, corr_val2_z, corr_val2, gesture_2_xs, gesture_2_ys, gesture_2_zs);
  set_corr_vals(corr_val3_x, corr_val3_y, corr_val3_z, corr_val3, gesture_3_xs, gesture_3_ys, gesture_3_zs);
  //updates current_action variable
  update_current_act();
}

void has_happened(String action) {
  //CHECKS IF ACTION HAS ALREADY BEEN POSTED
  for (int i = 0; i < act_length; i++) {
    if (running_acts[i] == action) {
      act_happened = true;
    }
  }
  if (current_action == "Block"){
    act_happened = false;
  }
}

void update_leds(uint32_t c){
  //UPDATES THE LEDS
  float percent_charged = charged/(charge_length * 0.6);
  //CHARGING GESTURE -> LEDS BLINK UP STRIP
  if (percent_charged >= 0.2){
    for (uint16_t i = 0; i < 5; i++) {
        strip.setPixelColor(i, strip.Color(0,0,0));
    }
    if (percent_charged >= .2){
      strip.setPixelColor(0, strip.Color(0,50,0));
    }
    if (percent_charged >= .4){
      strip.setPixelColor(1, strip.Color(0,50,0));
    }
    if (percent_charged >= .6){
      strip.setPixelColor(2, strip.Color(0,50,0));
    }
    if (percent_charged >= .8){
      strip.setPixelColor(3, strip.Color(0,50,0));
    }
    if (percent_charged >= 1.0){
      strip.setPixelColor(4, strip.Color(0,50,0));
    }
  }
  else{
    //NOT IN GAME -> ALL BLUE LEDS
    if(health == -1){
      for (uint16_t i = 0; i < 5; i++) {
        strip.setPixelColor(i, c);
      }
      strip.show();
    }
    else{
      //IN GAME => RED HEARTS FOR NUMBER OF HEALTH
      Serial.println("health print");
      for (uint16_t i = 0; i < 5; i++) {
        strip.setPixelColor(i, strip.Color(0,0,0));
      }
      
      for (uint16_t i = 0; i < health; i++) {
        strip.setPixelColor(i, strip.Color(50,0,0));
      }
    }
  }
  strip.show();
  
}
//==================================================================================================================================================================
