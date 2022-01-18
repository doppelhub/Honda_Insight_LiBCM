//This proof of concept shows how to:
//  -determine whether new firmware was uploaded since the last CPU reset (by comparing build dates) 
//  -store a device's runtime in EEPROM
//  -prevent code execution if the firmware hasn't been updated recently enough
//  -allow code execution once new firmware has been loaded

#include <EEPROM.h>

#define BYTES_IN_DATE 12

//store the day customer compiled the source code in program memory
const uint8_t COMPILE_DATE_PROGRAM[BYTES_IN_DATE]= __DATE__; //Format: Mmm DD YYYY //Ex: Jan 23 2022 //Mar  5 2022

//EEPROM address map:
const uint16_t EEPROM_ADDRESS_COMPILE_DATE       = 0x000; //EEPROM range is 0x000:0x00B (12B)
const uint16_t EEPROM_ADDRESS_HOURS_SINCE_UPDATE = 0x010; //EEPROM range is 0x00C:0x00D ( 2B)

//compile date previously stored in EEPROM (the last time the firmware was updated)
uint8_t compileDateEEPROM[BYTES_IN_DATE] = {};
uint16_t hoursSinceLastFirmwareUpdate = 0;

////////////////////////////////////////////////////////////////////////////////////

//compare compile date to EEPROM date
//if dates are different, then firmware was just updated by user
bool wasFirmwareJustUpdated(void)
{
  bool areDatesIdentical = true;
  Serial.print("\nCompare Dates: (EEPROM)(program)");
  for(uint8_t ii = 0; ii < BYTES_IN_DATE; ii++)
  {
    if(compileDateEEPROM[ii] != COMPILE_DATE_PROGRAM[ii]) { areDatesIdentical = false; }
    Serial.print("\n(" + String(compileDateEEPROM[ii]) + ")(" + String(COMPILE_DATE_PROGRAM[ii]) + ")");  
  }
  if(areDatesIdentical == true) { Serial.print("\nFirmware not Updated"); return false; }
  else                          { Serial.print("\nFirmware was Updated"); return  true; }
}

////////////////////////////////////////////////////////////////////////////////////

void setup()
{
  Serial.begin(115200);
  Serial.print("\ntest started");
  delay(100);

  //read compileDate array from EEPROM
  Serial.print("\n");
  for(int ii = 0; ii < BYTES_IN_DATE; ii++)
  {
    compileDateEEPROM[ii] = EEPROM.read(ii + EEPROM_ADDRESS_COMPILE_DATE);
    Serial.print(compileDateEEPROM[ii]);
    Serial.print(' ');
  }

  //read hoursSinceLastFirmwareUpdate from EEPROM
  hoursSinceLastFirmwareUpdate  = ( EEPROM.read(EEPROM_ADDRESS_HOURS_SINCE_UPDATE) << 8 ); //retrieve upper byte
  hoursSinceLastFirmwareUpdate +=   EEPROM.read(EEPROM_ADDRESS_HOURS_SINCE_UPDATE + 1); //retrieve lower byte
  
  Serial.print("\nHours since last update:" + String(hoursSinceLastFirmwareUpdate) );

  if(wasFirmwareJustUpdated() == true)
  {
    //customer just loaded new firmware, 

    //reset hour counter to zero (uint16_t)
    EEPROM.update(EEPROM_ADDRESS_HOURS_SINCE_UPDATE    , 0); //clear lower byte
    EEPROM.update(EEPROM_ADDRESS_HOURS_SINCE_UPDATE + 1, 0); //clear upper byte

    //store new compile date in EEPROM (so we can compare again on future powerups)
    for(int ii = 0; ii < BYTES_IN_DATE; ii++)
    {
      EEPROM.update( (ii + EEPROM_ADDRESS_COMPILE_DATE), COMPILE_DATE_PROGRAM[ii]);
    }
   
    //continue to main()
  }
  else
  {
    //firmware did not change since last powerup (i.e. user didn't update the firmware)
    
    //#define HOURS_IN_40_DAYS (40*24)
    #define HOURS_IN_40_DAYS 300 //test
    
    if(hoursSinceLastFirmwareUpdate > HOURS_IN_40_DAYS)
    {
      //lcd4x20("UPDATE FIRMWARE!")
      Serial.print("\nUPDATE FIRMWARE!");
      while(1) { ; } //THIS DOES NOT RETURN!
    } 
  }
}

void loop()
{
  
  for(uint16_t hourCount = hoursSinceLastFirmwareUpdate; hourCount < 500; hourCount++)
  {
    if( (hourCount % 10) == 0) { Serial.print("\n"); }    
    
    Serial.print(String(hourCount) + " ");
    delay(200);

    //update 'hours' device is on in EEPROM
    EEPROM.update(EEPROM_ADDRESS_HOURS_SINCE_UPDATE    , highByte(hourCount) );
    EEPROM.update(EEPROM_ADDRESS_HOURS_SINCE_UPDATE + 1,  lowByte(hourCount) );
  }

  while(1) { ; } //hang here forever (to prevent wearing out EEPROM)
}
