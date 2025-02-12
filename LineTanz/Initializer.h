#include <ArduinoLog.h>
#include "MixerClient.h" 

class Initializer {

  class SequencedCommand {
  public:
    uint8_t sequenceNumber;
    commandType command;
  };

public:
  int initializationStatus;

  Initializer(MixerClient* client) {
    this->client = client;
    initializationStatus = 0;
    isInitialized = false;

    for (int i = 0; i < 8; i++) {
      outgoingInitMessages[i] = nullptr;  // Initialize the array with nullptr
    }
  }
  ~Initializer();

  bool ensureInitialized() {
    if (!isInitialized) {

      if (initializationStatus == 0) {
        sendInitRequest(cmdTypeKeepAlive, EMPTY, 0);
        increaseInitStatus();
      } else if (initializationStatus == 2) {
        uint8_t *body = new uint8_t[8]{ 0, 0, 0, 0, 0, 0, 0, 0 };
        sendInitRequest(cmdTypeMessageSizeControl, body, 8);
        increaseInitStatus();
      } else if (initializationStatus == 4) {
        sendInitRequest(cmdTypeHandshake, EMPTY, 0);
        increaseInitStatus();
      } else if (initializationStatus == 6) {
        uint8_t *body = new uint8_t[4]{ 0, 0, 0, 0x02 };
        sendInitRequest(cmdTypeInfo, body, 4);
        increaseInitStatus();
      } else if (initializationStatus == 8) {
        increaseInitStatus();
        isInitialized = true;
        Log.noticeln("Init done");
      }
    }

    return isInitialized;
  }

  void handleIncomingMessage(dlMessage *m) {
    if (!isInitialized && m->type == msgTypeResponse) {
      bool isHandled = handleInitializationResponse(m);
    }
  }

private:
  MixerClient* client;
  uint8_t *EMPTY = new uint8_t[0];
  bool isInitialized;
  SequencedCommand *outgoingInitMessages[8];

  void increaseInitStatus() {
    initializationStatus++;

    char *description;

    switch (initializationStatus) {
      case 1:
        description = "Awaiting KeepAlive response";
        break;
      case 2:
        description = "KeepAlive response received";
        break;
      case 3:
        description = "Awaiting MessageSizeControl response";
        break;
      case 4:
        description = "MessageSizeControl response received";
        break;
      case 5:
        description = "Awaiting Handshake response";
        break;
      case 6:
        description = "Handshake response received";
        break;
      case 7:
        description = "Awaiting Info response";
        break;
      case 8:
        description = "Info response received";
        break;
      case 9:
        description = "Initialized";
        break;
    }

    Log.noticeln("Updated init status to: %d (%s)", initializationStatus, description);
  }

  bool handleInitializationResponse(dlMessage *response) {
    int previousInitStatus = initializationStatus - 1;
    SequencedCommand *request = outgoingInitMessages[previousInitStatus];

    if (request == nullptr) {
      Log.noticeln("Request was null");
      return false;
    }

    if (request->command != response->command) {
      Log.notice("Request command was ");
      Log.notice("%d", request->command);
      Log.notice(" but the response command was ");
      Log.notice("%d", response->command);
      return false;
    }

    // TODO: Maybe redundant:
    outgoingInitMessages[previousInitStatus] = nullptr;

    switch (initializationStatus) {
      case 1:
        increaseInitStatus();
        break;

      case 3:
        increaseInitStatus();
        break;

      case 5:
        increaseInitStatus();
        break;

      case 7:
        increaseInitStatus();
        break;

      default:
        break;
    }

    return true;
  }

  void sendInitRequest(commandType command, uint8_t body[], int size) {
    char seqNo = client->sendRequest(command, body, size);

    SequencedCommand *sc = new SequencedCommand;
    sc->sequenceNumber = seqNo;
    sc->command = command;
    outgoingInitMessages[initializationStatus] = sc;
  }
};