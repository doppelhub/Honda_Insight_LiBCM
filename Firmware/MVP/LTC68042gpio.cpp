//Copyright 2021(c) John Sullivan
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

uint8_t ADAX[2]; //GPIO conversion command


//---------------------------------------------------------------------------------------

//Start GPIO conversion
void LTC6804_adax()
{
  uint8_t cmd[4];
  uint16_t temp_pec;


  //Load adax command into cmd array
  cmd[0] = ADAX[0];
  cmd[1] = ADAX[1];

  //Calculate adax cmd PEC and load pec into cmd array
  temp_pec = LTC68042configure_calcPEC15(2, ADAX);
  cmd[2] = (uint8_t)(temp_pec >> 8);
  cmd[3] = (uint8_t)(temp_pec);

  LTC68042configure_wakeupIsoSPI(); //Guarantees LTC6804 isoSPI port is awake.

  //send broadcast adax command to LTC6804 stack
  digitalWrite(PIN_SPI_CS,LOW);
  LTC68042configure_spiWrite(4,cmd);
  digitalWrite(PIN_SPI_CS,HIGH);
}

//---------------------------------------------------------------------------------------


//Reads and parses aux voltages from LTC6804 registers into 'aux_codes' variable.
int8_t LTC6804_rdaux(uint8_t reg, //controls which aux voltage register to read (0=all, 1=A, 2=B)
                     uint8_t total_ic,
                     uint16_t aux_codes[][6],
                     uint8_t addr_first_ic
                    )
{
  const uint8_t NUM_RX_BYTES = 8;
  const uint8_t NUM_BYTES_IN_REG = 6;
  const uint8_t GPIO_IN_REG = 3;

  uint8_t *data;
  uint8_t data_counter = 0;
  int8_t pec_error = 0;
  uint16_t received_pec;
  uint16_t data_pec;
  data = (uint8_t *) malloc((NUM_RX_BYTES*total_ic)*sizeof(uint8_t));

  if (reg == 0)
  { //Read GPIO voltage registers A-B for every IC in the stack
    for (uint8_t gpio_reg = 1; gpio_reg<3; gpio_reg++) //executes once for each aux voltage register
    {
      data_counter = 0;
      LTC6804_rdaux_reg(gpio_reg, total_ic,data, addr_first_ic);
      for (uint8_t current_ic = 0 ; current_ic < total_ic; current_ic++) //executes once for each LTC6804
      {
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
        data_counter=data_counter+2;
      }
    }
  } else {
    //Read single GPIO voltage register for all ICs in stack
    LTC6804_rdaux_reg(reg, total_ic, data, addr_first_ic);
    for (int current_ic = 0 ; current_ic < total_ic; current_ic++) // executes for every LTC6804 in the stack
    {
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
  free(data);
  return (pec_error);
}

//---------------------------------------------------------------------------------------


/***********************************************//**
 \brief Read the raw data from the LTC6804 auxiliary register

 The function reads a single GPIO voltage register and stores the read data
 in the *data point as a byte array. This function is rarely used outside of
 the LTC6804_rdaux() command.

 @param[in] uint8_t reg; This controls which GPIO voltage register is read back.

          1: Read back auxiliary group A

          2: Read back auxiliary group B


 @param[in] uint8_t total_ic; This is the number of ICs in the stack

 @param[out] uint8_t *data; An array of the unparsed aux codes
 *************************************************/
void LTC6804_rdaux_reg(uint8_t reg,
                       uint8_t total_ic,
                       uint8_t *data,
                       uint8_t addr_first_ic
                      )
{
  uint8_t cmd[4];
  uint16_t cmd_pec;

  //1
  if (reg == 1)
  {
    cmd[1] = 0x0C;
    cmd[0] = 0x00;
  }
  else if (reg == 2)
  {
    cmd[1] = 0x0e;
    cmd[0] = 0x00;
  }
  else
  {
    cmd[1] = 0x0C;
    cmd[0] = 0x00;
  }
  //2
  cmd_pec = LTC68042configure_calcPEC15(2, cmd);
  cmd[2] = (uint8_t)(cmd_pec >> 8);
  cmd[3] = (uint8_t)(cmd_pec);

  //3
  LTC68042configure_wakeupIsoSPI(); //This will guarantee that the LTC6804 isoSPI port is awake, this command can be removed.
  //4
  for (int current_ic = 0; current_ic<total_ic; current_ic++)
  {
    cmd[0] = 0x80 + ( (current_ic + addr_first_ic) << 3); //Setting address
    cmd_pec = LTC68042configure_calcPEC15(2, cmd);
    cmd[2] = (uint8_t)(cmd_pec >> 8);
    cmd[3] = (uint8_t)(cmd_pec);
    digitalWrite(PIN_SPI_CS,LOW);
    LTC68042configure_spiWriteRead(cmd,4,&data[current_ic*8],8);
    digitalWrite(PIN_SPI_CS,HIGH);
  }
}
/*
  LTC6804_rdaux_reg Function Process:
  1. Determine Command and initialize command array
  2. Calculate Command PEC
  3. Wake up isoSPI, this step is optional
  4. Send Global Command to LTC6804 stack
*/

//---------------------------------------------------------------------------------------