#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include "../../hardware/sensors/rtc/rv8803_driver.h"
#include "../../hardware/sensors/gnss/ublox_driver.h"

/**
 * @file time_manager.h
 * @brief Service-level time management coordinating RTC and GNSS drivers
 * 
 * Extracted and modernized from main.txt solar monitoring system
 * Provides coordinated time synchronization between RTC and GNSS modules
 * 
 * Key Features:
 * - Automatic GNSS time synchronization every 2 hours
 * - Timezone-aware local time management
 * - Fallback to RTC-only mode when GNSS unavailable
 * - Professional error handling and status reporting
 * - Service-layer coordination of hardware drivers
 */

/**
 * @brief Status codes for TimeManager operations
 */
enum class TimeManagerStatus : uint8_t {
    SUCCESS = 0,                    // Operation completed successfully
    RTC_NOT_AVAILABLE = 1,          // RTC driver not initialized or failed
    GNSS_NOT_AVAILABLE = 2,         // GNSS driver not initialized or failed
    GNSS_NO_FIX = 3,               // GNSS has no valid position/time fix
    GNSS_INVALID_TIME = 4,         // GNSS time data is invalid
    RTC_SET_FAILED = 5,            // Failed to update RTC time
    COMMUNICATION_ERROR = 6,        // Communication failure with hardware
    INVALID_CONFIGURATION = 7,      // Invalid time manager configuration
    SYNC_IN_PROGRESS = 8,          // Synchronization operation in progress
    SYNC_NOT_NEEDED = 9,           // Synchronization not currently needed
    INITIALIZATION_FAILED = 10      // TimeManager initialization failed
};

/**
 * @brief Time manager configuration
 */
struct TimeManagerConfig {
    uint32_t sync_interval_ms;      // Sync interval (default: 2 hours)
    bool enable_automatic_sync;     // Enable automatic periodic sync
    bool require_gnss_validation;   // Require GNSS fix validation before sync
    bool fallback_to_rtc;          // Allow RTC-only operation when GNSS unavailable
    TimeZone default_timezone;      // Default timezone for local time
    uint16_t sync_timeout_ms;       // Timeout for sync operations
    uint8_t max_sync_retries;       // Maximum retry attempts for failed sync
    
    // Default constructor
    TimeManagerConfig() :
        sync_interval_ms(2UL * 60UL * 60UL * 1000UL),  // 2 hours
        enable_automatic_sync(true),
        require_gnss_validation(true),
        fallback_to_rtc(true),
        default_timezone(TimeZone::CST),
        sync_timeout_ms(30000),     // 30 seconds
        max_sync_retries(3)
    {}
};

/**
 * @brief Time synchronization statistics
 */
struct TimeSyncStats {
    uint32_t total_sync_attempts;   // Total synchronization attempts
    uint32_t successful_syncs;      // Successful synchronizations
    uint32_t failed_syncs;         // Failed synchronizations
    uint32_t last_sync_time_ms;    // Timestamp of last successful sync
    uint32_t next_sync_time_ms;    // Timestamp of next scheduled sync
    TimeManagerStatus last_status; // Status of last sync attempt
    bool is_gnss_available;        // Current GNSS availability
    bool is_rtc_available;         // Current RTC availability
    
    // Constructor
    TimeSyncStats() :
        total_sync_attempts(0),
        successful_syncs(0),
        failed_syncs(0),
        last_sync_time_ms(0),
        next_sync_time_ms(0),
        last_status(TimeManagerStatus::SUCCESS),
        is_gnss_available(false),
        is_rtc_available(false)
    {}
};

/**
 * @brief Service class for coordinating time management between RTC and GNSS
 * 
 * Extracts functionality from main.txt:
 * - syncRTCWithGNSSLocal() → syncWithGNSS()
 * - checkPeriodicGNSSTimeSync() → periodicSync()
 */
class TimeManager {
private:
    // Driver references
    RV8803_Driver* rtc_driver;     // RTC driver instance
    UBLOX_Driver* gnss_driver;     // GNSS driver instance
    
    // Configuration and state
    TimeManagerConfig config;       // Time manager configuration
    TimeSyncStats stats;           // Synchronization statistics
    bool is_initialized;           // Initialization status
    uint32_t initialization_time_ms; // Time when manager was initialized
    
    // Private helper methods
    bool validateDrivers() const;
    bool isGNSSAvailable() const;
    bool isRTCAvailable() const;
    TimeManagerStatus performGNSSSync();
    void updateSyncStatistics(TimeManagerStatus result);
    void calculateNextSyncTime();
    const char* statusToString(TimeManagerStatus status) const;
    
public:
    /**
     * @brief Constructor with default configuration
     */
    TimeManager();
    
    /**
     * @brief Constructor with custom configuration
     * @param cfg Custom time manager configuration
     */
    explicit TimeManager(const TimeManagerConfig& cfg);
    
    /**
     * @brief Initialize time manager with RTC and GNSS drivers
     * @param rtc Reference to initialized RTC driver
     * @param gnss Reference to initialized GNSS driver
     * @return TimeManagerStatus indicating success or specific error
     * 
     * Note: Drivers must be already initialized before calling this method
     */
    TimeManagerStatus initialize(RV8803_Driver& rtc, UBLOX_Driver& gnss);
    
    /**
     * @brief Force immediate synchronization with GNSS
     * @return TimeManagerStatus indicating result
     * 
     * Extracted from: bool syncRTCWithGNSSLocal() in main.txt
     * Enhanced with proper error handling and timezone management
     */
    TimeManagerStatus syncWithGNSS();
    
    /**
     * @brief Check and perform periodic synchronization if needed
     * @return TimeManagerStatus indicating result
     * 
     * Extracted from: void checkPeriodicGNSSTimeSync() in main.txt
     * Enhanced with configurable intervals and error handling
     */
    TimeManagerStatus periodicSync();
    
    /**
     * @brief Get current time from RTC (with timezone applied)
     * @param timestamp Reference to RTCTimestamp structure to fill
     * @return true if successful
     */
    bool getCurrentTime(RTCTimestamp& timestamp);
    
    /**
     * @brief Check if time synchronization is needed
     * @return true if sync is due or overdue
     */
    bool isTimeSyncNeeded() const;
    
    /**
     * @brief Get time until next scheduled synchronization
     * @return milliseconds until next sync (0 if overdue)
     */
    uint32_t getTimeUntilNextSync() const;
    
    /**
     * @brief Enable or disable automatic periodic synchronization
     * @param enable true to enable automatic sync
     */
    void setAutomaticSync(bool enable);
    
    /**
     * @brief Update sync interval
     * @param interval_ms New sync interval in milliseconds
     */
    void setSyncInterval(uint32_t interval_ms);
    
    /**
     * @brief Set default timezone for local time operations
     * @param tz Timezone to use for local time
     */
    void setDefaultTimezone(TimeZone tz);
    
    /**
     * @brief Get current timezone setting
     * @return Current default timezone
     */
    TimeZone getDefaultTimezone() const;
    
    /**
     * @brief Force set RTC time manually (bypass GNSS)
     * @param year 4-digit year
     * @param month Month (1-12)
     * @param day Day (1-31)
     * @param hour Hour (0-23)
     * @param minute Minute (0-59)
     * @param second Second (0-59)
     * @return TimeManagerStatus indicating result
     */
    TimeManagerStatus setManualTime(uint16_t year, uint8_t month, uint8_t day,
                                   uint8_t hour, uint8_t minute, uint8_t second);
    
    /**
     * @brief Toggle timezone between CST and CDT
     * @return New timezone after toggle
     */
    TimeZone toggleTimezone();
    
    /**
     * @brief Check if time manager is properly initialized
     * @return true if manager is ready for use
     */
    bool isReady() const;
    
    /**
     * @brief Get synchronization statistics
     * @return Current TimeSyncStats structure
     */
    const TimeSyncStats& getStatistics() const;
    
    /**
     * @brief Reset synchronization statistics
     */
    void resetStatistics();
    
    /**
     * @brief Print comprehensive status to Serial
     * 
     * Includes driver status, sync statistics, and next sync time
     */
    void printStatus() const;
    
    /**
     * @brief Print sync statistics to Serial
     */
    void printStatistics() const;
    
    /**
     * @brief Get current time manager configuration
     * @return Current TimeManagerConfig
     */
    const TimeManagerConfig& getConfig() const;
    
    /**
     * @brief Update time manager configuration
     * @param new_config New configuration to apply
     */
    void updateConfig(const TimeManagerConfig& new_config);
    
    /**
     * @brief Get time since manager initialization
     * @return milliseconds since initialize() was called
     */
    uint32_t getUptime() const;
    
    /**
     * @brief Force immediate recalculation of next sync time
     * 
     * Useful after configuration changes
     */
    void recalculateNextSync();
    
    /**
     * @brief Get human-readable status string
     * @param status TimeManagerStatus to convert
     * @return String representation of status
     */
    static const char* getStatusString(TimeManagerStatus status);
    
    /**
     * @brief Validate time manager configuration
     * @param cfg Configuration to validate
     * @return true if configuration is valid
     */
    static bool validateConfig(const TimeManagerConfig& cfg);
};

#endif // TIME_MANAGER_H 