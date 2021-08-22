#ifndef metsci_h
  #define metci_h

  #define METSCI_BYTES_IN_FRAME 6
  #define RUNNING 1
  #define STOPPED 0

  void METSCI_begin();

  void METSCI_enable();

  void METSCI_disable();

  uint8_t METSCI_getPacketB3();
  uint8_t METSCI_getPacketB4();
  uint8_t METSCI_getPacketE1();
  uint8_t METSCI_getPacketE6();

  uint8_t METSCI_readByte();

  uint8_t METSCI_bytesAvailable();

  void METSCI_processLatestFrame(void);

  uint8_t METSCI_isChecksumValid( uint8_t type, uint8_t data, uint8_t checksum );

#endif