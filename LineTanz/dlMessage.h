class dlMessage {

private:
  const char MESSAGE_START = 0xAB;

public:

  dlMessage(messageType t, commandType c, uint8_t sequenceNumber, uint8_t b[])
    : type(t), command(c), sequenceNumber(sequenceNumber) {
    size = sizeof(b);
    body = new uint8_t[size];
    memcpy(body, b, size);
  }

  uint8_t sequenceNumber;
  enum messageType type;
  enum commandType command;
  uint8_t size;
  uint8_t* body;
};