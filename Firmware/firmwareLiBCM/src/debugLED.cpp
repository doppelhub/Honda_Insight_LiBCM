//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//debug stuff

#include "libcm.h"

void debugLED(uint8_t LED_number, bool illuminated)
{
    #ifdef LED_DEBUG
        switch (LED_number)
        {
            case 1: digitalWrite(PIN_LED1,illuminated); break;
            case 2: digitalWrite(PIN_LED2,illuminated); break;
            case 3: digitalWrite(PIN_LED3,illuminated); break;
            case 4: digitalWrite(PIN_LED4,illuminated); break;
        }
    #else
        LED_number  +=0; //prevent "unused parameter" compiler warning
        illuminated +=0; //prevent "unused parameter" compiler warning
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

void LED(uint8_t LED_number, bool illuminated)
{
    #ifdef LED_NORMAL
        switch (LED_number)
        {
            case 1: digitalWrite(PIN_LED1,illuminated); break;
            case 2: digitalWrite(PIN_LED2,illuminated); break;
            case 3: digitalWrite(PIN_LED3,illuminated); break;
            case 4: digitalWrite(PIN_LED4,illuminated); break;
        }
    #else
        LED_number  +=0; //prevent "unused parameter" compiler warning
        illuminated +=0; //prevent "unused parameter" compiler warning
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

void LED_heartbeat(void)
{
    #ifdef LED_NORMAL
        static uint32_t lastTimeTurnedOff_millis = 0;

        if (digitalRead(PIN_LED2) == HIGH)
        {
            digitalWrite(PIN_LED2, LOW); //turn LED off
            lastTimeTurnedOff_millis = millis();
        }
        else if (millis() - lastTimeTurnedOff_millis > 1000)
        {
            digitalWrite(PIN_LED2, HIGH);
        }
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

void LED_turnAllOff(void)
{
    LED(1,OFF);
    LED(2,OFF);
    LED(3,OFF);
    LED(4,OFF);
}

/////////////////////////////////////////////////////////////////////////////////////////
