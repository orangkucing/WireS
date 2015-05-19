// Two Address Master Sender -- WireS library sample
//
// This code is derived from
//   http://www.arduino.cc/en/Tutorial/MasterReader
// Modified by Hisashi ITO (info at mewpro.cc) (c) 2015
// to demonstrate how to make slave with two addresses.
//
// Refer to the "Two Address Slave Sender" example for use with this.
// THIS CODE IS FOR ARDUINO UNO or ARDUINO w/ ATmega CORE
// Upload the code to Arduino Uno (or similar) and connect four pins
// SDA, SCL, GND, VCC to ATtiny.

// The original copyright follows:
// ------------------------------------------------------------
// Wire Master Reader
// by Nicholas Zambetti <http://www.zambetti.com>

// Demonstrates use of the Wire library
// Reads data from an I2C/TWI slave device
// Refer to the "Wire Slave Sender" example for use with this

// Created 29 March 2006

// This example code is in the public domain.


#include <Wire.h>

void setup()
{
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output
}

void loop()
{
  Wire.requestFrom(2, 6);    // request 6 bytes from slave device #2

  while(Wire.available())    // slave may send less than requested
  {
    char c = Wire.read(); // receive a byte as character
    Serial.print(c);         // print the character
  }

  delay(500);
  
  Wire.requestFrom(3, 6);    // request 6 bytes from slave device #3

  while(Wire.available())    // slave may send less than requested
  {
    char c = Wire.read(); // receive a byte as character
    Serial.print(c);         // print the character
  }

  delay(500);
}

