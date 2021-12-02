//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef battsci_h
  #define battsci_h

  #define BATTSCI_BYTES_IN_FRAME 24 //0x87 is 12 bytes //0xAA is 12 bytes 

  void BATTSCI_begin();

  void BATTSCI_enable();

  void BATTSCI_disable();

  uint8_t BATTSCI_readByte();

  uint8_t BATTSCI_bytesAvailableToRead();

  void BATTSCI_processLatestFrame(void);

  void BATTSCI_setSpoofedCurrent(int16_t);

  void BATTSCI_setPackVoltage(uint8_t);

#endif