/***************************************************
  An Arduino IoT client for DesgnSpark ESDK
  that reacts to environmental CO2 levels

  Version: 0.8
  Date: 06 December 2021
  Author: Peter Milne

  Copywrite 2021 Peter Milne
  Released under GNU GENERAL PUBLIC LICENSE
  Version 3, 29 June 2007

 ****************************************************/

#include <WiFiNINA.h>
#include "arduino_secrets.h"
#include "ESDKCanary.h"
#include <SPI.h>
#include "epd2in9_V2.h"
#include "epdpaint.h"
#include "rslogo.h"
#include "tombstone.h"
#include <PubSubClient.h>
#include "ArduinoJson.h"

// CO2 settings - change to suit your environment
#define STUFFY_CO2 1000  // CO2 ppm
#define OPEN_WINDOW_CO2 2000
#define PASS_OUT_CO2 3000
#define DEAD_CO2 4000

// Interval between audio / phisical warings - change if it's
// nagging you too often
unsigned long DELAY_TIME = 20000;  // 20 seconds

// ESDK MQTT server name or IP address
//const char broker[] = "192.168.0.75";
const char broker[] = "airquality";
int        port     = 1883;

// ESDK topic root
#define TOPIC "airquality/#"

// Servo enumeration
#define SERVO 0  // Flapping servo
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz

// Connect to the RST pin on the Sound Board
#define SFX_RST 4

#define COLORED     0
#define UNCOLORED   1

unsigned char image[1024];
Paint paint(image, 0, 0);    // width should be the multiple of 8
Epd epd; // default reset: 8, dc: 9, cs: 10, busy: 7

/////// Enter sensitive data in arduino_secrets.h
char ssid[] = SECRET_SSID;  // Network SSID
char pass[] = SECRET_PASS;  // WPA key

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();  // Default address 0x40

Adafruit_Soundboard sfx = Adafruit_Soundboard(&Serial1, NULL, SFX_RST);

ESDKCanary myCanary = ESDKCanary();

unsigned long delayStart = 0; // the time the delay started
bool delayRunning = false; // true if still waiting for delay to finish

int co2 = 0;
double temperature = 0;
double humidity = 0;
int tvoc = 0;
int pm = 0;

bool updateDisplayFlag = false;
bool tombstoneFlag = false;

uint16_t pulselen = 0;
int prevSensorValue = 0;

enum states {NORMAL, STUFFY, OPEN_WINDOW, PASS_OUT, THATS_BETTER, DEAD} state = NORMAL;
enum states previousState = NORMAL;

void setup() {
  Serial.begin(115200);
//  while (!Serial) {
//    ; // wait for serial port to connect
//  }
  Serial.println("Canary Controller test!");

  Serial1.begin(9600);
  while (!Serial1) {
    ; // wait for serial port to connect
  }
  Serial.println("Serial1 attached");

  //  EDP stuff
  if (epd.Init() != 0) {
    Serial.print("e-Paper init failed");
    return;
  }
  Serial.println("EDP attached");

  epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black
  epd.DisplayFrame();

  // Remove power during this delay to shut down with blank screen
  delay(5000);

  if (epd.Init() != 0) {
    Serial.print("e-Paper init failed ");
    return;
  }

  /**
      there are 2 memory areas embedded in the e-paper display
      and once the display is refreshed, the memory area will be auto-toggled,
      i.e. the next action of SetFrameMemory will set the other memory area
      therefore you have to set the frame memory and refresh the display twice.
  */
  epd.SetFrameMemory_Base(RSLOGO);
  epd.DisplayFrame();

  updateEPDGreeting();

  // Setup sound board
  if (!sfx.reset()) {
    Serial.println("SFX board not found");
    //while (1);
  }
  else Serial.println("SFX board attached");
  myCanary.Tweet(sfx, YAWN_TRACK);

  // Setup servo
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates
  myCanary.ServoInit(pwm, SERVO);
  Serial.println("Servo initialised");

  // Attempt connection to Wifi network
  Serial.print("Connecting to Wifi: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // Retry
    Serial.print(".");
    delay(5000);
  }

  Serial.println("You're connected to the network");
  Serial.println();

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  mqttClient.setServer(broker, port);
  mqttClient.setCallback(callback);
  mqttClient.setBufferSize(384);

  delayStart = millis();
  delayRunning = true;

  delay(2000);
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();

  if (tombstoneFlag) {
    displayTombStone();
    delay(5000);
    displayClear();
    while (1);// Program ends!! Reboot
  }
  else if (updateDisplayFlag) {
    updateDisplayFlag = false;
    updateEPD();
  }

  if (delayRunning && ((millis() - delayStart) >= DELAY_TIME)) {
    delayStart += DELAY_TIME; // this prevents drift in the delays

    switch (state = getState(previousState, co2)) {
      case 0: // Normal do nothing
        break;
      case 1: // Stuffy
        Serial.println("Stuffy...");
        myCanary.Tweet(sfx, STUFFY_TRACK);
        myCanary.Flap(pwm, SERVO);
        myCanary.Flap(pwm, SERVO);
        myCanary.Flap(pwm, SERVO);
        break;
      case 2: // Open a window
        Serial.println("Open a window...");
        myCanary.Tweet(sfx, OPEN_WINDOW_TRACK);
        myCanary.OpenWindow(pwm, SERVO);
        myCanary.OpenWindow(pwm, SERVO);
        myCanary.OpenWindow(pwm, SERVO);
        break;
      case 3: // Pass out
        Serial.println("Pass out...");
        myCanary.Tweet(sfx, PASS_OUT_TRACK);
        myCanary.PassOut(pwm, SERVO);
        break;
      case 4: // Thats better
        Serial.println("That's better...");
        myCanary.Tweet(sfx, THATS_BETTER_TRACK);
        myCanary.ThatsBetter(pwm, SERVO);
        break;
      case 5: // Dead
        Serial.println("Dead...");
        myCanary.Tweet(sfx, DEAD_TRACK);
        myCanary.Dead(pwm, SERVO);
        tombstoneFlag = true;
    }
    previousState = state;
  }
}

// Sets the rules for changing state
states getState(states previousState, int co2) {
  states state;
  if ((previousState == PASS_OUT) && (co2 < PASS_OUT_CO2)) {
    state = THATS_BETTER;
  }
  else if (co2 < STUFFY_CO2) {
    state = NORMAL;
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
  return state;
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect("arduinoClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.subscribe("airquality/#");
    } else {
      Serial.print("Failed: ");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// Update sensor variables each time a message is received
void callback(char* topic, byte * payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // ESDK sends a large JSON payload
  // - ensure you have enough memory allocated
  StaticJsonDocument<384> doc;
  deserializeJson(doc, payload, length);
  co2 = doc["co2"]["co2"];
  temperature = doc["thv"]["temperature"];
  humidity = doc["thv"]["humidity"];
  tvoc = doc["thv"]["vocIndex"];
  pm = doc["pm"]["pm2.5"];

  updateDisplayFlag = true;
}

void updateEPDGreeting() {
  paint.SetWidth(120);
  paint.SetHeight(32);
  paint.SetRotate(ROTATE_180);

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 4, " Good Air", &Font16, COLORED);
  epd.SetFrameMemory_Partial(paint.GetImage(), 0, 140, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 4, "  Canary  ", &Font16, COLORED);
  epd.SetFrameMemory_Partial(paint.GetImage(), 0, 120, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 4, "Concept:", &Font16, COLORED);
  epd.SetFrameMemory_Partial(paint.GetImage(), 0, 80, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 4, "Jude Pullen", &Font16, COLORED);
  epd.SetFrameMemory_Partial(paint.GetImage(), 0, 60, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 0, "Code:", &Font16, COLORED);
  epd.SetFrameMemory_Partial(paint.GetImage(), 0, 20, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 0, "Pete Milne", &Font16, COLORED);
  epd.SetFrameMemory_Partial(paint.GetImage(), 0, 0, paint.GetWidth(), paint.GetHeight());

  epd.DisplayFrame_Partial();
}

void displayTombStone() {
  Serial.println("Tombstone!...");
  delay(2000);
  if (epd.Init() != 0) {
    Serial.print("e-Paper init failed ");
    return;
  }
  epd.SetFrameMemory_Base(TOMBSTONE);
  epd.DisplayFrame();
  delay(5000);
}

void displayClear() {
  Serial.println("Clearing display...");
  delay(2000);
  if (epd.Init() != 0) {
    Serial.print("e-Paper init failed ");
    return;
  }
  epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black
  epd.DisplayFrame();
  delay(2000);
  epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black
  epd.DisplayFrame();
  delay(2000);
}

void updateEPD() {
  if (co2 > 9999) {
    co2 = 0;
  }
  char CO2_string[] = {'0', '0', '0', '0', 'p', 'p', 'm', '\0'};
  CO2_string[0] = co2 / 100 / 10 + '0';
  CO2_string[1] = co2 / 100 % 10 + '0';
  CO2_string[2] = co2 % 100 / 10 + '0';
  CO2_string[3] = co2 % 100 % 10 + '0';

  int temp = 0;
  if (temperature < 99.9) {
    temp = (int)(temperature * 10);
    Serial.println(temp);
  }
  char TEMP_string[] = {'0', '0', '.', '0', 'C', '\0'};
  TEMP_string[0] = temp / 10 / 10 + '0';
  TEMP_string[1] = temp / 10 % 10 + '0';
  TEMP_string[3] = temp % 10 + '0';

  int hum = 0;
  if (humidity < 99.9) {
    hum = (int)(humidity * 10);
  }
  char RH_string[] = {'0', '0', '.', '0', '%', '\0'};
  RH_string[0] = hum / 10 / 10 + '0';
  RH_string[1] = hum / 10 % 10 + '0';
  RH_string[3] = hum % 10 + '0';

  if (tvoc > 9999) {
    tvoc = 0;
  }
  char TVOC_string[] = {'0', '0', '0', '0', 'p', 'p', 'm', '\0'};
  TVOC_string[0] = tvoc / 100 / 10 + '0';
  TVOC_string[1] = tvoc / 100 % 10 + '0';
  TVOC_string[2] = tvoc % 100 / 10 + '0';
  TVOC_string[3] = tvoc % 100 % 10 + '0';

  if (pm > 9) {
    pm = 0;
  }
  char PART_string[] = {'0', '\0'};
  PART_string[0] = pm % 100 % 10 + '0';

  paint.SetWidth(120);
  paint.SetHeight(32);
  paint.SetRotate(ROTATE_180);

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 4, "CO2", &Font24, COLORED);
  epd.SetFrameMemory_Partial(paint.GetImage(), 0, 260, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 4, CO2_string, &Font20, COLORED);
  epd.SetFrameMemory_Partial(paint.GetImage(), 0, 240, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 4, "TEMP", &Font24, COLORED);
  epd.SetFrameMemory_Partial(paint.GetImage(), 0, 210, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 4, TEMP_string, &Font20, COLORED);
  epd.SetFrameMemory_Partial(paint.GetImage(), 0, 190, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 0, "RH", &Font24, COLORED);
  epd.SetFrameMemory_Partial(paint.GetImage(), 0, 160, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 0, RH_string, &Font20, COLORED);
  epd.SetFrameMemory_Partial(paint.GetImage(), 0, 140, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 0, "TVOC", &Font24, COLORED);
  epd.SetFrameMemory_Partial(paint.GetImage(), 0, 110, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 0, TVOC_string, &Font20, COLORED);
  epd.SetFrameMemory_Partial(paint.GetImage(), 0, 90, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 0, "PM2.5", &Font20, COLORED);
  epd.SetFrameMemory_Partial(paint.GetImage(), 0, 60, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 0, PART_string, &Font20, COLORED);
  epd.SetFrameMemory_Partial(paint.GetImage(), 0, 40, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 0, "Wifi API", &Font20, COLORED);
  epd.SetFrameMemory_Partial(paint.GetImage(), 0, 0, paint.GetWidth(), paint.GetHeight());

  epd.DisplayFrame_Partial();
}
