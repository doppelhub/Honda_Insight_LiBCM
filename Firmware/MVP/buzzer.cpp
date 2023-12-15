//Copyright 2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//handles buzzer
//LiBCM allows different subsystems to independently control buzzer tone.
//The buzzer will sound at the highest requested tone (OFF/LOW/HIGH)

//JTS2doNow: Buzzer sometimes doesn't work even though we're exciting it.  Come up with routine that guarantees buzzer vibrates

#include "libcm.h"

uint8_t buzzerTone_now  = BUZZER_OFF; //actual buzzer state now (OFF/LOW/HIGH)
uint8_t buzzerTone_goal = BUZZER_OFF; //desired buzzer state (highest tone requested by any subsystem)

uint8_t buzzerTone_allRequestors = BUZZER_FORCE_OFF; //each subsystem's buzzer tone request is stored in 2 bits

////////////////////////////////////////////////////////////////////////////////////

uint8_t buzzer_getTone_now(void) { return buzzerTone_now;  }

////////////////////////////////////////////////////////////////////////////////////

uint8_t buzzer_getAllRequestors_mask(void) { return buzzerTone_allRequestors; }

////////////////////////////////////////////////////////////////////////////////////

void buzzer_requestTone(uint8_t requestor, uint8_t newBuzzerTone)
{
	const uint8_t requestorMask = BUZZER_REQUESTOR_MASK << requestor;

	const uint8_t buzzerToneThisRequestor = (newBuzzerTone << requestor) & requestorMask; //left shift to requestor's position in bit mask

	buzzerTone_allRequestors &= ~(requestorMask); //first, mask out this requestor's previous request
	buzzerTone_allRequestors |= buzzerToneThisRequestor; //apply new request
}

////////////////////////////////////////////////////////////////////////////////////

void determineFastestBuzzerToneRequest(void)
{
	if     (buzzerTone_allRequestors & BUZZER_HI_MASK) { buzzerTone_goal = BUZZER_HIGH; } //at least one subsystem is requesting high tone
	else if(buzzerTone_allRequestors & BUZZER_LO_MASK) { buzzerTone_goal = BUZZER_LOW;  } //at least one subsystem is requesting low tone
	else                                               { buzzerTone_goal = BUZZER_OFF;  } //no subsystem is requesting buzzer
}

////////////////////////////////////////////////////////////////////////////////////

bool hasEnoughTimePassedToChangeBuzzerTone(void)
{
	static uint32_t lastBuzzerToneChange_ms = 0;

	bool hasEnoughTimePassed = NO;

	const uint32_t currentTime_ms = millis();

	if((currentTime_ms - lastBuzzerToneChange_ms) > BUZZER_CHANGE_MIN_DURATION_ms)
	{
		//enough time has passed
		hasEnoughTimePassed = YES;
		lastBuzzerToneChange_ms = currentTime_ms;
	}

	return hasEnoughTimePassed;
}

////////////////////////////////////////////////////////////////////////////////////

void buzzer_handler(void)
{
	determineFastestBuzzerToneRequest(); //result stored in buzzerTone_goal

	//Guard clause to avoid further unnecessary actions
	if( (buzzerTone_now == buzzerTone_goal            ) ||
		(hasEnoughTimePassedToChangeBuzzerTone() == NO)  )
	{ return; }

	buzzerTone_now = buzzerTone_goal;
	switch(buzzerTone_now)
	{
		case BUZZER_HIGH:       gpio_turnBuzzer_on_highFreq(); break;
		case BUZZER_LOW:        gpio_turnBuzzer_on_lowFreq();  break;
		default: /*BUZZER_OFF*/ gpio_turnBuzzer_off();
	}
}