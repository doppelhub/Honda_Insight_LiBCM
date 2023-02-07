//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//Handles serial debug data transfers from LIBCM to  host
//FYI:    Serial       data transfers from host  to LiBCM are handled in USB_interface.c.

//JTS2doLater: Gather all Serial Monitor transmissions here

#include "libcm.h"

uint16_t cellBalanceBitmaps[TOTAL_IC] = {0};
uint16_t cellBalanceThreshold = CELL_VMAX_REGEN; //no cells are reported as balancing until first balance status update occurs
uint8_t dataTypeToStream = DEBUGUSB_STREAM_POWER;
uint32_t dataUpdatePeriod_ms = 250;
uint8_t transmitStatus = NOT_TRANSMITTING_LARGE_MESSAGE;

/////////////////////////////////////////////////////////////////////////////////////////////

void    debugUSB_dataTypeToStream_set(uint8_t dataType) { dataTypeToStream = dataType; }
uint8_t debugUSB_dataTypeToStream_get(void) { return dataTypeToStream; }

/////////////////////////////////////////////////////////////////////////////////////////////

void     debugUSB_dataUpdatePeriod_ms_set(uint16_t newPeriod) { dataUpdatePeriod_ms = newPeriod; }
uint16_t debugUSB_dataUpdatePeriod_ms_get(void) { return dataUpdatePeriod_ms; }

/////////////////////////////////////////////////////////////////////////////////////////////

//print one IC's QTY12 cell voltages
//the first IC's data is stored in the 0th array element, regardless of the first IC's actual address (e.g. 0x2),  
//t=2.4 milliseconds worst case
//JTS2doLater: Place inside debugUSB_printData_cellVoltages()
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

//JTS2doLater: Place inside debugUSB_printData_cellVoltages()?
void debugUSB_printCellBalanceStatus(void)
{
	bool anyCellsBalancing = NO;
	for(uint8_t ii=0; ii<TOTAL_IC; ii++)
	{
		if(cellBalanceBitmaps[ii] != 0) { anyCellsBalancing = YES; }
	}

	if(anyCellsBalancing == YES)
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
	else //(anyCellsBalancing == NO)
	{
		//JTS2doLater: Change text if pack is unbalanced, but something else is preventing balancing
		Serial.print(F("\nPack Balanced"));
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
	static uint32_t previousMillisGrid = 0;

	if( (uint32_t)(millis() - previousMillisGrid) >= DEBUG_USB_UPDATE_PERIOD_GRIDCHARGE_mS)
	{
		previousMillisGrid = millis();

		for( uint8_t ii = 0; ii < TOTAL_IC; ii++) { debugUSB_printOneICsCellVoltages(ii, FOUR_DECIMAL_PLACES); }

		debugUSB_printCellBalanceStatus();

		Serial.print('\n');
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void debugUSB_printData_power(void)
{
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

	transmitStatus = NOT_TRANSMITTING_LARGE_MESSAGE;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void debugUSB_printData_BATTMETSCI(void)
{
	transmitStatus = NOT_TRANSMITTING_LARGE_MESSAGE;
	//BATTSCI and METSCI are printed per-byte within functions METSCI_readByte() and BATTSCI_writeByte()
}

/////////////////////////////////////////////////////////////////////////////////////////////

void debugUSB_printData_cellVoltages(void)
{
	static uint8_t icToPrint = 0; //first IC's data is stored in the 0th array element, regardless of the first IC's actual address (e.g. 0x2),

	Serial.print(F("\nIC"));
	Serial.print(String(icToPrint));
	for(uint8_t cellToPrint = 0; cellToPrint < CELLS_PER_IC; cellToPrint++)
	{
		Serial.print(',');
		Serial.print( String( LTC68042result_specificCellVoltage_get(icToPrint,cellToPrint) * 0.0001, FOUR_DECIMAL_PLACES) );
	}

	if(++icToPrint < TOTAL_IC) { transmitStatus = TRANSMITTING_LARGE_MESSAGE; }
	else                       { transmitStatus = NOT_TRANSMITTING_LARGE_MESSAGE; icToPrint = 0; Serial.print(F("\ncell voltages:")); }
}

/////////////////////////////////////////////////////////////////////////////////////////////

void debugUSB_printData_temperatures(void)
{
	Serial.print(F("\nT_batt:"));
	Serial.print(String(temperature_battery_getLatest()));
	Serial.print(F(", T_in:"));
	Serial.print(String(temperature_intake_getLatest()));
	Serial.print(F(", T_out:"));
	Serial.print(String(temperature_exhaust_getLatest()));
	Serial.print(F(", T_charger:"));
	Serial.print(String(temperature_gridCharger_getLatest()));
	Serial.print(F(", T_bay:"));
	Serial.print(String(temperature_ambient_getLatest()));
	Serial.print('C');

}

/////////////////////////////////////////////////////////////////////////////////////////////

//JTS2doLater: Add debug data
void debugUSB_printData_debug(void)
{
	status_printState();
}

/////////////////////////////////////////////////////////////////////////////////////////////

//Sending more than 63 characters per call makes this function blocking (until the buffer empties)!
void debugUSB_printLatestData(void)
{	
	static uint32_t previousMillis = 0;

	//print debug data if enabled ('$DISP=DBG')
	//this will overflow serial transmit buffer if too much data transmitted //necessary to capture all debug data, even though timing may exceed loopPeriod_ms
	if(debugUSB_dataTypeToStream_get() == DEBUGUSB_STREAM_DEBUG) { debugUSB_printData_debug(); }
	
	//print message if it's time and there's room in the serial transmit buffer
	else if( ( ((uint32_t)(millis() - previousMillis) >= debugUSB_dataUpdatePeriod_ms_get()) || (transmitStatus == TRANSMITTING_LARGE_MESSAGE) ) &&
		(Serial.availableForWrite() > 62) )
	{
		previousMillis = millis();

		if     (debugUSB_dataTypeToStream_get() == DEBUGUSB_STREAM_POWER)      { debugUSB_printData_power();        }
		else if(debugUSB_dataTypeToStream_get() == DEBUGUSB_STREAM_BATTMETSCI) { debugUSB_printData_BATTMETSCI();   }
		else if(debugUSB_dataTypeToStream_get() == DEBUGUSB_STREAM_CELL)       { debugUSB_printData_cellVoltages(); }
		else if(debugUSB_dataTypeToStream_get() == DEBUGUSB_STREAM_TEMP)       { debugUSB_printData_temperatures(); }
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void debugUSB_printHardwareRevision(void)
{
	Serial.print(F("\nHW:"));
	if     (gpio_getHardwareRevision() == HW_REV_C) { Serial.print('C'); }
	else if(gpio_getHardwareRevision() == HW_REV_D) { Serial.print('D'); }
	else if(gpio_getHardwareRevision() == HW_REV_E) { Serial.print('E'); }
	else if(gpio_getHardwareRevision() == HW_REV_F) { Serial.print('F'); }
}

/////////////////////////////////////////////////////////////////////////////////////////////

void debugUSB_printConfigParameters(void)
{
	#ifdef        SET_CURRENT_HACK_00
		Serial.print(F("/+00%"));
	#elif defined SET_CURRENT_HACK_20
		Serial.print(F("/+20%"));
	#elif defined SET_CURRENT_HACK_20
		Serial.print(F("/+40%"));
	#elif defined SET_CURRENT_HACK_20
		Serial.print(F("/+60%"));
	#endif

	#ifdef        BATTERY_TYPE_5AhG3
		Serial.print(F("/5AhG3"));
	#elif defined BATTERY_TYPE_47AhFoMoCo
		Serial.print(F("/FoMoCo"));
	#endif

	#ifdef        STACK_IS_48S
		Serial.print(F("/48S"));
	#elif defined STACK_IS_60S
		Serial.print(F("/60S"));
	#endif

	#ifdef        VOLTAGE_SPOOFING_DISABLE
		Serial.print(F("/Vs=off"));
	#elif defined VOLTAGE_SPOOFING_ASSIST_ONLY_VARIABLE
		Serial.print(F("/Vs=ast"));
	#elif defined VOLTAGE_SPOOFING_ASSIST_ONLY_BINARY
		Serial.print(F("/Vs=bin"));
	#elif defined VOLTAGE_SPOOFING_ASSIST_AND_REGEN
		Serial.print(F("/Vs=all"));
	#endif

	Serial.print(F("/Heat:"));
	if(heater_isInstalled() == YES) { Serial.print('Y'); }
	else                            { Serial.print('N'); }
}
