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

	//turn on lcd display
  pinMode(PIN_HMI_EN,OUTPUT);
  digitalWrite(PIN_HMI_EN,HIGH);

  //Enable BCM current sensor & constant 5V load
  pinMode(PIN_I_SENSOR_EN,OUTPUT);
  digitalWrite(PIN_I_SENSOR_EN,LOW);

  pinMode(PIN_LED1,OUTPUT);
  pinMode(PIN_LED2,OUTPUT);
  pinMode(PIN_LED3,OUTPUT);
  pinMode(PIN_LED4,OUTPUT);

  pinMode(PIN_MCME_PWM,OUTPUT);
  analogWrite(PIN_MCME_PWM,0);
  pinMode(PIN_FAN_PWM,OUTPUT);
  pinMode(PIN_FANOEM_LOW,OUTPUT);
  pinMode(PIN_FANOEM_HI,OUTPUT);
  pinMode(PIN_GRID_EN,OUTPUT);

  analogReference(EXTERNAL); //use 5V AREF pin, which is coupled to filtered VCC

  if( digitalRead(PIN_KEY_ON) ){ LED(3,HIGH); } //LED3 turns on if key was on when LiBCM first booted

  Serial.begin(115200); //USB

  //lcd_initialize(); //Don't call here, so LiBCM can recover quickly if watchdog resets while driving 

  METSCI_begin();
  BATTSCI_begin();

  LTC6804_initialize();

  TCCR1B = (TCCR1B & B11111000) | B00000001; // Set onboard fan PWM frequency to 31372 Hz (pins D11 & D12)
  //TCCR0B = (TCCR0B & B11111000) | B00000001; // JTSdebug: for PWM frequency of 62500 Hz D04 & D13. This hoses delay()!

  Serial.print(F("\n\nWelcome to LiBCM v" FW_VERSION "," BUILD_DATE "\n\n")); //memory efficient (see useful_tidbits.txt)
}

void loop()
{
	uint8_t keyStatus_now = digitalRead(PIN_KEY_ON);  //Get key position // executes in ~t=10 microseconds
	static uint8_t keyStatus_previous = 1; //JTS2doNow: See if setting '0' prevents BATTSCI P-code

	//---------------------------------------------------------------------------------------
	//This section executes in t=  5 microseconds when key state has NOT changed
	//This section executes in t= ?? milliseconds when key state has     changed to ON
	//This section executes in t= ?? milliseconds when key state has     changed to OFF

	//steps to perform when key state changes (on->off, off->on)
	if( keyStatus_now != keyStatus_previous)
	{

	  Serial.print(F("\nKey:")); //takes t=?? microseconds to execute
	  
	  if( keyStatus_now == 0 )
	  { //takes t=?? milliseconds to execute
	    Serial.print(F("OFF"));
	    LED(1,LOW);
	    BATTSCI_disable(); //Must disable BATTSCI when key is off to prevent backdriving MCM
	    METSCI_disable();
	    digitalWrite(PIN_FANOEM_LOW,LOW);
	    digitalWrite(PIN_I_SENSOR_EN,LOW); //disable current sensor & constant 5V load
	    lcd_displayOFF();
	    vPackSpoof_handleKeyOFF();
  
	  } else { //takes t=?? milliseconds to execute
	  	Serial.print(F("ON"));
	  	//vPackSpoof_handleKeyON(); //JTS2doNow: Figure out keyON VPIN spooging
	    BATTSCI_enable();
	    METSCI_enable();
	    digitalWrite(PIN_FANOEM_LOW,HIGH);
	    digitalWrite(PIN_I_SENSOR_EN,HIGH); //enable current sensor & constant 5V load
	    lcd_displayON();
	    //JTS2doLater: Add gridCharger_isPluggedIn(); if true, hang in while(keyON), to cause P-code (to alert user)
	    LED(1,HIGH);
	  }

	  keyStatus_previous = keyStatus_now;
	}

	//---------------------------------------------------------------------------------------
	//This section executes in t=8 microseconds when grid charger state has NOT changed

	//JTS2doLater: if key is on, don't run this code (make sure to turn charger off in keyON handler)
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
	    lcd_displayOFF();
	    LED(4,LOW);

	  } else {                          //grid charger was just plugged in
	    Serial.print(F("Plugged In"));
	    lcd_displayON();
	    LED(4,HIGH);
	  }
	}
	gridChargerPowered_previous = gridChargerPowered_now;

	//---------------------------------------------------------------------------------------

	if( (keyStatus_now == 1) || (gridChargerPowered_now == 1) ) //key is on or grid charger plugged in
	{
	  LTC6804_startCellVoltageConversion();  //executes in ~t=230 microseconds
	  //We don't immediately read the results afterwards because it takes a second to digitize
	  //In Coop Task setting we'll need to invoke reading n microseconds after this function is called

	  //---------------------------------------------------------------------------------------
  
	  LTC6804_readCellVoltages();	//individual cell results stored in 'cell_codes' array
	  														//executes in t=56 millisconds 
	  
	  //---------------------------------------------------------------------------------------
	  //This section executes in t=53 milliseconds

	  uint8_t packVoltage_actual  = LTC6804_getStackVoltage();
	  debugUSB_VpackActual_volts(packVoltage_actual);
	  uint8_t packVoltage_spoofed = (uint8_t)(packVoltage_actual*0.94);
	  debugUSB_VpackSpoofed_volts(packVoltage_spoofed);

	  //---------------------------------------------------------------------------------------

	  if( keyStatus_now ) //key is on
	  {
	    METSCI_processLatestFrame();
	    //executes in ~t=5 microseconds when MCM is NOT sending data to LiBCM
	    //executes in  t=? microseconds when MCM is     sending data to LiBCM
	    
	    int16_t packCurrent_actual = adc_batteryCurrent_Amps();
	    debugUSB_batteryCurrentActual_amps(packCurrent_actual);

	    int16_t packCurrent_spoofed;

			if( ENABLE_CURRENT_HACK )
			{
				packCurrent_spoofed = (int16_t)(packCurrent_actual * 0.7); //140% current hack = tell MCM 70% actual
				debugUSB_batteryCurrentSpoofed_amps(packCurrent_spoofed);



			} else {
				packCurrent_spoofed = packCurrent_actual;
			}

	    BATTSCI_setPackCurrent(packCurrent_spoofed);
    	
    	lcd_printPower(packVoltage_actual, packCurrent_actual);

	  	vPackSpoof_updateVoltage(packVoltage_actual, packVoltage_spoofed);

	  	BATTSCI_sendFrames();

	  }
	  //---------------------------------------------------------------------------------------

	}
	else
	{
	  //key is off & grid charger unplugged
		//JTS2doLater: Balance cells
	}
	blinkLED2(); //Heartbeat
	debugUSB_printLatest_data();
}
