#include "Inbox.h"
#include "messageType.h"
#include "commandType.h"
#include "dlMessage.h"

const int footswitchPin = 2;
const int ledPin = 13;
int muteState = HIGH;
int footswitchState;
int lastFootswitchState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
int keepAliveInterval = 2000;  // 2 seconds between sending keep-alive to the mixer
uint8_t seqNo;
uint8_t EMPTY[0] = {};

unsigned long lastKeepAliveSent;
Inbox *inbox;

void setup() {
  Serial.begin(9600);
  Serial.println("LineTanz");

  pinMode(footswitchPin, INPUT);
  pinMode(ledPin, OUTPUT);

  // set initial LED state
  digitalWrite(ledPin, muteState);

  seqNo = 0;
}

void loop() {
  handleFootswitch();

  // if (initializer.Initialize())
  //       {
  keepAlive();
  //       }

  receive();
  //       HandleIncomingMessages();

  //       // Execute logic
  //       // Handle outgoing messages
}

uint8_t getNextSeqNo() {
  uint8_t n = ++seqNo;
  if (n == 0) {
    n = 1;
  }

  return n;
}

void SerialPrintMessage(dlMessage &message) {
  Serial.print(getMessageTypeName(message.type));
  Serial.print(" (");
  Serial.print(message.sequenceNumber);
  Serial.print(") ");
  Serial.print(getCommandTypeName(message.command));
  Serial.println("");
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
      }
    }
  }

  // set the LED:
  digitalWrite(ledPin, muteState);

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastFootswitchState = reading;
}

void keepAlive() {
  unsigned long now = millis();

  // Send every x milliseconds
  if (now - lastKeepAliveSent > keepAliveInterval) {
    lastKeepAliveSent = now;
    sendRequest(cmdTypeKeepAlive, EMPTY);
  }
}

void receive() {
  // TODO: use TCP instead of serial

  // check for incoming serial data:
  int available = Serial.available();
  if (available > 0) {

    // read incoming serial data:

    char buffer[available];
    int bytesRead = Serial.readBytes(buffer, available);

    Serial.print("Received: ");
    Serial.println(bytesRead);

    if (bytesRead > 0) {
      inbox->receive(buffer);
    }
  }
}

char sendRequest(commandType command, uint8_t body[]) {
  return send(msgTypeRequest, command, body);
}

char send(messageType type, commandType command, uint8_t body[]) {
  uint8_t sequenceNumber = getNextSeqNo();
  dlMessage message(type, command, sequenceNumber, body);

  send(message);
  return seqNo;
}

void send(dlMessage &message) {
  //var data = Serializer.Serialize(message);

  Serial.print("OUT <<< ");
  SerialPrintMessage(message);

  //socket.Send(data);
}