/*
    Test Reconnecting MQTT with ESDK payload

    Same as mqtt_reconnect_nonblocking but adding ArduinoJson lib
    to parse the standard ESDK payload.

    This is to try and determine if the parser and / or payload is causing
    the mqtt reconnection to fail (enter a continuous loop), since
    mqtt_reconnect_nonblocking runs continuously on the latest NINA firmware.

    Note: PubSubClient max default apyload: 256 bytes!!!

  Modified example from PubSubClient lib

  This sketch demonstrates how to keep the client connected
  using a non-blocking reconnect function. If the client loses
  its connection, it attempts to reconnect every 5 seconds
  without blocking the main loop.

*/

#include <WiFiNINA.h>
#include "arduino_secrets.h"
#include <PubSubClient.h>
#include "ArduinoJson.h"

// ESDK topic root
#define TOPIC "airquality/#"
#define MQTT_PACKET_SIZE 384 // bytes

// You may need to use server IP address depending on your network
//const char broker[] = "192.168.X.X";
const char server[] = "airquality";
int        port     = 1883;

const int cbLed = 10; // Red - Changes state when callback is entered
int cbLedState = LOW;

const int wifiLed = 9;  // Green - On if connected
const int mqttLed = 15;  // Yellow - On if connected
const int jsonLed = 14;  // Red - Solid if a parser error occured

// Store environmental data
volatile int co2 = 400;
volatile double temperature = 21.0;
volatile double humidity = 40.0;
volatile int tvoc = 100;
volatile int pm = 1;

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
  co2 = doc["co2"]["co2"];
  temperature = doc["thv"]["temperature"];
  humidity = doc["thv"]["humidity"];
  tvoc = doc["thv"]["vocIndex"];
  pm = doc["pm"]["pm2.5"];
}

/////// Enter sensitive data in arduino_secrets.h
char ssid[] = SECRET_SSID;  // Network SSID
char pass[] = SECRET_PASS;  // WPA key

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

long lastReconnectAttempt = 0;
long lastHeartBeat = 0;

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

boolean reconnect() {
  if (mqttClient.connect("arduinoNano")) {
    // Once connected, publish an announcement...
    mqttClient.publish("test/announce", "Nano alive");
    // ... and resubscribe
    mqttClient.subscribe(TOPIC);
  }
  return mqttClient.connected();
}

void setup()
{
  pinMode(cbLed, OUTPUT);
  digitalWrite(cbLed, LOW);
  pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, LOW);
  pinMode(mqttLed, OUTPUT);
  digitalWrite(mqttLed, LOW);
  pinMode(jsonLed, OUTPUT);
  digitalWrite(jsonLed, LOW);

  mqttClient.setBufferSize(MQTT_PACKET_SIZE);
  mqttClient.setServer(server, 1883);
  mqttClient.setCallback(callback);

  lastReconnectAttempt = 0;
}


void loop()
{
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(wifiLed, LOW);
    reconnectWiFi();
    delay(2000);
    digitalWrite(wifiLed, HIGH);
  }

  if (!mqttClient.connected()) {
    digitalWrite(mqttLed, LOW);
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
      digitalWrite(mqttLed, HIGH);
    }
  } else {
    // mqttClient connected

    mqttClient.loop();
  }

}
