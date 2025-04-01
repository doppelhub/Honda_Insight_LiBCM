//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef LTC68042cell_h
    #define LTC68042cell_h

    // trigger mode argument values to LTC68042cell_nextVoltages()
    //  ROUND_ROBIN: trigger another acquisition as soon as possible
    //    This mode pipelines the acquisitions, so that the next acquisition can occur
    //    (in the BMS ic's) while the data from the current one is being processed.
    //    60 cells require 21 calls (at appropriate intervals) to LTC68042cell_nextVoltages()
    //    for a complete acquisition cycle.
    //  TRIGGERED: trigger an acquisition only at the next call after DONE__CELL_DATA_PROCESSED
    //    This mode allows an acquisition to be synchronized with events outside of
    //    LTC68042cell_nextVoltages(), and is optimum for testing cell voltage changes
    //    in response to specific events. This adds 1 extra call for a complete cycle.
    //  FORCE_TRIGGERED: transition to "ready to trigger" ASAP, abandoning any previous acqusition
    //    that is in process. This is optimum when switching from ROUND_ROBIN to TRIGGERED to get
    //    ready for a new acquisition without having to take the time to complete a full call cycle
    //    on the current acquisition.
    // NOTE: If an acquisition is in process when LTC_TRIGGERMODE_FORCE_TRIGGERED is used, it will
    //       return NO__WAITING_FOR_READY, so keep calling until it returns DONE__READY_TO_TRIGGER,
    //       indicating all is ready to start a new acquisition cycle. Following calls should be with
    //       LTC_TRIGGERMODE_TRIGGERED until LTC68042cell_nextVoltages() returns DONE__CELL_DATA_PROCESSED.
    // Switching between modes at any time is supported.  LTC68042cell_nextVoltages() will do the
    //    best it can.
    #define LTC_TRIGGERMODE_ROUND_ROBIN     0
    #define LTC_TRIGGERMODE_TRIGGERED       1
    #define LTC_TRIGGERMODE_FORCE_TRIGGERED 2

    #define LTC_STATE_FIRSTRUN           0
    #define LTC_WAITING_FOR_ADC          1
    #define LTC_STATE_GATHER             2
    #define LTC_STATE_PROCESS            3
    #define LTC_STATE_PROCESS_TRIGGERED  4
    #define LTC_STATE_TRIGGER            5

    #define NO__GATHERING_CELL_DATA    0
    #define DONE__CELL_DATA_PROCESSED  1
    #define NO__WAITING_FOR_READY      2
    #define DONE__READY_TO_TRIGGER     3

    #define LTC6804_MAX_CONVERSION_TIME_ms 5 //4.43 ms in '2kHz' sampling mode

    uint8_t LTC68042cell_nextVoltages(uint8_t triggerMode);
    void LTC68042cell_acquireAllCellVoltages(void);

#endif
