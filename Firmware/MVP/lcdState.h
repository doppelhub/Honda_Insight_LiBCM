
#ifndef lcdState_h
	#define lcdState_h

	#define LCDSTATE_LIBCM_JUST_TURNED_ON           0
	#define LCDSTATE_TURNON_NOW                     1
	#define LCDSTATE_TURNING_ON                     2
	#define LCDSTATE_ON                             3
	#define LCDSTATE_PREOFF_SPLASHSCREEN            4
	#define LCDSTATE_PREOFF_DELAY_READTEXT          5
	#define LCDSTATE_PREOFF_WAIT_FOR_SESSION_TO_END 6
	#define LCDSTATE_OFF                            7

	#define LCD_TURNON_DELAY_ms 50
	#define LCD_PREOFF_DELAY_TOREADSCREEN_ms 2000
	#define LCD_PREOFF_DELAY_TOENDSESSION_ms 50

	void lcdState_handler(void);


#endif