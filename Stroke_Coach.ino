// MPU-6050 Short Example Sketch
//www.elegoo.com
//2016.12.9

#include <Wire.h>
#include <LiquidCrystal.h>
#include <time.h>

#define LED 3

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

const int MPU_addr=0x68;  // I2C address of the MPU-6050
int buttonPin = 13, buzzerPin = 4;

// stroke coach state
// current states:
// 0  welcome
// 1  rate + timer
// 2  go!
// 3  attention
// 4  ready
int currentState = 0, lastTickState = 0, startTime = 0, countdownTime = 0;

// rate calculation
long int dif = 0, lastDif = 0;
int rates[3] = {0, 0, 0};
float rate = 0;
bool lastSign = false;

// timer
long int timerTime = 0, timerStartTime = 0, pauseStartTime = 0, buttonPressStartTime = 0;
long int buttonStartPress = 0;
bool buttonPressed = false, timing = false, paused = false;

// MPU
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;

// set up gyro
void gyroInit(){
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  Serial.begin(9600);
}

// set up button pin
void buttonInit(){
  pinMode(buttonPin, INPUT_PULLUP); 
}

void buzzerInit(){
  pinMode(buzzerPin, OUTPUT);
}

// set up led
void ledInit(){
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
}

// lcd set states
void lcdSetWelcome(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Welcome to Eli's");
  lcd.setCursor(0, 1);
  lcd.print(" stroke coach!   ");
}

void lcdSetStandard(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Rate: ");
  lcd.setCursor(0, 1);
  lcd.print("Timer: ");
}

void lcdSetRateAndTimer(){
  lcd.setCursor(6, 0);
  lcd.print(rate);

//  lcd.setCursor(7, 1);
//  lcd.print(timerTime);

  int timerTimeSecs = (timerTime / 1000) % 60;
  int timerTimeMins = timerTime / 60000;
  lcd.setCursor(7, 1);
  if (timerTimeMins < 1) lcd.print("00");
  else if (timerTimeMins < 10) { lcd.setCursor(8, 1); lcd.print(timerTimeMins); }
  else lcd.print(timerTimeMins);
  lcd.setCursor(9, 1);
  lcd.print(":");
  if (timerTimeSecs < 10){
    lcd.setCursor(10, 1);
    lcd.print("0");
    lcd.setCursor(11, 1);
    lcd.print(timerTimeSecs);
  }
  else{
    lcd.setCursor(10, 1);
    lcd.print(timerTimeSecs);
  }
}

void lcdSetReady(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Sit ready...");
}

void lcdSetAttention(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   Attention");
}

void lcdSetGo(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("++++++Row!++++++");
  lcd.setCursor(0, 1);
  lcd.print("rowrowrowrowrow");
}

// set up lcd
void lcdInit(){
  lcd.begin(16, 2);
  lcdSetWelcome();
}

//output a frequency
void playFrequency(int amount, int buzzDelay){
  for (int i = 0; i < amount; i++)
  {
    digitalWrite(buzzerPin,HIGH);
    delay(buzzDelay);
    digitalWrite(buzzerPin,LOW);
    delay(buzzDelay);
  }
}

// entry point
void setup(){
  gyroInit();

  buttonInit();

  ledInit();

  lcdInit();

  buzzerInit();

  playFrequency(100, 2);
  playFrequency(200, 1);
  playFrequency(66, 3);
}

// updates rate
void addRate(int inpRate){
  rates[2] = rates[1];
  rates[1] = rates[0];
  rates[0] = inpRate;
  if (rates[2] != 0) rate = 180000/float(rates[0] + rates[1] + rates[2]);
}

// gyroscope manage
void gyroManage(){
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true);  // request a total of 14 registers
  AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
  AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
  dif = millis();

  Serial.print(dif); Serial.print(" | "); Serial.print(lastDif); Serial.print(" | "); Serial.println(dif - lastDif);
  if (lastSign == false && AcX >= 0 && (dif - lastDif) > 600){
    lastSign = true;
    addRate(dif - lastDif);
    lastDif = millis();
  }

  if (lastSign == true && AcX < 0){
    lastSign = false;
  }
}

// manages timer when button pressed
void toggleTimer(){
  // if timer not yet started at all
  if (!timing){
    timerStartTime = millis() + 4000;
    timerTime = millis() + 4000;
    paused = false;
    timing = true;
    countdownTime = 0;
    currentState = 4;
    startTime = millis();
  }
  // if timer has been started before
  else{
    // if paused
    if (paused){
      timerStartTime = timerStartTime + (millis() - pauseStartTime);
      paused = false;
      countdownTime = 0;
      currentState = 4;
      startTime = millis();
      timerStartTime = timerStartTime + 4000;
      timerTime = timerTime + 4000;
    }
    else if (!paused){
      pauseStartTime = millis();
      paused = true;
    }
  }
}

// resets timer to 0
void resetTimer(){
  timerTime = 0;
  timerStartTime = 0;
  pauseStartTime = 0;
  buttonPressStartTime = 0;
  timing = false;
  for (int i = 0; i < 3; i++) rates[i] = 0;
  rate = 0.0;
  lastDif = millis();

  lcd.setCursor(7, 1);
  lcd.print("00:00");

  lcd.setCursor(6, 0);
  lcd.print("0.00");
}

void buttonManage(){
  // if screen has already said go
  if (currentState > 0 && countdownTime > 4000){
    if (timing && !paused) timerTime = millis() - timerStartTime;
    
    // if button pressed
    if (digitalRead(buttonPin) == LOW){
      analogWrite(LED, 0);
      if (!buttonPressed){
        buttonPressStartTime = millis();
      }
      buttonPressed = true;
      
      // if button pressed for more than 3 seconds
      if (millis() - buttonPressStartTime >= 3000) resetTimer();
    }
  
    // if button not pressed
    else{
      if (buttonPressed == true && (millis() - buttonPressStartTime < 3000)) toggleTimer();
      analogWrite(LED, 255);
      buttonPressed = false;
    }
  }
}

// Screen update 
void screenManage(){
  if (lastTickState != currentState){
    switch (currentState){
      default:
        break;
  
      // welcome
      case 0: lcdSetWelcome(); break;
        
      // rate + timer
      case 1: lcdSetStandard(); break;
  
      // go!
      case 2: 
        lcdSetGo(); 
        playFrequency(200, 1); 
        lastDif = millis(); 
        for (int i = 0; i < 3; i++) rates[i] = 0;
        rate = 0.0;
        break;
  
      // attention
      case 3: lcdSetAttention();  playFrequency(100, 2); break;
  
      // ready
      case 4: lcdSetReady(); playFrequency(100, 2); break;
    }
  }
  if (currentState == 1) lcdSetRateAndTimer();
  lastTickState = currentState;
}

//main loop
void loop(){
  if (countdownTime >= 0 && countdownTime <= 5000) countdownTime = millis() - startTime;

  // coming off of welcome screen
  if (countdownTime > 3000 && currentState == 0) currentState = 1;

  // countdown state changes
  if (currentState == 4 && countdownTime > 2000){
    currentState = 3;
  }
  if (currentState == 3 && countdownTime > 4000){
    currentState = 2;
  }
  if (currentState == 2 && countdownTime > 5000){
    currentState = 1;
  }

  gyroManage();

  if (currentState > 0) buttonManage();

  screenManage();
  
  delay(100);
}
