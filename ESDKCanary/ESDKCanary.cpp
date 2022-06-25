#include "ESDKCanary.h"

ESDKCanary::ESDKCanary(Adafruit_Soundboard *sfx, Adafruit_PWMServoDriver *pwm, int servo)
  :ESDKCanary(pwm, servo) {
  _sfx = sfx;
}

ESDKCanary::ESDKCanary(Adafruit_PWMServoDriver *pwm, int servo) {
  _pwm = pwm;
  _servo = servo;
  _pulselen = WINGS_START;
}

void ESDKCanary::updateCanary() {
  switch (state) {
    case THATS_BETTER:
      Tweet(THATS_BETTER_TRACK, audioOn);
      StartPos(WINGS_DOWN);
      break;
    case STUFFY:
      Tweet(STUFFY_TRACK, audioOn);
      Flap(WINGS_DOWN, WINGS_UP_A_BIT, VSLOW, 3);
      break;
    case OPEN_WINDOW:
      Tweet(OPEN_WINDOW_TRACK, audioOn);
      Flap(WINGS_DOWN, WINGS_UP_A_LOT, FAST, 4);
      break;
    case PASS_OUT:
      Tweet(PASS_OUT_TRACK, audioOn);
      PassOut(PASS_OUT_POS, FAST);
      break;
    case DEAD:
      if (!demoOn) {
        Tweet(DEAD_TRACK, audioOn);
        Dead(DEAD_POS, VFAST);
        while (1);// Program ends!! Reboot
      } else {
        StartPos(WINGS_DOWN);
        delay(2000);
        Tweet(DEAD_TRACK, audioOn);
        PassOut(PASS_OUT_POS, FAST);
        StartPos(WINGS_DOWN);
      }
  }
}

// Sets the rules for changing state
States ESDKCanary::updateState() {
  static States previousState = NORMAL;

  if ((previousState == PASS_OUT) && (co2 < PASS_OUT_CO2)) {
    state = THATS_BETTER;
  }
  else if ((previousState == OPEN_WINDOW) && (co2 < OPEN_WINDOW_CO2)) {
    state = THATS_BETTER;
  }
  else if (co2 < STUFFY_CO2) {
    state = THATS_BETTER;
  }
  else if (co2 < OPEN_WINDOW_CO2) {
    state = STUFFY;
  }
  else if (co2 < PASS_OUT_CO2) {
    state = OPEN_WINDOW;
  }
  else if (co2 < DEAD_CO2) {
    state = PASS_OUT;
  }
  else state = DEAD;

  // Update canary if state has changed
  if ( state != previousState ) {
    previousState = state;
    updateCanary();
  }
  return state;
}

void ESDKCanary::StartPos(uint16_t start_pos) {
  // Move wings to start position
  for (; _pulselen < start_pos; _pulselen++) {
    _pwm->setPWM(_servo, 0, _pulselen);
    delay(VSLOW);
  }
}

void ESDKCanary::Flap(uint16_t down_pos, uint16_t up_pos, int speed_idx, int flaps) {
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

void ESDKCanary::Tweet(uint8_t track, boolean audio = true) {
  if (audio) {
    if (! _sfx->playTrack(track)) {
      ;
    }
  }
}
