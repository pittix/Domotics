/**
   ESP8266 code.

   This sketch allows to connect, via WiFi, to the Raspberry pi 1B and via I2C to the ATMega328p.
   The code allows the controller to
    - check the temperature and store it
    - store information for other arduinos
    - allow to update the connected ATMega via I2C [probably]
    - allow OTA updates via the Raspberry
    - send the information stored to the raspberry
    - sleep the rest of the time

   The SPIFFS filesystem is organized as follows:
   - /down/<arduino addr>.bin           the firmware to be uploaded to an arduino
   - /down/ota.bin                      the firmware for this ESP8266
   - /temp/<arduino addr>.temp      the temperature file for each arduino
   - /stat/<arduino addr>.st            the status file of each arduino
   - /conf/<arduino addr>.cfg               the config assigned by the controller to each arduino
   - /conf/addresses.cfg                    the I2C addresses of each arduino, one per line
   - /wifi/wpa.txt                          the credentials of the wifi. SSID is first row, pwd is second. Formatted as string
   - /wifi/ip.bin                           the ip addr, gateway and netmask. bytes[0:3] are ip, [4:7] are gw and [7:10] are the netmask

   Uses:
    - https://github.com/bblanchon/ArduinoJson to parse JSON from and to the raspberry
    - Wire to communicate to the arduinos attached
    - ESP8266WiFi to communicate via WiFi
    -


*/

// I2C addresses of the arduinos connected. Not used as addresses are stored in spiffs
//#define ARD_SMALL_BATH 0x10
//#define ARD_BIG_BATH 0x11
//#define ARD_PARENT 0x12
//#define ARD_CHILD 0x13
//#define ARD_OFFICE 0x14
//#define ARD_LIVING 0x15
//#define ARD_ENTRANCE 0x16
//#define ARD_KITCHEN 0x17

#define TRANSMISSION_OK 0
//Arduinos data constants
#define ARD_N 5 //number of arduinos connected to ESP
#define ARD_DATA_LEN 6 //byte

//sleep time (in micro seconds)
#define SLEEP_TIME 300e6 // 5 minutes sleep. Duty cycle should be around 1%

#include "ArduinoJson.h"
//#define ARDUINOJSON_ENABLE_PROGMEM 1 //fix problem

#include <Wire.h>
#include<ESP8266WiFi.h>
#include <FS.h> // write into the 3M SPIFFS
#include <ArduinoOTA.h>

//#include <avr/pgmspace.h> //
//arduino data variable
uint8_t ARD_ADDRESSES[ARD_N];// =  { ARD_SMALL_BATH, ARD_BIG_BATH, ARD_PARENT,
//ARD_CHILD,      ARD_OFFICE};
const char *dataPath = "/data/";
//WiFi connection parameters
char* ssid;
char* pwd;
char* myJsonD; // the path where to retrieve the file
char* myJsonU; // the path where to upload the file
uint8_t ip[4];// = {192,168,1,10};
uint8_t gw[4];// = {192,168,1,1};
uint8_t netmask[4];// = {255,255,255,0};

WiFiClient client;
//SPIFFS memory (3MB max) allocation files. strings are saved into flash, not in ram
PROGMEM const char* fwPath = "/down/";
PROGMEM const char* wifiPath = "/wifi/wpa.txt";
PROGMEM const char* ipPath = "/wifi/ip.bin";
PROGMEM const char* confPath = "/conf/";
PROGMEM const char* jsonPath = "/conf/json.cfg";
PROGMEM const char* addrPath = "/conf/addresses.cfg";
PROGMEM const char* statusPath = "/stat/";
PROGMEM const char* tempPath = "/temp/";
PROGMEM const char* upPath = "/up/toRasp.json";
PROGMEM const char* tmpJSON = "/tmp.json";
PROGMEM const char* tempExt = ".temp";
PROGMEM const char* statusExt = ".st";


//ftp strings
PROGMEM const char* FTP_RC = "RETR ";
PROGMEM const char* FTP_TX = "STOR ";
PROGMEM const char* FTP_QUIT = "QUIT";


void setup() {
  loadVars();
  //saves data from the arduino
  for (uint8_t i = 1; i < ARD_N; i++) {
    //    ardData[i] = receiveArduinoData()  ;
    delay(100);// 100 ms between one request and the other
  }
  connectWifi();
  encodeJSON();
  requestControllerData();
  WiFi.mode(WIFI_OFF);
  decodeJSON();
  setSleep();

}
/**
   Load variables stored inside the memory
*/
void loadVars() {
  SPIFFS.begin(); //mount fs
  //connection parameters
  File ipF = SPIFFS.open(ipPath, "r");
  for (uint8_t i = 0; i < 12; i++) {
    if (i < 4) {
      ip[i] = ipF.read();
    }
    else if (i < 8) {
      gw[i - 4] = ipF.read();
    }
    else {
      netmask[i - 8] = ipF.read();
    }
  }
  ipF.close();

  File wpaF = SPIFFS.open(wifiPath, "r");
  String tmp = wpaF.readStringUntil('\n');
  uint8_t tmpL = tmp.length();
  tmp.toCharArray(ssid, tmpL);
  tmp  = wpaF.readStringUntil('\n');
  tmpL = tmp.length();
  tmp.toCharArray(pwd, tmpL);
  wpaF.close();

  //arduino addresses on I2C
  File addrF = SPIFFS.open(addrPath, "r");
  for (uint8_t i = 0; i < ARD_N; i++) {
    ARD_ADDRESSES[i] = addrF.read();
  }
  addrF.close();

  //JSON filename
  File jsonF = SPIFFS.open(jsonPath, "r");
  tmp = jsonF.readStringUntil('\n');
  tmpL = tmp.length();
  tmp.toCharArray(myJsonD, tmpL);

  tmp = jsonF.readStringUntil('\n');
  tmpL = tmp.length();
  tmp.toCharArray(myJsonU, tmpL);
  jsonF.close();
}

void requestControllerData() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWifi(); // reconnect
  }

  File json = SPIFFS.open(tmpJSON, "w");//temp file
  client.connect(gw, 21); //raspberry is the gatewayù
  //retrieve json file
  client.print(FTP_RC);
  client.println(myJsonD);
  client.print(FTP_TX);
  client.println(myJsonU);
  //end connection
  client.println(FTP_QUIT);
  //  client.close();
}
/**
   The JSON is structured as follows
   - 1 value per each arduino containing the settings to send to each arduino
   - 1 url per each arduino where to find the code to upload. if the url is empty, no update is available
   - 1 url where to find the OTA for the ESP. If the url is empty, no value is given
*/
uint8_t decodeJSON() {
  File json = SPIFFS.open(tmpJSON, "r");//temp file
  String tmp = json.readString();
  //    const uint16_t bufSize = tmp.length()*2; //double the buffer
  StaticJsonBuffer<800> jsonBuffer;

  JsonObject &data = jsonBuffer.parseObject(tmp);
  if (!data.success())
  {
    // Parsing fail TODO

  }
  uint8_t ardSetup[data.size()];
  /**
     Update the configuration of each arduino
  */
  for (uint8_t i = 0; i < ARD_N; i++) {
    ardSetup[i] = data[String(i) + "c"];
    if (data[String(i) + "u"] != "") {

    }

  }
  return 0;
}
/**
   encode the data to be raspberry-readable
   return the string with the json data
*/
void encodeJSON() {
  String out;
  StaticJsonBuffer<800> jsonBuffer;
  JsonObject* parser = &jsonBuffer.createObject();
  JsonObject* ardParser;

  File json;
  for (uint8_t k = 0; k < ARD_N; k++) {
    json = SPIFFS.open(tempPath + String(ARD_ADDRESSES[k], HEX) + tempExt, "r"); //temp file for arduino k
    ardParser  = &jsonBuffer.createObject();
    uint8_t s = json.size();//file size
    for (uint8_t i = 1; i <= s; i++) {
      ardParser->set(String(i), String(json.read(), DEC));
    }
    String tmp;
    ardParser->printTo(tmp);
    parser->set(String(k), tmp.c_str());
    json.close();
  }

  parser->printTo(out); // creates the string with all the data
  //stores the data
  if (SPIFFS.exists(upPath)) {
    SPIFFS.remove(upPath);
  }
  File jsonFile = SPIFFS.open(upPath, "w");
  jsonFile.write((int8_t)*out.c_str());
  jsonFile.close();
}


/**
   Send data to the arduino at address addr. Makes sure that the data was correctly received
*/
void sendArduinoData(uint8_t data , uint8_t addr) {
  Wire.beginTransmission(0);  //wake the arduinos if they're asleep
  Wire.endTransmission();
  delay(100);//wait for the wakeup. 100ms should be sufficient
  bool goodTx = false;
  uint8_t tmp;
  while (!goodTx) { // retransmit until it's ok
    Wire.beginTransmission(addr);
    tmp = Wire.write(data);
    goodTx = (tmp == TRANSMISSION_OK);// true if the transmission was good, false otherwise
    Wire.endTransmission();
  }
}
/**
   Query the arduino to get the temperature data and status. This is the default out data
*/
uint8_t* receiveArduinoData(uint8_t addr) {
  Wire.beginTransmission(0);
  Wire.endTransmission();
  Wire.requestFrom(addr, ARD_DATA_LEN);
  uint8_t data[ARD_DATA_LEN];
  uint8_t i = 0;

  while (Wire.available() > 0) {
    data[i] = Wire.read();
    i++;
  }

  //STORE INTO MEMORY
  String filename = tempPath + String(addr, HEX) + tempExt;
  if (SPIFFS.exists(filename)) {
    SPIFFS.remove(filename);
  }
  File temp = SPIFFS.open(filename, "w");
  for (uint8_t i = 0; i < ARD_DATA_LEN - 1; i++) {
    temp.write(data[i]); //write 1 byte
  }
  temp.close();

  filename = statusPath + String(addr, HEX) + statusExt;
  if (SPIFFS.exists(filename)) {
    SPIFFS.remove(filename);
  }
  File st = SPIFFS.open(filename, "w");
  st.write(data[ARD_DATA_LEN - 1]);
  st.close();
  return 0;
}
/**
   connect to wifi and then return. Use a static IP address
*/
void connectWifi() {
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gw, netmask);
  WiFi.begin(ssid, pwd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200); //wait before returning
  }

}
/**
   Put the ESP to sleep for a fixed amount of time and then wake up as just started
*/
void setSleep() {
  ESP.deepSleep(SLEEP_TIME, WAKE_RF_DEFAULT);
}

/**
   download and store in memory the arduino firmware update file so that it will be flashed later
*/
void storeArduinoUpdate(uint8_t addr) {
  // SPIFFS.open(path + String(addr),w);


}

/* Useless as the controller sleeps almost all the time and to wake it, I need to reset it.
    All the code is inside the setup() and the functions called inside
*/
void loop() {}
