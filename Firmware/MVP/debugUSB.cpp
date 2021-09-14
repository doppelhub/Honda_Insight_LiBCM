//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//Handles serial debug data transfers from LIBCM to  host
//FYI:    Serial       data transfers from host  to LiBCM are handled elsewhere.

//JTS2doLater: Gather all Serial Monitor transmissions here

#include "libcm.h"

char debugCharacter = '.';

/////////////////////////////////////////////////////////////////////////////////////////////

//print all cell voltages from one IC
//t=2.4 milliseconds worst case
void debugUSB_printOneICsCellVoltages(uint8_t icToPrint, uint8_t decimalPlaces)
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

	if((++icToPrint) >= TOTAL_IC) { icToPrint = 0; }
}


/////////////////////////////////////////////////////////////////////////////////////////////

uint8_t cellBitmaps[TOTAL_IC] = {0};

void debugUSB_setCellBalanceStatus(uint8_t icNumber, uint16_t cellBitmap)
{
	cellBitmaps[icNumber] = cellBitmap;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void debugUSB_printCellBalanceStatus()
{
	Serial.print("\nBalance:");
	for(uint8_t ii = 0; ii < TOTAL_IC; ii++)
	{
	    Serial.print(String(cellBitmaps[ii], HEX));
	   	Serial.print(',');
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

//This function can print more than 63 characters in a single call
void debugUSB_printLatest_data_gridCharger(void)
{	
	static uint32_t previousMillisDebug = 0;
	static uint32_t previousMillisCellVoltages = 0;
	static uint8_t icCellVoltagesToPrint = 0;

	if( millis() - previousMillisDebug >= DEBUG_USB_UPDATE_PERIOD_MS)
	{
		previousMillisDebug = millis();
		previousMillisCellVoltages = millis(); //prevent cell voltage printing at same time as debug packet
		icCellVoltagesToPrint = 0;

		debugUSB_printCellBalanceStatus();
	}

	else if( (millis() - previousMillisCellVoltages >= (DEBUG_USB_UPDATE_PERIOD_MS / (TOTAL_IC + 1) ) )
		     && (icCellVoltagesToPrint < TOTAL_IC) ) //print all cell data 
	{	 
		previousMillisCellVoltages = millis();
		debugUSB_printOneICsCellVoltages(icCellVoltagesToPrint++, FOUR_DECIMAL_PLACES);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

//This function MUST NOT PRINT more than 63 characters in a single call (that's the maximum Serial buffer size)
void debugUSB_printLatest_data_keyOn(void)
{	
	static uint32_t previousMillisDebug = 0;
	static uint32_t previousMillisCellVoltages = 0;
	static uint8_t icCellVoltagesToPrint = 0;

	debugLED(1,ON);

	if( millis() - previousMillisDebug >= DEBUG_USB_UPDATE_PERIOD_MS) //JTS2doNow: && (Serial.availableForWrite() >= 63)
	{
		previousMillisDebug = millis();
		previousMillisCellVoltages = millis(); //prevent cell voltage printing at same time as debug packet

		//t= 1080 microseconds max
		//comma delimiter to simplify data analysis 
		//Complete string should be less than 64 characters (to prevent filling buffer)
	    //               ****************************************************************
	  //Serial.print(F("\n140,100,A, 170,156,V, 3.79,3.77,V, e,255,p,255, 28.5,kW, c"    ));
		Serial.print(F("\n"                                                              ));
		Serial.print(String( adc_getLatestBatteryCurrent_amps()                          ));
		Serial.print(F(     ","                                                          ));
		Serial.print(String( adc_getLatestSpoofedCurrent_amps()                          ));
		Serial.print(F(         ",A, "                                                   ));
		Serial.print(String( LTC68042result_packVoltage_get()                            ));
		Serial.print(F(                ","                                               ));
		Serial.print(String( vPackSpoof_getSpoofedPackVoltage()                          ));
		Serial.print(F(                    ",V, "                                        ));
		Serial.print(String( (LTC68042result_hiCellVoltage_get() * 0.0001), 3)            );
		Serial.print(F(                            ","                                   ));
		Serial.print(String( (LTC68042result_loCellVoltage_get() * 0.0001), 3)            );
		Serial.print(F(                                 ",V, e,"                         ));
		Serial.print(String( vPackSpoof_getPWMcounts_MCMe()                              ));
		Serial.print(F(                                          ",p,"                   ));
		Serial.print(String( vPackSpoof_getPWMcounts_VPIN()                              ));		
		Serial.print(F(                                                ", "              ));
		Serial.print(String( (LTC68042result_packVoltage_get() * adc_getLatestBatteryCurrent_amps() * 0.001), 1 )); //JTS2doLater: do power calc elsewhere
		Serial.print(F(                                                      ",kW, "     ));

		Serial.print(String( debugCharacter                                              ));

		icCellVoltagesToPrint = 0;
	}

	#ifdef PRINT_ALL_CELL_VOLTAGES_TO_USB
		else if( (millis() - previousMillisCellVoltages >= (DEBUG_USB_UPDATE_PERIOD_MS / (TOTAL_IC + 1) ) )
			     && (icCellVoltagesToPrint < TOTAL_IC) ) //print all cell data 
		{	 
			previousMillisCellVoltages = millis();
			debugUSB_printOneICsCellVoltages(icCellVoltagesToPrint++, TWO_DECIMAL_PLACES);
		}
	#else
		previousMillisCellVoltages += 0; //prevent "unused variable" compiler warning
		icCellVoltagesToPrint += 0; //prevent "unused variable" compiler warning
	#endif

	debugLED(1,OFF);
}

/////////////////////////////////////////////////////////////////////////////////////////////

//user entry takes form LNNN (e.g. 's123', 's003', s019)
//s: a single letter 's'
//NNN: exactly three integer digits between 0 & 255 (e.g. '006', '100', '255')
//This is not a well-written input handler... follow the above syntax EXACTLY!
uint8_t debugUSB_getUserInput(void)
{
	static uint8_t userEntry = 150; //initial value (when user hasn't entered value)

	uint8_t bytesSentFromUser = Serial.available();
	if( bytesSentFromUser >= 4 )
	{
		uint8_t byteRead;

		//scan USB serial until specific character encountered
		do { byteRead = Serial.read(); } while( !(byteRead == 's') );

		if( Serial.available() ) //verify there's still data to read
		{			
			uint8_t userInteger = Serial.parseInt();
			if(byteRead == 's') { userEntry = userInteger; }
			Serial.print(" User typed: ");
			Serial.print(String(userEntry)); 
		}
	}

	return userEntry;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void debugUSB_sendChar(char characterToSend)
{
	debugCharacter = characterToSend;
}