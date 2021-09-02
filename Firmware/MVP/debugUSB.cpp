//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//Handles serial debug data transfers from LIBCM to  host
//FYI:    Serial       data transfers from host  to LiBCM are handled elsewhere.

//JTS2doLater: Gather all Serial Monitor transmissions here

#include "libcm.h"

void debugUSB_printLatest_data(void)
{	//t= 1080 microseconds max
	static uint32_t previousMillis = 0;

	if( (millis() - previousMillis >= 500) /*&& (Serial.availableForWrite() >= 63)*/ )
	{
		previousMillis = millis();

		//comma delimiter to simplify data analysis 
		//Complete string should be less than 64 characters (to prevent filling buffer)
	    //               ****************************************************************
	  //Serial.print(F("\n140,100,A, 170,156,V, 3.79,3.77,V, e,255 p,255, 28.5,kW"       ));
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
		Serial.print(F(                                           "p,"                   ));
		Serial.print(String( vPackSpoof_getPWMcounts_VPIN()                              ));		
		Serial.print(F(                                                ", "              ));
		Serial.print(String( (LTC68042result_packVoltage_get() * adc_getLatestBatteryCurrent_amps() * 0.001), 1 )); //JTS2doLater: do power calc elsewhere
		Serial.print(F(                                                      ",kW"       ));		 
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

//user entry takes form LNNN (e.g. 's123', 's003', s019)
//s: a single letter 's'
//NNN: exactly three integer digits between 0 & 255 (e.g. '006', '100', '255')
//This is not a well-written input handler... follow the above syntax EXACTLY!
uint8_t debugUSB_getSpoofedVoltage(void)
{
	static uint8_t userEntry_spoofedVoltage = 150;

	uint8_t bytesSentFromUser = Serial.available();
	if( bytesSentFromUser >= 4 )
	{
		uint8_t byteRead;

		//scan USB serial until specific character encountered
		do { byteRead = Serial.read(); } while( !(byteRead == 's') );

		if( Serial.available() ) //verify there's still data to read
		{			
			uint8_t userInteger = Serial.parseInt();
			if(byteRead == 's') { userEntry_spoofedVoltage = userInteger; }
			Serial.print("\nUser typed: ");
			Serial.print(String(userEntry_spoofedVoltage)); 
		}
	}

	return userEntry_spoofedVoltage;
}