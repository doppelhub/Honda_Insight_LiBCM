//handles all communication with lcd display(s) (4x20 and/or Nextion)

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

#define LCDUPDATE_LOOPCOUNT     0
#define LCDUPDATE_VPACK_ACTUAL  1
#define LCDUPDATE_VPACK_SPOOFED 2
#define LCDUPDATE_NUMERRORS     3
#define LCDUPDATE_CELL_HI       4
#define LCDUPDATE_CELL_LO       5
#define LCDUPDATE_CELL_DELTA    6
#define LCDUPDATE_POWER         7

#define LCDUPDATE_RESET_VALUE 7 //must be equal to the highest defined number (above) 

uint8_t lcdUpdate_Counter = LCDUPDATE_NUMERRORS;

//These variables are reset at each key change
uint16_t loopCount;
uint8_t  stackVoltageActual_previous;
uint8_t  stackVoltageSpoofed_previous;
uint8_t  errorCount_previous;

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
		delay(1000); //allow time for operator to read firmware version

		Wire.end();
		delay(50);
		lcd_initialize();
		delay(50);
		lcd_printStaticText();

		lcd2.noBacklight();
		lcd2.noDisplay();

		loopCount=0;
		stackVoltageActual_previous = 0;
		stackVoltageSpoofed_previous = 0;
		errorCount_previous = 0;
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

void lcd_printStackVoltage_actual(uint8_t stackVoltage)
{
	#ifdef LCD_4X20_CONNECTED
		if(   (lcdUpdate_Counter == LCDUPDATE_VPACK_ACTUAL)
			 && (stackVoltage != stackVoltageActual_previous) )
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
		if(   (lcdUpdate_Counter == LCDUPDATE_VPACK_SPOOFED)
		   && (stackVoltage != stackVoltageSpoofed_previous) )
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
		if(   (lcdUpdate_Counter == LCDUPDATE_NUMERRORS)
	     && (errorCount != errorCount_previous) )
		{
			errorCount_previous = errorCount;
			lcd2.setCursor(2,3);
			lcd2.print(errorCount);
		}
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_incrementLoopCount(void)
{
	#ifdef LCD_4X20_CONNECTED
		static bool didCounterOverflow = false;
		if(lcdUpdate_Counter == LCDUPDATE_LOOPCOUNT)
		{
			lcd2.setCursor(5,3);
			if(didCounterOverflow == false)
			{
				lcd2.print(loopCount);
			}
			else //counter wrapped around to zero
			{
				lcd2.print("     ");
				didCounterOverflow = false;
			}
		}
		if(loopCount == 0) { didCounterOverflow = true; }

		//JTS2doNow: Change loopCount to output seconds (probably rename function, too)
		loopCount++; //number of superloop runs since key change
		
		lcdUpdate_Counter++; //which LCD variable is updated next
		if(lcdUpdate_Counter > LCDUPDATE_RESET_VALUE) {lcdUpdate_Counter = 0;}
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_printCellVoltage_hi(uint16_t cellVoltage_counts)
{
	#ifdef LCD_4X20_CONNECTED
	static uint16_t cellHI_previous = 0;
		if(   (lcdUpdate_Counter == LCDUPDATE_CELL_HI)
	     && (cellVoltage_counts != cellHI_previous) )
		{
			cellHI_previous = cellVoltage_counts;
			lcd2.setCursor(3,0); //high
			lcd2.print( (cellVoltage_counts * 0.0001), 3 );
		}
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_printCellVoltage_lo(uint16_t cellVoltage_counts)
{
	#ifdef LCD_4X20_CONNECTED
	static uint16_t cellLO_previous = 0;
		if(   (lcdUpdate_Counter == LCDUPDATE_CELL_LO)
	     && (cellVoltage_counts != cellLO_previous) )
		{
			cellLO_previous = cellVoltage_counts;
			lcd2.setCursor(3,1); //low
			lcd2.print( (cellVoltage_counts * 0.0001), 3 );
		}
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_printCellVoltage_delta(uint16_t highCellVoltage, uint16_t lowCellVoltage)
{
	#ifdef LCD_4X20_CONNECTED
	static uint16_t delta_previous = 0; 
		if(lcdUpdate_Counter == LCDUPDATE_CELL_DELTA)
		{
			uint16_t delta = highCellVoltage - lowCellVoltage;
			if (delta != delta_previous)
			{
				delta_previous = delta;
				lcd2.setCursor(2,2); //delta
				lcd2.print( (delta * 0.0001), 3 );
			}
		}
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_printMaxEverVoltage(uint16_t voltage_counts)
{ //only called when new maxEver occurs
	#ifdef LCD_4X20_CONNECTED
		lcd2.setCursor(14,0);
		lcd2.print( (voltage_counts * 0.0001) , 3);
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_printMinEverVoltage(uint16_t voltage_counts)
{ //only called when new minEver occurs
	#ifdef LCD_4X20_CONNECTED
		lcd2.setCursor(14,1);
		lcd2.print( (voltage_counts * 0.0001) , 3);
	#endif
}

////////////////////////////////////////////////////////////////////////

void lcd_printPower(uint8_t packVoltage, int16_t packAmps)
{
	#ifdef LCD_4X20_CONNECTED
	static int16_t packAmps_previous = 0; //don't want to multiply to determine power
		if(   (lcdUpdate_Counter == LCDUPDATE_POWER)
	     && (packAmps != packAmps_previous) )
		{
			packAmps_previous = packAmps;
			lcd2.setCursor(15,3);
			if(packAmps >=0 )
			{
				lcd2.print("+");
			}
			lcd2.print( (packVoltage * packAmps * 0.001), 1 );
		}
	#endif
}