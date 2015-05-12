/*
    WireS - Slave only I2C library for ATtiny1634 and ATtiny841
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
#if !defined(WIRE_S_H) && (defined(__AVR_ATtiny1634__) || defined(__AVR_ATtiny841__) || defined(__AVR_ATtiny441__) || defined(__AVR_ATtiny828__) || defined(__AVR_ATtiny40__) || defined(__AVR_ATtiny20__)) 
#define WIRE_S_H

#include <inttypes.h>
#include "Arduino.h"

// ======================================================================================================
// == Start User Define Section =========================================================================
// ======================================================================================================

// ------------------------------------------------------------------------------------------------------
// Tx/Rx buffer sizes - modify these as needed.
//
#define I2C_BUFFER_LENGTH 64

// ------------------------------------------------------------------------------------------------------
// Interrupt flag - uncomment and set below to make the specified pin high whenever the
//                  I2C interrupt occurs.  This is useful as a trigger signal when using a logic analyzer.
//
//#define I2C_INTR_FLAG_PIN 6

// ======================================================================================================
// == End User Define Section ===========================================================================
// ======================================================================================================

// ------------------------------------------------------------------------------------------------------
// Interrupt flag setup
//
#if defined(I2C_INTR_FLAG_PIN)
    #define I2C_INTR_FLAG_INIT do             \
    {                                         \
        pinMode(I2C_INTR_FLAG_PIN, OUTPUT);   \
        digitalWrite(I2C_INTR_FLAG_PIN, LOW); \
    } while(0)

    #define I2C_INTR_FLAG_ON   do {digitalWrite(I2C_INTR_FLAG_PIN, HIGH);} while(0)
    #define I2C_INTR_FLAG_OFF  do {digitalWrite(I2C_INTR_FLAG_PIN, LOW);} while(0)
#else
    #define I2C_INTR_FLAG_INIT do{}while(0)
    #define I2C_INTR_FLAG_ON   do{}while(0)
    #define I2C_INTR_FLAG_OFF  do{}while(0)
#endif

// ------------------------------------------------------------------------------------------------------
// Main I2C data structure
//
struct i2cStruct
{
    uint8_t  Buffer[I2C_BUFFER_LENGTH];      // Tx/Rx Buffer                      (ISR)
    volatile size_t   rxBufferIndex;         // Rx Index                          (User&ISR)
    volatile size_t   rxBufferLength;        // Rx Length                         (ISR)
    volatile size_t   txBufferIndex;         // Tx Index                          (User&ISR)
    volatile size_t   txBufferLength;        // Tx Length                         (User&ISR)
    volatile char     startCount;            // repeated START count              (User&ISR)
    volatile uint16_t  Addr;                 // Tx/Rx address                     (User&ISR)
    boolean (*user_onAddrReceive)(uint16_t, uint8_t);  // Slave Addr Callback Function      (User)
    void (*user_onReceive)(size_t);          // Slave Rx Callback Function        (User)
    void (*user_onRequest)(void);            // Slave Tx Callback Function        (User)
    void (*user_onStop)(void);               // Stop Callback Function            (User)
};

extern "C" void i2c_isr_handler(struct i2cStruct* i2c);

class i2c_tinyS : public Stream
{
private:
    //
    // I2C data structures - these need to be static so "C" ISRs can use them
    //
    static struct i2cStruct i2cData;
    friend void i2c_isr_handler(void);

public:
    //
    // I2C structure pointer - this is a local, passed as an argument to base functions
    //                         since static functions cannot see it.
    struct i2cStruct* i2c;

    // ------------------------------------------------------------------------------------------------------
    // Constructor
    //
    i2c_tinyS();
    ~i2c_tinyS();

    // ------------------------------------------------------------------------------------------------------
    // Initialize I2C (base routine)
    //
    static void begin_(struct i2cStruct* i2c, uint8_t address, uint8_t mask);
    //
    // Initialize I2C (Slave) - initializes I2C as Slave mode using address
    // return: none
    // parameters:
    //      address = 7bit slave address of device
    //
    inline void begin(int address)
    {
        begin_(i2c, (uint8_t)address, 0);
    }
    //
    // Initialize I2C - initializes I2C as two address Slave or address range Slave
    // return: none
    // parameters:
    //      address = 7bit for specifying 1st address
    //      mask = if bit[0]==1 then bit[7:1] for specifying 2nd address
    //             if bit[0]==0 then bit[7:1] for address mask
    //
    inline void begin(uint8_t address, uint8_t mask)
    {
        begin_(i2c, address, mask);
    }

    // ------------------------------------------------------------------------------------------------------
    // Write - write data byte to Tx buffer
    // return: 1=success, 0=fail
    // parameters:
    //      data = data byte
    //
    size_t write(uint8_t data);
    inline size_t write(unsigned long n) { return write((uint8_t)n); }
    inline size_t write(long n)          { return write((uint8_t)n); }
    inline size_t write(unsigned int n)  { return write((uint8_t)n); }
    inline size_t write(int n)           { return write((uint8_t)n); }

    // ------------------------------------------------------------------------------------------------------
    // Write Array - write length number of bytes from data array to Tx buffer
    // parameters:
    //      data = pointer to uint8_t (or char) array of data
    //      length = number of bytes to write
    //
    size_t write(const uint8_t* data, size_t quantity);
    inline size_t write(const char* str) { write((const uint8_t*)str, strlen(str)); }

    // ------------------------------------------------------------------------------------------------------
    // Available - returns number of remaining available bytes in Rx buffer
    // return: #bytes available
    //
    inline int available(void) { return i2c->rxBufferLength - i2c->rxBufferIndex; }

    // ------------------------------------------------------------------------------------------------------
    // Read (base routine)
    //
    static int read_(struct i2cStruct* i2c);
    //
    // Read - returns next data byte (signed int) from Rx buffer
    // return: data, -1 if buffer empty
    //
    inline int read(void) { return read_(i2c); }

    // ------------------------------------------------------------------------------------------------------
    // Peek (base routine)
    //
    static int peek_(struct i2cStruct* i2c);
    //
    // Peek - returns next data byte (signed int) from Rx buffer without removing it from Rx buffer
    // return: data, -1 if buffer empty
    //
    inline int peek(void) { return peek_(i2c); }

    // ------------------------------------------------------------------------------------------------------
    // Read Byte (base routine)
    //
    static uint8_t readByte_(struct i2cStruct* i2c);
    //
    // Read Byte - returns next data byte (uint8_t) from Rx buffer
    // return: data, 0 if buffer empty
    //
    inline uint8_t readByte(void) { return readByte_(i2c); }

    // ------------------------------------------------------------------------------------------------------
    // Peek Byte (base routine)
    //
    static uint8_t peekByte_(struct i2cStruct* i2c);
    //
    // Peek Byte - returns next data byte (uint8_t) from Rx buffer without removing it from Rx buffer
    // return: data, 0 if buffer empty
    //
    inline uint8_t peekByte(void) { return peekByte_(i2c); }

    // ------------------------------------------------------------------------------------------------------
    // Flush (not implemented)
    //
    inline void flush(void) {}

    // ------------------------------------------------------------------------------------------------------
    // Get Rx Address - returns target address of incoming I2C command.
    // return: address of last received command
    //
    inline uint16_t getRxAddr(void) { return i2c->Addr; }

    // ------------------------------------------------------------------------------------------------------
    // Get number of sent bytes
    // return: number of bytes actually sent via last transmission.
    //
    inline size_t getTransmitBytes(void) { return i2c->txBufferIndex; }

    // ------------------------------------------------------------------------------------------------------
    // Set callback function for Slave address received
    //
    inline void onAddrReceive(boolean (*function)(uint16_t, uint8_t)) { i2c->user_onAddrReceive = function; }

    // ------------------------------------------------------------------------------------------------------
    // Set callback function for Slave Rx
    //
    inline void onReceive(void (*function)(size_t)) { i2c->user_onReceive = function; }

    // ------------------------------------------------------------------------------------------------------
    // Set callback function for Slave Tx
    //
    inline void onRequest(void (*function)(void)) { i2c->user_onRequest = function; }

    // ------------------------------------------------------------------------------------------------------
    // Set callback function for stop received after Slave Tx
    //
    inline void onStop(void (*function)(void)) { i2c->user_onStop = function; }

};

extern i2c_tinyS Wire;

#endif // I2C_TINY1634_H
