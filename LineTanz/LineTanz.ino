#include "Inbox.h"
#include "messageType.h"
#include "commandType.h"
#include "dlMessage.h"
#include "secrets.h"
#include <WiFiS3.h>
#include "Arduino_LED_Matrix.h"

WiFiClient tcpClient;
Inbox* inbox;
ArduinoLEDMatrix matrix;

// Define values in the file secrets.h
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
char tcpServer[] = SECRET_TCP_SERVER_ADDR;
uint16_t tcpPort = SECRET_TCP_SERVER_PORT;

int status = WL_IDLE_STATUS;  // the WiFi radio's status
const int footswitchPin = 2;
const int ledPin = 13;
int muteState = HIGH;
int footswitchState;
int lastFootswitchState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
int keepAliveInterval = 2000;  // 2 seconds between sending keep-alive to the mixer
uint8_t seqNo;
uint8_t* EMPTY = new uint8_t[0];
unsigned long lastKeepAliveSent;
bool isConnected;
bool isInitialized;
int initializationStatus;
unsigned long lastReportedStatus;


class SequencedCommand {
public:
  uint8_t sequenceNumber;
  commandType command;
};

SequencedCommand* outgoingInitMessages[8];


void setup() {
  Serial.begin(9600);
  Serial.println("LineTanz");

  matrix.begin();

  matrix.loadSequence(LEDMATRIX_ANIMATION_WIFI_SEARCH);
  matrix.begin();
  matrix.play(true);

  setupWifi();
  isConnected = setupTcp();
  matrix.loadFrame(LEDMATRIX_HEART_BIG);

  pinMode(footswitchPin, INPUT);
  pinMode(ledPin, OUTPUT);
  // set initial LED state
  digitalWrite(ledPin, muteState);

  seqNo = 0;
  inbox = new Inbox();
  initializationStatus = 0;
  isInitialized = false;

  for (int i = 0; i < 8; i++) {
    outgoingInitMessages[i] = nullptr;  // Initialize the array with nullptr
  }
}

void loop() {
  if (!isConnected) {
    matrix.loadFrame(LEDMATRIX_DANGER);
    return;
  }

  //TODO: try interrupts instead
  handleFootswitch();

  if (initialize()) {
    keepAlive();
  }

  receive();
  handleIncomingMessages();

  // Execute logic
  // Handle outgoing messages

  reportStatus();
}

void setupWifi() {
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  Serial.println("Connected to the network");
  printCurrentNet();
  printWifiData();
}

bool setupTcp() {
  Serial.print("Attempting to connect to mixer: ");
  Serial.print(tcpServer);
  Serial.print(" on port ");
  Serial.println(tcpPort);

  if (tcpClient.connect(tcpServer, tcpPort)) {
    Serial.println("Connected to mixer");
    tcpClient.flush();
    return true;
  } else {
    Serial.println("Failed to connect to mixer");
    return false;
  }
}

uint8_t getNextSeqNo() {
  uint8_t n = ++seqNo;
  if (n == 0) {
    n = 1;
  }

  return n;
}

bool initialize() {
  if (!isInitialized) {

    if (initializationStatus == 0) {
      sendInitRequest(cmdTypeKeepAlive, EMPTY, 0);
      increaseInitStatus();

    } else if (initializationStatus == 2) {
      uint8_t* body = new uint8_t[8]{ 0, 0, 0, 0, 0, 0, 0, 0 };
      sendInitRequest(cmdTypeMessageSizeControl, body, 8);
      increaseInitStatus();

    } else if (initializationStatus == 4) {
      sendInitRequest(cmdTypeHandshake, EMPTY, 0);
      increaseInitStatus();

    } else if (initializationStatus == 6) {
      uint8_t* body = new uint8_t[4]{ 0, 0, 0, 0x02 };
      sendInitRequest(cmdTypeInfo, body, 4);
      increaseInitStatus();

    } else if (initializationStatus == 8) {
      increaseInitStatus();
      isInitialized = true;
      Serial.println("Init done");

    } 
  }

  return isInitialized;
}

void increaseInitStatus() {
  initializationStatus++;

  Serial.print("Updated init status to: ");
  Serial.print(initializationStatus);

  char* description;

  switch (initializationStatus) {
    case 1:
      description = "Awaiting KeepAlive response";
      break;
    case 2:
      description = "KeepAlive response received";
      break;
    case 3:
      description = "Awaiting MessageSizeControl response";
      break;
    case 4:
      description = "MessageSizeControl response received";
      break;
    case 5:
      description = "Awaiting Handshake response";
      break;
    case 6:
      description = "Handshake response received";
      break;
    case 7:
      description = "Awaiting Info response";
      break;
    case 8:
      description = "Info response received";
      break;
    case 9:
      description = "Initialized";
      break;
  }

  Serial.print(" (");
  Serial.print(description);
  Serial.println(")");
}

void handleIncomingMessages() {
  for (int i = 0; i < inbox->messageCount; i++) {
    dlMessage* m = inbox->messages[i];

    if (!isInitialized && m->type == msgTypeResponse) {
      bool isHandled = handleInitializationResponse(m);
    }

    inbox->remove(m->index);
  }
}

bool handleInitializationResponse(dlMessage* response) {
  Serial.println("handleInitializationResponse...");

  int previousInitStatus = initializationStatus - 1;
  Serial.print("Looking at position ");
  Serial.println(previousInitStatus);

  SequencedCommand* request = outgoingInitMessages[previousInitStatus];

  if (request == nullptr) {
    Serial.println("Request was null");
    return false;
  }

  if (request->command != response->command) {
    Serial.print("Request command was ");
    Serial.print(request->command);
    Serial.print(" but the response command was ");
    Serial.println(response->command);
    return false;
  }

  Serial.print("Matched response to request with seqno: ");
  Serial.print(request->sequenceNumber);
  Serial.print(" command: ");
  Serial.println(request->command);

  // TODO: Maybe reduntant:
  outgoingInitMessages[previousInitStatus] = nullptr;

  switch (initializationStatus) {
    case 1:
      increaseInitStatus();
      break;

    case 3:
      increaseInitStatus();
      break;

    case 5:
      increaseInitStatus();
      break;

    case 7:
      increaseInitStatus();
      break;

    default:
      break;
  }

  return true;
}

void handleFootswitch() {
  // read the state of the switch into a local variable:
  int reading = digitalRead(footswitchPin);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastFootswitchState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != footswitchState) {
      footswitchState = reading;

      // only toggle the LED if the new button state is HIGH
      if (footswitchState == HIGH) {
        muteState = !muteState;

        SerialPrintState();

        int size = 12;

        uint8_t state = muteState ? 0x01 : 0x00;        
        uint8_t* body = new uint8_t[size]{ 0x00, 0x00, 0x13, 0x00, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, state };

        // 13 67 is the channel number for FX1 input
        body[3] = 0x67;
        sendRequest(cmdTypeChannelValues, body, size);

        // 13 C9 is the channel number for FX2 input
        body[3] = 0xC9;
        sendRequest(cmdTypeChannelValues, body, size);
      }
    }
  }

  // set the LED:
  digitalWrite(ledPin, muteState);

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastFootswitchState = reading;
}

void reportStatus() {
  unsigned long now = millis();
  // Report every x milliseconds
  if (now - lastReportedStatus > 5000) {
    lastReportedStatus = now;
    Serial.print("STATUS | Initialization status: ");
    Serial.print(initializationStatus);
    Serial.print(" | Inbox messages: ");
    Serial.println(inbox->messageCount);
  }
}

void keepAlive() {
  unsigned long now = millis();
  // Send every x milliseconds
  if (now - lastKeepAliveSent > keepAliveInterval) {
    lastKeepAliveSent = now;
    sendRequest(cmdTypeKeepAlive, EMPTY, 0);
  }
}

void receive() {
  if (tcpClient.available()) {
    size_t size = tcpClient.available();
    uint8_t* buffer = new uint8_t[size];
    int bytesRead = tcpClient.read(buffer, size);

    if (bytesRead > 0) {
      inbox->receive(buffer, size);
    }

    // Free the allocated memory
    delete[] buffer;
  }
}

void sendInitRequest(commandType command, uint8_t body[], int size) {
  char seqNo = sendRequest(command, body, size);

  SequencedCommand* sc = new SequencedCommand;
  sc->sequenceNumber = seqNo;
  sc->command = command;
  outgoingInitMessages[initializationStatus] = sc;

  Serial.print("Stored an init command, seqno: ");
  Serial.print(sc->sequenceNumber);
  Serial.print(" command: ");
  Serial.print(sc->command);
  Serial.print(" in position ");
  Serial.println(initializationStatus);
}

char sendRequest(commandType command, uint8_t body[], int size) {
  return send(msgTypeRequest, command, body, size);
}

char send(messageType type, commandType command, uint8_t body[], int size) {
  uint8_t sequenceNumber = getNextSeqNo();
  dlMessage message(type, command, sequenceNumber, body, size);

  send(message);
  return seqNo;
}

void send(dlMessage& message) {
  Serial.print("OUT <<< ");
  SerialPrintMessage(message);

  bool hasBody = message.size > 0;
  uint8_t bodyChunkCount1 = message.size / 4;
  uint8_t bodyChunkCount2 = 0;  //TODO: handle such large bodies, larger than 255 chunks? Yes, probably - song playback messages appear to have 364 bytes of data in the body
  uint16_t headerChecksum = 0xFFFF - (0xAB + message.sequenceNumber + bodyChunkCount2 + bodyChunkCount1 + message.type + message.command);
  uint8_t hc1 = (headerChecksum >> 8) & 0xFF;
  uint8_t hc2 = headerChecksum & 0xFF;

  // Assume no body
  int len = 8;

  if (hasBody) {
    len += message.size;
    len += 4;  // space for body checksum
  }

  uint8_t data[len] = {
    0xAB,
    message.sequenceNumber,
    bodyChunkCount2,
    bodyChunkCount1,
    message.type,
    message.command,
    hc1,
    hc2
  };

  // data[0] = 0xAB;
  // data[1] = message.sequenceNumber;
  // data[2] = bodyChunkCount2;
  // data[3] = bodyChunkCount1;
  // data[4] = message.type;
  // data[5] = message.command;
  // data[6] = hc1;
  // data[7] = hc2;

  if (hasBody) {
    uint32_t bodyChecksum = 0xFFFFFFFF;

    for (int i = 0; i < message.size; i++) {
      uint8_t b = message.body[i];
      data[8 + i] = b;

      // Calculate checksum
      bodyChecksum -= b;
    }

    uint8_t bc0 = (bodyChecksum >> 24) & 0xFF;
    uint8_t bc1 = (bodyChecksum >> 16) & 0xFF;
    uint8_t bc2 = (bodyChecksum >> 8) & 0xFF;
    uint8_t bc3 = bodyChecksum & 0xFF;

    int bcOffset = 8 + message.size;

    data[bcOffset + 0] = bc0;
    data[bcOffset + 1] = bc1;
    data[bcOffset + 2] = bc2;
    data[bcOffset + 3] = bc3;
  }

  // Send the byte array
  if (tcpClient.write(data, len) == len) {
    //Serial.println("Byte array sent successfully");
  } else {
    Serial.println("Failed to send byte array");
  }
}

void printWifiData() {
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");

  Serial.println(ip);

  // print MAC address:
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
}

void printCurrentNet() {
  // print the SSID of the network
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router:
  uint8_t bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI): ");
  Serial.println(rssi);

  // print the encryption type:
  uint8_t encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
}

void printMacAddress(uint8_t mac[]) {
  for (int i = 0; i < 6; i++) {
    if (i > 0) {
      Serial.print(":");
    }
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
  }
  Serial.println();
}