#include <SPI.h>
#include <RF24.h>
#include "printf.h"
#include <LCD5110_Basic.h>
#include <Wire.h>
#include <RTClib.h>

int channel = 84;
RF24 radio(9,10);


const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

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

//25cm min
const int SENS_HEIGHT = 233;//full heigh 33cm
const int FULL_HEIGHT = 200;

long lastRecieved;

int i, t;

boolean connected = false;
int timeout = 12;//mins till timeout

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
    
    radio.setPayloadSize(sizeof(data));
    
    radio.setChannel(channel);
    
    radio.setPALevel(RF24_PA_LOW);//-12dbm
    
    radio.setDataRate(RF24_250KBPS);

    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1,pipes[0]);
    
    radio.powerUp();
    radio.printDetails(); 
    radio.startListening();   
    
     sprintf(line, "Top");
     lcd.setFont(SmallFont);
     lcd.print(line, LEFT, 0);
     
     sprintf(line, "2nd");
     lcd.setFont(SmallFont);
     lcd.print(line, RIGHT, 0);   
     
     sprintf(line, "100");
     lcd.setFont(BigNumbers);
     lcd.print(line, CENTER, 8);
     lcd.setFont(SmallFont);
     lcd.print("%", 62, 16);
     
     sprintf(line, "3rd");
     lcd.setFont(SmallFont);
     lcd.print(line, CENTER, 32);
     
     sprintf(line, "4th");
     lcd.setFont(SmallFont);
     lcd.print(line, CENTER, 40);
     
     
     delay(1000);
     //while(1);
     lcd.clrScr();
     
     lastRecieved = RTC.now().get();
    
}

void loop() {

  
     DateTime now = RTC.now();
  
     sprintf(line, "%d:%02d ", (now.hour()>12?now.hour()-12:now.hour()), now.minute());
     lcd.setFont(SmallFont);
     lcd.print(line, LEFT, 0);
     
     DateTime timeSince = now.get()-lastRecieved;
     
     if(connected && timeSince.get() > timeout*60){connected = false;t++;}
     
     
     sprintf(line, "%d:%02d/%d ", int(timeSince.get()/60), timeSince.second(),t);
     lcd.setFont(SmallFont);
     lcd.print(line, LEFT, 40);
     

  
  if ( radio.available() ){
    
       i++;
       lastRecieved = now.get();
       connected = true;
       
       data payload;
       radio.read(&payload, sizeof(payload));
       
        printf("\n\rDistance: %d cm, Voltage: %d\n\n\r",payload.distance, payload.voltage);
        
        int percent = map(payload.distance, SENS_HEIGHT, SENS_HEIGHT-FULL_HEIGHT, 0, 100);
        
         sprintf(line, "%d",percent);
         lcd.setFont(BigNumbers);
         lcd.print(line, CENTER, 8);
         lcd.setFont(SmallFont);
         lcd.print("%", 62, 16);
        
         sprintf(line, "%d.%dv", int(payload.voltage/10), payload.voltage-(int(payload.voltage/10)*10));
         lcd.setFont(SmallFont);
         lcd.print(line, RIGHT, 0);
         
         sprintf(line, "%dcm", payload.distance);
         lcd.setFont(SmallFont);
         lcd.print(line, RIGHT, 40);       

        
  } 

 

}
