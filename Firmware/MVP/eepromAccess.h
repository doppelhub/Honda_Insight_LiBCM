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

#endif
