#ifndef _ESDK_DEVICE_DISPLAY_H_
#define _ESDK_DEVICE_DISPLAY_H_

class DeviceDisplay {
  public:
  DeviceDisplay(void);
  virtual void initDisplay(void) = 0;
  virtual void clearDisplay(void) = 0;
  virtual void updateDisplay(void) = 0;
  virtual void showGreeting(void) = 0;
};

#endif
