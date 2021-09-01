//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

/*The MCM measures pack voltage in three different spots:
/ -MCM'E' connector, which is the actual analog pack voltage (e.g. 170 volts).  LiBCM man-in-the-middles this voltage.
/ -VPIN connector, which is a 0:5 volt analog voltage 52 times less than actual pack voltage. LiBCM man-in-the-middles this voltage.
/ -BATTSCI, which is a serial bus the BCM uses to send data to the MCM.  LiBCM replaces the OEM signal with its own implementation.
/ 
/ The MCM will throw a P-code if all three voltages aren't spoofed to within ~10** volts.
/  **Some comparisons are actually 20 volts, but for simplicity this code treats them all as having to be within 10 volts.
*/

#include "libcm.h"

uint8_t spoofedPackVoltage = 0;

//---------------------------------------------------------------------------------------

void spoofVoltageMCMe(void)
{
	//Derivation:
	//Empirically determined, see: ~/Electronics/PCB (KiCAD)/RevB/V&V/voltage spoofing results.ods
	      //pwmCounts_MCME = (              actualPackVoltage                  * 512) / spoofedPackVoltage          - 551
	      //pwmCounts_MCME = (              actualPackVoltage                  * 256) / spoofedPackVoltage    * 2   - 551 //prevent 16b overflow
	      //pwmCounts_MCME = (    (int16_t)(actualPackVoltage                ) * 256) / spoofedPackVoltage    * 2   - 551 
	int16_t pwmCounts_MCME = ( ( ((int16_t)(LTC68042result_packVoltage_get()) << 8 ) / spoofedPackVoltage ) << 1 ) - 551;
	  
	//bounds checking
	if     (pwmCounts_MCME > 255) {pwmCounts_MCME = 255;}
	else if(pwmCounts_MCME <   0) {pwmCounts_MCME =   0;}

	analogWrite(PIN_MCME_PWM, (uint8_t)pwmCounts_MCME);
}

void spoofVoltage_VPINout(void)
{
	//spoof VPIN_OUT voltage (to MCM).
	//LTC68042result_packVoltage_get()
	//vPackSpoof_getSpoofedPackVoltage()

	//JTS2doNow: Figure out maths to map VPIN_OUT to VPIN_IN

	//uint8_t VPIN_out_PWM = VPIN_uint8_t vpinPWM = actualPackVoltage - ()

	analogWrite(PIN_VPIN_OUT_PWM, adc_packVoltage_VpinIn());	
		//Derivation: Vpack (volts) ~= 0:5v PWM 8b value (counts)
		//Example: when pack voltage is 184 volts, send analogWrite(VPIN_OUT, 184)
}

//---------------------------------------------------------------------------------------

void vPackSpoof_updateVoltages(void)
{
	spoofedPackVoltage = (uint8_t)(LTC68042result_packVoltage_get() * 0.94); //t=20 microseconds

	spoofVoltage_VPINout();
	spoofVoltageMCMe();
	BATTSCI_setPackVoltage(spoofedPackVoltage);
}

//---------------------------------------------------------------------------------------

void vPackSpoof_handleKeyON(void)
{
	;
}

//---------------------------------------------------------------------------------------

void vPackSpoof_handleKeyOFF(void) { pinMode(PIN_VPIN_OUT_PWM,INPUT); } //set VPIN back to high impedance

//---------------------------------------------------------------------------------------

uint8_t vPackSpoof_getSpoofedPackVoltage(void) { return spoofedPackVoltage; }