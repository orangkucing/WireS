/* Using Arduino with an I2C EEPROM 24C00

   This code is derived from
     http://playground.arduino.cc/Code/I2CEEPROM
   Modified by Hisashi ITO (info at mewpro.cc) (c) 2015
   to demonstrate how to use slave emulating I2C EEPROM.

   Refer to the "Virtual I2C EEPROM" example for use with this.
   THIS CODE IS FOR ARDUINO UNO or ARDUINO w/ ATmega CORE
   Upload the code to Arduino Uno (or similar) and connect four pins
   SDA, SCL, GND, VCC to ATtiny.
   
   This code works with virtual I2C EEPROM on ATtiny as well as 
   real 24C00.
   
 */
// The original copyright follows:
//---------------------------------------------------------
/* 
  *  Use the I2C bus with EEPROM 24LC64 
  *  Sketch:    eeprom.pde
  *  
  *  Author: hkhijhe
  *  Date: 01/10/2010
  * 
  *   
  */
 
#include <Wire.h> //I2C library

void i2c_eeprom_write_byte( int deviceaddress, byte eeaddress, byte data ) {
  Wire.beginTransmission(deviceaddress);
  Wire.write(eeaddress);
  Wire.write(data);
  Wire.endTransmission(true); // send STOP
  delayMicroseconds(4000); // short delay for real 24C00
}

byte i2c_eeprom_read_byte( int deviceaddress, byte eeaddress ) {
  byte rdata = 0xFF;
  Wire.beginTransmission(deviceaddress);
  Wire.write(eeaddress);
  Wire.endTransmission(false); // send repeated START
  Wire.requestFrom(deviceaddress,1);
  if (Wire.available()) rdata = Wire.read();
  return rdata;
}

void setup() 
{
  char somedata[] = "data from rom"; // data to write
  Wire.begin(); // initialise the connection
  Serial.begin(9600);
  for (int i = 0; i < sizeof(somedata); i++)
    i2c_eeprom_write_byte(0x50, (byte)i, somedata[i]); // write to EEPROM 

  Serial.println("Memory written");
}

void loop() 
{
  int addr=0; //first address
  byte b = i2c_eeprom_read_byte(0x50, 0); // access the first address from the memory

  while (b!=0) 
  {
  Serial.print((char)b); //print content to serial port
    addr++; //increase address
    b = i2c_eeprom_read_byte(0x50, addr); //access an address from the memory
  }
  Serial.println(" ");
  delay(2000);

}
