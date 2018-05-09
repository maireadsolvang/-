#include <SPI.h>
#include <U8g2lib.h>
#include "esp32-hal-ledc.h"
#define SCREEN U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI 
SCREEN oled(U8G2_R0, 5, 17,16);

int emitter_pin = 19; //19 //35
int receiver_pin = 34; 
int button = 2;
int button_reading;
int freq = 38000;
int ledChannel = 0;
int resolution = 8;
int receiving;

void setup() {
  Serial.begin(115200);
  pinMode(receiver_pin, INPUT_PULLUP);
  pinMode(button, INPUT_PULLUP);
  oled.begin();     // initialize the OLED
  oled_print("STARTING",15); 

  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(emitter_pin, ledChannel);

}

void loop() {
  button_reading = !digitalRead(button);
  receiving = !digitalRead(receiver_pin);
  Serial.println(receiving);

  if(button_reading and receiving){ 
    //oled_print("RECEIVING",30);
    oled_print("EMITTING",15);
    ledcWrite(ledChannel,200);
    }
  else if (receiving){
    oled_print("RECEIVING",30);
    ledcWrite(ledChannel,0);
    }
  else if (button_reading){
    oled_print("EMITTING",15);
    ledcWrite(ledChannel,200);
  }
  else{
    oled.clearBuffer();
    oled_print("WAITING",15);
    ledcWrite(ledChannel,0);
  }

}

void oled_print(String message,int start){
  oled.clearBuffer();    
  oled.setFont(u8g2_font_5x7_tf);   
  oled.setCursor(0,start); 
  oled.print(message); 
  oled.sendBuffer();
}

