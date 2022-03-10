//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//functions related to ignition (key) status

#include "libcm.h"

uint8_t keyState_sampled  = KEYSTATE_UNINITIALIZED; //updated by key_didStateChange() to prevent mid-loop state changes
uint8_t keyState_previous = KEYSTATE_UNINITIALIZED;
uint32_t key_lastTimeTurnedOn_ms = 0;
uint32_t key_lastTimeTurnedOff_ms = 0;

////////////////////////////////////////////////////////////////////////////////////

void     key_latestTurnOnTime_ms_set(uint32_t keyOnTime) { key_lastTimeTurnedOn_ms = keyOnTime; }
uint32_t key_latestTurnOnTime_ms_get(void)               { return key_lastTimeTurnedOn_ms;      }

void key_latestTurnOffTime_ms_set(uint32_t keyOffTime) { key_lastTimeTurnedOff_ms = keyOffTime; }
uint32_t key_latestTurnOffTime_ms_get(void)            { return key_lastTimeTurnedOff_ms;       }

////////////////////////////////////////////////////////////////////////////////////

void key_handleKeyEvent_off(void)
{
	Serial.print(F("OFF"));
    LED(1,LOW);
    BATTSCI_disable(); //Must disable BATTSCI when key is off to prevent backdriving MCM
    METSCI_disable();
    gpio_setFanSpeed_OEM('0');
    SoC_updateUsingOpenCircuitVoltage(); //JTS2doNow: Is this required?  I believe the goal is to make sure some SoC value exists when LiBCM first powers on.  See also: SoC_keyOff_cellMeasurement_handler()
    adc_calibrateBatteryCurrentSensorOffset();
    gpio_turnPowerSensors_off();
    LTC68042configure_handleKeyStateChange();
    vPackSpoof_handleKeyOFF();
    gpio_turnHMI_off();
    gpio_turnTemperatureSensors_off();
    EEPROM_checkForExpiredFirmware(); //must occur before lcd_displayOFF()
    lcd_displayOFF();

    key_latestTurnOffTime_ms_set(millis()); //MUST RUN LAST!   
}

////////////////////////////////////////////////////////////////////////////////////

void key_handleKeyEvent_on(void)
{
	Serial.print(F("ON"));
	BATTSCI_enable();
	METSCI_enable();
	gpio_turnTemperatureSensors_on();
	gpio_turnHMI_on();
	gpio_turnPowerSensors_on();
	lcd_displayOn();
	gpio_setFanSpeed_OEM('L');
	gpio_turnGridCharger_off();
	LTC68042configure_programVolatileDefaults(); //turn discharge resistors off, set ADC LPF, etc.
	//cellBalance_disableBalanceResistors(); //not required //handled by LTC68042configure_programVolatileDefaults()
	LTC68042configure_handleKeyStateChange();
	LED(1,HIGH);

	key_latestTurnOnTime_ms_set(millis()); //MUST RUN LAST!
}

////////////////////////////////////////////////////////////////////////////////////

bool key_didStateChange(void)
{
	bool didKeyStateChange = NO;

	keyState_sampled = gpio_keyStateNow();

	if( (keyState_sampled == KEYOFF) && ((keyState_previous == KEYON) || (keyState_previous == KEYSTATE_UNINITIALIZED)) )
	{	//key state just changed from 'ON' to 'OFF'.
		//don't immediately handle keyOFF event, in case this is due to noise.
		//if the key is still off the next time thru the loop, then we'll handle keyOFF event
		keyState_previous = KEYOFF_JUSTOCCURRED;
	}
	else if( (keyState_sampled == KEYON) && (keyState_previous == KEYOFF_JUSTOCCURRED) )
	{	//key is now 'ON', but last time was 'OFF', and the time before that it was 'ON'
		//therefore the previous 'OFF' reading was noise... the key was actually ON all along
		keyState_previous = KEYON;
	}
	else if(keyState_sampled != keyState_previous)
	{
		didKeyStateChange = YES;
		keyState_previous = keyState_sampled;
	}
	return didKeyStateChange;
}

////////////////////////////////////////////////////////////////////////////////////

void key_stateChangeHandler(void)
{
	if( key_didStateChange() == YES )
	{
		Serial.print(F("\nKey:"));
		if( keyState_sampled == KEYON ) { key_handleKeyEvent_on() ; }
		if( keyState_sampled == KEYOFF) { key_handleKeyEvent_off(); }
	}
}

////////////////////////////////////////////////////////////////////////////////////

//only called outside this file
uint8_t key_getSampledState(void)
{
	if(keyState_previous == KEYOFF_JUSTOCCURRED) { return KEYON;            } //prevent noise from accidentally turning LiBCM off
	else                                         { return keyState_sampled; }
}

////////////////////////////////////////////////////////////////////////////////////

bool key_hasKeyOff_updatePeriodElapsed(void)
{
	static uint32_t latestUpdate_milliseconds = 0;
	uint32_t updatePeriod_milliseconds = 0;

	if( gpio_isGridChargerPluggedInNow() == true ) { updatePeriod_milliseconds =   1000; }
	else                                           { updatePeriod_milliseconds = 600000; } //600000 ms = 10 minutes

	if( ( (millis() - latestUpdate_milliseconds     ) >= updatePeriod_milliseconds ) && //time has elapsed since last measurement
		( (millis() - key_latestTurnOffTime_ms_get()) >= updatePeriod_milliseconds )  ) //time has elapsed since last key off
	{
		latestUpdate_milliseconds = millis();
		return true;
	}
	
	return false;
}