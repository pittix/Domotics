#define CONV_TEMP 0.48828125 //LM35 converting 
void setup() {
  Serial.begin(9600);
  Serial.println("inizializzati i sensori");

}
long counter=1;
void loop() {
  float sum1=0;
  float sum2=0;
  float sum3=0;
  float sum4=0;
  float sum5=0;
  float sum6=0;
  for(int i=0;i<20;i++){
      sum1+=(analogRead(A0)*CONV_TEMP);
      sum2+=(analogRead(A1)*CONV_TEMP);
      sum3+=(analogRead(A2)*CONV_TEMP);
      sum4+=(analogRead(A3)*CONV_TEMP);
      sum5+=(analogRead(A4)*CONV_TEMP);
      sum6+=(analogRead(A5)*CONV_TEMP);
  }
  
  Serial.print("rilevazione numero "); 
  Serial.println(counter++);
  Serial.print("Sensore 1: ");
  Serial.println(sum1/20);
  Serial.print("Sensore 2: ");
  Serial.println(sum2/20);
  Serial.print("Sensore 3: ");
  Serial.println(sum3/20);
  Serial.print("Sensore 4: ");
  Serial.println(sum4/20);
  Serial.print("Sensore 5: ");
  Serial.println(sum5/20);
  Serial.print("Sensore 6: ");
  Serial.println(sum6/20);
  Serial.println("====fine===");
  
  delay(500);

}
