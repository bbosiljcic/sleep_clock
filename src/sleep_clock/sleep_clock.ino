#include <Adafruit_NeoPixel.h>

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);


// Backglight setting
const int RESET_TIME = 10; // in seconds
int BACKLIGHT_TIMEOUT = RESET_TIME;// set initital timeout

bool BACKLIGHT_STATE = true; // true is on; false is off

/*
 * Timing variables
 */

// For accurate Time reading, use Arduino Real Time Clock and not just delay()
static uint32_t last_time, now = 0; // RTC

// initial Time
int HOURS = 0;
int MINUTES = 0;
int SECONDS = 0;

// some constants we need later for the calculations
const int TWELVE_HOUR_MINUTES = 720;
const int TWENTYFOUR_HOUR_MINUTES = 1440;

// day/night switch hours - this defines when the day starts
int DAY_HOURS = 8;
int DAY_MINUTES = 0;

int CURRENT_STATE = 0; // 0 = night; 1 = day; 
float CURRENT_PERCENT_PASSED = 0;

// Time Set Buttons
int HOURS_BUTTON_VALUE;
int MINUTES_BUTTON_VALUE;
int DAY_SWITCH_BUTTON_VALUE;

// Pins definition for Buttons
int HOURS_PIN = 9; // button for hour selection
int MINUTES_PIN = 10; // button for minute selection
int DAY_SWITCH_PIN = 11; // button for switching mode

const int LED_PIN = 12;
const int LED_COUNT = 24;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t day = strip.Color(252, 186, 3);
uint32_t night = strip.Color(13, 57, 255);

void handleTimeOverflow() {
  if (SECONDS == 60){
    SECONDS = 0;
    MINUTES = MINUTES + 1;
  }
  if (MINUTES == 60) {
    MINUTES = 0;
    HOURS = HOURS + 1;
  }
  if (HOURS == 24) {
    HOURS = 0;
  }
  if (DAY_MINUTES == 60) {
    DAY_MINUTES = 0;
  }
  if (DAY_HOURS == 13) {
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
  // Line 1
  lcd.setCursor(0,0);
  lcd.print("Time    ");

  printTwoDigitValue(HOURS);
  lcd.print(":");
  printTwoDigitValue(MINUTES);
  lcd.print(":");
  printTwoDigitValue(SECONDS);

  // Line 2
  lcd.setCursor(0,1); 
  lcd.print(CURRENT_STATE ? 'D' : 'N');
  lcd.print(" ");
  printTwoDigitValue(round(CURRENT_PERCENT_PASSED * 100));
  lcd.print("%     ");

  printTwoDigitValue(DAY_HOURS);
  lcd.print(":");
  printTwoDigitValue(DAY_MINUTES);
}

void resetBacklightTimeout() {
  BACKLIGHT_TIMEOUT=RESET_TIME;
  lcd.backlight(); 
}

void setLEDs() {
  float PERCENT_LEFT = 1.0 - CURRENT_PERCENT_PASSED;
  float LED_LEFT =  PERCENT_LEFT * 12.0;
  int LED_PAST = 12 - (int) LED_LEFT;
  strip.clear();
  if (CURRENT_STATE) {
    strip.fill(day, 0 + LED_PAST, LED_LEFT);
  } else {
    strip.fill(night, 12 + LED_PAST, LED_LEFT);

  }
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

  setLEDs();
}

void setup() {
  Serial.begin(9600); 

  lcd.init(); 
  lcd.backlight(); 

  strip.begin();

  strip.setBrightness(10);
  strip.fill(day, 0, 12);
  strip.fill(night, 12, 12);


  // BUTTON PINS
  pinMode(HOURS_PIN,INPUT_PULLUP);
  pinMode(MINUTES_PIN,INPUT_PULLUP);
  pinMode(DAY_SWITCH_PIN,INPUT_PULLUP);

  // LCD PINS
  BACKLIGHT_STATE = true;

  now=millis(); // read RTC initial value
}


void loop() {
  strip.show();

  printDisplay();
  // make 5 time 200ms loop, for faster button response
  for (int i = 0; i < 5; i++ ) { 
    while ((now-last_time) < 200) { // delay 200ms
      now=millis();
    }
  
    last_time = now;

    // read Setting Buttons
    HOURS_BUTTON_VALUE = digitalRead(HOURS_PIN);
    MINUTES_BUTTON_VALUE = digitalRead(MINUTES_PIN);
    DAY_SWITCH_BUTTON_VALUE = digitalRead(DAY_SWITCH_PIN);

    // hit any button while the backlight is off
    if ((HOURS_BUTTON_VALUE == LOW | MINUTES_BUTTON_VALUE == LOW | DAY_SWITCH_BUTTON_VALUE == LOW) & (BACKLIGHT_STATE == false)) {
      BACKLIGHT_TIMEOUT = RESET_TIME;
      lcd.backlight(); 
      BACKLIGHT_STATE = true;
      // wait until Button released
      while (HOURS_BUTTON_VALUE == LOW | MINUTES_BUTTON_VALUE == LOW | DAY_SWITCH_BUTTON_VALUE == LOW) {
        HOURS_BUTTON_VALUE = digitalRead(HOURS_PIN);
        MINUTES_BUTTON_VALUE = digitalRead(MINUTES_PIN);
        DAY_SWITCH_BUTTON_VALUE = digitalRead(DAY_SWITCH_PIN);
      }
    } else {
      // when backlight is on we can start processing our buttin inputs
      if (HOURS_BUTTON_VALUE == LOW) {
        if (DAY_SWITCH_BUTTON_VALUE == LOW) {
          DAY_HOURS = DAY_HOURS + 1;
        } else {
          HOURS = HOURS + 1;
        }
      }

      if (MINUTES_BUTTON_VALUE == LOW) {
        if (DAY_SWITCH_BUTTON_VALUE == LOW) {
          DAY_MINUTES = DAY_MINUTES + 1;
        } else {
          SECONDS = 0;
          MINUTES = MINUTES + 1;
        }
      }

      handleTimeOverflow();

      if (HOURS_BUTTON_VALUE == LOW | MINUTES_BUTTON_VALUE == LOW) {
        BACKLIGHT_TIMEOUT = RESET_TIME;
        printDisplay();
      }
    }
  }

  // increment seconds
  SECONDS = SECONDS + 1;
  if (SECONDS == 60) {
    handleMinuteCheck();
  }

  // decriment the backlight timeout time
  BACKLIGHT_TIMEOUT--;
  if (BACKLIGHT_TIMEOUT == 0) {
    // turn the backlight off
    lcd.noBacklight();
    BACKLIGHT_STATE = false;
  }

  // TODO: remove this line below just for debugging
  handleMinuteCheck();
  
  handleTimeOverflow();
}

