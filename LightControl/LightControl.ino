#define LDR A0 //light sesor
#define PIR 0 //movement ir sensor
#define RELAY 1
 #define LM35 A2 //temp sensor
//Transmission
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
#define OUT_COMMAND  0b00101010 //isOut ask
#define IS_OUT 0b01000000
#define GET_TEMP 0b10000000
#define CONV_TEMP 0.48828125 //LM35 converting from Analog input to celsius degrees
#define SET_TEMP 0b01000000 
#define GET_IS_OUT 0b11000000

#include <RH_ASK.h>
#include <math.h>
RH_ASK trasm(SPEED,RX_PIN,TX_PIN,PTT_PIN,PTT_INVERTED);
bool isOut=false;
uint8_t buflen = 3;
void setup() {
  
  pinMode(PIR,INPUT);
  pinMode (RELAY,OUTPUT);
  if(trasm.init()) { error(); }
  initData();
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
   
   if(trasm.available()){
      getMsg();
   }

}
void initData(){
    uint8_t msg[]={RASP_HEADER,GET_IS_OUT};
    uint8_t *buf;
    bool recieved=false;
    while(!recieved){
      trasm.setModeTx();
      trasm.send(msg,2);
      trasm.waitPacketSent();
      trasm.setModeRx();
      delay(100);
      if(trasm.recv(buf,&buflen)){
         if((buf[1] & TO_ME)==TO_ME){
            if((buf[1] & ID)==ID){
              if((buf[2] & IS_OUT)==IS_OUT){
                isOut=true;
              }
              else {
                isOut=false;
                 
              }
            }
         }       
        }
    }   
}
   void getMsg(){
  uint8_t *buf;
    if(trasm.recv(buf,&buflen)){
              
              if((buf[1] & TO_ME)==TO_ME){ //dal raspi
                  if((buf[1] & ID)==ID){//sono io
                    switch(buf[2] & COMMAND_BIT ) {
                      case IS_OUT:{
                            if((buf[2]&OUT_COMMAND)==OUT_COMMAND){
                                isOut=true;
                                sendOK();
                             }
                            else
                                isOut=false;    
                        }
                      case GET_TEMP:{
                              getTemp(); //set only if has the temperature sensor
                        }
                      
                      }
                  }
              } 
    
    }
  }
 void getTemp(){
    float temp=0;
    float sumTemp=0;
    for(byte i=0;i<20;i++){
      temp=(analogRead(LDR)*CONV_TEMP);
      sumTemp +=temp;
      delay(50);
    }
    temp=sumTemp/20;
    uint8_t prepTemp = (uint8_t ) round(temp);
    uint8_t sendTemp= (SET_TEMP | prepTemp);
    uint8_t msg[]={RASP_HEADER,sendTemp};
    trasm.setModeTx();
    trasm.send(msg,2);
    trasm.waitPacketSent();
    trasm.setModeRx();  
  }
void sendOK(){
      uint8_t msg[]={RASP_HEADER,OK_ANSWER};
   trasm.setModeTx();
    trasm.send(msg,2);
    trasm.waitPacketSent();
    trasm.setModeRx(); 
  }  
void error(){
    while(true){
        pinMode(RELAY,HIGH);
        delay(500);
        pinMode(RELAY,LOW);
        delay(500);
      }
  }
