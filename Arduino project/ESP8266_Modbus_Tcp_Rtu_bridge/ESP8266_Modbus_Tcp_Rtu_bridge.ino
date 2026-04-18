/***************************
Created by profi-max (Oleg Linnik) 2024
https://profimaxblog.ru
https://github.com/profi-max

MODIFICATION: Added WiFi reconnection mechanism after failure
***************************/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "SoftwareSerial.h"

// Modbus defines
#define MB_PORT 502
#define MB_BUFFER_SIZE 256
#define MB_RTU_SLAVE_RESPONSE_TIMEOUT_MS 1000

#define MB_FC_NONE 0
#define MB_FC_READ_REGISTERS 3
#define MB_FC_WRITE_REGISTER 6
#define MB_FC_WRITE_MULTIPLE_REGISTERS 16
#define MB_FC_ERROR_MASK 128

// User defines
#define MB_UART_RX_PIN 13
#define MB_UART_TX_PIN 15

// WiFi settings
const char* ssid = "YourSSID";
const char* password = "YourPassword";

WiFiServer mbServer(MB_PORT);
WiFiClient client;
SoftwareSerial swSerial;
uint8_t mbByteArray[MB_BUFFER_SIZE];
bool wait_for_rtu_response = false;
uint32_t timeStamp;

// WiFi monitoring variables
bool isWiFiConnected = false;
uint32_t lastWiFiCheckTime = 0;
const int wifiCheckInterval = 5000;  // 5 seconds

// Function: Check and reconnect WiFi
void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    if (isWiFiConnected) {
      Serial.println("WiFi disconnected. Attempting to reconnect...");
      isWiFiConnected = false;
    }

    WiFi.disconnect();
    WiFi.begin(ssid, password);

    // Try reconnecting
    uint32_t startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {  // 10-second timeout
      delay(500);
      Serial.print(".");
    }

    // Check connection status
    if (WiFi.status() == WL_CONNECTED) {
      isWiFiConnected = true;
      Serial.println("\nReconnected to WiFi!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      mbServer.begin();
    } else {
      Serial.println("\nFailed to reconnect to WiFi!");
    }
  } else if (!isWiFiConnected) {
    // Update connection status
    isWiFiConnected = true;
    Serial.println("\nWiFi is connected.");
  }
}

// TCP Modbus server task
void modbusTcpServerTask() {
  // Pause if WiFi is disconnected
  if (!isWiFiConnected) return;

  uint16_t crc;
  uint8_t mb_func = MB_FC_NONE;
  uint16_t len = 0;

  if (mbServer.hasClient()) {
    if (!client || !client.connected()) {
      if (client) client.stop();
      client = mbServer.accept();
    } else {
      WiFiClient serverClient = mbServer.accept();
      serverClient.stop();
    }
  }

  if (client && client.connected() && client.available()) {
    delay(1);
    uint16_t bytesReady;
    while ((bytesReady = client.available()) && len < MB_BUFFER_SIZE) {
      len += client.readBytes(&mbByteArray[len], bytesReady);
    }
    if (len > 8) mb_func = mbByteArray[7];
  } else {
    return;
  }

  // Process Modbus requests (same logic as original)
  // ...
}

// Modbus RTU master task
void modbusRtuMasterTask() {
  if (!isWiFiConnected) return;

  // RTU processing (same logic as original)
  // ...
}

void setup() {
  swSerial.begin(9600, SWSERIAL_8N1, MB_UART_RX_PIN, MB_UART_TX_PIN);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(115200);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  isWiFiConnected = true;

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  mbServer.begin();
}

void loop() {
  uint32_t currentMillis = millis();
  if (currentMillis - lastWiFiCheckTime >= wifiCheckInterval) {
    lastWiFiCheckTime = currentMillis;
    checkWiFiConnection();
  }

  modbusTcpServerTask();
  if (wait_for_rtu_response) modbusRtuMasterTask();
}
