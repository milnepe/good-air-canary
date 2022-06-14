#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_Soundboard.h>

#ifndef _ESDK_CANARY_H_
#define _ESDK_CANARY_H_

// CO2 levels ppm - change to suit your environment
#define NORMAL_CO2 400
#define STUFFY_CO2 1000
#define OPEN_WINDOW_CO2 2000
#define PASS_OUT_CO2 3000
#define DEAD_CO2 4000

// Audio track numbers
#define YAWN_TRACK 0
#define STUFFY_TRACK 1
#define OPEN_WINDOW_TRACK 2
#define PASS_OUT_TRACK 3
#define THATS_BETTER_TRACK 4
#define DEAD_TRACK 5

// Wing positions - adjust as required
// If the servo is chattering at the end positions,
// adjust the min or max value by 5ish
#define WINGS_START 470
#define WINGS_DOWN 480  // Max position
#define WINGS_UP_A_BIT 400
#define WINGS_UP_A_LOT 300
#define PASS_OUT_POS 225
#define DEAD_POS 150  // Min position

enum flap_speeds {VFAST = 1, FAST, SLOW, VSLOW};

enum States {NORMAL, STUFFY, OPEN_WINDOW, PASS_OUT, DEAD, THATS_BETTER};

class ESDKCanary {
  public:
    int co2 = 400;
    double temperature = 21.0;
    double humidity = 40.0;
    int tvoc = 100;
    int pm = 1;
    volatile bool wifiOn = false;
    volatile bool audioOn = true;
    volatile bool demoOn = false;
    States state = NORMAL;
    ESDKCanary(Adafruit_PWMServoDriver *pwm, int servo);
    ESDKCanary(Adafruit_Soundboard *sfx, Adafruit_PWMServoDriver *pwm, int servo);
 private:
    Adafruit_PWMServoDriver *_pwm;
    Adafruit_Soundboard *_sfx;
    int _servo;
    uint16_t _pulselen;
 public:
    void updateCanary();
    States updateState();
    void StartPos(uint16_t start_pos);
    void Flap(uint16_t down_pos, uint16_t up_pos, int speed_idx, int flaps);
    void PassOut(uint16_t end_pos, int speed_idx);
    void Dead(uint16_t end_pos, int speed_idx);
    void Tweet(uint8_t track, boolean audio);
};

#endif
