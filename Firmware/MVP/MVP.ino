//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM
//Lithium battery BMS for G1 Honda Insight.  Replaces OEM BCM module.

//JTS2doLater: Make this the only line of code in this file:
#include "libcm.h"

uint32_t previousMillis = millis();

void setup() //~t=2 milliseconds, BUT NOTE this doesn't include CPU_CLOCK warmup or bootloader delay 
{
	gpio_begin();
	Serial.begin(115200); //USB
	METSCI_begin();
	BATTSCI_begin();

	LTC68042configure_initialize();

	if( gpio_keyStateNow() == KEYON ){ LED(3,ON); } //turn LED3 on if LiBCM (re)boots while keyON (e.g. while driving)
  	
  	//JTS2doLater: Configure watchdog

	Serial.print(F("\n\nWelcome to LiBCM v" FW_VERSION "," BUILD_DATE "\n\n"));
}

void loop()
{
	key_stateChangeHandler();

	gridCharger_handler();

	if( key_getSampledState() == KEYON )
	{
		if( gpio_isGridChargerPluggedInNow() == PLUGGED_IN ) { lcd_gridChargerWarning(); } //grid charging not allowed when keyON
		else { BATTSCI_sendFrames(); } //BATTSCI data only sent if grid charger unplugged (forces P-code if car tethered)

	 	LTC68042cell_nextVoltages(); //round-robin handler measures QTY3 cell voltages per call

    	METSCI_processLatestFrame();

    	adc_updateBatteryCurrent();

  		vPackSpoof_setVoltage();

		debugUSB_printLatest_data_keyOn();

		lcd_refresh();
	}

	blinkLED2(); //Heartbeat

	LED(4,HIGH); //LED4 brightness proportional to how much CPU time is left //if off, exceeding LOOP_RATE_MS
	while( (millis() - previousMillis) < LOOP_RATE_MS ) { ; } //wait here to start next loop //JTS2doLater: Determine Behavior after overflow (50 days)
	//JTS2doLater: Feed watchdog
	LED(4,LOW);

	previousMillis = millis(); //placed at end to prevent delay at keyON event
}