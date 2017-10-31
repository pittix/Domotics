/**
 * Acquire and reports temperature 1 sample per minute and then every 5 minutes reports it to the ESP8266
 */

//

// //Message parameters
// #define TO_ME 0b00000001
// #define ID 0b00000010 //ID of the sensor, IT MUST CHANGE FOR EACH DEVICE!!!
// #define RASP_HEADER 0b00000100 //Logic shift left 1 position of id
// #define OK_ANSWER 0b10101010
// #define COMMAND_BIT  0b11000000
// #define GET_TEMP_BIT 0b00111111
// #define ASK_TEMP 0b00000000
// #define ERROR_MSG 0b01010101
// #define SET_TEMP 0b01000000

//pinout parameters
#define LED_PIN 2
#define TEMP_PIN 3

//Temperature parameters
#define SENSOR_PIN A0
#define CONV_TEMP 0.48828125 //LM35 converting from Analog input to celsius degrees
#include<Wire.h>
#include<EEPROM.h>
#include <math.h> //needed for (int) = round(float/decimal var);

// EEPROM address where data is stored
#define ADDR_CONF 0
#define ADDR_STATUS 1
//10 bytes: ADDR 2-11 are reserved. uint16_t for temperature with 1 decimal sign. value 0 means -20 Celsius . I just need 10 bit, so 6bit per acquisition are wasted. NEED IMPROVEMENT TODO
#define ADDR_STORE_TMP 2 //

PROGMEM const uint8_t ADDR_I2C = 0x11;
uint16_t histTemp[5];
uint8_t status;
uint8_t config;

float curTemp=0;

void setup() {
  pinMode(LED_PIN,OUTPUT);
  pinMode(TEMP_PIN,OUTPUT);
  loadVars();
  Wire.begin(ADDR_I2C);
  Wire.onReceive(receiveConfig);
  Wire.onRequest(sendTemp);
}

void loadVars(){
  status = EEPROM.read(ADDR_STATUS);
  config = EEPROM.read(ADDR_CONF)

  for(uint8_t i=0;i<TEMP_N;i++){
    histTemp[i] = (EEPROM.read(ADDR_STORE_TMP+2*i)) | (EEPROM.read(ADDR_STORE_TMP+2*i+1));
  }

}

void loop() {
  float sumTemp=0;
  for(byte i=0; i<20;i++){ //The value is more precise
    curTemp=(analogRead(sensorT)*CONV_TEMP); //need other operation to get the temp value from a value which goes from 0 to 1024
    sumTemp += curTemp;
    delay(50);
    }
   curTemp=sumTemp/20;
   else delay (10000);//do the loop() every 10 seconds
}

void receiveConfig(){

}
void sendData(int numBytes){
  uint16_t data[6];
  data[0] = histTemp[0];data[1] = histTemp[1];data[2] = histTemp[2];
  data[3] = histTemp[3];data[4] = histTemp[4];
  data[5] = status;
  Wire.write(data,6);
}
void blinkError(){
  while(true){
    digitalWrite(LED_PIN,HIGH);
    delay(50);//human eye can see a flash so quick. the rest of the time the sensor is sleeping
    digitalWrite(LED_PIN,LOW);
    delay(500);
    }
  }
