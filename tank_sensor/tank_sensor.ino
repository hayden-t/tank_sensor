#include <SPI.h>
#include "nRF24L01.h"
#include <RF24.h>
#include "printf.h"
#include "LowPower.h"
#include <NewPing.h>

int channel = 84;
RF24 radio(9,10);

//#define MIN_PA
#define LOW_PA
//#define HIGH_PA

//#define DEBUG //if enabled, prints to serial affects battery life

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

typedef struct {
  int distance;
  byte voltage;
}data;

byte ack;//standard reply ack

int txTimes = 3;//times to resend same reading
int txDelay = 100;//delay between txTimes //100 seems to work

const int SleepTimes = 75;//x8 sec = sleep timer 75 = 10min, 0 = dont sleep, keep reading

const int MAX_DISTANCE = 445;//maximum distance sensor can read, over is error

const int TRIG_PIN = 2;
const int ECHO_PIN = 4;
const int ULTR_PIN = 3;
const int VDIV_PIN = 7;
const int RF24_PIN = 8;
const int BATT_PIN = A3;

NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

void setup() {
  
  Serial.begin(9600);
  printf_begin();
  printf("\n\n\rWATER SENSOR START\n\n\r");
  
  #ifndef DEBUG
     printf("\n\rDebug OFF\n\r");
  #endif

  pinMode(BATT_PIN, INPUT);//battery voltage divider output
  pinMode(ULTR_PIN, OUTPUT);//sensor switch
  pinMode(VDIV_PIN, OUTPUT);//battery voltage divider switch
  pinMode(RF24_PIN, OUTPUT);//sensor switch
  digitalWrite(ULTR_PIN, LOW);//turn off sensor power
  digitalWrite(VDIV_PIN, LOW);//turn off divider power
  digitalWrite(RF24_PIN, LOW);//turn off sensor power 
  

}

void loop(){

     #ifdef DEBUG
       printf("\n\rPower ON\n\r");
     #endif
     
     int distance = getDistance();
     int voltage = readBatt();
     
     #ifdef DEBUG
       printf("\n\rDistance: %d cm, Voltage: %d\n\r",distance, voltage); 
       if(!distance)printf("** Error reading Sensor **\n\r");
     #endif
     
     transmit(distance, voltage);    
    
    #ifdef DEBUG
      if(SleepTimes){
         printf("Power OFF\n\r\n\r");
      }
    #endif
    
     int i = 0;
     while(i < SleepTimes){
      
      
       #ifdef DEBUG
         printf("%d.",SleepTimes - i);
         delay(100);
       #endif
       LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);

        i++;
       
     }
     if(!SleepTimes)delay(3000);//no sleep delay between reads
     #ifdef DEBUG
       printf("\n\r");
     #endif
     
}//end loop

int getDistance(){
  
  #ifdef DEBUG
    printf("\n\rReading Distance\n\r");
  #endif
  
  digitalWrite(ULTR_PIN, HIGH);//turn on sensor
  delay(100);//risetime  
  
  int duration = sonar.ping_median();
  
  digitalWrite(ULTR_PIN, LOW);//turn off sensor
  
  #ifdef DEBUG
    printf("Time: %lu", duration); 
  #endif
  
  int result = sonar.convert_cm(duration);
  //if(result > maxDistance || result < minDistance)result = 0;
  
  return result;
}

void transmit(int distance, int voltage) {
  
   data payload = { distance, voltage};
  
   digitalWrite(RF24_PIN, HIGH);//turn on power
   delay(10);//rise time
  
   #ifdef DEBUG
     printf("\n\rRadio Start\n\n\r");
   #endif
   
   SPI.begin();
   radio.begin();
   radio.enableAckPayload();
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
  
   radio.setRetries(15,10);

   radio.openWritingPipe(pipes[0]);
   radio.openReadingPipe(1,pipes[1]);   
   
   radio.powerUp();
   
   radio.flush_tx();
   
   #ifdef DEBUG
     radio.printDetails();printf("\n\r");
   #endif
      
     int i = 0;
     while(1){
       #ifdef DEBUG
         printf("Transmiting...");
       #endif
       if (radio.write( &payload, sizeof(payload))){
         #ifdef DEBUG
           printf("Sent...");
         #endif
         if(radio.isAckPayloadAvailable()){
           #ifdef DEBUG
             printf("Ack...");
           #endif
           radio.read( &ack, 1 );
           if(ack == 255){//simple reply recived ok
              #ifdef DEBUG
                printf("Recieved.\n\n\r");
              #endif
              break;             
           }
         }
  
       }else{
       #ifdef DEBUG
         printf("Transmit Failed.\n\n\r");
       #endif
       }
       
       i++;
       if(i == txTimes)break;//delay only if going to tx again
       delay(txDelay);
     }
     
   digitalWrite(RF24_PIN, LOW);//turn off rf
     

}

int readBatt() {
  
   digitalWrite(VDIV_PIN, HIGH);//turn div on
   
   delay(10);//rise time
   
   int voltage = analogRead(BATT_PIN) * (float)(50 / 1023.0);//read voltage at divider
   
   voltage = int((voltage * 2)/6);//convert to cell voltage
   
   digitalWrite(VDIV_PIN, LOW);//turn div off
    
   return voltage;
}
