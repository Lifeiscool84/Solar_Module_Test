#include "Adafruit_SPIDevice.h"

// Minimal stub implementation for compatibility

Adafruit_SPIDevice::Adafruit_SPIDevice(int8_t cspin, uint32_t freq,
                                       BitOrder dataOrder, uint8_t dataMode,
                                       SPIClass *theSPI) {
  _cs = cspin;
  _spi = theSPI;
  _spiSetting = SPISettings(freq, dataOrder, dataMode);
  _dataMode = dataMode;
}

bool Adafruit_SPIDevice::begin(void) {
  _spi->begin();
  if (_cs >= 0) {
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH);
  }
  return true;
}

bool Adafruit_SPIDevice::read(uint8_t *buffer, size_t len, uint8_t sendvalue) {
  memset(buffer, sendvalue, len); // Pre-fill with sendvalue
  return write_and_read(buffer, len);
}

bool Adafruit_SPIDevice::write(const uint8_t *buffer, size_t len,
                              const uint8_t *prefix_buffer, size_t prefix_len) {
  if (_cs >= 0) {
    digitalWrite(_cs, LOW);
    // Separate transaction for prefix and buffer
    _spi->beginTransaction(_spiSetting);
    
    if (prefix_len > 0) {
      _spi->transfer((uint8_t *)prefix_buffer, prefix_len);
    }
    if (len > 0) {
      _spi->transfer((uint8_t *)buffer, len);
    }
    
    _spi->endTransaction();
    digitalWrite(_cs, HIGH);
  } else {
    _spi->beginTransaction(_spiSetting);
    
    if (prefix_len > 0) {
      _spi->transfer((uint8_t *)prefix_buffer, prefix_len);
    }
    if (len > 0) {
      _spi->transfer((uint8_t *)buffer, len);
    }
    
    _spi->endTransaction();
  }

  return true;
}

bool Adafruit_SPIDevice::write_then_read(const uint8_t *write_buffer,
                                        size_t write_len, uint8_t *read_buffer,
                                        size_t read_len, uint8_t sendvalue) {
  // Write first
  write(write_buffer, write_len);
  // Then read
  return read(read_buffer, read_len, sendvalue);
}

bool Adafruit_SPIDevice::write_and_read(uint8_t *buffer, size_t len) {
  if (_cs >= 0) {
    digitalWrite(_cs, LOW);
    _spi->beginTransaction(_spiSetting);
    _spi->transfer(buffer, len);
    _spi->endTransaction();
    digitalWrite(_cs, HIGH);
  } else {
    _spi->beginTransaction(_spiSetting);
    _spi->transfer(buffer, len);
    _spi->endTransaction();
  }

  return true;
} 