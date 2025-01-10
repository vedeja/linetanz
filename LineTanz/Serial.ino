#ifndef SERIAL_H
#define SERIAL_H

void SerialPrintState() {
  Serial.print("State: ");
  Serial.println(muteState);
}


void SerialPrintMessage(dlMessage& message) {
  Serial.print(getMessageTypeName(message.type));
  Serial.print(" (");
  Serial.print(message.sequenceNumber, HEX);
  Serial.print(") ");
  Serial.print(getCommandTypeName(message.command));
  Serial.println("");
}

#endif SERIAL_H