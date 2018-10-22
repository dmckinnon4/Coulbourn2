#include "Keypad.h"
#include <SPI.h>
//#include <SD.h>
#include <Wire.h> 
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>
// set the LCD address to 0x27 for a 20 chars 4 line display
// Set the pins on the I2C chip used for LCD connections:
//                    addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
RTC_DS1307 RTC; // define the Real Time Clock object

//File logfile;  // the logging file

// Time Constants 
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24L)
// Macros for getting elapsed time 
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)  
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN) 
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)
#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY) 

// Constants for Experiment 1
//const unsigned long day_ms = 86400000;
//const unsigned long half_day_ms = 43200000;
//const unsigned long night_shock_period = 120000;  // 2 minutes between shocks (ms), gives 3.75 shocks/hr/box for 8 boxes
//const unsigned long day_shock_period = 450000;    // 7.5 minutes between shocks (ms), gives 1 shock/hr/box for 8 boxes

// Constants for Experiment 1 Testing
const unsigned long day_ms = 32000;       // just for testing
const unsigned long half_day_ms = 10000;  // just for testing
unsigned long day_shock_period = 4000;    // 4 seconds between shocks (ms)
unsigned long night_shock_period = 4000;  // 4 seconds between shocks (ms), close to minimum

// Set up Pins
int Relay1_Pin = 3;            // Pin 3 to control Relay 1 (relay in box)
int Right_Relay1_Pin = 43;     // Pin 34 to control relay 1 in right outboard box
int Right_Relay2_Pin = 45;     // Pin 32 to control relay 2 in right outboard box
int Right_Relay3_Pin = 47;     // Pin 30 to control relay 3 in right outboard box
int Left_Relay1_Pin = 31;      // Pin 52 to control relay 1 in left outboard box
int Left_Relay2_Pin = 33;      // Pin 50 to control relay 2 in left outboard box
int Left_Relay3_Pin = 35;      // Pin 48 to control relay 3 in left outboard box
int Coulbourn_ttL_Pin = 12;    // ttL pulse to Coulbourn stimulator, digital pin 13 

// keypad type definition
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] =
 {{'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}};
byte rowPins[ROWS] = {44, 46, 48, 50}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {38, 40, 42}; // connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// for the data logging shield, we use digital pin 10 for the SD cs line
const int chipSelect = 10;      

// Coulbourn ttl pulse definition
int switch_delay = 10;                     // 10 ms, delay period to allow relay switch to settle

void error(char *str) {
  Serial.print("error: ");
  Serial.println(str);
  lcd.setCursor(0,1); //Start at character 0 on line 1
  lcd.print(str);
  while(1);
}

void setup() {
  //start serial connection
  Serial.begin(57600);  // Opening serial port resets arduino, which is why "Reset" is written twice  
  
  // Set up output pins
  pinMode(Coulbourn_ttL_Pin, OUTPUT);  // OUTPUT mode for Coulbourn ttL pulse
  pinMode(Relay1_Pin, OUTPUT);         // OUTPUT mode for Relay 1
  pinMode(Right_Relay1_Pin, OUTPUT);   // OUTPUT mode for Right Relay 1
  pinMode(Right_Relay2_Pin, OUTPUT);   // OUTPUT mode for Right Relay 3
  pinMode(Right_Relay3_Pin, OUTPUT);   // OUTPUT mode for Right Relay 4
  pinMode(Left_Relay1_Pin, OUTPUT);    // OUTPUT mode for Left Relay 1
  pinMode(Left_Relay2_Pin, OUTPUT);    // OUTPUT mode for Left Relay 2
  pinMode(Left_Relay3_Pin, OUTPUT);    // OUTPUT mode for Left Relay 3
  
  // Set relays HIGH, which turns relays off, to limit current flow
  turnRelaysOff(); 
  
  // Blink backlight 1 time, then display message
  lcd.begin (20,4); //   20x4 screen
  for(int i = 0; i< 1; i++) {
    lcd.backlight();
    delay(250);
    lcd.noBacklight();
    delay(250);
  }
  lcd.backlight(); // finish with backlight on  
  lcd.setCursor(0,0); //Start at character 0 on line 0
  lcd.print("Press Key to Start: ");
  lcd.setCursor(0,1); //Start at character 0 on line 1
  lcd.print("1: Short Test boxes ");
  lcd.setCursor(0,2); //Start at character 0 on line 2
  lcd.print("2: Long Test boxes  ");
  lcd.setCursor(0,3); //Start at character 0 on line 2
  lcd.print("3: Experiment 1     ");
  
//
//  // see if the card is present and can be initialized:
//  if (!SD.begin(chipSelect)) {    // these pins are used for Mega board
//    error("Card failed, or not present");
//  }
//  Serial.println(" SD card initialized.");
//  lcd.setCursor(0,1); //Start at character 0 on line 1
//  lcd.print("SD Card initialized.");
//  lcd.setCursor(0,2); //Start at character 0 on line 1
//  lcd.print("Press 1 to start");

}  // setup

void turnRelaysOff() {
  // Set all relays HIGH, which is off, to limit current flow
  digitalWrite(Relay1_Pin, HIGH);        
  digitalWrite(Right_Relay1_Pin, HIGH);        
  digitalWrite(Right_Relay2_Pin, HIGH);  
  digitalWrite(Right_Relay3_Pin, HIGH);   
  digitalWrite(Left_Relay1_Pin, HIGH);  
  digitalWrite(Left_Relay2_Pin, HIGH);  
  digitalWrite(Left_Relay3_Pin, HIGH);   
}

void deliverShock(unsigned long ttL_Pulse_width, unsigned long ttL_delay) {
  delay(switch_delay);                      // delay to allow relay switch to settle before passing current
  digitalWrite(Coulbourn_ttL_Pin, HIGH);    // turn Coulbourn ttL on
  delay(ttL_Pulse_width);                   // wait for ttL_Pulse_width
  digitalWrite(Coulbourn_ttL_Pin, LOW);     // turn Coulbourn ttt off
  delay(switch_delay);                      // delay to allow relay switch to settle before passing current
  turnRelaysOff();
  delay(ttL_delay);                         // wait for ttL_delay  
}

void setCage(int cageNumber, unsigned long ttL_Pulse_width, unsigned long ttL_delay) {
  switch (cageNumber) {
      case 1:
        Serial.print("Left Box 1"); 
        lcd.setCursor(0,1); //Start at character 0 on line 1
        lcd.print("Left Box 1          ");
        digitalWrite(Relay1_Pin, HIGH);             // set Relay 1 to left
        digitalWrite(Left_Relay1_Pin, LOW);       // set Relay 2 to right
        digitalWrite(Left_Relay2_Pin, LOW);  // set Relay 3 to front
        digitalWrite(Left_Relay3_Pin, HIGH);   // set Relay 4 to right
        deliverShock(ttL_Pulse_width, ttL_delay);  
        turnRelaysOff();
        break;
      case 2:
        Serial.print("Left Box 2");
        lcd.setCursor(0,1); //Start at character 0 on line 1
        lcd.print("Left Box 2      ");
        digitalWrite(Relay1_Pin, HIGH);           // set Relay 1 to right
        digitalWrite(Left_Relay1_Pin, HIGH);           // set Relay 2 to right
        digitalWrite(Left_Relay2_Pin, HIGH);           // set Relay 2 to right
        digitalWrite(Left_Relay3_Pin, LOW);           // set Relay 2 to right
        deliverShock(ttL_Pulse_width, ttL_delay);  
        turnRelaysOff(); 
        break;
      case 3:
        Serial.print("Left Box 3");
        lcd.setCursor(0,1); //Start at character 0 on line 1
        lcd.print("Left Box 3      ");
        digitalWrite(Relay1_Pin, HIGH);           // set Relay 1 to right
        digitalWrite(Left_Relay1_Pin, LOW);           // set Relay 2 to right
        digitalWrite(Left_Relay2_Pin, HIGH);           // set Relay 2 to right
        digitalWrite(Left_Relay3_Pin, HIGH);           // set Relay 2 to right
        deliverShock(ttL_Pulse_width, ttL_delay);  
        turnRelaysOff(); 
        break;
      case 4:
        Serial.print("Left Box 4");
        lcd.setCursor(0,1); //Start at character 0 on line 1
        lcd.print("Left Box 4      ");
        digitalWrite(Relay1_Pin, HIGH);           // set Relay 1 to right
        digitalWrite(Left_Relay1_Pin, HIGH);           // set Relay 2 to right
        digitalWrite(Left_Relay2_Pin, HIGH);           // set Relay 2 to right
        digitalWrite(Left_Relay3_Pin, HIGH);           // set Relay 2 to right
        deliverShock(ttL_Pulse_width, ttL_delay);  
        turnRelaysOff(); 
        break;
      case 5:
        Serial.print("Right Box 1"); 
        lcd.setCursor(0,1); //Start at character 0 on line 1
        lcd.print("Right Box 1      ");
        digitalWrite(Relay1_Pin, LOW);             // set Relay 1 to left
        digitalWrite(Right_Relay1_Pin, LOW);       // set Relay 2 to right
        digitalWrite(Right_Relay2_Pin, HIGH);  // set Relay 3 to front
        digitalWrite(Right_Relay3_Pin, HIGH);   // set Relay 2 to right
        deliverShock(ttL_Pulse_width, ttL_delay);  
        turnRelaysOff(); 
        break;    
      case 6:
        Serial.print("Right Box 2");
        lcd.setCursor(0,1); //Start at character 0 on line 1
        lcd.print("Right Box 2      ");
        digitalWrite(Relay1_Pin, LOW);           // set Relay 1 to right
        digitalWrite(Right_Relay1_Pin, HIGH);           // set Relay 2 to right
        digitalWrite(Right_Relay2_Pin, HIGH);           // set Relay 2 to right
        digitalWrite(Right_Relay3_Pin, HIGH);           // set Relay 2 to right
        deliverShock(ttL_Pulse_width, ttL_delay); 
        turnRelaysOff();  
        break;
      case 7:
        Serial.print("Right Box 3");
        lcd.setCursor(0,1); //Start at character 0 on line 1
        lcd.print("Right Box 3      ");
        digitalWrite(Relay1_Pin, LOW);           // set Relay 1 to right
        digitalWrite(Right_Relay1_Pin, LOW);           // set Relay 2 to right
        digitalWrite(Right_Relay2_Pin, LOW);           // set Relay 2 to right
        digitalWrite(Right_Relay3_Pin, HIGH);           // set Relay 2 to right
        deliverShock(ttL_Pulse_width, ttL_delay); 
        turnRelaysOff();  
        break;  
      case 8:
        Serial.print("Right Box 4");
        lcd.setCursor(0,1); //Start at character 0 on line 1
        lcd.print("Right Box 4      ");
        digitalWrite(Relay1_Pin, LOW);           // set Relay 1 to right
        digitalWrite(Right_Relay1_Pin, HIGH);           // set Relay 2 to right
        digitalWrite(Right_Relay2_Pin, HIGH);           // set Relay 2 to right
        digitalWrite(Right_Relay3_Pin, LOW);           // set Relay 2 to right
        deliverShock(ttL_Pulse_width, ttL_delay); 
        turnRelaysOff();  
        break;
    }  // Switch
}  // setPath

void testBoxes(unsigned long ttL_Pulse_width, unsigned long ttL_delay) {  
  lcd.setCursor(0,0); //Start at character 0 on line 0
  lcd.print(":    Test Boxes    :");
  lcd.setCursor(0,2); //Start at character 0 on line 2
  lcd.print("                    ");
  lcd.setCursor(0,3); //Start at character 0 on line 3
  lcd.print("                    ");
  while (true) {
    setCage(1,ttL_Pulse_width, ttL_delay);  // left 1
    setCage(2,ttL_Pulse_width, ttL_delay);  // left 2
    setCage(3,ttL_Pulse_width, ttL_delay);  // left 3
    setCage(4,ttL_Pulse_width, ttL_delay);  // left 4
    setCage(5,ttL_Pulse_width, ttL_delay);  // right 1
    setCage(6,ttL_Pulse_width, ttL_delay);  // right 2
    setCage(7,ttL_Pulse_width, ttL_delay);  // right 3
    setCage(8,ttL_Pulse_width, ttL_delay);  // right 4
  } // while`
} // testBoxes

void printTime(long val){  
  // digital clock display of current time
  int days = elapsedDays(val);
  int hours = numberOfHours(val);
  int minutes = numberOfMinutes(val);
  int seconds = numberOfSeconds(val);
  
  Serial.print(days,DEC);  
  printDigits(hours);  
  printDigits(minutes);
  printDigits(seconds);
  Serial.print("\t");
}

void printDigits(byte digits){
 // utility function for digital clock display: prints colon and leading 0
 Serial.print(":");
 if(digits < 10)
   Serial.print('0');
 Serial.print(digits,DEC);  
}

void printDate(){ 
  DateTime now = RTC.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print("\t");
  if(now.hour() < 10)
   Serial.print('0');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  if(now.minute() < 10)
   Serial.print('0');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  if(now.second() < 10)
   Serial.print('0');
  Serial.print(now.second(), DEC);
  Serial.print("\t");
}

void experiment1() {  
  const unsigned long ttL_Pulse_width = 2000;   // 2 second constant length pulse
  long start_time;              // time to first shock, about 1.5 seconds
  unsigned long dayTime;        // variable for calculating time of day
  long day_shock_Number = 1;    // tracks number of shocks delivered during day, used to determine timing of shocks
  long night_shock_Number = 1;  // tracks number of shocks delivered during day, used to determine timing of shocks
  int nextCageNumber;           // which cage to shock next
  int cageNumber;               // which cage to shock
  
  lcd.setCursor(0,0); //Start at character 0 on line 0
  lcd.print(":   Experiment 1   :");
  lcd.setCursor(0,1); //Start at character 0 on line 1
  lcd.print("                   ");
  lcd.setCursor(0,2); //Start at character 0 on line 2
  lcd.print("                   ");
  lcd.setCursor(0,3); //Start at character 0 on line 3
  lcd.print("                   ");
  start_time = millis();
  while (true) {
    printTime((millis()-start_time) / 1000);  // print current time minus time taken to start
    printDate();
    cageNumber = nextCageNumber;
    nextCageNumber = random(1,9);            // select cage to shock next
    lcd.setCursor(0,2); //Start at character 0 on line 1
    switch (nextCageNumber) {
      case 1: lcd.print("Next: Left Box 1    "); break;
      case 2: lcd.print("Next: Left Box 2    "); break;
      case 3: lcd.print("Next: Left Box 3    "); break;
      case 4: lcd.print("Next: Left Box 4    "); break;
      case 5: lcd.print("Next: Right Box 1   "); break;
      case 6: lcd.print("Next: Right Box 2   "); break;
      case 7: lcd.print("Next: Right Box 3   "); break;
      case 8: lcd.print("Next: Right Box 4   "); break;
    }  // Switch
    
    switch (cageNumber) {
      case 1:
        setCage(1,ttL_Pulse_width, 0);  // Left Box 1
        break;
      case 2:
        setCage(2,ttL_Pulse_width, 0);  // Left Box 2
        break;
      case 3:
        setCage(3,ttL_Pulse_width, 0);  // Left Box 3
        break;
      case 4:
        setCage(4,ttL_Pulse_width, 0);  // Left Box 4
        break;
      case 5:
        setCage(5,ttL_Pulse_width, 0);  // Right Box 1
        break;    
      case 6:
        setCage(6,ttL_Pulse_width, 0);  // Right Box 2
        break;
      case 7:
        setCage(7,ttL_Pulse_width, 0);  // Right Box 3  
        break;  
      case 8:
        setCage(8,ttL_Pulse_width, 0);  // Right Box 4 
        break;
    }  // Switch
    
    // determine which 12 hour period started
    dayTime = (millis()-start_time) % day_ms; // ms elapsed in this day/night period, using modulus with ms/day
    // wait time until next shock depends on day or night
    if (dayTime < half_day_ms) { // night
      Serial.print("\tnight");
      Serial.print("\t");
      Serial.println(night_shock_Number);
      while (((millis()-start_time) % day_ms) < (night_shock_Number*night_shock_period)) {}
      night_shock_Number = night_shock_Number + 1;
      day_shock_Number = 1;
    }
    else { // day
      Serial.print("\tday");
      Serial.print("\t");
      Serial.println(day_shock_Number);
      while ((((millis()-start_time) % day_ms) - half_day_ms) < (day_shock_Number*day_shock_period)) {}
      day_shock_Number = day_shock_Number + 1;
      night_shock_Number = 1;
    }  
  } // while
} // experiment1

void loop() {  // main code, runs until menu selection: 
  char eKey = keypad.getKey();
  switch (eKey) {
    case '1': {
      unsigned long ttL_Pulse_width = 2000;      // 2 seconds, duration of stimulation
      unsigned long ttL_delay = 1000;            // 1 second, time between shocks
      testBoxes(ttL_Pulse_width, ttL_delay);
      break; }
    case '2': {
      unsigned long ttL_Pulse_width = 20000;     // 20 seconds, duration of stimulation
      unsigned long ttL_delay = 1000;            // 1 second, time between shocks
      testBoxes(ttL_Pulse_width, ttL_delay);
      break; }
    case '3': 
      experiment1(); 
      break;
  } // switch
} //loop


