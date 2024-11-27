//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//LiBCM low power functions

//JTS2donow: other ways to save power in sleep mode:
    //disable ADC
    //turn off ADC PRR0 &= ~(1<<PRADC); //there are several other things to turn off in PRR0
    //ideally we'd turn off brownout detector, but this requires fuse changes (which requires external hardware)
    //turn off digital input buffers on analog inputs DIDR2/1/0

#include "libcm.h"

volatile uint8_t interruptSource = USB_INTERRUPT; //see ISR(PCINT1_vect) for more info

/////////////////////////////////////////////////////////////////////////////////////////

//turn LiBCM off if any cell voltage is too low
//LiBCM remains off until the next keyON occurs
//prevents over-discharge during extended keyOFF
void powerSave_turnOffLiBCM_ifPackEmpty(void)
{
    if (LTC68042result_loCellVoltage_get() < CELL_VMIN_GRIDCHARGER)
    {
        Serial.print(F("\nBattery is empty"));
        gpio_turnLiBCM_off(); //game over, thanks for playing
    }
    else if ((LTC68042result_loCellVoltage_get() < CELL_VMIN_KEYOFF) && //battery is low
             (time_hasKeyBeenOffLongEnough_toTurnOffLiBCM() == true) && //give user time to plug in charger
             (gpio_isGridChargerChargingNow() == NO)                  ) //grid charger isn't charging
    {   
        Serial.print(F("\nBattery is low"));
        gpio_turnLiBCM_off(); //game over, thanks for playing
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

bool powerSave_isThermalManagementAllowed(void)
{
    bool enoughEnergy = NO;

    if ((key_getSampledState() == KEYSTATE_ON)                                                  ||
        ((gpio_isGridChargerPluggedInNow() == YES) && (SoC_getBatteryStateNow_percent() > 3))   ||
        (SoC_getBatteryStateNow_percent() > KEYOFF_DISABLE_THERMAL_MANAGEMENT_BELOW_SoC_PERCENT) )
    { enoughEnergy = YES; }

    return enoughEnergy;
}

/////////////////////////////////////////////////////////////////////////////////////////

void wakeupInterrupts_keyOn_enable(void)
{
    PCIFR  |= (1 << PCIF0 ); //clear pending interrupt flag, if set
    PCICR  |= (1 << PCIE0 ); //enable PCINT0..7 pin change interrupt
    PCMSK0 |= (1 << PCINT7); //enable pin D13 state change interrupt //atmega2560 pin PB7
}

void wakeupInterrupts_keyOn_disable(void)
{
    PCMSK0 &= ~(1 << PCINT7); //disable D13 state change interrupt
    PCICR  &= ~(1 << PCIE0 ); //disable PCINT0..7 interrupts, used by D13 to detect keyOn
}

/////////////////////////////////////////////////////////////////////////////////////////

void wakeupInterrupts_USB_enable(void)
{
    PCIFR  |= (1 << PCIF1 ); //clear pending interrupt flag, if set
    PCICR  |= (1 << PCIE1 ); //enable PCINT8..15 pin change interrupt
    PCMSK1 |= (1 << PCINT8); //enable USB RX pin interrupt //atmega2560 pin PE0
}

void wakeupInterrupts_USB_disable(void)
{
    PCMSK1 &= ~(1 << PCINT8); //disable USB Rx interrupt
    PCICR  &= ~(1 << PCIE1 ); //disable PCINT8..15 interrupts, used by USB RX to detect incoming user data
}

/////////////////////////////////////////////////////////////////////////////////////////

void wakeupInterrupts_timer2_enable(void)
{
    power_timer2_enable();  //enable timer2 clock
    TCNT2 = 0; //set timer2 count to zero
    TIFR2  |= (1 << TOV2 ); //clear pending interrupt flag, if set      
    TIMSK2 |= (1 << TOIE2); //enable timer2 overflow interrupt
}

void wakeupInterrupts_timer2_disable(void)
{
    TIMSK2 &= ~(1 << TOIE2); //disable timer2 overflow interrupt
    power_timer2_disable();  //disable timer2 clock
}

/////////////////////////////////////////////////////////////////////////////////////////

void wakeupInterrupts_enable(void)
{
    wakeupInterrupts_keyOn_enable();
    wakeupInterrupts_USB_enable();
    wakeupInterrupts_timer2_enable();
}

/////////////////////////////////////////////////////////////////////////////////////////

void wakeupInterrupts_disable(void)
{
    wakeupInterrupts_keyOn_disable();
    wakeupInterrupts_USB_disable();
    wakeupInterrupts_timer2_disable();
}

/////////////////////////////////////////////////////////////////////////////////////////

void powerSave_init(void)
{
    TCCR2A = 0; //set timer2 to normal mode
    TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20); //set timer2 prescaler to 1024

    ACSR = 0; //disable analog comparator to save power //LiBCM doesn't use

    set_sleep_mode(SLEEP_MODE_PWR_SAVE); //this only configures MCU sleep mode //doesn't actually cause MCU to sleep
}

/////////////////////////////////////////////////////////////////////////////////////////

void powerSave_gotoSleep(void)
{
    interruptSource = USB_INTERRUPT; //see ISR(PCINT1_vect) for more info

    LED_turnAllOff(); //saves power
    
    USB_delayUntilTransmitBufferEmpty();
    USB_end();

    time_addSleepPeriodToMillis(); //millis() counter runs slower when sleeping

    noInterrupts();
    {
        wakeupInterrupts_enable();
        clock_prescale_set(clock_div_64); //64 = 250 kHz
        sleep_enable();
    }
    interrupts();

    sleep_cpu(); //see <avr/sleep.h>

    //CPU halts here until (timer2 overflow, USB data, and/or keyOn) interrupt occurs
    //After CPU wakes up and runs ISR, code resumes from here

    noInterrupts();
    {
        sleep_disable();
        clock_prescale_set(clock_div_1); //16 MHz
        wakeupInterrupts_disable();
    }
    interrupts();

    USB_begin();

    if(interruptSource == USB_INTERRUPT)
    {
        time_latestUserInputUSB_set();
        Serial.print(F("\nRepeat command (LiBCM was asleep)"));
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

void powerSave_sleepIfAllowed(void)
{
    if ((cellBalance_areCellsBalancing()  == NO) /* LiBCM must stay on for safety */                &&
        (gpio_isGridChargerPluggedInNow() == NO) /* LiBCM must stay on for safety */                &&
        (time_sinceLatestUserInputUSB_get_ms() > PERIOD_TO_DISABLE_SLEEP_AFTER_USB_DATA_RECEIVED_ms) )
    {
        powerSave_gotoSleep();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

void powerSave_turnOffIfAllowed(void)
{
    #ifdef POWEROFF_DELAY_AFTER_KEYOFF_DAYS
        uint32_t timeSinceLatestKeyOff_ms = millis() - time_latestKeyOff_ms_get();

        if ((gpio_isGridChargerPluggedInNow() == NO)                                                            &&
            (time_sinceLatestGridChargerUnplug_get_ms() > PERIOD_TO_DISABLE_TURNOFF_AFTER_CHARGER_UNPLUGGED_ms) &&
            (timeSinceLatestKeyOff_ms > (POWEROFF_DELAY_AFTER_KEYOFF_DAYS * MILLISECONDS_PER_DAY))               )
        {
            uint32_t timeSinceLastKeyOff_ms = millis() - time_latestKeyOff_ms_get();
            uint16_t delta_hours = timeSinceLastKeyOff_ms / MILLISECONDS_PER_HOUR;
            eeprom_hoursSinceLastFirmwareUpdate_set(delta_hours + eeprom_hoursSinceLastFirmwareUpdate_get());

            gpio_turnLiBCM_off();
        }
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

//timer2 wakes LiBCM up every ~1049 ms
//actual time is: 256_countsToOverflow/(16MHz/clockDiv64/timer2Div1024)
//this interrupt disabled when LiBCM is awake
ISR(TIMER2_OVF_vect)
{
    interruptSource = TIMER2_INTERRUPT;
    wakeupInterrupts_disable();
}

/////////////////////////////////////////////////////////////////////////////////////////

//key turns on while LiBCM is sleeping
//this interrupt disabled when LiBCM is awake
ISR(PCINT0_vect)
{
    interruptSource = KEYON_INTERRUPT;
    wakeupInterrupts_disable();
}

/////////////////////////////////////////////////////////////////////////////////////////

//user sends data via USB while LiBCM is sleeping
//this interrupt disabled when LiBCM is awake
ISR(PCINT1_vect)
{
    //interruptSource = USB_INTERRUPT; //this code isn't guaranteed to run (see below) 
    //wakeupInterrupts_disable();      //this code isn't guaranteed to run (see below)

    //the hardware pin change interrupt that occurs when USB Rx pin toggles is guaranteed to wake the CPU from sleep.
    //HOWEVER, the code in PCINT1_vect ISR isn't guaranteed to run (and probably won't), as explained next.
    //This was super annoying to figure out, hence the labored explanation:
    //
    //From the atmega2560 manual, Chapter 15 "External interrupts": 
        //"Note that if a level triggered interrupt is used for wake-up from Power-down," ...
        //..."the required level must be held long enough for the MCU to complete the wake-up to trigger the level interrupt."
        //"If the level disappears before the end of the Start-up Time, the MCU will still wake up, but no interrupt will be generated."

    //This hardware interrupt is looking at the USB RX signal, which is only guaranteed to stay low for ~17 us when each transmission starts.
    //After 17 us, user-typed data from the host USB device is sent, which causes the pin to toggle 'unpredictably'.
    //therefore, anything you put in this ISR probably won't actually execute...
    //...unless you get lucky and the RX pin is still receiving data and is low when MCU hardware interrupt handler runs.
    //In my testing, I could always eventually cause this ISR to run by repeatedly spamming the USB bus, but it isn't guaranteed.
    //
    //Fortunately, this is the only wake-from-sleep interrupt that fails to meet the "HW pin still toggled on wakeup" requirements.
    //Since the other two interrupt handlers are guaranteed to executure their ISR code...
    //...we know a USB interrupt occurred if no other ISR handler sets 'interruptSource' (i.e. to KEYON_INTERRUPT or TIMER2_INTERRUPT).

    ; //this interrupt handler doesn't actually do anything, because it isn't guaranteed to run (see above)
}

///////////////////////////////////////////////////////////////////

//keyOff HVDC power consumption with 188 Vpack (current measured with 1kohm inline shunt)
//LiBCM awake (all power saving features disabled) : 470 mW (~2.500 mA)
//LiBCM asleep (pending USB/timer2/keyOn interrupt): 280 mW (~1.500 mA)
//LiBCM off (RevD and previous, after +5V rail off):  21 mW (~0.112 mA)
//LiBCM off (RevE and later, after   HVDC rail off):  ~4 mW (~0.020 mA) //estimates, pending RevE HW (not designed yet)
