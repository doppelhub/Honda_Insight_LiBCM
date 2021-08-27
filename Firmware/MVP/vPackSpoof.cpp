/*The MCM measures pack voltage in three different spots:
/ -MCM'E' connector, which is the actual analog pack voltage (e.g. 170 volts).  LiBCM man-in-the-middles this voltage.
/ -VPIN connector, which is a 0:5 volt analog voltage 52 times less than actual pack voltage. LiBCM man-in-the-middles this voltage.
/ -BATTSCI, which is a serial bus the BCM uses to send data to the MCM.  LiBCM replaces the OEM signal with its own implementation.
/ 
/ The MCM will throw a P-code if all three voltages aren't spoofed to within ~10** volts.
/  **Some comparisons are actually 20 volts, but for simplicity this code treats them all as having to be within 10 volts.
*/

#include "libcm.h"

//---------------------------------------------------------------------------------------

void spoofVoltageMCMe(uint8_t desiredSpoofedVoltage, uint8_t actualPackVoltage)
{
	//Derivation:
	//Empirically determined, see: ~/Electronics/PCB (KiCAD)/RevB/V&V/voltage spoofing results.ods
	      //pwmCounts_MCME = (              actualPackVoltage  * 512) / desiredSpoofedVoltage          - 551
	      //pwmCounts_MCME = (              actualPackVoltage  * 256) / desiredSpoofedVoltage    * 2   - 551 //prevent 16b overflow
	      //pwmCounts_MCME = (    (int16_t)(actualPackVoltage) * 256) / desiredSpoofedVoltage    * 2   - 551 
	int16_t pwmCounts_MCME = ( ( ((int16_t)(actualPackVoltage) << 8 ) / desiredSpoofedVoltage ) << 1 ) - 551;
	  
	//bounds checking
	if     (pwmCounts_MCME > 255) {pwmCounts_MCME = 255;}
	else if(pwmCounts_MCME <   0) {pwmCounts_MCME =   0;}

	analogWrite(PIN_MCME_PWM, (uint8_t)pwmCounts_MCME);
}

//---------------------------------------------------------------------------------------

void vPackSpoof_updateVoltage(uint8_t actualPackVoltage, uint8_t voltageToSpoof)
{
	//spoof VPIN_OUT voltage (to MCM).  VPIN_IN (from PDU) ignored after PDU caps are charged (immediately after keyON)
	//JTS2doNow: Use spoofed voltage as long as actual voltage is within 20 volts of V6804.  This handles keyOFF event.
	analogWrite(PIN_VPIN_OUT_PWM, voltageToSpoof);	
		//Derivation: Vpack (volts) ~= 0:5v PWM 8b value (counts)
		//Example: when pack voltage is 184 volts, send analogWrite(VPIN_OUT, 184)

	//spoof MCM E connector voltage
	spoofVoltageMCMe(voltageToSpoof, actualPackVoltage);

	//spoof BATTSCI voltage		
	BATTSCI_setPackVoltage(voltageToSpoof);

	lcd_printStackVoltage_spoofed(voltageToSpoof);
	lcd_printStackVoltage_actual(actualPackVoltage);
}

//---------------------------------------------------------------------------------------

void vPackSpoof_handleKeyON(void)
{
	uint8_t packVoltage_VPINin;

	LTC6804_startCellVoltageConversion();
	delayMicroseconds(500);
	LTC6804_readCellVoltages(); //getStackVoltage sums cell voltages

	uint8_t actualPackVoltage = LTC6804_getStackVoltage();
	Serial.print("\nV6804=" + String(actualPackVoltage) );

	do
	{
		packVoltage_VPINin = adc_packVoltage_VpinIn(); 
		Serial.print("\nVPIN=" + String(packVoltage_VPINin) );
		analogWrite(PIN_VPIN_OUT_PWM, packVoltage_VPINin);
		blinkLED1();		
	} while( ((packVoltage_VPINin + 18) <= actualPackVoltage ) && digitalRead(PIN_KEY_ON) );
	Serial.print("\nVPIN=V6804");
}

//---------------------------------------------------------------------------------------

void vPackSpoof_handleKeyOFF(void)
{
	pinMode(PIN_VPIN_OUT_PWM,INPUT); //set VPIN back to high impedance
}