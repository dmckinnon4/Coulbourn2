#include "Keypad.h"
#include <SPI.h>
#include <SD.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
// set the LCD address to 0x27 for a 20 chars 4 line display
// Set the pins on the I2C chip used for LCD connections:
//                    addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address


// Time Constants 
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24L)

// switches for experiment 
//boolean variablePulseLength = false;
boolean variablePulseLength = true;
//boolean variableDelayTime = false;
boolean variableDelayTime = true;

//// Use these values in actual experiment
const unsigned long day_ms = 86400000;
const unsigned long half_day_ms = 43200000; 
unsigned long night_shock_period = 112500;  // 1 min 52.5 sec between shocks, gives 4 shocks/hr/box for 8 boxes
unsigned long day_shock_period = 450000;    // 7
const unsigned long min_night_shock_period = 10000;   // two values give average 4 shocks/hr/box for 8 boxes during night
const unsigned long max_night_shock_period = 215000;  // range 10 to 215 seconds
const unsigned long min_day_shock_period = 10000;     // two values give average 1 shock/hr/box for 8 boxes during day
const unsigned long max_day_shock_period = 890000;    // range 10 to 890 seconds

// Use these values for testing
//const unsigned long day_ms = 60000;       // just for testing
//const unsigned long half_day_ms = 30000;  // just for testing
//unsigned long night_shock_period = 6000;  // 6 seconds between shocks (ms), close to minimum
//unsigned long day_shock_period = 8000;    // 8 seconds between shocks (ms)
//const unsigned long min_night_shock_period = 6000;   // range for night delay
//const unsigned long max_night_shock_period = 10000;  
//const unsigned long min_day_shock_period = 6000;     // range for day delay
//const unsigned long max_day_shock_period = 10000; 

// Coulbourn ttl pulse definition
unsigned long ttL_Pulse_width = 2000;       // default length of shock, 2 second

const int switch_delay = 5;                 // 4 ms, delay period to allow relay switch to settle

// Set up Pins
const int Relay1_Pin = 3;            // Pin 3 to control Relay 1 (relay in box)
const int Right_Relay1_Pin = 43;     // Pin 34 to control relay 1 in right outboard box
const int Right_Relay2_Pin = 45;     // Pin 32 to control relay 2 in right outboard box
const int Right_Relay3_Pin = 47;     // Pin 30 to control relay 3 in right outboard box
const int Left_Relay1_Pin = 31;      // Pin 52 to control relay 1 in left outboard box
const int Left_Relay2_Pin = 33;      // Pin 50 to control relay 2 in left outboard box
const int Left_Relay3_Pin = 35;      // Pin 48 to control relay 3 in left outboard box
const int Coulbourn_ttL_Pin = 12;    // ttL pulse to Coulbourn stimulator, digital pin 13

long start_time;              // time to first shock, about 1.5 seconds
unsigned long dayTime;        // variable for calculating time of day
unsigned long loopStartTime;  // time at start of loop
long day_shock_Number = 1;    // tracks number of shocks delivered during day, used to determine timing of shocks
long night_shock_Number = 1;  // tracks number of shocks delivered during day, used to determine timing of shocks
int nextCageNumber;           // which cage to shock next
int cageNumber;               // which cage to shock

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

// Macros for getting elapsed time 
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)  
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN) 
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)
#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY)  


void setup() {  // setup code, runs once
  //start serial connection
  Serial.begin(57600);  // Opening serial port resets arduino, which is why "Reset" is written twice
  Serial.println("Reset"); 
  
  // Blink backlight 3 times, then display message
  lcd.begin (20,4); //   20x4 screen
  for(int i = 0; i< 3; i++) {
    lcd.backlight();
    delay(250);
    lcd.noBacklight();
    delay(250);
  }
  lcd.backlight(); // finish with backlight on  
  lcd.setCursor(0,0); //Start at character 0 on line 0
  lcd.print(": Shock Protocol 1 :");
  lcd.setCursor(0,1); //Start at character 0 on line 1
  lcd.print("Press '1' to start");
  
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

  // select first cage to be shocked first
  nextCageNumber = 1;
  cageNumber = 0;
  
  // hold in loop until Keaypad '1' button is presssed to begin the program proper
  while (true) {
    char key = keypad.getKey();
    if (key == '1') {
      break;
    }
  }
  
  Serial.println("Program: Shock Protocol 1: start");
  lcd.setCursor(0,3); //Start at character 0 on line 3
  lcd.print("Night  ");
  
  start_time = millis();
}  // setup


void loop() {  // main code, runs repeatedly
  loopStartTime = millis()-start_time;
  
  Serial.print(loopStartTime);      // print current time minus time taken to start (ms)
  Serial.print("\t");
  printTime(loopStartTime / 1000);  // print current time minus time taken to start (day hh:mm:ss format)

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
  
  if (variablePulseLength) { 
    ttL_Pulse_width = random(1,6)*1000; // msec
  }
  
  switch (cageNumber) {
    case 1:
      Serial.print("Left Box 1"); 
      Serial.print("\t");
      Serial.print(ttL_Pulse_width/1000);
      Serial.print(" sec");
      lcd.setCursor(0,1); //Start at character 0 on line 1
      lcd.print("Left Box 1          ");
      lcd.setCursor(14,1); //Start at character 14 on line 1
      lcd.print(ttL_Pulse_width/1000);  // convert to seconds
      lcd.setCursor(16,1); //Start at character 16 on line 1
      lcd.print("sec");
      digitalWrite(Relay1_Pin, LOW);    // these 2 lines create a click with the relay, otherwise this cage doesn't get a click
      delay(5);                         // minimum time to get a nice click this means that this rat gets slightly longer delay before shock
      digitalWrite(Relay1_Pin, HIGH);             // set Relay 1 to left
      digitalWrite(Left_Relay1_Pin, HIGH);       // set Relay 2 to right
      digitalWrite(Left_Relay2_Pin, HIGH);  // set Relay 3 to front
      digitalWrite(Left_Relay3_Pin, HIGH);   // set Relay 4 to right
      delay(switch_delay);                      // delay to allow relay switch to settle before passing current
      digitalWrite(Coulbourn_ttL_Pin, HIGH);    // turn Coulbourn ttL on
      delay(ttL_Pulse_width);                   // wait for ttL_Pulse_width
      digitalWrite(Coulbourn_ttL_Pin, LOW);     // turn Coulbourn ttL off
      delay(switch_delay);                      // delay before before switching off
      turnRelaysOff();
      break;
  
    case 2:
      Serial.print("Left Box 2");
      Serial.print("\t");
      Serial.print(ttL_Pulse_width/1000);
      Serial.print(" sec");
      lcd.setCursor(0,1); //Start at character 0 on line 1
      lcd.print("Left Box 2      ");
      lcd.setCursor(14,1); //Start at character 14 on line 1
      lcd.print(ttL_Pulse_width/1000);  // convert to seconds
      lcd.setCursor(16,1); //Start at character 16 on line 1
      lcd.print("sec");
      digitalWrite(Relay1_Pin, HIGH);           // set Relay 1 to right
      digitalWrite(Left_Relay1_Pin, HIGH);           // set Relay 2 to right
      digitalWrite(Left_Relay2_Pin, LOW);           // set Relay 2 to right
      digitalWrite(Left_Relay3_Pin, HIGH);           // set Relay 2 to right
      delay(switch_delay);                      // delay to allow relay switch to settle before passing current
      digitalWrite(Coulbourn_ttL_Pin, HIGH);    // turn Coulbourn ttL on
      delay(ttL_Pulse_width);                   // wait for ttL_Pulse_width
      digitalWrite(Coulbourn_ttL_Pin, LOW);     // turn Coulbourn ttL off
      delay(switch_delay);                      // delay before before switching off
      turnRelaysOff();
      break;
  
    case 3:
      Serial.print("Left Box 3");
      Serial.print("\t");
      Serial.print(ttL_Pulse_width/1000);
      Serial.print(" sec");
      lcd.setCursor(0,1); //Start at character 0 on line 1
      lcd.print("Left Box 3      ");
      lcd.setCursor(14,1); //Start at character 14 on line 1
      lcd.print(ttL_Pulse_width/1000);  // convert to seconds
      lcd.setCursor(16,1); //Start at character 16 on line 1
      lcd.print("sec");
      digitalWrite(Relay1_Pin, HIGH);           // set Relay 1 to right
      digitalWrite(Left_Relay1_Pin, LOW);           // set Relay 2 to right
      digitalWrite(Left_Relay2_Pin, HIGH);           // set Relay 2 to right
      digitalWrite(Left_Relay3_Pin, HIGH);           // set Relay 2 to right
      delay(switch_delay);                      // delay to allow relay switch to settle before passing current
      digitalWrite(Coulbourn_ttL_Pin, HIGH);    // turn Coulbourn ttL on
      delay(ttL_Pulse_width);                   // wait for ttL_Pulse_width
      digitalWrite(Coulbourn_ttL_Pin, LOW);     // turn Coulbourn ttL off 
      delay(switch_delay);                      // delay before before switching off
      turnRelaysOff();
      break;
  
    case 4:
      Serial.print("Left Box 4");
      Serial.print("\t");
      Serial.print(ttL_Pulse_width/1000);
      Serial.print(" sec");
      lcd.setCursor(0,1); //Start at character 0 on line 1
      lcd.print("Left Box 4      ");
      lcd.setCursor(14,1); //Start at character 14 on line 1
      lcd.print(ttL_Pulse_width/1000);  // convert to seconds
      lcd.setCursor(16,1); //Start at character 16 on line 1
      lcd.print("sec");
      digitalWrite(Relay1_Pin, HIGH);           // set Relay 1 to right
      digitalWrite(Left_Relay1_Pin, LOW);           // set Relay 2 to right
      digitalWrite(Left_Relay2_Pin, HIGH);           // set Relay 2 to right
      digitalWrite(Left_Relay3_Pin, LOW);           // set Relay 2 to right
      delay(switch_delay);                      // delay to allow relay switch to settle before passing current
      digitalWrite(Coulbourn_ttL_Pin, HIGH);    // turn Coulbourn ttL on
      delay(ttL_Pulse_width);                   // wait for ttL_Pulse_width
      digitalWrite(Coulbourn_ttL_Pin, LOW);     // turn Coulbourn ttL off    
      delay(switch_delay);                      // delay before before switching off
      turnRelaysOff();
      break;
      
    case 5:
      Serial.print("Right Box 1"); 
      Serial.print("\t");
      Serial.print(ttL_Pulse_width/1000);
      Serial.print(" sec");
      lcd.setCursor(0,1); //Start at character 0 on line 1
      lcd.print("Right Box 1      ");
      lcd.setCursor(14,1); //Start at character 14 on line 1
      lcd.print(ttL_Pulse_width/1000);  // convert to seconds
      lcd.setCursor(16,1); //Start at character 16 on line 1
      lcd.print("sec");
      digitalWrite(Relay1_Pin, LOW);             // set Relay 1 to left
      digitalWrite(Right_Relay1_Pin, HIGH);       // set Relay 2 to right
      digitalWrite(Right_Relay2_Pin, HIGH);  // set Relay 3 to front
      digitalWrite(Right_Relay3_Pin, HIGH);   // set Relay 2 to right
      delay(switch_delay);                      // delay to allow relay switch to settle before passing current
      digitalWrite(Coulbourn_ttL_Pin, HIGH);    // turn Coulbourn ttL on
      delay(ttL_Pulse_width);                   // wait for ttL_Pulse_width
      digitalWrite(Coulbourn_ttL_Pin, LOW);     // turn Coulbourn t
      delay(switch_delay);                      // delay to allow relay switch to settle before passing current
      turnRelaysOff();
      break;
   
    case 6:
      Serial.print("Right Box 2");
      Serial.print("\t");
      Serial.print(ttL_Pulse_width/1000);
      Serial.print(" sec");
      lcd.setCursor(0,1); //Start at character 0 on line 1
      lcd.print("Right Box 2      ");
      lcd.setCursor(14,1); //Start at character 14 on line 1
      lcd.print(ttL_Pulse_width/1000);  // convert to seconds
      lcd.setCursor(16,1); //Start at character 16 on line 1
      lcd.print("sec");
      digitalWrite(Relay1_Pin, LOW);           // set Relay 1 to right
      digitalWrite(Right_Relay1_Pin, HIGH);           // set Relay 2 to right
      digitalWrite(Right_Relay2_Pin, LOW);           // set Relay 2 to right
      digitalWrite(Right_Relay3_Pin, HIGH);           // set Relay 2 to right
      delay(switch_delay);                      // delay to allow relay switch to settle before passing current
      digitalWrite(Coulbourn_ttL_Pin, HIGH);    // turn Coulbourn ttL on
      delay(ttL_Pulse_width);                   // wait for ttL_Pulse_width
      digitalWrite(Coulbourn_ttL_Pin, LOW);     // turn Coulbourn ttL off
      delay(switch_delay);                      // delay before before switching
      turnRelaysOff();
      break;
  
    case 7:
      Serial.print("Right Box 3");
      Serial.print("\t");
      Serial.print(ttL_Pulse_width/1000);
      Serial.print(" sec");
      lcd.setCursor(0,1); //Start at character 0 on line 1
      lcd.print("Right Box 3      ");
      lcd.setCursor(14,1); //Start at character 14 on line 1
      lcd.print(ttL_Pulse_width/1000);  // convert to seconds
      lcd.setCursor(16,1); //Start at character 16 on line 1
      lcd.print("sec");
      digitalWrite(Relay1_Pin, LOW);           // set Relay 1 to right
      digitalWrite(Right_Relay1_Pin, LOW);           // set Relay 2 to right
      digitalWrite(Right_Relay2_Pin, HIGH);           // set Relay 2 to right
      digitalWrite(Right_Relay3_Pin, HIGH);           // set Relay 2 to right
      delay(switch_delay);                      // delay to allow relay switch to settle before passing current
      digitalWrite(Coulbourn_ttL_Pin, HIGH);    // turn Coulbourn ttL on
      delay(ttL_Pulse_width);                   // wait for ttL_Pulse_width
      digitalWrite(Coulbourn_ttL_Pin, LOW);     // turn Coulbourn ttL off
      delay(switch_delay);                      // delay before before switching
      turnRelaysOff();
      break;
  
    case 8:
      Serial.print("Right Box 4");
      Serial.print("\t");
      Serial.print(ttL_Pulse_width/1000);
      Serial.print(" sec");
      lcd.setCursor(0,1); //Start at character 0 on line 1
      lcd.print("Right Box 4      ");
      lcd.setCursor(14,1); //Start at character 14 on line 1
      lcd.print(ttL_Pulse_width/1000);  // convert to seconds
      lcd.setCursor(16,1); //Start at character 16 on line 1
      lcd.print("sec");
      digitalWrite(Relay1_Pin, LOW);           // set Relay 1 to right
      digitalWrite(Right_Relay1_Pin, LOW);           // set Relay 2 to right
      digitalWrite(Right_Relay2_Pin, HIGH);           // set Relay 2 to right
      digitalWrite(Right_Relay3_Pin, LOW);           // set Relay 2 to right
      delay(switch_delay);                      // delay to allow relay switch to settle before passing current
      digitalWrite(Coulbourn_ttL_Pin, HIGH);    // turn Coulbourn ttL on
      delay(ttL_Pulse_width);                   // wait for ttL_Pulse_width
      digitalWrite(Coulbourn_ttL_Pin, LOW);     // turn Coulbourn ttL off
      delay(switch_delay);                      // delay before before switching
      turnRelaysOff();
      break;
  }  // Switch
  
  // determine which 12 hour period started
  dayTime = (millis()-start_time) % day_ms; // ms elapsed in this day/night period, using modulus with ms/day
  // wait time until next shock depends on day or night
  if (dayTime < half_day_ms) { // night
    if (variableDelayTime) {  // use for random delay
      night_shock_period = random(min_night_shock_period, max_night_shock_period+1);
    }
    
    Serial.print("\tnight\t");
    Serial.print(night_shock_Number);
    Serial.print("\tdelay\t");
    Serial.println(night_shock_period);
    
    lcd.setCursor(0,3); //Start at character 0 on line 3
    lcd.print("Night               ");
    lcd.setCursor(8,3); //Start at character 0 on line 3
    lcd.print(night_shock_period/1000);           
    lcd.setCursor(12,3); //Start at character 0 on line 3
    lcd.print("sec");
    
    // wait until time for next shock
    while ((millis()-start_time -loopStartTime) < night_shock_period) {}

    night_shock_Number = night_shock_Number + 1;
    day_shock_Number = 1;
  }
  else { // day
    if (variableDelayTime) {  // use for random delay
      day_shock_period = random(min_day_shock_period, max_day_shock_period+1); 
    }
    
    Serial.print("\tday\t");
    Serial.print(day_shock_Number);
    Serial.print("\tdelay\t");
    Serial.println(day_shock_period);
    
    lcd.setCursor(0,3); //Start at character 0 on line 3
    lcd.print("Day                 ");
    lcd.setCursor(8,3); //Start at character 0 on line 3
    lcd.print(day_shock_period/1000);         
    lcd.setCursor(12,3); //Start at character 0 on line 3
    lcd.print("sec");
    
    // wait until time for next shock
    while ((millis()-start_time -loopStartTime) < day_shock_period) {}
    
    day_shock_Number = day_shock_Number + 1;
    night_shock_Number = 1;
  }  
}  //loop

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

void printTime(long val){  
  // digital clock display of current time
  int days = elapsedDays(val);
  int hours = numberOfHours(val);
  int minutes = numberOfMinutes(val);
  int seconds = numberOfSeconds(val);
  
  Serial.print(days,DEC);
  Serial.print(" d\t");   
  Serial.print(hours,DEC);
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

