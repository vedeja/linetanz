enum commandType {
  cmdTypeKeepAlive = 0x01
};

const char* getCommandTypeName(commandType command) {
    switch (command) {
        case cmdTypeKeepAlive: return "KeepAlive";        
        default: return "UNKNOWN";
    }
}