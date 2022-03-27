#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_Soundboard.h>

#ifndef _ESDK_CANARY_H_
#define _ESDK_CANARY_H_

#define WINGS_DOWN 495
#define WINGS_UP 445
#define PASS_OUT_POS 225
#define DEAD_POS 150

// Tracks
#define YAWN_TRACK 0
#define STUFFY_TRACK 1
#define OPEN_WINDOW_TRACK 2
#define PASS_OUT_TRACK 3
#define THATS_BETTER_TRACK 4
#define DEAD_TRACK 5

class ESDKCanary {
  public:
    ESDKCanary();

    void ServoInit(Adafruit_PWMServoDriver pwm, int servo);
    void SlowInit(Adafruit_PWMServoDriver pwm, int servo);
    void Flap(Adafruit_PWMServoDriver pwm, int servo);
    void OpenWindow(Adafruit_PWMServoDriver pwm, int servo);
    void PassOut(Adafruit_PWMServoDriver pwm, int servo);
    void ThatsBetter(Adafruit_PWMServoDriver pwm, int servo);
    void Dead(Adafruit_PWMServoDriver pwm, int servo);
    void Tweet(Adafruit_Soundboard sfx, uint8_t track);
};

#endif
