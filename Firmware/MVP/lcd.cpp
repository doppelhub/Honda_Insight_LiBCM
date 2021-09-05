//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//handles all communication with 4x20 lcd display

//The 4x20 LCD I2C bus is super slow... therefore, only one screen variable is updated per superloop iteration.
//This is handled by token 'lcd_whichFunctionUpdates', which is used for a round-robin scheduler
//Avoid calling 'lcd2.' at all costs; it's ALWAYS faster to do math to see if you need to send data to screen.

#include "libcm.h"

#ifdef I2C_LIQUID_CRYSTAL
  #include "LiquidCrystal_I2C.h"
  LiquidCrystal_I2C lcd2(0x27, 20, 4);
#endif

#ifdef I2C_LCD
  #include "TwiLiquidCrystal.h"
  TwiLiquidCrystal lcd2(0x27);
#endif

#ifdef LCD_JTS
  #include "lcd_I2C.h"
  lcd_I2C_jts lcd2(0x27);
#endif

//These variables are reset during key change
uint16_t loopCount = 64001;
uint8_t  packVoltageActual_onScreen = 0;
uint8_t  packVoltageSpoofed_onScreen = 0;
uint8_t  errorCount_onScreen = 0;
uint16_t  maxEverCellVoltage_onScreen = 0;
uint16_t  minEverCellVoltage_onScreen = 0;

////////////////////////////////////////////////////////////////////////

void lcd_initialize(void)
{
	#ifdef LCD_4X20_CONNECTED
		#ifdef I2C_LIQUID_CRYSTAL
			lcd2.begin();
		#endif

		#ifdef I2C_LCD
			lcd2.begin(20,4);
		#endif

		#ifdef LCD_JTS
			lcd2.begin(20,4);
		#endif
	#endif
}

////////////////////////////////////////////////////////////////////////

//Update loop iteration ("CCCCC") on screen
bool lcd_printLoopCount(void)
{
	#ifdef LCD_4X20_CONNECTED

		lcd2.setCursor(5,3);

		if(loopCount > 64000)
		{	//overflow occurred... replace "65535" with "0    "
			lcd2.print("0    ");
			loopCount = 0; //reset loop count
		} 
		else { lcd2.print(loopCount); }
	#endif

	bool didscreenUpdateOccur = SCREEN_UPDATED; //loopCount is always different
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
			lcd2.setCursor(11,2);
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
			lcd2.setCursor(16,2);
			lcd2.print(packVoltageSpoofed_onScreen);
		}
	#endif

	return didscreenUpdateOccur;
}

////////////////////////////////////////////////////////////////////////

//only call this function when an error occurs
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
			lcd2.setCursor(3,0); //high
			lcd2.print( (hiCellVoltage_onScreen * 0.0001), 3 );
			
			didscreenUpdateOccur = SCREEN_UPDATED;
		}
	#endif

	//JTS2doNow: flash screen if cellMin less than 3.0 or cellMax > 4.1

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
			lcd2.setCursor(3,1); //low
			lcd2.print( (loCellVoltage_onScreen * 0.0001), 3 );

			didscreenUpdateOccur = SCREEN_UPDATED;
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
			lcd2.setCursor(2,2); //delta
			lcd2.print( (deltaVoltage_onScreen * 0.0001), 3 );

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
			lcd2.setCursor(14,0);
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
			lcd2.setCursor(14,1);
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

		if( packAmps_onScreen != adc_getLatestBatteryCurrent_amps() )
		{
			packAmps_onScreen = adc_getLatestBatteryCurrent_amps();
			lcd2.setCursor(15,3);
			if(packAmps_onScreen >=0 )
			{
				lcd2.print("+");
			}
			lcd2.print( (LTC68042result_packVoltage_get() * packAmps_onScreen * 0.001), 1 );
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
		case LCDUPDATE_LOOPCOUNT    : didScreenUpdateOccur = lcd_printLoopCount();            break;
		case LCDUPDATE_VPACK_ACTUAL : didScreenUpdateOccur = lcd_printStackVoltage_actual();  break;
		case LCDUPDATE_VPACK_SPOOFED: didScreenUpdateOccur = lcd_printStackVoltage_spoofed(); break;
		case LCDUPDATE_NUMERRORS    : didScreenUpdateOccur = lcd_printNumErrors();            break;
		case LCDUPDATE_CELL_HI      : didScreenUpdateOccur = lcd_printCellVoltage_hi();       break;
		case LCDUPDATE_CELL_LO      : didScreenUpdateOccur = lcd_printCellVoltage_lo();       break;
		case LCDUPDATE_CELL_DELTA   : didScreenUpdateOccur = lcd_printCellVoltage_delta();    break;
		case LCDUPDATE_POWER        : didScreenUpdateOccur = lcd_printPower();                break;
		case LCDUPDATE_CELL_MAXEVER : didScreenUpdateOccur = lcd_printMaxEverVoltage();       break;
		case LCDUPDATE_CELL_MINEVER : didScreenUpdateOccur = lcd_printMinEverVoltage();       break;
		default                     : didScreenUpdateOccur = SCREEN_UPDATED; break; //if illigal input, exit immediately
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

		loopCount++; //JTS2doLater: Figure out best spot for this
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_printStaticText(void) //screen updates are slow //only call during keyOFF
{
	#ifdef LCD_4X20_CONNECTED
		lcd2.setCursor(0,0);
		//                                          1111111111
		//                                01234567890123456789
		//4x20 screen text display format:********************
		lcd2.setCursor(0,0);  lcd2.print("hi:h.hhh (max:H.HHH)"); //row0, (3,0)=h.hhh, (14,0)=H.HHH
		lcd2.setCursor(0,1);  lcd2.print("lo:l.lll (min:L.LLL)"); //row1, (3,1)=l.lll, (14,1)=L.LLL
		lcd2.setCursor(0,2);  lcd2.print("d:d.ddd, V:VVV (SSS)"); //row2, (2,2)=d.ddd, (11,2)=VVV  , (16,2)=SSS
		lcd2.setCursor(0,3);  lcd2.print("E:0 /CCCCC, kW:+WW.0"); //row3, (2,3)=    0, (5,3)=CCCCC , (16,3)=WW.0
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

		Wire.end();
		delay(50);
		lcd_initialize();
		delay(50);
		lcd_printStaticText();

		lcd2.noBacklight();
		lcd2.noDisplay();

		
		packVoltageActual_onScreen = 0;
		errorCount_onScreen = 0;

		packVoltageSpoofed_onScreen = 0;
		LTC68042result_errorCount_set(0);
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_displayON(void)
{ 
	loopCount=64001;
	#ifdef LCD_4X20_CONNECTED
		lcd2.backlight();
		lcd2.display();
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_gridChargerWarning(void)
{
	//JTS2doNow: display warning if grid charger plugged in and key is on
}