/*
Virtual I2C EEPROM -- WireS library sample

(c) copyright 2015 Hisashi ITO (info at mewpro.cc)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
//
// Microchip 24AA00/24LC00/24C00 128-Bit I2C Bus Serial EEPROM hardware emulator
//
// The following functions are completely emulated.
//   - Device addressing:
//       1010xxxD  --- x = Don't care, D = 1:Read 0:Write
//   - Byte write
//   - Current address read
//   - Random read
//   - Sequential read
//
// The values are stored to / retrieved from in-system programmable EEPROM of micro
// controller.
//
// Note: In the emulator the length of a "Sequential read" must be equal to or 
// less than the size of I2C_BUFFER_LENGTH. Real EEPROM doesn't have the limitation.
//
#include <EEPROM.h>
#include <WireS.h>
#define ROMSIZE 16
#define EEPROMOFFSET 0

volatile unsigned wordAddr;
volatile boolean repeatedStart;

boolean addressHandler(uint16_t slaveAddress, uint8_t startCount)
{
  repeatedStart = (startCount > 0 ? true : false);
  if (repeatedStart && Wire.available()) {
    wordAddr = Wire.read();
  }
  return true;
}

void receiveHandler(size_t numBytes)
{
  if (Wire.available()) {
    byte addr = Wire.read();
    if (Wire.available()) {
      // Byte write
      EEPROM.write(addr % ROMSIZE + EEPROMOFFSET, Wire.read());
      return;
    }
  }
}

void requestHandler()
{
  if (repeatedStart) {
    // Random Read or Sequential Read
    for (int i = 0; i < I2C_BUFFER_LENGTH; i++) {
      Wire.write(EEPROM.read((i + wordAddr) % ROMSIZE + EEPROMOFFSET));
    }
  } else {
    // Current Address Read
    Wire.write((uint8_t)wordAddr);
  }
}

void stopHandler()
{
  if (repeatedStart) {
    wordAddr = (wordAddr + Wire.getTransmitBytes()) % ROMSIZE;
  }
}

void setup() {
  Wire.begin(0x50, B1110); // lower three bits are don't care
  Wire.onAddrReceive(addressHandler);
  Wire.onReceive(receiveHandler);
  Wire.onRequest(requestHandler);
  Wire.onStop(stopHandler);
}

void loop() {
}
