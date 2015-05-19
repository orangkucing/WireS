// attiny841_i2c_analog: This example is from attiny85_i2c_analog in TinyWireS library
//
// Modified by Hisashi ITO (info at mewpro.cc) 2015 for WireS

/**
 * Example sketch for writing to and reading from a slave in transactional manner
 *
 * On write the first byte received is considered the register addres to modify/read
 * On each byte sent or read the register address is incremented (and it will loop back to 0)
 *
 * You can try this with the Arduino I2C REPL sketch at https://github.com/rambo/I2C/blob/master/examples/i2crepl/i2crepl.ino 
 * If you have bus-pirate remember that the older revisions do not like the slave streching the clock, this leads to all sorts of weird behaviour
 * Examples use bus-pirate semantics (like the REPL)
 *
 * The basic idea is:
 *  1. Choose your ADC channel (0-X), say A1, then use "byte ch = 1;".
 *  2. Combine the channel and conversion start flag to single calue: byte start_on_ch = (ch | _BV(7)); // This is 0x81
 *  3. Set averaging count to 10 [ 8 1 A ]
 *  4. Write start_on_ch to the first register on the attiny [ 8 0 81 ]
 *  5. Come back later and check the first register [ 8 0 [ 9 r ], if the value is same as ch then the conversion is complete, you can now read the value
 *  6. read the value [ 8 2 [ 9 r r ] (first one is low, second high byte)
 */


/**
 Pinout
 
 ATtinyX41
       arduino pin :     :  SOIC   :     : arduino pin
                         +---\_/---+
                     VCC | 1    14 | GND
          0  (A11) : PB0 | 2    13 | PA0 : (A0)  10  AREF
          1  (A10) : PB1 | 3    12 | PA1 : (A1)  9
   RESET  11 (A9)  : PB3 | 4    11 | PA2 : (A2)  8
          2  (A8)  : PB2 | 5    10 | PA3 : (A3)  7
          3  (A7)  : PA7 | 6     9 | PA4 : (A4)  6   SCL
   SDA    4  (A6)  : PA6 | 7     8 | PA5 : (A5)  5
                         +---------+
                                           (A12)  internal temperature sensor

 ATtiny1634
       arduino pin :     :  SOIC   :     : arduino pin
                         +---\_/---+
          0  (A5)  : PB0 | 1    20 | PB1 : (A6)  16  SDA
          1  (A4)  : PA7 | 2    19 | PB2 : (A7)  15
          2  (A3)  : PA6 | 3    18 | PB3 : (A8)  14
          3  (A2)  : PA5 | 4    17 | PC0 : (A9)  13
          4  (A1)  : PA4 | 5    16 | PC1 : (A10) 12  SCL
          5  (A0)  : PA3 | 6    15 | PC2 : (A11) 11
          6        : PA2 | 7    14 | PC3 :       17  RESET
          7        : PA1 | 8    13 | PC4 :       10
   AREF   8        : PA0 | 9    12 | PC5 :       9
                     GND | 10   11 | VCC
                         +---------+
                                           (A12)  internal temperature sensor
 */
#define I2C_SLAVE_ADDRESS 0x4 // the 7-bit address (remember to change this when adapting this example)
#include <WireS.h>

// For the ADC_xxx helpers
#include <core_adc.h>

// The "registers" we expose to I2C
volatile uint8_t i2c_regs[] =
{
    0x0, // Status register, writing (1<<7 & channel) will start a conversion on that channel, the flag will be set low when conversion is done.
    0x1, // Averaging count, make this many conversions in row and average the result (well, actually it's a rolling average since we do not want to have the possibility of integer overflows)
    0x0, // low byte 
    0x0, // high byte
};
const byte reg_size = sizeof(i2c_regs);
// Tracks the current register pointer position
volatile byte reg_position;
// Tracks wheter to start a conversion cycle
volatile boolean start_conversion;
// Counter to track where we are averaging
byte avg_count;
// Some temp value holders
int avg_temp1;
int avg_temp2;

/**
 * The I2C data requested -handler
 */
void requestEvent()
{  
    for (int i = 0; i < reg_size; i++) {
      Wire.write(i2c_regs[(reg_position + i) % reg_size]); // copy all to I2C transmit buffer
    }
}

/**
 * The I2C data received -handler
 */
void receiveEvent(size_t howMany)
{
    reg_position = Wire.read();
    while(Wire.available())
    {
        i2c_regs[reg_position] = Wire.read();
        if (   reg_position == 0 // If it was the first register
            && bitRead(i2c_regs[0], 7) // And the highest bit is set
            && !ADC_ConversionInProgress() // and we do not actually have a conversion running already
            )
        {
            start_conversion = true;
        }
        reg_position++;
        if (reg_position >= reg_size)
        {
            reg_position = 0;
        }
    }
}

/**
 * The I2C address received -handler
 */
boolean addressEvent(uint16_t addr, uint8_t count)
{
    if (count > 0) reg_position = Wire.read(); // repeated START
    return true;
}

/**
 * The I2C STOP received after transmit received -handler
 */
void stopEvent()
{
    // Increment the reg position on each read, and loop back to zero
    reg_position = (reg_position + Wire.getTransmitBytes()) % reg_size;
}


void setup()
{
    Wire.begin(I2C_SLAVE_ADDRESS);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
    Wire.onAddrReceive(addressEvent);
    Wire.onStop(stopEvent);
}

void loop()
{
    // Thus stuff is basically copied from wiring_analog.c
    if (start_conversion)
    {
        //Avoid doubled starts
        start_conversion = false;
        byte adcpin = (i2c_regs[0] & 0x7f); // Set the channel from the control reg, dropping the highest bit.
#if defined( CORE_ANALOG_FIRST )
        if ( adcpin >= CORE_ANALOG_FIRST ) adcpin -= CORE_ANALOG_FIRST; // allow for channel or pin numbers
#endif
        // NOTE: These handy helpers (ADC_xxx) are only present in the tiny-core, for other cores you need to check their wiring_analog.c source.
        ADC_SetInputChannel( (adc_ic_t)adcpin ); // we need to typecast
        ADC_StartConversion();
        // Reset these variables
        avg_count = 0;
        avg_temp2 = 0;
    }
    
    if (   bitRead(i2c_regs[0], 7) // We have conversion flag up
        && !ADC_ConversionInProgress()) // But the conversion is complete
    {
        // So handle it
        avg_temp1 = ADC_GetDataRegister();
        // Rolling average
        if (avg_count)
        {
            avg_temp2 = (avg_temp2+avg_temp1)/2;
        }
        else
        {
            avg_temp2 = avg_temp1;
        }
        avg_count++;
        if (avg_count >= i2c_regs[1])
        {
            // All done, set the bytes to registers
            noInterrupts();
            i2c_regs[2] = lowByte(avg_temp2);
            i2c_regs[3] = highByte(avg_temp2);
            interrupts();
            // And clear the conversion flag so the master knows we're ready
            bitClear(i2c_regs[0], 7);
        }
        else
        {
            // Re-trigger conversion
            ADC_StartConversion();
        }
    }

}
