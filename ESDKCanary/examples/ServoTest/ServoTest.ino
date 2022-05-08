/***************************************************
  An servo test client for DesgnSpark ESDK canary

  Version: 3.0
  Date: 8 May 2022
  Author: Peter Milne

  Copyright 2022 Peter Milne
  Released under GNU GENERAL PUBLIC LICENSE
  Version 3, 29 June 2007

 ****************************************************/

#include "ESDKCanary.h"

#define WINGS_DOWN 485
//#define WINGS_MID 470
#define WINGS_UP_A_BIT 400
#define WINGS_UP_A_LOT 300
#define PASS_OUT_POS 225
#define DEAD_POS 150

// CO2 levels - change to suit your environment
#define STUFFY_CO2 1000  // CO2 ppm
#define OPEN_WINDOW_CO2 2000
#define PASS_OUT_CO2 3000
#define DEAD_CO2 4000

#define SERVO 0  // Flapping servo
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();  // Default address 0x40

ESDKCanary myCanary = ESDKCanary(&pwm, SERVO);

uint16_t pulselen = 0;
int prevSensorValue = 0;

enum states {NORMAL, STUFFY, OPEN_WINDOW, PASS_OUT, THATS_BETTER, DEAD} state = NORMAL;
enum states previousState = NORMAL;


void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect
  }

  // Setup servo
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates
  myCanary.StartPos(WINGS_DOWN);
  Serial.println("Servo initialised");

  Serial.println("Servo test");

  delay(1000);
}

void loop() {
  Serial.println("Flapping...");
  myCanary.Flap(WINGS_DOWN, WINGS_UP_A_BIT, 3, 4);
  Serial.println(myCanary.getPulselen());
  delay(2000);

  Serial.println("Flapping frantically...");
  myCanary.Flap(WINGS_DOWN, WINGS_UP_A_LOT, 4, 2);
  Serial.println(myCanary.getPulselen());  
  delay(2000);
  
  Serial.println("Passing out...");  
  myCanary.PassOut(PASS_OUT_POS, 2);
  Serial.println(myCanary.getPulselen());  
  delay(2000);
  
  Serial.println("Returning to start...");    
  myCanary.StartPos(WINGS_DOWN);
  Serial.println(myCanary.getPulselen());  
  delay(2000);

  Serial.println("Dead...");  
  myCanary.Dead(DEAD_POS, 1);
  Serial.println(myCanary.getPulselen());  
  delay(2000);

  Serial.println("Returning to start...");    
  myCanary.StartPos(WINGS_DOWN);
  Serial.println(myCanary.getPulselen());  
  delay(2000);    
}
