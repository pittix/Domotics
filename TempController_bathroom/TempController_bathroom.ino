/**
 * @author Andrea Pittaro
 * @date 2015/09/18
 * @comment This sketch, when ATtiny85 boots ask the RaspberryPi the temperature I set for my house bedroom and 
 * when asked by the RPI turns on or off. periodically it gives to rpi the temperature measured.
 */
//pin relay
#define relay 0 
//Transmission spec
#define SPEED 2000 //bps
#define TX_PIN 1
#define RX_PIN A1
#define PTT_PIN 2         //dr3100 
#define PTT_INVERTED true //parameters
//Message parameters
#define TO_ME 0b00000001
#define ID 0b00000010
#define RASP_HEADER 0b00000100
#define OK_ANSWER 0b10101010
#define COMMAND_BIT  0b11000000
#define GET_TEMP_BIT 0b00111111
#define ASK_TEMP 0b00000000 //I care about the first 2 bits
#define SET_TEMP 0b01000000 // i used the ANDs and ORs to choose
#define ON       0b10000000 //or set the right packet for the message
#define OFF      0b11000000 //arrived or sent

//Temperature parameters
#define sensorT A0
#define CONV_TEMP 0.48828125 //LM35 converting from Analog input to celsius degrees

int chosenTemp;
float curTemp;
uint8_t buflen = 3;

#include <RH_ASK.h> //transmission library
#include <math.h> //needed for (int) = round(float/decimal var);
//Initializes the ricetransmitter
RH_ASK trasm(SPEED,RX_PIN,TX_PIN,PTT_PIN,PTT_INVERTED);
void setup() {
    
    pinMode(relay,OUTPUT);
  if(!trasm.init()){
      error();        //activate and deactivate relay to show there's an error
      }
  initTemp();
}

void loop() {
  float sumTemp=0;
  for(byte i=0; i<20;i++){ //The value is more precise
    curTemp=(analogRead(sensorT)*CONV_TEMP); //need other operation to get the temp value from a value which goes from 0 to 1024
    sumTemp += curTemp;
    delay(50);
    }
   curTemp=sumTemp/20;
   if(curTemp<chosenTemp-5)
      digitalWrite(relay,HIGH);
   if (curTemp>chosenTemp+5)
      digitalWrite(relay,LOW);
      
  if(trasm.available())
      getMsg();   //read the message
}
void initTemp(){
  uint8_t msg[]={RASP_HEADER,ASK_TEMP};  
  trasm.setModeTx();
  trasm.send(msg,2);
  trasm.waitPacketSent();
  trasm.setModeRx();
  
  uint8_t buf[3];

  if(trasm.available()) { //wait at max 1 second
        if(trasm.recv(buf,&buflen)){
              
              if((buf[1] & TO_ME)==TO_ME){ //dal raspi
                  if((buf[1] & ID)==ID){//sono io
                    switch(buf[2] & COMMAND_BIT ) { //shows only the first 2 bits
                      case SET_TEMP:{
                            chosenTemp=(uint8_t)(buf[2] & GET_TEMP_BIT);
                            break;
                            }
                      case ON:{
                        chosenTemp=(uint8_t)(buf[2] & GET_TEMP_BIT); // Ignores the first 2 command bit and reads the temp
                        break;
                        }
                      case OFF:{
                        chosenTemp=0; // I could never reach a temperature less than 5 degrees in my house
                        break;
                        }                  
                      }
                  }
              }    
          }
      
    } 
 else error();   
 }
void error(){
    while(true){
        pinMode(relay,HIGH);
        delay(500);
        pinMode(relay,LOW);
        delay(500);
      }
  }

  void getMsg(){
  uint8_t *buf;
  uint8_t buflen = 3;
    if(trasm.recv(buf,&buflen)){
              
              if((buf[1] & TO_ME)==TO_ME){ //dal raspi
                  if((buf[1] & ID)==ID){//sono io
                    switch(buf[2] & COMMAND_BIT ) {
                     
                      case ASK_TEMP: {
                        sendTemp();
                        }                    
                      
                      case SET_TEMP:{
                            chosenTemp=(buf[2] & GET_TEMP_BIT);
                            sendOK();
                            break;
                            }
                      case ON:{
                        chosenTemp=(buf[2] & GET_TEMP_BIT);
                        sendOK();
                        break;
                        }
                      case OFF:{
                        chosenTemp=0;
                        sendOK();
                        break;
                        }                  
                      }
                  }
              } 
    
    }
  }
void sendTemp(){
  bool OK_rec=false;
  uint8_t temp=(uint8_t)round(curTemp);
  temp= ( SET_TEMP | temp); //bitwise or: gives the instruction and the temp
  uint8_t msg[]={RASP_HEADER,temp};
  uint8_t *buf;  
  while(!OK_rec){
    trasm.setModeTx();
    trasm.send(msg,2);
    trasm.waitPacketSent();
    trasm.setModeRx();
    delay(100);
    if(trasm.recv(buf,&buflen) ){
      if((buf[1] & TO_ME)==TO_ME){ //dal raspi
                  if(((int)buf[1] & ID)==ID){//sono io
                      if(buf[2] == OK_ANSWER)
                          OK_rec=true;
                  }
        }
    }
  }
 }

 void sendOK(){
  
  uint8_t msg[]={RASP_HEADER,OK_ANSWER};
  trasm.setModeTx();
  trasm.send(msg,2);
  trasm.waitPacketSent();
  trasm.setModeRx();  
 }

