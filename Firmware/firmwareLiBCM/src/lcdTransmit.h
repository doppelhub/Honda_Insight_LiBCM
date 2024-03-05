//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef lcdTransmit_h
    #define lcdTransmit_h

    #define SCREEN_DIDNT_UPDATE false
    #define SCREEN_UPDATED      true

    //define screen elements //one screen element is updated at a time, using round robbin state machine 
    #define LCDVALUE_NO_UPDATE        0
    #define LCDVALUE_SECONDS          1
    #define LCDVALUE_VPACK_ACTUAL     2
    #define LCDVALUE_VPACK_SPOOFED    3
    #define LCDVALUE_LTC6804_ERRORS   4
    #define LCDVALUE_CELL_HI          5
    #define LCDVALUE_CELL_LO          6
    #define LCDVALUE_CELL_DELTA       7
    #define LCDVALUE_POWER            8
    #define LCDVALUE_CELL_MAXEVER     9
    #define LCDVALUE_CELL_MINEVER    10
    #define LCDVALUE_SoC             11
    #define LCDVALUE_CURRENT         12
    #define LCDVALUE_TEMP_BATTERY    13
    #define LCDVALUE_FAN_STATUS      14
    #define LCDVALUE_GRID_STATUS     15
    #define LCDVALUE_HEATER_STATUS   16
    #define LCDVALUE_FLASH_BACKLIGHT 17
    //
    #define LCDVALUE_MAX_VALUE       17 //must be equal to the highest defined number (above)
    #define MAX_LCDVALUE_ATTEMPTS LCDVALUE_MAX_VALUE

    //the following static text never changes, and is only sent once each time the display turns on
    #define LCDSTATIC_SET_DEFAULTS   18
    #define LCDSTATIC_SECONDS        19
    #define LCDSTATIC_VPACK_ACTUAL   20
    #define LCDSTATIC_VPACK_SPOOFED  21
    #define LCDSTATIC_CHAR_FLAGS     22
    #define LCDSTATIC_CELL_HI        23
    #define LCDSTATIC_CELL_LO        24
    #define LCDSTATIC_CELL_DELTA     25
    #define LCDSTATIC_POWER          26
    #define LCDSTATIC_CELL_MAXEVER   27
    #define LCDSTATIC_CELL_MINEVER   28
    #define LCDSTATIC_SoC            29
    #define LCDSTATIC_CURRENT        30
    #define LCDSTATIC_TEMP_BATTERY   31
    //
    #define LCDSTATIC_MAX_VALUE      31 //must be equal to the highest static number (above)

    #define BACKLIGHT_FLASHING_PERIOD_ms 200

    void lcdTransmit_begin(void);
    void lcdTransmit_end(void);

    void lcdTransmit_displayOn(void);
    void lcdTransmit_displayOff(void);

    void lcdTransmit_printNextElement(void); //primary interface //each call updates one screen element

    void lcdTransmit_splashscreenKeyOff(void);

    void lcdTransmit_testText(void);

    #define LCD_WARN_KEYON_GRID 1
    #define LCD_WARN_FW_EXPIRED 2
    #define LCD_WARN_COVER_GONE 3
    #define LCD_WARN_CELL_COUNT 4
    void lcdTransmit_Warning(uint8_t warningToDisplay);

#endif
