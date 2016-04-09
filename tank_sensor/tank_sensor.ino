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

//const int SleepTimes = 1;
const int SleepTimes = 75;//x8 sec = sleep timer 75 = 10min

const int speedSound = 343;//m/s
const int timeOut = 26000;//445cm (spec 25cm - 4.5m)

const int TRIG_PIN = 2;
const int ECHO_PIN = 4;
const int POWR_PIN = 5;
const int ULTR_PIN = 3;
const int RF24_PIN = 8;

void setup() {
  
  Serial.begin(9600);
  printf_begin();
  printf("\n\n\rWATER SENSOR START\n\n\r");
  pinMode(ECHO_PIN, INPUT);//echo
  pinMode(TRIG_PIN, OUTPUT);//triger
  pinMode(POWR_PIN, OUTPUT);//power switch
  pinMode(ULTR_PIN, OUTPUT);//sensor switch
  pinMode(RF24_PIN, OUTPUT);//sensor switch
  digitalWrite(ULTR_PIN, LOW);//turn off sensor  
  digitalWrite(RF24_PIN, LOW);//turn off sensor  

}

void loop(){
   

     digitalWrite(POWR_PIN, LOW);//turn on boost power
     printf("\n\rPower ON\n\r");
     delay(500);
  
     int distance = getDistance();
     int voltage = readVcc();
     
     printf("\n\rDistance: %d cm, Voltage: %d\n\r",distance, voltage);  
     
     transmit(distance, voltage);
     
     printf("Sleeping");

     printf("\n\rPower OFF\n\r");
     delay(100);
     digitalWrite(POWR_PIN, HIGH);//turn off power
    
     int i = 0;
     while(i < SleepTimes){
      
        printf(".%d",SleepTimes - i);//debug - turn off ?
        delay(100);
       #ifdef SLEEP_4 
         LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
       #else
         LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
       #endif
        i++;
       
     }
     if(!SleepTimes)delay(5000);//no sleep
     printf("\n\r");
     
}//end loop

int getDistance(){
  
  printf("\n\rReading Distance\n\r");
  
  digitalWrite(ULTR_PIN, HIGH);//turn on sensor
  delay(500);
  
  unsigned long duration; 

  digitalWrite(TRIG_PIN, HIGH);//trigger start
  delayMicroseconds(100);
  digitalWrite(TRIG_PIN, LOW);//trigger stop
  
  duration = pulseIn(ECHO_PIN, HIGH, timeOut);//read result
  

  digitalWrite(ULTR_PIN, LOW);//turn off sensor  
  delay(100);
  
  return (duration * speedSound)/20000;//cm
}

void transmit(int distance, int voltage) {
  
   data payload = { distance, voltage};
  
  digitalWrite(RF24_PIN, HIGH);//turn on power
  delay(500);
  
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

int readVcc() {

  static long result;
  
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result; // Back-calculate AVcc in mV 
  
   result /= 100;
    
  return result;
}
