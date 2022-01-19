//JTS2doLater: eeprom.h isn't wrapped into MVP yet

#ifndef eepromAccess_h
#define eepromAccess_h

	void EEPROM_checkForExpiredFirmware(void);
	uint16_t EEPROM_calculateTotalHoursSinceLastFirmwareUpdate(void);

	#define REQUIRED_FIRMWARE_UPDATE_PERIOD_DAYS 40
    #define REQUIRED_FIRMWARE_UPDATE_PERIOD_HOURS (REQUIRED_FIRMWARE_UPDATE_PERIOD_DAYS * 24)

#endif
