//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//Handles serial debug data transfers from LIBCM to  host
//FYI:    Serial       data transfers from host  to LiBCM are handled elsewhere.

//JTS2doLater: Gather all Serial Monitor transmissions here

#include "libcm.h"

char debugCharacter = '.';

void debugUSB_printLatest_data(void)
{	//t= 1080 microseconds max
	static uint32_t previousMillis = 0;

	if( millis() - previousMillis >= DEBUG_USB_UPDATE_PERIOD_MS) //JTS2doNow: && (Serial.availableForWrite() >= 63)
	{
		previousMillis = millis();

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
	}
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