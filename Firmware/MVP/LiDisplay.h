//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef lidisplay_h
	#define lidisplay_h

	void LiDisplay_begin(void);

	void LiDisplay_refresh(void);

	void LiDisplay_keyOn(void);

	void LiDisplay_keyOff(void);

	void LiDisplay_gridChargerUnplugged(void);

	void LiDisplay_gridChargerPluggedIn(void);

	void LiDisplay_setPageNumber(uint8_t page);

	uint8_t LiDisplay_bytesAvailableForWrite(void);

	uint8_t LiDisplay_writeByte(uint8_t data);

	uint8_t LiDisplay_readByte(void);

	uint8_t LiDisplay_bytesAvailableToRead();

#endif
