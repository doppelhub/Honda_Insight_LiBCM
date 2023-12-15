/*
LT_SPI implements the low level master SPI bus routines using
the 328p's hardware SPI port.

SPI Frequency = (CPU Clock frequency)/(16+2(TWBR)*Prescaler)
SPCR = SPI Control Register (SPIE SPE DORD MSTR CPOL CPHA SPR1 SPR0)
SPSR = SPI Status Register (SPIF WCOL - - - - - SPI2X)

Data Modes:
CPOL  CPHA  Leading Edge    Trailing Edge
0      0    sample rising   setup falling
0      1    setup rising    sample falling
1      0    sample falling  setup rising
1      1    sample rising   setup rising

CPU Frequency = 16MHz on Arduino Uno
SCK Frequency
SPI2X  SPR1  SPR0  Frequency  Uno_Frequency
  0      0     0     fosc/4     4 MHz
  0      0     1     fosc/16    1 MHz
  0      1     0     fosc/64    250 kHz
  0      1     1     fosc/128   125 kHz
  0      0     0     fosc/2     8 MHz
  0      0     1     fosc/8     2 MHz
  0      1     0     fosc/32    500 kHz

Copyright 2018(c) Analog Devices, Inc.

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
 - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in
   the documentation and/or other materials provided with the
   distribution.
 - Neither the name of Analog Devices, Inc. nor the names of its
   contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.
 - The use of this software may or may not infringe the patent rights
   of one or more patent holders.  This license does not release you
   from the requirement that you obtain separate licenses from these
   patent holders to use this software.
 - Use of the software either in source or binary form, must be run
   on or directly connected to an Analog Devices Inc. component.

THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <Arduino.h>
#include <stdint.h>
#include <SPI.h>
#include "LT_SPI.h"

void spi_transfer_byte(uint8_t cs_pin, uint8_t tx, uint8_t *rx)
{
    digitalWrite(cs_pin,LOW);
    *rx = SPI.transfer(tx); //read and send byte
    digitalWrite(cs_pin,HIGH);
}

/////////////////////////////////////////////////////////////////////////////////////////

void spi_transfer_word(uint8_t cs_pin, uint16_t tx, uint16_t *rx)
{
    union { uint8_t b[2]; uint16_t w; } data_tx;
    union { uint8_t b[2]; uint16_t w; } data_rx;

    data_tx.w = tx;

    digitalWrite(cs_pin,LOW);

    data_rx.b[1] = SPI.transfer(data_tx.b[1]);  //read and send MSB
    data_rx.b[0] = SPI.transfer(data_tx.b[0]);  //read and send LSB

    *rx = data_rx.w;

    digitalWrite(cs_pin,HIGH);
}

/////////////////////////////////////////////////////////////////////////////////////////

//read and send byte array
void spi_transfer_block(uint8_t cs_pin, uint8_t *tx, uint8_t *rx, uint8_t length)
{
    int8_t i;

    digitalWrite(cs_pin,LOW);
    for (i=(length-1);  i >= 0; i--) { rx[i] = SPI.transfer(tx[i]); }
    digitalWrite(cs_pin,HIGH);
}

/////////////////////////////////////////////////////////////////////////////////////////

//setup hardware for SPI communication
//must call before using other SPI functions
//configures SCK frequency using constant defined in header file
void spi_enable(uint8_t spi_clock_divider)
{
    //pinMode(SCK, OUTPUT);
    //pinMode(MOSI, OUTPUT);
    //pinMode(QUIKEVAL_CS, OUTPUT);
    SPI.begin();
    SPI.setClockDivider(spi_clock_divider);
}

/////////////////////////////////////////////////////////////////////////////////////////

void spi_disable() { SPI.end(); }

/////////////////////////////////////////////////////////////////////////////////////////

void spi_write(int8_t  data)  // Byte to be written to SPI port
{
    #if defined(ARDUINO_ARCH_AVR)
        SPDR = data;                  //start the SPI transfer
        while (!(SPSR & _BV(SPIF)));  //wait for transfer to complete
    #else
        SPI.transfer(data);
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////

//read and write data byte
//return read data byte
int8_t spi_read(int8_t data) //'data' is byte to write
{
    #if defined(ARDUINO_ARCH_AVR)
        SPDR = data;                  //start SPI transfer
        while (!(SPSR & _BV(SPIF)));  //wait for transfer to complete
        return SPDR;                  //return read data
    #else
        return SPI.transfer(data);
    #endif
}

/////////////////////////////////////////////////////////////////////////////////////////
