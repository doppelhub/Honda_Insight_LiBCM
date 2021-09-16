//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//functions related to ignition (key) status

#include "libcm.h"

uint8_t keyState_sampled  = KEYSTATE_UNINITIALIZED; //updated by key_didStateChange() to prevent mid-loop state changes
uint8_t keyState_previous = KEYSTATE_UNINITIALIZED;

////////////////////////////////////////////////////////////////////////////////////

void key_handleKeyEvent_off(void)
{
	Serial.print(F("OFF"));
    LED(1,LOW);
    BATTSCI_disable(); //Must disable BATTSCI when key is off to prevent backdriving MCM
    METSCI_disable();
    gpio_setFanSpeed_OEM('0');
    gpio_turnCurrentSensor_off();
    LTC6804configure_handleKeyOff();
    lcd_displayOFF();
    vPackSpoof_handleKeyOFF();
    //JTS2doLater: store keyOff time, so LiBCM can turn itself off (e.g. after an hour)
}

////////////////////////////////////////////////////////////////////////////////////

void key_handleKeyEvent_on(void)
{
	Serial.print(F("ON"));
	BATTSCI_enable();
	METSCI_enable();
	gpio_setFanSpeed_OEM('L');
	gpio_turnCurrentSensor_on();
	lcd_displayON();
	LTC68042result_maxEverCellVoltage_set(0    ); //reset maxEver cell voltage
	LTC68042result_minEverCellVoltage_set(65535); //reset minEver cell voltage
	LTC68042configure_cellBalancing_disable();
	LED(1,HIGH);
}

////////////////////////////////////////////////////////////////////////////////////

bool key_didStateChange(void)
{
	bool didKeyStateChange = NO;

	keyState_sampled = gpio_keyStateNow(); //after startup, this is the only time LiBCM samples actual key state

	if( (keyState_sampled == KEYOFF) && (keyState_previous == KEYON) ) 
	{	//key state just changed from 'ON' to 'OFF'.
		//don't immediately handle keyOFF event, in case this is due to noise.
		//if the key is still off the next time thru the loop, then we'll handle keyOFF event
		keyState_previous = KEYOFF_JUSTOCCURRED;
	}
	else if( (keyState_sampled == KEYON) && (keyState_previous == KEYOFF_JUSTOCCURRED) )
	{	//key is now 'ON', but last time was 'OFF', and the time before that it was 'ON'
		//therefore the previous 'OFF' reading was noise... the key was actually ON all along
		//no need to do anything.
		;
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