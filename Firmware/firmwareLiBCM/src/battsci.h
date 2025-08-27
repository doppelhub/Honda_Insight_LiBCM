//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef battsci_h
    #define battsci_h

    #define BATTSCI_BYTES_IN_FRAME 12
    #define RUNNING 1
    #define STOPPED 0

    //BATTSCI chargeRequestByte values
    #define BATTSCI_IMA_START_ALLOWED     0x40 //use IMA to start engine
    #define BATTSCI_IMA_START_DISABLED    0x20 //use backup starter
    #define BATTSCI_NO_CHARGE_REQUEST     0x12 //engine started
    #define BATTSCI_REQUEST_REGEN_FLAG    0x20 //request strong background regen
    #define BATTSCI_REQUEST_NO_REGEN_FLAG 0x40 //request no background regen

    //BATTSCI disable regen/assist flags
    #define BATTSCI_DISABLE_ASSIST_FLAG 0x10
    #define BATTSCI_DISABLE_REGEN_FLAG  0x20

    void BATTSCI_begin();

    void BATTSCI_enable();

    void BATTSCI_disable();

    void BATTSCI_sendFrames();

    void BATTSCI_setPackVoltage(uint8_t voltage);

    void BATTSCI_setSpoofedCurrent_deciAmps(int16_t deciAmps);

    uint8_t BATTSCI_writeByte(uint8_t data);

    void BATTSCI_framePeriod_ms_set(uint8_t period);
    uint8_t BATTSCI_framePeriod_ms_get(void);

#endif
