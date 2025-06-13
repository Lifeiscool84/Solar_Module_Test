/**
 * @file time_manager_usage.cpp
 * @brief Comprehensive usage example for TimeManager service
 * 
 * Demonstrates coordinated time management between RTC and GNSS drivers
 * using the service-level TimeManager architecture
 * 
 * Hardware Required:
 * - RV8803 RTC module (I2C)
 * - u-blox GNSS module (I2C)
 * - Clear view of sky for GNSS reception
 */

#include <Arduino.h>
#include <Wire.h>
#include "../time_manager.h"

// Global driver instances
RV8803_Driver rtc_driver;
UBLOX_Driver gnss_driver;
TimeManager time_manager;

// Configuration and timing
bool time_manager_enabled = true;
unsigned long last_status_print = 0;
unsigned long last_periodic_check = 0;
const unsigned long STATUS_PRINT_INTERVAL = 30000;     // Print status every 30 seconds
const unsigned long PERIODIC_CHECK_INTERVAL = 5000;    // Check for periodic sync every 5 seconds

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println(F("\n======================================================="));
    Serial.println(F("    TIMEMANAGER SERVICE DEMONSTRATION"));
    Serial.println(F("    Coordinated RTC & GNSS Time Synchronization"));
    Serial.println(F("======================================================="));
    Serial.println();
    
    // Initialize I2C
    Serial.print(F("Initializing I2C bus... "));
    Wire.begin();
    Serial.println(F("Success!"));
    
    // Initialize RTC driver
    Serial.println(F("Initializing RTC driver..."));
    RTCStatus rtc_status = rtc_driver.initialize();
    if (rtc_status != RTCStatus::SUCCESS) {
        Serial.print(F("FATAL ERROR: RTC initialization failed: "));
        Serial.println((int)rtc_status);
        while (1) delay(1000);
    }
    Serial.println(F("RTC driver initialized successfully"));
    
    // Initialize GNSS driver
    Serial.println(F("Initializing GNSS driver..."));
    GNSSStatus gnss_status = gnss_driver.initialize();
    if (gnss_status != GNSSStatus::SUCCESS) {
        Serial.print(F("WARNING: GNSS initialization failed: "));
        Serial.println((int)gnss_status);
        Serial.println(F("Continuing with RTC-only mode..."));
    } else {
        Serial.println(F("GNSS driver initialized successfully"));
    }
    
    // Configure TimeManager for demonstration
    TimeManagerConfig config;
    config.sync_interval_ms = 10UL * 60UL * 1000UL;  // 10 minutes for demo (normally 2 hours)
    config.enable_automatic_sync = true;
    config.require_gnss_validation = true;
    config.fallback_to_rtc = true;
    config.default_timezone = TimeZone::CST;
    config.sync_timeout_ms = 30000;
    config.max_sync_retries = 3;
    
    // Initialize TimeManager service
    TimeManager time_manager_custom(config);
    time_manager = time_manager_custom;
    
    TimeManagerStatus tm_status = time_manager.initialize(rtc_driver, gnss_driver);
    if (tm_status != TimeManagerStatus::SUCCESS) {
        Serial.print(F("FATAL ERROR: TimeManager initialization failed: "));
        Serial.println(TimeManager::getStatusString(tm_status));
        while (1) delay(1000);
    }
    
    Serial.println(F("=== TimeManager Service Ready ==="));
    time_manager.printStatus();
    Serial.println();
    
    // Demonstrate initial sync attempt
    Serial.println(F("=== Initial Sync Demonstration ==="));
    TimeManagerStatus sync_result = time_manager.syncWithGNSS();
    Serial.print(F("Initial sync result: "));
    Serial.println(TimeManager::getStatusString(sync_result));
    Serial.println();
    
    // Print current time
    RTCTimestamp current_time;
    if (time_manager.getCurrentTime(current_time)) {
        Serial.print(F("Current local time: "));
        Serial.println(current_time.iso8601_string);
    }
    
    Serial.println(F("=== Starting Periodic Operations ==="));
    Serial.println(F("Commands: 's' = force sync, 't' = toggle timezone, 'r' = reset stats, 'p' = print status"));
    Serial.println();
}

void loop() {
    unsigned long current_time = millis();
    
    // Handle serial commands
    handleSerialCommands();
    
    // Periodic sync check
    if (time_manager_enabled && (current_time - last_periodic_check >= PERIODIC_CHECK_INTERVAL)) {
        TimeManagerStatus periodic_result = time_manager.periodicSync();
        
        // Only print if sync was actually attempted
        if (periodic_result != TimeManagerStatus::SYNC_NOT_NEEDED) {
            Serial.print(F("[PERIODIC] Sync result: "));
            Serial.println(TimeManager::getStatusString(periodic_result));
        }
        
        last_periodic_check = current_time;
    }
    
    // Regular status printing
    if (current_time - last_status_print >= STATUS_PRINT_INTERVAL) {
        printCurrentStatus();
        last_status_print = current_time;
    }
    
    delay(100);  // Small delay for responsiveness
}

void handleSerialCommands() {
    if (Serial.available()) {
        char command = Serial.read();
        
        switch (command) {
            case 's':
            case 'S':
                Serial.println(F("\n=== Manual Sync Command ==="));
                {
                    TimeManagerStatus result = time_manager.syncWithGNSS();
                    Serial.print(F("Manual sync result: "));
                    Serial.println(TimeManager::getStatusString(result));
                    
                    if (result == TimeManagerStatus::SUCCESS) {
                        RTCTimestamp timestamp;
                        if (time_manager.getCurrentTime(timestamp)) {
                            Serial.print(F("Updated time: "));
                            Serial.println(timestamp.iso8601_string);
                        }
                    }
                }
                Serial.println();
                break;
                
            case 't':
            case 'T':
                Serial.println(F("\n=== Timezone Toggle ==="));
                {
                    TimeZone old_tz = time_manager.getDefaultTimezone();
                    TimeZone new_tz = time_manager.toggleTimezone();
                    
                    Serial.print(F("Timezone changed: "));
                    Serial.print(old_tz == TimeZone::CST ? F("CST") : F("CDT"));
                    Serial.print(F(" → "));
                    Serial.println(new_tz == TimeZone::CST ? F("CST") : F("CDT"));
                    
                    RTCTimestamp timestamp;
                    if (time_manager.getCurrentTime(timestamp)) {
                        Serial.print(F("Current time ("));
                        Serial.print(new_tz == TimeZone::CST ? F("CST") : F("CDT"));
                        Serial.print(F("): "));
                        Serial.println(timestamp.iso8601_string);
                    }
                }
                Serial.println();
                break;
                
            case 'r':
            case 'R':
                Serial.println(F("\n=== Statistics Reset ==="));
                time_manager.resetStatistics();
                Serial.println(F("Sync statistics reset"));
                time_manager.printStatistics();
                Serial.println();
                break;
                
            case 'p':
            case 'P':
                Serial.println(F("\n=== Status Report ==="));
                time_manager.printStatus();
                time_manager.printStatistics();
                Serial.println();
                break;
                
            case 'e':
            case 'E':
                time_manager_enabled = !time_manager_enabled;
                Serial.print(F("\nTimeManager periodic sync: "));
                Serial.println(time_manager_enabled ? F("ENABLED") : F("DISABLED"));
                time_manager.setAutomaticSync(time_manager_enabled);
                Serial.println();
                break;
                
            case 'h':
            case 'H':
            case '?':
                printHelpMenu();
                break;
                
            default:
                // Ignore invalid commands
                break;
        }
    }
}

void printCurrentStatus() {
    Serial.println(F("=== Periodic Status Update ==="));
    
    // Current time
    RTCTimestamp current_time;
    if (time_manager.getCurrentTime(current_time)) {
        Serial.print(F("Current Time: "));
        Serial.println(current_time.iso8601_string);
    } else {
        Serial.println(F("Current Time: ERROR - Unable to read"));
    }
    
    // Sync status
    uint32_t time_until_sync = time_manager.getTimeUntilNextSync();
    Serial.print(F("Next Sync: "));
    if (time_until_sync > 0) {
        Serial.print(time_until_sync / 60000);
        Serial.println(F(" minutes"));
    } else {
        Serial.println(F("OVERDUE"));
    }
    
    // Driver availability
    const TimeSyncStats& stats = time_manager.getStatistics();
    Serial.print(F("RTC Available: "));
    Serial.print(stats.is_rtc_available ? F("YES") : F("NO"));
    Serial.print(F(", GNSS Available: "));
    Serial.println(stats.is_gnss_available ? F("YES") : F("NO"));
    
    // Success rate
    if (stats.total_sync_attempts > 0) {
        float success_rate = (float)stats.successful_syncs / stats.total_sync_attempts * 100.0;
        Serial.print(F("Sync Success Rate: "));
        Serial.print(success_rate, 1);
        Serial.print(F("% ("));
        Serial.print(stats.successful_syncs);
        Serial.print(F("/"));
        Serial.print(stats.total_sync_attempts);
        Serial.println(F(")"));
    }
    
    Serial.print(F("Uptime: "));
    Serial.print(time_manager.getUptime() / 1000);
    Serial.println(F(" seconds"));
    
    Serial.println(F("==============================="));
}

void printHelpMenu() {
    Serial.println(F("\n=== TimeManager Command Menu ==="));
    Serial.println(F("s/S - Force manual GNSS sync"));
    Serial.println(F("t/T - Toggle timezone (CST ↔ CDT)"));
    Serial.println(F("r/R - Reset sync statistics"));
    Serial.println(F("p/P - Print detailed status"));
    Serial.println(F("e/E - Toggle automatic sync"));
    Serial.println(F("h/H/? - Show this help menu"));
    Serial.println(F("==============================="));
}

// Optional: Custom time manager configuration example
void demonstrateCustomConfiguration() {
    Serial.println(F("=== Custom Configuration Demo ==="));
    
    // Create custom configuration
    TimeManagerConfig custom_config;
    custom_config.sync_interval_ms = 5UL * 60UL * 1000UL;  // 5 minutes
    custom_config.enable_automatic_sync = true;
    custom_config.require_gnss_validation = false;  // Less strict for demo
    custom_config.fallback_to_rtc = true;
    custom_config.default_timezone = TimeZone::CDT;
    custom_config.sync_timeout_ms = 15000;  // 15 seconds
    custom_config.max_sync_retries = 5;
    
    // Validate configuration
    if (TimeManager::validateConfig(custom_config)) {
        Serial.println(F("Custom configuration is valid"));
        
        // Apply configuration
        time_manager.updateConfig(custom_config);
        Serial.println(F("Configuration updated successfully"));
    } else {
        Serial.println(F("Custom configuration is invalid"));
    }
    
    Serial.println(F("=============================="));
} 