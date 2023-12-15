//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM
#include "libcm.h"

//////////////////////////////////////////////////////////////////////////////////

void LiControl_handler(void)
{
    ;
}

//////////////////////////////////////////////////////////////////////////////////

void LiControl_begin(void)
{
    pinMode(PIN_GPIO0_CS_MIMA,OUTPUT);
    digitalWrite(PIN_GPIO0_CS_MIMA,HIGH);
}