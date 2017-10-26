/**
 * Script for general arduinos. It has light control feature (which turns on and off lights if there's movement in a room) and register the room temperature.
 * In this way the main controller can know the exact temperature and what to do (e.g. turn on or off the heat/cooling)
 * The sketch exploit the sleeping to reduce its power consumption (though it can be powered by the power grid)
 */
// PIN
#define LDR A0 //light sesor
#define PIR 0 //movement ir sensor
#define RELAY 1
#define sensorT A2 //temp sensor

//pin relay
#define relay 0 
#define FORCE_MANUAL 1
#define TEMPERATURE  2


//Transmission spec
#define ARD_ID 0x50
//Message parameters
#define IN_HOME   0b10000000
#define STATUS    0b01100000
#define AUTO_OFF  0b00100000
#define HEAT_COOL 0b01000000
#define TEMP_MASK 0b00001111
#define ZERO_TEMP 150

#define CONV_TEMP 0.48828125 //LM35 converting from Analog input to celsius degrees
#define SET_TEMP 0b01000000 
#define GET_IS_OUT 0b11000000

#include <Wire.h> //transmission library
#include <math.h> //needed for (int) = round(float/decimal var);
#include <EEPROM.h> // store values into memory. I need 4 bytes, ATMega328p has 1024 bytes
#include <avr/sleep.h> // power save mode
#include <avr/wdt.h> // watchdog

#define WAIT 7 //cycles of 8 seconds

//EEPROM addresses
#define ADDR_ID     11
#define ADDR_TEMP   12 // address where temperature is set
#define ADDR_STATUS 13 // address where the status is set (manual mode and cooling or heating is on)
#define ADDR_BATT   14
bool isOut;
uint8_t status;
uint8_t histTemp[5];
bool forceManual=false;
const long InternalReferenceVoltage = 1062; // power voltage. Needed for temperature 

void setup() {
  analogReference (INTERNAL);//Internal AREF
  Wire.begin(ARD_ID); // the arduino is a slave. 
  TWAR = (ARD_ID << 1) | 1;  // enable broadcasts to be received
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);
  pinMode(PIR,INPUT);
  pinMode (RELAY,OUTPUT);
//  pinMode(reset,OUTPUT); // if needed to do an update via I2C
//  
}
void loop() {
   
   int light = analogRead(LDR);
   if(!isOut){
      if(digitalRead(PIR)==HIGH){
          if(light<150){
                digitalWrite(RELAY,HIGH);
            }
        }
      else {
        digitalWrite(RELAY,LOW);
      }
    
   }
   else {
     digitalWrite(RELAY,LOW);
   }
  

}

void receiveData(int numB){
  byte data = Wire.read(); // 1 byte data (0,0,0,0,0, in home/out[1], status[2])
  uint8_t temperature = data & TEMP_MASK; // extract temperature
  status = data & !TEMP_MASK; //the other 4 bits are for the status
   
}
//send data on ESP8266 requests. Stupid version, need optimization
void sendData(){
  uint8_t data[6];
  data[0] = histTemp[0];data[1] = histTemp[1];data[2] = histTemp[2];
  data[3] = histTemp[3];data[4] = histTemp[4];
  data[5] = status;
  Wire.write(data,6);
}
/**
 * Get the temperature of the room and returns the value
 */
uint8_t getTemp(){
  digitalWrite(TEMPERATURE,HIGH);
  float sumTemp=0;
  uint8_t tmpCurTemp=0;
  uint8_t curTemp=0;
  float conversionFactor = float(EEPROM.read(ADDR_BATT)); //TODO current conversion factor. It depends on the battery level as there's no external reference
  for(byte i=0; i<10;i++){ //The value is more precise
    tmpCurTemp=(analogRead(sensorT)*conversionFactor); //need other operation to get the temp value from a value which goes from 0 to 1024
    sumTemp += tmpCurTemp;
    delay(5);
  }
  digitalWrite(TEMPERATURE,LOW);  //stops giving current to the temperature sensor
  //1 second lasted while acquiring data
  curTemp=sumTemp/10;
  return curTemp;
}
/* Code courtesy of "Coding Badly" and "Retrolefty" from the Arduino forum
 * results are Vcc * 10
 * So for example, 5V would be 50.
 * - Exploits temp results, saves results into "status" var and in EEPROM
 */
void getBattery () 
  {
  // REFS0 : Selects AVcc external reference
  // MUX3 MUX2 MUX1 : Selects 1.1V (VBG)  
   ADMUX = bit (REFS0) | bit (MUX3) | bit (MUX2) | bit (MUX1);
   ADCSRA |= bit( ADSC );  // start conversion
   while (ADCSRA & bit (ADSC))
     { }  // wait for conversion to complete
   uint8_t batt = (((InternalReferenceVoltage * 1024) / ADC) + 5) / 100; // range [0;50] 
   //encode the value
   uint8_t codedBatteryStatus;
   if(batt > 45) { codedBatteryStatus= 0b11; }
   else if(batt > 33) { codedBatteryStatus= 0b10; }
   else if(batt > 30) { codedBatteryStatus= 0b1; }
   else { codedBatteryStatus= 0b0; }
   //store the battery status
   uint8_t tmpStatus = EEPROM.read(ADDR_STATUS);
   if((tmpStatus & 0b11)!= codedBatteryStatus){ //update battery status
      tmpStatus = tmpStatus & 0b11111100; //reset battery info, keep other info
      status = tmpStatus | codedBatteryStatus; //update status variable
      EEPROM.update(ADDR_STATUS,status); // store it on eeprom if changed
   }
  } // end of getBandgap


