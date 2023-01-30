//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef lcdTransmit_h
	#define lcdTransmit_h

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

	#define SCREEN_UPDATE_RATE_MILLIS 32 //one element is updated each time
		// Number of screen element updates per second = (1.0 / SCREEN_UPDATE_RATE_MILLIS)
		// Since only one screen element updates at a time, the per-element update rate is:
		//     ( (1.0 / SCREEN_UPDATE_RATE_MILLIS) / LCDUPDATE_MAX_VALUE)
		//  Ex:( (1.0 / 32E-3                    ) / 14                 ) = worst case (all elements have changed), each screen element updates 2.2x/second

	//JTS2doNow: See which functions can be deleted:

	void lcd_begin(void);
	void lcd_end(void);

	void lcdTransmit_refreshKeyOn(void); //primary interface //each call updates one screen element

	void lcd_turnDisplayOnNow(void);
	void lcd_turnDisplayOffNow(void);

	void lcd_resetVariablesToDefault(void);

	void lcd_splashscreen_keyOff(void);

	void lcd_printStaticText(void);

	void lcd_Warning_gridCharger(void);

	void lcd_Warning_firmwareUpdate(void);

	void lcd_Warning_coverNotInstalled(void);
#endif