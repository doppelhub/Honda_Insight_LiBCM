//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef metsci_h
    #define metci_h

    #define METSCI_BYTES_IN_FRAME 6
    #define RUNNING 1
    #define STOPPED 0

    void METSCI_begin(void);

    void METSCI_enable(void);

    void METSCI_disable(void);

    uint8_t METSCI_getPacketB3(void);
    uint8_t METSCI_getPacketB4(void);
    uint8_t METSCI_getPacketE1(void);
    uint8_t METSCI_getPacketE6(void);

    uint8_t METSCI_readByte(void);

    uint8_t METSCI_bytesAvailableToRead(void);

    void METSCI_processLatestFrame(void);

    uint8_t METSCI_isChecksumValid( uint8_t type, uint8_t data, uint8_t checksum );

#endif
