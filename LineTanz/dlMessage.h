class dlMessage {

  private:
  const char MESSAGE_START = 0xAB;

  public:
  uint8_t sequenceNumber;
  enum messageType type;
  enum commandType command;
  char body;
};