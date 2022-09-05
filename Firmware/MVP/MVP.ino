//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM
//Lithium battery BMS for G1 Honda Insight.  Replaces OEM BCM module.

#include "libcm.h"

void setup() //~t=2 milliseconds, BUT NOTE this doesn't include CPU_CLOCK warmup or bootloader delay 
{
	gpio_begin();
	wdt_disable(); //disable watchdog if running previously
	Serial.begin(115200); //USB
	METSCI_begin();
	BATTSCI_begin();
	LiDisplay_begin();
	lcd_begin();

	LTC68042configure_initialize();

	if(gpio_keyStateNow() == KEYSTATE_ON){ LED(3,ON); } //turn LED3 on if LiBCM (re)boots while keyON (e.g. while driving)

	gpio_safetyCoverCheck(); //this function hangs forever if safety cover isn't installed

	#ifdef RUN_BRINGUP_TESTER
	  	bringupTester_run(); //this function never returns
	#endif

	wdt_enable(WDTO_2S); //set watchdog reset vector to 2 seconds

	EEPROM_verifyDataValid();

	debugUSB_printHardwareRevision();

	Serial.print(F("\n\nWelcome to LiBCM v" FW_VERSION ", " BUILD_DATE "\nType '$HELP' for more info\n"));

}

void loop()
{
	key_stateChangeHandler();
	temperature_handler();
	SoC_handler();
	fan_handler();

	if( key_getSampledState() == KEYSTATE_ON )
	{
		if( gpio_isGridChargerPluggedInNow() == PLUGGED_IN ) { lcd_Warning_gridCharger(); } //P1648 occurs if grid charger powered while keyON
		else if( EEPROM_firmwareStatus_get() != FIRMWARE_STATUS_EXPIRED ) { BATTSCI_sendFrames(); } //P1648 occurs if firmware is expired

		LTC68042cell_nextVoltages(); //round-robin handler measures QTY3 cell voltages per call

		METSCI_processLatestFrame();

		adc_updateBatteryCurrent();

		vPackSpoof_setVoltage();

		debugUSB_printLatestData_keyOn();

		lcd_refresh();
	}
	else if( key_getSampledState() == KEYSTATE_OFF )
	{	
		if( time_toUpdate_keyOffValues() == true )
		{ 
			LTC68042cell_sampleGatherAndProcessAllCellVoltages();
			
			SoC_updateUsingLatestOpenCircuitVoltage();

			SoC_turnOffLiBCM_ifPackEmpty();

			cellBalance_handler();
			
			debugUSB_printLatest_data_gridCharger();
		}

		gridCharger_handler();
	}

	USB_userInterface_handler();

	wdt_reset(); //Feed watchdog

	blinkLED2(); //Heartbeat

	time_waitForLoopPeriod(); //wait here until next iteration
}