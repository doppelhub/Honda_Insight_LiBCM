/*The MCM measures pack voltage in three different spots:
/ -MCM'E' connector, which is the actual analog pack voltage (e.g. 170 volts).  LiBCM man-in-the-middles this voltage.
/ -VPIN connector, which is a 0:5 volt analog voltage 52 times less than actual pack voltage. LiBCM man-in-the-middles this voltage.
/ -BATTSCI, which is a serial bus the BCM uses to send data to the MCM.  LiBCM replaces the OEM signal with its own implementation.
/ 
/ The MCM will throw a P-code if all three voltages aren't spoofed to within ~10** volts.
/  **Some comparisons are actually 20 volts, but for simplicity this code treats them all as having to be within 10 volts.
*/

#include "libcm.h"

static uint8_t igbtCapsCharged = 0;

//---------------------------------------------------------------------------------------

void spoofVoltageMCMe(uint8_t desiredSpoofedVoltage, uint8_t actualPackVoltage)
{
	//Derivation:
		    //Empirically determined, see: ~/Electronics/PCB (KiCAD)/RevB/V&V/voltage spoofing results.ods
	        //pwmCounts_MCME = (              actualPackVoltage  * 512) / desiredSpoofedVoltage          - 551
	        //pwmCounts_MCME = (              actualPackVoltage  * 256) / desiredSpoofedVoltage    * 2   - 551 //prevent 16b overflow
	        //pwmCounts_MCME = (    (int16_t)(actualPackVoltage) * 256) / desiredSpoofedVoltage    * 2   - 551 
	  uint8_t pwmCounts_MCME = ( ( ((int16_t)(actualPackVoltage) << 8 ) / desiredSpoofedVoltage ) << 2 ) - 551;
	  //JTS2do: Add bounds checking on the output (e.g. 0:255)

	  analogWrite(PIN_MCME_PWM, pwmCounts_MCME);
}

//---------------------------------------------------------------------------------------

uint8_t getPackVoltage_VpinIn(void)
{
	//Derivation:
	      //packVoltage_VpinIn  = VPIN_0to5v                               * 52 
	      //                      VPIN_0to5v = adc_VPIN_raw() * 5 ) / 1024            //adc_VPIN_raw() returns 10b ADC result
	      //packVoltage_VpinIn  =              adc_VPIN_raw() * 5 ) / 1024 * 52
	      //packVoltage_VpinIn  =              adc_VPIN_raw() * 5 ) / 1024 * 52 
	      //packVoltage_VpinIn  =              adc_VPIN_raw() * 260 / 1024
	      //packVoltage_VpinIn ~=              adc_VPIN_raw() /  4
	      //packVoltage_VpinIn ~=              adc_VPIN_raw() >> 2
	uint8_t packVoltage_VpinIn  =   (uint8_t)( adc_VPIN_raw() >> 2 );

	return packVoltage_VpinIn; //pack voltage in volts
} 

//---------------------------------------------------------------------------------------

void vPackSpoof_updateVoltage(uint8_t actualPackVoltage, uint8_t voltageToSpoof)
{
	if(igbtCapsCharged == 0) //true until PDU capacitors are charged (thru pre-contactor)
	{	
		uint8_t packVoltage_VPINin = getPackVoltage_VpinIn();
		
		Serial.println(); //Otherwise newline doesn't occur on Serial debug
		if( (packVoltage_VPINin + 18) >= actualPackVoltage ) //true after PDU capacitors are mostly charged
		{
			igbtCapsCharged = 1; //Remains true until next keyOff event
			Serial.print("\nBegin spoofing VPIN");
		}
	}

	//don't combine with above
	if(igbtCapsCharged == 1) //true for remainder of keyON (until key turned off) //executes in ~t=9.5 milliseconds
	{	//PDU capacitors are now fully charged
		//HVDC relay is connected
		//actual VPIN_IN voltage no longer matters; LiBCM spoofs VPIN_OUT

		//spoof VPIN voltage
		analogWrite(PIN_VPIN_OUT_PWM, voltageToSpoof);	//analogWrite automatically connects PWM timer output to VPIN_OUT
							    						//Derivation: Vpack (volts) ~= 0:5v PWM 8b value (counts)
							     						//Example: when pack voltage is 184 volts, send analogWrite(VPIN_OUT, 184)
		//spoof MCM E connector voltage
		spoofVoltageMCMe(voltageToSpoof, actualPackVoltage);

		//spoof BATTSCI voltage		
		BATTSCI_sendFrames(voltageToSpoof);
	}
}

//---------------------------------------------------------------------------------------

void vPackSpoof_handleKeyON(void)
{
	;
}

//---------------------------------------------------------------------------------------

void vPackSpoof_handleKeyOFF(void)
{
	igbtCapsCharged = 0;
	pinMode(PIN_VPIN_OUT_PWM,INPUT); //set VPIN back to high impedance
}

uint8_t vPackSpoof_isVpinSpoofed(void)
{
	return igbtCapsCharged;
}