# INA228 Configuration and Calibration Guide

## Overview

The Texas Instruments INA228 is a high-precision, bidirectional current and power monitor with I2C interface. This guide provides essential configuration instructions and calibration procedures for embedded applications.

## Hardware Specifications

### Key Features
- **Supply Voltage**: 2.7V to 5.5V
- **Current Measurement Range**: ±163.84mV across shunt
- **Resolution**: 20-bit ADC with programmable gain
- **I2C Interface**: Standard (100kHz) and Fast (400kHz) modes

### I2C Address Configuration

| A1 | A0 | Address | Solar Module Usage |
|----|----|---------|--------------------|
| GND | GND | 0x40 | **Solar Panel Sensor** |
| GND | VCC | 0x41 | **Load Monitoring Sensor** |
| VCC | GND | 0x44 | **Battery Monitoring Sensor** |
| VCC | VCC | 0x45 | Reserved |

### Standard Three-Sensor Configuration
- **0x40 Solar**: Monitors solar panel input power
- **0x44 Battery**: Monitors battery charge/discharge
- **0x41 Load**: Monitors system power consumption

## Basic Configuration

### I2C Setup
```cpp
#include <Wire.h>

void setupI2C() {
    Wire.begin();
    Wire.setClock(400000);  // 400kHz fast mode
}
```

### Sensor Initialization
```cpp
bool ina228_init(uint8_t address, const char* name) {
    // Verify device presence
    uint16_t deviceID = ina228_readRegister16(address, INA228_REG_DEVICEID);
    if (deviceID != 0x2280 && deviceID != 0x2281) {
        return false;
    }
    
    // Configure for continuous measurement
    uint16_t config = 0x4233;  // Optimal settings
    ina228_writeRegister16(address, INA228_REG_CONFIG, config);
    
    // Set shunt calibration
    uint16_t shuntCal = calculateShuntCalibration(RSHUNT_OHMS, MAX_CURRENT_A);
    ina228_writeRegister16(address, INA228_REG_SHUNTCAL, shuntCal);
    
    return true;
}
```

## Calibration Procedures

### Shunt Calibration Calculation
```cpp
uint16_t calculateShuntCalibration(float rshunt_ohms, float max_current_A) {
    float current_lsb = max_current_A / 524288.0;  // 2^19 = 524288
    float shunt_cal_f = 13107.2e6 * current_lsb * rshunt_ohms;
    return (uint16_t)(shunt_cal_f + 0.5);  // Round to nearest integer
}
```

### Measurement Functions
```cpp
float ina228_readBusVoltage(uint8_t address) {
    uint32_t raw = ina228_readRegister24(address, INA228_REG_VBUS);
    return (float)((raw >> 4) * 195.3125e-6);  // Convert to volts
}

float ina228_readCurrent(uint8_t address) {
    uint32_t raw = ina228_readRegister24(address, INA228_REG_CURRENT);
    int32_t signed_raw = (int32_t)raw;
    
    // Sign extend if negative
    if (signed_raw & 0x800000) {
        signed_raw |= 0xFF000000;
    }
    
    signed_raw >>= 4;  // Right shift by 4 bits
    return (float)(signed_raw * CURRENT_LSB * 1000.0);  // Convert to mA
}
```

## Troubleshooting

### Common Issues

1. **Zero Readings**: Check shunt calibration register (must be non-zero)
2. **Noisy Measurements**: Enable averaging in configuration register
3. **I2C Communication Errors**: Verify pull-up resistors (4.7kΩ recommended)
4. **Incorrect Current Readings**: Verify shunt resistance value and connections

### Diagnostic Functions
```cpp
void diagnoseINA228(uint8_t address) {
    uint16_t deviceID = ina228_readRegister16(address, INA228_REG_DEVICEID);
    uint16_t config = ina228_readRegister16(address, INA228_REG_CONFIG);
    uint16_t shuntCal = ina228_readRegister16(address, INA228_REG_SHUNTCAL);
    
    Serial.printf("Device ID: 0x%04X\n", deviceID);
    Serial.printf("Configuration: 0x%04X\n", config);
    Serial.printf("Shunt Calibration: %u\n", shuntCal);
}
```

## Configuration Examples

### Low-Power Monitoring
```cpp
// Configuration for battery-powered applications
uint16_t config = 0x4000 | (0x4 << 6) | (0x4 << 3) | 0x03;
// Continuous mode, 1052µs conversion time, no averaging
```

### High-Precision Measurement
```cpp
// Configuration for laboratory applications
uint16_t config = 0x4000 | (0x6 << 6) | (0x6 << 3) | (0x3 << 0) | 0x03;
// Continuous mode, 2074µs conversion, 64-sample averaging
```

For complete implementation details and advanced features, refer to the full INA228 driver documentation and reference implementation.