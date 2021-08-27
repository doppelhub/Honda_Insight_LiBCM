//Handles serial debug data transfers from LIBCM to  host
//FYI:    Serial       data transfers from host  to LiBCM are handled elsewhere.

//JTS2doLater: Gather all Serial Monitor transmissions here

#include "libcm.h"

static uint8_t  latest_batteryCurrentActual_amps    = 0;
static uint8_t  latest_batteryCurrentSpoofed_amps   = 0;
static uint16_t latest_batteryCurrent_counts        = 0;
static uint16_t latest_cellHI_counts                = 0;
static uint16_t latest_cellLO_counts                = 0;
static uint8_t  latest_VpackActual_volts            = 0;
static uint8_t  latest_VpackSpoofed_volts           = 0;

void debugUSB_batteryCurrentActual_amps(uint8_t current_amps)
{
	latest_batteryCurrentActual_amps = current_amps;
}

void debugUSB_batteryCurrentSpoofed_amps(uint8_t current_amps)
{
	latest_batteryCurrentSpoofed_amps = current_amps;
}

void debugUSB_batteryCurrent_counts(uint16_t current_counts)
{
	latest_batteryCurrent_counts = current_counts;
}

void debugUSB_cellHI_counts(uint16_t voltage_counts)
{
	latest_cellHI_counts = voltage_counts;
}

void debugUSB_cellLO_counts(uint16_t voltage_counts)
{
	latest_cellLO_counts = voltage_counts;
}

void debugUSB_VpackActual_volts(uint8_t voltage_volts)
{
	latest_VpackActual_volts = voltage_volts;
}

void debugUSB_VpackSpoofed_volts(uint8_t voltage_volts)
{
	latest_VpackSpoofed_volts = voltage_volts;
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
		Serial.print(String(latest_batteryCurrentActual_amps)                             );
		Serial.print(F(       "A("                                                       ));
		Serial.print(String(latest_batteryCurrentSpoofed_amps)                            );
		Serial.print(F(            "A)("                                                 ));
		Serial.print(String(latest_batteryCurrent_counts)                                 );
		Serial.print(F(                   "d), Vp:"                                      ));
		Serial.print(String(latest_VpackActual_volts)                                     );
		Serial.print(F(                             "V("                                 ));
		Serial.print(String(latest_VpackSpoofed_volts)                                    );
		Serial.print(F(                                   "), VcH:"                      ));
		Serial.print(String(latest_cellHI_counts*0.0001,4)                                );
		Serial.print(F(                                               ", VcL:"           ));
		Serial.print(String(latest_cellLO_counts*0.0001,4)                                );
	}


}