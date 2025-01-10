#ifndef CMDTYPE_H
#define CMDTYPE_H

enum commandType {
  cmdTypeKeepAlive = 0x01,
  cmdTypeHandshake = 0x03,
  cmdTypeMessageSizeControl = 0x06,
  cmdTypeInfo = 0x0E,
  cmdTypeChannelValues = 0x13
};

const char* getCommandTypeName(commandType command) {
    switch (command) {
        case cmdTypeKeepAlive: return "KeepAlive";
        case cmdTypeHandshake: return "Handshake";
        case cmdTypeMessageSizeControl: return "MessageSizeControl";
        case cmdTypeInfo: return "Info";
        case cmdTypeChannelValues: return "ChannelValues";
        default: return "UNKNOWN";
    }
}

commandType getCommandType(uint8_t value) {
    switch (value) {
        case 0x01:
            return cmdTypeKeepAlive;
        case 0x03:
            return cmdTypeHandshake;
        case 0x06:
          return cmdTypeMessageSizeControl;
        case 0x0E:
          return cmdTypeInfo;
        case 0x13:
          return cmdTypeChannelValues;
    }
}

#endif // CMDTYPE_H