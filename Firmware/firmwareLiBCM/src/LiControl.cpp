//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

#include "libcm.h"

/////////////////////////////////////////////////////////////////////////////////////////

void LiControl_handler(void)
{
    static uint32_t lastSend = 0;

    if ((millis() - lastSend) < 100)
    {
        return;
    }

    lastSend = millis();

    uint8_t tx = SoC_getBatteryStateNow_percent();
    uint8_t rx;

    spi_transfer_byte(
        PIN_GPIO0_CS_MIMA,
        tx,
        &rx
    );
}

/////////////////////////////////////////////////////////////////////////////////////////

void LiControl_begin(void)
{
    pinMode(PIN_GPIO0_CS_MIMA, OUTPUT);
    digitalWrite(PIN_GPIO0_CS_MIMA, HIGH);

    spi_enable(SPI_CLOCK_DIV16);
}

////////////////////////////////////////////////////////////////////////
