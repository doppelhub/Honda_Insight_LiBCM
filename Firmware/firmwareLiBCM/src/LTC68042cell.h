//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#ifndef LTC68042cell_h
    #define LTC68042cell_h

    // trigger mode argument values to LTC68042cell_nextVoltages()
    //  CONTINUOUS: trigger another acqusition as soon as possible
    //    This mode piplines the acquisitions, so that the next acquisition can occur
    //    (in the BMS ic's) while the data from the current one is being processed.
    //    60 cells require 21 calls (at apropriate intervels) to LTC68042cell_nextVoltages()
    //    for a complete acquisition cycle.
    //  TRIGGERED: trigger an acqusition only at the next call after CELL_DATA_PROCESSED
    //    This mode allows a acquisition to be syncronized with events outside of
    //    LTC68042cell_nextVoltages(), and is optimum for testing cell votage changes
    //    in response to specific events. This adds 1 extra call for a complete cycle.
    //  FORCE_TRIGGERED: trigger an acqusition NOW, abandoning any previous acqusition
    //    that is in process. This is optimum as the first, single, call when when
    //    switching from CONTINUOUS to TRIGGERED to get a new acquisition without having
    //    to take the time to complete a full call cycle on the current acquisition.
    // NOTE: It is only appripriate to make a call with LTC_TRIGGERMODE_FORCE_TRIGGERED one
    //       time, to start a new acquisition cycle. Following calls should be with
    //       LTC_TRIGGERMODE_TRIGGERED until LTC68042cell_nextVoltages() returns CELL_DATA_PROCESSED.
    // Switching between modes at any time is supported.  LTC68042cell_nextVoltages() will do the
    //    best it can.
    #define LTC_TRIGGERMODE_CONTINUOUS      0
    #define LTC_TRIGGERMODE_TRIGGERED       1
    #define LTC_TRIGGERMODE_FORCE_TRIGGERED 2

    #define LTC_STATE_FIRSTRUN 0
    #define LTC_STATE_GATHER   1
    #define LTC_STATE_PROCESS  2
    #define LTC_STATE_GATHER_TRIGGERED   4 // not used
    #define LTC_STATE_PROCESS_TRIGGERED  8
    #define LTC_STATE_TRIGGER  16

    #define GATHERING_CELL_DATA  0
    #define CELL_DATA_PROCESSED  1
    #define WAITING_TO_TRIGGER   2

    #define LTC6804_MAX_CONVERSION_TIME_ms 5 //4.43 ms in '2kHz' sampling mode

    uint8_t LTC68042cell_nextVoltages(uint8_t triggerMode);
    void LTC68042cell_acquireAllCellVoltages(void);

#endif
