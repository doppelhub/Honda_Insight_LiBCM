//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

/*The MCM measures pack voltage in three different spots:
/ -MCM'E' connector, which is the actual analog pack voltage (e.g. 170 volts).  LiBCM man-in-the-middles this voltage.
/ -VPIN connector, which is a 0:5 volt analog voltage 52 times less than actual pack voltage. LiBCM man-in-the-middles this voltage.
/ -BATTSCI, which is a serial bus the BCM uses to send data to the MCM.  LiBCM replaces the OEM signal with its own implementation.
/
/ The MCM will throw a P-code if all three voltages aren't spoofed to within ~10** volts.
/  **Some comparisons are actually 20 volts, but for simplicity this code treats them all as having to be within 10 volts.
*/

#include "libcm.h"

uint8_t spoofedPackVoltage = 0;

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t vPackSpoof_getSpoofedPackVoltage(void) { return spoofedPackVoltage; }

/////////////////////////////////////////////////////////////////////////////////////////

void vPackSpoof_handleKeyON(void)
{
    analogWrite(PIN_VPIN_OUT_PWM, 0);
    analogWrite(PIN_MCME_PWM    , 0);
}

/////////////////////////////////////////////////////////////////////////////////////////

void vPackSpoof_handleKeyOFF(void)
{
    pinMode(PIN_VPIN_OUT_PWM, INPUT); //set VPIN high impedance (to save power)
    pinMode(PIN_MCME_PWM,     INPUT); //Set MCMe high impedance (to save power)
}

/////////////////////////////////////////////////////////////////////////////////////////

void spoofVoltageMCMe(void)
{
    int16_t pwmCounts_MCMe = 0;

    uint8_t spoofedPackVoltage_MCMe = spoofedPackVoltage + ADDITIONAL_MCMe_OFFSET_VOLTS;
    
    //Derivation, empirically determined (see: ~/Electronics/PCB (KiCAD)/RevB/V&V/voltage spoofing results.ods)
    //pwmCounts_MCMe = (               actualPackVoltage                 * 512) / spoofedPackVoltage_MCMe         - 551
    //pwmCounts_MCMe = (               actualPackVoltage                 * 256) / spoofedPackVoltage_MCMe   * 2   - 551 //prevent 16b overflow
    //pwmCounts_MCMe = (( ( ((uint16_t)actualPackVoltage )               * 256) / spoofedPackVoltage_MCMe)  * 2 ) - 551 
      pwmCounts_MCMe = (( ( ((uint16_t)LTC68042result_packVoltage_get()) << 8 ) / spoofedPackVoltage_MCMe) << 1 ) - 551;

    //bounds checking
    if      (pwmCounts_MCMe > 255) {pwmCounts_MCMe = 255;}
    else if (pwmCounts_MCMe <   0) {pwmCounts_MCMe =   0;}

    analogWrite(PIN_MCME_PWM, (uint8_t)pwmCounts_MCMe);
}

/////////////////////////////////////////////////////////////////////////////////////////

void spoofVoltage_VPINout(void)
{
    int16_t pwmCounts_VPIN_out = 0;

    //      V_DIV_CORRECTION = RESISTANCE_MCM / RESISTANCE_R34
    //      V_DIV_CORRECTION = 100k           / 10k
    #define V_DIV_CORRECTION 1.1

    uint8_t spoofedPackVoltage_VPIN = spoofedPackVoltage + ADDITIONAL_VPIN_OFFSET_VOLTS;

    //remap measured Vpin_in value ratiometrically to desired spoofed voltage
    //It's important to look at VPIN_in, since V_PDU is different from the Vpack during keyON capacitor charging event
    uint16_t intermediateMath = (uint16_t)(adc_packVoltage_VpinIn() * spoofedPackVoltage_VPIN) * V_DIV_CORRECTION;
    pwmCounts_VPIN_out = (int16_t)( (uint16_t)intermediateMath / LTC68042result_packVoltage_get() );

    //bounds checking
    if      (pwmCounts_VPIN_out > 255) {pwmCounts_VPIN_out = 255;}
    else if (pwmCounts_VPIN_out <  26) {pwmCounts_VPIN_out =  26;} //MCM ADC rolls under if VPIN is less than 0.5 volts (0.5/5*256 = 25.6 ~= 26)

    analogWrite(PIN_VPIN_OUT_PWM, (uint8_t)pwmCounts_VPIN_out);
}

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t calculate_Vspoof_maxPossible(void)
{
    //Hardware limitations prevent us from spoofing the entire voltage range
    //The max allowed voltage is a function of the actual pack voltage
    //Derivation: ~/Electronics/PCB (KiCAD)/RevD/V&V/VPIN-MCMe Calibration.ods

    uint8_t actualPackVoltage = LTC68042result_packVoltage_get();
    uint8_t maxAllowedVspoof = 0;

    if      (actualPackVoltage < 109) { maxAllowedVspoof = actualPackVoltage -  6; }
    else if (actualPackVoltage < 119) { maxAllowedVspoof = actualPackVoltage -  7; }
    else if (actualPackVoltage < 128) { maxAllowedVspoof = actualPackVoltage -  8; }
    else if (actualPackVoltage < 138) { maxAllowedVspoof = actualPackVoltage -  9; }
    else if (actualPackVoltage < 148) { maxAllowedVspoof = actualPackVoltage - 10; }
    else if (actualPackVoltage < 158) { maxAllowedVspoof = actualPackVoltage - 11; }
    else if (actualPackVoltage < 167) { maxAllowedVspoof = actualPackVoltage - 12; }
    else if (actualPackVoltage < 177) { maxAllowedVspoof = actualPackVoltage - 13; }
    else if (actualPackVoltage < 187) { maxAllowedVspoof = actualPackVoltage - 14; }
    else if (actualPackVoltage < 197) { maxAllowedVspoof = actualPackVoltage - 15; }
    else if (actualPackVoltage < 206) { maxAllowedVspoof = actualPackVoltage - 16; }
    else if (actualPackVoltage < 216) { maxAllowedVspoof = actualPackVoltage - 17; }
    else if (actualPackVoltage < 226) { maxAllowedVspoof = actualPackVoltage - 18; }
    else if (actualPackVoltage < 236) { maxAllowedVspoof = actualPackVoltage - 19; }
    else if (actualPackVoltage < 245) { maxAllowedVspoof = actualPackVoltage - 20; }
    else                              { maxAllowedVspoof = actualPackVoltage - 21; }

    return maxAllowedVspoof;
}

/////////////////////////////////////////////////////////////////////////////////////////

uint8_t calculate_vspoofMCM_max(void)
{
    //Keep the MCM Happy by Spoofing no higher than 184 Volts for 60s.
    //When maxAllowedVspoof is less than 184 volts, use maxAllowedVspoof instead.
    
    uint8_t maxAllowedVspoof = calculate_Vspoof_maxPossible();
    uint8_t vspoofMCM_max = 0;

    if      (maxAllowedVspoof < 184) { vspoofMCM_max = maxAllowedVspoof; }
    else                              { vspoofMCM_max = 184; }

    return vspoofMCM_max;
}

/////////////////////////////////////////////////////////////////////////////////////////

void spoofVoltage_calculateValue(void)
{
    uint8_t maxPossibleVspoof = calculate_Vspoof_maxPossible();

    #if defined VOLTAGE_SPOOFING_DISABLE
        //For those that don't want variable voltage spoofing, spoof maximum possible pack voltage
        
        #ifdef STACK_IS_48S
            //48S pack voltage range is close enough to OEM that we can pass just pass the actual maxPossibleVspoof value to the MCM
            spoofedPackVoltage = maxPossibleVspoof;

        #elif defined STACK_IS_60S
            spoofedPackVoltage = LTC68042result_packVoltage_get() * 0.67; //Vspoof(60S)=136 @ Vcell=3.4 //Vspoof(60S)=169 @ Vcell=4.2
            if (spoofedPackVoltage < MIN_SPOOFED_VOLTAGE_60S) { spoofedPackVoltage = MIN_SPOOFED_VOLTAGE_60S; } //prevent P1440 during heavy assist (due to MCM increasing current as voltage drops) //JTS2doLater: Automate this process (e.g. limit output power to 23 kW) 
        #endif

    //JTS2doLater: Add 60S logic to all other modes (below)

    //---------------------------------------------------------------------------

    #elif defined VOLTAGE_SPOOFING_ASSIST_ONLY_BINARY
        #ifdef STACK_IS_60S
            #error (60S only works with VOLTAGE_SPOOFING_DISABLE selected in config.h)
        #endif
        if (adc_getLatestBatteryCurrent_amps() > MAXIMIZE_POWER_ABOVE_CURRENT_AMPS) { spoofedPackVoltage = VSPOOF_TO_MAXIMIZE_POWER; } //more power during heavy assist
        else { spoofedPackVoltage = maxPossibleVspoof; } //no voltage spoofing during regen, idle, or light assist

    //---------------------------------------------------------------------------

    //JTS2doLater: Remove this mode before exiting beta
    //DEPRECATED: This mode is no longer updated and may not work in future firmware versions
    //Based on test data, spoofing pack voltage during regen causes erratic and/or heavy regen behavior.
    //Recommendation: use VOLTAGE_SPOOFING_ASSIST_ONLY_VARIABLE

    #elif defined VOLTAGE_SPOOFING_ASSIST_AND_REGEN
        #ifdef STACK_IS_60S
            #error (60S only works with VOLTAGE_SPOOFING_DISABLE selected in config.h)
        #endif
        //Derivation:
        //Maximum assist occurs when MCM thinks pack is at 120 volts.
        //Therefore, we want to adjust the pack voltage over that range:
        //vAdjustRange_mV = (maxPossibleVspoof - 120) * 1000

        //Since there's ~2x more assist current than regen current, set "0 A" pack voltage to 2/3 the above limits:
        //vPackTwoThirdPoint_mV = vAdjustRange_mV * 2 / 3 + 120,000

        //Next we linearize the (constant) maximum possible assist+regen current:
        //TOTAL_CURRENT_RANGE_A = 140 A + 75 A  //215 A

        //We then calculate the voltage adjustment per amp, across the (variable) spoofed voltage range:
        //voltageAdjustment_mV_per_A = vAdjustRange_mV / TOTAL_CURRENT_RANGE_A

        //Putting these equations together, we determine the correct pack voltage to spoof
        //for any given actual pack voltage at any given current:
        //spoofedVoltage_mV = vPackTwoThirdPoint_mV - actualCurrent_A * voltageAdjustment_mV_per_A

        //Putting it all together, plug the equations above into the last equation:   
        //spoofedVoltage_mV = vAdjustRange_mV             * 2 / 3 + 120,000   - actualCurrent_A * vAdjustRange_mV / TOTAL_CURRENT_RANGE_A
        //spoofedVoltage_mV = ((vPackActual_mV - 132,000) * 2 / 3 + 120,000 ) - actualCurrent_A * ((vPackActual_mV - 132,000) / 215 )
        //spoofedVoltage_mV = vPackActual_mV            * ( 2 / 3 - actualCurrent_A / 215 ) + 614 * actualCurrent_A + 32,000
        //spoofedVoltage_V = (vPackActual_mV * (667 - actualCurrent_A / 256 * 1000 )/1000 + 614 * actualCurrent_A + 32,000) / 1000
        //spoofedVoltage_V = (vPackActual_V * (667 - actualCurrent_A / 256 * 1000 )/1 + 614 * actualCurrent_A + 32,000) / 1000

        //approximate:
        //spoofedVoltage_V = (vPackActual_V * (667 - actualCurrent_A / 256 * 1024 )/1 + 614 * actualCurrent_A + 32,000) / 1024
        //spoofedVoltage_V = (vPackActual_V * (667 - actualCurrent_A >> 8 << 10) + 614 * actualCurrent_A + 32,000) >> 10
        //spoofedVoltage_V = (vPackActual_V * (667 - actualCurrent_A << 2 ) + 614 * actualCurrent_A + 32,000) >> 10

        //prevent uint16_t overflow:
        //spoofedVoltage_V = (vPackActual_V * (667 - actualCurrent_A << 2 ) / 4 + ( 614 * actualCurrent_A + 32,000) / 4) >> 8
        //spoofedVoltage_V = ((vPackActual_V * (667 - actualCurrent_A << 2 ) >> 2 ) + 154 * actualCurrent_A + 8,000) >> 8
        //spoofedVoltage_V = ((vPackActual_V * (167 - actualCurrent_A) + 154 * actualCurrent_A + 8,000) >> 8

        //But our rounding lowered the gain more than we wanted, so fudge the number a bit:
        //spoofedVoltage_V = ((vPackActual_V * (167 - actualCurrent_A) + 135 * actualCurrent_A + 8,000) >> 8

        spoofedPackVoltage = (uint8_t)((uint16_t)(LTC68042result_packVoltage_get() * ( 167 - adc_getLatestBatteryCurrent_amps() )
                                + 135 * adc_getLatestBatteryCurrent_amps() + 8000) >> 8);

    //---------------------------------------------------------------------------

    #elif defined   VOLTAGE_SPOOFING_ASSIST_ONLY_VARIABLE
        #ifdef STACK_IS_60S
            #error (60S only works with VOLTAGE_SPOOFING_DISABLE selected in config.h)
        #endif
        
        if     ((maxPossibleVspoof < VSPOOF_TO_MAXIMIZE_POWER)                           || //pack voltage too low     
                (adc_getLatestBatteryCurrent_amps() < BEGIN_SPOOFING_VOLTAGE_ABOVE_AMPS)  ) { spoofedPackVoltage = maxPossibleVspoof; } //regen, idle, or light assist
        else if (adc_getLatestBatteryCurrent_amps() > MAXIMIZE_POWER_ABOVE_CURRENT_AMPS)    { spoofedPackVoltage = VSPOOF_TO_MAXIMIZE_POWER; } //heavy assist
        else
        {
            //medium assist
            //decrease spoofedPackVoltage inversely proportional to assist current

            uint8_t vAdjustRange_V = maxPossibleVspoof - VSPOOF_TO_MAXIMIZE_POWER;

            //Calculate voltage adjustment per amp assist
            //       voltageAdjustment_mV_per_A =            vAdjustRange_V   * 1000  / ADDITIONAL_AMPS_UNTIL_MAX_VSPOOF;
            //       voltageAdjustment_mV_per_A =            vAdjustRange_V   * 1024  / ADDITIONAL_AMPS_UNTIL_MAX_VSPOOF; //change multiply to 2^n (2.4% error)
            //       voltageAdjustment_mV_per_A =            vAdjustRange_V  <<   10 >> ADDITIONAL_AMPS__2_TO_THE_N     ; //substitute mult/div with bit shifts
            //       voltageAdjustment_mV_per_A =            vAdjustRange_V  <<  (10  - ADDITIONAL_AMPS__2_TO_THE_N)    ; //combine bit shifts
            uint16_t voltageAdjustment_mV_per_A = ((uint16_t)vAdjustRange_V) <<  (10  - ADDITIONAL_AMPS__2_TO_THE_N)    ; //cast intermediate math
            //max counts: 3712 (when 60S pack is 4.3 V/cell)
            //min counts:    0 (when 48S pack is 2.7 V/cell) 

            //Calculate how much to reduce actual pack voltage at any current
            //      packVoltageReduction_mV =  (actualCurrent_A                    - BEGIN_SPOOFING_VOLTAGE_ABOVE_AMPS) * voltageAdjustment_mV_per_A         ;
            //      packVoltageReduction_V  =  (actualCurrent_A                    - BEGIN_SPOOFING_VOLTAGE_ABOVE_AMPS) * voltageAdjustment_mV_per_A  * 0.001; //change mV to V
            uint8_t packVoltageReduction_V  = ((adc_getLatestBatteryCurrent_amps() - BEGIN_SPOOFING_VOLTAGE_ABOVE_AMPS) * voltageAdjustment_mV_per_A) * 0.001;

            //Calculate spoofed pack voltage
            spoofedPackVoltage = maxPossibleVspoof - packVoltageReduction_V;
        }

    //---------------------------------------------------------------------------

	#elif defined   VOLTAGE_SPOOFING_VARIABLE_60S_BULL_DOG
            #ifdef STACK_IS_48S
            #error (VOLTAGE_SPOOFING_VARIABLE_60S_BULL_DOG only works with 60S config.)
        #endif

        uint8_t vspoofMCM_max = calculate_vspoofMCM_max();
        uint8_t initialSpoofedPackVoltage = 0;

        if     ((maxPossibleVspoof < DISABLE_60S_VSPOOF_VOLTAGE)                           || //pack voltage too low
                (vspoofMCM_max > maxPossibleVspoof)) { spoofedPackVoltage = maxPossibleVspoof; }//If the voltage we want to spoof is greater than maxPossibleVspoof, use maxPossibleVspoof instead.

        else if (adc_getLatestBatteryCurrent_amps() < BEGIN_SPOOFING_VOLTAGE_ABOVE_AMPS)    { spoofedPackVoltage = vspoofMCM_max; } //regen, idle, or light assist
        
		//Assist Spoofing
        else if (adc_getLatestBatteryCurrent_amps() > MAXIMIZE_POWER_ABOVE_CURRENT_AMPS)    
        { 
            initialSpoofedPackVoltage = LTC68042result_packVoltage_get() * 0.68;  //heavy assist

            //Limit the minimum spoofed voltage. This helps prevent P1440s from happening for Balto.
			if (initialSpoofedPackVoltage < MIN_SPOOFED_VOLTAGE_60S) {spoofedPackVoltage = MIN_SPOOFED_VOLTAGE_60S;}  
			else {spoofedPackVoltage = initialSpoofedPackVoltage;}
		
		} 
        
        else
        {
            //medium assist
            //decrease spoofedPackVoltage inversely proportional to assist current

            uint8_t vAdjustRange_V = vspoofMCM_max - (LTC68042result_packVoltage_get() * 0.68);

            //Calculate voltage adjustment per amp assist
            //       voltageAdjustment_mV_per_A =            vAdjustRange_V   * 1000  / ADDITIONAL_AMPS_UNTIL_MAX_VSPOOF;
            //       voltageAdjustment_mV_per_A =            vAdjustRange_V   * 1024  / ADDITIONAL_AMPS_UNTIL_MAX_VSPOOF; //change multiply to 2^n (2.4% error)
            //       voltageAdjustment_mV_per_A =            vAdjustRange_V  <<   10 >> ADDITIONAL_AMPS__2_TO_THE_N     ; //substitute mult/div with bit shifts
            //       voltageAdjustment_mV_per_A =            vAdjustRange_V  <<  (10  - ADDITIONAL_AMPS__2_TO_THE_N)    ; //combine bit shifts
            uint16_t voltageAdjustment_mV_per_A = ((uint16_t)vAdjustRange_V) <<  (10  - ADDITIONAL_AMPS__2_TO_THE_N)    ; //cast intermediate math
            //max counts: 3712 (when 60S pack is 4.3 V/cell)
            //min counts:    0 (when 48S pack is 2.7 V/cell) 

            //Calculate how much to reduce actual pack voltage at any current
            //      packVoltageReduction_mV =  (actualCurrent_A                    - BEGIN_SPOOFING_VOLTAGE_ABOVE_AMPS) * voltageAdjustment_mV_per_A         ;
            //      packVoltageReduction_V  =  (actualCurrent_A                    - BEGIN_SPOOFING_VOLTAGE_ABOVE_AMPS) * voltageAdjustment_mV_per_A  * 0.001; //change mV to V
            uint16_t packVoltageReduction_V  = ((adc_getLatestBatteryCurrent_amps() - BEGIN_SPOOFING_VOLTAGE_ABOVE_AMPS) * voltageAdjustment_mV_per_A) * 0.001;

            //Calculate spoofed pack voltage
            uint8_t initialSpoofedPackVoltage = vspoofMCM_max - packVoltageReduction_V;
			
            //Limit the minimum spoofed voltage.
			if (initialSpoofedPackVoltage < MIN_SPOOFED_VOLTAGE_60S) {spoofedPackVoltage = MIN_SPOOFED_VOLTAGE_60S;}  
			else {spoofedPackVoltage = initialSpoofedPackVoltage;}

        }
        

   
    //---------------------------------------------------------------------------


    #elif defined  VOLTAGE_SPOOFING_LINEAR
	
	           spoofedPackVoltage = maxPossibleVspoof * 0.4 + 78; 
	
		   // adjusts spoof voltage across entire range so that current is 50A continuous, 83A peak
		   // 48S yields from +19% power
		   // 60S yields from +28% power
		   // max spoofing is within 67% of Vpack
		   // this is compatible with standard 100A OEM fuse.

    //---------------------------------------------------------------------------
	
    #else
        #error (VOLTAGE_SPOOFING value not selected in config.h)
    #endif

    //---------------------------------------------------------------------------

    //bound values
    if      ( spoofedPackVoltage > maxPossibleVspoof                                ) { spoofedPackVoltage = maxPossibleVspoof; }
    else if ((spoofedPackVoltage < 120) && (LTC68042result_packVoltage_get() > 130) ) { spoofedPackVoltage = 120;               }
}

/////////////////////////////////////////////////////////////////////////////////////////

void vPackSpoof_setVoltage(void)
{
    spoofVoltage_calculateValue(); //result saved in 'spoofedPackVoltage'

    spoofVoltageMCMe();
    spoofVoltage_VPINout();
    BATTSCI_setPackVoltage(spoofedPackVoltage);
}

/////////////////////////////////////////////////////////////////////////////////////////
