#include <WiFiS3.h>
#include <ArduinoLog.h>
#include "secrets.h"

class Network
{

public:
  int status = WL_IDLE_STATUS; // the WiFi radio's status

  void setupWifi()
  {

    // check for the WiFi module:
    if (WiFi.status() == WL_NO_MODULE)
    {
      Log.noticeln("Communication with WiFi module failed!");
      // don't continue
      while (true)
        ;
    }

    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION)
    {
      Log.noticeln("Please upgrade the firmware");
    }
  }

  void connect()
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      return;
    }

    // Define values in the file secrets.h
    char ssid[] = SECRET_SSID;
    char pass[] = SECRET_PASS;

    Log.noticeln("Attempting to connect to WIFI: %s ", ssid);

    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED)
    {
      status = WiFi.status();
      delay(1000);
    }

    status = WiFi.status();
    Log.noticeln("Connected to the network");
    printCurrentNet();
    printWifiData();
  }

private:
  void printWifiData()
  {
    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    Log.notice("IP Address: ");

    Log.noticeln(ip);
    // Serial.println(ip);
  }

  void printCurrentNet()
  {
    // print the SSID of the network
    Log.noticeln("SSID: %s", WiFi.SSID());

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Log.noticeln("Signal strength (RSSI): %d dBm", rssi);
  }
};