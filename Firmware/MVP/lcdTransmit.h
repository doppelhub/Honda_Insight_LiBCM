//Copyright 2021-2023(c) John Sullivan
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
	#define MAX_LCDUPDATE_ATTEMPTS LCDUPDATE_MAX_VALUE

	//the following static text never changes, and is only sent once after the display turns on
	#define LCDSTATIC_NO_UPDATE     15 //this state reduces CPU time when the key first turns on
	#define LCDSTATIC_SECONDS       16
	#define LCDSTATIC_VPACK_ACTUAL  17
	#define LCDSTATIC_VPACK_SPOOFED 18
	#define LCDSTATIC_NUMERRORS     19
	#define LCDSTATIC_CELL_HI       20
	#define LCDSTATIC_CELL_LO       21
	#define LCDSTATIC_CELL_DELTA    22
	#define LCDSTATIC_POWER         23
	#define LCDSTATIC_CELL_MAXEVER  24
	#define LCDSTATIC_CELL_MINEVER  25
	#define LCDSTATIC_SoC           26
	#define LCDSTATIC_CURRENT       27
	#define LCDSTATIC_TEMP_BATTERY  28
	#define LCDSTATIC_GRID_STATUS   29

	#define LCDSTATIC_MAX_VALUE     28 //must be equal to the highest static number (above)

	#define SCREEN_UPDATE_RATE_MILLIS 32 //one element is updated each time
		// Number of screen element updates per second = (1.0 / SCREEN_UPDATE_RATE_MILLIS)
		// Since only one screen element updates at a time, the per-element update rate is:
		//     ( (1.0 / SCREEN_UPDATE_RATE_MILLIS) / LCDUPDATE_MAX_VALUE)
		//  Ex:( (1.0 / 32E-3                    ) / 14                 ) = worst case (all elements have changed), each screen element updates 2.2x/second

	void lcd_begin(void);
	void lcd_end(void);

	void lcdTransmit_printNextElement_keyOn(void); //primary interface //each call updates one screen element

	void lcd_turnDisplayOnNow(void);
	void lcd_turnDisplayOffNow(void);

	void lcd_splashscreen_keyOff(void);

	void lcd_printStaticText(void);

	#define LCD_WARN_KEYON_GRID 1
	#define LCD_WARN_FW_EXPIRED 2
	#define LCD_WARN_COVER_GONE 3
	#define LCD_WARN_CELL_COUNT 4
	void lcd_displayWarning(uint8_t warningToDisplay);

#endif