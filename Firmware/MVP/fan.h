#ifndef fan_h
	#define fan_h

	//Mask to store each subsystem's fan speed request
	//Each subsystem stores 2 bits (OFF/LOW/MED/HIGH)
	#define FAN_REQUESTOR_BATTERY     0b00000011 //the grid charger subsystem stores its fan request status in the two lowest bits
	#define FAN_REQUESTOR_USER        0b00001100
	#define FAN_REQUESTOR_GRIDCHARGER 0b00110000
	#define FAN_REQUESTOR_CELLBALANCE 0b11000000 //the cell balance subsystem stores its fan request status in the two highest bits

	#define FAN_REQUESTOR_BITSHIFTS_BATTERY     0
	#define FAN_REQUESTOR_BITSHIFTS_USER        2
	#define FAN_REQUESTOR_BITSHIFTS_GRIDCHARGER 4
	#define FAN_REQUESTOR_BITSHIFTS_CELLBALANCE 6

	//fan request status bits (for each subsystem)
	#define FAN_OFF  0b00
	#define FAN_LOW  0b01
	//#define FAN_MED  0b10 //JTS2doLater: not fully implemented in hasEnoughTimePassedToChangeFanSpeed()
	#define FAN_HIGH 0b11

	#define FAN_HI_MASK   0b10101010 //OR this mask with fanSpeed_allRequestors to see if any subsystem is requesting high speed fans
	#define FAN_LO_MASK   0b01010101 //OR this mask with fanSpeed_allRequestors to see if any subsystem is requesting  low speed fans
	#define FAN_FORCE_OFF 0b00000000

	#define FAN_SPEED_INCREASE_HYSTERESIS_ms  (5 * 1000) //period required before fan speed can increase
	#define FAN_SPEED_DECREASE_HYSTERESIS_ms (10 * 1000) //period required before fan speed can decrease

	#define FAN_SPEED_HYSTERESIS_HIGH_degC 4 //adds temperature based hysteresis to battery related fan requests
	#define FAN_SPEED_HYSTERESIS_LOW_degC  2
	#define FAN_SPEED_HYSTERESIS_OFF_degC  0

	#define FAN_TIME_ON_TO_SAMPLE_CABIN_AIR_ms         ( 5 *  1000) //When the key is off and the battery is hot or cold, how long to run fans to draw cabin air into plenum
	#define FAN_TIME_ON_BEFORE_INTAKE_TEMP_VALID_ms    (10 *  1000) //period fan must run before intake temperature sensor accurately measures cabin air temperature
	#define FAN_TIME_OFF_BEFORE_INTAKE_TEMP_INVALID_ms ( 2 * 60000) //period after fan stops where the intake temperature sensor value still reflects cabin air temperature

	#define FAN_ADDITIONAL_TEMP_FOR_HIGH_SPEED_degC    5 //this many additional degrees enables fan at high speed (instead of low speed)
	#define MIN_CABIN_AIR_DELTA_FOR_FANS_degC 5 //intake air must be at least this many degrees 'better' for fans to heat/cool pack

	#define SAMPLE_CABIN_AIR_INTERVAL_KEY_OFF_ms   (15 * 60000) //if pack too hot or cold, LiBCM will briefly run fans at least this often to draw cabin air across intake temp sensor
	#define SAMPLE_CABIN_AIR_INTERVAL_KEY_ON_ms    ( 3 * 60000)
	#define SAMPLE_CABIN_AIR_INTERVAL_PLUGGEDIN_ms ( 3 * 60000)

	void fan_handler(void);

	uint8_t fan_getSpeed_now(void);

	void fan_requestSpeed(uint8_t requestor, char newFanSpeed); //request fan speed //Example: fan_requestSpeed(FAN_REQUESTOR_GRIDCHARGER, FAN_HIGH)
#endif
