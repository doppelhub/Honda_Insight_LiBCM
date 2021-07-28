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

//JTS2do: Move pindefs to a separate header file
#define HW_REVB

#ifdef HW_REVB
  #define PIN_BATTCURRENT A0
  #define PIN_FANOEM_LOW A1
  #define PIN_FANOEM_HI A2
  #define PIN_TEMP_YEL A3
  #define PIN_TEMP_GRN A4
  #define PIN_TEMP_WHT A5
  #define PIN_TEMP_BLU A6
  #define PIN_VPIN_IN A7
  #define PIN_TURNOFFLiBCM A8
  #define PIN_HMI_EN A9
  #define PIN_BATTSCI_DE A10
  #define PIN_BATTSCI_REn A11
  #define PIN_LED1 A12
  #define PIN_LED2 A13
  #define PIN_LED3 A14
  #define PIN_LED4 A15
  
  #define PIN_METSCI_DE 2
  #define PIN_METSCI_REn 3
  #define PIN_VPIN_OUT_PWM 4
  #define PIN_SPI_EXT_CS 5 
  #define PIN_TEMP_EN 6
  #define PIN_CONNE_PWM 7
  #define PIN_GRID_PWM 8
  #define PIN_GRID_SENSE 9
  #define PIN_GRID_EN 10
  #define PIN_FAN_PWM 11
  #define PIN_I_SENSOR_EN 12
  #define PIN_KEY_ON 13
  #define PIN_SPI_CS SS
  #define PIN_GPIO1 48
  #define PIN_USER_SW 49
  
  //Serial3
  #define METSCI_TX 14
  #define METSCI_RX 15
  
  //Serial2
  #define BATTSCI_TX 16
  #define BATTSCI_RX 17
  
  //Serial1
  #define HMI_TX 18
  #define HMI_RX 19
  
  #define DEBUG_SDA 20
  #define DEBUG_CLK 21
#endif

#include <Arduino.h>
#include <stdint.h>
#include "LT_SPI.h"
#include <SPI.h>
#include "LTC68042.h"

struct packetTypes
{
  uint8_t latestE6Packet_assistLevel;
  uint8_t latestB4Packet_engine;
  uint8_t latestB3Packet_engine;
  uint8_t latestE1Packet_SoC;
} METSCI_Packets;

void setup()
{
	//Prevent LiBCM from turning off the 12V->5V DCDC
	pinMode(PIN_TURNOFFLiBCM,OUTPUT);
	digitalWrite(PIN_TURNOFFLiBCM,LOW);
 
	pinMode(PIN_I_SENSOR_EN,OUTPUT);
	digitalWrite(PIN_I_SENSOR_EN,LOW);

	pinMode(PIN_LED1,OUTPUT);
	digitalWrite(PIN_LED1,HIGH);
	pinMode(PIN_LED2,OUTPUT);
	pinMode(PIN_LED3,OUTPUT);
	pinMode(PIN_LED4,OUTPUT);

	pinMode(PIN_CONNE_PWM,OUTPUT);
  analogWrite(PIN_CONNE_PWM,0);
	pinMode(PIN_FAN_PWM,OUTPUT);
	pinMode(PIN_FANOEM_LOW,OUTPUT);
	pinMode(PIN_FANOEM_HI,OUTPUT);
	pinMode(PIN_GRID_EN,OUTPUT);
	pinMode(PIN_VPIN_OUT_PWM,OUTPUT);
  Serial.print("\nPins Set");

  pinMode(PIN_HMI_EN,OUTPUT);
  digitalWrite(PIN_HMI_EN,HIGH); //turn on 4x20 display
  delay(10); //wait for 4x20 to turn on
  Serial.print("\nPass: IO Config");

	analogReference(EXTERNAL); //use 5V AREF pin, which is coupled to filtered VCC

	Serial.begin(115200);	//USB
  Serial.print("\nPass: USB init"); delay(100);
  METSCI_begin();
  Serial.print("\nPass: METSCI init"); delay(100);
  BATTSCI_begin();
  Serial.print("\nPass: BATTSCI init"); delay(100);
  Serial.print("\nPass: one second delay"); delay(1000);
  
  LTC6804_initialize();
  Serial.print("\nPass: LTC6804 init"); delay(100);

  TCCR1B = (TCCR1B & B11111000) | B00000001; // Set onboard fan PWM frequency to 31372 Hz (pins D11 & D12)
  Serial.print("\nPass: set D11/D12 PWM frequency"); delay(100);
    
	Serial.print(F("\n\nPeter's LiBCM HW Debug 2021JUL27\n\n")); delay(100);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop()
{  
  //Get key position status
  uint8_t keyStatus_now = digitalRead(PIN_KEY_ON);
  static uint8_t keyStatus_previous;
  Serial.print("\nPass(): Key is "); delay(100);
  if(keyStatus_now)
  {
    Serial.print("ON"); delay(100);
  } else {
    Serial.print("OFF"); delay(100);
  }

  //steps to perform when key state changes (on->off, off->on)
  if( keyStatus_now != keyStatus_previous)
  {
    Serial.print(F("\nKey is: ")); delay(100);
    LTC6804_isoSPI_errorCountReset();

    if( keyStatus_now == 0 )
    {
      Serial.print(F("OFF"));
      BATTSCI_disable(); //Must disable BATTSCI when key is off to prevent backdriving MCM
      METSCI_disable();
      digitalWrite(PIN_FANOEM_LOW,LOW);
      digitalWrite(PIN_I_SENSOR_EN,LOW); //disable current sensor & constant 5V load
    } else {
      Serial.print(F("ON"));
      BATTSCI_enable();
      METSCI_enable();
      digitalWrite(PIN_FANOEM_LOW,HIGH);
      digitalWrite(PIN_I_SENSOR_EN,HIGH); //enable current sensor & constant 5V load
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

    } else {                          //grid charger was just plugged in
      Serial.print(F("Plugged In"));
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
    Serial.print("\nPass: LTC6804 Conversion Started"); delay(100);
  
    //---------------------------------------------------------------------------------------

  	//get 64x oversampled current sensor value
    uint16_t ADC_oversampledAccumulator = 0;
    for(int ii=0; ii<64; ii++)  //This takes ~112 us per run (7.2 ms total)
    { 
      ADC_oversampledAccumulator += analogRead(PIN_BATTCURRENT);
    }
    Serial.print("\nPass: Current Sampled"); delay(100);
    
    int16_t ADC_oversampledResult = int16_t( (ADC_oversampledAccumulator >> 6) );
    Serial.print(F("\n\nRaw ADC result is: "));
    Serial.print( String(ADC_oversampledResult) );  
    
  	//convert current sensor result into approximate amperage for MCM & user-display
    //don't use this result for current accumulation... it's not accurate enough
  	int16_t battCurrent_amps = ( (ADC_oversampledResult * 13) >> 6) - 67; //Accurate to within 3.7 amps of actual value
  	Serial.print(F(" counts, which is: "));
    Serial.print( String(battCurrent_amps) );
    Serial.print(F(" amps.  Telling MCM current is: "));
    
    battCurrent_amps = (int16_t)(battCurrent_amps * 0.7); //140% current output required telling MCM 70% of current value
    Serial.print( String(battCurrent_amps) );


    //---------------------------------------------------------------------------------------

  	LTC6804_getCellVoltages(); //individual cell results stored in 'cell_codes' array 
  	
    //sum all 48 cells
  	uint8_t stackVoltage = LTC6804_getStackVoltage();

    //---------------------------------------------------------------------------------------

    if( keyStatus_now ) //key is on
    {
    	//METSCI Decoding
      METSCI_Packets = METSCI_getLatestFrame();
      Serial.print("\nMETSCI E6: " + String(METSCI_Packets.latestE6Packet_assistLevel,HEX) +
                          ", B3: " + String(METSCI_Packets.latestB3Packet_engine,HEX) +
                          ", B4: " + String(METSCI_Packets.latestB4Packet_engine,HEX) +
                          ", E1: " + String(METSCI_Packets.latestE1Packet_SoC,HEX) );

      //---------------------------------------------------------------------------------------  
    	
    	//Send BATTSCI packets to MCM
    	//Need to limit how often this occurs
      BATTSCI_sendFrames(METSCI_Packets, stackVoltage, battCurrent_amps);
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
}
