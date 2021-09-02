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


int16_t pwmCounts_MCMe = 0;
int16_t pwmCounts_VPIN_out = 0;

//---------------------------------------------------------------------------------------

int16_t vPackSpoof_getPWMcounts_MCMe(void) { return pwmCounts_MCMe; }

//---------------------------------------------------------------------------------------

int16_t vPackSpoof_getPWMcounts_VPIN(void) { return pwmCounts_VPIN_out; }

//---------------------------------------------------------------------------------------

void spoofVoltageMCMe(void)
{
  //Derivation, empirically determined (see: ~/Electronics/PCB (KiCAD)/RevB/V&V/voltage spoofing results.ods)
  //pwmCounts_MCMe = (               actualPackVoltage                 * 512) / spoofedPackVoltage         - 551
  //pwmCounts_MCMe = (               actualPackVoltage                 * 256) / spoofedPackVoltage   * 2   - 551 //prevent 16b overflow
  //pwmCounts_MCMe = (( ( ((uint16_t)actualPackVoltage               ) * 256) / spoofedPackVoltage)  * 2   - 551 
	pwmCounts_MCMe = (( ( ((uint16_t)LTC68042result_packVoltage_get()) << 8 ) / spoofedPackVoltage) << 1 ) - 551;

	//bounds checking
	if     (pwmCounts_MCMe > 255) {pwmCounts_MCMe = 255;}
	else if(pwmCounts_MCMe <   0) {pwmCounts_MCMe =   0;}

	analogWrite(PIN_MCME_PWM, (uint8_t)pwmCounts_MCMe);
}

//---------------------------------------------------------------------------------------

void spoofVoltage_VPINout(void)
{
	//      V_DIV_CORRECTION = RESISTANCE_MCM / RESISTANCE_R34
	//      V_DIV_CORRECTION = 100k           / 10k
	#define V_DIV_CORRECTION 1.1

	pwmCounts_VPIN_out = (adc_packVoltage_VpinIn() * vPackSpoof_getSpoofedPackVoltage() * V_DIV_CORRECTION )
	                     / LTC68042result_packVoltage_get();

	//bounds checking
	if     (pwmCounts_VPIN_out > 255) {pwmCounts_VPIN_out = 255;}
	else if(pwmCounts_VPIN_out <   0) {pwmCounts_VPIN_out =   0;}

	analogWrite(PIN_VPIN_OUT_PWM, (uint8_t)pwmCounts_VPIN_out);
}

//---------------------------------------------------------------------------------------

void vPackSpoof_setVoltage(uint8_t newSpoofedVoltage)
{
	spoofedPackVoltage = newSpoofedVoltage; //t=20 microseconds

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