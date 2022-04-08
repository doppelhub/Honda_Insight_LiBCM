//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef lidisplay_h
	#define lidisplay_h

	void LiDisplay_begin(void);

	uint8_t LiDisplay_bytesAvailableForWrite(void);

	uint8_t LiDisplay_writeByte(uint8_t data);

	uint8_t LiDisplay_readByte(void);

	uint8_t LiDisplay_bytesAvailableToRead();

#endif