#include "ESDKCanary.h"

ESDKCanary::ESDKCanary(Adafruit_PWMServoDriver *pwm, int servo) {
  _pwm = pwm;
  _servo = servo;
  _pulselen = WINGS_MID;
}

uint16_t ESDKCanary::getPulselen(void) {
    return _pulselen;
}

void ESDKCanary::StartPos(uint16_t start_pos) {
  // Move wings to start position
  for (; _pulselen < start_pos; _pulselen++) {
    _pwm->setPWM(_servo, 0, _pulselen);
    delay(3);
  }
}

void ESDKCanary::Flap(uint16_t down_pos, uint16_t up_pos, int flaps, int speed_idx) {
  // Move wings back and fourth
  // Note that the value of pulselen is inverted!
  // speed_idx  0 is fastest

  for (int i = 0; i < flaps; i++) {
    // Up
    for (; _pulselen > up_pos; _pulselen--) {
      _pwm->setPWM(_servo, 0, _pulselen);
      delay(speed_idx);
    }
    delay(100);
    // Down
    for (; _pulselen < down_pos; _pulselen++) {
      _pwm->setPWM(_servo, 0, _pulselen);
      delay(speed_idx);
    }
    delay(100);
  }
}

void ESDKCanary::PassOut(uint16_t end_pos, int speed_idx) {
  // Move wings to pass out position and hold it
  // Note that the value of pulselen is inverted!

  for (; _pulselen > end_pos; _pulselen--) {
    _pwm->setPWM(_servo, 0, _pulselen);
    delay(speed_idx);
  }
}


void ESDKCanary::Dead(uint16_t end_pos, int speed_idx) {
  // Move servo past tipping point then retract wings.
  // This position should only be recovered by resetting the system

  for (; _pulselen > end_pos; _pulselen--) {
    _pwm->setPWM(_servo, 0, _pulselen);
    delay(speed_idx);
  }
}

/*
//void ESDKCanary::Tweet(Adafruit_Soundboard sfx, uint8_t track) {
  //Serial.println("Playing track");
  //if (! sfx.playTrack(track)) {
    //Serial.println("Failed to play track?");
  //}
//}
*/
