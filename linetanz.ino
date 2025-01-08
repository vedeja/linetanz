/*
 * Created by ArduinoGetStarted.com
 *
 * This example code is in the public domain
 *
 * Tutorial page: https://arduinogetstarted.com/tutorials/arduino-tcp-client
 */

#include <WiFiS3.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";          // CHANGE TO YOUR WIFI SSID
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";       // CHANGE TO YOUR WIFI PASSWORD
const char* TCP_SERVER_ADDR = "192.168.0.26";  // CHANGE TO TCP SERVER'S IP ADDRESS
const int TCP_SERVER_PORT = 1470;

WiFiClient TCP_client;

void setup() {
  Serial.begin(9600);

  Serial.println("Arduino: TCP CLIENT");

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true)
      ;
  }

//   String fv = WiFi.firmwareVersion();
//   if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
//     Serial.println("Please upgrade the firmware");
//   }

  Serial.print("Attempting to connect to SSID: ");
  Serial.println(WIFI_SSID);
  // attempt to connect to WiFi network:
  while (WiFi.begin(WIFI_SSID, WIFI_PASSWORD) != WL_CONNECTED) {
    delay(10000);  // wait 10 seconds for connection:
  }

  Serial.print("Connected to WiFi ");
  Serial.println(WIFI_SSID);

  // connect to TCP server
  if (TCP_client.connect(TCP_SERVER_ADDR, TCP_SERVER_PORT)) {
    Serial.println("Connected to TCP server");
    TCP_client.write("Hello!");  // send to TCP Server
    TCP_client.flush();
  } else {
    Serial.println("Failed to connect to TCP server");
  }
}

void loop() {
  // Read data from server and print them to Serial
  if (TCP_client.available()) {
    char c = TCP_client.read();
    Serial.print(c);
  }

  if (!TCP_client.connected()) {
    Serial.println("Connection is disconnected");
    TCP_client.stop();

    // reconnect to TCP server
    if (TCP_client.connect(TCP_SERVER_ADDR, TCP_SERVER_PORT)) {
      Serial.println("Reconnected to TCP server");
      TCP_client.write("Hello!");  // send to TCP Server
      TCP_client.flush();
    } else {
      Serial.println("Failed to reconnect to TCP server");
      delay(1000);
    }
  }
}
