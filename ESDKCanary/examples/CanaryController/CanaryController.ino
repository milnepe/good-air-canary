/***************************************************
  An Arduino IoT client for DesgnSpark ESDK
  that reacts to environmental CO2 levels

  Version: 3.0
  Date: 14 June 2022
  Author: Peter Milne

  Copyright 2022 Peter Milne
  Released under GNU GENERAL PUBLIC LICENSE
  Version 3, 29 June 2007

 ****************************************************/
// Un-comment for debugging
// In debug mode the system will not run until a serial monitor is attached!
#define DEBUG

#include <WiFiNINA.h>
#include "arduino_secrets.h"
#include <PubSubClient.h>
#include "ArduinoJson.h"
#include "ESDKCanary.h"
#include <SPI.h>
#include "CanaryDisplay.h"

// ESDK topic root
#define TOPIC "airquality/#"
#define MQTT_PACKET_SIZE 384 // bytes

// ESDK MQTT server name
// You may need to substiture its IP address on your network
//const char broker[] = "192.168.0.75";
const char server[] = "airquality";
int        port     = 1883;

// Debug LEDs
const int cbLed = 10; // Red - Changes state when callback is entered
int cbLedState = LOW;

const int wifiLed = 9;  // Green - On if connected
const int mqttLed = 15;  // Yellow - On if connected
const int jsonLed = 14;  // Red - Solid if a parser error occured

/////// Enter sensitive data in arduino_secrets.h
char ssid[] = SECRET_SSID;  // Network SSID
char pass[] = SECRET_PASS;  // WPA key

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long lastReconnectAttempt;

volatile bool updateDisplayFlag = false;
int prevSensorValue = 0;

// Button code
enum buttons {LEFT_BUTTON = 2, RIGHT_BUTTON = 3, DEMO_BUTTON = 9};

#define SERVO 0  // Flapping servo
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz

#define SFX_RST 4 // Sound board RST pin

// Servo board - default address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Sound board connected to Serial1 - must be set to 9600 baud
Adafruit_Soundboard sfx = Adafruit_Soundboard(&Serial1, NULL, SFX_RST);

ESDKCanary myCanary = ESDKCanary(&sfx, &pwm, SERVO);
// Create Canary Display object
CanaryDisplay epd(&myCanary);

void setup() {
  pinMode(cbLed, OUTPUT);
  digitalWrite(cbLed, LOW);
  pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, LOW);
  pinMode(mqttLed, OUTPUT);
  digitalWrite(mqttLed, LOW);
  pinMode(jsonLed, OUTPUT);
  digitalWrite(jsonLed, LOW);

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

  epd.initDisplay();
  epd.showGreeting();

  mqttClient.setBufferSize(MQTT_PACKET_SIZE);
  mqttClient.setServer(server, 1883);
  mqttClient.setCallback(callback);

  lastReconnectAttempt = 0;

  // Init sound board
  if (!sfx.reset()) {
    Serial.println("SFX board not found");
  }
  else Serial.println("SFX board attached");
  myCanary.Tweet(YAWN_TRACK, myCanary.audioOn);

  // Init servo
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates
  delay(1000);
  myCanary.StartPos(WINGS_DOWN);
  Serial.println("Wings down");

  delay(5000);
}

void loop() {
  if (!myCanary.demoOn) {
    if (WiFi.status() != WL_CONNECTED) {
      digitalWrite(wifiLed, LOW);
      myCanary.wifiOn = false;
      reconnectWiFi();
      delay(2000);
      if (WiFi.status() == WL_CONNECTED) {
        digitalWrite(wifiLed, HIGH);
        myCanary.wifiOn = true;
      }
      updateDisplayFlag = true;
    }

    if (!mqttClient.connected()) {
      digitalWrite(mqttLed, LOW);
      unsigned long now = millis();
      if (now - lastReconnectAttempt > 5000) {
        lastReconnectAttempt = now;
        // Attempt to reconnect
        if (reconnectMQTT()) {
          lastReconnectAttempt = 0;
          digitalWrite(mqttLed, HIGH);
        }
      }
    } else {
      // mqttClient connected
      mqttClient.loop();
    }
  } else {
    doDemo();
  }

  if (updateDisplayFlag) {
    updateDisplayFlag = false;
    epd.updateDisplay();
  }

  myCanary.updateState();
}

void doDemo() {
  // Simulate co2 values
  int  co2_array[] = {NORMAL_CO2, STUFFY_CO2, OPEN_WINDOW_CO2, PASS_OUT_CO2, DEAD_CO2};

  Serial.println("Entering demo mode");
  if (WiFi.status() == WL_CONNECTED) {
    mqttClient.disconnect();
    digitalWrite(mqttLed, LOW);
    WiFi.disconnect();
    digitalWrite(wifiLed, LOW);
    delay(1000);
  }

  for (unsigned int i = 0; i < sizeof(co2_array) / sizeof(co2_array[0]); i++) {
    myCanary.co2 = co2_array[i];
    epd.updateDisplay();
    myCanary.updateState();
    delay(5000);
  }
}

// Toggle audio on / off
void leftButtonIsr() {
  myCanary.audioOn = ! myCanary.audioOn;
  updateDisplayFlag = true;
}

// Enter demo mode
void rightButtonIsr() {
  myCanary.demoOn = true;
  updateDisplayFlag = true;
}



int reconnectWiFi() {
  // WL_IDLE_STATUS     = 0
  // WL_NO_SSID_AVAIL   = 1
  // WL_SCAN_COMPLETED  = 2
  // WL_CONNECTED       = 3
  // WL_CONNECT_FAILED  = 4
  // WL_CONNECTION_LOST = 5
  // WL_DISCONNECTED    = 6

  // Force a disconnect
  WiFi.disconnect();
  delay(1000);
  WiFi.begin(ssid, pass);
  return WiFi.status();
}

boolean reconnectMQTT() {
  if (mqttClient.connect("arduinoNano")) {
    // Once connected, publish an announcement...
    mqttClient.publish("nano/alive", "Nano alive");
    // ... and resubscribe
    mqttClient.subscribe(TOPIC);
  }
  return mqttClient.connected();
}

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  digitalWrite(cbLed, cbLedState);
  cbLedState = !cbLedState;

  // Parse ESDK payload
  StaticJsonDocument<MQTT_PACKET_SIZE> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  //  error = DeserializationError::NoMemory;  // force an error for testing
  if (error) {
    // Turn on led if any deserialization errors occur
    digitalWrite(jsonLed, HIGH);
    return;
  }
  myCanary.co2 = doc["co2"]["co2"];
  myCanary.temperature = doc["thv"]["temperature"];
  myCanary.humidity = doc["thv"]["humidity"];
  myCanary.tvoc = doc["thv"]["vocIndex"];
  myCanary.pm = doc["pm"]["pm2.5"];

  updateDisplayFlag = true;
}
