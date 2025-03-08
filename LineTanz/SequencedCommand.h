#ifndef SEQCMD_H
#define SEQCMD_H

  class SequencedCommand {
  public:
    uint8_t sequenceNumber;
    commandType command;
  };

#endif