/**
 * ESP8266 code. The chip need to do:
 * 
 * [list copied from other PC]
 * 
 * Uses https://github.com/bblanchon/ArduinoJson to parse JSON from and to the raspberry
 * 
 * 
 */

// I2C addresses of the arduinos connected
#define ARD_SMALL_BATH 0x10
#define ARD_BIG_BATH 0x11
#define ARD_PARENT 0x12
#define ARD_CHILD 0x13
#define ARD_OFFICE 0x14
#define ARD_LIVING 0x15
#define ARD_ENTRANCE 0x16
#define ARD_KITCHEN 0x17

#define TRANSMISSION_OK 4
//Arduinos data constants
#define ARD_N 5 //number of arduinos connected to ESP
#define ARD_DATA_LEN 6 //byte

#include "ArduinoJson.h"
#define ARDUINOJSON_ENABLE_PROGMEM 1 //fix problem

#include <Wire.h>
#include<ESP8266WiFi.h>


//arduino data variable
uint8_t ardData[ARD_N][ARD_DATA_LEN];

char* ssid = "SSID";
char* pwd = "PASSWD";
uint8_t ip[4] = {192,168,1,10};
uint8_t gw[4] = {192,168,1,1};
uint8_t netmask[4] = {255,255,255,0};

//JSON library
StaticJsonBuffer<400> jsonBuffer;


void setup() {
  //saves data from the arduino
  for(uint8_t i=1;i<ARD_N;i++){
//    ardData[i] = receiveArduinoData()  ;
    delay(100);// 100 ms between one request and the other
  }
  connectWifi();
  String JsonData = requestControllerData();
  WiFi.mode(WIFI_OFF);
  decodeJSON(JsonData);
  setSleep();
  
}

/* Useless as the controller sleeps almost all the time and to wake it, I need to reset it. 
 *  All the code is inside the setup() and the functions called inside
 */
void loop() {}  

String requestControllerData(){return "";}

//uint8_t[][6] decodeJSON(String in){
//  uint8_t out[ARD_N][ARD_DATA_LEN];
//  return out;
//}

uint8_t decodeJSON(String JsonData){
    JsonObject* data = jsonBuffer.parse(JsonData);
    return 0;
}
/**
 * encode the data to be raspberry-readable
 * return the string with the json data
 */
String encodeJSON(int data[]){
  String out;
  JsonObject* parser = &jsonBuffer.createObject();
  JsonObject* ardParser = &jsonBuffer.createObject();
  
  for(uint8_t i=1;i<=ARD_N;i++){
    for(uint8_t j=1;j<ARD_DATA_LEN;j++){ 
      (*ardParser)[String(j)] = (char*)ardData[i][j]; // convert each temp into string. the temp number is the acquisition number
    }
    (*ardParser)["status"] = (char)ardData[i][5]; // status
    String tmp;
    ardParser->printTo(tmp); //nested JSON.
    (*parser)[String(i)]= tmp;
    ardParser = &jsonBuffer.createObject(); // reset the parser to accept new values 
  }
   parser->printTo(out); // creates the string with all the data
   return out;
}

/**
 * Send data to the arduino at address addr. Makes sure that the data was correctly received
 */
void sendArduinoData(uint8_t data , uint8_t addr){
  Wire.beginTransmission(0);  //wake the arduinos if they're asleep
  Wire.endTransmission();
  delay(100);//wait for the wakeup. 100ms should be sufficient
  bool goodTx=false;
  uint8_t tmp;
  while(!goodTx){ // retransmit until it's ok
    Wire.beginTransmission(addr);
    tmp = Wire.write(data);
    goodTx = (tmp == TRANSMISSION_OK);// true if the transmission was good, false otherwise
    Wire.endTransmission(); 
  }
}
/**
 * Query the arduino to get the temperature data and status. This is the default out data
 */
uint8_t* receiveArduinoData(uint8_t addr){
  Wire.beginTransmission(0);
  Wire.endTransmission();
  Wire.beginTransmission(addr);
  uint8_t data[ARD_DATA_LEN+3]; //just in case
  uint8_t i=0;
  uint8_t out[ARD_DATA_LEN];
  while(Wire.available()>0){
      data[i] = Wire.read();
      i++;
  }
  if(data[ARD_DATA_LEN]|data[ARD_DATA_LEN+1]|data[ARD_DATA_LEN+2] ==0){
    for(uint8_t i =0;i<ARD_DATA_LEN;i++){
      out[i] = data[i];
    }
    return out;
  }
  else{
    return data;
  }
}
/**
 * connect to wifi and then return. Use a static IP address
 */
void connectWifi(){
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.config(ip,gw,netmask);
  WiFi.begin(ssid,pwd);
  while (WiFi.status() != WL_CONNECTED) { delay(200);} //wait before returning

}
/**
 * Put the ESP to sleep for a fixed amount of time and 
 */
void setSleep(){}
