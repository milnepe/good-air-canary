/*
  Test Reconnecting MQTT - non-blocking

  Modified example from PubSubClient lib

  This sketch demonstrates how to keep the client connected
  using a non-blocking reconnect function. If the client loses
  its connection, it attempts to reconnect every 5 seconds
  without blocking the main loop.

*/

#include <WiFiNINA.h>
#include "arduino_secrets.h"
#include <PubSubClient.h>

#define TOPIC "test/hello"

// You may need to use server IP address depending on your network
//const char broker[] = "192.168.X.X";
const char server[] = "airquality";
int        port     = 1883;

const int cbLed = 10; // Red
int cbLedState = LOW;

const int wifiLed = 9;  // Green
const int mqttLed = 12;  // Yellow


void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  digitalWrite(cbLed, cbLedState);
  cbLedState = !cbLedState;
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
