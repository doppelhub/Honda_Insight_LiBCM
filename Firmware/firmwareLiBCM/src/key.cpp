//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//functions related to ignition (key) status

//JTS2doLater: If key is 'ON' but the engine isn't running for more than an hour, LiBCM needs to beep/CEL/etc,
//to prevent over-discharging IMA battery.  @Thibble's IMA battery was pulled too low due to stuck-on 12V_Ignition

#include "libcm.h"

uint8_t keyState_sampled  = KEYSTATE_UNINITIALIZED; //updated by key_didStateChange() to prevent mid-loop state changes
uint8_t keyState_previous = KEYSTATE_UNINITIALIZED;

/////////////////////////////////////////////////////////////////////////////////////////

void key_handleKeyEvent_off(void)
{
    Serial.print(F("OFF"));
    LED(1,OFF);
    LED(3,OFF);
    BATTSCI_disable(); //Must disable BATTSCI when key is off to prevent backdriving MCM
    METSCI_disable();
    LTC68042cell_acquireAllCellVoltages();
    SoC_updateUsingLatestOpenCircuitVoltage(); //JTS2doLater: Add ten minute delay before VoC->SoC LUT
    adc_calibrateBatteryCurrentSensorOffset(DEBUG_TEXT_ENABLED);
    gpio_turnPowerSensors_off();
    LTC68042configure_handleKeyStateChange();
    vPackSpoof_handleKeyOFF();
    //JTS2doLater: Add built-in test suite, including VREF, VCELL, Balancing, temp verify (batt and OEM), etc.
    eeprom_keyOffCheckForExpiredFirmware();
    LTC68042configure_doesActualPackSizeMatchUserConfig();

    time_latestKeyOff_ms_set(millis()); //MUST RUN LAST!
}

/////////////////////////////////////////////////////////////////////////////////////////

void key_handleKeyEvent_on(void)
{
    delay( eeprom_delayKeyON_ms_get() ); //this is a test tool to verify LiBCM is turning on fast enough to prevent P-code //JTS2doLater: Delete
    Serial.print(F("ON"));
    BATTSCI_enable();
    METSCI_enable();
    gpio_turnPowerSensors_on();
    LTC68042configure_programVolatileDefaults(); //turn discharge resistors off, set ADC LPF, etc.
    LTC68042configure_handleKeyStateChange();
    vPackSpoof_handleKeyON();
    LED(1,HIGH);

    time_latestKeyOn_ms_set(millis()); //MUST RUN LAST!
}

/////////////////////////////////////////////////////////////////////////////////////////

//JTS2doLater: make 'KEYSTATE_OFF_JUSTOCCURRED' persist for one second (before reporting 'keyOff')
bool key_didStateChange(void)
{
    bool didKeyStateChange = NO;

    //JTS2doLater: move to handler
    if (gpio_keyStateNow() == GPIO_KEY_ON) { keyState_sampled = KEYSTATE_ON; }
    else                                   { keyState_sampled = KEYSTATE_OFF; }

    if ((keyState_sampled == KEYSTATE_OFF)                                                   &&
        ((keyState_previous == KEYSTATE_ON) || (keyState_previous == KEYSTATE_UNINITIALIZED)) )
    {   //key state just changed from 'ON' to 'OFF'.
        //don't immediately handle keyOFF event, in case this is due to noise.
        //if the key is still off the next time thru the loop, then we'll handle keyOFF event
        keyState_previous = KEYSTATE_OFF_JUSTOCCURRED;
    }
    else if ( (keyState_sampled == KEYSTATE_ON) && (keyState_previous == KEYSTATE_OFF_JUSTOCCURRED) )
    {   //key is now 'ON', but last time was 'OFF', and the time before that it was 'ON'
        //therefore the previous 'OFF' reading was noise... the key was actually ON all along
        keyState_previous = KEYSTATE_ON;
    }
    else if (keyState_sampled != keyState_previous)
    {
        didKeyStateChange = YES;
        keyState_previous = keyState_sampled;
    }

    return didKeyStateChange;
}

/////////////////////////////////////////////////////////////////////////////////////////

void key_stateChangeHandler(void)
{
    if ( key_didStateChange() == YES )
    {
        Serial.print(F("\nKey:"));
        if ( keyState_sampled == KEYSTATE_ON ) { key_handleKeyEvent_on() ; }
        if ( keyState_sampled == KEYSTATE_OFF) { key_handleKeyEvent_off(); }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

//only called outside this file
uint8_t key_getSampledState(void)
{
    if (keyState_previous == KEYSTATE_OFF_JUSTOCCURRED) { return KEYSTATE_ON;      } //prevent signal noise from accidentally turning LiBCM off
    else                                                { return keyState_sampled; }
}

/////////////////////////////////////////////////////////////////////////////////////////

void keyOn_coldBootTasks(void)
{
    //initialize hardware
    gpio_turnPowerSensors_on();
    LTC68042configure_pulseChipSelectLow(SPECIFIED_MAX_WAKEUP_TIME_LTCCORE_MICROSECONDS); //wake LTC6804
    LTC68042cell_nextVoltages(); //first call starts LTC6804 conversion
    uint32_t timeSinceLTC6804conversionStarted_us = millis();

    //other startup initialization tasks
    vPackSpoof_handleKeyON();
    keyState_previous = KEYSTATE_ON; //prevent key_handleKeyEvent_on() from repeating many of these tasks
    METSCI_enable();
    LED(3,ON);

    //process cell voltages
    while(millis() - timeSinceLTC6804conversionStarted_us < LTC6804_MAX_CONVERSION_TIME_ms) { ; } //wait for conversion to finish
    while(LTC68042cell_nextVoltages() != CELL_DATA_PROCESSED) { ; } //read all cell voltages back
    vPackSpoof_setVoltage();
    SoC_setBatteryStateNow_percent(SoC_estimateFromRestingCellVoltage_percent());
    BATTSCI_enable(); //must occur after we have valid Vcell data
    adc_calibrateBatteryCurrentSensorOffset(DEBUG_TEXT_DISABLED); //current sensor settles almost immediately
}

/////////////////////////////////////////////////////////////////////////////////////////
