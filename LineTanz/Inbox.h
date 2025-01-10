#include "dlMessage.h"
#include "Serial.h"

class Inbox {
private:



public:

  dlMessage* messages[16];
  int messageCount;

  Inbox() {
    messageCount = 0;

    for (int i = 0; i < 16; i++) {
      messages[i] = nullptr;  // Initialize the array with nullptr
    }
  }

  void receive(uint8_t data[], size_t size) {
    dlMessage* message = dlMessage::fromData(data);

    Serial.print("IN >>> ");
    Serial.print(getMessageTypeName(message->type));
    Serial.print(" (");
    Serial.print(message->sequenceNumber, HEX);
    Serial.print(") ");
    Serial.print(getCommandTypeName(message->command));
    Serial.print(" { ");

    // Print the received bytes
    for (size_t i = 0; i < size; i++) {
      // Buffer to hold the formatted string
      char stringBuffer[3];  // 2 characters for hex value + 1 for null terminator

      // Print each byte in hexadecimal format with leading zeros
      sprintf(stringBuffer, "%02X", data[i]);

      Serial.print(stringBuffer);
      Serial.print(" ");
    }
    Serial.println("}");

    // Store message
    if (messageCount < 16) {
      message->index = messageCount;
      messages[messageCount] = message;
      messageCount++;
    } else {
      Serial.println("Message list is full!");
      return;
    }
  }

  void remove(int index) {
    if (index < 0 || index >= messageCount) {
      Serial.println("Invalid index!");
      return;
    }
    delete messages[index];  // Free the memory
    messages[index] = nullptr;
    // Shift the remaining messages
    for (int j = index; j < messageCount - 1; j++) {
      messages[j] = messages[j + 1];
    }
    messages[messageCount - 1] = nullptr;
    messageCount--;
  }

  ~Inbox() {
    for (int i = 0; i < messageCount; i++) {
      delete messages[i];
    }
  }
};