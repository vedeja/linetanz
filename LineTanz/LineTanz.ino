#include <WDT.h>
#include <ArduinoLog.h>
#include <WiFiS3.h>
#include <queue>
#include <map>
#include "Network.h"
#include "messageType.h"
#include "commandType.h"

#define BUTTON_PIN 2

Network *network;
WiFiClient tcpClient;

uint8_t START = 0xAB;
uint8_t *EMPTY = new uint8_t[0];

const int muteLedPin = 12;
const int keepAliveLedPin = 13;
volatile byte isrButtonState = LOW;
byte buttonState = LOW;
unsigned long lastSwitchPress = 0;
const long wdtInterval = 2684;
unsigned long wdtMillis = 0;

bool isTcpConnected;
const char *tcpServer;
bool isFx1Muted;
bool isFx2Muted;
bool muteChanged;
bool muteChangedRemotely;
unsigned long lastReportedStatus;
unsigned long lastKeepAliveSent;
uint8_t seqNo;

byte *tcpBuffer = nullptr;
int tcpBufferSize = 0;

std::queue<std::pair<byte *, int>> rawMessageQueue;
std::map<uint8_t, std::pair<commandType, bool>> outgoingMessages;

enum systemState
{
    pristine,
    connecting,
    connected,
    initKeepAliveSent,
    initKeepAliveReceived,
    initMessageSizeControlSent,
    initMessageSizeControlReceived,
    initHandshakeSent,
    initHandshakeReceived,
    initInfoSent,
    initInfoReceived,
    running
};

systemState state;

void setup()
{
    Serial.begin(115200);
    //Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    Log.begin(LOG_LEVEL_SILENT, &Serial);
    Log.noticeln("LineTanz v2");

    tcpServer = SECRET_TCP_SERVER_ADDR;

    // hw setup
    pinMode(muteLedPin, OUTPUT);
    pinMode(keepAliveLedPin, OUTPUT);
    // set initial LED state
    digitalWrite(muteLedPin, false);
    digitalWrite(keepAliveLedPin, false);
    pinMode(BUTTON_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleSwitchPress, RISING);

    // join wifi
    network = new Network();
    network->setupWifi();

    if(WDT.begin(wdtInterval)) {
    Serial.print("WDT interval: ");
    WDT.refresh();
    Serial.print(WDT.getTimeout());
    WDT.refresh();
    Serial.println(" ms");
    WDT.refresh();
  } else {
    Serial.println("Error initializing watchdog");
    while(1){}
  }

    state = pristine;
    seqNo = 0;
    Log.noticeln("Setup complete");
}

void loop()
{
    network->connect();
    connectTcp(tcpServer);
    readTcp();
    parseTcpBuffer();
    processIncomingMessage();
    initMixerConnection();
    keepAlive();
    handleFootswitch();
    updateMuteLed();
    sendFxMute();
    reportStatus();

    // Reset for next loop
    muteChanged = false;
    muteChangedRemotely = false;

    if(millis() - wdtMillis >= wdtInterval - 1) {
    WDT.refresh(); // Comment this line to stop refreshing the watchdog
    wdtMillis = millis();
  }
}

void handleSwitchPress() {
    
    // debounce
    if (millis() - lastSwitchPress < 500) {
        return;
    }

    buttonState = !buttonState;
    lastSwitchPress = millis();
}

void sendFxMute()
{
    if (muteChanged)
    {
        int size = 12;

        // 0x13 0x67 is the channel number for FX1 input
        uint8_t *body1 = new uint8_t[size]{0x00, 0x00, 0x13, 0x67, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, isFx1Muted ? 0x01 : 0x00};
        send(getNextSeqNo(), msgTypeRequest, cmdTypeChannelValues, body1, size, false);

        // 0x13 0xC9 is the channel number for FX2 input
        uint8_t *body2 = new uint8_t[size]{0x00, 0x00, 0x13, 0xC9, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, isFx2Muted ? 0x01 : 0x00};
        send(getNextSeqNo(), msgTypeRequest, cmdTypeChannelValues, body2, size, false);

        delete[] body1;
        delete[] body2;
    }
}

void handleFootswitch()
{
    if (buttonState != isrButtonState)
    {
        buttonState = isrButtonState;

        muteChanged = true;
        isFx1Muted = !isFx1Muted;
        isFx2Muted = !isFx2Muted;
    }
}

void updateMuteLed()
{
    if (muteChanged || muteChangedRemotely)
    {
        // update the mute LED
        Log.noticeln("Mute changed: %d %d", isFx1Muted, isFx2Muted);
        digitalWrite(muteLedPin, isFx1Muted && isFx2Muted);
    }
}

void initMixerConnection()
{
    switch (state)
    {
    case connected:
        send(getNextSeqNo(), msgTypeRequest, cmdTypeKeepAlive, EMPTY, 0, true);
        state = initKeepAliveSent;
        break;

    case initKeepAliveReceived:
        send(getNextSeqNo(), msgTypeRequest, cmdTypeMessageSizeControl, new uint8_t[8]{0, 0, 0, 0, 0, 0, 0, 0}, 8, true);
        state = initMessageSizeControlSent;
        break;

    case initMessageSizeControlReceived:
        send(getNextSeqNo(), msgTypeRequest, cmdTypeHandshake, EMPTY, 0, true);
        state = initHandshakeSent;
        break;

    case initHandshakeReceived:
        send(getNextSeqNo(), msgTypeRequest, cmdTypeInfo, new uint8_t[4]{0, 0, 0, 0x02}, 4, true);
        state = initInfoSent;
        break;

    case initInfoReceived:
        state = running;
        break;

    default:
        break;
    }
}

uint8_t getNextSeqNo()
{
    uint8_t n = ++seqNo;
    if (n == 0)
    {
        n = 1;
    }

    return n;
}

void keepAlive()
{
    unsigned long now = millis();
    // Send every x milliseconds
    if (state == running && now - lastKeepAliveSent > 3000)
    {
        lastKeepAliveSent = now;
        send(getNextSeqNo(), msgTypeRequest, cmdTypeKeepAlive, EMPTY, 0, false);
        Log.noticeln("Keep alive sent");
        
        // Turn on keep alive LED
        digitalWrite(keepAliveLedPin, 1);
    }
    else if (now - lastKeepAliveSent > 100) {
        // Turn off keep alive LED
        digitalWrite(keepAliveLedPin, 0);
  }
    
}

void reportStatus()
{
    unsigned long now = millis();
    // Report every x milliseconds
    if (now - lastReportedStatus > 5000)
    {
        lastReportedStatus = now;
        Log.noticeln("STATUS | Free SRAM: %d bytes | State: %d", freeRam(), state);
    }
}

extern "C" char *sbrk(int incr);

int freeRam()
{
    char top;
    return &top - reinterpret_cast<char *>(sbrk(0));
}

void processIncomingMessage()
{
    if (!rawMessageQueue.empty())
    {
        std::pair<byte *, int> messagePair = rawMessageQueue.front();
        byte *message = messagePair.first;
        int messageLength = messagePair.second;
        rawMessageQueue.pop();

        bool logMessage = true;

        // printBytes(message, messageLength);

        // Extract sequence number
        uint8_t sequenceNumber = message[1];

        // Extract number of 4-byte chunks in the body
        uint16_t numBodyChunks = (message[2] << 8) | message[3];

        // Extract body
        int bodyLength = numBodyChunks * 4;
        uint8_t *body = new uint8_t[bodyLength];
        memcpy(body, message + 8, bodyLength);

        // Extract message type and subtype
        uint8_t messageTypeByte = message[4];
        uint8_t commandTypeByte = message[5];

        messageType type = getMessageType(messageTypeByte);
        commandType command = getCommandType(commandTypeByte);

        const char *messageTypeName = getMessageTypeName(type);
        const char *commandTypeName = getCommandTypeName(command);

        if (type == msgTypeResponse)
        {
            // get the command that was sent
            std::pair<commandType, bool> outgoingMessage = outgoingMessages[sequenceNumber];
            commandType outgoingCommand = outgoingMessage.first;
            bool isInitRequest = outgoingMessage.second;

            if (command != outgoingCommand)
            {
                Log.errorln("Response command type does not match outgoing command type");
                return;
            }

            if (state == running)
            {
                // handle running response
            }
            else
            {
                handleInitResponse(isInitRequest, command);
            }

            // remove outgoing message
            outgoingMessages.erase(sequenceNumber);
        }
        else if (type == msgTypeRequest)
        {
            if (state == running)
            {
                handleRunningRequest(command, body, bodyLength, sequenceNumber);
            }
            else
            {
                // handle init request
            }
        }

        if (logMessage)
        {
            const char *verb = (type == msgTypeRequest ? "for" : "to");
            Log.noticeln("Seq: %d | %s %s %s | Body chunks: %d", sequenceNumber, messageTypeName, verb, commandTypeName, numBodyChunks);
        }

        // Free the allocated memory for the message
        delete[] message;
    }
}

void printBytes(byte *bytes, int length)
{
    Serial.print("Body bytes: ");
    for (int i = 0; i < length; i++)
    {
        if (bytes[i] < 0x10)
        {
            Serial.print("0");
        }
        Serial.print(bytes[i], HEX);
        if ((i + 1) % 4 == 0 && i != length - 1)
        {
            Serial.print(", ");
        }
        else
        {
            Serial.print(" ");
        }
    }
    Serial.println();
}

void handleRunningRequest(commandType command, byte *messageBody, int bodyLength, uint8_t sequenceNumber)
{
    if (command == cmdTypeMessageSizeControl)
    {
        uint8_t body[4];
        memcpy(body, messageBody, 4); // Copy bytes 0-3 from the received body
        send(sequenceNumber, msgTypeResponse, cmdTypeMessageSizeControl, body, 4, false);
    }
    else if (command == cmdTypeHandshake)
    {
        // handle handshake request
    }
    else if (command == cmdTypeInfo)
    {
        // handle info request
    }
    else if (command == cmdTypeChannelValues)
    {
        printBytes(messageBody, bodyLength);

        // https://github.com/jskeet/DemoCode/blob/main/DigiMixer/DigiMixer.Mackie/DL32RProfile.cs
        const uint16_t fx1MuteAddress = 0x1367;
        const uint16_t fx2MuteAddress = 0x13C9;

        uint32_t mixerAddress = (messageBody[0] << 24) | (messageBody[1] << 16) | (messageBody[2] << 8) | messageBody[3];
        uint16_t dataCount = (messageBody[4] << 8) | messageBody[5];
        uint8_t valueType = messageBody[6];

        // create an array of 4-byte values, length is dataCount
        uint32_t *values = new uint32_t[dataCount];
        for (int i = 0; i < dataCount; i++)
        {
            int offset = 8 + (i * 4);
            values[i] = (messageBody[offset] << 24) | (messageBody[offset + 1] << 16) | (messageBody[offset + 2] << 8) | messageBody[offset + 3];
        }

        if (mixerAddress == fx1MuteAddress)
        {
            isFx1Muted = messageBody[11] == 0x01;
            muteChangedRemotely = true;
        }
        else if (mixerAddress == fx2MuteAddress)
        {
            isFx2Muted = messageBody[11] == 0x01;
            muteChangedRemotely = true;
        }

        // unmute fx1: (seqno 4)
        // AB 4 0 3 0 13 FF 3A    0 0 13 67, 0 1 5 0, 0 0 0 0    FF FF FF 7F

        // mute fx1: (seqno 3)
        // AB 3 0 3 0 13 FF 3B    0 0 13 67, 0 1 5 0, 0 0 0 1    FF FF FF 7E
    }
    else if (command == cmdTypeKeepAlive)
    {
        // handle keep alive request
    }
    else
    {
        Log.errorln("Unknown command type");
    }
}

void handleInitResponse(bool isInitRequest, commandType command)
{
    if (state == initKeepAliveSent && isInitRequest && command == cmdTypeKeepAlive)
    {
        state = initKeepAliveReceived;
    }
    else if (state == initMessageSizeControlSent && isInitRequest && command == cmdTypeMessageSizeControl)
    {
        state = initMessageSizeControlReceived;
    }
    else if (state == initHandshakeSent && isInitRequest && command == cmdTypeHandshake)
    {
        state = initHandshakeReceived;
    }
    else if (state == initInfoSent && isInitRequest && command == cmdTypeInfo)
    {
        state = initInfoReceived;
    }
}

void connectTcp(const char *tcpServer)
{
    if (tcpClient.connected())
    {
        return;
    }

    state = connecting;
    isTcpConnected = false;

    Log.noticeln("Attempting to connect to mixer: %s on port %d", tcpServer, 50001);

    if (tcpClient.connect(tcpServer, 50001))
    {
        Log.noticeln("Connected to mixer");
        tcpClient.flush();
        isTcpConnected = true;
        state = connected;
    }
    else
    {
        Log.noticeln("Failed to connect to mixer");
        isTcpConnected = false;
    }
}

void parseTcpBuffer()
{
    if (tcpBufferSize > 0)
    {
        // Look for the start byte 0xAB
        int startIndex = -1;
        for (int i = 0; i < tcpBufferSize; i++)
        {
            if (tcpBuffer[i] == START)
            {
                startIndex = i;
                break;
            }
        }

        if (startIndex > 0)
        {
            Log.warningln("Skipping %d bytes", startIndex);
        }

        int bytesRemaining = tcpBufferSize - startIndex;
        if (bytesRemaining < 8)
        {
            return; // Not enough bytes for a complete, minimum length message
        }

        // Calculate header checksum
        uint16_t headerChecksum = 0xFFFF;
        for (int i = 0; i < 6; i++)
        {
            headerChecksum -= tcpBuffer[startIndex + i];
        }

        // Extract checksum from message
        uint16_t messageChecksum = (tcpBuffer[startIndex + 6] << 8) | tcpBuffer[startIndex + 7];

        // Validate checksum
        if (headerChecksum != messageChecksum)
        {
            Log.errorln("Invalid checksum");
            // TODO: remove message from buffer
            return;
        }

        // Extract number of 4-byte chunks in the body
        uint16_t numBodyChunks = (tcpBuffer[startIndex + 2] << 8) | tcpBuffer[startIndex + 3];
        bool hasBody = numBodyChunks > 0;
        int messageLength = 8 + (numBodyChunks * 4) + (hasBody ? 4 : 0);
        int endIndex = startIndex + messageLength;

        // Throw away broadcast messages
        uint8_t messageTypeByte = tcpBuffer[startIndex + 4];
        if (messageTypeByte != msgTypeBroadcast)
        {
            if (endIndex > tcpBufferSize)
            {
                return; // Not enough bytes for a complete message, body and body checksum included
            }

            if (hasBody)
            {
                // Check body checksum
                uint32_t bodyChecksum = 0xFFFFFFFF;

                for (int i = 8; i < 8 + numBodyChunks * 4; i++)
                {
                    bodyChecksum -= tcpBuffer[startIndex + i];
                }

                uint32_t messageBodyChecksum = (static_cast<uint32_t>(tcpBuffer[startIndex + 8 + numBodyChunks * 4]) << 24) |
                                               (static_cast<uint32_t>(tcpBuffer[startIndex + 9 + numBodyChunks * 4]) << 16) |
                                               (static_cast<uint32_t>(tcpBuffer[startIndex + 10 + numBodyChunks * 4]) << 8) |
                                               static_cast<uint32_t>(tcpBuffer[startIndex + 11 + numBodyChunks * 4]);

                if (bodyChecksum != messageBodyChecksum)
                {
                    Log.errorln("Invalid body checksum");
                    // TODO: remove message from buffer
                    return;
                }
            }

            // Extract and enqueue the message
            byte *message = new byte[messageLength];
            memcpy(message, tcpBuffer + startIndex, messageLength);
            rawMessageQueue.push(std::make_pair(message, messageLength));
        }

        // Remove the processed message from the buffer
        int newBufferSize = tcpBufferSize - endIndex;
        byte *newBuffer = nullptr;
        if (newBufferSize > 0)
        {
            newBuffer = new byte[newBufferSize];
            memcpy(newBuffer, tcpBuffer + endIndex, newBufferSize);
        }
        delete[] tcpBuffer;
        tcpBuffer = newBuffer;
        tcpBufferSize = newBufferSize;
    }
}

void readTcp()
{
    if (!isTcpConnected)
    {
        return;
    }

    size_t size = tcpClient.available();

    if (size > 0)
    {
        uint8_t *buffer = new uint8_t[size];
        int bytesRead = tcpClient.read(buffer, size);

        if (bytesRead > 0)
        {
            byte *newArray = appendBytes(tcpBuffer, tcpBufferSize, buffer, size);
            delete[] tcpBuffer; // Free the old buffer
            tcpBuffer = newArray;
            tcpBufferSize += size;
        }

        // Free the allocated memory
        delete[] buffer;
    }
}

byte *appendBytes(byte *arr, int arrSize, byte *appendArr, int appendSize)
{
    // Allocate memory for the new array
    byte *newArray = (byte *)malloc(arrSize + appendSize);

    // Copy the original array to the new array
    for (int i = 0; i < arrSize; i++)
    {
        newArray[i] = arr[i];
    }

    // Append the new bytes to the new array
    for (int i = 0; i < appendSize; i++)
    {
        newArray[arrSize + i] = appendArr[i];
    }

    // Free the original array
    if (arr != nullptr)
    {
        free(arr);
    }

    return newArray;
}

void send(u_int8_t sequenceNumber, messageType type, commandType command, uint8_t body[], int size, bool isInitRequest)
{
    outgoingMessages[sequenceNumber] = std::make_pair(command, isInitRequest);

    bool hasBody = size > 0;
    uint8_t bodyChunkCount1 = size / 4;
    uint8_t bodyChunkCount2 = 0; // TODO: handle such large bodies, larger than 255 chunks? Yes, probably - song playback messages appear to have 364 bytes of data in the body
    uint16_t headerChecksum = 0xFFFF - (START + sequenceNumber + bodyChunkCount2 + bodyChunkCount1 + type + command);
    uint8_t hc1 = (headerChecksum >> 8) & 0xFF;
    uint8_t hc2 = headerChecksum & 0xFF;

    // Assume no body
    int len = 8;

    if (hasBody)
    {
        len += size;
        len += 4; // space for body checksum
    }

    uint8_t data[len] = {
        START,
        sequenceNumber,
        bodyChunkCount2,
        bodyChunkCount1,
        type,
        command,
        hc1,
        hc2};

    if (hasBody)
    {
        uint32_t bodyChecksum = 0xFFFFFFFF;

        for (int i = 0; i < size; i++)
        {
            uint8_t b = body[i];
            data[8 + i] = b;

            // Calculate checksum
            bodyChecksum -= b;
        }

        uint8_t bc0 = (bodyChecksum >> 24) & 0xFF;
        uint8_t bc1 = (bodyChecksum >> 16) & 0xFF;
        uint8_t bc2 = (bodyChecksum >> 8) & 0xFF;
        uint8_t bc3 = bodyChecksum & 0xFF;

        int bcOffset = 8 + size;

        data[bcOffset + 0] = bc0;
        data[bcOffset + 1] = bc1;
        data[bcOffset + 2] = bc2;
        data[bcOffset + 3] = bc3;
    }

    // Send the byte array
    if (!tcpClient.write(data, len) == len)
    {
        Log.errorln("Failed to send byte array");
    }
}