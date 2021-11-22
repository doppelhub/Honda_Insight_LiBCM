//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//handles all communication with 4x20 lcd display

//The 4x20 LCD I2C bus is super slow... therefore, only one screen variable is updated per superloop iteration.
//This is handled by token 'lcd_whichFunctionUpdates', which is used for a round-robin scheduler
//Avoid calling 'lcd2.' at all costs; it's ALWAYS faster to do math to see if you need to send data to screen.

#include "libcm.h"

#include "lcd_I2C.h" //ensure these funcitons are only called within this file

lcd_I2C_jts lcd2(0x27);

//These variables are reset during key change
uint8_t  packVoltageActual_onScreen = 0;
uint8_t  packVoltageSpoofed_onScreen = 0;
uint8_t  errorCount_onScreen = 0;
uint16_t maxEverCellVoltage_onScreen = 0;
uint16_t minEverCellVoltage_onScreen = 0;
uint8_t  SoC_onScreen = 0;
int8_t   temp_onScreen = 0;

////////////////////////////////////////////////////////////////////////

void lcd_initialize(void)
{
	#ifdef LCD_4X20_CONNECTED
		lcd2.begin(20,4);
	#endif
}

////////////////////////////////////////////////////////////////////////

//time since last keyON
bool lcd_printTime_seconds(void)
{
	bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

	#ifdef LCD_4X20_CONNECTED
		static uint16_t seconds_onScreen = 0;
		uint32_t keyOnTime_ms = millis() - key_latestTurnOnTime_ms_get();
		uint16_t keyOnTime_seconds = (uint16_t)(keyOnTime_ms * 0.001); //rolls over after keyON for 18 hours

		if(seconds_onScreen != keyOnTime_seconds)
		{
			seconds_onScreen = keyOnTime_seconds;

			lcd2.setCursor(1,3);

			if     (keyOnTime_seconds < 10   ) { lcd2.print(F("    ")); } //one   digit  //   0:   9
			else if(keyOnTime_seconds < 100  ) { lcd2.print(F("   ") ); } //two   digits //  10:  99
			else if(keyOnTime_seconds < 1000 ) { lcd2.print(F("  ")  ); } //three digits // 100: 999
			else if(keyOnTime_seconds < 10000) { lcd2.print(F(" ")   ); } //four  digits //1000:9999
                                                                    //five  digits doesn't require leading spaces //10000:65535
			lcd2.print(String(keyOnTime_seconds));

			didscreenUpdateOccur = SCREEN_UPDATED;
		}
	#endif

	return didscreenUpdateOccur;
}

////////////////////////////////////////////////////////////////////////

bool lcd_printSoC(void)
{
	bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

	#ifdef LCD_4X20_CONNECTED
		if( SoC_onScreen != SoC_getBatteryStateNow_percent() )
		{
			SoC_onScreen = SoC_getBatteryStateNow_percent();
			lcd2.setCursor(10,3); //SoC screen position
			if(SoC_onScreen < 10) { lcd2.print( '0');         } //add leading '0' to single digit number
			if(SoC_onScreen > 99) { lcd2.print("99");         } //can't display '100' (only QTY2 digits)
			else                  { lcd2.print(SoC_onScreen); } //print actual value

			didscreenUpdateOccur = SCREEN_UPDATED;
		}
	#endif

	return didscreenUpdateOccur;
}

////////////////////////////////////////////////////////////////////////

bool lcd_printStackVoltage_actual(void)
{
	bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

	#ifdef LCD_4X20_CONNECTED
		if( packVoltageActual_onScreen != LTC68042result_packVoltage_get() )
		{
			packVoltageActual_onScreen = LTC68042result_packVoltage_get();
			lcd2.setCursor(2,2); //actual pack voltage position
			lcd2.print(packVoltageActual_onScreen);

			didscreenUpdateOccur = SCREEN_UPDATED;
		}
	#endif

	return didscreenUpdateOccur;
}

////////////////////////////////////////////////////////////////////////

bool lcd_printStackVoltage_spoofed(void)
{
	bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

	#ifdef LCD_4X20_CONNECTED
		if( packVoltageSpoofed_onScreen != vPackSpoof_getSpoofedPackVoltage() )
		{
			packVoltageSpoofed_onScreen = vPackSpoof_getSpoofedPackVoltage();
			lcd2.setCursor(6,2); //spoofed pack voltage position
			lcd2.print(packVoltageSpoofed_onScreen);

			didscreenUpdateOccur = SCREEN_UPDATED;
		}
	#endif

	return didscreenUpdateOccur;
}

////////////////////////////////////////////////////////////////////////

bool lcd_printTempBattery(void)
{
	bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

	#ifdef LCD_4X20_CONNECTED
		int8_t batteryTempNow = temperature_battery_getLatest(); //get single ADC measurement

		if( temp_onScreen != batteryTempNow )
		{
			temp_onScreen = batteryTempNow;
			lcd2.setCursor(12,2);
			if((batteryTempNow >= 0) && (batteryTempNow < 10) ) { lcd2.print(' '); } //leading space on " 0" to " 9" degC
			if(batteryTempNow < -9 ) { lcd2.print(-9); } //JTS2doNow: Add additional digit for temperatures "-10" and lower
			else                     { lcd2.print(batteryTempNow); }

			didscreenUpdateOccur = SCREEN_UPDATED;
		}
	#endif

	return didscreenUpdateOccur;
}

////////////////////////////////////////////////////////////////////////

bool lcd_printNumErrors(void)
{
	bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

	#ifdef LCD_4X20_CONNECTED
		if( errorCount_onScreen != LTC68042result_errorCount_get() )
		{
			errorCount_onScreen = LTC68042result_errorCount_get();
			lcd2.setCursor(2,3);
			lcd2.print(errorCount_onScreen);

			didscreenUpdateOccur = SCREEN_UPDATED;
		}
	#endif

	return didscreenUpdateOccur;
}

////////////////////////////////////////////////////////////////////////

bool lcd_printCellVoltage_hi(void)
{
	bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

	#ifdef LCD_4X20_CONNECTED
		static uint16_t hiCellVoltage_onScreen = 0;
		if( hiCellVoltage_onScreen != LTC68042result_hiCellVoltage_get() )
		{
			hiCellVoltage_onScreen = LTC68042result_hiCellVoltage_get();
			lcd2.setCursor(1,0); //high cell voltage position
			lcd2.print( (hiCellVoltage_onScreen * 0.0001), 3 );

			didscreenUpdateOccur = SCREEN_UPDATED;
		}

		static bool isBacklightOn = true;

		if( (LTC68042result_hiCellVoltage_get() > 41500) || (isBacklightOn == false) )
		{ //at least one cell overcharged

			if ( isBacklightOn == true ) {
				lcd2.noBacklight();
				gpio_turnBuzzer_on_lowFreq();
				isBacklightOn = false;
			} else {
				lcd2.backlight();
				gpio_turnBuzzer_off();
				isBacklightOn = true;
			}
		}
	#endif

	return didscreenUpdateOccur;
}

////////////////////////////////////////////////////////////////////////

bool lcd_printCellVoltage_lo(void)
{
	bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

	#ifdef LCD_4X20_CONNECTED
		static uint16_t loCellVoltage_onScreen = 0;
		if( loCellVoltage_onScreen != LTC68042result_loCellVoltage_get() )
		{
			loCellVoltage_onScreen = LTC68042result_loCellVoltage_get();
			lcd2.setCursor(1,1); //low screen position
			lcd2.print( (loCellVoltage_onScreen * 0.0001), 3 );

			didscreenUpdateOccur = SCREEN_UPDATED;
		}

		static bool isBacklightOn = true;

		if( (LTC68042result_loCellVoltage_get() < 31500) || (isBacklightOn == false) )
		{ //at least one cell undercharged
			if ( isBacklightOn == true ) {
				lcd2.noBacklight();
				isBacklightOn = false;
			} else {
				lcd2.backlight();
				isBacklightOn = true;
			}
		}
	#endif

	return didscreenUpdateOccur;
}

////////////////////////////////////////////////////////////////////////

bool lcd_printCellVoltage_delta(void)
{
	bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

	#ifdef LCD_4X20_CONNECTED
		static uint16_t deltaVoltage_onScreen = 0;

		uint16_t deltaVoltage_LTC6804 = LTC68042result_hiCellVoltage_get() - LTC68042result_loCellVoltage_get();

		if( deltaVoltage_onScreen != deltaVoltage_LTC6804 )
		{
			deltaVoltage_onScreen = deltaVoltage_LTC6804;
			lcd2.setCursor(15,0); //delta cell voltage position
			lcd2.print( (deltaVoltage_onScreen * 0.0001), 3 );

			didscreenUpdateOccur = SCREEN_UPDATED;
		}
	#endif

	return didscreenUpdateOccur;
}

////////////////////////////////////////////////////////////////////////

//JTS2doNow: Add pack current display function
bool lcd_printCurrent(void)
{
	bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

	#ifdef LCD_4X20_CONNECTED
		static int16_t packAmps_onScreen = 0; //don't want to multiply to determine power

		if( packAmps_onScreen != adc_getLatestBatteryCurrent_amps() )
		{
			packAmps_onScreen = adc_getLatestBatteryCurrent_amps();
			lcd2.setCursor(15,1);
			if(adc_getLatestBatteryCurrent_amps() >= 0 )
			{
				if      (adc_getLatestBatteryCurrent_amps() <  10 ) { lcd2.print("  "); }
				else if (adc_getLatestBatteryCurrent_amps() < 100 ) { lcd2.print(" ");  }
				lcd2.print('+');
			}
			else //negative current
			{
				if      (adc_getLatestBatteryCurrent_amps() >  -10 ) { lcd2.print("  "); }
				else if (adc_getLatestBatteryCurrent_amps() > -100 ) { lcd2.print(" ");  }
			}
			lcd2.print(adc_getLatestBatteryCurrent_amps());

			didscreenUpdateOccur = SCREEN_UPDATED;
		}
	#endif

	return didscreenUpdateOccur;
}

////////////////////////////////////////////////////////////////////////

bool lcd_printMaxEverVoltage()
{
	bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

	#ifdef LCD_4X20_CONNECTED
		if( maxEverCellVoltage_onScreen != LTC68042result_maxEverCellVoltage_get() )
		{
			maxEverCellVoltage_onScreen = LTC68042result_maxEverCellVoltage_get();
			lcd2.setCursor(7,0); //maxEver screen position
			lcd2.print( (maxEverCellVoltage_onScreen * 0.0001) , 3);

			didscreenUpdateOccur = SCREEN_UPDATED;
		}
	#endif

	return didscreenUpdateOccur;
}

////////////////////////////////////////////////////////////////////////

bool lcd_printMinEverVoltage()
{
	bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

	#ifdef LCD_4X20_CONNECTED
		if( minEverCellVoltage_onScreen != LTC68042result_minEverCellVoltage_get() )
		{
			minEverCellVoltage_onScreen = LTC68042result_minEverCellVoltage_get();
			lcd2.setCursor(7,1); //minEver screen position
			lcd2.print( (minEverCellVoltage_onScreen * 0.0001) , 3);

			didscreenUpdateOccur = SCREEN_UPDATED;
		}
	#endif

	return didscreenUpdateOccur;
}

////////////////////////////////////////////////////////////////////////

bool lcd_printPower(void)
{
	bool didscreenUpdateOccur = SCREEN_DIDNT_UPDATE;

	#ifdef LCD_4X20_CONNECTED
		static int16_t packAmps_onScreen = 0; //don't want to multiply to determine power
		static uint8_t packVoltage_onScreen = 0;

		if( packAmps_onScreen != adc_getLatestBatteryCurrent_amps() ||
			  packVoltage_onScreen != LTC68042result_packVoltage_get() )
		{
			packAmps_onScreen = adc_getLatestBatteryCurrent_amps();
			packVoltage_onScreen = LTC68042result_packVoltage_get();

			int16_t packWatts = LTC68042result_packVoltage_get() * adc_getLatestBatteryCurrent_amps();

			lcd2.setCursor(15,3);

			if(packWatts >=0)
			{
				if(packWatts < 10000) { lcd2.print(' '); } //" +0.0" to " +9.9" kW
				lcd2.print("+");
			}
			else //negative watts
			{
				if(packWatts > -10000) { lcd2.print(' '); } //" -0.1" to " -9.9" kW
			}
			lcd2.print( (packWatts * 0.001), 1 );

			didscreenUpdateOccur = SCREEN_UPDATED;
		}
	#endif

	return didscreenUpdateOccur;
}

////////////////////////////////////////////////////////////////////////

//calling directly always updates screen immediately
bool lcd_updateValue(uint8_t stateToUpdate)
{
	bool didScreenUpdateOccur = SCREEN_DIDNT_UPDATE;
	switch(stateToUpdate)
	{
		case LCDUPDATE_LOOPCOUNT    : didScreenUpdateOccur = lcd_printTime_seconds();         break;
		case LCDUPDATE_VPACK_ACTUAL : didScreenUpdateOccur = lcd_printStackVoltage_actual();  break;
		case LCDUPDATE_VPACK_SPOOFED: didScreenUpdateOccur = lcd_printStackVoltage_spoofed(); break;
		case LCDUPDATE_NUMERRORS    : didScreenUpdateOccur = lcd_printNumErrors();            break;
		case LCDUPDATE_CELL_HI      : didScreenUpdateOccur = lcd_printCellVoltage_hi();       break;
		case LCDUPDATE_CELL_LO      : didScreenUpdateOccur = lcd_printCellVoltage_lo();       break;
		case LCDUPDATE_CELL_DELTA   : didScreenUpdateOccur = lcd_printCellVoltage_delta();    break;
		case LCDUPDATE_POWER        : didScreenUpdateOccur = lcd_printPower();                break;
		case LCDUPDATE_CELL_MAXEVER : didScreenUpdateOccur = lcd_printMaxEverVoltage();       break;
		case LCDUPDATE_CELL_MINEVER : didScreenUpdateOccur = lcd_printMinEverVoltage();       break;
		case LCDUPDATE_SoC          : didScreenUpdateOccur = lcd_printSoC();                  break;
		case LCDUPDATE_CURRENT      : didScreenUpdateOccur = lcd_printCurrent();              break;
		case LCDUPDATE_TEMP_BATTERY : didScreenUpdateOccur = lcd_printTempBattery();          break;
		default                     : didScreenUpdateOccur = SCREEN_UPDATED;                  break; //if illigal input, exit immediately
	}

	return didScreenUpdateOccur;
}

////////////////////////////////////////////////////////////////////////

//primary interface
//update one screen element (if any have changed)
void lcd_refresh(void)
{
	#ifdef LCD_4X20_CONNECTED

		static uint8_t lcdUpdate_state = LCDUPDATE_NUMERRORS; //init round-robin with least likely state to have changed
		static uint8_t lastElementUpdated = LCDUPDATE_NUMERRORS; //last LCD screen element updated //cannot = LCDUPDATE_NO_UPDATE
		static uint32_t millis_previous = 0;

		#define SCREEN_UPDATE_RATE_MILLIS 32
		// Number of screen element updates per second = (1.0 / SCREEN_UPDATE_RATE_MILLIS)
		// Since only one screen element updates at a time, the per-element update rate is:
		//     ( (1.0 / SCREEN_UPDATE_RATE_MILLIS) / LCDUPDATE_MAX_VALUE)
		//  Ex:( (1.0 / 32E-3                    ) / 8                  ) = each screen element updates 3.9x/second

		//Only update screen at a human-readable rate
		if(millis() - millis_previous > SCREEN_UPDATE_RATE_MILLIS)
		{ //update which screen element is allowed to update (if changed via another lcd_ function)
			millis_previous = millis();

			//always true unless Superloop hangs longer than SCREEN_UPDATE_RATE_MILLIS
			if(lcdUpdate_state == LCDUPDATE_NO_UPDATE) { lcdUpdate_state = lastElementUpdated; } //restore last updated screen element

			#define MAX_LCDUPDATE_ATTEMPTS    LCDUPDATE_MAX_VALUE
			uint8_t updateAttempts = 0;
			do
			{	//repeats until ONE screen element update occurs
				lcdUpdate_state++; //select which LCD variable is next in line to update

				if(lcdUpdate_state > LCDUPDATE_MAX_VALUE) {lcdUpdate_state = 1;} //reset to first element
				updateAttempts++;
			} while( (lcd_updateValue(lcdUpdate_state) == SCREEN_DIDNT_UPDATE) && (updateAttempts < MAX_LCDUPDATE_ATTEMPTS) );

			lastElementUpdated = lcdUpdate_state; //store last updated screen element
		}

		else
		{
			lcdUpdate_state = LCDUPDATE_NO_UPDATE; //disable screen updates until SCREEN_UPDATE_RATE_MILLIS time has passed
		}

	#endif
}

////////////////////////////////////////////////////////////////////////

//old format
// 		//                                          1111111111
// 		//                                01234567890123456789
// 		//4x20 screen text display format:********************
// 		lcd2.setCursor(0,0);  lcd2.print("hi:h.hhh (max:H.HHH)"); //row0, (3,0)=h.hhh, (14,0)=H.HHH
// 		lcd2.setCursor(0,1);  lcd2.print("lo:l.lll (min:L.LLL)"); //row1, (3,1)=l.lll, (14,1)=L.LLL
// 		lcd2.setCursor(0,2);  lcd2.print("d:d.ddd, V:VVV (SSS)"); //row2, (2,2)=d.ddd, (11,2)=VVV  , (16,2)=SSS
// 		lcd2.setCursor(0,3);  lcd2.print("E:0 /CCCCC, kW:+WW.0"); //row3, (2,3)=    0, (5,3)=CCCCC , (16,3)=WW.0

//only call during keyOFF (screen updates are slow)
void lcd_printStaticText(void)
{
	#ifdef LCD_4X20_CONNECTED
		lcd2.setCursor(0,0);
		//                                          1111111111
		//                                01234567890123456789
		//4x20 screen text display format:********************
		lcd2.setCursor(0,0);  lcd2.print("Hx.xxx(y.yyy) dz.zzz"); //row0: x.xxx=(1,0)   y.yyy=(7,0) z.zzz=(15,0)
		                                                          //      x.xxx:cellHI  y.yyy:Vmax  z.zzz:deltaV
		lcd2.setCursor(0,1);  lcd2.print("La.aaa(b.bbb) A-ccc "); //row1: a.aaa=(1,1)   b.bbb=(7,1) ccc=(15,1)
	                                                            //      a.aaa:cellLO  b.bbb:Vmin  ccc:current
		lcd2.setCursor(0,2);  lcd2.print("Vprrr(fff) TggC     "); //row2: rrr=(2,2)     fff=(6,2)   gg=(12,2)
	                                                            //      rrr:Vpack     fff:Vspoof  gg:hiTemp   hh:loTemp
		lcd2.setCursor(0,3);  lcd2.print("Tuuuuu SoCss kW-kk.k"); //row3: uuuuu=(1,3)   ss=(10,3)   kk.k=(15,3)
	                                                            //      uuuuu:T_keyOn ss:SoC(%)   kk.k:power
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_displayOFF(void)
{
	#ifdef LCD_4X20_CONNECTED
		lcd2.clear();
		lcd2.setCursor(0,0);
		lcd2.print("LiBCM v" + String(FW_VERSION) );
		delay(1000); //allow time for operator to read firmware version
		//JTS2doLater: refresh screen during keyON (will need to send one instruction per loop)

		Wire.end();
		delay(50);
		lcd_initialize();
		delay(50);
		lcd_printStaticText();

		lcd2.noBacklight();
		lcd2.noDisplay();


		packVoltageActual_onScreen  = 0;
		errorCount_onScreen         = 0;
		SoC_onScreen                = 0;
		packVoltageSpoofed_onScreen = 0;
		temp_onScreen               = 0;
		LTC68042result_errorCount_set(0);
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_displayON(void)
{
	#ifdef LCD_4X20_CONNECTED
		lcd2.backlight();
		lcd2.display();
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_gridChargerWarning(void)
{
	gpio_turnBuzzer_on_highFreq();
	lcd2.backlight();
	lcd2.display();
	lcd2.clear();
	//                               ********************
	lcd2.setCursor(0,0); lcd2.print("ALERT: Grid Charger ");
	lcd2.setCursor(0,1); lcd2.print("       Plugged In!! ");
	gpio_turnBuzzer_on_lowFreq();
	lcd2.setCursor(0,2); lcd2.print("LiBCM sent P1648 to ");
	lcd2.setCursor(0,3); lcd2.print("prevent IMA start.  ");
	gpio_turnBuzzer_off();
}
