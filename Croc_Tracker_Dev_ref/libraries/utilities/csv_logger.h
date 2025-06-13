/**
 * CSV Logger Module Header
 * ========================
 * 
 * Comprehensive CSV data logging system optimized for embedded systems
 * This module provides efficient, power-aware CSV file operations with
 * multiple buffering strategies and data validation.
 * 
 * Features:
 * - Multiple buffering strategies (immediate, buffered, batch)
 * - Automatic header generation and management
 * - Data type validation and formatting
 * - Memory-efficient operations
 * - Error recovery and data integrity checking
 * - Power-optimized write patterns
 * 
 * Designed for:
 * - Sensor data logging
 * - Power consumption monitoring
 * - Scientific data collection
 * - Industrial monitoring systems
 * 
 * @author Reference Implementation Team
 * @date 2025
 */

#ifndef CSV_LOGGER_H
#define CSV_LOGGER_H

#include <Arduino.h>
#include <SdFat.h>

// ==============================================================================
// CSV LOGGER CONFIGURATION
// ==============================================================================

// Default configuration values
#define CSV_DEFAULT_BUFFER_SIZE      128   // Default buffer size in entries
#define CSV_MAX_BUFFER_SIZE          512   // Maximum buffer size
#define CSV_MIN_BUFFER_SIZE          16    // Minimum buffer size
#define CSV_MAX_FIELD_COUNT          32    // Maximum fields per entry
#define CSV_MAX_FIELD_LENGTH         64    // Maximum length of a field
#define CSV_MAX_FILENAME_LENGTH      32    // Maximum filename length
#define CSV_BATCH_WRITE_THRESHOLD    64    // Default batch write threshold

// ==============================================================================
// DATA TYPE DEFINITIONS
// ==============================================================================

/**
 * CSV field data types for validation and formatting
 */
typedef enum {
    CSV_TYPE_STRING = 0,        // String/text data
    CSV_TYPE_INTEGER,           // Integer numbers
    CSV_TYPE_FLOAT,             // Floating point numbers
    CSV_TYPE_TIMESTAMP,         // Timestamp values
    CSV_TYPE_BOOLEAN,           // Boolean values (true/false)
    CSV_TYPE_HEX,               // Hexadecimal values
    CSV_TYPE_CUSTOM             // Custom formatting function
} CSV_FieldType_t;

/**
 * CSV write strategies for different scenarios
 */
typedef enum {
    CSV_WRITE_IMMEDIATE = 0,    // Write each entry immediately
    CSV_WRITE_BUFFERED,         // Buffer entries in RAM, write periodically
    CSV_WRITE_BATCH,            // Collect entries in batches
    CSV_WRITE_ADAPTIVE          // Automatically choose best strategy
} CSV_WriteStrategy_t;

/**
 * CSV field definition structure
 */
typedef struct {
    const char* name;           // Field name for header
    CSV_FieldType_t type;       // Data type for validation
    uint8_t precision;          // Decimal precision for floats
    const char* unit;           // Optional unit suffix
    bool required;              // Whether field is required
    float min_value;            // Minimum valid value (for numbers)
    float max_value;            // Maximum valid value (for numbers)
} CSV_FieldDef_t;

// Function declarations would follow here...
// [Additional content truncated for brevity - see full implementation]

#endif // CSV_LOGGER_H