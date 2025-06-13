/**
 * SD Power Manager Module Header
 * ==============================
 * 
 * Comprehensive SD card power management interface for embedded systems
 * This module provides intelligent power management strategies for SD cards
 * to minimize power consumption in battery-operated devices.
 * 
 * Features:
 * - Multiple power management modes
 * - Intelligent initialization/deinitialization
 * - Power state monitoring
 * - Battery-optimized write strategies
 * - Automatic power optimization
 * 
 * Power Management Strategies:
 * 1. De-initialization: Complete SD card shutdown
 * 2. Idle Standby: Minimal power consumption
 * 3. Active Write: Optimized for continuous operations
 * 4. Batch Write: Periodic bulk operations
 * 
 * @author Reference Implementation Team
 * @date 2025
 */

#ifndef SD_POWER_MANAGER_H
#define SD_POWER_MANAGER_H

#include <Arduino.h>
#include <SdFat.h>

// ==============================================================================
// SD POWER MANAGEMENT CONSTANTS
// ==============================================================================

// SD card chip select pin configuration
#define SD_DEFAULT_CS_PIN        8
#define SD_POWER_CONTROL_PIN     -1  // Optional power control pin

// Power management timing constants
#define SD_INIT_DELAY_MS         100   // Delay after SD card initialization
#define SD_DEINIT_DELAY_MS       50    // Delay before SD card deinitialization
#define SD_WRITE_SETTLE_MS       10    // Delay between write operations
#define SD_BATCH_WRITE_INTERVAL  30000 // Default batch write interval (30s)

// Buffer size configurations
#define SD_DEFAULT_BUFFER_SIZE   64    // Default buffer size for data
#define SD_MAX_BUFFER_SIZE       256   // Maximum buffer size
#define SD_MIN_BUFFER_SIZE       16    // Minimum buffer size

// ==============================================================================
// SD POWER STATE DEFINITIONS
// ==============================================================================

/**
 * SD card power states for different operational modes
 * Each state represents a different power consumption profile
 */
typedef enum {
    SD_POWER_OFF = 0,           // SD card completely powered down
    SD_POWER_DEINITIALIZED,     // SD card powered but not initialized
    SD_POWER_IDLE_STANDBY,      // SD card initialized but idle
    SD_POWER_ACTIVE_READ,       // SD card in active read mode
    SD_POWER_ACTIVE_WRITE,      // SD card in active write mode
    SD_POWER_BATCH_MODE,        // SD card in batch operation mode
    SD_POWER_ERROR              // Error state
} SD_PowerState_t;

/**
 * SD write strategies for different power optimization scenarios
 */
typedef enum {
    SD_WRITE_IMMEDIATE = 0,     // Write data immediately (highest power)
    SD_WRITE_BUFFERED,          // Buffer data in RAM, write periodically
    SD_WRITE_BATCH,             // Collect data in batches, write in bursts
    SD_WRITE_ADAPTIVE           // Automatically select best strategy
} SD_WriteStrategy_t;

/**
 * SD power optimization levels
 */
typedef enum {
    SD_POWER_MAX_PERFORMANCE = 0, // Maximum performance, higher power
    SD_POWER_BALANCED,            // Balanced performance and power
    SD_POWER_MAX_EFFICIENCY,      // Maximum power efficiency
    SD_POWER_ULTRA_LOW            // Ultra-low power mode
} SD_PowerLevel_t;

// ==============================================================================
// DATA STRUCTURES
// ==============================================================================

/**
 * SD card configuration structure
 */
typedef struct {
    uint8_t cs_pin;                    // Chip select pin
    uint8_t power_control_pin;         // Optional power control pin
    uint32_t spi_speed;                // SPI communication speed
    SD_PowerLevel_t power_level;       // Power optimization level
    SD_WriteStrategy_t write_strategy; // Default write strategy
    uint16_t buffer_size;              // Buffer size for operations
    uint32_t batch_interval_ms;        // Batch write interval
    bool auto_deinit;                  // Auto-deinitialize when idle
    uint32_t idle_timeout_ms;          // Timeout before auto-deinit
    const char* volume_label;          // Optional volume label
} SD_Config_t;

// Function declarations would follow here...
// [Additional content truncated for brevity - see full implementation]

#endif // SD_POWER_MANAGER_H