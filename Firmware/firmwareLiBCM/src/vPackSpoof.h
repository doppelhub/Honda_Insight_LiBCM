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

    void vPackSpoof_setVoltage(void);

    void vPackSpoof_handleKeyOFF(void);
    void vPackSpoof_handleKeyON(void);

    int8_t vPackSpoof_offsetBVO_get(void);
    int8_t vPackSpoof_offsetMDV_get(void);
    int8_t vPackSpoof_offsetSPF_get(void);

    void vPackSpoof_offsetBVO_adjust(uint8_t action);
    void vPackSpoof_offsetMDV_adjust(uint8_t action);
    void vPackSpoof_offsetSPF_adjust(uint8_t action);

    uint8_t vPackSpoof_getSpoofedPackVoltage(void);

#endif

