// Two Address Slave Sender -- WireS library sample
//
// This code is derived from
//   http://www.arduino.cc/en/Tutorial/MasterReader
// Modified by Hisashi ITO (info at mewpro.cc) (c) 2015
// to demonstrate how to make slave with two addresses.
//
// Refer to the "Two Address Master Reader" example for use with this

// The original copyright follows:
// ------------------------------------------------------------
// Wire Slave Sender
// by Nicholas Zambetti <http://www.zambetti.com>

// Demonstrates use of the Wire library
// Sends data as an I2C/TWI slave device
// Refer to the "Wire Master Reader" example for use with this

// Created 29 March 2006

// This example code is in the public domain.

#include <WireS.h>

volatile uint8_t slaveAddr;

void setup()
{
  Wire.begin(2, (3 << 1 | 1));      // join i2c bus with addresses #2 and #3
  Wire.onAddrReceive(addressEvent); // register event
  Wire.onRequest(requestEvent);     // register event
}

void loop()
{
  delay(100);
}

// function that executes whenever address is received from master
// this function is registered as an event, see setup()
boolean addressEvent(uint16_t address, uint8_t count)
{
  slaveAddr = (address >> 1);
  return true; // send ACK to master
}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent()
{
  switch (slaveAddr) {
  case 2:
    Wire.write("hello "); // respond with message of 6 bytes
                          // as expected by master
    break;
  case 3:
    Wire.write("aloha "); // respond with another message
    break;
  }
}
