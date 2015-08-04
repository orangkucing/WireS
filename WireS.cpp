/*
    WireS - Slave only I2C library for ATtiny1634, ATtiny441/841, and ATtiny828.
    Copyright (c) 2015 by Hisashi Ito (info at mewpro.cc)

    ------------------------------------------------------------------------------------------------------
    Some code segments derived from:
    i2c_t3 - I2C library for Teensy 3.0/3.1/LC

    - (v8) Modified 02Apr15 by Brian (nox771 at gmail.com)

    and

    TwoWire.cpp - TWI/I2C library for Wiring & Arduino
    Copyright (c) 2006 Nicholas Zambetti.  All right reserved.
    Modified 2012 by Todd Krein (todd@krein.org) to implement repeated starts
    ------------------------------------------------------------------------------------------------------
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

// This library should work with the following AVR microcontrollers
// having an I2C compatible, two-wire slave only interface.

#if defined(__AVR_ATtiny1634__) || defined(__AVR_ATtiny841__) || defined(__AVR_ATtiny441__) || defined(__AVR_ATtiny828__) || defined(__AVR_ATtiny40__) || defined(__AVR_ATtiny20__)

// Disclaimer: The author only confirmed the library to work with ATtiny1634, ATtiny841, and ATtiny828.

#include <avr/io.h>
#include "WireS.h"

//#define TWI_HIGH_NOISE_MODE _BV(TWHNM) // only for ATtiny441/841 or ATtiny828
#define TWI_HIGH_NOISE_MODE 0

#ifndef TWAE // avr/iotn1634.h doesn't define
#define TWAE 0
#endif
 
// use MSB of i2c->Addr as a flag to indicate 10-bit slave address is on receiving
#define SET_TENBIT   do { i2c->Addr |= 0x8000; } while(0)
#define CLEAR_TENBIT do { i2c->Addr &= 0x7FFF; } while(0)
#define IS_TENBIT (i2c->Addr & 0x8000)

struct i2cStruct i2c_tinyS::i2cData;

// ------------------------------------------------------------------------------------------------------
// Constructor/Destructor
//
i2c_tinyS::i2c_tinyS()
{
    i2c = &i2cData;
}

i2c_tinyS::~i2c_tinyS()
{
}


// ------------------------------------------------------------------------------------------------------
// Initialize I2C - initializes I2C as two address or address range Slave
// return: none
// parameters:
//      address = 7bit address for specifying 1st Slave address
//      mask = 8bit integer;
//             if bit[0]==1 then bit[7:1] 2nd Slave address
//             if bit[0]==0 then bit[7:1] mask bits for address
//
void i2c_tinyS::begin_(struct i2cStruct* i2c, uint8_t address, uint8_t mask)
{
    I2C_INTR_FLAG_INIT; // init I2C interrupt flag if used
    TWSA = (address << 1);
    TWSAM = mask; // if mask == 0 then only one address
    i2c->startCount = -1;
    TWSCRA = (_BV(TWSHE) | _BV(TWDIE) | _BV(TWASIE) | _BV(TWEN) | _BV(TWSIE));
}


// ------------------------------------------------------------------------------------------------------
// Write - write data to Tx buffer
// return: 1=success, 0=fail
// parameters:
//      data = data byte
//
size_t i2c_tinyS::write(uint8_t data)
{
    if(i2c->txBufferLength < I2C_BUFFER_LENGTH)
    {
        i2c->Buffer[i2c->txBufferLength++] = data;
        return 1;
    }
    return 0;
}


// ------------------------------------------------------------------------------------------------------
// Write Array - write length number of bytes from data array to Tx buffer
// parameters:
//      data = pointer to uint8_t array of data
//      length = number of bytes to write
//
size_t i2c_tinyS::write(const uint8_t* data, size_t quantity)
{
    if(i2c->txBufferLength < I2C_BUFFER_LENGTH)
    {
        size_t avail = I2C_BUFFER_LENGTH - i2c->txBufferLength;
        uint8_t* dest = i2c->Buffer + i2c->txBufferLength;

        if(quantity > avail)
        {
            quantity = avail; // truncate to space avail if needed
        }
        for(size_t count=quantity; count; count--)
            *dest++ = *data++;
        i2c->txBufferLength += quantity;
    }
}


// ------------------------------------------------------------------------------------------------------
// Read - returns next data byte (signed int) from Rx buffer
// return: data, -1 if buffer empty
//
int i2c_tinyS::read_(struct i2cStruct* i2c)
{
    if(i2c->rxBufferIndex >= i2c->rxBufferLength) return -1;
    return i2c->Buffer[i2c->rxBufferIndex++];
}


// ------------------------------------------------------------------------------------------------------
// Peek - returns next data byte (signed int) from Rx buffer without removing it from Rx buffer
// return: data, -1 if buffer empty
//
int i2c_tinyS::peek_(struct i2cStruct* i2c)
{
    if(i2c->rxBufferIndex >= i2c->rxBufferLength) return -1;
    return i2c->Buffer[i2c->rxBufferIndex];
}


// ------------------------------------------------------------------------------------------------------
// Read Byte - returns next data byte (uint8_t) from Rx buffer
// return: data, 0 if buffer empty
//
uint8_t i2c_tinyS::readByte_(struct i2cStruct* i2c)
{
    if(i2c->rxBufferIndex >= i2c->rxBufferLength) return 0;
    return i2c->Buffer[i2c->rxBufferIndex++];
}


// ------------------------------------------------------------------------------------------------------
// Peek Byte - returns next data byte (uint8_t) from Rx buffer without removing it from Rx buffer
// return: data, 0 if buffer empty
//
uint8_t i2c_tinyS::peekByte_(struct i2cStruct* i2c)
{
    if(i2c->rxBufferIndex >= i2c->rxBufferLength) return 0;
    return i2c->Buffer[i2c->rxBufferIndex];
}


// ======================================================================================================
// ------------------------------------------------------------------------------------------------------
// I2C Interrupt Service Routine
// ------------------------------------------------------------------------------------------------------
// ======================================================================================================

void i2c_isr_handler()
{
    struct i2cStruct *i2c = &(i2c_tinyS::i2cData);
    byte status = TWSSRA;
    if ((status & (_BV(TWC) | _BV(TWBE)))) {
        // Bus error or transmit collision
        i2c->startCount = -1;
        CLEAR_TENBIT;
        TWSSRA |= (_BV(TWASIF) | _BV(TWDIF) | _BV(TWBE)); // Release hold
        return;
    }
    if ((status & _BV(TWASIF)) || IS_TENBIT) {
        if ((status & _BV(TWAS))) {
            // A valid address has been received
            if (IS_TENBIT) {
                i2c->Addr = (((i2c->Addr & B110) << 7) | TWSD);
                // CLEAR_TENBIT;
            } else {
                i2c->Addr = TWSD;
                i2c->startCount++;
                if ((i2c->Addr & B11111001) == B11110000) {
                    SET_TENBIT;
                    TWSCRB = (B0011 | TWI_HIGH_NOISE_MODE); // Send ACK
                    return;
                }
            }
            if (i2c->user_onAddrReceive != (void *)NULL) {
                i2c->rxBufferIndex = 0;
                if (!i2c->user_onAddrReceive(i2c->Addr, i2c->startCount)) {
                    TWSCRB = (B0111 | TWI_HIGH_NOISE_MODE); // Send NACK
                    return;
                }
            }
            if ((status & _BV(TWDIR))) {
                // A master read operation is in progress
                i2c->txBufferLength = 0;
                if (i2c->user_onRequest != (void *)NULL) {
                    i2c->user_onRequest(); // load Tx buffer with data
                }
                i2c->txBufferIndex = 0;
            } else {
                // A master write operation is in progress
                i2c->rxBufferLength = 0;
            }
        } else {
            // Stop condition is detected
            if ((status & _BV(TWDIR))) {
                if (i2c->user_onStop != (void *)NULL) {
                    i2c->user_onStop();
                }
            } else {
                if (i2c->user_onReceive != (void *)NULL) {
                    i2c->rxBufferIndex = 0;
                    i2c->user_onReceive(i2c->rxBufferLength);
                }
            }
            i2c->startCount = -1;
            CLEAR_TENBIT;
            TWSSRA = _BV(TWASIF); // clear interrupt
            return;
        }
    } else if ((status & _BV(TWDIF))) {
        if ((status & _BV(TWDIR))) {
            // Send a data byte to master
            if (i2c->txBufferIndex < i2c->txBufferLength) {
                TWSD = i2c->Buffer[i2c->txBufferIndex++];
            } else {
                // buffer overrun
                TWSCRB = (B0010 | TWI_HIGH_NOISE_MODE); // Wait for any START condition
                return;
            }
        } else {
            // A data byte has been received
            if (i2c->rxBufferLength < I2C_BUFFER_LENGTH) {
                i2c->Buffer[i2c->rxBufferLength++] = TWSD;
            } else {
                // buffer overrun
                TWSCRB = (B0110 | TWI_HIGH_NOISE_MODE); // Send NACK and wait for any START condition
                return;
            }
        }
    }
    TWSCRB = (B0011 | TWI_HIGH_NOISE_MODE);
}

ISR(TWI_SLAVE_vect)
{
    I2C_INTR_FLAG_ON;
    i2c_isr_handler();
    I2C_INTR_FLAG_OFF;
}

// ------------------------------------------------------------------------------------------------------
// Instantiate
//
i2c_tinyS Wire  = i2c_tinyS();

#endif
