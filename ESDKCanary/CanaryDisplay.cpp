#include "CanaryDisplay.h"

void CanaryDisplay::initDisplay(void) {
  if (_epd.Init() != 0) {
    return;
  }
  Serial.println("EDP attached");

  _epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black
  _epd.DisplayFrame();

  delay(2000);

  _epd.SetFrameMemory_Base(RSLOGO);
  _epd.DisplayFrame();
}

void CanaryDisplay::showTombStone() {
  if (_epd.Init() != 0) {
    return;
  }

  delay(2000);

  _epd.SetFrameMemory_Base(TOMBSTONE);
  _epd.DisplayFrame();
}

void CanaryDisplay::updateDisplay() {
  if (_canary->co2 > 9999) {
    _canary->co2 = 0;
  }
  if (_canary->co2 >= DEAD_CO2) {
    showTombStone();
    return;
  }
  char CO2_string[] = {'0', '0', '0', '0', 'p', 'p', 'm', '\0'};
  CO2_string[0] = _canary->co2 / 100 / 10 + '0';
  CO2_string[1] = _canary->co2 / 100 % 10 + '0';
  CO2_string[2] = _canary->co2 % 100 / 10 + '0';
  CO2_string[3] = _canary->co2 % 100 % 10 + '0';

  int temp = 0;
  if (_canary->temperature < 99.9) {
    temp = (int)(_canary->temperature * 10);
  }
  char TEMP_string[] = {'0', '0', '.', '0', 'C', '\0'};
  TEMP_string[0] = temp / 10 / 10 + '0';
  TEMP_string[1] = temp / 10 % 10 + '0';
  TEMP_string[3] = temp % 10 + '0';

  int hum = 0;
  if (_canary->humidity < 99.9) {
    hum = (int)(_canary->humidity * 10);
  }
  char RH_string[] = {'0', '0', '.', '0', '%', '\0'};
  RH_string[0] = hum / 10 / 10 + '0';
  RH_string[1] = hum / 10 % 10 + '0';
  RH_string[3] = hum % 10 + '0';

  if (_canary->tvoc > 9999) {
    _canary->tvoc = 0;
  }
  char TVOC_string[] = {'0', '0', '0', '0', 'p', 'p', 'm', '\0'};
  TVOC_string[0] = _canary->tvoc / 100 / 10 + '0';
  TVOC_string[1] = _canary->tvoc / 100 % 10 + '0';
  TVOC_string[2] = _canary->tvoc % 100 / 10 + '0';
  TVOC_string[3] = _canary->tvoc % 100 % 10 + '0';

  if (_canary->pm > 9999) {
    _canary->pm = 0;
  }
  char PART_string[] = {'0', '0', '0', '0', '\0'};
  PART_string[0] = _canary->pm / 100 / 10 + '0';
  PART_string[1] = _canary->pm / 100 % 10 + '0';
  PART_string[2] = _canary->pm % 100 / 10 + '0';
  PART_string[3] = _canary->pm % 100 % 10 + '0';

  _paint.SetWidth(120);
  _paint.SetHeight(40);
  _paint.SetRotate(ROTATE_180);

  _paint.Clear(UNCOLORED);
  _paint.DrawStringAt(0, 4, "CO2", &Font24, COLORED);
  _epd.SetFrameMemory_Partial(_paint.GetImage(), 0, 250, _paint.GetWidth(), _paint.GetHeight());

  _paint.Clear(UNCOLORED);
  _paint.DrawStringAt(0, 4, CO2_string, &Font20, COLORED);
  _epd.SetFrameMemory_Partial(_paint.GetImage(), 0, 230, _paint.GetWidth(), _paint.GetHeight());

  _paint.Clear(UNCOLORED);
  _paint.DrawStringAt(0, 4, "TEMP", &Font24, COLORED);
  _epd.SetFrameMemory_Partial(_paint.GetImage(), 0, 200, _paint.GetWidth(), _paint.GetHeight());

  _paint.Clear(UNCOLORED);
  _paint.DrawStringAt(0, 4, TEMP_string, &Font20, COLORED);
  _epd.SetFrameMemory_Partial(_paint.GetImage(), 0, 180, _paint.GetWidth(), _paint.GetHeight());

  _paint.Clear(UNCOLORED);
  _paint.DrawStringAt(0, 0, "RH", &Font24, COLORED);
  _epd.SetFrameMemory_Partial(_paint.GetImage(), 0, 150, _paint.GetWidth(), _paint.GetHeight());

  _paint.Clear(UNCOLORED);
  _paint.DrawStringAt(0, 0, RH_string, &Font20, COLORED);
  _epd.SetFrameMemory_Partial(_paint.GetImage(), 0, 130, _paint.GetWidth(), _paint.GetHeight());

  _paint.Clear(UNCOLORED);
  _paint.DrawStringAt(0, 0, "TVOC", &Font24, COLORED);
  _epd.SetFrameMemory_Partial(_paint.GetImage(), 0, 100, _paint.GetWidth(), _paint.GetHeight());

  _paint.Clear(UNCOLORED);
  _paint.DrawStringAt(0, 0, TVOC_string, &Font20, COLORED);
  _epd.SetFrameMemory_Partial(_paint.GetImage(), 0, 80, _paint.GetWidth(), _paint.GetHeight());

  _paint.Clear(UNCOLORED);
  _paint.DrawStringAt(0, 0, "PM2.5", &Font20, COLORED);
  _epd.SetFrameMemory_Partial(_paint.GetImage(), 0, 50, _paint.GetWidth(), _paint.GetHeight());

  _paint.Clear(UNCOLORED);
  _paint.DrawStringAt(0, 0, PART_string, &Font20, COLORED);
  _epd.SetFrameMemory_Partial(_paint.GetImage(), 0, 30, _paint.GetWidth(), _paint.GetHeight());

  _paint.Clear(UNCOLORED);

  if (_canary->demoOn) {
    _paint.DrawStringAt(0, 0, "Demo Mode", &Font16, COLORED);
  }
  else if (_canary->audioOn && _canary->wifiOn) {
    _paint.DrawStringAt(0, 0, "Wifi Audio", &Font16, COLORED);
  }
  else if (_canary->wifiOn) {
    _paint.DrawStringAt(0, 0, "Wifi", &Font16, COLORED);
  }
  else if (_canary->audioOn) {
    _paint.DrawStringAt(0, 0, "Audio", &Font16, COLORED);
  }
  else {
    _paint.DrawStringAt(0, 0, "", &Font16, COLORED);
  }
  _epd.SetFrameMemory_Partial(_paint.GetImage(), 0, 0, _paint.GetWidth(), _paint.GetHeight());

  _epd.DisplayFrame_Partial();
}

void CanaryDisplay::showGreeting(void) {
  _paint.SetWidth(120);
  _paint.SetHeight(32);
  _paint.SetRotate(ROTATE_180);

  _paint.Clear(UNCOLORED);
  _paint.DrawStringAt(0, 4, " Good Air", &Font16, COLORED);
  _epd.SetFrameMemory_Partial(_paint.GetImage(), 0, 140, _paint.GetWidth(), _paint.GetHeight());

  _paint.Clear(UNCOLORED);
  _paint.DrawStringAt(0, 4, "  Canary  ", &Font16, COLORED);
  _epd.SetFrameMemory_Partial(_paint.GetImage(), 0, 120, _paint.GetWidth(), _paint.GetHeight());

  _paint.Clear(UNCOLORED);
  _paint.DrawStringAt(0, 4, "Concept:", &Font16, COLORED);
  _epd.SetFrameMemory_Partial(_paint.GetImage(), 0, 80, _paint.GetWidth(), _paint.GetHeight());

  _paint.Clear(UNCOLORED);
  _paint.DrawStringAt(0, 4, "Jude Pullen", &Font16, COLORED);
  _epd.SetFrameMemory_Partial(_paint.GetImage(), 0, 60, _paint.GetWidth(), _paint.GetHeight());

  _paint.Clear(UNCOLORED);
  _paint.DrawStringAt(0, 0, "Code:", &Font16, COLORED);
  _epd.SetFrameMemory_Partial(_paint.GetImage(), 0, 20, _paint.GetWidth(), _paint.GetHeight());

  _paint.Clear(UNCOLORED);
  _paint.DrawStringAt(0, 0, "Pete Milne", &Font16, COLORED);
  _epd.SetFrameMemory_Partial(_paint.GetImage(), 0, 0, _paint.GetWidth(), _paint.GetHeight());

  _epd.DisplayFrame_Partial();
}
