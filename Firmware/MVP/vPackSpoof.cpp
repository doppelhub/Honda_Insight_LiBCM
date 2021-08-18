//This file is vPackSpoof.c

/*The MCM measures pack voltage in three different spots:
/ -MCM'E' connector, which is man-in-the-middled by LiBCM, and;
/ -VPIN connector, which is man-in-the-middled by LiBCM, and;
/ -BATTSCI, which is sent by LiBCM over BATTSCI serial bus.
/ 
/ The MCM will throw a P-code if all three voltages aren't spoofed to within ~10** volts.
/  **Some comparisons are actually 20 volts, but this code treats them all as having to be less than 10 volts.
*/

#include "libcm.h"

static uint8_t igbtCapsCharged = 0;

static const uint8_t LUT_MCM_E[256] =
{
	000, 000, 000, 000, 000, 000, 000, 000,   000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000,   000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000,   000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000,   000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000,   000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000,   000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000,   000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000,   000, 000, 000, 000, 000, 000, 000, 000,

	000, 000, 000, 000, 000, 000, 000, 000,   000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000,   000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000,   000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000,   000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000,   000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000,   000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000,   000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000,   000, 000, 000, 000, 000, 000, 000, 000,
};


//---------------------------------------------------------------------------------------

void vPackSpoof_updateVoltage(uint8_t spoofedPackVoltage, int16_t packCurrent)
{
	static uint16_t vSpoof_BATTSCI = 0;  //JTSdebug

	if(igbtCapsCharged == 0) 
	{	//key just turned on, but PDU capacitors are still charging thru pre-contactor (VPIN_IN tells us this)
		uint16_t stackVoltageVpinIn = ( ( (uint16_t)(analogRead(PIN_VPIN_IN)) ) );
				//Derivation:
				//VPIN_In_Counts = analogRead(VPIN_IN)
				//VPIN_In_0to5v = (VPIN_In_Counts * 5V) / 1024counts
				//stackVoltageVpinIn = (VPIN_In_0to5v * 52Vpack/VPIN)
				//stackVoltageVpinIn = (analogRead(VPIN_IN) * 5 /1024 * 52
				//stackVoltageVpinIn = analogRead(VPIN_IN) * 260 / 1024
				//stackVoltageVpinIn ~= analogRead(VPIN_IN) / 4

		//if(stackVoltageVpinIn ~= actualPackVoltage@LTC6804)
		{
			igbtCapsCharged = 1; //Remains true until next keyOff event
		}
		//vSpoof_BATTSCI = stackVoltageVpinIn; //JTSdebug.  Get initial Lambda Gen voltage setpoint
		//Serial.print( "\nvSpoof_BATTSCI = " + String(vSpoof_BATTSCI) );
	}

	if(igbtCapsCharged == 1) //don't combine with above (e.g. "else if").  Needs to execute ASAP!
	{	//PDU capacitors are charged and high current HVDC relay is connected

		//spoof VPIN voltage
		//analogWrite(PIN_VPIN_OUT_PWM, spoofedPackVoltage ); //override VPIN_IN with LTC6804-derived voltage
							     							//analogWrite automatically connects PWM timer output to VPIN_OUT 
							     							//VPIN_IN measurements are no longer valid after setting PWM output
							     					//Derivation:
													//vStack(in volts) is ~= VPIN_OUT
							     					//Example: 184 vPack = analogWrite(VPIN_OUT, 184)
							     					//         ^^^                               ^^^

		//spoof MCM E connector voltage
		//This needs to be a lookup table, based on real-world data gathered in car
		

		//JTS Debug (to determine what MCM measures 'E' voltage at, versus actual voltage (average) at E connector)
		uint8_t bytesSentFromUser = Serial.available();
		if( bytesSentFromUser >= 4 )
		{
			uint8_t byteRead;
			do //scan USB serial until specific character encountered
			{
				byteRead = Serial.read();
			} while( !( (byteRead == 'b') || (byteRead == 'p' || (byteRead == 'e') ) ) && (Serial.available() > 0) );

			if( Serial.available() ) //verify there's still data to read
			{			
				uint8_t userInteger = Serial.parseInt();

				if(byteRead == 'b') //lambda voltage command entered
				{
					vSpoof_BATTSCI = userInteger;
					Serial.print( "\n vBATTSCI = " + String(vSpoof_BATTSCI) );
				} 
				else if(byteRead == 'p')
				{
					analogWrite(PIN_VPIN_OUT_PWM, userInteger);
					Serial.print( "\n VPIN_DUTY = " + String(userInteger) );
				}
				else if(byteRead == 'e')
				{
					analogWrite(PIN_CONNE_PWM, userInteger);
					Serial.print( "\n MCM'E'_DUTY = " + String(userInteger) );
				}
			}
		}

		//spoof BATTSCI voltage		
		BATTSCI_sendFrames(vSpoof_BATTSCI, packCurrent);
		//BATTSCI_sendFrames(spoofedPackVoltage, packCurrent);
	}
}

//---------------------------------------------------------------------------------------
