//handles all communication with lcd display(s) (4x20 and/or Nextion)

#include "libcm.h"



////////////////////////////////////////////////////////////////////////

void lcd_initialize(void)
{
  #ifdef I2C_LIQUID_CRYSTAL
    lcd2.begin();
  #endif
  #ifdef I2C_TWI
    lcd2.begin(20,4);
  #endif
}

////////////////////////////////////////////////////////////////////////