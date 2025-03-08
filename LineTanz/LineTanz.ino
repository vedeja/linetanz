#include "Inbox.h"
#include "Network.h"
#include "Initializer.h"
#include "MixerClient.h"
#include "messageType.h"
#include "commandType.h"
#include "dlMessage.h"
#include "secrets.h"
//#include "Arduino_LED_Matrix.h"
#include <ArduinoLog.h>
#include "SequencedCommand.h"
#include <ArduinoQueue.h>

Network* network;
Initializer* initializer;
Inbox* inbox;
ArduinoQueue<msg> outbox(100);
MixerClient* client;
//ArduinoLEDMatrix matrix;

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
unsigned int incomingRequestCount = 0;
unsigned int outgoingResponseCount = 0;

void setup() {
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_INFO, &Serial);
  //Log.begin(LOG_LEVEL_SILENT, &Serial);
  Log.noticeln("LineTanz");

  // matrix.begin();

  // matrix.loadSequence(LEDMATRIX_ANIMATION_WIFI_SEARCH);
  // matrix.begin();
  // matrix.play(true);

  network = new Network();
  client = new MixerClient();

  network->setupWifi();
  client->setupTcp();
  //matrix.loadFrame(LEDMATRIX_HEART_BIG);

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
    //matrix.loadFrame(LEDMATRIX_DANGER);
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

  handleOutbox();

  reportStatus();
}

void handleIncomingMessages() {

  for (int i = 0; i < inbox->messageCount; i++) {
    dlMessage* m = inbox->messages[i];

    Log.noticeln("Handle %s", m->toString());
    //Log.noticeln("Handle %d", m->command);

    if (initializer->isInitialized) {
      //TODO: handle
      if (m->type == msgTypeRequest) {
        handleIncomingRequest(m);
      } else if (m->type == msgTypeResponse) {
      } else if (m->type == msgTypeBroadcast) {
      } else if (m->type == msgTypeError) {
        while (1)
        {          
        }
        
      }
    } else initializer->handleIncomingMessage(m);

    inbox->remove(m->index);
  }
}

void handleIncomingRequest(dlMessage* message) {
  incomingRequestCount++;
  if (message->command == cmdTypeMessageSizeControl) {
    Log.noticeln(message->toString(true));

    int size = 4;
    uint8_t* body = new uint8_t[size];
    memcpy(body, message->body, size);

    //client->sendResponse(cmdTypeMessageSizeControl, body, message->sequenceNumber, size);
    enqueueResponse(cmdTypeMessageSizeControl, body, message->sequenceNumber, size);

    //delete[] body;
  } else if (message->command == cmdTypeChannelValues) {
    //TODO: indicate a change to that information (e.g. "please mute this channel") ???
    //client->sendResponse(cmdTypeChannelValues, EMPTY, message->sequenceNumber, 0);
    enqueueResponse(cmdTypeChannelValues, EMPTY, message->sequenceNumber, 0);
    //while (1) {}
  } else {
    Log.errorln("Unhandled request! Halting.");
    while (1) {}
  }
}

void handleOutbox() {
  // while (!outbox.isEmpty()) {
  if (outbox.isEmpty()) return;

  msg m = outbox.dequeue();
  //Log.noticeln("Dequeue %s", getMessageTypeName(m.t));

  if (m.t == msgTypeRequest) {
    client->sendRequest(m.c, m.b, m.s);
  } else if (m.t == msgTypeResponse) {    
    client->sendResponse(m.c, m.b, m.q, m.s);
    outgoingResponseCount++;
  }
  delete[] m.b;
  //}
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

        Log.noticeln("FX mute %s", muteState ? "ON" : "OFF");

        int size = 12;

        uint8_t state = muteState ? 0x01 : 0x00;
        
        // 13 67 is the channel number for FX1 input
        uint8_t* body1 = new uint8_t[size]{ 0x00, 0x00, 0x13, 0x67, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, state };
        //client->sendRequest(cmdTypeChannelValues, body, size);
        enqueueRequest(cmdTypeChannelValues, body1, size);

        // 13 C9 is the channel number for FX2 input
        uint8_t* body2 = new uint8_t[size]{ 0x00, 0x00, 0x13, 0xC9, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, state };
        //client->sendRequest(cmdTypeChannelValues, body, size);
        enqueueRequest(cmdTypeChannelValues, body2, size);
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
  int memory = freeRam();
  if (now - lastReportedStatus > 5000) {
    lastReportedStatus = now;
    //Log.noticeln("STATUS | Initialization status: %d | Free SRAM: %d bytes | Inbox messages: %d", initializer->initializationStatus, freeRam(), inbox->messageCount);
    Log.noticeln("STATUS | Free SRAM: %d bytes | Handled mixer requests: %d/%d", freeRam(), outgoingResponseCount, incomingRequestCount);
    
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
    //client->sendRequest(cmdTypeKeepAlive, EMPTY, 0);
    enqueueRequest(cmdTypeKeepAlive, EMPTY, 0);

    // Turn on keep alive LED
    digitalWrite(keepAliveLedPin, 1);
  } else if (now - lastKeepAliveSent > 100) {
    // Turn off keep alive LED
    digitalWrite(keepAliveLedPin, 0);
  }
}

void enqueueRequest(commandType c, uint8_t b[], uint8_t s) {
  enqueue(msgTypeRequest, c, b, 0, s);
}

void enqueueResponse(commandType c, uint8_t b[], uint8_t q, uint8_t s) {
  enqueue(msgTypeResponse, c, b, q, s);
}

void enqueue(messageType t, commandType c, uint8_t b[], uint8_t q, uint8_t s) {
  msg m;
  m.t = t;
  m.c = c;
  m.q = q;
  m.s = s;
  
  memcpy(m.b, b, s);
  delete[] b;


  outbox.enqueue(m);

}
