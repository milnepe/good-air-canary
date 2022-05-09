/***************************************************
  An Arduino IoT client for DesgnSpark ESDK
  that reacts to environmental CO2 levels

  Version: 3.0
  Date: 8 May 2022
  Author: Peter Milne

  Copyright 2022 Peter Milne
  Released under GNU GENERAL PUBLIC LICENSE
  Version 3, 29 June 2007

 ****************************************************/
// Un-comment for debugging
// In debug mode the system will not run until a serial monitor
// is attached!
#define DEBUG

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

// CO2 levels - change to suit your environment
#define STUFFY_CO2 1000  // CO2 ppm
#define OPEN_WINDOW_CO2 2000
#define PASS_OUT_CO2 3000
#define DEAD_CO2 4000

#define NORMAL_DELAY 30 * 60 * 1000  // 30 mins default
#define DEMO_DELAY 10 * 1000    // 10 sec for debug

#ifdef DEBUG
volatile unsigned long delay_time = DEMO_DELAY;
#else
// Interval between audio / physical warnings - change if it's
// nagging you too often
unsigned long delay_time = NORMAL_DELAY;
#endif
unsigned long previousMillis = 0;

// ESDK MQTT server name
// You may need to substiture its IP address on your network
//const char broker[] = "192.168.0.75";
const char broker[] = "airquality";
int        port     = 1883;

// ESDK topic root
#define TOPIC "airquality/#"

#define COLORED     0
#define UNCOLORED   1

unsigned char image[1024];
Paint paint(image, 0, 0);    // width should be the multiple of 8
Epd epd; // default reset: 8, dc: 9, cs: 10, busy: 7

/////// Enter sensitive data in arduino_secrets.h
char ssid[] = SECRET_SSID;  // Network SSID
char pass[] = SECRET_PASS;  // WPA key

WiFiClient wifiClient;
long lastReconnectWIFIAttempt = 0;
PubSubClient mqttClient(wifiClient);
long lastReconnectMQTTAttempt = 0;

//unsigned long delayStart = 0; // time the delay started
bool delayRunning = false; // true if still waiting for delay to finish

volatile int co2 = 400;
double temperature = 21.0;
double humidity = 40.0;
int tvoc = 100;
int pm = 1;

volatile bool updateDisplayFlag = false;
bool tombstoneFlag = false;
int prevSensorValue = 0;

//enum states {NORMAL, STUFFY, OPEN_WINDOW, PASS_OUT, THATS_BETTER, DEAD, DEMO_DEAD} state = NORMAL;
//enum States {NORMAL, STUFFY, OPEN_WINDOW, PASS_OUT, THATS_BETTER, DEAD, DEMO_DEAD};
enum States {THATS_BETTER, STUFFY, OPEN_WINDOW, PASS_OUT, DEAD, DEMO_DEAD};

// Button code
enum buttons {LEFT_BUTTON = 2, RIGHT_BUTTON = 3, DEMO_BUTTON = 9};
volatile boolean audioOn = true;
volatile boolean demo_mode = false;
volatile boolean reentrant = true;

int wifiState = WL_IDLE_STATUS;

// Wing positions - adjust as required
// If the servo is chattering at the end positions,
// adjust the min or max value by 5ish
#define WINGS_DOWN 485  // Max position
#define WINGS_UP_A_BIT 400
#define WINGS_UP_A_LOT 300
#define PASS_OUT_POS 225
#define DEAD_POS 150  // Min position

// Audio track numbers
#define YAWN_TRACK 0
#define STUFFY_TRACK 1
#define OPEN_WINDOW_TRACK 2
#define PASS_OUT_TRACK 3
#define THATS_BETTER_TRACK 4
#define DEAD_TRACK 5

#define TIME_DELAY 10000 // time delay in ms

#define SERVO 0  // Flapping servo
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz

#define SFX_RST 4 // Sound board RST pin

// Servo board - default address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Sound board connected to Serial1 - must be set to 9600 baud
Adafruit_Soundboard sfx = Adafruit_Soundboard(&Serial1, NULL, SFX_RST);

ESDKCanary myCanary = ESDKCanary(&sfx, &pwm, SERVO);

void setup() {
  // Setup buttons
  pinMode(LEFT_BUTTON, INPUT);
  pinMode(RIGHT_BUTTON, INPUT);
  //  pinMode(DEMO_BUTTON, INPUT);
  //  pinMode(isrLED, OUTPUT);
  //  digitalWrite(isrLED, HIGH);
  attachInterrupt(digitalPinToInterrupt(LEFT_BUTTON), leftButtonIsr, FALLING);
  // DEMO_BUTTON duplicates the RIGHT_BUTTON function - activates demo
  attachInterrupt(digitalPinToInterrupt(RIGHT_BUTTON), rightButtonIsr, FALLING);
  //  attachInterrupt(digitalPinToInterrupt(DEMO_BUTTON), rightButtonIsr, FALLING);

  Serial.begin(115200);
#ifdef DEBUG
  while (!Serial) {
    ; // wait for serial port to connect
  }
#endif
  Serial.println("Canary Controller");
  // Serial 1 used for sound board at 9600 baud
  Serial1.begin(9600);
  while (!Serial1) {
    ; // wait for serial port to connect
  }
  Serial.println("Serial1 attached");

  if (epd.Init() != 0) {
    Serial.print("e-Paper init failed ");
    return;
  }
  Serial.println("EDP attached");

  epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black
  epd.DisplayFrame();

  /**
      there are 2 memory areas embedded in the e-paper display
      and once the display is refreshed, the memory area will be auto-toggled,
      i.e. the next action of SetFrameMemory will set the other memory area
      therefore you have to set the frame memory and refresh the display twice.
  */
  epd.SetFrameMemory_Base(RSLOGO);
  epd.DisplayFrame();

  updateEPDGreeting();

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  mqttClient.setServer(broker, port);
  mqttClient.setCallback(callback);
  mqttClient.setBufferSize(384);

  // Init sound board
  if (!sfx.reset()) {
    Serial.println("SFX board not found");
  }
  else Serial.println("SFX board attached");
  myCanary.Tweet(YAWN_TRACK, audioOn);

  // Init servo
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates
  myCanary.StartPos(WINGS_DOWN);
  Serial.println("Servo initialised");

  delay(5000);
}

void loop() {
  if (demo_mode) {
    if (WiFi.status() == WL_CONNECTED) {
      mqttClient.disconnect();
      WiFi.disconnect();
      wifiState = WiFi.status();
      delay(1000);
    }

  } else {  // WiFi mode
    // Attempt to reconnect - Wifi.begin blocks until connect or failure
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi mode...");
      updateEPD();
      reconnectWiFi();
    }

    // Attempt to reconnect without blocking
    if (!mqttClient.connected()) {
      long now = millis();
      if (now - lastReconnectMQTTAttempt > 5000) {
        lastReconnectMQTTAttempt = now;
        if (reconnectMQTT()) {
          lastReconnectMQTTAttempt = 0;
        }
      }
    } else {
      mqttClient.loop();
    }
  }

  if (updateDisplayFlag) {
    updateDisplayFlag = false;
    updateEPD();
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= delay_time) {
    previousMillis = currentMillis;
    updateState(co2);
  }
}

void updateCanary(States state) {
  switch (state) {
    case THATS_BETTER:
      Serial.println("That's better...");
      myCanary.Tweet(THATS_BETTER_TRACK, audioOn);
      myCanary.StartPos(WINGS_DOWN);
      break;
    case STUFFY:
      Serial.println("Stuffy...");
      updateDisplayFlag = true;
      myCanary.Tweet(STUFFY_TRACK, audioOn);
      myCanary.Flap(WINGS_DOWN, WINGS_UP_A_BIT, VSLOW, 3);
      break;
    case OPEN_WINDOW:
      Serial.println("Open a window...");
      updateDisplayFlag = true;
      myCanary.Tweet(OPEN_WINDOW_TRACK, audioOn);
      myCanary.Flap(WINGS_DOWN, WINGS_UP_A_LOT, FAST, 4);
      break;
    case PASS_OUT:
      Serial.println("Pass out...");
      updateDisplayFlag = true;
      myCanary.Tweet(PASS_OUT_TRACK, audioOn);
      myCanary.PassOut(PASS_OUT_POS, FAST);
    case DEAD:
      Serial.println("Dead...");
      myCanary.Tweet(DEAD_TRACK, audioOn);
      myCanary.Dead(DEAD_POS, VFAST);
      displayTombStone();
      delay(5000);
      displayClear();
      while (1);// Program ends!! Reboot
      break;
    case DEMO_DEAD: // Dead
      Serial.println("Demo dead...");
      myCanary.Tweet(DEAD_TRACK, audioOn);
      myCanary.PassOut(PASS_OUT_POS, FAST);
      displayTombStone();
      delay(5000);
      displayClear();
      myCanary.StartPos(WINGS_DOWN);
  }
}

// Toggle audio on / off
void leftButtonIsr() {
  audioOn = ! audioOn;
  updateEPD();
}

// Enter demo mode
void rightButtonIsr() {
  demo_mode = true;
  delay_time = DEMO_DELAY;
  audioOn = true;
  co2 = 400;
  reentrant = true;
}

// Sets the rules for changing state
void updateState(int co2) {
  static States previousState = DEAD;
  States state = THATS_BETTER;

  if ((previousState == PASS_OUT) && (co2 < PASS_OUT_CO2)) {
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
    updateCanary(state);
  }
}

void reconnectWiFi() {
#ifdef DEBUG
  Serial.print("Wifi Status: ");
  Serial.println(WiFi.status());
#endif
  // WL_IDLE_STATUS     = 0
  // WL_NO_SSID_AVAIL   = 1
  // WL_SCAN_COMPLETED  = 2
  // WL_CONNECTED       = 3
  // WL_CONNECT_FAILED  = 4
  // WL_CONNECTION_LOST = 5
  // WL_DISCONNECTED    = 6
  WiFi.disconnect();
  delay(1000);
  WiFi.begin(ssid, pass);
  delay(1000);
  wifiState = WiFi.status();
  Serial.println(WiFi.localIP());
}

boolean reconnectMQTT() {
  if (mqttClient.connect("arduinoClient")) {
    mqttClient.subscribe("airquality/#");
    Serial.println("MQTT connected");
  }
  return mqttClient.connected();
}

// Update sensor variables each time a message is received
void callback(char* topic, byte * payload, unsigned int length) {
#ifdef DEBUG
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
#endif
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
  if (reentrant) {
    reentrant = false;
  }
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
  tombstoneFlag = false;
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

  if (pm > 9999) {
    pm = 0;
  }
  char PART_string[] = {'0', '0', '0', '0', '\0'};
  PART_string[0] = pm / 100 / 10 + '0';
  PART_string[1] = pm / 100 % 10 + '0';
  PART_string[2] = pm % 100 / 10 + '0';
  PART_string[3] = pm % 100 % 10 + '0';

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
  if (demo_mode) {
    paint.DrawStringAt(0, 0, "Demo Mode", &Font16, COLORED);
  }
  else if ((audioOn) && (wifiState == WL_CONNECTED)) {
    paint.DrawStringAt(0, 0, "Wifi Audio", &Font16, COLORED);
  }
  else if (wifiState == WL_CONNECTED) {
    paint.DrawStringAt(0, 0, "Wifi", &Font16, COLORED);
  }
  else if (audioOn) {
    paint.DrawStringAt(0, 0, "Audio", &Font16, COLORED);
  }
  else {
    paint.DrawStringAt(0, 0, "", &Font16, COLORED);
  }
  epd.SetFrameMemory_Partial(paint.GetImage(), 0, 0, paint.GetWidth(), paint.GetHeight());

  epd.DisplayFrame_Partial();
}
