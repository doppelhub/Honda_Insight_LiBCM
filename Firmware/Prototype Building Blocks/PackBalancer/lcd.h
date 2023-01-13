//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef lcd_h
	#define lcd_h

	#define SCREEN_DIDNT_UPDATE false
	#define SCREEN_UPDATED      true

	//define screen elements //one screen element is updated at a time, using round robbin state machine 
	#define LCDUPDATE_NO_UPDATE     0
	#define LCDUPDATE_SECONDS       1
	#define LCDUPDATE_VPACK_ACTUAL  2
	#define LCDUPDATE_VPACK_SPOOFED 3
	#define LCDUPDATE_NUMERRORS     4
	#define LCDUPDATE_CELL_HI       5
	#define LCDUPDATE_CELL_LO       6
	#define LCDUPDATE_CELL_DELTA    7
	#define LCDUPDATE_POWER         8
	#define LCDUPDATE_CELL_MAXEVER  9
	#define LCDUPDATE_CELL_MINEVER 10
	#define LCDUPDATE_SoC          11
	#define LCDUPDATE_CURRENT      12
	#define LCDUPDATE_TEMP_BATTERY 13
	#define LCDUPDATE_GRID_STATUS  14

	#define LCDUPDATE_MAX_VALUE    14 //must be equal to the highest defined number (above) 

	void lcd_begin(void);

	void lcd_refresh(void); //primary interface //each call updates one screen element

	void lcd_displayOn(void);

	void lcd_displayOFF(void);

	void lcd_printStaticText(void);

	void lcd_Warning_gridCharger(void);

	void lcd_Warning_firmwareUpdate(void);

	void lcd_Warning_coverNotInstalled(void);
#endif