#ifndef Adafruit_I2CRegister_h
#define Adafruit_I2CRegister_h

#include <Arduino.h>
#include "Adafruit_I2CDevice.h"

typedef enum _adafruit_i2c_register_address_increment {
  ADDRBIT8_HIGH_TOREAD = 0x80,
  AD16_bit_HIGH_TOREAD = 0x80,
  ADDRBIT8_HIGH_TOWRITE = 0x00
} adafruit_i2c_register_address_increment_t;

typedef enum _addrsize_t {
  ADDRSIZE_8BIT, ///< register address sits in a single 8 bit
  ADDRSIZE_16BIT ///< register address sits in 16 bits (big-endian)
} adafruit_i2c_registers_addrsize_t;

class Adafruit_I2CRegister {
public:
  Adafruit_I2CRegister(Adafruit_I2CDevice *device, uint16_t reg_addr,
                       adafruit_i2c_registers_addrsize_t addr_size =
                           ADDRSIZE_8BIT,
                       uint8_t width = 1,
                       adafruit_i2c_register_address_increment_t
                           high_addr_toread = ADDRBIT8_HIGH_TOREAD);

  bool write(uint32_t value, uint8_t numbytes = 0);
  bool read(uint8_t *buffer, uint8_t len);
  bool read(uint32_t *value, uint8_t numbytes = 0);
  uint32_t readCached(uint8_t numbytes = 0);
  bool write_bits(uint8_t bits, uint8_t bitpos, uint8_t bitwidth);

private:
  Adafruit_I2CDevice *_i2cdevice;
  uint16_t _addrsize;
  bool _read_buffer;
  adafruit_i2c_registers_addrsize_t _addr_size;
  adafruit_i2c_register_address_increment_t _high_addr_toread;

  uint16_t _address;
  uint8_t _width, _cached;
  uint8_t _buffer[4]; // we wont support anything larger than uint32 for
                      // non-buffered read
  uint32_t _register_value;
};

class Adafruit_BusIO_Register {
public:
  Adafruit_BusIO_Register(Adafruit_I2CDevice *i2cdevice, uint16_t reg_addr,
                          uint8_t width = 1, uint8_t bitpos = 0,
                          uint8_t bitwidth = 1);
  Adafruit_BusIO_Register(Adafruit_I2CDevice *i2cdevice, uint16_t reg_addr,
                          adafruit_i2c_registers_addrsize_t addr_size,
                          uint8_t width = 1, uint8_t bitpos = 0,
                          uint8_t bitwidth = 1);
  bool write(uint32_t value);
  bool read(uint32_t *value);
  uint32_t read(void);
  bool write(uint8_t *buffer, uint8_t len);
  bool read(uint8_t *buffer, uint8_t len);

private:
  Adafruit_I2CRegister *_addrReg;
  uint8_t _bitwidth, _bitpos;
};

#endif // Adafruit_I2CRegister_h 