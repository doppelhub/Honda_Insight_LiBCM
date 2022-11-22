//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef debugUSB_h
	#define debugUSB_h
	
	#define TWO_DECIMAL_PLACES 2
	#define FOUR_DECIMAL_PLACES 4

	void debugUSB_printLatestData(void);
	void debugUSB_printLatest_data_gridCharger(void);

	void debugUSB_printOneICsCellVoltages(uint8_t icToPrint, uint8_t decimalPlaces);

	void debugUSB_setCellBalanceStatus(uint8_t icNumber, uint16_t cellBitmap, uint16_t cellDischargeVoltageThreshold);

	void debugUSB_displayUptime_seconds(void);

	void debugUSB_printCellBalanceStatus(void);

	void debugUSB_printHardwareRevision(void);

	void debugUSB_dataTypeToStream_set(uint8_t dataType);
	uint8_t debugUSB_dataTypeToStream_get(void);

	void debugUSB_dataUpdatePeriod_ms_set(uint16_t newPeriod);
	uint16_t debugUSB_dataUpdatePeriod_ms_get(void);

	void debugUSB_printConfigParameters(void);

	#define DEBUGUSB_STREAM_POWER      0x11
	#define DEBUGUSB_STREAM_BATTMETSCI 0x22
	#define DEBUGUSB_STREAM_CELL       0x33
	#define DEBUGUSB_STREAM_TEMP       0x44
	#define DEBUGUSB_STREAM_DEBUG      0x55
	#define DEBUGUSB_STREAM_NONE       0x66

	#define TRANSMITTING_LARGE_MESSAGE     0x66
	#define NOT_TRANSMITTING_LARGE_MESSAGE 0x55
	
#endif