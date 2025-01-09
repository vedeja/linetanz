enum messageType {
  msgTypeRequest,
  msgTypeResponse,
  msgTypeBroadcast,
  msgTypeError
};

const char* getMessageTypeName(messageType type) {
    switch (type) {
        case msgTypeRequest: return "Request";        
        default: return "UNKNOWN";
    }
}