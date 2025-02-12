#ifndef MYHEADER_H
#define MYHEADER_H

#include "messageType.h"
#include "commandType.h"


class dlMessage {

public:

  const char MESSAGE_START = 0xAB;

  dlMessage(messageType t, commandType c, uint8_t sequenceNumber, uint8_t b[], uint8_t size)
    : type(t), command(c), sequenceNumber(sequenceNumber), size(size) {
    body = new uint8_t[size];
    memcpy(body, b, size);    
    delete[] b;
  }

  int index;
  uint8_t sequenceNumber;
  enum messageType type;
  enum commandType command;
  uint8_t size;
  uint8_t* body;

  ~dlMessage() {
    delete[] body;  // Ensure body memory is freed
  }

  static dlMessage* fromData(uint8_t data[]) {

    // TODO: validate header and body checksums, and handle invalid messages by throwing or returning some special error object
    // if (data[0] != MESSAGE_START) {
    //   SLog.notice("Message did not start with 0xAB"CR);
    //   return
    // }

    uint8_t seqno = data[1];
    messageType type = getMessageType(data[4]);
    commandType command = getCommandType(data[5]);

    int bodyChunkCount1 = data[3];
    int bodyLength = bodyChunkCount1 * 4;
    int bodyOffset = 8;

    // Create body
    uint8_t* body = new uint8_t[bodyLength];

    // Copy relevant bytes from data into body
    for (size_t i = 0; i < bodyLength; i++) {
      body[i] = data[bodyOffset + i];
    }

    return new dlMessage(type, command, seqno, body, bodyLength);
  }
};

#endif  // MYHEADER_H