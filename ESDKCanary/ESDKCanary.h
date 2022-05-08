#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_Soundboard.h>

#ifndef _ESDK_CANARY_H_
#define _ESDK_CANARY_H_
#define WINGS_MID 470

// Tracks
#define YAWN_TRACK 0
#define STUFFY_TRACK 1
#define OPEN_WINDOW_TRACK 2
#define PASS_OUT_TRACK 3
#define THATS_BETTER_TRACK 4
#define DEAD_TRACK 5

class ESDKCanary {
  public:
    ESDKCanary(Adafruit_PWMServoDriver *pwm, int servo);
 private:
    Adafruit_PWMServoDriver *_pwm;
    int _servo;
    uint16_t _pulselen;
 public:
    uint16_t getPulselen();
    void StartPos(uint16_t start_pos);
    void Flap(uint16_t down_pos, uint16_t up_pos, int flaps, int speed_idx);
    void PassOut(uint16_t end_pos, int speed_idx);
    void Dead(uint16_t end_pos, int speed_idx);
    //void Tweet(Adafruit_Soundboard sfx, uint8_t track);
};

#endif
