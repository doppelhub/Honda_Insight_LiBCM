//MVP PoC:

/*
* Don't care about:
* -SoC (accumulation)
* -Temperature
* -Fan speed
* -Spoofing ConnE
* -Spoofing Vpin
* -Grid charger
*/

/*This will eventually be the only line of code in this file:
#include "libcm.h"

//See main.c for superloop
*/

#include "libcm.h"

void setup()
{
  //Ensure 12V->5V DCDC stays on
  pinMode(PIN_TURNOFFLiBCM,OUTPUT);
  digitalWrite(PIN_TURNOFFLiBCM,LOW);

  //Enable BCM current sensor & constant 5V load
  pinMode(PIN_I_SENSOR_EN,OUTPUT);
  digitalWrite(PIN_I_SENSOR_EN,LOW);

  pinMode(PIN_LED1,OUTPUT);
  pinMode(PIN_LED2,OUTPUT);
  pinMode(PIN_LED3,OUTPUT);
  pinMode(PIN_LED4,OUTPUT);

  digitalWrite(PIN_LED1,HIGH); //CPU booted successfully

  pinMode(PIN_CONNE_PWM,OUTPUT);
  analogWrite(PIN_CONNE_PWM,0);
  pinMode(PIN_FAN_PWM,OUTPUT);
  pinMode(PIN_FANOEM_LOW,OUTPUT);
  pinMode(PIN_FANOEM_HI,OUTPUT);
  pinMode(PIN_GRID_EN,OUTPUT);
  //pinMode(PIN_VPIN_OUT_PWM,OUTPUT);

  pinMode(PIN_HMI_EN,OUTPUT);
  digitalWrite(PIN_HMI_EN,HIGH); //turn on 4x20 display

  analogReference(EXTERNAL); //use 5V AREF pin, which is coupled to filtered VCC

  if( digitalRead(PIN_KEY_ON) ){ digitalWrite(PIN_LED3,HIGH); } //if the key is on when LiBCM starts up, turn LED3 on

  Serial.begin(115200); //USB

  METSCI_begin();
  BATTSCI_begin();

  LTC6804_initialize();

  TCCR1B = (TCCR1B & B11111000) | B00000001; // Set onboard fan PWM frequency to 31372 Hz (pins D11 & D12)

  Serial.print(F("\n\nWelcome to LiBCM v"));
  Serial.print(String(FW_VERSION));
  Serial.print("," + String(BUILD_DATE) + "\n\n");
  
}

void loop()
{
	uint8_t keyStatus_now = digitalRead(PIN_KEY_ON);  //Get key position
	static uint8_t keyStatus_previous;

	//steps to perform when key state changes (on->off, off->on)
	if( keyStatus_now != keyStatus_previous)
	{
	  Serial.print(F("\nKey is: "));
	  LTC6804_isoSPI_errorCountReset();

	  if( keyStatus_now == 0 )
	  {
	    Serial.print(F("OFF"));
	    BATTSCI_disable(); //Must disable BATTSCI when key is off to prevent backdriving MCM
	    METSCI_disable();
	    digitalWrite(PIN_FANOEM_LOW,LOW);
	    digitalWrite(PIN_I_SENSOR_EN,LOW); //disable current sensor & constant 5V load
	    LTC6804_4x20displayOFF();
	  } else {
	    Serial.print(F("ON"));
	    BATTSCI_enable();
	    METSCI_enable();
	    digitalWrite(PIN_FANOEM_LOW,HIGH);
	    digitalWrite(PIN_I_SENSOR_EN,HIGH); //enable current sensor & constant 5V load
	    LTC6804_4x20displayON();
	  }
	}
	keyStatus_previous = keyStatus_now;

	//---------------------------------------------------------------------------------------

	//Determine whether grid charger is plugged in
	uint8_t gridChargerPowered_now = !(digitalRead(PIN_GRID_SENSE));
	static uint8_t gridChargerPowered_previous;

	//steps to perform when grid charger state changes (plugged in || unplugged)
	if( gridChargerPowered_now != gridChargerPowered_previous)
	{
	  Serial.print(F("\nGrid Charger: "));
	  if( gridChargerPowered_now == 0 ) //grid charger was just unplugged
	  {
	    Serial.print(F("Unplugged"));
	    analogWrite(PIN_FAN_PWM,0);     //turn onboard fans off
	    digitalWrite(PIN_GRID_EN,0);    //turn grid charger off
	    Serial.print("\nGrid Charger Disabled");
	    LTC6804_4x20displayOFF();

	  } else {                          //grid charger was just plugged in
	    Serial.print(F("Plugged In"));
	    LTC6804_4x20displayON();
	  }
	}
	gridChargerPowered_previous = gridChargerPowered_now;

	//---------------------------------------------------------------------------------------

	if( (keyStatus_now == 1) || (gridChargerPowered_now == 1) ) //key is on or grid charger plugged in
	{
	  digitalWrite(PIN_LED4,HIGH);

	  LTC6804_startCellVoltageConversion();
	  //We don't immediately read the results afterwards because it takes a second to digitize
	  //In Coop Task setting we'll need to invoke reading n microseconds after this function is called

	  //---------------------------------------------------------------------------------------

	  //get 64x oversampled current sensor value
	  uint16_t ADC_oversampledAccumulator = 0;
	  for(int ii=0; ii<64; ii++)  //This takes ~112 us per run (7.2 ms total)
	  {
	    ADC_oversampledAccumulator += analogRead(PIN_BATTCURRENT);
	  }

	  int16_t ADC_oversampledResult = int16_t( (ADC_oversampledAccumulator >> 6) );
	  Serial.print(F("\nADC:"));
	  Serial.print( String(ADC_oversampledResult) );

	  //convert current sensor result into approximate amperage for MCM & user-display
	  //don't use this result for current accumulation... it's not accurate enough
	  int16_t battCurrent_amps = ( (ADC_oversampledResult * 13) >> 6) - 67; //Accurate to within 3.7 amps of actual value
	  Serial.print(F(", "));
	  Serial.print( String(battCurrent_amps) );
	  Serial.print(F(" A(raw), "));

	  if (ENABLE_CURRENT_HACK) {battCurrent_amps = (int16_t)(battCurrent_amps * 0.7);} //140% current hack = tell MCM 70% actual
	  Serial.print( String(battCurrent_amps) );
	  Serial.print(F(" A(MCM)"));


	  //---------------------------------------------------------------------------------------

	  LTC6804_getCellVoltages(); //individual cell results stored in 'cell_codes' array

	  //sum all 48 cells
	  uint8_t stackVoltage = LTC6804_getStackVoltage();
	  stackVoltage = (uint8_t)(stackVoltage*0.94);

	  //---------------------------------------------------------------------------------------

	  if( keyStatus_now ) //key is on
	  {
	    METSCI_processLatestFrame();

	    BATTSCI_sendFrames(stackVoltage, battCurrent_amps);
	  }

	  delay(100); //forcing buffers to overqueue to verify LiBCM responds correctly
	} else {
	  //key is off & grid charger unplugged
	  static uint16_t toggleTimer = 0;
	  if( ++toggleTimer >= 10000 )
	  {
	    digitalWrite(PIN_LED4, !digitalRead(PIN_LED4)); //Toggle LED4 (heartbeat)
	    toggleTimer=0;
	  }
	}
	digitalWrite(PIN_LED2, !digitalRead(PIN_LED2)); //Toggle LED2 (heartbeat)
}