#include <mpu9255_esp32.h>
#include <U8g2lib.h>
#include<math.h>
#include <WiFi.h>
#define SCREEN U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI

const int LOOP_SPEED = 50; //milliseconds
int primary_timer = 0;
float x,y,z; //variables for grabbing x,y,and z values
float accelMagnitude[3] = {0.0, 0.0, 0.0};
MPU9255 imu; //imu object called, appropriately, imu
SCREEN oled(U8G2_R0, 5, 17,16);
const int button_pin = 15;
const int response_timeout = 6000; //ms to wait for response from host
const int button_pin_2 = 2;
const int post_response_timeout = 3000;
int response_timer;

void setup() {
  Serial.begin(115200); //for debugging if needed.
  delay(50); //pause to make sure comms get set up
  Wire.begin();
  delay(50); //pause to make sure comms get set up
  setup_imu();
  primary_timer = millis();
  
//  WiFi.begin("MIT GUEST"); //attempt to connect to wifi
  WiFi.begin("6s08","iesc6s08"); 
  oled.begin();     // initialize the OLED
  oled.clearBuffer();    //clear the screen contents
  oled.setFont(u8g2_font_5x7_tf);  //set font on oled  
  oled.setCursor(0,15); //set cursor
  oled.print("Starting"); //print starting message
  oled.sendBuffer(); //send it (so it displays)
  int count = 0; //count used for Wifi check times
  while (WiFi.status() != WL_CONNECTED && count<6) {
    delay(500);
    //Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    //Serial.println(WiFi.localIP().toString() + " (" + WiFi.macAddress() + ") (" + WiFi.SSID() + ")");
    oled_print("All Connected");
    delay(500);
  } else { //if we failed to connect just ry again.
    //Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP
  }
  
  pinMode(button_pin,INPUT_PULLUP);
  pinMode(button_pin_2,INPUT_PULLUP);
}
bool pushedBefore = false;
int data_timer = 0;
String data = "";

void loop() {
  
  int reading = digitalRead(button_pin);
  int reading_2 = digitalRead(button_pin_2);
  if (!reading){
    if (!pushedBefore){
      oled_print("RECORDING");
      primary_timer =millis();
      data_timer = millis();
      data = "";
      pushedBefore = true;
    }
    imu.readAccelData(imu.accelCount);
    x = imu.accelCount[0]*imu.aRes;
    y = imu.accelCount[1]*imu.aRes;
    z = imu.accelCount[2]*imu.aRes;
    Serial.println(String(x));
    Serial.println("test");
    data += "Time:"+String(millis()-data_timer)+"Acc:"+String(x)+","+String(y)+","+String(z)+";";
    
    //Serial.println("Time:"+String(millis()-data_timer)+"Acc:"+String(x)+","+String(y)+","+String(z));
    while (millis()-primary_timer<LOOP_SPEED); //wait for primary timer to increment
    primary_timer =millis();
    response_timer = millis();
  }
  else{
    if (pushedBefore){
      if(!reading_2 && millis()-response_timer < post_response_timeout){
        oled_print("POSTING");
        do_POST(data);
        pushedBefore = false; 
      }
      else if (millis() - response_timer < post_response_timeout){
        oled_print("Want to post?");
      }
      else if (millis()-response_timer > post_response_timeout){
        pushedBefore = false;
        oled_print("DID NOT POST");
      }
    }
  }
  

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

void do_POST(String data){
  WiFiClient client; //instantiate a client object
  if (client.connect("iesc-s1.mit.edu", 80)) { //try to connect to class server
    // This will send the request to the server
    // If connected, fire off HTTP GET:
    int gestureID = 1;
    String thing = "username=mairead&data="+data+"&gestureID="+String(gestureID);
    client.println("POST /608dev/sandbox/smathew/final_project/server.py HTTP/1.1");
    client.println("Host: iesc-s1.mit.edu");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Content-Length: " + String(thing.length()));
    client.print("\r\n");
    client.print(thing);
    unsigned long count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      String line = client.readStringUntil('\n');
      //Serial.println(line);
      if (line == "\r") { //found a blank line!
        //headers have been received! (indicated by blank line)
        break;
      }
      if (millis()-count>response_timeout) break;
    }
    count = millis();
    String op; //create empty String object
    while (client.available()) { //read out remaining text (body of response)
      op+=(char)client.read();
    }
    oled_print(op.substring(0,25));
    //Serial.println(op);
    client.stop();
    //Serial.println();
    //Serial.println("-----------");
  }else{
    oled_print("Connection Failed");
    //Serial.println("connection failed");
    //Serial.println("wait 0.5 sec...");
    client.stop();
    delay(300);
  }
}        



