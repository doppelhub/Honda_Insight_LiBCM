//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef eepromAccess_h
    #define eepromAccess_h

    #define EEPROM_LIBCM_DISABLED_REGEN  0xAA
    #define EEPROM_REGEN_NEVER_LIMITED   0x55
    #define EEPROM_LIBCM_DISABLED_ASSIST 0xCC
    #define EEPROM_ASSIST_NEVER_LIMITED  0x33

    #define EEPROM_ADDRESS_FACTORY_DEFAULT_VALUE 0xFF
    #define EEPROM_ADDRESS_FORMATTED_VALUE       0x00

    #define BYTES_IN_DATE 12 //JTS2doLater: Is date 11 bytes or 12?
    #define BYTES_IN_TIME  9 //JTS2doLater: Is time  9 bytes or  8?

    #define REQUIRED_FIRMWARE_UPDATE_PERIOD_DAYS 120
    #define REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS (REQUIRED_FIRMWARE_UPDATE_PERIOD_DAYS * 24)

    #define FIRMWARE_EXPIRED   0b10101010 //alternating bit pattern for EEPROM read/write integrity
    #define FIRMWARE_UNEXPIRED 0b01010101

    void eeprom_keyOffCheckForExpiredFirmware(void);
    
    uint16_t eeprom_hoursSinceLastFirmwareUpdate_get(void);
    void     eeprom_hoursSinceLastFirmwareUpdate_set(uint16_t);

    uint8_t eeprom_expirationStatus_get(void);

    uint8_t eeprom_hasLibcmDisabledAssist_get(void);
    void    eeprom_hasLibcmDisabledAssist_set(uint8_t);

    uint8_t eeprom_hasLibcmDisabledRegen_get(void);
    void    eeprom_hasLibcmDisabledRegen_set(uint8_t);

    uint8_t eeprom_delayKeyON_ms_get(void);
    void    eeprom_delayKeyON_ms_set(uint8_t);

    void eeprom_resetDebugValues(void);

    void eeprom_verifyDataValid(void);

    void eeprom_batteryHistory_reset(void);

    void eeprom_resetAll(void);
    void eeprom_resetAll_userConfirm(void);

    void eeprom_batteryHistory_incrementValue(uint8_t indexTemperature, uint8_t indexSoC);

    uint16_t eeprom_batteryHistory_getValue(uint8_t indexTemperature, uint8_t indexSoC);

    void eeprom_begin(void);

    void writeToEEPROM_uint16(uint16_t startAddress, uint16_t value);

#endif
