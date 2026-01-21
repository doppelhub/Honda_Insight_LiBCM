//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM
//Functionality added by AfterEffect

#ifndef LiControl_h
    #define LiControl_h
    
    //==============================================================================
    // Core Functions (existing interface)
    //==============================================================================
    void LiControl_handler(void);
    void LiControl_begin(void);
    
    //==============================================================================
    // Status and Health Monitoring
    //==============================================================================
    bool LiControl_isConnected(void);
    bool LiControl_isHealthy(void);
    
    //==============================================================================
    // Data Access Functions
    //==============================================================================
    uint8_t LiControl_getMode(void);           // Returns current MIMA mode
    uint8_t LiControl_getAssistLevel(void);    // Returns assist level (0-100%)
    uint8_t LiControl_getRegenLevel(void);     // Returns regen level (0-100%)
    int16_t LiControl_getInstantaneousPower(void); // Returns power in watts
    int16_t LiControl_getCurrent_deciAmps(void);   // Returns current in deciAmps
    
    //==============================================================================
    // Control Functions
    //==============================================================================
    void LiControl_sendEmergencyStop(void);
    void LiControl_disable(void);
    
    //==============================================================================
    // MIMA Mode Constants
    //==============================================================================
    #define MIMA_MODE_OFF      0
    #define MIMA_MODE_ASSIST   1
    #define MIMA_MODE_REGEN    2
    #define MIMA_MODE_MANUAL   3
    
    //==============================================================================
    // Error Flag Constants
    //==============================================================================
    #define LICONTROL_ERROR_NONE           0x00
    #define LICONTROL_ERROR_OVERHEAT       0x01
    #define LICONTROL_ERROR_OVERCURRENT    0x02
    #define LICONTROL_ERROR_COMMUNICATION  0x04
    #define LICONTROL_ERROR_SENSOR         0x08
    #define LICONTROL_ERROR_SAFETY         0x10
    
#endif