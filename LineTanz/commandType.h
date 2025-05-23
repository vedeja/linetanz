#ifndef CMDTYPE_H
#define CMDTYPE_H

enum commandType {
  //cmdX = 0x00 ?
  cmdTypeKeepAlive = 0x01, // Used as the very first request message, and sent every 2.5 seconds by the client to keep the connection alive.
  cmdTypeHandshake = 0x03, //Sent once from each side early on, to establish basic client and mixer information.
  cmfFirmwareInfo = 0x04, //Sent once from client to mixer, this requests detailed version information.
  cmdTypeMessageSizeControl = 0x06, //Indicates the size of message the requester is willing to receive.
  //cmdTypePlayback = 0x07, //Requested by app with body 0000000b 00000002 00000000 or 0000  00 Song_1000b 00000001 00000000 or 0000000b 000003e8 42203430 20746165 00007469 => mixer response 0000000b
  // = 0x09, //Requested by app with body 00000002 00000000 => mixer response 00000002 00000000 000028e4 00000001 94e91824
  // = 0x0A, //Requested by app with body 80000007 676e6f53 61642e73 00000074 => mixer response 00000001 00000000
  // = 0x0B, //Requested by app with body 00000000 => mixer response without body
  // = 0x0C, //Requested by app with body 00000000 000028e4 00000000 000028e4 => mixer response? (note, also requested by mixer) Potentially playlist content?
  cmdTypeInfo = 0x0E, //Requests various kinds of general information from the peer.
  cmdTypeTransport = 0x0F, //Broadcast from the mixer when the selected song was changed
  cmdTypeChannelValues = 0x13, //Used to both report channel information (e.g. fader position, muting etc), or indicate a change to that information (e.g. "please mute this channel"). Also used to report meter information.
  cmdTypeMeterLayout = 0x15, //Client request for the mixer to report meter information periodically
  cmdTypeMeterControl = 0x16, //Client request indicating which meter values should be reported
  cmdTypeChannelNames = 0x18, //sed to report channel names.
  //cmdX = 0x95 Requested when confirming to stop recording?
};

const char* getCommandTypeName(commandType command) {
  switch (command) {
    case cmdTypeKeepAlive: return "KeepAlive";
    case cmdTypeHandshake: return "Handshake";
    case cmfFirmwareInfo: return "FirmwareInfo";
    case cmdTypeMessageSizeControl: return "MessageSizeControl";
    //ase cmdTypePlayback: return "Playback";
    case cmdTypeInfo: return "Info";
    case cmdTypeTransport: return "TransportControl";
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
        case 0x04:
          return cmfFirmwareInfo;
        case 0x06:
          return cmdTypeMessageSizeControl;
        //case 0x07:
          //return cmdTypeSongSelect;
        case 0x0E:
          return cmdTypeInfo;
        case 0x0F:
          return cmdTypeTransport;
        case 0x13:
          return cmdTypeChannelValues;
        case 0x15:
          return cmdTypeMeterLayout;
        case 0x16:
          return cmdTypeMeterControl;
        case 0x18:
          return cmdTypeChannelNames;
    }
}

#endif // CMDTYPE_H