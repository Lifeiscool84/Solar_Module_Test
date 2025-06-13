/**
 * @file time_manager.cpp
 * @brief Implementation of TimeManager service class
 * 
 * Professional time coordination service extracted from main.txt solar monitoring system
 * Coordinates RTC and GNSS drivers for automatic time synchronization
 * 
 * Key Features:
 * - Automatic periodic sync with configurable interval (default: 2 hours)
 * - Timezone-aware local time management with CST/CDT support
 * - Robust error handling and fallback to RTC-only mode
 * - Service-layer architecture above hardware drivers
 * - Comprehensive statistics and monitoring
 */

#include "time_manager.h"

// Constructor with default configuration
TimeManager::TimeManager() : 
    rtc_driver(nullptr),
    gnss_driver(nullptr),
    config(),
    stats(),
    is_initialized(false),
    initialization_time_ms(0)
{
}

// Constructor with custom configuration
TimeManager::TimeManager(const TimeManagerConfig& cfg) : 
    rtc_driver(nullptr),
    gnss_driver(nullptr),
    config(cfg),
    stats(),
    is_initialized(false),
    initialization_time_ms(0)
{
}

// Initialize time manager with driver references
TimeManagerStatus TimeManager::initialize(RV8803_Driver& rtc, UBLOX_Driver& gnss) {
    Serial.println(F("Initializing TimeManager service..."));
    
    // Validate configuration first
    if (!validateConfig(config)) {
        Serial.println(F("ERROR: Invalid TimeManager configuration"));
        return TimeManagerStatus::INVALID_CONFIGURATION;
    }
    
    // Store driver references
    rtc_driver = &rtc;
    gnss_driver = &gnss;
    
    // Validate that drivers are ready
    if (!validateDrivers()) {
        Serial.println(F("ERROR: Driver validation failed"));
        is_initialized = false;
        return TimeManagerStatus::INITIALIZATION_FAILED;
    }
    
    // Initialize statistics
    stats = TimeSyncStats();
    stats.is_rtc_available = isRTCAvailable();
    stats.is_gnss_available = isGNSSAvailable();
    
    // Record initialization time
    initialization_time_ms = millis();
    
    // Calculate initial next sync time
    calculateNextSyncTime();
    
    is_initialized = true;
    
    Serial.println(F("TimeManager initialized successfully"));
    Serial.print(F("Sync interval: "));
    Serial.print(config.sync_interval_ms / 60000);
    Serial.println(F(" minutes"));
    Serial.print(F("Default timezone: "));
    Serial.println(config.default_timezone == TimeZone::CST ? F("CST") : F("CDT"));
    
    return TimeManagerStatus::SUCCESS;
}

// Force immediate synchronization with GNSS
TimeManagerStatus TimeManager::syncWithGNSS() {
    if (!is_initialized) {
        return TimeManagerStatus::INITIALIZATION_FAILED;
    }
    
    stats.total_sync_attempts++;
    
    Serial.print(F("Syncing RTC with GNSS ("));
    Serial.print(config.default_timezone == TimeZone::CST ? F("CST") : F("CDT"));
    Serial.print(F(")... "));
    
    TimeManagerStatus result = performGNSSSync();
    updateSyncStatistics(result);
    
    if (result == TimeManagerStatus::SUCCESS) {
        Serial.println(F("Success!"));
        calculateNextSyncTime();
    } else {
        Serial.print(F("Failed: "));
        Serial.println(getStatusString(result));
    }
    
    return result;
}

// Check and perform periodic synchronization
TimeManagerStatus TimeManager::periodicSync() {
    if (!is_initialized) {
        return TimeManagerStatus::INITIALIZATION_FAILED;
    }
    
    if (!config.enable_automatic_sync) {
        return TimeManagerStatus::SYNC_NOT_NEEDED;
    }
    
    if (!isTimeSyncNeeded()) {
        return TimeManagerStatus::SYNC_NOT_NEEDED;
    }
    
    Serial.println(F("Performing periodic GNSS time sync..."));
    return syncWithGNSS();
}

// Get current time from RTC
bool TimeManager::getCurrentTime(RTCTimestamp& timestamp) {
    if (!is_initialized || !isRTCAvailable()) {
        return false;
    }
    
    return rtc_driver->getTimestamp(timestamp);
}

// Check if time synchronization is needed
bool TimeManager::isTimeSyncNeeded() const {
    if (!is_initialized || !config.enable_automatic_sync) {
        return false;
    }
    
    uint32_t current_time = millis();
    return (current_time >= stats.next_sync_time_ms);
}

// Get time until next synchronization
uint32_t TimeManager::getTimeUntilNextSync() const {
    if (!is_initialized) {
        return 0;
    }
    
    uint32_t current_time = millis();
    
    if (current_time >= stats.next_sync_time_ms) {
        return 0;  // Overdue
    }
    
    return stats.next_sync_time_ms - current_time;
}

// Core synchronization implementation
TimeManagerStatus TimeManager::performGNSSSync() {
    // Check driver availability
    if (!isRTCAvailable()) {
        return TimeManagerStatus::RTC_NOT_AVAILABLE;
    }
    
    if (!isGNSSAvailable()) {
        return TimeManagerStatus::GNSS_NOT_AVAILABLE;
    }
    
    // Check for valid GNSS fix
    if (config.require_gnss_validation && !gnss_driver->hasValidFix()) {
        return TimeManagerStatus::GNSS_NO_FIX;
    }
    
    // Get GNSS time data
    GNSSTimeData gnss_time;
    if (!gnss_driver->getGNSSTime(gnss_time) || !gnss_time.time_valid) {
        return TimeManagerStatus::GNSS_INVALID_TIME;
    }
    
    // Extract timezone offset logic from main.txt syncRTCWithGNSSLocal()
    // Convert timezone enum to quarter-hour offset (like original code)
    int8_t currentTimeZone;
    if (config.default_timezone == TimeZone::CST) {
        currentTimeZone = -24;  // CST = UTC-6 = -6*4 quarter hours
    } else if (config.default_timezone == TimeZone::CDT) {
        currentTimeZone = -20;  // CDT = UTC-5 = -5*4 quarter hours
    } else {
        currentTimeZone = 0;    // UTC
    }
    
    // Apply timezone offset (extracted from main.txt logic)
    uint8_t hour = gnss_time.hour;
    uint8_t minute = gnss_time.minute;
    uint8_t second = gnss_time.second;
    uint8_t day = gnss_time.day;
    uint8_t month = gnss_time.month;
    uint16_t year = gnss_time.year;
    
    // Convert timezone from quarter hours to hours
    int8_t timezoneHours = currentTimeZone / 4;
    
    // Convert to minutes for calculation (from main.txt)
    int totalMinutes = hour * 60 + minute + (timezoneHours * 60);
    
    // Handle day rollover (extracted from main.txt)
    if (totalMinutes < 0) {
        totalMinutes += 24 * 60; // Add 24 hours
        day--; // Previous day
        if (day == 0) {
            // Handle month rollover
            month--;
            if (month == 0) {
                month = 12;
                year--;
            }
            // Set to last day of previous month (simplified)
            day = 31; // Will be corrected by RTC library
        }
    } else if (totalMinutes >= 24 * 60) {
        totalMinutes -= 24 * 60; // Subtract 24 hours
        day++; // Next day
        if (day > 31) { // Simplified day check
            day = 1;
            month++;
            if (month > 12) {
                month = 1;
                year++;
            }
        }
    }
    
    // Convert back to hours and minutes
    hour = totalMinutes / 60;
    minute = totalMinutes % 60;
    
    // Set RTC time using the extracted setTimeFromGNSS method
    bool result = rtc_driver->setTimeFromGNSS(year, month, day, hour, minute, second, false);
    
    if (!result) {
        return TimeManagerStatus::RTC_SET_FAILED;
    }
    
    return TimeManagerStatus::SUCCESS;
}

// Private helper methods
bool TimeManager::validateDrivers() const {
    if (!rtc_driver || !gnss_driver) {
        return false;
    }
    
    // Check if drivers are ready (they should be initialized before TimeManager)
    bool rtc_ready = rtc_driver->isReady();
    bool gnss_ready = gnss_driver->isReady();
    
    if (!rtc_ready) {
        Serial.println(F("WARNING: RTC driver not ready"));
        if (!config.fallback_to_rtc) {
            return false;
        }
    }
    
    if (!gnss_ready) {
        Serial.println(F("WARNING: GNSS driver not ready"));
    }
    
    // Require at least RTC to be available
    return rtc_ready;
}

bool TimeManager::isGNSSAvailable() const {
    return gnss_driver && gnss_driver->isReady();
}

bool TimeManager::isRTCAvailable() const {
    return rtc_driver && rtc_driver->isReady();
}

void TimeManager::updateSyncStatistics(TimeManagerStatus result) {
    if (result == TimeManagerStatus::SUCCESS) {
        stats.successful_syncs++;
        stats.last_sync_time_ms = millis();
    } else {
        stats.failed_syncs++;
    }
    
    stats.last_status = result;
    stats.is_gnss_available = isGNSSAvailable();
    stats.is_rtc_available = isRTCAvailable();
}

void TimeManager::calculateNextSyncTime() {
    uint32_t current_time = millis();
    stats.next_sync_time_ms = current_time + config.sync_interval_ms;
}

// Configuration and management methods
void TimeManager::setAutomaticSync(bool enable) {
    config.enable_automatic_sync = enable;
    
    if (enable) {
        calculateNextSyncTime();
    }
}

void TimeManager::setSyncInterval(uint32_t interval_ms) {
    // Minimum 5 minutes, maximum 24 hours
    if (interval_ms < 5UL * 60UL * 1000UL || interval_ms > 24UL * 60UL * 60UL * 1000UL) {
        Serial.println(F("WARNING: Invalid sync interval, using default"));
        return;
    }
    
    config.sync_interval_ms = interval_ms;
    calculateNextSyncTime();
}

void TimeManager::setDefaultTimezone(TimeZone tz) {
    config.default_timezone = tz;
    
    // Also update the RTC driver timezone
    if (rtc_driver) {
        rtc_driver->setTimezone(tz);
    }
}

TimeZone TimeManager::getDefaultTimezone() const {
    return config.default_timezone;
}

TimeManagerStatus TimeManager::setManualTime(uint16_t year, uint8_t month, uint8_t day,
                                           uint8_t hour, uint8_t minute, uint8_t second) {
    if (!is_initialized || !isRTCAvailable()) {
        return TimeManagerStatus::RTC_NOT_AVAILABLE;
    }
    
    bool result = rtc_driver->setCustomTime(year, month, day, hour, minute, second);
    
    if (result) {
        Serial.println(F("Manual time set successfully"));
        calculateNextSyncTime(); // Reset sync timer
        return TimeManagerStatus::SUCCESS;
    } else {
        return TimeManagerStatus::RTC_SET_FAILED;
    }
}

TimeZone TimeManager::toggleTimezone() {
    TimeZone new_tz = (config.default_timezone == TimeZone::CST) ? TimeZone::CDT : TimeZone::CST;
    setDefaultTimezone(new_tz);
    return new_tz;
}

// Status and monitoring methods
bool TimeManager::isReady() const {
    return is_initialized && validateDrivers();
}

const TimeSyncStats& TimeManager::getStatistics() const {
    return stats;
}

void TimeManager::resetStatistics() {
    stats = TimeSyncStats();
    stats.is_rtc_available = isRTCAvailable();
    stats.is_gnss_available = isGNSSAvailable();
    calculateNextSyncTime();
}

void TimeManager::printStatus() const {
    Serial.println(F("=== TimeManager Status ==="));
    Serial.print(F("Initialized: "));
    Serial.println(is_initialized ? F("YES") : F("NO"));
    
    Serial.print(F("RTC Available: "));
    Serial.println(isRTCAvailable() ? F("YES") : F("NO"));
    
    Serial.print(F("GNSS Available: "));
    Serial.println(isGNSSAvailable() ? F("YES") : F("NO"));
    
    Serial.print(F("Default Timezone: "));
    Serial.println(config.default_timezone == TimeZone::CST ? F("CST") : F("CDT"));
    
    Serial.print(F("Auto Sync: "));
    Serial.println(config.enable_automatic_sync ? F("ENABLED") : F("DISABLED"));
    
    Serial.print(F("Sync Interval: "));
    Serial.print(config.sync_interval_ms / 60000);
    Serial.println(F(" minutes"));
    
    // Current time
    RTCTimestamp current_time;
    if (getCurrentTime(current_time)) {
        Serial.print(F("Current Time: "));
        Serial.println(current_time.iso8601_string);
    }
    
    // Next sync time
    uint32_t time_until_sync = getTimeUntilNextSync();
    if (time_until_sync > 0) {
        Serial.print(F("Next Sync In: "));
        Serial.print(time_until_sync / 60000);
        Serial.println(F(" minutes"));
    } else {
        Serial.println(F("Next Sync: OVERDUE"));
    }
    
    Serial.print(F("Uptime: "));
    Serial.print(getUptime() / 1000);
    Serial.println(F(" seconds"));
    
    Serial.println(F("========================"));
}

void TimeManager::printStatistics() const {
    Serial.println(F("=== Sync Statistics ==="));
    Serial.print(F("Total Attempts: "));
    Serial.println(stats.total_sync_attempts);
    
    Serial.print(F("Successful: "));
    Serial.println(stats.successful_syncs);
    
    Serial.print(F("Failed: "));
    Serial.println(stats.failed_syncs);
    
    if (stats.total_sync_attempts > 0) {
        float success_rate = (float)stats.successful_syncs / stats.total_sync_attempts * 100.0;
        Serial.print(F("Success Rate: "));
        Serial.print(success_rate, 1);
        Serial.println(F("%"));
    }
    
    Serial.print(F("Last Status: "));
    Serial.println(getStatusString(stats.last_status));
    
    if (stats.last_sync_time_ms > 0) {
        Serial.print(F("Last Sync: "));
        Serial.print((millis() - stats.last_sync_time_ms) / 60000);
        Serial.println(F(" minutes ago"));
    }
    
    Serial.println(F("====================="));
}

const TimeManagerConfig& TimeManager::getConfig() const {
    return config;
}

void TimeManager::updateConfig(const TimeManagerConfig& new_config) {
    if (!validateConfig(new_config)) {
        Serial.println(F("ERROR: Invalid configuration rejected"));
        return;
    }
    
    config = new_config;
    calculateNextSyncTime();
    
    Serial.println(F("TimeManager configuration updated"));
}

uint32_t TimeManager::getUptime() const {
    if (!is_initialized) {
        return 0;
    }
    
    return millis() - initialization_time_ms;
}

void TimeManager::recalculateNextSync() {
    calculateNextSyncTime();
}

// Static utility methods
const char* TimeManager::getStatusString(TimeManagerStatus status) {
    switch (status) {
        case TimeManagerStatus::SUCCESS: return "Success";
        case TimeManagerStatus::RTC_NOT_AVAILABLE: return "RTC Not Available";
        case TimeManagerStatus::GNSS_NOT_AVAILABLE: return "GNSS Not Available";
        case TimeManagerStatus::GNSS_NO_FIX: return "GNSS No Fix";
        case TimeManagerStatus::GNSS_INVALID_TIME: return "GNSS Invalid Time";
        case TimeManagerStatus::RTC_SET_FAILED: return "RTC Set Failed";
        case TimeManagerStatus::COMMUNICATION_ERROR: return "Communication Error";
        case TimeManagerStatus::INVALID_CONFIGURATION: return "Invalid Configuration";
        case TimeManagerStatus::SYNC_IN_PROGRESS: return "Sync In Progress";
        case TimeManagerStatus::SYNC_NOT_NEEDED: return "Sync Not Needed";
        case TimeManagerStatus::INITIALIZATION_FAILED: return "Initialization Failed";
        default: return "Unknown Status";
    }
}

bool TimeManager::validateConfig(const TimeManagerConfig& cfg) {
    // Validate sync interval (5 minutes to 24 hours)
    if (cfg.sync_interval_ms < 5UL * 60UL * 1000UL || 
        cfg.sync_interval_ms > 24UL * 60UL * 60UL * 1000UL) {
        return false;
    }
    
    // Validate timeout (1 second to 5 minutes)
    if (cfg.sync_timeout_ms < 1000 || cfg.sync_timeout_ms > 5UL * 60UL * 1000UL) {
        return false;
    }
    
    // Validate retry count (0 to 10)
    if (cfg.max_sync_retries > 10) {
        return false;
    }
    
    return true;
} 