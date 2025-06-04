/**
 * @file sd_power_test_config.h
 * @brief Configuration constants and shared definitions for SD Card Power Testing
 * 
 * This header file contains all configuration constants, hardware definitions,
 * and shared structures used by the SD card power consumption testing system.
 * 
 * @author Solar Module Test System
 * @date 2025
 */

#ifndef SD_POWER_TEST_CONFIG_H
#define SD_POWER_TEST_CONFIG_H

#include <stdint.h>

// ===== HARDWARE CONFIGURATION =====
// These values are copied from main.cpp for consistency

/** SD Card chip select pin (from main.cpp) */
const int SD_CHIP_SELECT_PIN = 8;

/** INA228 I2C Addresses (from main.cpp) */
const uint8_t INA228_SOLAR_ADDRESS = 0x40;
const uint8_t INA228_BATTERY_ADDRESS = 0x44;
const uint8_t INA228_LOAD_ADDRESS = 0x41;

/** Shunt resistor values (from main.cpp) */
const float SHUNT_RESISTOR_OHMS = 0.015f;
const float MAX_EXPECTED_CURRENT = 5.0f;

// ===== TEST CONFIGURATION =====

/** Number of rapid samples to collect per test */
const uint16_t SAMPLES_PER_MODE_TEST = 50;

/** Microsecond interval between rapid samples (20kHz sampling rate) */
const uint16_t SAMPLE_INTERVAL_MICROSECONDS = 50;

/** Number of test iterations for statistical significance */
const uint16_t TEST_ITERATION_COUNT = 2;

/** Delay between different tests in milliseconds */
const uint32_t INTER_TEST_DELAY_MILLISECONDS = 100;

/** Minimum power threshold for peak detection (mW) */
const float POWER_PEAK_THRESHOLD_MW = 10.0f;

/** I2C communication speed for measurements */
const uint32_t I2C_MEASUREMENT_SPEED = 400000; // 400kHz

// ===== DATA LOGGING CONFIGURATION =====

/** CSV data file name */
const char SD_POWER_CSV_FILENAME[] = "sd_power_data.csv";

/** Text log file name */
const char SD_POWER_LOG_FILENAME[] = "sd_power_log.txt";

/** Summary report file name */
const char SD_POWER_SUMMARY_FILENAME[] = "sd_power_summary.txt";

/** Maximum filename length for generated files */
const uint8_t MAX_FILENAME_LENGTH = 32;

// ===== MEASUREMENT PRECISION =====

/** Decimal places for voltage measurements */
const uint8_t VOLTAGE_DECIMAL_PLACES = 6;

/** Decimal places for current measurements */
const uint8_t CURRENT_DECIMAL_PLACES = 6;

/** Decimal places for power measurements */
const uint8_t POWER_DECIMAL_PLACES = 6;

/** Decimal places for summary statistics */
const uint8_t STATISTICS_DECIMAL_PLACES = 3;

// ===== MEMORY MANAGEMENT =====

/** Enable memory usage optimization for large datasets */
#define ENABLE_MEMORY_OPTIMIZATION 1

/** Maximum number of power peaks to track per test */
const uint8_t MAX_POWER_PEAKS_TRACKED = 10;

/** Buffer size for CSV line writing */
const uint16_t CSV_LINE_BUFFER_SIZE = 256;

// ===== VALIDATION THRESHOLDS =====

/** Minimum voltage for valid measurement (V) */
const float MIN_VALID_VOLTAGE = 0.1f;

/** Maximum voltage for valid measurement (V) */
const float MAX_VALID_VOLTAGE = 6.0f;

/** Maximum current for valid measurement (mA) */
const float MAX_VALID_CURRENT = 6000.0f;

/** Maximum power for valid measurement (mW) */
const float MAX_VALID_POWER = 30000.0f;

// ===== DEBUG AND LOGGING =====

/** Enable verbose debug output */
#define ENABLE_VERBOSE_DEBUG 0

/** Enable timing measurements */
#define ENABLE_TIMING_MEASUREMENTS 1

/** Enable power peak detection */
#define ENABLE_POWER_PEAK_DETECTION 1

/** Enable correlation analysis */
#define ENABLE_CORRELATION_ANALYSIS 1

// ===== SYSTEM COMPATIBILITY =====

/** Arduino platform specific optimizations */
#ifdef ARDUINO
  #define USE_ARDUINO_OPTIMIZATIONS 1
  #define USE_F_MACRO_FOR_STRINGS 1
#endif

/** Platform-specific yield function for long operations */
#ifdef ESP32
  #define SYSTEM_YIELD() yield()
#elif defined(ARDUINO)
  #define SYSTEM_YIELD() yield()
#else
  #define SYSTEM_YIELD() do {} while(0)
#endif

#endif // SD_POWER_TEST_CONFIG_H 