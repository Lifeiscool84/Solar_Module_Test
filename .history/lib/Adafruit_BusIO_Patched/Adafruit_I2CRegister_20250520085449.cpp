#include "Adafruit_I2CRegister.h"

/*!
 *    @brief  Create a register we access over an I2C Device (which defines the
 * bus and address)
 *    @param  i2cdevice The I2C device to use for underlying I2C access
 *    @param  reg_addr The address pointer value for the I2C/SMBus register, can
 * be 8 or 16 bits
 *    @param  width    The width of the register data itself, defaults to 1 byte
 *    @param  high_addr_toread Whether to add a +0x80 to the register when
 * reading, some devices like this
 *    @param  bitpos   The bit position we are interested in (can be 0 for entire
 * register)
 *    @param  bits     The number of bits we are interested in (can be 8 for
 * entire byte register)
 */
Adafruit_I2CRegister::Adafruit_I2CRegister(
    Adafruit_I2CDevice *device, uint16_t reg_addr,
    adafruit_i2c_registers_addrsize_t addr_size, uint8_t width,
    adafruit_i2c_register_address_increment_t high_addr_toread) {
  _i2cdevice = device;
  _addrsize = addr_size;
  _address = reg_addr;
  _width = width;
  _high_addr_toread = high_addr_toread;
  _cached = false;
  _read_buffer = false;
  _addr_size = addr_size;
}

/*!
 *    @brief  Write a buffer of data to the register location
 *    @param  buffer Data to write
 *    @param  len Number of bytes to write
 *    @return True if the write was successful, false if it failed for some
 * reason
 */
bool Adafruit_I2CRegister::write(uint32_t value, uint8_t numbytes) {
  if (numbytes == 0)
    numbytes = _width;
  if (numbytes > 4)
    return false;

  uint8_t buffer[4];
  for (int i = 0; i < numbytes; i++) {
    buffer[i] = value & 0xFF;
    value >>= 8;
  }
  return write_bytes(buffer, numbytes);
}

/*!
 *    @brief  Write a buffer of data to the register location
 *    @param  buffer Data to write
 *    @param  len Number of bytes to write
 *    @return True if the write was successful, false if it failed for some
 * reason
 */
bool Adafruit_I2CRegister::write_bytes(uint8_t *buffer, uint8_t len) {
  uint8_t addrbuffer[2];
  uint8_t addrlen = 1;

  if (_addr_size == ADDRSIZE_16BIT) {
    addrbuffer[0] = _address >> 8;
    addrbuffer[1] = _address & 0xFF;
    addrlen = 2;
  } else {
    addrbuffer[0] = _address;
  }

  return _i2cdevice->write(buffer, len, true, addrbuffer, addrlen);
}

/*!
 *    @brief  Read data from the register location
 *    @param  buffer Where we want to place read data
 *    @param  len Number of bytes to read
 *    @return True if the read was successful, false if it failed for some
 * reason
 */
bool Adafruit_I2CRegister::read(uint8_t *buffer, uint8_t len) {
  uint8_t addrbuffer[2];
  uint8_t addrlen = 1;

  if (_addr_size == ADDRSIZE_16BIT) {
    addrbuffer[0] = _address >> 8;
    addrbuffer[1] = _address & 0xFF;
    addrlen = 2;
  } else {
    addrbuffer[0] = _address;
  }

  if (_high_addr_toread == ADDRBIT8_HIGH_TOREAD)
    addrbuffer[0] |= 0x80;

  return _i2cdevice->write_then_read(addrbuffer, addrlen, buffer, len);
}

/*!
 *    @brief  Read data from the register location
 *    @param  value Pointer to uint32_t variable to read into
 *    @param  numbytes how many bytes wide we want to read
 *    @return True if the read was successful, false if it failed for some
 * reason
 */
bool Adafruit_I2CRegister::read(uint32_t *value, uint8_t numbytes) {
  if (numbytes == 0)
    numbytes = _width;
  if (numbytes > 4)
    return false;

  if (_read_buffer) {
    if (numbytes != _width)
      return false;
    _read_buffer = false;
    *value = 0;
    for (int i = numbytes - 1; i >= 0; i--) {
      *value <<= 8;
      *value |= _buffer[i];
    }
    return true;
  }

  if (read(_buffer, numbytes)) {
    *value = 0;
    for (int i = numbytes - 1; i >= 0; i--) {
      *value <<= 8;
      *value |= _buffer[i];
    }
    _cached = true;
    _register_value = *value;
    return true;
  }
  return false;
}

/*!
 *    @brief  Read cached data from previous register access
 *    @param  numbytes how many bytes wide we want to read
 *    @return Cached value, which may be invalid if we haven't read before
 */
uint32_t Adafruit_I2CRegister::readCached(uint8_t numbytes) {
  (void)numbytes;
  return _register_value;
}

/*!
 *    @brief  Write a specific bit pattern to a register!
 *    @param  bits The bits to write
 *    @param  bitpos The position of the bits, starting at 0
 *    @param  bitwidth The number of bits wide to write
 *    @return True if the write was successful, false otherwise
 */
bool Adafruit_I2CRegister::write_bits(uint8_t bits, uint8_t bitpos,
                                      uint8_t bitwidth) {
  uint8_t bit_mask = ((1 << bitwidth) - 1) << bitpos;
  uint8_t b;

  if (!read(&b, 1)) {
    return false;
  }
  b &= ~bit_mask;            // remove the current mask bits
  b |= (bits << bitpos);     // add in the new bits
  if (!write(b, 1)) {
    return false;
  }
  return true;
}

/*!
 *    @brief  Create a register object for device access
 *    @param  i2cdevice The I2C device to use for underlying I2C access
 *    @param  reg_addr The address pointer value for the I2C/SMBus register, can
 * be 8 or 16 bits
 *    @param  width    The width of the register data itself, defaults to 1 byte
 *    @param  bitpos   The bit position we are interested in (can be 0 for entire
 * register)
 *    @param  bitwidth The number of bits we are interested in (can be 8 for
 * entire byte register)
 */
Adafruit_BusIO_Register::Adafruit_BusIO_Register(Adafruit_I2CDevice *i2cdevice,
                                                 uint16_t reg_addr,
                                                 uint8_t width, uint8_t bitpos,
                                                 uint8_t bitwidth) {
  _addrReg = new Adafruit_I2CRegister(i2cdevice, reg_addr, ADDRSIZE_8BIT, width);
  _bitwidth = bitwidth;
  _bitpos = bitpos;
}

/*!
 *    @brief  Create a register object for device access
 *    @param  i2cdevice The I2C device to use for underlying I2C access
 *    @param  reg_addr The address pointer value for the I2C/SMBus register, can
 * be 8 or 16 bits
 *    @param  addr_size How many bytes the address pointer value is. For most
 * cases this is ADDRSIZE_8BIT but can be ADDRSIZE_16BIT
 *    @param  width    The width of the register data itself, defaults to 1 byte
 *    @param  bitpos   The bit position we are interested in (can be 0 for entire
 * register)
 *    @param  bitwidth The number of bits we are interested in (can be 8 for
 * entire byte register)
 */
Adafruit_BusIO_Register::Adafruit_BusIO_Register(
    Adafruit_I2CDevice *i2cdevice, uint16_t reg_addr,
    adafruit_i2c_registers_addrsize_t addr_size, uint8_t width, uint8_t bitpos,
    uint8_t bitwidth) {
  _addrReg =
      new Adafruit_I2CRegister(i2cdevice, reg_addr, addr_size, width);
  _bitwidth = bitwidth;
  _bitpos = bitpos;
}

/*!
 *    @brief  Write a buffer of data to the register
 *    @param  buffer Data to write
 *    @param  len Number of bytes to write
 *    @return True if the write was successful, false if it failed for some
 * reason
 */
bool Adafruit_BusIO_Register::write(uint8_t *buffer, uint8_t len) {
  return _addrReg->write_bytes(buffer, len);
}

/*!
 *    @brief  Write a value to the register
 *    @param  value Data to write
 *    @return True if the write was successful, false if it failed for some
 * reason
 */
bool Adafruit_BusIO_Register::write(uint32_t value) {
  if (_bitwidth == 1) {
    return _addrReg->write_bits(value & 1, _bitpos, _bitwidth);
  }

  if ((_bitwidth + _bitpos) <= 8) {
    uint8_t register_value;

    if (!_addrReg->read(&register_value, 1)) {
      return false;
    }

    uint8_t mask = ((1 << (_bitwidth)) - 1) << _bitpos;
    register_value &= ~mask;
    register_value |= (value & ((1 << _bitwidth) - 1)) << _bitpos;

    if (!_addrReg->write(register_value, 1)) {
      return false;
    }
  } else if ((_bitwidth + _bitpos) <= 16) {
    uint16_t register_value;

    if (!_addrReg->read((uint32_t *)&register_value, 2)) {
      return false;
    }

    uint16_t mask = ((1 << (_bitwidth)) - 1) << _bitpos;
    register_value &= ~mask;
    register_value |= (value & ((1 << _bitwidth) - 1)) << _bitpos;

    if (!_addrReg->write(register_value, 2)) {
      return false;
    }
  } else if ((_bitwidth + _bitpos) <= 32) {
    uint32_t register_value;

    if (!_addrReg->read(&register_value, 4)) {
      return false;
    }

    uint32_t mask = (((uint32_t)1 << (_bitwidth)) - 1) << _bitpos;
    register_value &= ~mask;
    register_value |= (value & (((uint32_t)1 << _bitwidth) - 1)) << _bitpos;

    if (!_addrReg->write(register_value, 4)) {
      return false;
    }
  } else {
    return false;
  }

  return true;
}

/*!
 *    @brief  Read data from the register
 *    @param  buffer Where we want to place read data
 *    @param  len Number of bytes to read
 *    @return True if the read was successful, false if it failed for some
 * reason
 */
bool Adafruit_BusIO_Register::read(uint8_t *buffer, uint8_t len) {
  return _addrReg->read(buffer, len);
}

/*!
 *    @brief  Read a value from the register
 *    @param  value Pointer to 32-bit value to read into
 *    @return True if the read was successful, false if it failed for some
 * reason
 */
bool Adafruit_BusIO_Register::read(uint32_t *value) {
  if (_bitwidth == 1) {
    uint8_t register_value;
    if (!_addrReg->read(&register_value, 1)) {
      return false;
    }
    *value = (register_value >> _bitpos) & 1;
    return true;
  }

  if ((_bitwidth + _bitpos) <= 8) {
    uint8_t register_value;
    if (!_addrReg->read(&register_value, 1)) {
      return false;
    }
    *value = (register_value >> _bitpos) & ((1 << _bitwidth) - 1);
  } else if ((_bitwidth + _bitpos) <= 16) {
    uint16_t register_value;
    if (!_addrReg->read((uint32_t *)&register_value, 2)) {
      return false;
    }
    *value = (register_value >> _bitpos) & ((1 << _bitwidth) - 1);
  } else if ((_bitwidth + _bitpos) <= 32) {
    uint32_t register_value;
    if (!_addrReg->read(&register_value, 4)) {
      return false;
    }
    *value = (register_value >> _bitpos) & (((uint32_t)1 << _bitwidth) - 1);
  } else {
    return false;
  }

  return true;
}

/*!
 *    @brief  Pretty print the last read register
 */
uint32_t Adafruit_BusIO_Register::read(void) {
  uint32_t val;

  if (!read(&val)) {
    return ~0;
  }
  return val;
} 