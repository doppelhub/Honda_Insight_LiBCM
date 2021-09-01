//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//Handles serial debug data transfers from LIBCM to  host
//FYI:    Serial       data transfers from host  to LiBCM are handled elsewhere.

//JTS2doLater: Gather all Serial Monitor transmissions here

#include "libcm.h"

//JTS2do: roll this into a function in adc.c and then remove it from this file
static uint8_t  latest_batteryCurrentSpoofed_amps   = 0;

void debugUSB_batteryCurrentSpoofed_amps(uint8_t current_amps)
{
	latest_batteryCurrentSpoofed_amps = current_amps;
}

void debugUSB_printLatest_data(void)
{	//t= 1080 microseconds max
	static uint32_t previousMillis = 0;

	if( (millis() - previousMillis >= 500) /*&& (Serial.availableForWrite() >= 63)*/ )
	{
		previousMillis = millis();

		//Complete string should be less than 64 characters (to prevent filling buffer)
	    //               ****************************************************************
	  //Serial.print(F("\nI:140A(100A)(1023d), Vp:170V(156V), VcH:3.789, VcL:3.771"      ));
		Serial.print(F("\nI:"                                                            ));
		Serial.print(String( adc_getLatestBatteryCurrent_amps()                          ));
		Serial.print(F(       "A("                                                       ));
		Serial.print(String( adc_getLatestSpoofedCurrent_amps()                          ));
		Serial.print(F(            "A)("                                                 ));
		Serial.print(String( adc_getLatestBatteryCurrent_counts()                        ));
		Serial.print(F(                   "d), Vp:"                                      ));
		Serial.print(String( LTC68042result_packVoltage_get()                           ));
		Serial.print(F(                             "V("                                 ));
		Serial.print(String( vPackSpoof_getSpoofedPackVoltage()                          ));
		Serial.print(F(                                   "), VcH:"                      ));
		Serial.print(String( (LTC68042result_hiCellVoltage_get() * 0.0001), 4)            );
		Serial.print(F(                                               ", VcL:"           ));
		Serial.print(String( (LTC68042result_loCellVoltage_get() * 0.0001), 4)            );
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
			Serial.print("User typed: ");
			Serial.print(String(userEntry_spoofedVoltage)); 
		}
	}

	return userEntry_spoofedVoltage;
}