#ifndef MIXERCLIENT_H
#define MIXERCLIENT_H

#include <WiFiS3.h>
#include <ArduinoLog.h>
#include "secrets.h"

WiFiClient tcpClient;

class MixerClient {

public:

  bool isConnected;

  MixerClient() {
    seqNo = 0;
  }

  void setupTcp() {
    // Define values in the file secrets.h
    char tcpServer[] = SECRET_TCP_SERVER_ADDR;
    uint16_t tcpPort = SECRET_TCP_SERVER_PORT;

    Log.noticeln("Attempting to connect to mixer: %s on port %d", tcpServer, tcpPort);

    if (tcpClient.connect(tcpServer, tcpPort)) {
      Log.noticeln("Connected to mixer");
      tcpClient.flush();
      isConnected = true;
    } else {
      Log.noticeln("Failed to connect to mixer");
      isConnected = false;
    }
  }

  void receive(Inbox* inbox) {
    size_t size = tcpClient.available();

    if (size > 0) {
      Log.noticeln("Received %d bytes", size);
      uint8_t* buffer = new uint8_t[size];
      int bytesRead = tcpClient.read(buffer, size);

      if (bytesRead > 0) {
        inbox->receive(buffer, size);
      }

      // Free the allocated memory
      delete[] buffer;
    }
  }

  char sendResponse(commandType command, uint8_t body[], uint8_t sequenceNumber, int size) {
    return send(msgTypeResponse, command, body, sequenceNumber, size);
  }

  char sendRequest(commandType command, uint8_t body[], int size) {
    return send(msgTypeRequest, command, body, size);
  }

private:

  uint8_t START = 0xAB;
  uint8_t seqNo;

  uint8_t getNextSeqNo() {
    uint8_t n = ++seqNo;
    if (n == 0) {
      n = 1;
    }

    return n;
  }

  char send(messageType type, commandType command, uint8_t body[], int size) {
    uint8_t sequenceNumber = getNextSeqNo();
    send(type, command, body, sequenceNumber, size);
    return seqNo;
  }

  char send(messageType type, commandType command, uint8_t body[], uint8_t sequenceNumber, int size) {
    dlMessage message(type, command, sequenceNumber, body, size);

    send(message);
    return seqNo;
  }

  void send(dlMessage& message) {
    //Log.traceln("OUT <<< %s (%d) %s", getMessageTypeName(message.type), message.sequenceNumber, getCommandTypeName(message.command));
    Log.noticeln("Send   %s", message.toString());

    bool hasBody = message.size > 0;
    uint8_t bodyChunkCount1 = message.size / 4;
    uint8_t bodyChunkCount2 = 0;  //TODO: handle such large bodies, larger than 255 chunks? Yes, probably - song playback messages appear to have 108 bytes of data in the body
    uint16_t headerChecksum = 0xFFFF - (START + message.sequenceNumber + bodyChunkCount2 + bodyChunkCount1 + message.type + message.command);
    uint8_t hc1 = (headerChecksum >> 8) & 0xFF;
    uint8_t hc2 = headerChecksum & 0xFF;

    // Assume no body
    int len = 8;

    if (hasBody) {
      len += message.size;
      len += 4;  // space for body checksum
    }

    uint8_t data[len] = {
      START,
      message.sequenceNumber,
      bodyChunkCount2,
      bodyChunkCount1,
      message.type,
      message.command,
      hc1,
      hc2
    };

    // data[0] = START;
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
};

#endif  // MIXERCLIENT_H