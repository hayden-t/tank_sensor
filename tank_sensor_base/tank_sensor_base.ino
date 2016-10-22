#include <SPI.h>
#include <RF24.h>
#include "printf.h"
#include <LCD5110_Basic.h>
#include <Wire.h>
#include <RTClib.h>
#include <Rotary.h>
#include <Button.h>

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

const int LCD_BACKLIGHT = A0;

Rotary r = Rotary(2, 3);
Button button = Button(A1, BUTTON_PULLUP_INTERNAL);

//25cm min
const int FULL_DISTANCE = 33;
const int EMPTY_DISTANCE = 222;

const int FULL_BATT = 48;//4.8v
const int EMPTY_BATT = 27;//2.7v

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
     
     lastRecieved = RTC.now().get();
     
     PCICR |= (1 << PCIE2);
     PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
     sei();
    
}

void loop() {
  
    DateTime now = RTC.now();
  
    static int lastSec = 0;
    static int percent = 0;
    static int voltage = 0;
    static int distance = 0;
    static int refresh = 0;
  
    unsigned char result = r.process();
    if (result) {
      Serial.println(result == DIR_CW ? "Right" : "Left");
    }
    if(button.uniquePress())Serial.println("Enter");
    
  
    if ( radio.available() ){
      
           i++;
           lastRecieved = now.get();
           connected = true;
           
           data payload;
           radio.read(&payload, sizeof(payload));
           
           distance = payload.distance;
           voltage = payload.voltage;
           
            printf("\n\rDistance: %d cm, Voltage: %d\n\n\r", distance, voltage);
            
            percent = map(constrain(payload.distance, FULL_DISTANCE, EMPTY_DISTANCE), EMPTY_DISTANCE, FULL_DISTANCE, 0, 100);
            
            refresh = 1;
    }
     
     if(lastSec != now.second() || refresh){//update every second
     
         lastSec = now.second();
         refresh = 0;
      
         sprintf(line, "%2d:%02d", (now.hour()>12?now.hour()-12:now.hour()), now.minute());
         lcd.setFont(SmallFont);
         lcd.print(line, LEFT, 0);
         
         DateTime timeSince = now.get()-lastRecieved;
         
         if(connected && timeSince.get() > timeout*60){connected = false;t++;}         
         
         sprintf(line, "%2d:%02d/%d", int(timeSince.get()/60), timeSince.second(),t);
         lcd.setFont(SmallFont);
         lcd.print(line, LEFT, 40);
    
    
         sprintf(line, "%03d",percent);
         lcd.setFont(BigNumbers);
         lcd.print(line, CENTER, 8);
         lcd.setFont(SmallFont);
         lcd.print("%", 62, 16);
         
         int percentage = map(voltage, EMPTY_BATT, FULL_BATT, 0, 100);
         percentage = constrain(percentage, 0, 100);
        
         sprintf(line, "%d%% %d.%dv", percentage, int(voltage/10), voltage-(int(voltage/10)*10));
         lcd.setFont(SmallFont);
         lcd.print(line, RIGHT, 0);
         
         sprintf(line, "%3dcm", distance);
         lcd.setFont(SmallFont);
         lcd.print(line, RIGHT, 40);
       
     }
}


ISR(PCINT2_vect) {
  unsigned char result = r.process();
  if (result == DIR_NONE) {
    // do nothing
  }
  else if (result == DIR_CW) {
    Serial.println("ClockWise");
  }
  else if (result == DIR_CCW) {
    Serial.println("CounterClockWise");
  }
}
