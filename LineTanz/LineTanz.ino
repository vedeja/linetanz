#include "Inbox.h"
#include "messageType.h"
#include "commandType.h"
#include "dlMessage.h"
#include "secrets.h"
#include <WiFiS3.h>
#include "Arduino_LED_Matrix.h"
#include <ArduinoLog.h>

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
const int keepAliveLedPin = 11;
const int muteLedPin = 12;
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
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  //Log.begin(LOG_LEVEL_SILENT, &Serial);
  Log.noticeln("LineTanz");

  matrix.begin();

  matrix.loadSequence(LEDMATRIX_ANIMATION_WIFI_SEARCH);
  matrix.begin();
  matrix.play(true);

  setupWifi();
  isConnected = setupTcp();
  matrix.loadFrame(LEDMATRIX_HEART_BIG);

  pinMode(footswitchPin, INPUT);
  pinMode(keepAliveLedPin, OUTPUT);
  pinMode(muteLedPin, OUTPUT);
  // set initial LED state
  digitalWrite(keepAliveLedPin, 0);
  digitalWrite(muteLedPin, muteState);

  seqNo = 0;
  inbox = new Inbox();
  initializationStatus = 0;
  isInitialized = false;

  for (int i = 0; i < 8; i++) {
    outgoingInitMessages[i] = nullptr;  // Initialize the array with nullptr
  }

  Log.noticeln("Setup complete");
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

bool setupTcp() {
  Log.noticeln("Attempting to connect to mixer: %s on port %d", tcpServer, tcpPort);

  if (tcpClient.connect(tcpServer, tcpPort)) {
    Log.noticeln("Connected to mixer");
    tcpClient.flush();
    return true;
  } else {
    Log.noticeln("Failed to connect to mixer");
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
      Log.noticeln("Init done");
    }
  }

  return isInitialized;
}

void increaseInitStatus() {
  initializationStatus++;

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

  Log.noticeln("Updated init status to: %d (%s)", initializationStatus, description);
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
  Log.noticeln("handleInitializationResponse...");

  int previousInitStatus = initializationStatus - 1;

  SequencedCommand* request = outgoingInitMessages[previousInitStatus];

  if (request == nullptr) {
    Log.noticeln("Request was null");
    return false;
  }

  if (request->command != response->command) {
    Log.notice("Request command was ");
    Log.notice("%d", request->command);
    Log.notice(" but the response command was ");
    Log.notice("%d", response->command);
    return false;
  }

  // TODO: Maybe redundant:
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

        Log.noticeln("State: %d", muteState);

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
  digitalWrite(muteLedPin, muteState);

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastFootswitchState = reading;
}

void reportStatus() {
  unsigned long now = millis();
  // Report every x milliseconds
  if (now - lastReportedStatus > 5000) {
    lastReportedStatus = now;
    Log.noticeln("STATUS | Initialization status: %d | Free SRAM: %d bytes | Inbox messages: %d", initializationStatus, freeRam(), inbox->messageCount);
  }
}

extern "C" char* sbrk(int incr);

int freeRam() {
  char top;
  return &top - reinterpret_cast<char*>(sbrk(0));
}

void keepAlive() {
  unsigned long now = millis();
  // Send every x milliseconds
  if (now - lastKeepAliveSent > keepAliveInterval) {
    lastKeepAliveSent = now;
    sendRequest(cmdTypeKeepAlive, EMPTY, 0);
    
    // Turn on keep alive LED
    digitalWrite(keepAliveLedPin, 1);
  }
  else if (now - lastKeepAliveSent > 100) {
    // Turn off keep alive LED
    digitalWrite(keepAliveLedPin, 0);
  }

}

void receive() {
    size_t size = tcpClient.available();

  if (size > 0) {
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
  Log.noticeln("OUT <<< %s (%d) %s", getMessageTypeName(message.type), message.sequenceNumber, getCommandTypeName(message.command));

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
    //Log.noticeln("Byte array sent successfully");
  } else {
    Log.noticeln("Failed to send byte array");
  }
}

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