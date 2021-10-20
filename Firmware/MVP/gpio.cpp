//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//all digitalRead(), digitalWrite(), analogRead(), analogWrite() functions live here
//JTS2doLater: Replace Arduino fcns with low level

#include "libcm.h"

//FYI: simple pin state read/writes take less than 10 us

void gpio_begin(void)
{
	//Ensure 12V->5V DCDC stays on
	pinMode(PIN_TURNOFFLiBCM,OUTPUT);
	digitalWrite(PIN_TURNOFFLiBCM,LOW);

	//turn on lcd display
	pinMode(PIN_HMI_EN,OUTPUT);
	digitalWrite(PIN_HMI_EN,HIGH);

	//Enable BCM current sensor & constant 5V load
	pinMode(PIN_SENSOR_EN,OUTPUT);
	digitalWrite(PIN_SENSOR_EN,LOW);

	pinMode(PIN_LED1,OUTPUT);
	pinMode(PIN_LED2,OUTPUT);
	pinMode(PIN_LED3,OUTPUT);
	pinMode(PIN_LED4,OUTPUT);

	analogWrite(PIN_MCME_PWM,0);
	pinMode(PIN_FAN_PWM,OUTPUT);
	pinMode(PIN_FANOEM_LOW,OUTPUT);
	pinMode(PIN_FANOEM_HI,OUTPUT);
	pinMode(PIN_GRID_EN,OUTPUT);

	analogReference(EXTERNAL); //use 5V AREF pin, which is coupled to filtered VCC

	TCCR1B = (TCCR1B & B11111000) | B00000001; // Set F_PWM to 31372.55 Hz //pins D11(fan) & D12()
	TCCR3B = (TCCR3B & B11111000) | B00000001; // Set F_PWM to 31372.55 Hz //pins D2() & D3() & D5(VPIN_OUT on RevC)
	//TCCR5B is set in Buzzer functions
}


////////////////////////////////////////////////////////////////////////////////////


bool gpio_keyStateNow(void) { return digitalRead(PIN_IGNITION_SENSE); }


////////////////////////////////////////////////////////////////////////////////////

void gpio_setFanSpeed_OEM(char speed)
{
	switch(speed)
	{
		case '0': digitalWrite(PIN_FANOEM_LOW,  LOW); digitalWrite(PIN_FANOEM_HI,  LOW); break;
		case 'L': digitalWrite(PIN_FANOEM_LOW, HIGH); digitalWrite(PIN_FANOEM_HI,  LOW); break;
		case 'H': digitalWrite(PIN_FANOEM_LOW,  LOW); digitalWrite(PIN_FANOEM_HI, HIGH); break;
	}
}

////////////////////////////////////////////////////////////////////////////////////

void gpio_setFanSpeed(char speed)
{
	switch(speed)
	{
		case '0': pinMode(PIN_FAN_PWM, INPUT); break; //high impedance
		case 'L': analogWrite(PIN_FAN_PWM, 5); break; //JTS2doLater: why are these values so low?
		case 'M': analogWrite(PIN_FAN_PWM, 40); break;
		case 'H': analogWrite(PIN_FAN_PWM, 255); break;
	}
}

////////////////////////////////////////////////////////////////////////////////////

//power current sensor & constant 5V load
void gpio_turnCurrentSensor_on( void) { digitalWrite(PIN_SENSOR_EN, HIGH); }
void gpio_turnCurrentSensor_off(void) { digitalWrite(PIN_SENSOR_EN,  LOW); }

////////////////////////////////////////////////////////////////////////////////////

bool gpio_isGridChargerPluggedInNow(void) { return !(digitalRead(PIN_GRID_SENSE)); }

////////////////////////////////////////////////////////////////////////////////////

void gpio_turnGridCharger_on( void) { digitalWrite(PIN_GRID_EN, HIGH); }
void gpio_turnGridCharger_off(void) { digitalWrite(PIN_GRID_EN,  LOW); }

////////////////////////////////////////////////////////////////////////////////////

void gpio_setGridCharger_powerLevel(char powerLevel)
{
	switch(powerLevel)
	{
		case '0': analogWrite(PIN_GRID_PWM, 255); break; //negative logic
		case 'L': analogWrite(PIN_GRID_PWM, 160); break; //JTS2doNow: Determine correct values
		case 'M': analogWrite(PIN_GRID_PWM, 80); break;
		case 'H': pinMode(PIN_GRID_PWM, INPUT); break; //reduces power consumption
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


#ifdef HW_REVB
	//features unique to RevB
#else
	//RevC+ supports the following
	bool gpio_isCoverInstalled(void)
	{
		if(digitalRead(PIN_COVER_SWITCH) == 1 ) {return  true;}
		else                                    {return false;}
	}

#endif

////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////