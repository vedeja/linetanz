#ifndef MSGTYPE_H
#define MSGTYPE_H

enum messageType {
  msgTypeRequest = 0x00,
  msgTypeResponse = 0x01,
  msgTypeError = 0x05,
  msgTypeBroadcast = 0x08  
};

const char* getMessageTypeName(messageType type) {
    switch (type) {
        case msgTypeRequest: return "Request";
        case msgTypeResponse: return "Response";
        case msgTypeError: return "Error";
        case msgTypeBroadcast: return "Broadcast";
        
        default: return "UNKNOWN";
    }
}

messageType getMessageType(uint8_t value) {
    switch (value) {
        case 0x00:
          return msgTypeRequest;

        case 0x01:
            return msgTypeResponse;

        case 0x05:
            return msgTypeError;

        case 0x08:
            return msgTypeBroadcast;
    }
}

#endif // MSGTYPE_H