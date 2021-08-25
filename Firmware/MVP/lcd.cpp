//handles all communication with lcd display(s) (4x20 and/or Nextion)

#include "libcm.h"

//JTS2doNow: only send value to screen if data has changed

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

////////////////////////////////////////////////////////////////////////

void lcd_printStaticText(void) //screen updates are slow
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

void lcd_displayOFF(void)
{
	#ifdef LCD_4X20_CONNECTED
		lcd2.clear();
		lcd2.setCursor(0,0);
		lcd2.print("LiBCM v" + String(FW_VERSION) );
		delay(1000); //allow time to read firmware version

		LTC6804_isoSPI_errorCountReset();

		Wire.end();
		delay(50);
		lcd_initialize();
		delay(50);
		lcd_printStaticText();

		lcd2.noBacklight();
		lcd2.noDisplay();
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_displayON(void)
{ 
	#ifdef LCD_4X20_CONNECTED
		lcd2.backlight();
		lcd2.display();
		loopCount=0;
		stackVoltageActual_previous = 0;
		stackVoltageSpoofed_previous = 0;
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_printStackVoltage_actual(uint8_t stackVoltage)
{
	#ifdef LCD_4X20_CONNECTED
		if(stackVoltage != stackVoltageActual_previous) //only update if different
		{
			lcd2.setCursor(11,2);
			lcd2.print(stackVoltage);
			stackVoltageActual_previous = stackVoltage;
		}
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_printStackVoltage_spoofed(uint8_t stackVoltage)
{
	#ifdef LCD_4X20_CONNECTED
		if(stackVoltage != stackVoltageSpoofed_previous) //only update if different
		{
			lcd2.setCursor(16,2);
			lcd2.print(stackVoltage);
			stackVoltageSpoofed_previous = stackVoltage;
		}
	#endif
}

////////////////////////////////////////////////////////////////////////

//only call this function when an error occurs
void lcd_printNumErrors(uint8_t errorCount)
{
	#ifdef LCD_4X20_CONNECTED
		lcd2.setCursor(2,3);
		lcd2.print(errorCount);
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_incrementLoopCount(void)
{
	#ifdef LCD_4X20_CONNECTED
		lcd2.setCursor(5,3);
		if(loopCount == 0)
		{
			lcd2.print("     ");
		} else {
			lcd2.print(loopCount);
		}
		loopCount++;
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_printCellVoltage_hiLoDelta(uint16_t highCellVoltage, uint16_t lowCellVoltage)
{	//t=18 milliseconds (previously 28 milliseconds, prior to rewriting sendQuartet)
	#ifdef LCD_4X20_CONNECTED
		lcd2.setCursor(3,0); //high
		lcd2.print( (highCellVoltage * 0.0001), 3 );
		lcd2.setCursor(3,1); //low
		lcd2.print( (lowCellVoltage * 0.0001), 3 );

		lcd2.setCursor(2,2); //delta
		lcd2.print( ((highCellVoltage - lowCellVoltage) * 0.0001), 3 );
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_printMaxEverVoltage(uint16_t voltage)
{
	#ifdef LCD_4X20_CONNECTED
		lcd2.setCursor(14,0);
		lcd2.print( (voltage * 0.0001) , 3);
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_printMinEverVoltage(uint16_t voltage)
{
	#ifdef LCD_4X20_CONNECTED
		lcd2.setCursor(14,1);
		lcd2.print( (voltage * 0.0001) , 3);
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_printPower(uint8_t packVoltage, int16_t packAmps)
{
	#ifdef LCD_4X20_CONNECTED
		lcd2.setCursor(15,3);
		if(packAmps >=0 )
		{
			lcd2.print("+");
		}
		lcd2.print( (packVoltage * packAmps * 0.001), 1 );
	#endif
}

////////////////////////////////////////////////////////////////////////