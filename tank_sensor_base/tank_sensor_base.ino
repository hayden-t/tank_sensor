#include <SPI.h>
#include <RF24.h>
#include "printf.h"
#include <LCD5110_Basic.h>
#include <Wire.h>
#include <RTClib.h>

int channel = 84;
RF24 radio(9,10);


byte addresses[][6] = {"1Node","2Node"};

typedef struct {
  int distance;
  byte voltage;
}data;

LCD5110 lcd(4,7,6,8,5);
extern uint8_t SmallFont[];
extern uint8_t BigNumbers[];
char line[15];

//init clock
RTC_DS1307 RTC;

const int LCD_BACKLIGHT = 3;

const int SENS_HEIGHT = 240;
const int FULL_HEIGHT = 200;


void setup() {
  // put your setup code here, to run once:
    Serial.begin(9600);
    printf_begin();
    
    Wire.begin();  
    RTC.begin();
    //RTC.adjust(DateTime(__DATE__, __TIME__).get());//36000 +10 hrs//uncomment and upload once to set clock, comment again and upload after
 
    
    lcd.InitLCD();    
    
    pinMode(LCD_BACKLIGHT, OUTPUT);//lcd backlight
    digitalWrite(LCD_BACKLIGHT, LOW);//lcd backlight  

    printf("\n\rwater_base\n\n\r");
    
    radio.begin();
    radio.setChannel(channel);
    radio.setDataRate(RF24_250KBPS);

    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1,addresses[0]);
    
    radio.printDetails(); 
    radio.startListening();   
    
     sprintf(line, "Hello");
     lcd.setFont(SmallFont);
     lcd.print(line, CENTER, 0);
     
     sprintf(line, "45");
     lcd.setFont(BigNumbers);
     lcd.print(line, CENTER, 8);
     
     sprintf(line, "another line");
     lcd.setFont(SmallFont);
     lcd.print(line, CENTER, 32);
     
     
     delay(3000);
     lcd.clrScr();
    
}
int i;
void loop() {
  // put your main code here, to run repeatedly:
  
  DateTime now = RTC.now();
  
  if ( radio.available() ){
      
       data payload;
       radio.read(&payload, sizeof(payload));
       
        printf("\n\rDistance: %d cm, Voltage: %d\n\n\r",payload.distance, payload.voltage);
        
        int percent = map(payload.distance, SENS_HEIGHT, SENS_HEIGHT-FULL_HEIGHT, 0, 100);
        
         sprintf(line, "%d",percent);
         lcd.setFont(BigNumbers);
         lcd.print(line, CENTER, 8);
        
         
         sprintf(line, "BATT: %d.%dv", int(payload.voltage/10), payload.voltage-(int(payload.voltage/10)*10));
         lcd.setFont(SmallFont);
         lcd.print(line, CENTER, 0);
         
         sprintf(line, "D: %d cm", payload.distance);
         lcd.setFont(SmallFont);
         lcd.print(line, CENTER, 32);

        i++;
  }
 
   sprintf(line, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
   lcd.setFont(SmallFont);
   lcd.print(line, CENTER, 40);
}
