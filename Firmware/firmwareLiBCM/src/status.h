//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef runtimeStatus_h
    #define runtimeStatus_h

    void status_printState(void);

    void status_setValue_8b(uint8_t parameterToSet, uint8_t value);

    //1b parameters
    
    //unsigned 8b parameters
    #define STATUS_SOC_PERCENT_ACTUAL
    #define STATUS_SOC_PERCENT_SPOOFED   
    #define STATUS_PACK_VOLTAGE_ACTUAL
    #define STATUS_PACK_VOLTAGE_SPOOFED  
    #define STATUS_PACK_CURRENT_ACTUAL
    #define STATUS_PACK_CURRENT_SPOOFED  
    #define STATUS_BATTSCI_LATEST_METSCI_PACKET_E6
    #define STATUS_BATTSCI_LATEST_METSCI_PACKET_E1
    #define STATUS_BATTSCI_LATEST_METSCI_PACKET_B3
    #define STATUS_BATTSCI_LATEST_METSCI_PACKET_B4
    #define STATUS_BATTSCI_FLAGS_REGEN_ASSIST
    #define STATUS_BATTSCI_FLAGS_CHARGE_REQUEST
    #define STATUS_LTC6804_ERROR_COUNT
    #define STATUS_LTC6804_LO_CELL_VOLTAGE
    #define STATUS_LTC6804_HI_CELL_VOLTAGE


    #define STATUS_NUM_PARAMETERS_8b    2 //must be equal to the total number of parameters

    //signed 8b parameters
    #define STATUS_TEMP_BATTERY
    #define STATUS_TEMP_GRID
    #define STATUS_TEMP_INTAKE
    #define STATUS_TEMP_EXHAUST
    #define STATUS_TEMP_AMBIENT

    //16b parameters
    #define STATUS_KEYON_TIME_SECONDS 1

    #define STATUS_NUM_PARAMETERS_16b 1

    //state parameters we don't need to print
        //STATUS_IGNITION //already prints out when key state changes
        //STATUS_HOURS_UNTIL_FW_EXPIRES

#endif
