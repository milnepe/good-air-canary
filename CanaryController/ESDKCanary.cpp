#include "ESDKCanary.h"

ESDKCanary::ESDKCanary() {}

void ESDKCanary::ServoInit(Adafruit_PWMServoDriver pwm, int servo) {
  pwm.setPWM(servo, 0, WINGS_DOWN);
}

void ESDKCanary::Flap(Adafruit_PWMServoDriver pwm, int servo) {
  // Move wings from start position (down) to up position
  // and back to start position
  // Note that the value of pulselen is inverted!

  uint16_t pulselen = WINGS_DOWN;

  // Up
  for (; pulselen > WINGS_UP; pulselen--) {
    pwm.setPWM(servo, 0, pulselen);
    delay(3);
  }

  // Down
  for (; pulselen < WINGS_DOWN; pulselen++) {
    pwm.setPWM(servo, 0, pulselen);
    delay(3);
  }
}

void ESDKCanary::OpenWindow(Adafruit_PWMServoDriver pwm, int servo) {
  // Move wings from start position (down) to up position
  // and back to start position
  // Note that the value of pulselen is inverted!

  uint16_t pulselen = WINGS_DOWN;

  // Up
  for (; pulselen > WINGS_UP; pulselen--) {
    pwm.setPWM(servo, 0, pulselen);
    delay(3);
  }

  // Down
  for (; pulselen < WINGS_DOWN; pulselen++) {
    pwm.setPWM(servo, 0, pulselen);
    delay(3);
  }
}

void ESDKCanary::PassOut(Adafruit_PWMServoDriver pwm, int servo) {
  // Move wings from start position (down) to pass out position
  // and hold it
  // Note that the value of pulselen is inverted!

  uint16_t pulselen = WINGS_DOWN;

  // Up
  for (; pulselen > PASS_OUT_POS; pulselen--) {
    pwm.setPWM(servo, 0, pulselen);
    delay(3);
  }
}

void ESDKCanary::ThatsBetter(Adafruit_PWMServoDriver pwm, int servo) {
  // Move wings from pass out position to start position
  // Note that the value of pulselen is inverted!

  uint16_t pulselen = PASS_OUT_POS;

  // Up
  for (; pulselen < WINGS_DOWN; pulselen++) {
    pwm.setPWM(servo, 0, pulselen);
    delay(3);
  }  
}

void ESDKCanary::Dead(Adafruit_PWMServoDriver pwm, int servo) {
  // Move servo past tipping point and enter an infinite loop
  // which can only be recovered by resetting the system

  uint16_t pulselen = WINGS_DOWN;

  for (; pulselen > DEAD_POS; pulselen--) {
    pwm.setPWM(servo, 0, pulselen);
    delay(3);
  }

  delay(2000);

  for (; pulselen < WINGS_UP; pulselen++) {
    pwm.setPWM(servo, 0, pulselen);
    delay(3);
  }
}

void ESDKCanary::Tweet(Adafruit_Soundboard sfx, uint8_t track) {
  Serial.println("Playing track");
  if (! sfx.playTrack(track)) {
    Serial.println("Failed to play track?");
  }
}
