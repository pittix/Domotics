/**
 * @author Andrea Pittaro
 * @date 2015/09/18
 * @comment This sketch, when ATtiny85 boots ask the RaspberryPi the temperature I set for my house bedroom and 
 * when asked by the RPI turns on or off. periodically it gives to rpi the temperature measured.
 */
//pin relay
#define relay 0 
#define FORCE_MANUAL 1
#define TEMPERATURE 2
//Transmission spec
#define ARD_ID 0x01
//Message parameters
#define IN_HOME   0b10000000
#define STATUS    0b01100000
#define AUTO_OFF  0b00100000
#define HEAT_COOL 0b01000000
#define TEMP_MASK 0b00001111
#define ZERO_TEMP 150
//Temperature parameters
#define sensorT A0
#define CONV_TEMP 0.48828125 //LM35 converting from Analog input to celsius degrees

//EEPROM addresses
#define ADDR_TEMP   1 // address where temperature is set
#define ADDR_STATUS 2 // address where the status is set (manual mode and cooling or heating is on)

uint8_t chosenTemp = 190; // default temperature, in integer, one decimal
float curTemp;
//store info in only one var [inHome,manualMode,coolOn,heatOn,0,0,0,0] in 0 the one not used
uint8_t status = 0b10000000; //default: in home and manual mode
bool forceManual=false;
uint8_t countIter = 0;
uint8_t histTemp[5]; //last 5 temperatures, which corresponds to the last 5 minutes approximatly
#include <Wire.h> //transmission library
#include <math.h> //needed for (int) = round(float/decimal var);
#include <EEPROM.h> // store values into memory. I need 4 bytes, ATMega328p has 1024 bytes
//Initializes the ricetransmitter
void setup() {
    Wire.begin(ARD_ID); // the arduino is a slave. 
    Wire.onReceive(receiveData);
    Wire.onRequest(sendData);
    pinMode(relay,OUTPUT);
    pinMode(TEMPERATURE,OUTPUT);

    pinMode(FORCE_MANUAL,INPUT);
    //re-initialize value if arduino is resetted
    chosenTemp = EEPROM.read(ADDR_TEMP);
    status = EEPROM.read(ADDR_STATUS);
}

void loop() {
  forceManual = (digitalRead(FORCE_MANUAL)==HIGH); //check if I set in failsafe mode
  //enable temperature sensor (gives current)
  digitalWrite(TEMPERATURE,HIGH);
  float sumTemp=0;
  uint8_t tmpCurTemp=0;
  for(byte i=0; i<10;i++){ //The value is more precise
    tmpCurTemp=(analogRead(sensorT)*CONV_TEMP); //need other operation to get the temp value from a value which goes from 0 to 1024
    sumTemp += tmpCurTemp;
    delay(5);
  }
  digitalWrite(TEMPERATURE,LOW);  //stops giving current to the temperature sensor
  //1 second lasted while acquiring data
  curTemp=sumTemp/10;
  if((status & 0b01000000) || forceManual) { // only in manual mode controls the temperature
    if(10*curTemp<chosenTemp-30 )
       digitalWrite(relay,HIGH);
    if (10*curTemp>chosenTemp+30)
       digitalWrite(relay,LOW);
  }
  else { // automatic mode. set heat and cooling based on the info from the raspberry/ESP8266
   if(status & 0b00010000>0) //heat on, put the relay pin on high
        digitalWrite(relay,HIGH);
   else
         digitalWrite(relay,LOW);
  }
   if(countIter == 50){
    histTemp[4] = histTemp[3];histTemp[3] = histTemp[2];histTemp[2] = histTemp[1];
    histTemp[0]=curTemp; //store this temperature
    countIter=0;
   }
   else{
    //
    countIter++; 
   }
   EEPROM.put(ADDR_TEMP, chosenTemp); // update EEPROM, if changed, with the new chosen temperature 
   EEPROM.put(ADDR_STATUS, status);
   
}
/*receive the data from the ESP8266 in the standard mode {inHome,manual,heatOn,coolOn,setTemp[4 bit]}
 * Temperature is given in the range [15;31]°C, which is sufficient to set the heat and cooling system
 */
void receiveData(int numB){
  byte data = Wire.read(); // 1 byte data (tempBit[5], in home/out[1], status[2])
  uint8_t temperature = data & TEMP_MASK; // extract temperature
  chosenTemp = temperature*10 + ZERO_TEMP; // transmit less data in coding the temperature
  status = data & !TEMP_MASK; //the other 4 bits are for the status
   
}
//send data on ESP8266 requests. Stupid version, need optimization
void sendData(int numB){
  uint8_t data[6];
  data[0] = histTemp[0];data[1] = histTemp[1];data[2] = histTemp[2];
  data[3] = histTemp[3];data[4] = histTemp[4];data[5] = status;
  Wire.write(data,6);
}


