//handles all communication with lcd display(s) (4x20 and/or Nextion)

#include "libcm.h"

//The 4x20 screen text should display as follows:
//00000000001111111111
//01234567890123456789
/**********************
 *hi:3.575 (max:3.575)*0 //'h' is  (0,0)
 *lo:3.575 (min:3.575)*1 //'l' is  (0,1)
 *d:0.014, V:160 (140)*2 //'M' is (14,2)
 *err:000, loop#:12345*3 //'8' is (17,3)
 **********************/
//0123456789ABCDEF0123//

//JTS2do: only send value to screen if data has changed

#ifdef I2C_LIQUID_CRYSTAL
  #include "LiquidCrystal_I2C.h"
  LiquidCrystal_I2C lcd2(0x27, 20, 4);
#endif
#ifdef I2C_TWI
  #include "TwiLiquidCrystal.h"
  TwiLiquidCrystal lcd2(0x27);
#endif

uint8_t screenUpdatesAllowed = 1;
const uint8_t loopsToDelayUpdatesAfterKeyOn = 10;
const uint8_t loopsToDisplaySplashScreen = 10; 
uint16_t loopCount = 0;

////////////////////////////////////////////////////////////////////////

void lcd_initialize(void)
{
  #ifdef I2C_LIQUID_CRYSTAL
    lcd2.begin();
  #endif
  #ifdef I2C_TWI
    lcd2.begin(20,4);
  #endif
}

////////////////////////////////////////////////////////////////////////

void lcd_displayOFF(void)
{
	LTC6804_isoSPI_errorCountReset();

	//JTS2do:
	//close lcd connection
	//reinitialize lcd

	lcd2.clear();
	lcd2.print("LiBCM v" + String(FW_VERSION) );

	lcd2.noBacklight();
}


////////////////////////////////////////////////////////////////////////

void lcd_displayON(void)
{
	//JTS2do: Probably do nothing but set a flag here... move I2C calls to incrementLoopCount 
	lcd2.backlight();
	lcd_printStaticText();
}


////////////////////////////////////////////////////////////////////////

//JTS2do: Splash screen
void lcd_displaySplash(void)
{

}

////////////////////////////////////////////////////////////////////////

void lcd_screenUpdates_disable(void)
{
	screenUpdatesAllowed = 0;
}

////////////////////////////////////////////////////////////////////////

void lcd_screenUpdates_enable(void)
{
	screenUpdatesAllowed = 1;
}

////////////////////////////////////////////////////////////////////////

uint8_t lcd_areScreenUpdatesAllowed(void)
{
	return screenUpdatesAllowed;
}

////////////////////////////////////////////////////////////////////////

//JTS2do: Figure out where to call this (LiBCM init & each keyOFF?)
//prints text that doesn't change often 
void lcd_printStaticText(void)
{
	//top line
	lcd2.setCursor(0,0);
	lcd2.print("hi:");
	lcd2.setCursor(9,0);
	lcd2.print("(max:");
	lcd2.setCursor(19,0);
	lcd2.print(")");

	//2nd line
	lcd2.setCursor(0,1);
	lcd2.print("lo:");
	lcd2.setCursor(9,1);
	lcd2.print("(min:");
	lcd2.setCursor(19,1);
	lcd2.print(")");

	//3rd line
	lcd2.setCursor(0,2);
	lcd2.print("d:");
	lcd2.setCursor(7,2);
	lcd2.print(", V:");
	lcd2.setCursor(14,2);
	lcd2.print(" (");
	lcd2.setCursor(19,2);
	lcd2.print(")");

	//bottom line
	lcd2.setCursor(0,3);
	lcd2.print("err:0");
	lcd2.setCursor(9,3);
	lcd2.print("loop#:");
}

////////////////////////////////////////////////////////////////////////

void lcd_printStackVoltage_actual(uint8_t stackVoltage)
{
	static uint8_t stackVoltage_previous = 0;

	if(stackVoltage != stackVoltage_previous) //only update if different
	{
		lcd2.setCursor(11,2);
		lcd2.print(stackVoltage);
		stackVoltage_previous = stackVoltage;
	}
}

////////////////////////////////////////////////////////////////////////

void lcd_printStackVoltage_spoofed(uint8_t stackVoltage)
{
	static uint8_t stackVoltage_previous = 0;

	if(stackVoltage != stackVoltage_previous) //only update if different
	{
		lcd2.setCursor(16,2);
		lcd2.print(stackVoltage);
		stackVoltage_previous = stackVoltage;
	}
}



////////////////////////////////////////////////////////////////////////

//only call this function when an error occurs
void lcd_printNumErrors(uint8_t errorCount)
{
	lcd2.setCursor(4,3);
	lcd2.print(errorCount , 3);
}

////////////////////////////////////////////////////////////////////////

void lcd_incrementLoopCount(void)
{
	//JTS2do: Need to prevent all I2C traffic during keyON
	static uint16_t loopCount = 0;
	
	lcd2.setCursor(15,3);
	if(loopCount == 65535)
	{
		lcd2.print("     ");
		loopCount = 0;
	} else {
		lcd2.print(loopCount);
		loopCount++;
	}
}

////////////////////////////////////////////////////////////////////////

void lcd_resetLoopCounter(void)
{
	loopCount = 0; //reset loop counter}
}

////////////////////////////////////////////////////////////////////////

void lcd_printCellVoltage_hiLoDelta(uint16_t highCellVoltage, uint16_t lowCellVoltage)
{
	lcd2.setCursor(3,0); //max
	lcd2.print( (highCellVoltage * 0.0001), 3 );
	lcd2.setCursor(3,1); //min
	lcd2.print( (lowCellVoltage * 0.0001), 3 );

	lcd2.setCursor(2,2); //delta
	lcd2.print( ((highCellVoltage - lowCellVoltage) * 0.0001), 3 );
}

////////////////////////////////////////////////////////////////////////


void lcd_printMaxEverVoltage(uint16_t voltage)
{
	lcd2.setCursor(14,0);
	lcd2.print( (voltage * 0.0001) , 3);
}


////////////////////////////////////////////////////////////////////////


void lcd_printMinEverVoltage(uint16_t voltage)
{
	lcd2.setCursor(14,1);
	lcd2.print( (voltage * 0.0001) , 3);
}


////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////