/*
 * sensor_driver_template.h
 * 
 * Generic sensor driver template for the Croc Tracker project
 * 
 * Usage:
 * 1. Copy this template to hardware/sensors/[sensor_name]/
 * 2. Replace all [SENSOR_NAME] placeholders with your sensor name
 * 3. Update I2C address, registers, and configuration values
 * 4. Implement sensor-specific calculations and conversions
 * 5. Add sensor-specific validation and error handling
 * 
 * Author: [Your Name]
 * Date: [Date]
 * Version: 1.0
 */

#ifndef [SENSOR_NAME]_DRIVER_H
#define [SENSOR_NAME]_DRIVER_H

#include <Arduino.h>
#include <Wire.h>

// ==================== CONFIGURATION SECTION ====================

// I2C Configuration
#define [SENSOR_NAME]_I2C_ADDRESS_DEFAULT    0x[XX]  // Replace with actual I2C address
#define [SENSOR_NAME]_I2C_ADDRESS_ALT        0x[XX]  // Alternative address if applicable

// Register Addresses (8-bit addresses)
#define [SENSOR_NAME]_REG_CONFIG             0x00    // Configuration register
#define [SENSOR_NAME]_REG_STATUS             0x01    // Status register  
#define [SENSOR_NAME]_REG_DATA_MSB           0x02    // Data register MSB
#define [SENSOR_NAME]_REG_DATA_LSB           0x03    // Data register LSB
#define [SENSOR_NAME]_REG_DEVICE_ID          0x[XX]  // Device ID register

// Expected Device ID
#define [SENSOR_NAME]_DEVICE_ID              0x[XXXX] // Expected device ID value

// Configuration Values
#define [SENSOR_NAME]_CONFIG_DEFAULT         0x[XXXX] // Default configuration
#define [SENSOR_NAME]_CONFIG_CONTINUOUS      0x[XXXX] // Continuous measurement mode
#define [SENSOR_NAME]_CONFIG_SINGLE_SHOT     0x[XXXX] // Single-shot measurement mode

// Conversion Constants
#define [SENSOR_NAME]_LSB_[MEASUREMENT]      [X.XXXX] // LSB value for measurement conversion
#define [SENSOR_NAME]_MAX_[MEASUREMENT]      [XXX.X]  // Maximum measurement value
#define [SENSOR_NAME]_MIN_[MEASUREMENT]      [XXX.X]  // Minimum measurement value

// ==================== DATA STRUCTURES ====================

typedef enum {
    [SENSOR_NAME]_MODE_SHUTDOWN = 0,
    [SENSOR_NAME]_MODE_SINGLE_SHOT,
    [SENSOR_NAME]_MODE_CONTINUOUS
} [sensor_name]_mode_t;

typedef enum {
    [SENSOR_NAME]_RANGE_[RANGE1] = 0,
    [SENSOR_NAME]_RANGE_[RANGE2],
    [SENSOR_NAME]_RANGE_[RANGE3]
} [sensor_name]_range_t;

typedef struct {
    uint8_t i2c_address;
    [sensor_name]_mode_t mode;
    [sensor_name]_range_t range;
    bool device_present;
    uint16_t device_id;
} [sensor_name]_config_t;

typedef struct {
    float [measurement1];       // Primary measurement (e.g., temperature, voltage)
    float [measurement2];       // Secondary measurement if applicable
    uint32_t timestamp_ms;      // Measurement timestamp
    bool valid;                 // Data validity flag
} [sensor_name]_data_t;

// ==================== FUNCTION DECLARATIONS ====================

// Core Functions
bool [sensor_name]_init(uint8_t i2c_address, const char* sensor_name);
bool [sensor_name]_begin([sensor_name]_config_t* config);
bool [sensor_name]_reset(uint8_t i2c_address);
bool [sensor_name]_setMode(uint8_t i2c_address, [sensor_name]_mode_t mode);
bool [sensor_name]_setRange(uint8_t i2c_address, [sensor_name]_range_t range);

// Data Reading Functions
[sensor_name]_data_t [sensor_name]_readData(uint8_t i2c_address);
float [sensor_name]_read[Measurement1](uint8_t i2c_address);
float [sensor_name]_read[Measurement2](uint8_t i2c_address);
uint16_t [sensor_name]_readRaw(uint8_t i2c_address, uint8_t reg);

// Status and Validation Functions
bool [sensor_name]_isDataReady(uint8_t i2c_address);
bool [sensor_name]_validateDevice(uint8_t i2c_address);
uint16_t [sensor_name]_getDeviceID(uint8_t i2c_address);
bool [sensor_name]_selfTest(uint8_t i2c_address);

// Low-level I2C Communication Functions
bool [sensor_name]_writeRegister8(uint8_t address, uint8_t reg, uint8_t value);
bool [sensor_name]_writeRegister16(uint8_t address, uint8_t reg, uint16_t value);
uint8_t [sensor_name]_readRegister8(uint8_t address, uint8_t reg);
uint16_t [sensor_name]_readRegister16(uint8_t address, uint8_t reg);
uint32_t [sensor_name]_readRegister24(uint8_t address, uint8_t reg);

// Utility Functions
const char* [sensor_name]_getModeString([sensor_name]_mode_t mode);
const char* [sensor_name]_getRangeString([sensor_name]_range_t range);
void [sensor_name]_printConfig([sensor_name]_config_t* config);
void [sensor_name]_printData([sensor_name]_data_t* data);

// ==================== CONFIGURATION MACROS ====================

// Quick configuration for common use cases
#define [SENSOR_NAME]_CONFIG_BASIC(addr) { \
    .i2c_address = addr, \
    .mode = [SENSOR_NAME]_MODE_CONTINUOUS, \
    .range = [SENSOR_NAME]_RANGE_[DEFAULT], \
    .device_present = false, \
    .device_id = 0 \
}

#define [SENSOR_NAME]_CONFIG_LOW_POWER(addr) { \
    .i2c_address = addr, \
    .mode = [SENSOR_NAME]_MODE_SINGLE_SHOT, \
    .range = [SENSOR_NAME]_RANGE_[LOW_POWER], \
    .device_present = false, \
    .device_id = 0 \
}

#define [SENSOR_NAME]_CONFIG_HIGH_PRECISION(addr) { \
    .i2c_address = addr, \
    .mode = [SENSOR_NAME]_MODE_CONTINUOUS, \
    .range = [SENSOR_NAME]_RANGE_[HIGH_PRECISION], \
    .device_present = false, \
    .device_id = 0 \
}

// ==================== INTEGRATION EXAMPLES ====================

/*
 * Basic Usage Example:
 * 
 * #include "[sensor_name]_driver.h"
 * 
 * void setup() {
 *     Wire.begin();
 *     
 *     if ([sensor_name]_init([SENSOR_NAME]_I2C_ADDRESS_DEFAULT, "Sensor1")) {
 *         Serial.println("Sensor initialized successfully");
 *     } else {
 *         Serial.println("Sensor initialization failed");
 *     }
 * }
 * 
 * void loop() {
 *     [sensor_name]_data_t data = [sensor_name]_readData([SENSOR_NAME]_I2C_ADDRESS_DEFAULT);
 *     
 *     if (data.valid) {
 *         Serial.print("Measurement 1: ");
 *         Serial.println(data.[measurement1]);
 *         Serial.print("Measurement 2: ");
 *         Serial.println(data.[measurement2]);
 *     }
 *     
 *     delay(1000);
 * }
 */

/*
 * Multi-sensor Configuration Example:
 * 
 * [sensor_name]_config_t sensor1 = [SENSOR_NAME]_CONFIG_BASIC(0x[XX]);
 * [sensor_name]_config_t sensor2 = [SENSOR_NAME]_CONFIG_LOW_POWER(0x[XX]);
 * [sensor_name]_config_t sensor3 = [SENSOR_NAME]_CONFIG_HIGH_PRECISION(0x[XX]);
 * 
 * void setup() {
 *     Wire.begin();
 *     
 *     [sensor_name]_begin(&sensor1);
 *     [sensor_name]_begin(&sensor2);
 *     [sensor_name]_begin(&sensor3);
 * }
 */

// ==================== TROUBLESHOOTING NOTES ====================

/*
 * Common Issues and Solutions:
 * 
 * 1. Device not detected:
 *    - Check I2C address (use I2C scanner)
 *    - Verify wiring (SDA, SCL, VCC, GND)
 *    - Check pull-up resistors (4.7kΩ typical)
 *    - Verify power supply voltage
 * 
 * 2. Incorrect readings:
 *    - Verify LSB conversion constants
 *    - Check register addresses in datasheet
 *    - Ensure proper measurement range
 *    - Allow settling time after configuration
 * 
 * 3. Communication failures:
 *    - Check I2C bus speed (100kHz/400kHz)
 *    - Verify endianness for multi-byte reads
 *    - Add delays between operations if needed
 *    - Check for bus conflicts with other devices
 * 
 * 4. Device ID validation fails:
 *    - Some sensors have multiple valid device IDs
 *    - Check datasheet for all possible ID values
 *    - Consider making device ID check optional
 */

#endif // [SENSOR_NAME]_DRIVER_H