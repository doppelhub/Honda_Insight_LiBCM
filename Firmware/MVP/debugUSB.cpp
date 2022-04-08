//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//Handles serial debug data transfers from LIBCM to  host
//FYI:    Serial       data transfers from host  to LiBCM are handled elsewhere.

//JTS2doLater: Gather all Serial Monitor transmissions here

#include "libcm.h"

uint16_t cellBalanceBitmaps[TOTAL_IC] = {0};
uint16_t cellBalanceThreshold = CELL_VMAX_REGEN; //no cells are reported as balancing until first balance status update occurs

/////////////////////////////////////////////////////////////////////////////////////////////

//print one IC's QTY12 cell voltages
//the first IC's data is stored in the 0th array element, regardless of the first IC's actual address (e.g. 0x2),  
//t=2.4 milliseconds worst case
void debugUSB_printOneICsCellVoltages(uint8_t icToPrint, uint8_t decimalPlaces)
{
	if(icToPrint > TOTAL_IC) { return; } //illegal IC number entered
	else
	{
		//puts QTY64 bytes into the USB serial buffer, which can take up to QTY64 bytes
		//Adding any more characters to this string will prevent Serial.print from returning (until the buffer isn't full)
		Serial.print(F("\nIC"));
		Serial.print(String(icToPrint));
		for(int cellToPrint = 0; cellToPrint < CELLS_PER_IC; cellToPrint++)
		{
			Serial.print(',');
			Serial.print( String( LTC68042result_specificCellVoltage_get(icToPrint,cellToPrint) * 0.0001, decimalPlaces) );
		}
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////

void debugUSB_setCellBalanceStatus(uint8_t icNumber, uint16_t cellBitmap, uint16_t cellDischargeVoltageThreshold)
{
	//For readability, need to flip cellBitmap bit-order (e.g.'0b000010000101' becomes '0b101000010000'), so that LSB/MSB is cell12/cell01, respectively  
	uint16_t bitUnderTest = 0;
	uint16_t flippedBitmap = 0;

	for(uint8_t ii = 0; ii < CELLS_PER_IC; ii++)
	{
		bitUnderTest = (cellBitmap & ((1<<(CELLS_PER_IC-1)) >> ii)); //flip single bit
		if(bitUnderTest != 0) { flippedBitmap |= (1 << ii); } //this specific bit is '1'
	}

	cellBalanceBitmaps[icNumber] = flippedBitmap;
	cellBalanceThreshold = cellDischargeVoltageThreshold;
}

/////////////////////////////////////////////////////////////////////////////////////////////
void debugUSB_printCellBalanceStatus(void)
{
	Serial.print(F("\nDischarging cells above "));
	Serial.print(cellBalanceThreshold*0.0001,4);
	Serial.print(F(" V (0x): "));

	//print discharge resistor bitmap status
	for(uint8_t ii = 0; ii < TOTAL_IC; ii++)
	{
	    Serial.print(String(cellBalanceBitmaps[ii], HEX));
	   	Serial.print(',');
	}

}

/////////////////////////////////////////////////////////////////////////////////////////////

void debugUSB_displayUptime_seconds(void)
{
	Serial.print(F("\nUptime(s): "));
	Serial.print( String(millis() * 0.001) );
}

/////////////////////////////////////////////////////////////////////////////////////////////

//This function can print more than 63 characters in a single call
//t=29 ms in v0.7.2
void debugUSB_printLatest_data_gridCharger(void)
{	
	static uint32_t previousMillisDebug = 0;

	if( millis() - previousMillisDebug >= DEBUG_USB_UPDATE_PERIOD_GRIDCHARGE_mS)
	{
		previousMillisDebug = millis();

		for( uint8_t ii = 0; ii < TOTAL_IC; ii++) { debugUSB_printOneICsCellVoltages(ii, FOUR_DECIMAL_PLACES); }

		debugUSB_printCellBalanceStatus();

		Serial.print('\n');
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

//This function MUST NOT print more than 63 characters in a single call (that's the maximum Serial buffer size)
void debugUSB_printLatest_data_keyOn(void)
{	
	static uint32_t previousMillisDebug = 0;
	static uint32_t previousMillisCellVoltages = 0;
	static uint8_t icCellVoltagesToPrint = 0;

	if( (millis() - previousMillisDebug >= DEBUG_USB_UPDATE_PERIOD_KEYON_mS) && /*enough time has passed*/
	    (Serial.availableForWrite() > 62) ) //the transmit buffer can intake the entire message
	{
		previousMillisDebug = millis();
		previousMillisCellVoltages = millis(); //prevent cell voltage printing at same time as debug packet

		//t= 1080 microseconds max
		//comma delimiter to simplify data analysis 
		//Complete string should be 63 characters or less (to prevent waiting for the buffer to empty)
		//                        111111111122222222223333333333444444444455555555556666
		//               123456789012345678901234567890123456789012345678901234567890123
	    //               ***************************************************************
	  //Serial.print(F("\n140,100,A, 170,156,V, 3.79,3.77,V, 34567,mAh, 28.5,kW, 23,C"  )); //use comma for easy parsing
		Serial.print(F("\n"                                                             ));
		Serial.print(String( adc_getLatestBatteryCurrent_amps()                         ));
		Serial.print(F(     ","                                                         ));
		Serial.print(String( adc_getLatestSpoofedCurrent_amps()                         ));
		Serial.print(F(         ",A, "                                                  ));
		Serial.print(String( LTC68042result_packVoltage_get()                           ));
		Serial.print(F(                ","                                              ));
		Serial.print(String( vPackSpoof_getSpoofedPackVoltage()                         ));
		Serial.print(F(                    ",V, "                                       ));
		Serial.print(String( (LTC68042result_hiCellVoltage_get() * 0.0001), 3           ));
		Serial.print(F(                            ","                                  ));
		Serial.print(String( (LTC68042result_loCellVoltage_get() * 0.0001), 3           ));
		Serial.print(F(                                 ",V, "                          ));
		Serial.print(String( SoC_getBatteryStateNow_mAh()                               ));		
		Serial.print(F(                                          ",mAh, "               ));
		Serial.print(String( (LTC68042result_packVoltage_get() * adc_getLatestBatteryCurrent_amps() * 0.001), 1 ));
		Serial.print(F(                                                    ",kW, "      ));
		Serial.print(String( temperature_battery_getLatest()                            ));		
		Serial.print(F(                                                           ",C " ));

		icCellVoltagesToPrint = 0;
	}

	#ifdef PRINT_ALL_CELL_VOLTAGES_TO_USB
		else if( (millis() - previousMillisCellVoltages >= (DEBUG_USB_UPDATE_PERIOD_KEYON_mS / (TOTAL_IC + 1) ) ) &&
			     (icCellVoltagesToPrint < TOTAL_IC) ) //print all cell data 
		{	 
			previousMillisCellVoltages = millis();
			debugUSB_printOneICsCellVoltages(icCellVoltagesToPrint++, TWO_DECIMAL_PLACES);
		}
	#else
		previousMillisCellVoltages += 0; //prevent "unused variable" compiler warning
		icCellVoltagesToPrint += 0; //prevent "unused variable" compiler warning
	#endif
}

////////////////////////////////////////////////////////////////////////////////////

//calculate delta between start and stop time
//store start time: DEBUGUSB_TIMER_START
//calculate delta:  DEBUGUSB_TIMER_STOP
void debugUSB_Timer(bool timerAction)
{
  static uint32_t startTime = 0;

  if(timerAction == DEBUGUSB_TIMER_START) { startTime = millis(); }
  else
  {
      uint32_t stopTime = millis();
      Serial.print(F("\nDelta: "));
      Serial.print(stopTime - startTime);
      Serial.print(F(" ms\n"));
  }
}