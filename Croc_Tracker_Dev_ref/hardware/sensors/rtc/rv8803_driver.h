#ifndef RV8803_DRIVER_H
#define RV8803_DRIVER_H

#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_RV8803.h>
#include "rtc_types.h"

/**
 * @file rv8803_driver.h
 * @brief Hardware abstraction layer for RV8803 Real-Time Clock
 * 
 * Extracted and modularized from main.txt solar monitoring system
 * Provides clean interface for RTC operations across multi-sensor applications
 * 
 * Key Features:
 * - Hardware initialization and configuration
 * - Time setting and reading with timezone support
 * - Serial interface for user time adjustment
 * - Integration with GNSS time synchronization
 * - Error handling and validation
 */

class RV8803_Driver {
private:
    RV8803 rtc;                     // SparkFun RV8803 library instance
    RTCConfig config;               // Configuration settings
    TimeZone current_timezone;      // Active timezone
    bool is_initialized;            // Initialization status
    
    // Private helper methods
    bool isTimeValid() const;
    bool setCompilerTime();
    bool adjustSecondsInternal(int seconds);
    
public:
    /**
     * @brief Constructor with default configuration
     */
    RV8803_Driver();
    
    /**
     * @brief Constructor with custom configuration
     * @param cfg Custom RTC configuration
     */
    explicit RV8803_Driver(const RTCConfig& cfg);
    
    /**
     * @brief Initialize RTC hardware and set default configuration
     * @return RTCStatus indicating success or specific error
     * 
     * Extracted from: bool initRTC() in main.txt
     */
    RTCStatus initialize();
    
    /**
     * @brief Get current timestamp with timezone applied
     * @param timestamp Reference to RTCTimestamp structure to fill
     * @return true if successful, false if RTC communication failed
     * 
     * Extracted from: void updateRTCTimestamp(SensorData& data) in main.txt
     */
    bool getTimestamp(RTCTimestamp& timestamp);
    
    /**
     * @brief Set timezone (CST or CDT)
     * @param tz Target timezone
     * @return true if successful
     * 
     * Extracted from: void setTimeZone(bool isDST) in main.txt
     */
    bool setTimezone(TimeZone tz);
    
    /**
     * @brief Get current timezone setting
     * @return Current TimeZone
     */
    TimeZone getTimezone() const;
    
    /**
     * @brief Apply time adjustment (add/subtract seconds, toggle timezone)
     * @param adjustment Time adjustment parameters
     * @return true if successful
     * 
     * Extracted from: void adjustTimeSeconds(int seconds) in main.txt
     */
    bool applyTimeAdjustment(const TimeAdjustment& adjustment);
    
    /**
     * @brief Set RTC time from GNSS data
     * @param gnss_year GNSS year (4-digit)
     * @param gnss_month GNSS month (1-12)
     * @param gnss_day GNSS day (1-31)
     * @param gnss_hour GNSS hour (0-23)
     * @param gnss_minute GNSS minute (0-59)
     * @param gnss_second GNSS second (0-59)
     * @param apply_timezone If true, apply current timezone offset to GNSS UTC time
     * @return true if successful
     * 
     * Extracted from: bool syncRTCWithGNSSLocal() in main.txt
     */
    bool setTimeFromGNSS(uint16_t gnss_year, uint8_t gnss_month, uint8_t gnss_day,
                         uint8_t gnss_hour, uint8_t gnss_minute, uint8_t gnss_second,
                         bool apply_timezone = true);
    
    /**
     * @brief Set custom time manually
     * @param year 4-digit year (2020-2030)
     * @param month Month (1-12)
     * @param day Day (1-31)
     * @param hour Hour (0-23)
     * @param minute Minute (0-59)
     * @param second Second (0-59)
     * @return true if successful
     */
    bool setCustomTime(uint16_t year, uint8_t month, uint8_t day,
                       uint8_t hour, uint8_t minute, uint8_t second);
    
    /**
     * @brief Check if RTC is properly initialized and communicating
     * @return true if RTC is ready for use
     */
    bool isReady() const;
    
    /**
     * @brief Print current RTC status and time to Serial
     * 
     * Extracted from: void printRTCStatus() in main.txt
     */
    void printStatus() const;
    
    /**
     * @brief Toggle between CST and CDT
     * @return New timezone after toggle
     * 
     * Extracted from: void toggleTimeZone() in main.txt
     */
    TimeZone toggleTimezone();
    
    /**
     * @brief Validate if stored time is reasonable
     * @return true if time appears valid (year in range, not all zeros)
     */
    bool validateStoredTime() const;
    
    /**
     * @brief Force set to compiler time (backup option)
     * @return true if successful
     */
    bool forceCompilerTime();
    
    /**
     * @brief Get underlying RV8803 library instance for advanced operations
     * @return Reference to SparkFun RV8803 object
     */
    RV8803& getRawDriver();
    
    /**
     * @brief Get driver configuration
     * @return Current RTCConfig
     */
    const RTCConfig& getConfig() const;
    
    /**
     * @brief Update driver configuration
     * @param new_config New configuration to apply
     */
    void updateConfig(const RTCConfig& new_config);
};

#endif // RV8803_DRIVER_H 