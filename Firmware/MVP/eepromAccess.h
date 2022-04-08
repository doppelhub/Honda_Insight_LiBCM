//JTS2doLater: eeprom.h isn't wrapped into MVP yet

#ifndef eepromAccess_h
#define eepromAccess_h

	void EEPROM_checkForExpiredFirmware(void);
	
	uint16_t EEPROM_uptimeStoredInEEPROM_hours_get(void);
	
	uint8_t EEPROM_firmwareStatus_get(void);

	#define REQUIRED_FIRMWARE_UPDATE_PERIOD_DAYS 40
    #define REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS (REQUIRED_FIRMWARE_UPDATE_PERIOD_DAYS * 24)

    #define FIRMWARE_STATUS_EXPIRED 0b10101010 //alternating bit pattern for EEPROM read/write integrity
    #define FIRMWARE_STATUS_VALID   0b01010101

    uint8_t EEPROM_hasLibcmDisabledAssist_get(void);
    void EEPROM_hasLibcmDisabledAssist_set(uint8_t);

    uint8_t EEPROM_hasLibcmDisabledRegen_get(void);
    void EEPROM_hasLibcmDisabledRegen_set(uint8_t);

    uint8_t EEPROM_delayKeyON_ms_get(void);
	void    EEPROM_delayKeyON_ms_set(uint8_t);

	uint8_t EEPROM_hasLibcmFailedTiming_get(void);
	void    EEPROM_hasLibcmFailedTiming_set(uint8_t);

    #define EEPROM_LICBM_DISABLED_REGEN  0xAA
    #define EEPROM_REGEN_NEVER_LIMITED   0x55
    #define EEPROM_LICBM_DISABLED_ASSIST 0xCC
    #define EEPROM_ASSIST_NEVER_LIMITED  0x33

    #define EEPROM_LIBCM_LOOPRATE_EXCEEDED 0xC3
    #define EEPROM_LIBCM_LOOPRATE_MET      0x3C

    #define EEPROM_ADDRESS_FACTORY_VALUE 0xFF //default EEPROM value if specific EEPROM address has never been written to

    void EEPROM_resetDebugValues(void);

    void EEPROM_verifyDataValid(void);

#endif
