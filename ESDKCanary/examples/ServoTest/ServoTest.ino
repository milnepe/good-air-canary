/***************************************************
  Test client for DesgnSpark ESDK canary
  Use this to adjust servo positions and test sound board

  Open the serial monitor to start cycle!
  Press reset on Arduino to re-start cycle

  Version: 3.0
  Date: 8 May 2022
  Author: Peter Milne

  Copyright 2022 Peter Milne
  Released under GNU GENERAL PUBLIC LICENSE
  Version 3, 29 June 2007

 ****************************************************/

#include "ESDKCanary.h"

// Wing positions - adjust as required
// If the servo is chattering at the end positions,
// adjust the min or max value by 5ish
#define WINGS_DOWN 485  // Max position
#define WINGS_UP_A_BIT 400
#define WINGS_UP_A_LOT 300
#define PASS_OUT_POS 225
#define DEAD_POS 150  // Min position

// Audio track numbers
#define YAWN_TRACK 0
#define STUFFY_TRACK 1
#define OPEN_WINDOW_TRACK 2
#define PASS_OUT_TRACK 3
#define THATS_BETTER_TRACK 4
#define DEAD_TRACK 5

#define TIME_DELAY 10000 // time delay in ms

#define SERVO 0  // Flapping servo
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz

#define SFX_RST 4 // Sound board RST pin

// Servo board - default address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Sound board connected to Serial1 - must be set to 9600 baud
Adafruit_Soundboard sfx = Adafruit_Soundboard(&Serial1, NULL, SFX_RST);

ESDKCanary myCanary = ESDKCanary(&sfx, &pwm, SERVO);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect
  }
  Serial.println("Serial monitor attached");

  // Serial 1 used for sound board at 9600 baud
  Serial1.begin(9600);
  while (!Serial1) {
    ; // wait for serial port to connect
  }
  Serial.println("Serial1 attached");

  // Init sound board
  if (!sfx.reset()) {
    Serial.println("SFX board not found");
  }
  else Serial.println("SFX board attached");
  myCanary.Tweet(YAWN_TRACK);

  // Init servo
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates
  myCanary.StartPos(WINGS_DOWN);
  Serial.println("Servo initialised");

  Serial.println("Servo test");

  delay(TIME_DELAY);
}

void loop() {
  myCanary.Tweet(STUFFY_TRACK);
  Serial.println("Flapping...");
  myCanary.Flap(WINGS_DOWN, WINGS_UP_A_BIT, VSLOW, 3);
  // Displays pulse length at end of movement
  Serial.println(myCanary.getPulselen());
  delay(TIME_DELAY);

  myCanary.Tweet(OPEN_WINDOW_TRACK);
  Serial.println("Flapping frantically...");
  myCanary.Flap(WINGS_DOWN, WINGS_UP_A_LOT, FAST, 4);
  Serial.println(myCanary.getPulselen());
  delay(TIME_DELAY);

  myCanary.Tweet(PASS_OUT_TRACK);
  Serial.println("Passing out...");
  myCanary.PassOut(PASS_OUT_POS, 2);
  Serial.println(myCanary.getPulselen());
  delay(TIME_DELAY);

  myCanary.Tweet(THATS_BETTER_TRACK);
  Serial.println("Returning to start...");
  myCanary.StartPos(WINGS_DOWN);
  Serial.println(myCanary.getPulselen());
  delay(TIME_DELAY);

  myCanary.Tweet(DEAD_TRACK);
  Serial.println("Dead...");
  myCanary.Dead(DEAD_POS, 1);
  Serial.println(myCanary.getPulselen());
  delay(TIME_DELAY);

  Serial.println("Returning to start...");
  myCanary.StartPos(WINGS_DOWN);
  Serial.println(myCanary.getPulselen());
  delay(TIME_DELAY);

  Serial.println("Press reset to repeat test");
  while (1);
}
