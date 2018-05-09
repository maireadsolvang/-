/***************************************************
DFPlayer - A Mini MP3 Player For Arduino
 <https://www.dfrobot.com/index.php?route=product/product&product_id=1121>
 
 ***************************************************
 This example shows the basic function of library for DFPlayer.
 
 Created 2016-12-07
 By [Angelo qiao](Angelo.qiao@dfrobot.com)
 
 GNU Lesser General Public License.
 See <http://www.gnu.org/licenses/> for details.
 All above must be included in any redistribution
 ****************************************************/

/***********Notice and Trouble shooting***************
 1.Connection and Diagram can be found here
 <https://www.dfrobot.com/wiki/index.php/DFPlayer_Mini_SKU:DFR0299#Connection_Diagram>
 2.This code is tested on Arduino Uno, Leonardo, Mega boards.
 ****************************************************/
#include <WiFi.h>
#include <mpu9255_esp32.h>
#include <compass.h>
#include <U8g2lib.h>
#include <adp5350.h>
#include <math.h>

#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"

int state=1;
int button_state;
int vol_but=2;
int vol_state=1;
int volume=20;
const int button_pin=15;
HardwareSerial mySerial(2); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);

void setup()
{
  // initialize button
  pinMode(button_pin,INPUT_PULLUP);
  pinMode(vol_but,INPUT_PULLUP);
  // Serial monitor
  Serial.begin(115200);
  
  // speaker module initailizaiton ///////////////////////////
  mySerial.begin(9600,SERIAL_8N1,32,33);
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  if (!myDFPlayer.begin(mySerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  Serial.println(F("DFPlayer Mini online."));
  myDFPlayer.volume(20);  //Set volume value. From 0 to 30
  //////////////////////////////////////////////////////////
  myDFPlayer.play(1);  //Play the first mp3
}

void loop()
{
  static unsigned long timer = millis();
  int button_state= digitalRead(button_pin);
  int vol_read=digitalRead(vol_but);
  if ((vol_state==1)&(vol_read==1)){
    vol_state=2;
  }
  if ((vol_state==2)&(vol_read==0)){
    volume=(volume+3)%30;
    myDFPlayer.volume(volume);
    vol_state=1;
  }
  
  Serial.println("state: "+String(state)+"   But: "+String(button_state));
  if ((state==1)&(button_state==1)){  /// pressed
    state=2;
    
  }
  else if ((state==2)&(button_state==0)){ //// let go.. changes song
    state=3;
    myDFPlayer.next();
  }
  else if ((state==3)&(button_state==1)){ ////pressed
    state=4;
  }
  else if ((state==4)&(button_state==0)){// let go.. turns off
    state=1;
    myDFPlayer.pause();
  }
  
  if (myDFPlayer.available()) {
    printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }
}

void printDetail(uint8_t type, int value){
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
