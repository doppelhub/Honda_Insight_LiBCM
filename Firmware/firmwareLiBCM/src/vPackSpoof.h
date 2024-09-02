//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef vpackspoof_h
    #define vpackspoof_h

    #define BEGIN_SPOOFING_VOLTAGE_ABOVE_AMPS 10 //below this many amps assist, spoofed pack voltage is as high as possible
        
    //choose the current range to adjust spoofed pack voltage from 0 to 100% over
        //#define ADDITIONAL_AMPS_UNTIL_MAX_VSPOOF  32 //2^5 = 32 //MUST be 2^n!
        //#define ADDITIONAL_AMPS__2_TO_THE_N        5 //2^5 = 32
    //OR
        #define ADDITIONAL_AMPS_UNTIL_MAX_VSPOOF  64 //2^6 = 64 //MUST be 2^n!
        #define ADDITIONAL_AMPS__2_TO_THE_N        6 //2^6 = 64

    #define MAXIMIZE_POWER_ABOVE_CURRENT_AMPS (BEGIN_SPOOFING_VOLTAGE_ABOVE_AMPS + ADDITIONAL_AMPS_UNTIL_MAX_VSPOOF)

    #define VSPOOF_TO_MAXIMIZE_POWER 125 //Maximum assist occurs when MCM thinks pack is at 120 volts
    
    #define DISABLE_60S_VSPOOF_VOLTAGE 172 //Disable voltage spoofing if maxPossibleVspoof drops below a specificed voltage. 172 = pack voltage of 186, or 3.1 volts/cell.
        
    
    #define ADDITIONAL_VPIN_OFFSET_VOLTS 0 //this many volts are added to VPIN output //use with OBDIIC&C to make BATTSCI voltage equal to VPIN voltage
    #define ADDITIONAL_MCMe_OFFSET_VOLTS 0 //this many volts are added to MCMe output //use with OBDIIC&C to make BATTSCI voltage equal to MCMe voltage

    void vPackSpoof_setVoltage(void);

    void vPackSpoof_handleKeyOFF(void);
    void vPackSpoof_handleKeyON(void);

    uint8_t vPackSpoof_getSpoofedPackVoltage(void);

#endif
