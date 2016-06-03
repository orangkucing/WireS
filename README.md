# WireS
Slave only hardware I2C library for ATtiny1634, ATtiny441/841, and ATtiny828.

### Introduction

Some Atmel microcontrollers have a built-in slave only I2C interface, namely, 
ATtiny1634, ATtiny441/841, ATtiny20/40, and ATtiny828. 
The hardware is completely different from the one in popular Arduinos as ATmega328 etc. 
so another version of Wire Library was needed.

The new Wire library, WireS is intended to become a drop-in replacement of existing code that is developed by using Arduino IDE.
For the purpose WireS contains the following compatible slave functions to the Wire Library:

* Wire.begin(_address_)
* Wire.write(_ARG_) (where (_ARG_) can be (_value_), (_string_), or (_data_, _length_))
* Wire.available()
* Wire.read()
* Wire.onReceive(_handler_)
* Wire.onRequest(_handler_)

Furthermore the library implements new functionalities 
such as switching multiple slave addresses, proper handling of repeated starts and so on.

The repo also includes an interesting example of emulating I2C EEPROM (Microchip 24AA00) 
that makes ATtiny microcontroller's internal EEPROM accessible as an external I2C EEPROM.

### Usage

Install Arduino IDE and Arduino core for ATtiny1634, ATtiny441/841, and ATtiny828 to your PC.

* [Arduino IDE download](http://www.arduino.cc/en/Main/Software)
* [Arduino core for ATtiny](https://github.com/SpenceKonde/ATTinyCore) 1634, x313, x4, x41, x5, x61, x7, x8 and 828 for Arduino 1.6.x 

(Currently there's no Arduino core for ATtiny20/40.)

Then download the zip file of WireS, unpack it to your library directory,
and you are ready to use WireS by including the header file as
```
#include <WireS.h>
```

### Reference

- - - 
##### Wire.begin(_address_)
##### Wire.begin(_address_, _mask_)
Initiate the WireS library and join the I2C bus as a slave. 

The 7-bit slave address is specified as _address_.
If LSB of _mask_ is set then (_mask_ >> 1) is the second 7-bit slave address,
otherwise the bits specified in (_mask_ >> 1) are masked (ignored) in _address_. 

For example
```
Wire.begin(0x50, (0x60 << 1 | 1));
```
listens two slave addresses 0x50 and 0x60, and
```
Wire.begin(0x50, B1110);
```
accepts transactions targeted to slave addresses from 0x50 to 0x57.

Note: 10-bit slave address is supported but the lower 7-bit part of address needs to be handled (ACKed/NACKed) by software in the onAddrReceive handler.

- - -
##### Wire.write(_value_)
##### Wire.write(_string_)
##### Wire.write(_data_, _length_)
Writes data in response to a request from a master.

A value to send as a single byte is _value_,
a string to send as a series of bytes is _string_, and
an array of data to send as bytes is _data_ whose size is in _length_.

Wire.write() returns a value that is equal to the number of bytes written to the internal buffer
and the value is not equal to the number of bytes actually transmitted through the I2C bus.
In order to know the actual numbers of sent bytes use Wire.getTransmitBytes() in the handler of Wire.onStop() (or in the handler of Wire.onAddrReceive()) instead.

- - -
##### Wire.available()
Returns the number of bytes available for retrieval with Wire.read().

This should be called on inside the onReceive() handler or under repeated start condition onAddrReceive() handler.

- - -
##### Wire.read()
Reads a byte that was transmitted from a master.

This should be called on inside the onReceive() handler or under repeated start condition onAddrReceive() handler.

- - -
##### Wire.getTransmitBytes()
Returns the number of bytes actually sent with Wire.write().

This should be called on inside the onStop() handler or under repeated start condition onAddrReceive() handler.

- - -
##### Wire.onAddrReceive(_handler_)
Registers a function to be called when the device receives a sequence of start / 7-bit or 10-bit address / direction bit from a master.

_handler_: the function to be called when the slave receives its address; this should take two integer parameters _address_ and _startCount_
(_address_[7:1] or _address_[10:1] by which the device is called, _address_[0] direction, and _startCount_ the number of start so far in the current transmission), and 
returns `true`: the device send ACK to the master for going on;
`false`: the device send NACK to the master and stop the current session.

e.g.: ```boolean myHandler(uint16_t address, uint8_t startCount)```

- - -
##### Wire.onReceive(_handler_)
Registers a function to be called when the device receives a sequence of data from a master.

_handler_: the function to be called when the slave receives data; this should take an integer parameter _numBytes_
(_numBytes_ the number of bytes read from the master) and return nothing,

e.g.: ```void myHandler(int numBytes)```

- - -
##### Wire.onRequest(_handler_)
Registers a function to be called when a master requests data from the device. 

_handler_: the function to be called, no parameters and returns nothing.

e.g.: ```void myHandler()```

- - -
##### Wire.onStop(_handler_)
Registers a function to be called when stop condition is detected after the transmission. 

_handler_:  the function to be called, no parameters and returns nothing.

e.g.: ```void myHandler()```

### Handler Invocation

The user defined handlers are called from the hardware interrupt routine.
The following diagram, originally from Atmel's datasheet, shows the exact timing when these handlers will be called.

![Handler Invocation Points](http://mewpro.cc/wp-content/uploads/I2C-slave.jpg)
