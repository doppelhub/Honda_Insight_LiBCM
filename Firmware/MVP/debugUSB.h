//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef debugUSB_h
	#define debugUSB_h
	
	#define TWO_DECIMAL_PLACES 2
	#define FOUR_DECIMAL_PLACES 4

	void debugUSB_printLatest_data_keyOn(void);
	void debugUSB_printLatest_data_gridCharger(void);

	uint8_t debugUSB_getUserInput(void);

	void debugUSB_sendChar(char characterToSend);

	void debugUSB_setCellBalanceStatus(uint8_t icNumber, uint16_t cellBitmap);
#endif