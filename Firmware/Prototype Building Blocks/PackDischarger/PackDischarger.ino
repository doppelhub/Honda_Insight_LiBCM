//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM
//Lithium battery BMS for G1 Honda Insight.  Replaces OEM BCM module.

#include "libcm.h"

void setup() //~t=2 milliseconds, BUT NOTE this doesn't include CPU_CLOCK warmup or bootloader delay 
{
	gpio_begin();
	wdt_disable(); //disable watchdog if running previously
	Serial.begin(115200); //USB
	lcd_begin();

	LTC68042configure_initialize();

	wdt_enable(WDTO_2S); //set watchdog reset vector to 2 seconds

	Serial.print(F("\n\nWelcome to Pack Discharger v" FW_VERSION ", " BUILD_DATE "\nType '$HELP' for more info\n"));

	adc_calibrateBatteryCurrentSensorOffset();

	gpio_setFanSpeed_PCB('H');
	gpio_turnPowerSensors_on();

	delay(200);

	gpio_setGridCharger_powerLevel('0'); //turn heater on (negative logic)

	lcd_displayOn();
	lcd_printStaticText();
}

void loop()
{
	SoC_handler();
	
	LTC68042cell_sampleGatherAndProcessAllCellVoltages();
	
	SoC_updateUsingLatestOpenCircuitVoltage();
	
	debugUSB_printLatest_data_gridCharger();

	USB_userInterface_handler();

	lcd_refresh();

	wdt_reset(); //Feed watchdog

	blinkLED2(); //Heartbeat

	if(LTC68042result_loCellVoltage_get() < 35200) //~30% SoC after discharging stops
	{
		gpio_setFanSpeed_PCB('0'); //
		gpio_setGridCharger_powerLevel('H'); //turn heater off (negative logic)
	}	

	time_waitForLoopPeriod(); //wait here until next iteration
}