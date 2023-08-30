//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//LTC6804 gpio (e.g. temperature) data and various functions

#include "libcm.h"

//Stores returned aux voltage
//aux_codes[n][0] = GPIO1
//aux_codes[n][1] = GPIO2
//aux_codes[n][2] = GPIO3
//aux_codes[n][3] = GPIO4
//aux_codes[n][4] = GPIO5
//aux_codes[n][5] = Vref
uint16_t aux_codes[TOTAL_IC][6];


//---------------------------------------------------------------------------------------


//Start GPIO conversion
void LTC6804_adax()
{
  uint8_t cmd[4];

  //JTS2do: Rewrite with #define logic
  uint8_t ADAX[2] { ((MD_NORMAL & 0x02) >> 1) + 0x04,
                    ((MD_NORMAL & 0x01) << 7) + 0x60 + AUX_CH_ALL };

  //Load adax command into cmd array
  cmd[0] = ADAX[0];
  cmd[1] = ADAX[1];

  //Calculate adax cmd PEC and load pec into cmd array
  uint16_t temp_pec = LTC68042configure_calcPEC15(2, ADAX);
  cmd[2] = (uint8_t)(temp_pec >> 8);
  cmd[3] = (uint8_t)(temp_pec);

  //send broadcast adax command to LTC6804 pack
  LTC68042configure_spiWrite(4,cmd);
}

//---------------------------------------------------------------------------------------

//read a single GPIO voltage register in a single IC and store the read data in *data
//only used in LTC6804_rdaux()
void LTC6804_rdaux_reg(uint8_t reg, //GPIO voltage register to read back (1:A, 2:B)
                       uint8_t current_ic,
                       uint8_t *data, //array of the unparsed aux codes
                       uint8_t addr_first_ic )
{
  uint8_t cmd[4];
  
  cmd[0] = 0x80 + ( (current_ic + addr_first_ic) << 3); //Set IC address

  if      (reg == 1) { cmd[1] = 0x0C; }
  else if (reg == 2) { cmd[1] = 0x0e; }
  else               { cmd[1] = 0x0C; }
  
  uint16_t cmd_pec = LTC68042configure_calcPEC15(2, cmd);
  cmd[2] = (uint8_t)(cmd_pec >> 8);
  cmd[3] = (uint8_t)(cmd_pec);

  LTC68042configure_spiWriteRead(cmd,4,data,8);
}

//---------------------------------------------------------------------------------------

//Read and parse aux voltages from LTC6804 registers into 'aux_codes' variable.
int8_t LTC6804_rdaux(uint8_t reg, //controls which aux voltage register to read (0=all, 1=A, 2=B)
                     uint8_t total_ic,
                     uint8_t addr_first_ic )
{
  const uint8_t NUM_RX_BYTES = 8;
  const uint8_t NUM_BYTES_IN_REG = 6;
  const uint8_t GPIO_IN_REG = 3;

  uint8_t data[NUM_RX_BYTES];
  uint8_t data_counter = 0;
  int8_t pec_error = 0;
  uint16_t received_pec;
  uint16_t data_pec;

  if (reg == 0)
  { //Read GPIO voltage registers A-B for every IC in the pack
    for (uint8_t gpio_reg = 1; gpio_reg<3; gpio_reg++) //executes once for each aux voltage register
    {
      for (uint8_t current_ic = 0 ; current_ic < total_ic; current_ic++) //executes once for each LTC6804
      {
        LTC6804_rdaux_reg(gpio_reg, current_ic, data, addr_first_ic);

        data_counter = 0;
        //Parse raw GPIO voltage data in aux_codes array
        for (uint8_t current_gpio = 0; current_gpio< GPIO_IN_REG; current_gpio++) //Parses GPIO voltage stored in the register
        {
          aux_codes[current_ic][current_gpio +((gpio_reg-1)*GPIO_IN_REG)] = data[data_counter] + (data[data_counter+1]<<8);
          data_counter=data_counter+2;
        }
        //Verify PEC matches calculated value for each read register command
        received_pec = (data[data_counter]<<8)+ data[data_counter+1];
        data_pec = LTC68042configure_calcPEC15(NUM_BYTES_IN_REG, &data[current_ic*NUM_RX_BYTES]);
        if (received_pec != data_pec)
        {
          pec_error = 1;
        }
      }
    }
  } else {
    //Read single GPIO voltage register for all ICs in pack
    for (int current_ic = 0 ; current_ic < total_ic; current_ic++) // executes for every LTC6804 in the pack
    {
      LTC6804_rdaux_reg(reg, current_ic, data, addr_first_ic);

      data_counter = 0;
      //Parse raw GPIO voltage data in aux_codes array
      for (int current_gpio = 0; current_gpio<GPIO_IN_REG; current_gpio++)  // This loop parses the read back data. Loops
      {
        // once for each aux voltage in the register
        aux_codes[current_ic][current_gpio +((reg-1)*GPIO_IN_REG)] = 0x0000FFFF & (data[data_counter] + (data[data_counter+1]<<8));
        data_counter=data_counter+2;
      }
      //Verify PEC matches calculated value for each read register command
      received_pec = (data[data_counter]<<8) + data[data_counter+1];
      data_pec = LTC68042configure_calcPEC15(6, &data[current_ic*8]);
      if (received_pec != data_pec)
      {
        pec_error = 1;
      }
    }
  }
  return (pec_error);
}

//---------------------------------------------------------------------------------------

bool LTC6804gpio_areAllVoltageReferencesPassing(void)
{
    //JTS2doNow: Add to keyOFF routine
    //verify LTC6804 VREF is in bounds
    LTC6804_adax();
    delay(5);
    LTC6804_rdaux(0,TOTAL_IC,FIRST_IC_ADDR);

    bool didTestPass = true;

    for(uint8_t ii = 0; ii<TOTAL_IC; ii++)
    {
      uint16_t countsVREF = aux_codes[ii][5];

      Serial.print(F("\nIC"));
      Serial.print(String(ii));
      Serial.print(F(" VREF2 is "));
      Serial.print(String(countsVREF));
      Serial.print(F(", "));

      if((countsVREF < 30150) && (countsVREF > 29850)) { Serial.print("ok"); }
      else                                             { Serial.print("FAIL"); didTestPass = false; }
    }

    return didTestPass;
}