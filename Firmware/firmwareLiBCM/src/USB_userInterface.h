//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef USB_userInterface_h
    #define USB_userInterface_h
    
    #define USER_INPUT_BUFFER_SIZE 32
    #define STRING_TERMINATION_CHARACTER 0

    #define INPUT_FLAG_INSIDE_COMMENT 0x01

    void USB_begin(void);
    void USB_end(void);

    uint8_t USB_userInterface_getUserInput(void);

    void USB_delayUntilTransmitBufferEmpty(void);

    void USB_userInterface_handler(void);

#endif
