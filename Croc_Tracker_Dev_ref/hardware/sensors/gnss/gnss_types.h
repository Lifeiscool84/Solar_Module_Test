#ifndef GNSS_TYPES_H
#define GNSS_TYPES_H

#include <Arduino.h>

/**
 * @file gnss_types.h
 * @brief Data structures and types for GNSS (u-blox) functionality
 * 
 * Extracted from main.txt solar monitoring system
 * Provides standardized types for GNSS position and time data
 */

// GNSS fix quality levels
enum class GNSSFixType : uint8_t {
    NO_FIX = 0,         // No GNSS fix available
    DEAD_RECKONING = 1, // Dead reckoning only
    FIX_2D = 2,         // 2D fix
    FIX_3D = 3,         // 3D fix
    GNSS_DEAD_RECKONING = 4, // GNSS + dead reckoning
    TIME_ONLY = 5       // Time only fix
};

// GNSS initialization result codes
enum class GNSSStatus : uint8_t {
    SUCCESS = 0,
    DEVICE_NOT_FOUND = 1,
    COMMUNICATION_ERROR = 2,
    CONFIGURATION_FAILED = 3,
    TIMEOUT = 4
};

// GNSS validation criteria for reliable positioning
struct GNSSValidationConfig {
    uint8_t min_satellites;         // Minimum satellites for valid fix
    float max_hdop;                 // Maximum HDOP for acceptable precision
    uint32_t max_age_ms;           // Maximum age of fix data (milliseconds)
    bool require_time_valid;        // Require valid time from GNSS
    
    // Default validation criteria from main.txt
    GNSSValidationConfig() :
        min_satellites(4),          // MIN_GNSS_SATS from validateGnssData()
        max_hdop(5.0f),            // GNSS_HDOP_THRESHOLD from validateGnssData()
        max_age_ms(10000),         // 10 seconds max age
        require_time_valid(true) {}
};

// Position data structure
struct GNSSPosition {
    double latitude_deg;            // Latitude in degrees (WGS84)
    double longitude_deg;           // Longitude in degrees (WGS84)
    float altitude_m;              // Altitude above mean sea level (meters)
    float horizontal_accuracy_m;    // Horizontal accuracy estimate (meters)
    float vertical_accuracy_m;      // Vertical accuracy estimate (meters)
    uint32_t timestamp_ms;         // millis() when reading was taken
    bool position_valid;           // True if position data is reliable
    
    GNSSPosition() :
        latitude_deg(0.0), longitude_deg(0.0), altitude_m(0.0f),
        horizontal_accuracy_m(999.9f), vertical_accuracy_m(999.9f),
        timestamp_ms(0), position_valid(false) {}
};

// Time data structure from GNSS
struct GNSSTimeData {
    uint16_t year;                 // 4-digit year
    uint8_t month;                 // Month (1-12)
    uint8_t day;                   // Day (1-31)
    uint8_t hour;                  // Hour (0-23) UTC
    uint8_t minute;                // Minute (0-59)
    uint8_t second;                // Second (0-59)
    uint32_t nanoseconds;          // Sub-second precision
    bool time_valid;               // True if time data is reliable
    bool time_confirmed;           // True if time is confirmed by GNSS
    
    GNSSTimeData() :
        year(2025), month(1), day(1), hour(0), minute(0), second(0),
        nanoseconds(0), time_valid(false), time_confirmed(false) {}
};

// Comprehensive GNSS data structure
struct GNSSData {
    // Fix information
    GNSSFixType fix_type;          // Type of GNSS fix
    uint8_t satellites_used;       // Number of satellites used in solution
    uint8_t satellites_visible;    // Total satellites visible
    float hdop;                    // Horizontal dilution of precision
    float pdop;                    // Position dilution of precision
    
    // Position and time
    GNSSPosition position;         // Position data
    GNSSTimeData time;            // Time data
    
    // Validation flags from main.txt logic
    bool gnss_fix_ok;             // GNSS fix OK flag from module
    bool validation_passed;        // Passed our validation criteria
    bool is_fixed;                // Overall fix status (used in main.txt)
    
    // Quality metrics
    uint32_t time_to_first_fix_ms; // Time since module start to first fix
    uint32_t last_update_ms;       // millis() of last data update
    
    // Constructor with safe defaults
    GNSSData() :
        fix_type(GNSSFixType::NO_FIX),
        satellites_used(0), satellites_visible(0),
        hdop(99.9f), pdop(99.9f),
        gnss_fix_ok(false), validation_passed(false), is_fixed(false),
        time_to_first_fix_ms(0), last_update_ms(0) {}
};

// GNSS configuration structure
struct GNSSConfig {
    uint32_t baud_rate;            // Communication baud rate
    uint16_t measurement_rate_ms;   // Measurement rate in milliseconds
    uint16_t navigation_rate;       // Navigation rate (measurements per nav solution)
    GNSSValidationConfig validation; // Validation criteria
    bool enable_high_precision;     // Enable high precision mode if available
    bool enable_time_sync;         // Enable automatic time synchronization
    uint32_t sync_interval_ms;     // Time sync interval
    
    // Default configuration based on main.txt
    GNSSConfig() :
        baud_rate(38400),          // Standard u-blox baud rate
        measurement_rate_ms(1000), // 1 Hz measurement rate
        navigation_rate(1),        // 1:1 nav rate
        enable_high_precision(true),
        enable_time_sync(true),
        sync_interval_ms(2UL * 60UL * 60UL * 1000UL) {} // 2 hours like main.txt
};

// Constants for GNSS operations
namespace GNSSConstants {
    const uint32_t COLD_START_TIMEOUT_MS = 90000; // 90 seconds for cold start
    const uint32_t WARM_START_TIMEOUT_MS = 30000; // 30 seconds for warm start
    const uint32_t DATA_TIMEOUT_MS = 5000;        // 5 seconds for data updates
    const uint8_t MAX_SATELLITES = 32;            // Maximum satellites to track
    
    // Position validation limits
    const double MIN_VALID_LATITUDE = -90.0;
    const double MAX_VALID_LATITUDE = 90.0;
    const double MIN_VALID_LONGITUDE = -180.0;
    const double MAX_VALID_LONGITUDE = 180.0;
    const float MAX_VALID_ALTITUDE = 50000.0f;    // 50km altitude limit
}

#endif // GNSS_TYPES_H 