#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_Soundboard.h>

#ifndef _ESDK_CANARY_H_
#define _ESDK_CANARY_H_

#define WINGS_MID 470

enum flap_speeds {VFAST = 1, FAST, SLOW, VSLOW};

class ESDKCanary {
  public:
    ESDKCanary(Adafruit_PWMServoDriver *pwm, int servo);
    ESDKCanary(Adafruit_Soundboard *sfx, Adafruit_PWMServoDriver *pwm, int servo);
 private:
    Adafruit_PWMServoDriver *_pwm;
    Adafruit_Soundboard *_sfx;
    int _servo;
    uint16_t _pulselen;
 public:
    uint16_t getPulselen();
    void StartPos(uint16_t start_pos);
    void Flap(uint16_t down_pos, uint16_t up_pos, int speed_idx, int flaps);
    void PassOut(uint16_t end_pos, int speed_idx);
    void Dead(uint16_t end_pos, int speed_idx);
    void Tweet(uint8_t track, boolean audio);
};

#endif
