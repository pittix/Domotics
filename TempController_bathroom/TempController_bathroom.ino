/**
 * @author Andrea Pittaro
 * @date 2015/09/18
 * @comment This sketch, when ATtiny85 boots ask the RaspberryPi the temperature I set for my house bedroom and 
 * when asked by the RPI turns on or off. periodically it gives to rpi the temperature measured.
 */
 
#include <Wire.h> //transmission library
#include <math.h> //needed for (int) = round(float/decimal var);
#include <EEPROM.h> // store values into memory. I need 4 bytes, ATMega328p has 1024 bytes
#include <avr/sleep.h> // power save mode
#include <avr/wdt.h> // watchdog

#define WAIT 7 //cycles of 8 seconds
 
//pin relay
#define relay 0 
#define FORCE_MANUAL 1
#define TEMPERATURE 2

//Transmission spec
#define ARD_ID 0x50
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
#define ADDR_ID     11
#define ADDR_TEMP   12 // address where temperature is set
#define ADDR_STATUS 13 // address where the status is set (manual mode and cooling or heating is on)
#define ADDR_BATT   14 //address where the battery voltage is stored
volatile unsigned long counter = 0; //timer for sleeping
const long InternalReferenceVoltage = 1062; // power voltage. Needed for temperature 
const uint8_t battTolerance = 20; // 0.1 V tolerance
uint8_t chosenTemp = 190; // default temperature, in integer, one decimal
float curTemp;

//store info in only one var [inHome,manualMode,coolOn,heatOn,0,0,battery[2 bit] ] in 0 the one not used
uint8_t status = 0b10000000; //default: in home and manual mode
/*battery info are 
 * 00 for Extremely low (< 3V)
 * 01 for Low (3V<= batt < 3.3V)
 * 10 for Normal (3.3V <= batt < 4V)
 * 11 for High (batt >= 4V)
 */

bool forceManual=false;
uint8_t histTemp[5]; //last 5 temperatures, which corresponds to the last 5 minutes approximatly
//Initializes the ricetransmitter
void setup() {
    analogReference (INTERNAL);//Internal AREF
    EEPROM.put(ADDR_ID, ARD_ID);

    Wire.begin(ARD_ID); // the arduino is a slave. 
    TWAR = (ARD_ID << 1) | 1;  // enable broadcasts to be received
    Wire.onReceive(receiveData);
    Wire.onRequest(sendData);
    pinMode(relay,OUTPUT);
    pinMode(TEMPERATURE,OUTPUT);

    pinMode(FORCE_MANUAL,INPUT);
    EEPROM.update(ADDR_BATT, (uint8_t)250);  //for first iteration
    //re-initialize value if arduino is resetted
    
    chosenTemp = EEPROM.read(ADDR_TEMP);
    status = EEPROM.read(ADDR_STATUS);
}

void loop() {
   // sleep for about 56s, then measures the temperature level
  //sleep doesn't change the pin status (i.e. the relay stays HIGH or LOW while arduino is sleeping)
  if(++counter >= WAIT){
    //
    byte old_ADCSRA = ADCSRA;
    // disable ADC
    ADCSRA = 0;  
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);  
    sleep_enable();
    sleep_cpu();
    sleep_disable();
    ADCSRA = old_ADCSRA;      
    counter=0;
    // release TWI bus / I2C
    TWCR = bit(TWEN) | bit(TWIE) | bit(TWEA) | bit(TWINT);
    // turn it back on again
    Wire.begin(ARD_ID); 
    TWAR = (ARD_ID << 1) | 1;  // enable broadcasts to be received
    getBattery(); //every minute check the battery status
   }
  
  forceManual = (digitalRead(FORCE_MANUAL)==HIGH); //check if I set in failsafe mode
  //enable temperature sensor (gives current)
  uint8_t curTemp = getTemp();
  if((status & 0b01000000) || forceManual) { // only in manual mode controls the temperature
    if(10*curTemp<chosenTemp-30 )
       digitalWrite(relay,HIGH);
    if (10*curTemp>chosenTemp+30)
       digitalWrite(relay,LOW);
  }
  else { // automatic mode. set heat and cooling based on the info from the raspberry/ESP8266
   if((status & 0b00010000)>0) //heat on, put the relay pin on high
        digitalWrite(relay,HIGH);
   else
         digitalWrite(relay,LOW);
  }
  histTemp[4] = histTemp[3];histTemp[3] = histTemp[2];histTemp[2] = histTemp[1];
  histTemp[0]=curTemp; //store this temperature
    
  // sleep for about 56s, then measures the temperature level
  //sleep doesn't change the pin status (i.e. the relay stays HIGH or LOW while arduino is sleeping)
  for(uint8_t sleep= 0; sleep < 7; sleep++ ){
    //
    byte old_ADCSRA = ADCSRA;
    // disable ADC
    ADCSRA = 0;  
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);  
    sleep_enable();
    sleep_cpu();
    sleep_disable();
    ADCSRA = old_ADCSRA;      
        
    // release TWI bus / I2C
    TWCR = bit(TWEN) | bit(TWIE) | bit(TWEA) | bit(TWINT);
    
    // turn it back on again
    Wire.begin(ARD_ID); 
    TWAR = (ARD_ID << 1) | 1;  // enable broadcasts to be received

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
   uint8_t batt = (((InternalReferenceVoltage * 1024) / ADC) + 5) / 20; // range [0;250] the voltage is 20 times lower (e.g. 5V is 250 = 500/2) so about 2 digits are stored
   //encode the value
   uint8_t codedBatteryStatus;
   if(batt > 90) { codedBatteryStatus= 0b11; } //4.5V
   else if(batt > 66) { codedBatteryStatus= 0b10; } // 3.3V
   else if(batt > 60) { codedBatteryStatus= 0b1; }//3V
   else { codedBatteryStatus= 0b0; }
   //store the battery status
   uint8_t tmpStatus = EEPROM.read(ADDR_STATUS);
   if((tmpStatus & 0b11)!= codedBatteryStatus){ //update battery status
      tmpStatus = tmpStatus & 0b11111100; //reset battery info, keep other info
      status = tmpStatus | codedBatteryStatus; //update status variable
      EEPROM.update(ADDR_STATUS,status); // store it on eeprom if changed
      uint8_t battStored = EEPROM.read(ADDR_BATT);
      if (!(battStored > batt - battTolerance && battStored < batt - battTolerance )){
        EEPROM.update(ADDR_BATT, batt);
      }
   }
  } // end of getBandgap


