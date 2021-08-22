//handles all communication with lcd display(s) (4x20 and/or Nextion)

#include "libcm.h"

//JTS2do: only send value to screen if data has changed
//JTS2do: Send up to 32 bytes per transmission (if timing allows it)

//4x20 display accepts SCL up to 100 kHz

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

uint16_t loopCount = 0;
uint8_t stackVoltageActual_previous = 0;
uint8_t stackVoltageSpoofed_previous = 0;

uint8_t screenUpdatesAllowed = 1; //JTS2do: not used anywhere... 
const uint8_t loopsToDelayUpdatesAfterKeyOn = 10; //JTS2do: not used anywhere... 
const uint8_t loopsToDisplaySplashScreen = 10; //JTS2do: not used anywhere... 

////////////////////////////////////////////////////////////////////////

//JTS2do: Figure out where to call this (LiBCM init & each keyOFF?)
//prints text that doesn't change often 
void lcd_printStaticText(void)
{
	lcd2.setCursor(0,0);
	//                                          1111111111
	//                                01234567890123456789
	//4x20 screen text display format:********************
	lcd2.setCursor(0,0);  lcd2.print("hi:h.hhh (max:H.HHH)"); //row0, (3,0)=h.hhh, (14,0)=H.HHH
	lcd2.setCursor(0,1);  lcd2.print("lo:l.lll (min:L.LLL)"); //row1, (3,1)=l.lll, (14,1)=L.LLL
	lcd2.setCursor(0,2);  lcd2.print("d:d.ddd, V:VVV (SSS)"); //row2, (2,2)=d.ddd, (11,2)=VVV,   (16,2)=SSS
	lcd2.setCursor(0,3);  lcd2.print("E:0 /CCCCC, kW:+WW.0"); //row3, (2,3)=    0, (5,3)=CCCCC,  (16,3)=WW.0
}

////////////////////////////////////////////////////////////////////////

void lcd_initialize(void)
{
  #ifdef I2C_LIQUID_CRYSTAL
    lcd2.begin();
  #endif

  #ifdef I2C_LCD
    lcd2.begin(20,4);
  #endif

  #ifdef LCD_JTS
    lcd2.begin(20,4);
  #endif

}

////////////////////////////////////////////////////////////////////////

void lcd_displayOFF(void)
{
	lcd2.clear();
	lcd2.print("LiBCM v" + String(FW_VERSION) );
	delay(1000);

	LTC6804_isoSPI_errorCountReset();

	//JTS2do:
	//-close lcd connection
	//-reinitialize lcd

	lcd_printStaticText();

	lcd2.noBacklight();
	lcd2.noDisplay();
}

////////////////////////////////////////////////////////////////////////

void lcd_displayON(void)
{
	//JTS2do: Ideally do nothing but set a flag here... move I2C calls to incrementLoopCount 
	lcd2.backlight();
	lcd2.display();
	loopCount=0;
	stackVoltageActual_previous = 0;
	stackVoltageSpoofed_previous = 0;
}

////////////////////////////////////////////////////////////////////////

//JTS2do: Splash screen
void lcd_displaySplash(void)
{
	;
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

void lcd_printStackVoltage_actual(uint8_t stackVoltage)
{
	if(stackVoltage != stackVoltageActual_previous) //only update if different
	{
		lcd2.setCursor(11,2);
		lcd2.print(stackVoltage);
		stackVoltageActual_previous = stackVoltage;
	}
}

////////////////////////////////////////////////////////////////////////

void lcd_printStackVoltage_spoofed(uint8_t stackVoltage)
{
	if(stackVoltage != stackVoltageSpoofed_previous) //only update if different
	{
		lcd2.setCursor(16,2);
		lcd2.print(stackVoltage);
		stackVoltageSpoofed_previous = stackVoltage;
	}
}

////////////////////////////////////////////////////////////////////////

//only call this function when an error occurs
void lcd_printNumErrors(uint8_t errorCount)
{
	lcd2.setCursor(2,3);
	lcd2.print(errorCount);
}

////////////////////////////////////////////////////////////////////////

void lcd_incrementLoopCount(void)
{
	//JTS2do: Reduce I2C traffic during keyON
	lcd2.setCursor(5,3);
	if(loopCount == 0)
	{
		lcd2.print("     ");
	} else {
		lcd2.print(loopCount);
	}
	loopCount++;
}

////////////////////////////////////////////////////////////////////////

void lcd_printCellVoltage_hiLoDelta(uint16_t highCellVoltage, uint16_t lowCellVoltage)
{	//t=28 milliseconds
	digitalWrite(PIN_LED1,HIGH); //temp

	lcd2.setCursor(3,0); //high
	lcd2.print( (highCellVoltage * 0.0001), 3 );
	lcd2.setCursor(3,1); //low
	lcd2.print( (lowCellVoltage * 0.0001), 3 );

	lcd2.setCursor(2,2); //delta
	lcd2.print( ((highCellVoltage - lowCellVoltage) * 0.0001), 3 );

	digitalWrite(PIN_LED1,LOW); //temp
}

////////////////////////////////////////////////////////////////////////

//JTS2do: Clear max/min on keyOFF
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

void lcd_printPower(uint8_t packVoltage, int16_t packAmps)
{
	lcd2.setCursor(15,3);
	if(packAmps >=0 )
	{
		lcd2.print("+");
	}
	lcd2.print( (packVoltage * packAmps * 0.001), 1 );
}

////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////