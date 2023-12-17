/*
LT_SPI: Routines to communicate with ATmega328P's hardware SPI port.

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

/*! @file
    @ingroup LT_SPI
    Library Header File for LT_SPI: Routines to communicate with ATmega328P's hardware SPI port.
*/

#ifndef LT_SPI_H
    #define LT_SPI_H

    #include <stdint.h>
    #include <SPI.h>

    // Uncomment the following to use functions that implement LTC SPI routines

    //SPI CLOCK DIVIDER CONSTANTS
        //#define SPI_CLOCK_DIV2    0x04 //8 Mhz
        //#define SPI_CLOCK_DIV4    0x00 //4 Mhz
        //#define SPI_CLOCK_DIV8    0x05 //2 Mhz
        //#define SPI_CLOCK_DIV16   0x01 //1 Mhz
        //#define SPI_CLOCK_DIV32   0x06 //500 khz
        //#define SPI_CLOCK_DIV64   0x02 //250 khz
        //#define SPI_CLOCK_DIV128  0x03 //125 khz

    //SPI HARDWARE MODE CONSTANTS
        //#define SPI_MODE0 0x00
        //#define SPI_MODE1 0x04
        //#define SPI_MODE2 0x08
        //#define SPI_MODE3 0x0C
  
    //SPI SET MASKS
        //#define SPI_MODE_MASK    0x0C //CPOL = bit 3, CPHA = bit 2 on SPCR
        //#define SPI_CLOCK_MASK   0x03 //SPR1 = bit 1, SPR0 = bit 0 on SPCR
        //#define SPI_2XCLOCK_MASK 0x01 //SPI2X = bit 0 on SPSR
 
    //read and send a byte
    void spi_transfer_byte(uint8_t cs_pin,
                           uint8_t tx,     //byte to transmit
                           uint8_t *rx     //byte received
                          );

    //read and send a word
    void spi_transfer_word(uint8_t cs_pin,
                           uint16_t tx,     //byte to transmit
                           uint16_t *rx     //byte received
                          );

    //read and send byte array
    void spi_transfer_block(uint8_t cs_pin,
                            uint8_t *tx,     //byte to transmit
                            uint8_t *rx,     //byte received
                            uint8_t length
                           );

    //setup SPI hardware
    void spi_enable(uint8_t spi_clock_divider);
                 
    //disable SPI hardware port
    void spi_disable();

    void spi_write(int8_t data);

    int8_t spi_read(int8_t data);

#endif
