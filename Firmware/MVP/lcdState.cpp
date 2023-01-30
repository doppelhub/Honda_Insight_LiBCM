//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//state machine that controls 4x20 lcd display.
//Actual text/control transmit functions are in lcdTransmit.c

#include "libcm.h"

////////////////////////////////////////////////////////////////////////

uint8_t requestDisplayOn(uint8_t state)
{
	static uint32_t timestamp_helper_ms = 0;

	Serial.print('1');

	if(state == LCDSTATE_TURNON_NOW)
	{
		Serial.print('A');
		lcd_begin();
		timestamp_helper_ms = millis();
		return LCDSTATE_TURNING_ON;
	}
	else if(state == LCDSTATE_TURNING_ON)
	{
		if( (millis() - timestamp_helper_ms) < LCD_TURNON_DELAY_ms) { Serial.print('B'); return LCDSTATE_TURNING_ON; } //repeat this state until delay finishes
		else
		{
			Serial.print('C'); 

			lcd_turnDisplayOnNow();

			return LCDSTATE_ON;
		}
	}
	else { return LCDSTATE_TURNON_NOW; }

}

////////////////////////////////////////////////////////////////////////

uint8_t requestDisplayOff(uint8_t state)
{
	static uint32_t timestamp_helper_ms = 0;

	Serial.print('0');

	if(state == LCDSTATE_PREOFF_SPLASHSCREEN)
	{
		Serial.print('D');
		lcd_resetVariablesToDefault();

		lcd_splashscreen_keyOff();

		timestamp_helper_ms = millis();
		return LCDSTATE_PREOFF_DELAY_READTEXT;
	}
	
	else if(state == LCDSTATE_PREOFF_DELAY_READTEXT)
	{
		if( (millis() - timestamp_helper_ms) < LCD_PREOFF_DELAY_TOREADSCREEN_ms ) { Serial.print('E'); return LCDSTATE_PREOFF_DELAY_READTEXT; } //repeat this state until delay finishes
		else
		{
			Serial.print('F');
			//timer expired
			lcd_end(); //close previous session to recover from (possible) data transmission corruption during previous session
			timestamp_helper_ms = millis(); //variable reused for next timer

			return LCDSTATE_PREOFF_WAIT_FOR_SESSION_TO_END;
		}
	}

	else if(state == LCDSTATE_PREOFF_WAIT_FOR_SESSION_TO_END)
	{
		if( (millis() - timestamp_helper_ms) < LCD_PREOFF_DELAY_TOENDSESSION_ms ) { Serial.print('G'); return LCDSTATE_PREOFF_WAIT_FOR_SESSION_TO_END; } //repeat this state until delay finishes
		else
		{
			Serial.print('H');
			//timer expired
			lcd_begin(); //reconnect to screen so we can turn the backlight off
			
			//might need another delay state here

			lcd_turnDisplayOffNow();

			//does the backlight and display stay off if we Wire.end() here?

			return LCDSTATE_OFF;
		}
	}

	else { Serial.print('J'); return LCDSTATE_PREOFF_SPLASHSCREEN; } //if statePrevious wasn't one of the above, start the turnoff procedure
}

////////////////////////////////////////////////////////////////////////

void lcdState_handler(void)
{
	#ifdef LCD_4X20_CONNECTED
		static uint8_t lcdStatePrevious = LCDSTATE_LIBCM_JUST_TURNED_ON;

		if(key_getSampledState() == KEYSTATE_ON)
		{
			if(lcdStatePrevious == LCDSTATE_ON) { lcdTransmit_refreshKeyOn();                            }
			else                                { lcdStatePrevious = requestDisplayOn(lcdStatePrevious); } //display is in the process of turning on
		}

		else if(gpio_isGridChargerPluggedInNow() == YES)
		{
			if(lcdStatePrevious == LCDSTATE_ON) { lcdTransmit_refreshKeyOn();                            } //JTS2doNow: Make separate grid charging handler
			else                                { lcdStatePrevious = requestDisplayOn(lcdStatePrevious); } //display is in the process of turning on
		}

		else //keyOFF and charger unplugged
		{
			if(lcdStatePrevious == LCDSTATE_OFF) { ;                                                       }
			else                                 { lcdStatePrevious = requestDisplayOff(lcdStatePrevious); } //display is in the process of turning off
		}
	#endif
}
