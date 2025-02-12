#ifndef CMDTYPE_H
#define CMDTYPE_H

enum commandType {
  cmdTypeKeepAlive = 0x01,
  cmdTypeHandshake = 0x03,
  cmfFirmwareInfo = 0x04,
  cmdTypeMessageSizeControl = 0x06,
  cmdTypeInfo = 0x0E,
  cmdTypeChannelValues = 0x13,
  cmdTypeMeterLayout = 0x15, // aka Broadcast control
  cmdTypeMeterControl = 0x16,
  cmdTypeChannelNames = 0x18,
};

const char* getCommandTypeName(commandType command) {
  switch (command) {
    case cmdTypeKeepAlive: return "KeepAlive";
    case cmdTypeHandshake: return "Handshake";
    case cmfFirmwareInfo: return "FirmwareInfo";
    case cmdTypeMessageSizeControl: return "MessageSizeControl";
    case cmdTypeInfo: return "Info";
    case cmdTypeChannelValues: return "ChannelValues";
    case cmdTypeMeterLayout: return "MeterLayout";
    case cmdTypeMeterControl: return "MeterControl";
    case cmdTypeChannelNames: return "ChannelNames";
    default: {
      static char formattedString[50];
      snprintf(formattedString, 50, "UNKNOWN (0x%02X)", command);
      return formattedString;
    }
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