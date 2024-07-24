//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//state machine that controls 4x20 lcd display.
//Actual text/control transmit functions are in lcdTransmit.c

#include "libcm.h"

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t requestDisplayOn(uint8_t state)
{
    static uint32_t timestamp_helper_ms = 0;

    if (state == LCDSTATE_TURNON_NOW)
    {
        lcdTransmit_begin();
        timestamp_helper_ms = millis();
        return LCDSTATE_TURNING_ON;
    }
    else if (state == LCDSTATE_TURNING_ON)
    {
        if ((millis() - timestamp_helper_ms) < LCD_TURNON_DELAY_ms) {                          return LCDSTATE_TURNING_ON; } //repeat this state until delay finishes
        else                                                        { lcdTransmit_displayOn(); return LCDSTATE_ON;         }
    }
    else { return LCDSTATE_TURNON_NOW; }
}

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t requestDisplayOff(uint8_t state)
{
    static uint32_t timestamp_helper_ms = 0;

    if (state == LCDSTATE_PREOFF_SPLASHSCREEN)
    {
        lcdTransmit_splashscreenKeyOff();

        timestamp_helper_ms = millis();
        return LCDSTATE_PREOFF_DELAY_READTEXT;
    }
    
    //JTS2doLater: keep display on longer after keyOff
    else if (state == LCDSTATE_PREOFF_DELAY_READTEXT)
    {
        if ((millis() - timestamp_helper_ms) < LCD_PREOFF_DELAY_TOREADSCREEN_ms) { return LCDSTATE_PREOFF_DELAY_READTEXT; } //repeat this state until delay finishes
        else
        {
            //timer expired
            lcdTransmit_end(); //close previous session to recover from (possible) data transmission corruption during previous session
            timestamp_helper_ms = millis(); //variable reused for next timer

            return LCDSTATE_PREOFF_WAIT_FOR_SESSION_TO_END;
        }
    }

    else if (state == LCDSTATE_PREOFF_WAIT_FOR_SESSION_TO_END)
    {
        if ((millis() - timestamp_helper_ms) < LCD_PREOFF_DELAY_TOENDSESSION_ms) { return LCDSTATE_PREOFF_WAIT_FOR_SESSION_TO_END; } //repeat this state until delay finishes
        else
        {
            //timer expired
            lcdTransmit_begin(); //reconnect to screen so we can turn the backlight off
            
            //might need another delay state here

            lcdTransmit_displayOff();

            //does the backlight and display stay off if we Wire.end() here?

            return LCDSTATE_OFF;
        }
    }

    else { return LCDSTATE_PREOFF_SPLASHSCREEN; } //if statePrevious wasn't one of the above, start the turnoff procedure
}

/////////////////////////////////////////////////////////////////////////////////////////

void lcdState_handler(void)
{
    #ifdef LCD_4X20_CONNECTED
        static uint8_t statePrevious = LCDSTATE_LIBCM_JUST_TURNED_ON;

        if ((key_getSampledState() == KEYSTATE_OFF) && (gpio_isGridChargerPluggedInNow() == NO))
        {
            //nobody is using display, so turn it off
            if (statePrevious != LCDSTATE_OFF) { statePrevious = requestDisplayOff(statePrevious); } //LCD is turning off
        }

        //if we get here, key is on and/or grid charger is plugged in
        else if (statePrevious != LCDSTATE_ON) { statePrevious = requestDisplayOn(statePrevious); } //LCD is turning on

        //if we get here, 4x20 LCD is on and ready to display data
        else if (key_getSampledState() == KEYSTATE_ON)
        {
            if      (gpio_isGridChargerPluggedInNow() == YES)           { lcdTransmit_Warning(LCD_WARN_KEYON_GRID); } 
            else if (gpio_isCoverInstalled() == false)                  { lcdTransmit_Warning(LCD_WARN_COVER_GONE); }
            else if (eeprom_expirationStatus_get() == FIRMWARE_EXPIRED) { lcdTransmit_Warning(LCD_WARN_FW_EXPIRED); }
            else /* no warnings... update next screen element */        { lcdTransmit_printNextElement();           }
        }

        else if (gpio_isGridChargerPluggedInNow() == YES) { lcdTransmit_printNextElement(); }

        //should never get here...
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////
