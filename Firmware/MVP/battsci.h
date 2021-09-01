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

	uint8_t BATTSCI_bytesAvailableForWrite();

	uint8_t BATTSCI_writeByte(uint8_t data);
	
	void BATTSCI_sendFrames();

	uint8_t BATTSCI_calculateChecksum( uint8_t frameSum );

	void BATTSCI_setPackVoltage(uint8_t voltage);

	void BATTSCI_setSpoofedCurrent(int16_t packCurrent);

#endif