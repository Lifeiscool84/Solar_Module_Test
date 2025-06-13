/**
 * INA228 Driver Module Header
 * ==========================
 * 
 * Complete driver interface for Texas Instruments INA228 current/voltage sensor
 * This header provides a clean interface for integrating INA228 sensors into
 * embedded systems requiring precise power monitoring.
 * 
 * Features:
 * - I2C communication functions
 * - Sensor initialization and calibration
 * - Voltage, current, and power measurement
 * - Multi-sensor support
 * - Error handling and validation
 * 
 * Usage:
 * 1. Include this header in your main application
 * 2. Call Wire.begin() before using any functions
 * 3. Initialize each sensor with initializeINA228()
 * 4. Read measurements using the appropriate functions
 * 
 * Hardware Requirements:
 * - I2C bus connection
 * - Proper shunt resistor for current measurement
 * - Pull-up resistors on SDA/SCL lines (typically 4.7kΩ)
 * 
 * @author Reference Implementation Team
 * @date 2025
 */

#ifndef INA228_DRIVER_H
#define INA228_DRIVER_H

#include <Arduino.h>
#include <Wire.h>

// ==============================================================================
// INA228 REGISTER DEFINITIONS
// ==============================================================================

// Primary register addresses (8-bit addressing)
#define INA228_REG_CONFIG    0x00  // Configuration register
#define INA228_REG_SHUNTCAL  0x02  // Shunt calibration register
#define INA228_REG_VBUS      0x05  // Bus voltage register (24-bit)
#define INA228_REG_CURRENT   0x07  // Current register (24-bit)
#define INA228_REG_POWER     0x08  // Power register (24-bit)
#define INA228_REG_ENERGY    0x09  // Energy accumulation register
#define INA228_REG_CHARGE    0x0A  // Charge accumulation register
#define INA228_REG_DIAGALRT  0x0B  // Diagnostic and alert register
#define INA228_REG_DEVICEID  0x3F  // Device ID register

// Expected device ID values
#define INA228_DEVICE_ID_1   0x2280
#define INA228_DEVICE_ID_2   0x2281

// Configuration register bit definitions
#define INA228_CONFIG_RESET     (1 << 15)  // Reset bit
#define INA228_CONFIG_MODE_MASK 0x0003     // Operating mode mask
#define INA228_CONFIG_MODE_CONTINUOUS 0x03 // Continuous shunt and bus measurement

// ADC configuration values
#define INA228_ADC_RANGE_163MV  0          // ±163.84 mV range
#define INA228_BUSCT_1052US     4          // 1052 µs conversion time
#define INA228_SHUNTCT_1052US   4          // 1052 µs conversion time

// ==============================================================================
// SENSOR CONFIGURATION CONSTANTS
// ==============================================================================

// I2C addresses for three-sensor configuration (based on user's main.txt)
#define INA228_SOLAR_I2C_ADDRESS       0x40  // Solar panel monitoring sensor
#define INA228_BATTERY_I2C_ADDRESS     0x44  // Battery monitoring sensor  
#define INA228_LOAD_I2C_ADDRESS        0x41  // Load monitoring sensor

// Backward compatibility
#define INA228_DEFAULT_I2C_ADDRESS     INA228_SOLAR_I2C_ADDRESS

// Calibration constants
#define INA228_CALIBRATION_FACTOR      13107.2e6  // From datasheet
#define INA228_VOLTAGE_LSB             195.3125e-6  // Bus voltage LSB in volts
#define INA228_POWER_LSB_MULTIPLIER    3.2         // Power LSB = 3.2 × Current LSB

// Current sensing parameters (adjust for your application)
extern const float INA228_RSHUNT_OHMS;     // Shunt resistance value
extern const float INA228_MAX_CURRENT_A;   // Maximum expected current
extern const float INA228_CURRENT_LSB;     // Current measurement resolution

// ==============================================================================
// DATA STRUCTURES
// ==============================================================================

/**
 * INA228 sensor configuration structure
 * Use this to customize sensor behavior for your application
 */
typedef struct {
    uint8_t i2c_address;        // I2C address of the sensor
    float shunt_resistance;     // Shunt resistor value in ohms
    float max_current;          // Maximum expected current in amperes
    uint16_t config_register;   // Configuration register value
    bool enable_alerts;         // Enable alert functionality
    const char* sensor_name;    // Human-readable sensor identifier
} INA228_Config_t;

/**
 * INA228 measurement data structure
 * Contains all measurements from a single reading cycle
 */
typedef struct {
    uint32_t timestamp_ms;      // Measurement timestamp
    float bus_voltage_V;        // Bus voltage in volts
    float current_mA;           // Current in milliamperes
    float power_hw_mW;          // Hardware calculated power in milliwatts
    float power_calc_mW;        // Software calculated power (V×I) in milliwatts
    bool valid;                 // Data validity flag
    uint8_t sensor_address;     // Source sensor I2C address
} INA228_Measurement_t;

// ==============================================================================
// FUNCTION DECLARATIONS
// ==============================================================================

/**
 * Low-level I2C communication functions
 */
bool ina228_writeRegister16(uint8_t address, uint8_t reg, uint16_t value);
uint16_t ina228_readRegister16(uint8_t address, uint8_t reg);
uint32_t ina228_readRegister24(uint8_t address, uint8_t reg);

/**
 * Sensor initialization and configuration
 */
bool ina228_init(uint8_t address, const char* sensorName);
bool ina228_initWithConfig(const INA228_Config_t* config);
bool ina228_reset(uint8_t address);
bool ina228_setConfiguration(uint8_t address, uint16_t config);
bool ina228_setShuntCalibration(uint8_t address, float shunt_ohms, float max_current);

/**
 * Device identification and status
 */
uint16_t ina228_getDeviceID(uint8_t address);
bool ina228_isDevicePresent(uint8_t address);
uint16_t ina228_getDiagnosticFlags(uint8_t address);

/**
 * Measurement functions
 */
float ina228_readBusVoltage(uint8_t address);
float ina228_readCurrent(uint8_t address);
float ina228_readPower(uint8_t address);
uint32_t ina228_readEnergy(uint8_t address);
uint32_t ina228_readCharge(uint8_t address);

/**
 * Advanced measurement functions
 */
bool ina228_takeMeasurement(uint8_t address, INA228_Measurement_t* measurement);
bool ina228_takeMultipleMeasurements(const uint8_t* addresses, uint8_t count, 
                                   INA228_Measurement_t* measurements);

/**
 * Utility functions
 */
float ina228_calculateCurrentLSB(float max_current);
uint16_t ina228_calculateShuntCal(float shunt_ohms, float current_lsb);
bool ina228_validateMeasurement(const INA228_Measurement_t* measurement);
void ina228_printMeasurement(const INA228_Measurement_t* measurement);

/**
 * Power management functions
 */
bool ina228_enterPowerDown(uint8_t address);
bool ina228_exitPowerDown(uint8_t address);
bool ina228_setOperatingMode(uint8_t address, uint8_t mode);

/**
 * Error handling
 */
typedef enum {
    INA228_ERROR_NONE = 0,
    INA228_ERROR_I2C_COMM,
    INA228_ERROR_INVALID_DEVICE_ID,
    INA228_ERROR_CONFIG_FAILED,
    INA228_ERROR_CALIBRATION_FAILED,
    INA228_ERROR_INVALID_PARAMETER,
    INA228_ERROR_MEASUREMENT_TIMEOUT
} INA228_Error_t;

INA228_Error_t ina228_getLastError(void);
const char* ina228_getErrorString(INA228_Error_t error);

// ==============================================================================
// CONFIGURATION MACROS
// ==============================================================================

/**
 * Default configuration for typical applications
 * Modify these values for your specific use case
 */
#define INA228_DEFAULT_CONFIG() { \
    .i2c_address = INA228_DEFAULT_I2C_ADDRESS, \
    .shunt_resistance = 0.001, \
    .max_current = 5.0, \
    .config_register = 0x4233, \
    .enable_alerts = false, \
    .sensor_name = "INA228" \
}

/**
 * High-precision configuration for low-current applications
 */
#define INA228_PRECISION_CONFIG() { \
    .i2c_address = INA228_DEFAULT_I2C_ADDRESS, \
    .shunt_resistance = 0.1, \
    .max_current = 1.0, \
    .config_register = 0x4233, \
    .enable_alerts = true, \
    .sensor_name = "INA228_Precision" \
}

/**
 * High-current configuration for power monitoring
 */
#define INA228_POWER_CONFIG() { \
    .i2c_address = INA228_DEFAULT_I2C_ADDRESS, \
    .shunt_resistance = 0.0001, \
    .max_current = 20.0, \
    .config_register = 0x4233, \
    .enable_alerts = false, \
    .sensor_name = "INA228_Power" \
}

/**
 * Three-sensor system configuration for solar monitoring
 * Based on user's main.txt configuration with three INA228 devices
 */
#define INA228_SOLAR_CONFIG() { \
    .i2c_address = INA228_SOLAR_I2C_ADDRESS, \
    .shunt_resistance = 0.015, \
    .max_current = 5.0, \
    .config_register = 0x4233, \
    .enable_alerts = false, \
    .sensor_name = "Solar" \
}

#define INA228_BATTERY_CONFIG() { \
    .i2c_address = INA228_BATTERY_I2C_ADDRESS, \
    .shunt_resistance = 0.0177186, \
    .max_current = 5.0, \
    .config_register = 0x4233, \
    .enable_alerts = false, \
    .sensor_name = "Battery" \
}

#define INA228_LOAD_CONFIG() { \
    .i2c_address = INA228_LOAD_I2C_ADDRESS, \
    .shunt_resistance = 0.015, \
    .max_current = 5.0, \
    .config_register = 0x4233, \
    .enable_alerts = false, \
    .sensor_name = "Load" \
}

// ==============================================================================
// DEVICE ID VALIDATION (OPTIONAL)
// ==============================================================================

/**
 * Device ID validation functions
 * NOTE: Device ID checking is OPTIONAL based on user's working implementation.
 * The user's main.txt shows device ID is used only for debugging/inspection,
 * not for initialization validation. Their system works reliably without
 * device ID validation during init.
 * 
 * Justification for making it optional:
 * 1. User's working code proves it's not required for basic operation
 * 2. I2C communication check (Wire.endTransmission()) is sufficient for detection
 * 3. Device ID adds complexity without critical functional benefit
 * 4. Some INA228 variants may have different device IDs
 * 5. Focus should be on communication reliability, not device validation
 * 
 * When to use device ID checking:
 * - Hardware debugging and development
 * - Production testing and validation
 * - Systems with mixed sensor types on same I2C bus
 * - Diagnostic and troubleshooting functions
 */
bool ina228_validateDeviceID(uint8_t address, bool enableDebugOutput = false);
void ina228_inspectRegisters(uint8_t address, const char* sensorName);

#endif // INA228_DRIVER_H

/**
 * INTEGRATION EXAMPLE - THREE SENSOR SYSTEM:
 * 
 * #include "ina228_driver.h"
 * 
 * void setup() {
 *     Serial.begin(115200);
 *     Wire.begin();
 *     
 *     // Initialize all three sensors (based on user's main.txt configuration)
 *     if (!ina228_init(INA228_SOLAR_I2C_ADDRESS, "Solar")) {
 *         Serial.println("Failed to initialize Solar INA228!");
 *         return;
 *     }
 *     
 *     if (!ina228_init(INA228_BATTERY_I2C_ADDRESS, "Battery")) {
 *         Serial.println("Failed to initialize Battery INA228!");
 *         return;
 *     }
 *     
 *     if (!ina228_init(INA228_LOAD_I2C_ADDRESS, "Load")) {
 *         Serial.println("Failed to initialize Load INA228!");
 *         return;
 *     }
 *     
 *     // Optional: Inspect registers for debugging (like user's main.txt)
 *     ina228_inspectRegisters(INA228_SOLAR_I2C_ADDRESS, "Solar");
 *     ina228_inspectRegisters(INA228_BATTERY_I2C_ADDRESS, "Battery");
 *     ina228_inspectRegisters(INA228_LOAD_I2C_ADDRESS, "Load");
 * }
 * 
 * void loop() {
 *     INA228_Measurement_t measurements[3];
 *     uint8_t addresses[] = {INA228_SOLAR_I2C_ADDRESS, INA228_BATTERY_I2C_ADDRESS, INA228_LOAD_I2C_ADDRESS};
 *     
 *     if (ina228_takeMultipleMeasurements(addresses, 3, measurements)) {
 *         for (int i = 0; i < 3; i++) {
 *             ina228_printMeasurement(&measurements[i]);
 *         }
 *     }
 *     delay(1000);
 * }
 * 
 * CALIBRATION NOTES:
 * - Shunt resistance must be accurately known (user has different values per sensor)
 * - Current LSB affects measurement resolution
 * - Power register provides hardware-calculated power
 * - Device ID validation is optional - user's system works without it
 * - Always validate measurements in critical applications
 * - User's battery sensor uses different effective resistance (0.0177186Ω vs 0.015Ω)
 */ 