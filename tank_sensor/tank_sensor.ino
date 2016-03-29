#include <SPI.h>
#include <RF24.h>
#include "printf.h"
#include "LowPower.h"

int channel = 84;
RF24 radio(9,10);


byte addresses[][6] = {"1Node","2Node"};

typedef struct {
  int distance;
  byte voltage;
}data;

const int SleepTimes = 0;
//const int SleepTimes = 75;//x8 sec = sleep timer 75 = 10min

const int speedSound = 343;//m/s
const int numSamples = 1;
const int delayLoop = 5000;
const int timeOut = 26000;//445cm (spec 25cm - 4.5m)

const int TRIG_PIN = 2;
const int ECHO_PIN = 4;
const int POWR_PIN = 5;

void setup() {
  
  Serial.begin(9600);
  printf_begin();
  printf("\n\rwater_sensor\n\n\r");
  pinMode(ECHO_PIN, INPUT);//echo
  pinMode(TRIG_PIN, OUTPUT);//triger
  pinMode(POWR_PIN, OUTPUT);//power switch  
 
 
    radio.begin();
    radio.setChannel(channel);
    radio.setDataRate(RF24_250KBPS);

    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1,addresses[1]);
    
    radio.printDetails(); 
 
}


void loop() {  
 
     int distance = getDistance();      
     int voltage = readVcc();
     
     printf("Distance: %d cm, Voltage: %d\n\n\r",distance, voltage);

     radio.powerUp();
     data payload = { distance, voltage};
     
     if (radio.write( &payload, sizeof(payload))) printf("Transmit Success.\n\r");
     else printf("Transmit Failed.\n\r");
  
  
     printf("Sleeping");
     radio.powerDown();
     
     int i = 0;
     while(i < SleepTimes){
      
        printf(".%d",SleepTimes - i);//debug - turn off ?
        delay(100);
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
        i++;
       
     }
     if(!SleepTimes)delay(1000);
     printf("\n\r");
    
}

int getDistance(){  
  unsigned long duration;
  
  
  digitalWrite(POWR_PIN, LOW);//turn on
  delay(100);

  digitalWrite(TRIG_PIN, HIGH);//trigger start
  delayMicroseconds(100);
  digitalWrite(TRIG_PIN, LOW);//trigger stop
  
  duration = pulseIn(ECHO_PIN, HIGH, timeOut);//read result
  
  delay(100);
  digitalWrite(POWR_PIN, LOW);//turn off
  
  return (duration * speedSound)/20000;//cm
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
  
  //VOLTAGE.addValue(int(result));
  //result = VOLTAGE.getAverage();
  
  return result;
}
