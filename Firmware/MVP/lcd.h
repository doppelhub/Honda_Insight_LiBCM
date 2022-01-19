//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef lcd_h
	#define lcd_h

	#define SCREEN_DIDNT_UPDATE false
	#define SCREEN_UPDATED      true

	//define round-robin states //one screen element is updated at a time 
	#define LCDUPDATE_NO_UPDATE     0
	#define LCDUPDATE_LOOPCOUNT     1
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


	#define LCDUPDATE_MAX_VALUE    13 //must be equal to the highest defined number (above) 

	void lcd_initialize(void);

	void lcd_refresh(void); //primary interface //each call updates one screen element

	void lcd_displayON(void);

	void lcd_displayOFF(void);

	void lcd_printStaticText(void);

	void lcd_gridChargerWarning(void);

	void lcd_firmwareUpdateWarning(void);
#endif