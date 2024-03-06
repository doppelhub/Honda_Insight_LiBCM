//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef lcdTransmit_h
    #define lcdTransmit_h

    #define SCREEN_DIDNT_UPDATE false
    #define SCREEN_UPDATED      true

    //define screen elements //one screen element is updated at a time, using round robbin state machine 
    #define LCDVALUE_NO_UPDATE        0
    #define LCDVALUE_CALC_CYCLEFRAME  1
    #define LCDVALUE_SECONDS          2
    #define LCDVALUE_VPACK_ACTUAL     3
    #define LCDVALUE_VPACK_SPOOFED    4
    #define LCDVALUE_LTC6804_ERRORS   5
    #define LCDVALUE_CELL_HI          6
    #define LCDVALUE_CELL_LO          7
    #define LCDVALUE_CELL_DELTA       8
    #define LCDVALUE_POWER            9
    #define LCDVALUE_CELL_MAXEVER    10
    #define LCDVALUE_CELL_MINEVER    11
    #define LCDVALUE_SoC             12
    #define LCDVALUE_CURRENT         13
    #define LCDVALUE_TEMP_BATTERY    14
    #define LCDVALUE_FAN_STATUS      15
    #define LCDVALUE_GRID_STATUS     16
    #define LCDVALUE_HEATER_STATUS   17
    #define LCDVALUE_BALANCE_STATUS  18
    #define LCDVALUE_FLASH_BACKLIGHT 19
    #define LCDVALUE_MAX_VALUE       19 //must equal the highest defined number (above)

    #define LCD_UPDATE_ATTEMPTS_PER_LOOP 10

    //the following static text never changes, and is only sent once each time the display turns on
    #define LCDSTATIC_SET_DEFAULTS   20
    #define LCDSTATIC_SECONDS        21
    #define LCDSTATIC_VPACK_ACTUAL   22
    #define LCDSTATIC_VPACK_SPOOFED  23
    #define LCDSTATIC_CHAR_FLAGS     24
    #define LCDSTATIC_CELL_HI        25
    #define LCDSTATIC_CELL_LO        26
    #define LCDSTATIC_CELL_DELTA     27
    #define LCDSTATIC_POWER          28
    #define LCDSTATIC_CELL_MAXEVER   29
    #define LCDSTATIC_CELL_MINEVER   30
    #define LCDSTATIC_SoC            31
    #define LCDSTATIC_CURRENT        32
    #define LCDSTATIC_TEMP_BATTERY   33
    #define LCDSTATIC_MAX_VALUE      33 //must equal the highest static number (above)

    #define BACKLIGHT_FLASHING_PERIOD_ms 200

    #define CYCLEFRAME_A_PERIOD_ms  3000
    #define CYCLEFRAME_B_PERIOD_ms  7000

    #define CYCLEFRAME_INIT         0
    #define CYCLEFRAME_A            1
    #define CYCLEFRAME_B            2
    #define CYCLEFRAME_MAX_VALUE    2 //must equal the highest defined number (above)

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
