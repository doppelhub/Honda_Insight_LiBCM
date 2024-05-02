//Copyright 2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//handles buzzer
//LiBCM allows different subsystems to independently control buzzer tone.
//The buzzer will sound at the highest requested tone (OFF/LOW/HIGH)

//JTS2doLater: Buzzer sometimes doesn't work even though we're exciting it.  Come up with routine that guarantees buzzer vibrates

#include "libcm.h"

uint8_t buzzerTone_allRequestors = BUZZER_FORCE_OFF; //each subsystem's buzzer tone request is stored in 2 bits

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t buzzer_getAllRequestors_mask(void) { return buzzerTone_allRequestors; }

/////////////////////////////////////////////////////////////////////////////////////////

void buzzer_requestTone(uint8_t requestor, uint8_t newBuzzerTone)
{
    const uint8_t requestorMask = BUZZER_REQUESTOR_MASK << requestor;

    const uint8_t buzzerToneThisRequestor = (newBuzzerTone << requestor) & requestorMask; //left shift to requestor's position in bit mask

    buzzerTone_allRequestors &= ~(requestorMask); //first, mask out this requestor's previous request
    buzzerTone_allRequestors |= buzzerToneThisRequestor; //apply new request
}

/////////////////////////////////////////////////////////////////////////////////////////

void buzzer_handler(void)
{
    if      (buzzerTone_allRequestors & BUZZER_HI_MASK) { gpio_turnBuzzer_on_highFreq(); } //subsystem(s) requesting high tone
    else if (buzzerTone_allRequestors & BUZZER_LO_MASK) { gpio_turnBuzzer_on_lowFreq();  } //subsystem(s) requesting low tone
    else                                                { gpio_turnBuzzer_off();         } //no subsystem requesting buzzer
}

/////////////////////////////////////////////////////////////////////////////////////////
