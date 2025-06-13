/**
 * @file rv8803_driver.cpp
 * @brief Implementation of RV8803 Real-Time Clock Hardware Driver
 * 
 * Extracted and modernized from main.txt solar monitoring system
 * Provides professional hardware abstraction layer for RTC operations
 * 
 * Original Functions Extracted:
 * - bool initRTC() → RTCStatus initialize()
 * - void setTimeZone(bool isDST) → bool setTimezone(TimeZone tz)
 * - void updateRTCTimestamp(SensorData& data) → bool getTimestamp(RTCTimestamp& timestamp)
 * - void adjustTimeSeconds(int seconds) → bool applyTimeAdjustment(const TimeAdjustment& adjustment)
 * 
 * @author Senior Embedded Systems Engineer
 * @date 2025-01-23
 * @version 1.0
 */

#include "rv8803_driver.h"

// Default constructor - uses default configuration
RV8803_Driver::RV8803_Driver() : 
    config(RTCConfig()),
    current_timezone(TimeZone::CDT),
    is_initialized(false) {
}

// Constructor with custom configuration
RV8803_Driver::RV8803_Driver(const RTCConfig& cfg) : 
    config(cfg),
    current_timezone(cfg.default_timezone),
    is_initialized(false) {
}

/**
 * @brief Initialize RTC hardware and set default configuration
 * 
 * Extracted from: bool initRTC() in main.txt (lines 1055-1072)
 * Improvements:
 * - Enhanced error handling with specific error codes
 * - Proper timezone initialization
 * - Validation of stored time
 * - Professional error reporting
 */
RTCStatus RV8803_Driver::initialize() {
    // Attempt to initialize the RV8803 hardware
    if (rtc.begin() == false) {
        is_initialized = false;
        return RTCStatus::DEVICE_NOT_FOUND;
    }
    
    // Set timezone to configured default
    if (!setTimezone(config.default_timezone)) {
        is_initialized = false;
        return RTCStatus::COMMUNICATION_ERROR;
    }
    
    // Check if stored time is valid
    if (!validateStoredTime()) {
        if (config.auto_compiler_time) {
            if (!setCompilerTime()) {
                is_initialized = false;
                return RTCStatus::COMPILER_TIME_SET_FAILED;
            }
        } else {
            // Time is invalid but auto-set disabled
            is_initialized = false;
            return RTCStatus::INVALID_TIME;
        }
    }
    
    is_initialized = true;
    return RTCStatus::SUCCESS;
}

/**
 * @brief Get current timestamp with timezone applied
 * 
 * Extracted from: void updateRTCTimestamp(SensorData& data) in main.txt (lines 1281-1301)
 * Improvements:
 * - Returns success/failure status
 * - Uses standardized RTCTimestamp structure
 * - Proper error handling for communication failures
 * - Maintains compatibility with original timestamp format
 */
bool RV8803_Driver::getTimestamp(RTCTimestamp& timestamp) {
    if (!is_initialized) {
        return false;
    }
    
    timestamp.timestamp_ms = millis(); // Keep millis for timing intervals
    timestamp.timezone = current_timezone;
    
    if (rtc.updateTime() == false) {
        // If RTC update fails, mark as invalid but preserve timestamp_ms
        strcpy(timestamp.date_str, "RTC_ERROR");
        strcpy(timestamp.time_str, "RTC_ERROR");
        timestamp.rtc_valid = false;
        return false;
    }
    
    // Create ISO 8601 formatted timestamp: YYYY-MM-DDTHH:MM:SS
    // Note: RV8803 library getYear() already returns 4-digit year (e.g., 2025)
    snprintf(timestamp.date_str, sizeof(timestamp.date_str), 
             "%04d-%02d-%02d",
             rtc.getYear(),      // Already 4-digit year, no need to add 2000
             rtc.getMonth(),
             rtc.getDate());
    snprintf(timestamp.time_str, sizeof(timestamp.time_str), 
             "%02d:%02d:%02d",
             rtc.getHours(),
             rtc.getMinutes(),
             rtc.getSeconds());
    
    timestamp.rtc_valid = true;
    return true;
}

/**
 * @brief Set timezone (CST or CDT)
 * 
 * Extracted from: void setTimeZone(bool isDST) in main.txt (lines 1074-1087)
 * Improvements:
 * - Uses enum instead of boolean for type safety
 * - Returns success status
 * - Preserves original quarter-hour timezone logic
 */
bool RV8803_Driver::setTimezone(TimeZone tz) {
    if (!is_initialized && tz != config.default_timezone) {
        // Allow setting default timezone even if not initialized
        return false;
    }
    
    int8_t quarter_hours = static_cast<int8_t>(tz);
    
    // Only call RTC functions if hardware is initialized
    if (is_initialized) {
        if (!rtc.setTimeZoneQuarterHours(quarter_hours)) {
            return false;
        }
    }
    
    current_timezone = tz;
    return true;
}

/**
 * @brief Get current timezone setting
 */
TimeZone RV8803_Driver::getTimezone() const {
    return current_timezone;
}

/**
 * @brief Apply time adjustment (add/subtract seconds, toggle timezone)
 * 
 * Extracted from: void adjustTimeSeconds(int seconds) in main.txt (lines 1348-1423)
 * Improvements:
 * - Unified interface for different adjustment types
 * - Enhanced overflow handling
 * - Professional error handling and validation
 * - Support for multiple adjustment types
 */
bool RV8803_Driver::applyTimeAdjustment(const TimeAdjustment& adjustment) {
    if (!is_initialized) {
        return false;
    }
    
    switch (adjustment.type) {
        case TimeAdjustment::ADD_SECONDS:
            return adjustSecondsInternal(adjustment.seconds);
            
        case TimeAdjustment::SUBTRACT_SECONDS:
            return adjustSecondsInternal(-adjustment.seconds);
            
        case TimeAdjustment::TOGGLE_TIMEZONE:
            {
                TimeZone new_tz = (current_timezone == TimeZone::CST) ? 
                                  TimeZone::CDT : TimeZone::CST;
                return setTimezone(new_tz);
            }
            
        case TimeAdjustment::SET_CUSTOM_TIME:
            return setCustomTime(adjustment.year, adjustment.month, adjustment.day,
                               adjustment.hour, adjustment.minute, adjustment.second);
            
        default:
            return false;
    }
}

/**
 * @brief Set RTC time from GNSS data
 * 
 * Extracted from GNSS sync functionality in main.txt
 * Provides interface for setting RTC from GNSS time with timezone handling
 */
bool RV8803_Driver::setTimeFromGNSS(uint16_t gnss_year, uint8_t gnss_month, uint8_t gnss_day,
                                     uint8_t gnss_hour, uint8_t gnss_minute, uint8_t gnss_second,
                                     bool apply_timezone) {
    if (!is_initialized) {
        return false;
    }
    
    // Validate GNSS time parameters
    if (gnss_year < RTCConstants::MIN_VALID_YEAR || gnss_year > RTCConstants::MAX_VALID_YEAR ||
        gnss_month < 1 || gnss_month > 12 ||
        gnss_day < 1 || gnss_day > 31 ||
        gnss_hour > 23 || gnss_minute > 59 || gnss_second > 59) {
        return false;
    }
    
    uint8_t final_hour = gnss_hour;
    uint8_t final_day = gnss_day;
    uint8_t final_month = gnss_month;
    uint16_t final_year = gnss_year;
    
    // Apply timezone offset if requested (GNSS time is UTC)
    if (apply_timezone) {
        int timezone_hours = static_cast<int>(current_timezone) / 4; // Convert quarter hours to hours
        int adjusted_hour = static_cast<int>(gnss_hour) + timezone_hours;
        
        // Handle day rollover
        if (adjusted_hour < 0) {
            adjusted_hour += 24;
            final_day = (gnss_day > 1) ? gnss_day - 1 : 31; // Simplified day handling
        } else if (adjusted_hour >= 24) {
            adjusted_hour -= 24;
            final_day = gnss_day + 1; // Simplified day handling
        }
        
        final_hour = static_cast<uint8_t>(adjusted_hour);
    }
    
    // Set the time using RTC library
    return rtc.setTime(gnss_second, gnss_minute, final_hour, 1, final_day, final_month, final_year);
}

/**
 * @brief Set custom time manually
 */
bool RV8803_Driver::setCustomTime(uint16_t year, uint8_t month, uint8_t day,
                                  uint8_t hour, uint8_t minute, uint8_t second) {
    if (!is_initialized) {
        return false;
    }
    
    // Validate parameters
    if (year < RTCConstants::MIN_VALID_YEAR || year > RTCConstants::MAX_VALID_YEAR ||
        month < 1 || month > 12 ||
        day < 1 || day > 31 ||
        hour > 23 || minute > 59 || second > 59) {
        return false;
    }
    
    return rtc.setTime(second, minute, hour, 1, day, month, year);
}

/**
 * @brief Check if RTC is properly initialized and communicating
 */
bool RV8803_Driver::isReady() const {
    return is_initialized;
}

/**
 * @brief Print current RTC status and time to Serial
 * 
 * Extracted from: void printRTCStatus() in main.txt (lines around 1088-1110)
 * Improvements:
 * - Comprehensive status reporting
 * - Timezone information display
 * - Professional formatting
 */
void RV8803_Driver::printStatus() const {
    if (!is_initialized) {
        Serial.println(F("RTC Status: NOT INITIALIZED"));
        return;
    }
    
    if (rtc.updateTime() == false) {
        Serial.println(F("RTC Status: COMMUNICATION ERROR"));
        return;
    }
    
    String currentDate = rtc.stringDateUSA(); // MM/DD/YYYY
    String currentTime = rtc.stringTime();     // HH:MM:SS
    
    Serial.print(F("RTC Status: ONLINE | Date: ")); Serial.print(currentDate);
    Serial.print(F(" | Time: ")); Serial.print(currentTime);
    
    if (current_timezone == TimeZone::CST) {
        Serial.println(F(" CST (UTC-6)"));
    } else if (current_timezone == TimeZone::CDT) {
        Serial.println(F(" CDT (UTC-5)"));
    } else {
        Serial.print(F(" UTC")); 
        Serial.println(static_cast<int>(current_timezone) / 4.0f, 1);
    }
}

/**
 * @brief Toggle between CST and CDT
 * 
 * Extracted from: void toggleTimeZone() in main.txt
 */
TimeZone RV8803_Driver::toggleTimezone() {
    TimeZone new_tz = (current_timezone == TimeZone::CST) ? TimeZone::CDT : TimeZone::CST;
    if (setTimezone(new_tz)) {
        return new_tz;
    }
    return current_timezone; // Return current if toggle failed
}

/**
 * @brief Validate if stored time is reasonable
 * 
 * Extracted from validation logic in main.txt setup() function
 */
bool RV8803_Driver::validateStoredTime() const {
    if (!rtc.updateTime()) {
        return false;
    }
    
    uint16_t year = rtc.getYear();
    uint8_t month = rtc.getMonth();
    uint8_t day = rtc.getDate();
    uint8_t hour = rtc.getHours();
    uint8_t minute = rtc.getMinutes();
    uint8_t second = rtc.getSeconds();
    
    // Check for obviously invalid time
    if (year < RTCConstants::MIN_VALID_YEAR || year > RTCConstants::MAX_VALID_YEAR ||
        (year == 2000 && month == 1 && day == 1 && hour == 0 && minute == 0 && second == 0)) {
        return false;
    }
    
    return true;
}

/**
 * @brief Force set to compiler time (backup option)
 */
bool RV8803_Driver::forceCompilerTime() {
    if (!is_initialized) {
        return false;
    }
    
    return setCompilerTime();
}

/**
 * @brief Get underlying RV8803 library instance for advanced operations
 */
RV8803& RV8803_Driver::getRawDriver() {
    return rtc;
}

/**
 * @brief Get driver configuration
 */
const RTCConfig& RV8803_Driver::getConfig() const {
    return config;
}

/**
 * @brief Update driver configuration
 */
void RV8803_Driver::updateConfig(const RTCConfig& new_config) {
    config = new_config;
    // Apply timezone change if different
    if (new_config.default_timezone != current_timezone) {
        setTimezone(new_config.default_timezone);
    }
}

// ========================================
// PRIVATE HELPER METHODS
// ========================================

/**
 * @brief Check if current time reading is valid
 */
bool RV8803_Driver::isTimeValid() const {
    return validateStoredTime();
}

/**
 * @brief Set RTC to compiler time
 * 
 * Extracted from compiler time setting logic in main.txt
 */
bool RV8803_Driver::setCompilerTime() {
    return rtc.setToCompilerTime();
}

/**
 * @brief Internal method for adjusting seconds with overflow handling
 * 
 * Extracted from: void adjustTimeSeconds(int seconds) in main.txt (lines 1348-1423)
 * Enhanced with proper error checking and validation
 */
bool RV8803_Driver::adjustSecondsInternal(int seconds) {
    if (rtc.updateTime() == false) {
        return false;
    }
    
    // Get current time components
    uint8_t currentSeconds = rtc.getSeconds();
    uint8_t currentMinutes = rtc.getMinutes();
    uint8_t currentHours = rtc.getHours();
    uint8_t currentDay = rtc.getDate();
    uint8_t currentMonth = rtc.getMonth();
    uint16_t currentYear = rtc.getYear();
    uint8_t currentWeekday = rtc.getWeekday();
    
    // Calculate new seconds with proper overflow handling
    int newSeconds = currentSeconds + seconds;
    int carryMinutes = 0;
    
    if (newSeconds >= 60) {
        carryMinutes = newSeconds / 60;
        newSeconds = newSeconds % 60;
    } else if (newSeconds < 0) {
        carryMinutes = (newSeconds - 59) / 60; // This will be negative
        newSeconds = (newSeconds % 60 + 60) % 60;
    }
    
    int newMinutes = currentMinutes + carryMinutes;
    int carryHours = 0;
    
    if (newMinutes >= 60) {
        carryHours = newMinutes / 60;
        newMinutes = newMinutes % 60;
    } else if (newMinutes < 0) {
        carryHours = (newMinutes - 59) / 60;
        newMinutes = (newMinutes % 60 + 60) % 60;
    }
    
    int newHours = currentHours + carryHours;
    if (newHours >= 24) {
        newHours = newHours % 24;
    } else if (newHours < 0) {
        newHours = (newHours % 24 + 24) % 24;
    }
    
    // Set the new time
    return rtc.setTime(static_cast<uint8_t>(newSeconds), 
                       static_cast<uint8_t>(newMinutes), 
                       static_cast<uint8_t>(newHours), 
                       currentWeekday, 
                       currentDay, 
                       currentMonth, 
                       currentYear);
} 