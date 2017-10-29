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
   - /conf/cypher.cfg                       the aes IV_key, data_key, the static IV  and the SHA passphrase

   Uses:
    - Wire to communicate to the arduinos attached
    - ESP8266WiFi to communicate via WiFi
    - mqtt library to communicate data to raspberry
    - aes library to encrypt data


*/


#define TRANSMISSION_OK 0
//Arduinos data constants
#define ARD_N 5 //number of arduinos connected to ESP
#define ARD_DATA_LEN uint8_t(6) //byte

//sleep time (in micro seconds)
#define SLEEP_TIME 300e6 // 5 minutes sleep. Duty cycle should be around 1%


#include <Wire.h>
#include<ESP8266WiFi.h>
#include <FS.h> // write into the 3M SPIFFS
#include <ArduinoOTA.h>
#include <PubSubClient.h>

//arduino data variable
uint8_t* ARD_ADDRESSES;
uint8_t* ARD_SN;
uint8_t ardN;

//WiFi connection parameters
char* ssid;
char* pwd;
char* mqttTopic;
char* mqttServer;
int mqttPort;
char* mqttSubs;


uint8_t ip[4];// = {192,168,1,10};
uint8_t gw[4];// = {192,168,1,1};
uint8_t netmask[4];// = {255,255,255,0};

WiFiClient espClient;
PubSubClient mqttCli(espClient);
//SPIFFS memory (3MB max) allocation files. strings are saved into flash, not in ram
PROGMEM const char* fwPath = "/down/";
PROGMEM const char* wifiPath = "/wifi/wpa.txt";
PROGMEM const char* ipPath = "/wifi/ip.bin";
PROGMEM const char* confPath = "/conf/";
PROGMEM const char* mqttPath = "/conf/mqtt.cfg";
PROGMEM const char* addrPath = "/conf/addresses.cfg";
PROGMEM const char* cypherPath = "/conf/cypher.cfg";
PROGMEM const char* statusPath = "/stat/";
PROGMEM const char* tempPath = "/temp/";
PROGMEM const char* upPath = "/up/toRasp.json";
PROGMEM const char* tempExt = ".temp";
PROGMEM const char* cfgExt = ".cfg";
PROGMEM const char* statusExt = ".st";
PROGMEM const char *dataPath = "/data/";



void setup() {
  loadVars(); // needed as the esp is resettes after each sleep
  //saves data from the arduino
  for (uint8_t i = 1; i < ARD_N; i++) {
    //    ardData[i] = receiveArduinoData()  ;
    delay(100);// 100 ms between one request and the other
  }
  connectWifi();
  sendControllerData();
  WiFi.mode(WIFI_OFF);
//  decodeJSON();
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
    ARD_ADDRESSES[i] = (uint8_t)addrF.readStringUntil(' ').toInt();
    ARD_SN[i] = (uint8_t) addrF.readStringUntil('\n').toInt();
  }
  addrF.close();

  //MQTT filename
  File mqttF = SPIFFS.open(mqttPath, "r");
  //extract mqtt publish topic, broker ip and subscription topic
  mqttTopic = (char*)mqttF.readStringUntil('\n').c_str();
  mqttServer = (char*)mqttF.readStringUntil(' ').c_str();
  mqttPort = (int)mqttF.readStringUntil('\n').c_str();
  mqttSubs = (char*)mqttF.readStringUntil('\n').c_str();
  mqttF.close();
}

void sendControllerData() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWifi(); // reconnect
  }
  //read data from SPIFFS and start encoding it
  uint8_t tmpAddr;
  String filename;
  File temperatureF;
  File addrF=SPIFFS.open(addrPath,"r");//use the serial number to identify the arduino
  String data;
  
  for(uint8_t i=0; i<ardN;i++){
    
    tmpAddr=(uint8_t)strtol(addrF.readStringUntil(' ').c_str(),NULL,HEX); //I2C address needed for filename
    data =data + addrF.readStringUntil('\n') + ':'; // [serial number]:[temperatures]\n
    filename=tempPath + String(tmpAddr,HEX) + tempExt;
    temperatureF=SPIFFS.open(filename,"r");
    data+=temperatureF.read();
    for(uint8_t j=1;j<ARD_DATA_LEN;i++){
      data= data + String(",") + String((uint8_t)temperatureF.read(),DEC);
    }
    data +='\n';
    temperatureF.close();//close current temperature file
  }
  addrF.close(); //close addresses file

  
  //encrypt data with aes
  char* encData=encryptTX((char*)data.c_str());

  //send data as a string of hex values. data is already encrypted
  mqttCli.publish(mqttTopic, encData, true);

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
   connect to wifi, then connects to mqtt broker and then return. Use a static IP address
*/
void connectWifi() {
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gw, netmask);
  WiFi.begin(ssid, pwd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200); //wait before returning
  }
  // connect to the server and set callback function
  mqttCli.setServer(mqttServer,mqttPort);
  mqttCli.setCallback(receive_mqtt_data);
  // subscribe to its topic
  mqttCli.subscribe(mqttSubs);
  
}
void receive_mqtt_data(char* topic, byte* payload, unsigned int length){
  if(topic == mqttTopic){
    char* payl ;
    File ardConf;
    memcpy(payl,payload,sizeof(char)*length);
    char* decData = decryptRC(payl);
    // String Parsing
    char line[30];
    uint8_t t=0;
    uint8_t i=0;
    while(decData[i+t] != '\0')
    {
      //copy each char until end of the line is reached
      while(decData[i+t]!='\n'){
        line[i]=decData[i+t];
        i++;
      }
      t+=i;
      store_mqtt_data(String(line));  
      i=0;  
    }
    
    
  }
}
/**
 * From each line, extract the config of each arduino and store it
 * @param line a sequence of char ending with '\n' in the form of [SN]:[byteConf]\n
 * @return true if decoding was successful, false otherwise
 */
boolean store_mqtt_data(String line){
 int len1 = line.indexOf(":");
 uint8_t* tmpArd=(uint8_t*)line.substring(0,len1).toInt();
 String filename;
  for(uint8_t i =0; i<ARD_N;i++){
    //find the I2C address of the arduino
    if(*tmpArd == ARD_SN[i]){
      filename=confPath + String(ARD_ADDRESSES[i],HEX) + cfgExt;
      if(SPIFFS.exists(filename)){
        SPIFFS.remove(filename);
      }
      File ardConf=SPIFFS.open(filename,"w");
      uint8_t* confData = (uint8_t*) line.substring(len1+1).toInt();
      ardConf.write(confData,sizeof(confData));
      ardConf.close();
      return true;
    }
  }
  return false;
}
/**
 * Encrypt data with AES-CBC. 
 * @param data the data to be encrypted
 * @return encData the AES encrypted data
 */
char* encryptTX(char* data){

   //TODO when AES library is OK

  return data;
}
/**
 * Decrypt received data with AES-CBC. 
 * @param data the data to be decrypted
 * @return decData plain data
 */
char* decryptRC(char* data){
  //TODO when AES library is OK
  return data;
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
void storeUpdate() {

//TODO
}
/**
 * Find the arduinos by serial number and stores them in the address file
 * This allows the program to identify the arduino by its serial number
 */
void connectedArduinos(){
  byte errCode,devFound;
  byte buf[4]; //serial number store
  unsigned long SN=0;
  String formatted;
  //open the file
  if (SPIFFS.exists(addrPath)) {
    SPIFFS.remove(addrPath);
  }
  File addr = SPIFFS.open(addrPath, "w");

  //Wake all arduinos if they're sleeping
  Wire.beginTransmission(0);
  Wire.endTransmission();
  delay(200); //wait for the arduinos to wake up
  for(uint8_t i=1;i<127;i++){
    Wire.beginTransmission(i);
    errCode=Wire.endTransmission();
    if(errCode==0){//No error means that the device was found
      Wire.requestFrom(i,4); //the serial number is an unsigned long
      for(uint8_t j=0;j<4;j++){
        buf[j] = Wire.read();
      }
      //generates the long
      SN+= (buf[0] << 24); 
      SN+= (buf[1] << 16); 
      SN+= (buf[2] << 8); 
      SN+= (buf[3] ); 
      if(devFound==0){
        formatted = String(i,HEX)+" "+String(SN,DEC) ;
      }
      else{
        formatted += '\n'+String(i,HEX)+" "+String(SN,DEC) ;
      }
      devFound++;
    }
  }
  //write the data at the end, so that the number of writing is reduced
  addr.write((uint8_t*)formatted.c_str(),formatted.length());
  addr.close();
  
}
/* Useless as the controller sleeps almost all the time and to wake it, I need to reset it.
    All the code is inside the setup() and the functions called inside
*/
void loop() {}
