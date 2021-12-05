//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM
//Lithium battery BMS for G1 Honda Insight.  Replaces OEM BCM module.

//JTS2doLater: Make this the only line of code in this file:
#include "libcm.h"

void setup() //~t=2 milliseconds, BUT NOTE this doesn't include CPU_CLOCK warmup or bootloader delay 
{
	gpio_begin();
	Serial.begin(115200); //USB
	METSCI_begin();
	BATTSCI_begin();
	LiDisplay_begin();

	Serial.print(F("\n\nWelcome to LiBCM_Watching_BCM v" FW_VERSION ", " BUILD_DATE "\n"));
}

void loop()
{
	key_stateChangeHandler();

	if(digitalRead(PIN_COVER_SWITCH) == 1) { Serial.print("\nButton"); }

	BATTSCI_processLatestFrame();

	METSCI_processLatestFrame();

	blinkLED2(); //Heartbeat

	//wait here for next iteration
	{
		static uint32_t previousMillis = millis();

		LED(4,HIGH); //LED4 brightness proportional to how much CPU time is left //if off, exceeding LOOP_RATE_MILLISECONDS
		while( (millis() - previousMillis) < LOOP_RATE_MILLISECONDS ) { ; } //wait here to start next loop //JTS2doLater: Determine Behavior after overflow (50 days)
		//JTS2doLater: Feed watchdog
		LED(4,LOW);
		
		previousMillis = millis(); //placed at end to prevent delay at keyON event
	}
}