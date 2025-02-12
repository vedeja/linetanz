#include <WiFiS3.h>
#include <ArduinoLog.h>
#include "secrets.h"

class Network {

public:
  void setupWifi() {

    // Define values in the file secrets.h
    char ssid[] = SECRET_SSID;
    char pass[] = SECRET_PASS;

    // check for the WiFi module:
    if (WiFi.status() == WL_NO_MODULE) {
      Log.noticeln("Communication with WiFi module failed!");
      // don't continue
      while (true)
        ;
    }

    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
      Log.noticeln("Please upgrade the firmware");
    }

    // attempt to connect to WiFi network:
    while (status != WL_CONNECTED) {
      Log.noticeln("Attempting to connect to SSID: %s", ssid);
      // Connect to WPA/WPA2 network:
      status = WiFi.begin(ssid, pass);

      // wait 10 seconds for connection:
      delay(10000);
    }

    Log.noticeln("Connected to the network");
    printCurrentNet();
    printWifiData();
  }

private:

  int status = WL_IDLE_STATUS;  // the WiFi radio's status


  void printWifiData() {
    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    Log.notice("IP Address: ");

    Log.noticeln("%s", ip);
  }

  void printCurrentNet() {
    // print the SSID of the network
    Log.noticeln("SSID: %s", WiFi.SSID());

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Log.noticeln("Signal strength (RSSI): %d", rssi);

    // print the encryption type:
    uint8_t encryption = WiFi.encryptionType();
    Log.noticeln("Encryption Type: %X", encryption);
  }
};