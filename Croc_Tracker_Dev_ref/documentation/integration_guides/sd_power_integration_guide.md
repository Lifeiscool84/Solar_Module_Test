# SD Card Power Monitoring Integration Guide

## Overview

This guide provides comprehensive instructions for integrating SD card power management and INA228 current/voltage monitoring into embedded systems. The reference implementation demonstrates best practices for power-efficient data logging in battery-operated devices.

## Quick Start

### Hardware Requirements

- **Microcontroller**: SparkFun RedBoard Artemis Nano (or compatible)
- **Current Sensors**: 2x Texas Instruments INA228 precision monitors
- **Storage**: MicroSD card (Class 10 recommended)
- **Power Supply**: Battery pack (3.7V LiPo recommended)

### Basic Integration Steps

1. **Initialize I2C and Sensors**
```cpp
#include "ina228_driver.h"
Wire.begin();
ina228_init(0x44, "Battery");
ina228_init(0x41, "Load");
```

2. **Configure SD Power Management**
```cpp
#include "sd_power_manager.h"
SD_Config_t config = SD_POWER_ULTRA_LOW_CONFIG();
sd_power_init(&config);
```

3. **Set Up CSV Logging**
```cpp
#include "csv_logger.h"
CSV_Config_t csv_config = CSV_LOGGER_LOW_POWER_CONFIG(
    "power_log.csv", power_fields, 7
);
csv_logger_init(&csv_config);
```

4. **Main Measurement Loop**
```cpp
void takePowerMeasurement() {
    INA228_Measurement_t battery, load;
    ina228_takeMeasurement(0x44, &battery);
    ina228_takeMeasurement(0x41, &load);
    
    csv_logger_writeValues(millis(), 
        battery.bus_voltage_V, battery.current_mA,
        load.bus_voltage_V, load.current_mA);
}
```

## Power Optimization Strategies

### SD Card Power States

1. **De-initialized**: Ultra-low power, 500ms wake time
2. **Idle Standby**: Low power, 50ms wake time  
3. **Active Write**: Medium power, 1ms wake time
4. **Batch Mode**: Optimized power, variable wake time

### Configuration Examples

**Ultra-Low Power**:
```cpp
SD_Config_t ultra_low = {
    .power_level = SD_POWER_ULTRA_LOW,
    .write_strategy = SD_WRITE_BATCH,
    .batch_interval_ms = 600000  // 10-minute batches
};
```

**Real-Time Monitoring**:
```cpp
SD_Config_t realtime = {
    .power_level = SD_POWER_MAX_PERFORMANCE,
    .write_strategy = SD_WRITE_IMMEDIATE,
    .batch_interval_ms = 1000
};
```

## Best Practices

1. **Memory Management**: Monitor RAM usage and size buffers appropriately
2. **Power Efficiency**: Use batch writing for maximum battery life
3. **Data Integrity**: Enable validation and checksums for critical data
4. **Error Handling**: Implement retry mechanisms and graceful degradation

## Troubleshooting

- **INA228 Not Detected**: Check I2C connections and pull-up resistors
- **SD Card Fails**: Try different SPI speeds, verify FAT32 formatting
- **High Power Usage**: Monitor SD active time, enable auto-deinitialization
- **Data Loss**: Increase sync frequency, implement backup strategies

For complete implementation details, see the reference files and full integration documentation.