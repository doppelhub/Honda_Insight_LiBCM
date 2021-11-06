//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef battsci_h
	#define battsci_h

	#define BATTSCI_BYTES_IN_FRAME 12
	#define RUNNING 1
	#define STOPPED 0

	void BATTSCI_begin();

	void BATTSCI_enable();

	void BATTSCI_disable();
	
	void BATTSCI_sendFrames();

	void BATTSCI_setPackVoltage(uint8_t voltage);

	void BATTSCI_setSpoofedCurrent(int16_t packCurrent);

	uint8_t BATTSCI_writeByte(uint8_t data);

#endif