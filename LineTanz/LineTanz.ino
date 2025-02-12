#include "Network.h"
#include "Inbox.h"
#include "Initializer.h"
#include "MixerClient.h"
#include "messageType.h"
#include "commandType.h"
#include "dlMessage.h"
#include "secrets.h"
#include "Arduino_LED_Matrix.h"
#include <ArduinoLog.h>

Network* network;
Initializer* initializer;
Inbox* inbox;
MixerClient* client;
ArduinoLEDMatrix matrix;

const int footswitchPin = 2;
const int keepAliveLedPin = 11;
const int muteLedPin = 12;
int muteState = HIGH;
int footswitchState;
int lastFootswitchState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
int keepAliveInterval = 2000;  // 2 seconds between sending keep-alive to the mixer
uint8_t* EMPTY = new uint8_t[0];
unsigned long lastKeepAliveSent;
unsigned long lastReportedStatus;

void setup() {
  Serial.begin(9600);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  //Log.begin(LOG_LEVEL_SILENT, &Serial);
  Log.noticeln("LineTanz");

  matrix.begin();

  matrix.loadSequence(LEDMATRIX_ANIMATION_WIFI_SEARCH);
  matrix.begin();
  matrix.play(true);

  network = new Network();
  client = new MixerClient();
  
  network->setupWifi();  
  client->setupTcp();
  matrix.loadFrame(LEDMATRIX_HEART_BIG);

  pinMode(footswitchPin, INPUT);
  pinMode(keepAliveLedPin, OUTPUT);
  pinMode(muteLedPin, OUTPUT);
  // set initial LED state
  digitalWrite(keepAliveLedPin, 0);
  digitalWrite(muteLedPin, muteState);

  inbox = new Inbox();
  initializer = new Initializer(client);

  Log.noticeln("Setup complete");
}

void loop() {
  if (!client->isConnected) {
    matrix.loadFrame(LEDMATRIX_DANGER);
    return;
  }

  //TODO: try interrupts instead
  handleFootswitch();

  if (initializer->ensureInitialized()) {
    keepAlive();
  }

  client->receive(inbox);
  handleIncomingMessages();

  // Execute logic
  // Handle outgoing messages

  reportStatus();
}

void handleIncomingMessages() {
  for (int i = 0; i < inbox->messageCount; i++) {
    dlMessage* m = inbox->messages[i];

    initializer->handleIncomingMessage(m);
    inbox->remove(m->index);
  }
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
        client->sendRequest(cmdTypeChannelValues, body, size);

        // 13 C9 is the channel number for FX2 input
        body[3] = 0xC9;
        client->sendRequest(cmdTypeChannelValues, body, size);
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
    Log.noticeln("STATUS | Initialization status: %d | Free SRAM: %d bytes | Inbox messages: %d", initializer->initializationStatus, freeRam(), inbox->messageCount);
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
    client->sendRequest(cmdTypeKeepAlive, EMPTY, 0);
    
    // Turn on keep alive LED
    digitalWrite(keepAliveLedPin, 1);
  }
  else if (now - lastKeepAliveSent > 100) {
    // Turn off keep alive LED
    digitalWrite(keepAliveLedPin, 0);
  }

}