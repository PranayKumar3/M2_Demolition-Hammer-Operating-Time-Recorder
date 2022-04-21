#include<Wire.h>
#include<EEPROM.h>
#include<LiquidCrystal.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#define MPU 0x68
#define MAX_X 0.1
#define MAX_Y 0.1
#define MAX_Z 0.1


// Object initilization
LiquidCrystal lcd(11, 10, 9, 8, 7, 6);

// Variable definitions
int secondRegister = 0;
int minuteRegister = 0;
float AccX, AccY, AccZ;
float x, y, z;
// Switches
int START = A2;
int STOP = A1;
int PREV = A0;
int NEXT = A3;
// Buzzer
int Buzzer = 5;
// Status LED
int statusLED = 12;

// Fuction Declarations
void writeToEEPROM(int addr, int value);
int readFromEEPROM(int addr);
void initMPU6050();
bool checkVibration();
void initTimer();
void pass();
void titleScreen();
void mainMenu();
void calibrating();
void previousValue(int addr);
void recordingDone();

void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2);
  pinMode(START, INPUT_PULLUP);
  pinMode(STOP, INPUT_PULLUP);
  pinMode(PREV, INPUT_PULLUP);
  pinMode(NEXT, INPUT_PULLUP);
  pinMode(Buzzer, OUTPUT);
  pinMode(statusLED , OUTPUT);
  titleScreen();
  delay(3000);
  mainMenu();
  while (1) {
    if (digitalRead(START) ^ digitalRead(PREV)) {
      if (digitalRead(START)) {
        calibrating();
        digitalWrite(Buzzer, HIGH);
        initMPU6050(); // Initilize MPU6050
        initTimer(); // Initilize Timer
        delay(2000);
        digitalWrite(Buzzer, LOW);
        digitalWrite(statusLED, HIGH);
        recordingDone();
        break;
      }
      if (digitalRead(PREV)) {
        previousValue(0);
        break;
      }
    }
    else {
      pass();
    }
  }

}

void loop() {

}

// Function Definitions
void writeToEEPROM(int addr, int value) {
  byte first = (0XFF00 & value) >> 8;
  EEPROM.update(addr * 2, first);
  byte sec = 0X00FF & value;
  EEPROM.update((addr * 2) + 1, sec);
}

int readFromEEPROM(int addr) {
  return (0XFFFF & (EEPROM.read(addr * 2) << 8)) | (EEPROM.read((addr * 2) + 1));
}

void initMPU6050() {
  Wire.begin();                      // Initialize comunication
  Wire.beginTransmission(MPU);       // Start communication with MPU6050 // MPU=0x68
  Wire.write(0x6B);                  // Talk to the register 6B
  Wire.write(0x00);                  // Make reset - place a 0 into the 6B register
  Wire.endTransmission(false);
  Wire.write(0x1C);                  // Talk to the register 1C
  Wire.write(0x08);                  // Write 0X08 to select +-4g for range
  Wire.endTransmission(true);        //end the transmission
  delay(20);
  Wire.beginTransmission(MPU);
  Wire.write(0x3B); // Start with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true); // Read 6 registers total, each axis value is stored in 2 registers
  //For a range of +-4g, we need to divide the raw values by 8192, according to the datasheet
  x = abs((Wire.read() << 8 | Wire.read()) / 8192.0); // X-axis value
  y = abs((Wire.read() << 8 | Wire.read()) / 8192.0); // Y-axis value
  z = abs((Wire.read() << 8 | Wire.read()) / 8192.0); // Z-axis value
}

void initTimer() {
  //cli(); // Disable global interrupt
  // Configuring Timer
  // Clearing Bits to clear Garbage values in registers
  TCCR1B = 0x00;
  TCCR1A = 0x00;
  // Starting Timer with Prescalar as 1024 (CSxx - for selecting prescalar)
  // WGM12 - used in mode 4 for using CTC mode (Clear Timer Capture mode)
  TCCR1B = (1 << CS12) | (0 << CS11) | (1 << CS10) | (1 << WGM12);
  // Initialize counter
  TCNT1 = 0;
  // Setting up TCNT1 to compare with 62500 on OCR1A value determined using formula
  OCR1A = 15625;
  // Enable interupt for compare
  TIMSK1 = (1 << OCIE1A);
  Serial.println("1");
  sei(); // Enable global interupt
  Serial.println("2");
}

bool checkVibration() {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B); // Start with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true); // Read 6 registers total, each axis value is stored in 2 registers
  //For a range of +-4g, we need to divide the raw values by 8192, according to the datasheet
  AccX = (Wire.read() << 8 | Wire.read()) / 8192.0; // X-axis value
  AccY = (Wire.read() << 8 | Wire.read()) / 8192.0; // Y-axis value
  AccZ = (Wire.read() << 8 | Wire.read()) / 8192.0; // Z-axis value
  if (AccX < 0) {
    AccX = abs(AccX);
  }
  if (AccY < 0) {
    AccY = abs(AccY);
  }
  if (AccZ < 0) {
    AccZ = abs(AccZ);
  }
  if (((AccX - x) > MAX_X) | ((AccY - y) > MAX_Y) | ((AccZ - z) > MAX_Z)) {
    x = AccX;
    y = AccY;
    z = AccZ;
    return true;
  }
  else {
    x = AccX;
    y = AccY;
    z = AccZ;
    return false;
  }
}

void pass() {
}

void titleScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("DHOTR - 6ECE-8");
  lcd.setCursor(0, 1);
  lcd.print("Minor Project");
}

void mainMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Choose One");
  lcd.setCursor(0, 1);
  lcd.print("START       PREV");
}

void calibrating() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibrating...");
  lcd.setCursor(0, 1);
  lcd.print("Do not move");
}

void previousValue(int addr) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("P");
  lcd.setCursor(1, 0);
  lcd.print(addr);
  lcd.setCursor(2, 0);
  lcd.print(" - ");
  lcd.setCursor(5, 0);
  int hh = (readFromEEPROM(addr) / 60);
  int mm = (readFromEEPROM(addr) % 60);
  if (hh <= 9) {
    lcd.print("0");
    lcd.setCursor(6, 0);
  }
  lcd.print(hh);
  lcd.setCursor(7, 0);
  lcd.print(":");
  lcd.setCursor(8, 0);
  if (mm <= 9) {
    lcd.print("0");
    lcd.setCursor(9, 0);
  }
  lcd.print(mm);
  lcd.setCursor(0, 1);
  lcd.print("PREV        NEXT");
}

void recordingDone() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Done. Time");
  lcd.setCursor(0, 1);
  lcd.print("Rec. Started");
}

//Declare and Define ISR for CTC Interrupt
ISR(TIMER1_COMPA_vect) {
  if (checkVibration()) {
    secondRegister++;
    if (secondRegister >= 60) {
      minuteRegister++;
      secondRegister = 0;
    }
    TCNT1 = 0x00;
  }
}