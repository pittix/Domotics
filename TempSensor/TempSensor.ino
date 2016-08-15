#define LED_PIN 0
//Transmission spec
#define SPEED 2000 //bps
#define TX_PIN 1
#define RX_PIN A1
#define PTT_PIN 2         //dr3100 
#define PTT_INVERTED true //parameters
//Message parameters
#define TO_ME 0b00000001
#define ID 0b00000010 //ID of the sensor, IT MUST CHANGE FOR EACH DEVICE!!!
#define RASP_HEADER 0b00000100 //Logic shift left 1 position of id
#define OK_ANSWER 0b10101010
#define COMMAND_BIT  0b11000000
#define GET_TEMP_BIT 0b00111111
#define ASK_TEMP 0b00000000 
#define ERROR_MSG 0b01010101
#define SET_TEMP 0b01000000

//Temperature parameters
#define sensorT A0
#define CONV_TEMP 0.48828125 //LM35 converting from Analog input to celsius degrees
#include <RH_ASK.h> //transmission library
#include <math.h> //needed for (int) = round(float/decimal var);
//Initializes the ricetransmitter
RH_ASK trasm(SPEED,RX_PIN,TX_PIN,PTT_PIN,PTT_INVERTED);
float curTemp=0;
uint8_t buflen = 3;
void setup() {
  pinMode(LED_PIN,OUTPUT);
  if(!trasm.init()){
       blinkError();//my blink function
      
  }
 trasm.setModeRx(); 
}



void loop() {
   float sumTemp=0;
  for(byte i=0; i<20;i++){ //The value is more precise
    curTemp=(analogRead(sensorT)*CONV_TEMP); //need other operation to get the temp value from a value which goes from 0 to 1024
    sumTemp += curTemp;
    delay(50);
    }
   curTemp=sumTemp/20;
    
   if(trasm.available()){
        getMsg();
    }
   else delay (10000);//do the loop() every 10 seconds
}
void sendError(){
    uint8_t *buf;
    trasm.setModeTx();
    uint8_t msg[]={RASP_HEADER,ERROR_MSG};
    bool OK_rec=false;
    while(!OK_rec){
      trasm.setModeTx();
      trasm.send(msg,2);
      trasm.waitPacketSent();
      trasm.setModeRx();
      delay(100);
       if(trasm.recv(buf, &buflen)){
           if((buf[1] & TO_ME ) == TO_ME){ //dal raspi
                    if(((int)buf[1] & (ID<<1))==(ID<<1)){//sono io
                        if(buf[2] == OK_ANSWER){
                            OK_rec=true;
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
              if((buf[1] & (uint8_t)ID) == (uint8_t)ID){//sono io
                  if(buf[2] & COMMAND_BIT == ASK_TEMP) {         
                      sendTemp();
                  }
              }
         } 
   }
  else sendError();
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
void blinkError(){
  while(true){
    digitalWrite(LED_PIN,HIGH);
    delay(500);
    digitalWrite(LED_PIN,LOW);
    delay(500);
    }
  }

