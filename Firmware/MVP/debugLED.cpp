//Copyright 2021-2023(c) John Sullivan
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

void blinkLED1()
{
    #ifdef LED_NORMAL
        static uint32_t previousMillis = 0;
        if ((uint32_t)(millis() - previousMillis) >= 100)
        {
            previousMillis = millis();  //JTS2doLater: Handle millis() overflow (~50 days)
            digitalWrite(PIN_LED1, !digitalRead(PIN_LED1) );
        }
    #endif  
}

/////////////////////////////////////////////////////////////////////////////////////////

void blinkLED2()
{
    #ifdef LED_NORMAL
        static uint32_t previousMillis = 0;
        if ((uint32_t)(millis() - previousMillis) >= 100)
        {
            previousMillis = millis();
            digitalWrite(PIN_LED2, !digitalRead(PIN_LED2) );
        }
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

void blinkLED3()
{
    #ifdef LED_NORMAL
        static uint32_t previousMillis = 0;
        if ((uint32_t)(millis() - previousMillis) >= 100)
        {
            previousMillis = millis();
            digitalWrite(PIN_LED3, !digitalRead(PIN_LED3) );
        }
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

void blinkLED4()
{
    #ifdef LED_NORMAL
        static uint32_t previousMillis = 0;
        if ((uint32_t)(millis() - previousMillis) >= 100)
        {
            previousMillis = millis();
            digitalWrite(PIN_LED4, !digitalRead(PIN_LED4) );
        }
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////
