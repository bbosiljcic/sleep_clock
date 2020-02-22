/*
 ###  simplest ever Arduino UNO digital clock  ###
 This clock needs only a 1602 LCD 2X16 and 2 push buttons 
 No Potentiometer for contrast, no resistors for pull-up or backlight !!!!
 * The simplest clock ever made with a Arduino UNO *    
 Button Functions:
 - short stroke on one of the buttons put Backlight on for 30 s
 Time settings
 - Press on H increments the Hours
 - Press on M increments the Minutes and resets the seconds
*/

#include "LiquidCrystal.h"

// This defines the LCD wiring to the DIGITALpins
const int rs = 2, en = 3, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Digital LCD Constrast setting
int CONTRAST_PIN=9;// pin 9 for contrast PWM
const int CONTRAST_VALUE = 100;// default contrast

// Backlight
const int RESET_TIME=100; // times 200ms
int BACKLIGHT_TIMEOUT=RESET_TIME;// Backlight Time-Out

int BACKLIGHT_PIN=10; // Backlight pin
const int BACKLIGHT_VALUE=50; // no more then 7mA !!!

// initial Time
int HOURS=0;
int MINUTES=0;
int SECONDS=0;

// Minutes for 12 hours
const int TWELVE_HOUR_MINUTES = 720;
const int TWENTYFOUR_HOUR_MINUTES = 1440;


// day/night switch hours - this defines when the day starts
int DAY_HOURS=8;
int DAY_MINUTES=0;

int CURRENT_STATE = 0; // 0 = night; 1 = day; 
float CURRENT_PERCENT_PASSED = 0;

// Time Set Buttons
int HOURS_BUTTON;
int MINUTES_BUTTON;
int DAY_SWITCH_BUTTON;

// Pins definition for Time Set Buttons
int HOURS_PIN=0; // pin 0 for Hours Setting
int MINUTES_PIN=1; // pin 1 for Minutes Setting

int DAY_SWITCH_PIN=11; // pin 1 for Minutes Setting


// For accurate Time reading, use Arduino Real Time Clock and not just delay()
static uint32_t last_time, now = 0; // RTC

void handleTimeOverflow() {
  if(SECONDS==60){
    SECONDS=0;
    MINUTES=MINUTES+1;
  }
  if(MINUTES==60) {
    MINUTES=0;
    HOURS=HOURS+1;
  }
  if(HOURS==24) {
    HOURS=0;
  }
  if(DAY_MINUTES == 60) {
    DAY_MINUTES = 0;
  }
  if(DAY_HOURS == 13) {
    // currently only day starts in the first 12 hours are supported
    DAY_HOURS = 0;
  }
}

void printTwoDigitValue(int value) {
  if (value < 10) {
    lcd.print("0");
  }

  lcd.print(value);
}

void printDisplay() {
  lcd.begin(16,2);
  lcd.setCursor(0,0);
  lcd.print("Time    ");
  printTwoDigitValue(HOURS);
  lcd.print(":");
  printTwoDigitValue(MINUTES);
  lcd.print(":");
  printTwoDigitValue(SECONDS);

  lcd.setCursor(0,1); // for Line 2
  lcd.print(CURRENT_STATE ? 'D' : 'N');
  lcd.print(" ");
  printTwoDigitValue(round(CURRENT_PERCENT_PASSED * 100));
  lcd.print("%     ");

  //lcd.print(" ");
  //lcd.print("Day Start  ");
  printTwoDigitValue(DAY_HOURS);
  lcd.print(":");
  printTwoDigitValue(DAY_MINUTES);
}

void handleMinuteCheck() {
  int dayTimeMinutes = DAY_HOURS * 60 + DAY_MINUTES;
  int currentTimeMinutes = HOURS * 60 + MINUTES;

  if ((currentTimeMinutes < dayTimeMinutes) | (currentTimeMinutes > dayTimeMinutes + TWELVE_HOUR_MINUTES)) {
    CURRENT_STATE = 0;
    int dayTimeMinutesPlusTwelveHours = dayTimeMinutes + TWELVE_HOUR_MINUTES;
    if (currentTimeMinutes < dayTimeMinutes) {
      int lastDayPassed = TWENTYFOUR_HOUR_MINUTES - dayTimeMinutesPlusTwelveHours;
      CURRENT_PERCENT_PASSED = (lastDayPassed + currentTimeMinutes) / (float)TWELVE_HOUR_MINUTES;
    } else {
      CURRENT_PERCENT_PASSED = (currentTimeMinutes - dayTimeMinutesPlusTwelveHours) / (float)TWELVE_HOUR_MINUTES;
    }
  } else {
    CURRENT_PERCENT_PASSED = (float)(currentTimeMinutes - dayTimeMinutes) / (float)TWELVE_HOUR_MINUTES;
    CURRENT_STATE = 1;
  }
}

void setup() {
  lcd.begin(16,2);

  pinMode(HOURS_PIN,INPUT_PULLUP);// avoid external Pullup resistors for Button 1
  pinMode(MINUTES_PIN,INPUT_PULLUP);// and Button 2
  pinMode(DAY_SWITCH_PIN,INPUT_PULLUP);// and Button 3

  analogWrite(CONTRAST_PIN,CONTRAST_VALUE);// Adjust Contrast VO
  analogWrite(BACKLIGHT_PIN,BACKLIGHT_VALUE);// Turn on Backlight
  now=millis(); // read RTC initial value  

  // TODO: remove debugging
  Serial.begin(9600); // open the serial port at 9600 bps:
}


void loop() {
  printDisplay();
  // improved replacement of delay(1000) 
  // Much better accuracy, no more dependant of loop execution time

  for (int i=0; i<5; i++) { // make 5 time 200ms loop, for faster Button response

    while ((now-last_time)<200) { //delay200ms
      now=millis();
    }
  
    last_time=now; // prepare for next loop 

    // read Setting Buttons
    HOURS_BUTTON=digitalRead(HOURS_PIN);
    MINUTES_BUTTON=digitalRead(MINUTES_PIN);
    DAY_SWITCH_BUTTON=digitalRead(DAY_SWITCH_PIN);

    //Backlight time out 
    BACKLIGHT_TIMEOUT--;
    if(BACKLIGHT_TIMEOUT==0) {
      analogWrite(BACKLIGHT_PIN,0);// Backlight OFF
      BACKLIGHT_TIMEOUT++;
    }
    
    // Hit any to activate Backlight 
    if(((HOURS_BUTTON==0)|(MINUTES_BUTTON==0)) & (BACKLIGHT_TIMEOUT==1)) {
      BACKLIGHT_TIMEOUT=RESET_TIME;
      analogWrite(BACKLIGHT_PIN,BACKLIGHT_VALUE);
      // wait until Button released
      while ((HOURS_BUTTON==0)|(MINUTES_BUTTON==0)) {
      HOURS_BUTTON=digitalRead(HOURS_PIN);// Read Buttons
      MINUTES_BUTTON=digitalRead(MINUTES_PIN);
      }
    } else { // Process Button 1 or Button 2 when hit while Backlight on 
      if(HOURS_BUTTON==0) {
        if (DAY_SWITCH_BUTTON==0) {
          DAY_HOURS=DAY_HOURS+1;
        } else {
          HOURS=HOURS+1;
        }
        BACKLIGHT_TIMEOUT=RESET_TIME;
        analogWrite(BACKLIGHT_PIN,BACKLIGHT_VALUE);
      }

      if(MINUTES_BUTTON==0) {
        if (DAY_SWITCH_BUTTON==0) {
          DAY_MINUTES=DAY_MINUTES+1;
        } else {
          SECONDS=0;
          MINUTES=MINUTES+1;
        }
        BACKLIGHT_TIMEOUT=RESET_TIME;
        analogWrite(BACKLIGHT_PIN,BACKLIGHT_VALUE);
      }

      handleTimeOverflow();

      if((HOURS_BUTTON==0)|(MINUTES_BUTTON==0)) { // Update display if time set button pressed
        printDisplay();
      }
    }
  }

  SECONDS=SECONDS+1; // increment seconds
  if (SECONDS == 60) {
    handleMinuteCheck();
  }
  // TODO: remove this line below just for debugging
  handleMinuteCheck();
  handleTimeOverflow();
}

