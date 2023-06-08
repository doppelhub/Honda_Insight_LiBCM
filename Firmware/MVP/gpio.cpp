//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//all digitalRead(), digitalWrite(), analogRead(), analogWrite() functions live here
//JTS2doLater: Replace Arduino fcns with low level

#include "libcm.h"

//FYI: simple pin state read/writes take less than 10 us

////////////////////////////////////////////////////////////////////////////////////

uint8_t gpio_getHardwareRevision(void)
{
	uint8_t hardwareRevision = 0;

	//These pins are either tied to ground or left floating, depending on HW revision
	pinMode(PIN_HW_VER0, INPUT_PULLUP);
	pinMode(PIN_HW_VER1, INPUT_PULLUP);

	if     ( (digitalRead(PIN_HW_VER1) == true ) && (digitalRead(PIN_HW_VER0) == true ) ) { hardwareRevision = HW_REV_C; }
	else if( (digitalRead(PIN_HW_VER1) == true ) && (digitalRead(PIN_HW_VER0) == false) ) { hardwareRevision = HW_REV_D; }
	else if( (digitalRead(PIN_HW_VER1) == false) && (digitalRead(PIN_HW_VER0) == true ) ) { hardwareRevision = HW_REV_E; } //future HW support
	else if( (digitalRead(PIN_HW_VER1) == false) && (digitalRead(PIN_HW_VER0) == false) ) { hardwareRevision = HW_REV_F; } //future HW support

	//disable pullups to save power
	pinMode(PIN_HW_VER0, INPUT);
	pinMode(PIN_HW_VER1, INPUT);

	return hardwareRevision;
}

////////////////////////////////////////////////////////////////////////////////////

void gpio_begin(void)
{
	//Ensure 12V->5V DCDC stays on
	pinMode(PIN_TURNOFFLiBCM,OUTPUT);
	digitalWrite(PIN_TURNOFFLiBCM,LOW);

	pinMode(PIN_HMI_EN,OUTPUT);

	//Controls BCM current sensor, constant 5V load, and BATTSCI/METSCI biasing
	pinMode(PIN_SENSOR_EN,OUTPUT);
	gpio_turnPowerSensors_on(); //if the key is off when LiBCM first powers up, the keyOff handler will turn the sensors back off

	pinMode(PIN_LED1,OUTPUT);
	pinMode(PIN_LED2,OUTPUT);
	pinMode(PIN_LED3,OUTPUT);
	pinMode(PIN_LED4,OUTPUT);

	analogWrite(PIN_MCME_PWM,0);
	pinMode(PIN_FAN_PWM,OUTPUT);
	pinMode(PIN_FANOEM_LOW,OUTPUT);
	pinMode(PIN_FANOEM_HI,OUTPUT);
	pinMode(PIN_GRID_EN,OUTPUT);
	pinMode(PIN_TEMP_EN,OUTPUT);
	pinMode(PIN_GPIO0,OUTPUT);
	digitalWrite(PIN_GPIO0,HIGH); //JTS2doLater: Move into LiControl function

	analogReference(EXTERNAL); //use 5V AREF pin, which is coupled to filtered VCC

	//JTS2doLater: Turn all this stuff off when the key is off
	TCCR1B = (TCCR1B & B11111000) | B00000001; // Set F_PWM to 31372.55 Hz //pins D11(fan) & D12()
	TCCR3B = (TCCR3B & B11111000) | B00000001; // Set F_PWM to 31372.55 Hz //pins D2() & D3() & D5(VPIN_OUT)
	TCCR4B = (TCCR4B & B11111000) | B00000010; // Set F_PWM to  3921.16 Hz //pins D7(MCMe) & D8(gridPWM) & D9()
	//TCCR5B is set in Buzzer functions
}

////////////////////////////////////////////////////////////////////////////////////

bool gpio_keyStateNow(void) { return digitalRead(PIN_IGNITION_SENSE); }

////////////////////////////////////////////////////////////////////////////////////

//THIS FUNCTION DOES NOT RETURN!
void gpio_turnLiBCM_off(void)
{
	//JTS2doLater: Write SoC to EEPROM (so LiBCM can read it back at next keyON, if not enough time to calculate it)
	Serial.print(F("\nLiBCM turning off"));
	delay(20); //wait for the above message to transmit
	digitalWrite(PIN_TURNOFFLiBCM,HIGH);
	while(1) { ; } //LiBCM takes a bit to turn off... wait here until that happens
}

////////////////////////////////////////////////////////////////////////////////////

void gpio_setFanSpeed_OEM(char speed)
{
	#ifdef OEM_FAN_INSTALLED
		switch(speed)
		{
			case FAN_OFF:  digitalWrite(PIN_FANOEM_LOW,  LOW); digitalWrite(PIN_FANOEM_HI,  LOW); break;
			case FAN_LOW:  digitalWrite(PIN_FANOEM_LOW, HIGH); digitalWrite(PIN_FANOEM_HI,  LOW); break;
			//case FAN_MED:  digitalWrite(PIN_FANOEM_LOW, HIGH); digitalWrite(PIN_FANOEM_HI,  LOW); break; //same as FAN_LOW... OEM fan only supports OFF/LOW/HIGH
			case FAN_HIGH: digitalWrite(PIN_FANOEM_LOW, HIGH); digitalWrite(PIN_FANOEM_HI, HIGH); break; //JTS2doNow: Change back
		}
	#endif
}

////////////////////////////////////////////////////////////////////////////////////

void gpio_setFanSpeed_PCB(char speed)
{
	uint8_t fanPWM = 0;

	switch(speed)
	{
		case FAN_OFF:  fanPWM =     0; break;
		case FAN_LOW:  fanPWM =    40; break;
		//case FAN_MED:  fanPWM =    60; break; //Not used
		case FAN_HIGH: fanPWM =   255; break;
		default: Serial.print(F("\nError: Invalid fan speed"));
	}

	if (fanPWM == 0) { pinMode(PIN_FAN_PWM, INPUT); } //saves power when fan is off
	else             { analogWrite(PIN_FAN_PWM, fanPWM); }
}

////////////////////////////////////////////////////////////////////////////////////

//powers OEM current sensor, METSCI/BATTSCI power rail, and constant 5V load
void gpio_turnPowerSensors_on( void) { digitalWrite(PIN_SENSOR_EN, HIGH); }
void gpio_turnPowerSensors_off(void) { digitalWrite(PIN_SENSOR_EN,  LOW); }

////////////////////////////////////////////////////////////////////////////////////

void gpio_turnHMI_on( void) { digitalWrite(PIN_HMI_EN, HIGH); }
void gpio_turnHMI_off(void) { digitalWrite(PIN_HMI_EN,  LOW); }
bool gpio_HMIStateNow(void) { return digitalRead(PIN_HMI_EN); }

////////////////////////////////////////////////////////////////////////////////////

bool gpio_isGridChargerPluggedInNow(void) { return !(digitalRead(PIN_GRID_SENSE)); }
bool gpio_isGridChargerChargingNow(void)  { return   digitalRead(PIN_GRID_EN);     }

////////////////////////////////////////////////////////////////////////////////////

void gpio_turnGridCharger_on(void)  { digitalWrite(PIN_GRID_EN, HIGH); }
void gpio_turnGridCharger_off(void) { digitalWrite(PIN_GRID_EN, LOW);  }

////////////////////////////////////////////////////////////////////////////////////

void gpio_setGridCharger_powerLevel(char powerLevel)
{
	switch(powerLevel)
	{
		#ifdef GRIDCHARGER_IS_1500W //wiring is different from other chargers		
			//JTS2doNow: Add #define in gpio.c to remap this to the correct daughterboard pin
			//JTS2doNow: Add #define in gpio.c to add charger on/off pin
			case '0': analogWrite(PIN_GRID_PWM,   0); break; //disable grid charger
			//case 'L' //this pin is connected to simple on/off control, so there are no intermediate states
			//case 'M' //this pin is connected to simple on/off control, so there are no intermediate states
			case 'H': analogWrite(PIN_GRID_PWM, 255); break; //enable grid charger
			case 'Z': pinMode(PIN_GRID_PWM, INPUT);   break; //reduces power consumption
			default:  analogWrite(PIN_GRID_PWM,   0); break; //disable charger
		#else
			case '0': analogWrite(PIN_GRID_PWM, 255); break; //negative logic
			case 'L': analogWrite(PIN_GRID_PWM, 160); break; //JTS2doLater: Determine correct grid charger values
			case 'M': analogWrite(PIN_GRID_PWM,  80); break;
			case 'H': analogWrite(PIN_GRID_PWM,   0); break;
			case 'Z': pinMode(PIN_GRID_PWM, INPUT);   break; //reduces power consumption
			default:  analogWrite(PIN_GRID_PWM, 255); break; //disable charger
		#endif
	}
}

////////////////////////////////////////////////////////////////////////////////////

//Buzzer present in RevC+ hardware
void gpio_turnBuzzer_on_highFreq(void)
{
	TCCR5B = (TCCR5B & B11111000) | B00000010; // set F_PWM to  3921.16 Hz //pins D44(GPIO3) & D45(BUZZER) & D46()
	analogWrite(PIN_BUZZER_PWM, 127 );
}

////////////////////////////////////////////////////////////////////////////////////

void gpio_turnBuzzer_on_lowFreq(void)
{
	TCCR5B = (TCCR5B & B11111000) | B00000011; // set F_PWM to   490.20 Hz //pins D44(GPIO3) & D45(BUZZER) & D46()
	analogWrite(PIN_BUZZER_PWM, 127 );
}

////////////////////////////////////////////////////////////////////////////////////

void gpio_turnBuzzer_off(void) { analogWrite(PIN_BUZZER_PWM,0); }

////////////////////////////////////////////////////////////////////////////////////

void gpio_playSound_firmwareUpdated(void)
{
	const uint8_t NUM_BEEPS = 10;
	for(uint8_t ii = 1; ii < (NUM_BEEPS); ii++)
	{
		gpio_turnBuzzer_on_lowFreq();
		delay(10 * ii);
		gpio_turnBuzzer_on_highFreq();
		delay(10 * (NUM_BEEPS - ii));
	}
	gpio_turnBuzzer_off();
}

////////////////////////////////////////////////////////////////////////////////////

bool gpio_isCoverInstalled(void)
{
	#ifdef CHECK_FOR_SAFETY_COVER
		if(digitalRead(PIN_COVER_SWITCH) == 1 ) {return  true;}
		else                                    {return false;}
	#else
		return true;
	#endif
}

////////////////////////////////////////////////////////////////////////////////////

bool gpio1_getState(void) { return digitalRead(PIN_GPIO1); }
bool gpio2_getState(void) { return digitalRead(PIN_GPIO2); }
bool gpio3_getState(void) { return digitalRead(PIN_GPIO3_HEATER); }

////////////////////////////////////////////////////////////////////////////////////

void gpio_turnTemperatureSensors_on( void) {digitalWrite(PIN_TEMP_EN,HIGH); }
void gpio_turnTemperatureSensors_off(void) {digitalWrite(PIN_TEMP_EN,LOW ); }

////////////////////////////////////////////////////////////////////////////////////

void gpio_turnPackHeater_on(void) { pinMode(PIN_GPIO3_HEATER,OUTPUT); digitalWrite(PIN_GPIO3_HEATER,HIGH); }
void gpio_turnPackHeater_off(void){ pinMode(PIN_GPIO3_HEATER,INPUT);  digitalWrite(PIN_GPIO3_HEATER,LOW);  }

////////////////////////////////////////////////////////////////////////////////////

uint8_t gpio_getPinMode(uint8_t pin)
{
	volatile uint8_t *reg;

	reg = portModeRegister( digitalPinToPort(pin) );

	if( (*reg) & digitalPinToBitMask(pin) ) { return OUTPUT; }
	else                                    { return INPUT;  }
}

////////////////////////////////////////////////////////////////////////////////////

uint8_t gpio_getPinState(uint8_t pin)
{
	uint8_t pinMode = gpio_getPinMode(pin);
	uint8_t pinLevel = digitalRead(pin);

	if     ( (pinLevel == LOW ) && (pinMode == OUTPUT) ) { return PIN_OUTPUT_LOW;  }
	else if( (pinLevel == HIGH) && (pinMode == OUTPUT) ) { return PIN_OUTPUT_HIGH; }
	else if( (pinLevel == LOW ) && (pinMode == INPUT ) ) { return PIN_INPUT_LOW;   }
	else if( (pinLevel == HIGH) && (pinMode == INPUT ) ) { return PIN_INPUT_HIGH;  }
	else                                                 { return PIN_STATE_ERROR; }
}

////////////////////////////////////////////////////////////////////////////////////

//JTS2doLater: Replace Arduino I/O functions
/*
  #define DIRECTION_DDR     DDRD
  #define DIRECTION_PORT    PORTD
  #define X_DIRECTION_BIT   5  // Uno Digital Pin 5
  #define Y_DIRECTION_BIT   6  // Uno Digital Pin 6
  #define Z_DIRECTION_BIT   7  // Uno Digital Pin 7
  #define DIRECTION_MASK    ((1<<X_DIRECTION_BIT)|(1<<Y_DIRECTION_BIT)|(1<<Z_DIRECTION_BIT)) // All direction bits
*/
