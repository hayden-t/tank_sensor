#include <SPI.h>
#include <RF24.h>
#include "printf.h"
#include "LowPower.h"

int channel = 84;
RF24 radio(9,10);

#define MIN_PA
//#define LOW_PA
//#define HIGH_PA

//#define SLEEP_4

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

typedef struct {
  int distance;
  byte voltage;
}data;

int txTimes = 3;//times to resend same reading
int txDelay = 1000;//delay between txTimes //100 seems to work


const int SleepTimes = 75;//x8 sec = sleep timer 75 = 10min, 0 = dont sleep, keep reading


const int speedSound = 343;//m/s
const int minDistance = 25;//minimum distance sensor can read, under is error
const int maxDistance = 445;//maximum distance sensor can read, over is error

const int TRIG_PIN = 2;
const int ECHO_PIN = 4;
const int ULTR_PIN = 3;
const int VDIV_PIN = 7;
const int RF24_PIN = 8;
const int BATT_PIN = A3;

void setup() {
  
  Serial.begin(9600);
  printf_begin();
  printf("\n\n\rWATER SENSOR START\n\n\r");
  pinMode(ECHO_PIN, INPUT);//echo
  pinMode(BATT_PIN, INPUT);//battery voltage
  pinMode(TRIG_PIN, OUTPUT);//triger
  pinMode(ULTR_PIN, OUTPUT);//sensor switch
  pinMode(VDIV_PIN, OUTPUT);//battery voltage divider switch
  pinMode(RF24_PIN, OUTPUT);//sensor switch
  digitalWrite(ULTR_PIN, LOW);//turn off sensor power
  digitalWrite(VDIV_PIN, LOW);//turn off divider power
  digitalWrite(RF24_PIN, LOW);//turn off sensor power

}

void loop(){

     printf("\n\rPower ON\n\r");
     
     int distance = getDistance();
     int voltage = readBatt();
     
     printf("\n\rDistance: %d cm, Voltage: %d\n\r",distance, voltage); 
     if(!distance)printf("** Error reading Sensor **\n\r");
     
     transmit(distance, voltage);    
    
    if(SleepTimes){
       printf("Power OFF\n\r\n\r");
    }
    
     int i = 0;
     while(i < SleepTimes){
      
        printf("%d.",SleepTimes - i);
        delay(100);
       #ifdef SLEEP_4 
         LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
       #else
         LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
       #endif
        i++;
       
     }
     if(!SleepTimes)delay(3000);//no sleep delay between reads
     printf("\n\r");
     
}//end loop

int getDistance(){
  
  printf("\n\rReading Distance\n\r");
  
  digitalWrite(ULTR_PIN, HIGH);//turn on sensor
  delay(500);//risetime
  
  unsigned long duration; 

  digitalWrite(TRIG_PIN, HIGH);//trigger start
  delayMicroseconds(100);
  digitalWrite(TRIG_PIN, LOW);//trigger stop
  
  duration = pulseIn(ECHO_PIN, HIGH);//read result
  printf("Time: %lu", duration);

  digitalWrite(ULTR_PIN, LOW);//turn off sensor  
  delay(100);
  
  int result = (duration * speedSound)/20000;//cm
  if(result > maxDistance || result < minDistance)result = 0;
  
  return result;
}

void transmit(int distance, int voltage) {
  
   data payload = { distance, voltage};
  
  digitalWrite(RF24_PIN, HIGH);//turn on power
  delay(100);//rise time
  
   printf("\n\rRadio Start\n\n\r");
   SPI.begin();
   radio.begin();
   
   radio.setPayloadSize(sizeof(data));
   
   radio.setChannel(channel);

    #ifdef MIN_PA 
      radio.setPALevel(RF24_PA_MIN);//-12dbm
    #endif     
    #ifdef LOW_PA 
      radio.setPALevel(RF24_PA_LOW);//-12dbm
    #endif  
    #ifdef HIGH_PA
      radio.setPALevel(RF24_PA_HIGH);
    #endif
   
   radio.setDataRate(RF24_250KBPS);
   
   radio.setRetries(15,15);

   radio.openWritingPipe(pipes[0]);
   radio.openReadingPipe(1,pipes[1]);   
   
   radio.powerUp();
   
   radio.flush_tx();
   
   radio.printDetails();printf("\n\r");
   
      
     int i = 0;
     while(i < txTimes){
       if (radio.write( &payload, sizeof(payload))) printf("Transmit Success.\n\n\r");//??
       else printf("Transmit Failed.\n\n\r");
       delay(txDelay);
       i++;
     }
     
   digitalWrite(RF24_PIN, LOW);//turn off rf
     

}

int readBatt() {
  
   digitalWrite(VDIV_PIN, HIGH);//turn div on
   
   delay(100);//rise time
   
   int voltage = analogRead(BATT_PIN) * (float)(50 / 1023.0);
   
  digitalWrite(VDIV_PIN, LOW);//turn div off
    
  return voltage;
}
