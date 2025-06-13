#ifndef RTC_TYPES_H
#define RTC_TYPES_H

#include <Arduino.h>

/**
 * @file rtc_types.h
 * @brief Data structures and types for RTC (RV8803) functionality
 * 
 * Extracted from main.txt solar monitoring system
 * Provides standardized types for time management across the multi-sensor system
 */

// Time zone definitions for Central Time (US)
enum class TimeZone : int8_t {
    CST = -24,  // Central Standard Time (UTC-6) in quarter hours
    CDT = -20   // Central Daylight Time (UTC-5) in quarter hours
};

// RTC initialization result codes
enum class RTCStatus : uint8_t {
    SUCCESS = 0,
    DEVICE_NOT_FOUND = 1,
    COMMUNICATION_ERROR = 2,
    INVALID_TIME = 3,
    COMPILER_TIME_SET_FAILED = 4
};

// Time setting modes
enum class TimeSetMode : uint8_t {
    COMPILER_TIME = 0,  // Set to compilation time
    MANUAL_TIME = 1,    // Set via user input
    GNSS_SYNC = 2,      // Set from GNSS time
    KEEP_CURRENT = 3    // Keep existing time
};

// Timestamp structure for consistent time representation
struct RTCTimestamp {
    uint32_t timestamp_ms;      // millis() when reading was taken
    char date_str[12];          // Date in YYYY-MM-DD format
    char time_str[10];          // Time in HH:MM:SS format (24-hour)
    TimeZone timezone;          // Current timezone setting
    bool rtc_valid;             // True if RTC communication successful
    
    // Constructor for easy initialization
    RTCTimestamp() : 
        timestamp_ms(0), 
        timezone(TimeZone::CDT), 
        rtc_valid(false) {
        strcpy(date_str, "RTC_ERROR");
        strcpy(time_str, "RTC_ERROR");
    }
};

// RTC configuration structure
struct RTCConfig {
    TimeZone default_timezone;
    bool auto_compiler_time;        // Set to compiler time if RTC invalid
    bool enable_serial_interface;   // Allow serial time setting
    uint32_t serial_timeout_ms;     // Timeout for serial time setting
    
    // Default configuration
    RTCConfig() :
        default_timezone(TimeZone::CDT),
        auto_compiler_time(true),
        enable_serial_interface(true),
        serial_timeout_ms(30000) {}
};

// Time adjustment structure for user commands
struct TimeAdjustment {
    enum Type {
        ADD_SECONDS,
        SUBTRACT_SECONDS,
        TOGGLE_TIMEZONE,
        SET_CUSTOM_TIME
    } type;
    
    int seconds;                // For ADD/SUBTRACT operations
    uint16_t year;              // For SET_CUSTOM_TIME
    uint8_t month, day, hour, minute, second;
    
    TimeAdjustment() : type(ADD_SECONDS), seconds(0), 
                      year(2025), month(1), day(1), 
                      hour(0), minute(0), second(0) {}
};

// Constants for time validation
namespace RTCConstants {
    const uint16_t MIN_VALID_YEAR = 2020;
    const uint16_t MAX_VALID_YEAR = 2030;
    const uint8_t MAX_ADJUSTMENT_HOURS = 1;  // Maximum single adjustment
    const uint32_t DEFAULT_SYNC_INTERVAL_MS = 2UL * 60UL * 60UL * 1000UL; // 2 hours
}

#endif // RTC_TYPES_H 