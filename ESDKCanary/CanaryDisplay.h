//#include "DeviceDisplay.h"
#include "epd2in9_V2.h"
#include "epdpaint.h"
#include "tombstone.h"
#include "rslogo.h"

#ifndef _ESDK_CANARY_DISPLAY_H_
#define _ESDK_CANARY_DISPLAY_H_

#define COLORED     0
#define UNCOLORED   1

class CanaryDisplay {
  public:
  unsigned char image[1024];
  Epd _epd; // default reset: 8, dc: 9, cs: 10, busy: 7
  Paint _paint = Paint(image, 0, 0);
  CanaryDisplay(void);
  void initDisplay(void);
  void clearDisplay(void);
  void updateDisplay(int co2, double temperature, double humidity, int tvoc, int pm);
  void showGreeting(void);
  void showTombStone(void);
};

#endif
