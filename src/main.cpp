/**
 * @file main.cpp
 * @brief GNSS Power Tracking Demonstration (MAIN VERSION)
 * 
 * This is the working main version using local SparkFun libraries directly.
 * For the professional driver version (with compilation issues), see:
 * gnss_power_tracking_demo.cpp.backup
 * 
 * Direct integration of GNSS tracking with power consumption monitoring
 * Uses available local libraries to demonstrate:
 * - GNSS position tracking with power consumption monitoring  
 * - RTC timestamping for data correlation
 * - Professional data logging to SD card
 * - Real-time power analysis during GNSS operations
 * 
 * Hardware Required:
 * - SparkFun RedBoard Artemis Nano
 * - RV8803 RTC module (I2C)
 * - u-blox GNSS module (I2C) 
 * - 2x INA228 power sensors (Battery: 0x44, Load: 0x41)
 * - SD card module
 * - Clear sky view for GNSS reception
 * 
 * @author Senior Embedded Systems Engineer
 * @date January 2025
 */

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SdFat.h>
#include <SparkFun_RV8803.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>

// Hardware Configuration
const uint8_t SD_CS_PIN = 8;
const uint8_t INA228_SOLAR_ADDRESS = 0x40;    // Solar panel sensor
const uint8_t INA228_BATTERY_ADDRESS = 0x44;  // Battery sensor
const uint8_t INA228_LOAD_ADDRESS = 0x41;     // Load sensor (after regulator)

// INA228 Register Addresses
const uint8_t INA228_REG_CONFIG = 0x00;
const uint8_t INA228_REG_VBUS = 0x05;
const uint8_t INA228_REG_CURRENT = 0x07;
const uint8_t INA228_REG_POWER = 0x08;
const uint8_t INA228_REG_SHUNTCAL = 0x02;
const uint8_t INA228_REG_DEVICEID = 0x3F;

// Power monitoring configuration
const float RSHUNT_OHMS = 0.0177186;           // Effective shunt resistance
const float MAX_CURRENT_A = 5.0;               // Maximum expected current
const float CURRENT_LSB = MAX_CURRENT_A / 524288.0;  // 19-bit resolution

// Demo Configuration
const uint32_t GNSS_SAMPLE_INTERVAL_MS = 5000;    // GNSS reading every 5 seconds
const uint32_t POWER_SAMPLE_INTERVAL_MS = 1000;   // Power monitoring every 1 second
const uint32_t STATUS_REPORT_INTERVAL_MS = 30000; // Status report every 30 seconds
const uint32_t DEMO_DURATION_MINUTES = 700;        // Total demo duration
const char CSV_FILENAME[] = "gnss_power_demo.csv";

// Timezone Configuration - Central Time Zone
// Note: RTC stores UTC time, we convert to local time for display/logging
const int CST_OFFSET_HOURS = -6;  // CST is UTC-6 (subtract 6 from UTC)
const int CDT_OFFSET_HOURS = -5;  // CDT is UTC-5 (subtract 5 from UTC)
bool isDST = true; // Currently in Central Daylight Time (GMT-5)

// Data Structure for Integrated Tracking
struct GNSSPowerLogEntry {
    // Timestamp information
    String timestamp_iso8601;         // ISO8601 formatted timestamp
    uint32_t system_millis;           // System millis for correlation
    
    // GNSS data
    bool gnss_reading_valid;          // GNSS data validity
    double latitude_deg;              // Latitude in degrees
    double longitude_deg;             // Longitude in degrees
    float altitude_m;                 // Altitude in meters
    uint8_t satellites_used;          // Number of satellites
    float hdop;                       // Horizontal dilution of precision
    uint8_t fix_type;                 // Fix type indicator
    
    // Power monitoring - Solar sensor
    float solar_voltage_V;
    float solar_current_mA;
    float solar_power_mW;
    
    // Power monitoring - Battery sensor
    float battery_voltage_V;
    float battery_current_mA;
    float battery_power_mW;
    
    // Power monitoring - Load sensor  
    float load_voltage_V;
    float load_current_mA;
    float load_power_mW;
    
    // Calculated power consumption
    float gnss_power_consumption_mW;   // Estimated GNSS power usage
    float system_efficiency_percent;    // Load/Battery power ratio
};

// Global Objects
RV8803 rtc;
SFE_UBLOX_GNSS gnss;
SdFat SD;

// Demo State Management
bool demo_active = true;
bool gnss_initialization_successful = false;
bool rtc_initialization_successful = false;
bool sd_card_available = false;  // Track SD card availability
uint32_t demo_start_time = 0;
uint32_t last_gnss_sample = 0;
uint32_t last_power_sample = 0;
uint32_t last_status_report = 0;
uint32_t total_gnss_readings = 0;
uint32_t successful_gnss_fixes = 0;

// Time Synchronization Management
bool gnss_time_available = false;           // GNSS time validity
bool rtc_synced_with_gnss = false;          // RTC sync status
uint32_t last_time_sync = 0;                // Last sync attempt
const uint32_t TIME_SYNC_INTERVAL_MS = 300000; // Sync every 5 minutes
bool rtc_stores_utc = true;                 // Will be determined automatically

// Power Monitoring Functions (Enhanced from sd_power_test_essentials.cpp)
bool ina228_writeRegister16(uint8_t address, uint8_t reg, uint16_t value) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write((value >> 8) & 0xFF);
    Wire.write(value & 0xFF);
    return (Wire.endTransmission() == 0);
}

uint16_t ina228_readRegister16(uint8_t address, uint8_t reg) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) return 0;
    
    Wire.requestFrom(address, (uint8_t)2);
    if (Wire.available() >= 2) {
        return ((uint16_t)Wire.read() << 8) | Wire.read();
    }
    return 0;
}

uint32_t ina228_readRegister24(uint8_t address, uint8_t reg) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) return 0;
    
    Wire.requestFrom(address, (uint8_t)3);
    if (Wire.available() >= 3) {
        uint32_t value = ((uint32_t)Wire.read() << 16) | 
                         ((uint32_t)Wire.read() << 8) | 
                         (uint32_t)Wire.read();
        return value;
    }
    return 0;
}

bool initializeINA228(uint8_t address, const char* sensor_name) {
    Serial.print(F("Initializing INA228 "));
    Serial.print(sensor_name);
    Serial.print(F(" (0x"));
    Serial.print(address, HEX);
    Serial.print(F(")... "));
    
    // Check device ID
    uint16_t deviceID = ina228_readRegister16(address, INA228_REG_DEVICEID);
    if (deviceID != 0x2280 && deviceID != 0x2281) {
        Serial.println(F("FAILED!"));
        return false;
    }
    
    // Configure for continuous measurement
    uint16_t config = 0x4000 | (0x4 << 6) | (0x4 << 3) | 0x03;
    if (!ina228_writeRegister16(address, INA228_REG_CONFIG, config)) {
        Serial.println(F("Configuration failed!"));
        return false;
    }
    
    // Set shunt calibration
    float shuntCal_f = 13107.2e6 * CURRENT_LSB * RSHUNT_OHMS;
    uint16_t shuntCal = (uint16_t)(shuntCal_f + 0.5);
    if (!ina228_writeRegister16(address, INA228_REG_SHUNTCAL, shuntCal)) {
        Serial.println(F("Calibration failed!"));
        return false;
    }
    
    Serial.println(F("Success!"));
    return true;
}

float ina228_readBusVoltage(uint8_t address) {
    uint32_t raw = ina228_readRegister24(address, INA228_REG_VBUS);
    return (float)((raw >> 4) * 195.3125e-6);  // Convert to volts
}

float ina228_readCurrent(uint8_t address) {
    uint32_t raw = ina228_readRegister24(address, INA228_REG_CURRENT);
    int32_t signed_raw = (int32_t)raw;
    
    if (signed_raw & 0x800000) {
        signed_raw |= 0xFF000000;  // Sign extend
    }
    signed_raw >>= 4;
    
    return (float)(signed_raw * CURRENT_LSB * 1000.0);  // Convert to mA
}

float ina228_readPower(uint8_t address) {
    uint32_t raw = ina228_readRegister24(address, INA228_REG_POWER);
    float powerLSB = 3.2 * CURRENT_LSB;
    return (float)(raw * powerLSB * 1000.0);  // Convert to mW
}

// GNSS Time Functions
struct GNSSTimeData {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    int32_t nanosecond;
    bool time_valid;
    bool time_confirmed;
};

bool getGNSSTime(GNSSTimeData& time_data) {
    if (!gnss_initialization_successful) {
        return false;
    }
    
    time_data.year = gnss.getYear();
    time_data.month = gnss.getMonth();
    time_data.day = gnss.getDay();
    time_data.hour = gnss.getHour();
    time_data.minute = gnss.getMinute();
    time_data.second = gnss.getSecond();
    time_data.nanosecond = gnss.getNanosecond();
    
    time_data.time_valid = gnss.getTimeValid();
    time_data.time_confirmed = gnss.getConfirmedTime();
    
    return time_data.time_valid;
}

// Time Synchronization Function
bool syncRTCWithGNSS() {
    if (!gnss_initialization_successful || !rtc_initialization_successful) {
        return false;
    }
    
    GNSSTimeData gnss_time;
    if (!getGNSSTime(gnss_time) || !gnss_time.time_confirmed) {
        return false;
    }
    
    // Set RTC to GNSS UTC time
    rtc.setTime(gnss_time.second, gnss_time.minute, gnss_time.hour, 
                0, // weekday (not used)
                gnss_time.day, gnss_time.month, gnss_time.year - 2000);
    
    rtc_synced_with_gnss = true;
    rtc_stores_utc = true;  // After sync, RTC definitely stores UTC
    last_time_sync = millis();
    
    Serial.println(F("RTC synchronized with GNSS UTC time"));
    return true;
}

// Determine if RTC is storing UTC or local time
void determineRTCTimeFormat() {
    if (!gnss_initialization_successful || !rtc_initialization_successful) {
        Serial.println(F("Cannot determine RTC format - using default assumption"));
        return;
    }
    
    GNSSTimeData gnss_time;
    if (!getGNSSTime(gnss_time) || !gnss_time.time_valid) {
        Serial.println(F("GNSS time not available - assuming RTC is UTC"));
        return;
    }
    
    // Read current RTC time
    if (!rtc.updateTime()) {
        Serial.println(F("Cannot read RTC time"));
        return;
    }
    
    int rtc_hour = rtc.getHours();
    int gnss_hour = gnss_time.hour;
    
    // Calculate hour difference
    int hour_diff = rtc_hour - gnss_hour;
    
    // Handle day boundary crossings
    if (hour_diff > 12) hour_diff -= 24;
    if (hour_diff < -12) hour_diff += 24;
    
    // If difference is close to timezone offset, RTC is in local time
    int expected_offset = isDST ? CDT_OFFSET_HOURS : CST_OFFSET_HOURS;
    
    if (abs(hour_diff - expected_offset) <= 1) {
        rtc_stores_utc = false;
        Serial.print(F("RTC stores LOCAL time ("));
        Serial.print(hour_diff);
        Serial.println(F(" hour offset from UTC)"));
    } else if (abs(hour_diff) <= 1) {
        rtc_stores_utc = true;
        Serial.println(F("RTC stores UTC time"));
    } else {
        Serial.print(F("RTC time unclear - "));
        Serial.print(hour_diff);
        Serial.println(F(" hour difference from GNSS"));
    }
}

// Enhanced RTC Functions with Time Synchronization
String getCurrentTimestamp() {
    // Try to use GNSS time first if available and valid
    if (gnss_initialization_successful) {
        GNSSTimeData gnss_time;
        if (getGNSSTime(gnss_time) && gnss_time.time_valid) {
            gnss_time_available = true;
            
            // Convert GNSS UTC time to local time
            int hour = gnss_time.hour;
            int day = gnss_time.day;
            int month = gnss_time.month;
            int year = gnss_time.year;
            
            // Apply timezone offset to convert UTC to local time
            int timezone_offset = isDST ? CDT_OFFSET_HOURS : CST_OFFSET_HOURS;
            hour += timezone_offset;
            
            // Handle day/month/year rollover
            if (hour < 0) {
                hour += 24;
                day--;
                if (day < 1) {
                    month--;
                    if (month < 1) {
                        month = 12;
                        year--;
                    }
                    // Simplified - assume 30 days per month for rollover
                    day = 30;
                }
            } else if (hour >= 24) {
                hour -= 24;
                day++;
                if (day > 30) {  // Simplified month boundary
                    day = 1;
                    month++;
                    if (month > 12) {
                        month = 1;
                        year++;
                    }
                }
            }
            
            // Format timestamp with GNSS-derived time
            String timestamp = String(year) + "-";
            if (month < 10) timestamp += "0";
            timestamp += String(month) + "-";
            if (day < 10) timestamp += "0";
            timestamp += String(day) + " ";
            if (hour < 10) timestamp += "0";
            timestamp += String(hour) + ":";
            if (gnss_time.minute < 10) timestamp += "0";
            timestamp += String(gnss_time.minute) + ":";
            if (gnss_time.second < 10) timestamp += "0";
            timestamp += String(gnss_time.second);
            
            return timestamp;
        }
    }
    
    // Fall back to RTC if GNSS time not available
    if (!rtc_initialization_successful) {
        return "TIME_NOT_AVAILABLE";
    }
    
    if (rtc.updateTime()) {
        int year = rtc.getYear();
        int month = rtc.getMonth();
        int day = rtc.getDate();
        int hour = rtc.getHours();
        int minute = rtc.getMinutes();
        int second = rtc.getSeconds();
        
        // Apply timezone conversion only if RTC stores UTC
        if (rtc_stores_utc) {
            int timezone_offset = isDST ? CDT_OFFSET_HOURS : CST_OFFSET_HOURS;
            hour += timezone_offset;
            
            // Handle day rollover
            if (hour < 0) {
                hour += 24;
                day--;
            } else if (hour >= 24) {
                hour -= 24;
                day++;
            }
        }
        // If RTC stores local time, use it directly
        
        // Format timestamp
        String timestamp = String(year) + "-";
        if (month < 10) timestamp += "0";
        timestamp += String(month) + "-";
        if (day < 10) timestamp += "0";
        timestamp += String(day) + " ";
        if (hour < 10) timestamp += "0";
        timestamp += String(hour) + ":";
        if (minute < 10) timestamp += "0";
        timestamp += String(minute) + ":";
        if (second < 10) timestamp += "0";
        timestamp += String(second);
        
        return timestamp;
    }
    
    return "TIME_READ_ERROR";
}

// GNSS Functions (Simplified from professional driver)
bool readGNSSPosition(GNSSPowerLogEntry& entry) {
    if (!gnss_initialization_successful) {
        entry.gnss_reading_valid = false;
        return false;
    }
    
    entry.latitude_deg = gnss.getLatitude() / 10000000.0;  // Convert to degrees
    entry.longitude_deg = gnss.getLongitude() / 10000000.0;
    entry.altitude_m = gnss.getAltitude() / 1000.0;  // Convert to meters
    entry.satellites_used = gnss.getSIV();
    entry.hdop = gnss.getHorizontalDOP() / 100.0;  // Convert to actual HDOP
    entry.fix_type = gnss.getFixType();
    
    // Validate the fix
    entry.gnss_reading_valid = (entry.fix_type >= 2) && (entry.satellites_used >= 4) && (entry.hdop <= 5.0);
    
    return true;
}

// Professional Data Logging with CSV Format
bool initializeCSVLogging() {
    File32 logFile = SD.open(CSV_FILENAME, O_WRONLY | O_CREAT | O_APPEND);
    if (!logFile) {
        Serial.println(F("ERROR: Could not create CSV file"));
        return false;
    }
    
    if (logFile.size() == 0) {
        // Write comprehensive CSV header
        logFile.println(F("Timestamp_24H,System_Millis_ms,GNSS_Valid,Latitude_deg,Longitude_deg,Altitude_m,Satellites_Used,HDOP,Fix_Type,Solar_Voltage_V,Solar_Current_mA,Solar_Power_mW,Battery_Voltage_V,Battery_Current_mA,Battery_Power_mW,Load_Voltage_V,Load_Current_mA,Load_Power_mW,GNSS_Power_Est_mW,System_Efficiency_pct"));
        Serial.println(F("CSV header written successfully"));
    }
    
    logFile.close();
    return true;
}

bool logGNSSPowerEntry(const GNSSPowerLogEntry& entry) {
    // Check if SD card is available for logging
    if (!sd_card_available) {
        return false;  // SD card not available, skip logging
    }
    
    File32 logFile = SD.open(CSV_FILENAME, O_WRONLY | O_APPEND);
    if (!logFile) return false;
    
    // Write data row
    logFile.print(entry.timestamp_iso8601);
    logFile.print(",");
    logFile.print(entry.system_millis);
    logFile.print(",");
    logFile.print(entry.gnss_reading_valid ? 1 : 0);
    logFile.print(",");
    
    if (entry.gnss_reading_valid) {
        logFile.print(entry.latitude_deg, 7);
        logFile.print(",");
        logFile.print(entry.longitude_deg, 7);
        logFile.print(",");
        logFile.print(entry.altitude_m, 2);
        logFile.print(",");
        logFile.print(entry.satellites_used);
        logFile.print(",");
        logFile.print(entry.hdop, 2);
        logFile.print(",");
        logFile.print(entry.fix_type);
    } else {
        logFile.print("0,0,0,0,0,0");  // Placeholder values
    }
    logFile.print(",");
    
    // Solar power data
    logFile.print(entry.solar_voltage_V, 4);
    logFile.print(",");
    logFile.print(entry.solar_current_mA, 2);
    logFile.print(",");
    logFile.print(entry.solar_power_mW, 2);
    logFile.print(",");
    
    // Battery power data
    logFile.print(entry.battery_voltage_V, 4);
    logFile.print(",");
    logFile.print(entry.battery_current_mA, 2);
    logFile.print(",");
    logFile.print(entry.battery_power_mW, 2);
    logFile.print(",");
    logFile.print(entry.load_voltage_V, 4);
    logFile.print(",");
    logFile.print(entry.load_current_mA, 2);
    logFile.print(",");
    logFile.print(entry.load_power_mW, 2);
    logFile.print(",");
    logFile.print(entry.gnss_power_consumption_mW, 2);
    logFile.print(",");
    logFile.print(entry.system_efficiency_percent, 1);
    logFile.println();
    
    logFile.close();
    return true;
}

// System Initialization
bool initializeSystem() {
    Serial.println(F("\n======================================================="));
    Serial.println(F("    GNSS POWER TRACKING DEMONSTRATION"));
    Serial.println(F("    Simplified Multi-Sensor Integration"));
    Serial.println(F("======================================================="));
    
    // Initialize I2C
    Serial.print(F("Initializing I2C bus... "));
    Wire.begin();
    Serial.println(F("Success!"));
    
    // Initialize RTC
    Serial.println(F("\n=== RTC Initialization ==="));
    if (rtc.begin()) {
        Serial.println(F("RTC initialized successfully"));
        rtc_initialization_successful = true;
        
        // Print current RTC time
        if (rtc.updateTime()) {
            Serial.print(F("Current RTC time: "));
            Serial.print(rtc.stringDateUSA());
            Serial.print(F(" "));
            Serial.print(rtc.stringTime());
            Serial.println();
        }
    } else {
        Serial.println(F("WARNING: RTC initialization failed"));
        rtc_initialization_successful = false;
    }
    
    // Initialize GNSS
    Serial.println(F("\n=== GNSS Initialization ==="));
    if (gnss.begin()) {
        Serial.println(F("GNSS initialized successfully"));
        gnss_initialization_successful = true;
        
        // Set measurement rate to 1Hz
        gnss.setNavigationFrequency(1);
        gnss.setAutoPVT(true);
        
        Serial.println(F("GNSS configured for 1Hz position updates"));
        
        // Wait a moment for GNSS to start providing data
        delay(2000);
        
        // Check GNSS time availability and determine RTC format
        Serial.println(F("\n=== Time Synchronization Setup ==="));
        if (rtc_initialization_successful) {
            determineRTCTimeFormat();
            
            // Attempt initial time synchronization
            if (syncRTCWithGNSS()) {
                Serial.println(F("Initial GNSS-RTC synchronization successful"));
            } else {
                Serial.println(F("Initial GNSS-RTC synchronization failed - will retry later"));
            }
        }
        
        // Show current time information
        Serial.print(F("Current timestamp: "));
        Serial.println(getCurrentTimestamp());
        
        if (gnss_time_available) {
            Serial.println(F("Time source: GNSS (satellite-accurate)"));
        } else if (rtc_initialization_successful) {
            Serial.print(F("Time source: RTC ("));
            Serial.print(rtc_stores_utc ? "UTC" : "Local");
            Serial.println(F(" format)"));
        } else {
            Serial.println(F("Time source: Not available"));
        }
        
    } else {
        Serial.println(F("WARNING: GNSS initialization failed"));
        Serial.println(F("Demo will continue with power monitoring only"));
        Serial.println(F("Time will use RTC only (no satellite synchronization)"));
        gnss_initialization_successful = false;
    }
    
    // Initialize power sensors
    Serial.println(F("\n=== Power Monitoring Initialization ==="));
    if (!initializeINA228(INA228_SOLAR_ADDRESS, "Solar")) return false;
    if (!initializeINA228(INA228_BATTERY_ADDRESS, "Battery")) return false;
    if (!initializeINA228(INA228_LOAD_ADDRESS, "Load")) return false;
    
    // Initialize SD card (optional - system continues without it)
    Serial.println(F("\n=== SD Card Initialization ==="));
    if (SD.begin(SD_CS_PIN)) {
        Serial.println(F("SD card initialized successfully"));
        sd_card_available = true;
        
        if (initializeCSVLogging()) {
            Serial.println(F("CSV logging initialized successfully"));
        } else {
            Serial.println(F("WARNING: CSV logging initialization failed"));
            Serial.println(F("Data will only be displayed on serial monitor"));
            sd_card_available = false;
        }
    } else {
        Serial.println(F("WARNING: SD card initialization failed"));
        Serial.println(F("System will continue without data logging to SD card"));
        Serial.println(F("Data will only be displayed on serial monitor"));
        sd_card_available = false;
    }
    
    sd_card_available = true;
    
    Serial.println(F("\n=== System Initialization Complete ==="));
    return true;
}

// Core Data Collection Function
void collectIntegratedData() {
    static float baseline_power_mW = 0;
    static bool baseline_established = false;
    
    GNSSPowerLogEntry entry;
    entry.system_millis = millis();
    entry.timestamp_iso8601 = getCurrentTimestamp();
    
    // Collect solar power data
    entry.solar_voltage_V = ina228_readBusVoltage(INA228_SOLAR_ADDRESS);
    entry.solar_current_mA = ina228_readCurrent(INA228_SOLAR_ADDRESS);
    entry.solar_power_mW = ina228_readPower(INA228_SOLAR_ADDRESS);
    
    // Collect battery power data
    entry.battery_voltage_V = ina228_readBusVoltage(INA228_BATTERY_ADDRESS);
    entry.battery_current_mA = ina228_readCurrent(INA228_BATTERY_ADDRESS);
    entry.battery_power_mW = ina228_readPower(INA228_BATTERY_ADDRESS);
    
    entry.load_voltage_V = ina228_readBusVoltage(INA228_LOAD_ADDRESS);
    entry.load_current_mA = ina228_readCurrent(INA228_LOAD_ADDRESS);
    entry.load_power_mW = ina228_readPower(INA228_LOAD_ADDRESS);
    
    // Calculate system efficiency (Solar to Load efficiency)
    if (entry.solar_power_mW > 0) {
        entry.system_efficiency_percent = (entry.load_power_mW / entry.solar_power_mW) * 100.0f;
    } else {
        entry.system_efficiency_percent = 0;
    }
    
    // Collect GNSS data if available
    uint32_t current_time = millis();
    if (gnss_initialization_successful && (current_time - last_gnss_sample >= GNSS_SAMPLE_INTERVAL_MS)) {
        readGNSSPosition(entry);
        
        if (entry.gnss_reading_valid) {
            total_gnss_readings++;
            successful_gnss_fixes++;
        } else {
            total_gnss_readings++;
        }
        
        last_gnss_sample = current_time;
        
        // Establish baseline power when GNSS is not actively tracking
        if (!baseline_established && !entry.gnss_reading_valid) {
            baseline_power_mW = entry.load_power_mW;
            baseline_established = true;
        }
        
        // Estimate GNSS power consumption
        if (baseline_established) {
            entry.gnss_power_consumption_mW = entry.load_power_mW - baseline_power_mW;
            if (entry.gnss_power_consumption_mW < 0) entry.gnss_power_consumption_mW = 0;
        }
    } else {
        entry.gnss_reading_valid = false;
        entry.gnss_power_consumption_mW = 0;
        entry.latitude_deg = 0;
        entry.longitude_deg = 0;
        entry.altitude_m = 0;
        entry.satellites_used = 0;
        entry.hdop = 0;
        entry.fix_type = 0;
    }
    
    // Log data to SD card if available
    bool logged_to_sd = logGNSSPowerEntry(entry);
    
    // Always output data to serial monitor for real-time monitoring
    Serial.print(F("["));
    Serial.print(entry.timestamp_iso8601);
    Serial.print(F("] "));
    
    // GNSS data
    if (entry.gnss_reading_valid) {
        Serial.print(F("GNSS: "));
        Serial.print(entry.latitude_deg, 6);
        Serial.print(F(","));
        Serial.print(entry.longitude_deg, 6);
        Serial.print(F(" Alt:"));
        Serial.print(entry.altitude_m, 1);
        Serial.print(F("m Sats:"));
        Serial.print(entry.satellites_used);
        Serial.print(F(" | "));
    } else {
        Serial.print(F("GNSS: No Fix | "));
    }
    
    // Power data
    Serial.print(F("Power: S:"));
    Serial.print(entry.solar_power_mW, 1);
    Serial.print(F("mW B:"));
    Serial.print(entry.battery_power_mW, 1);
    Serial.print(F("mW L:"));
    Serial.print(entry.load_power_mW, 1);
    Serial.print(F("mW"));
    
    // Logging status
    if (!sd_card_available) {
        Serial.print(F(" [SD:OFF]"));
    } else if (!logged_to_sd) {
        Serial.print(F(" [SD:FAIL]"));
    } else {
        Serial.print(F(" [SD:OK]"));
    }
    
    Serial.println();
}

// Status Reporting Function
void printStatusReport() {
    Serial.println(F("\n=== GNSS Power Tracking Status Report ==="));
    
    // System uptime
    uint32_t uptime_minutes = (millis() - demo_start_time) / 60000;
    Serial.print(F("Demo Runtime: "));
    Serial.print(uptime_minutes);
    Serial.println(F(" minutes"));
    
    // Current timestamp
    Serial.print(F("Current Time: "));
    Serial.println(getCurrentTimestamp());
    
    // GNSS performance
    if (gnss_initialization_successful) {
        float fix_success_rate = 0;
        if (total_gnss_readings > 0) {
            fix_success_rate = (float)successful_gnss_fixes / total_gnss_readings * 100.0f;
        }
        
        Serial.print(F("GNSS Fix Success Rate: "));
        Serial.print(fix_success_rate, 1);
        Serial.print(F("% ("));
        Serial.print(successful_gnss_fixes);
        Serial.print(F("/"));
        Serial.print(total_gnss_readings);
        Serial.println(F(")"));
        
        // Current GNSS status
        uint8_t fixType = gnss.getFixType();
        uint8_t satellites = gnss.getSIV();
        if (fixType >= 2) {
            Serial.print(F("Current Fix: Type "));
            Serial.print(fixType);
            Serial.print(F(", "));
            Serial.print(satellites);
            Serial.print(F(" satellites, HDOP: "));
            Serial.println(gnss.getHorizontalDOP() / 100.0, 2);
        } else {
            Serial.println(F("GNSS Status: No current fix"));
        }
    } else {
        Serial.println(F("GNSS Status: Not available"));
    }
    
    // Power consumption summary
    float current_solar_power = ina228_readPower(INA228_SOLAR_ADDRESS);
    float current_battery_power = ina228_readPower(INA228_BATTERY_ADDRESS);
    float current_load_power = ina228_readPower(INA228_LOAD_ADDRESS);
    
    Serial.print(F("Current Power: Solar "));
    Serial.print(current_solar_power, 2);
    Serial.print(F("mW, Battery "));
    Serial.print(current_battery_power, 2);
    Serial.print(F("mW, Load "));
    Serial.print(current_load_power, 2);
    Serial.println(F("mW"));
    
    // SD card logging status
    Serial.print(F("SD Card Logging: "));
    if (sd_card_available) {
        Serial.println(F("ACTIVE"));
    } else {
        Serial.println(F("DISABLED (No SD card)"));
    }
    
    // Time synchronization status
    Serial.print(F("Time Source: "));
    if (gnss_time_available) {
        Serial.print(F("GNSS (accurate)"));
        if (rtc_synced_with_gnss) {
            Serial.println(F(" + RTC synced"));
        } else {
            Serial.println(F(" + RTC not synced"));
        }
    } else if (rtc_initialization_successful) {
        Serial.print(F("RTC only ("));
        Serial.print(rtc_stores_utc ? "UTC" : "Local");
        Serial.println(F(" format)"));
    } else {
        Serial.println(F("Not available"));
    }
    
    Serial.println(F("=========================================="));
}

// Arduino Setup Function
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    // Initialize complete system
    if (!initializeSystem()) {
        Serial.println(F("FATAL ERROR: System initialization failed"));
        Serial.println(F("Check hardware connections and restart"));
        while (1) delay(1000);
    }
    
    // Record demo start time
    demo_start_time = millis();
    
    Serial.println(F("\n=== Starting GNSS Power Tracking Demo ==="));
    Serial.print(F("Demo Duration: "));
    Serial.print(DEMO_DURATION_MINUTES);
    Serial.println(F(" minutes"));
    Serial.print(F("GNSS Sampling: Every "));
    Serial.print(GNSS_SAMPLE_INTERVAL_MS / 1000);
    Serial.println(F(" seconds"));
    Serial.print(F("Power Sampling: Every "));
    Serial.print(POWER_SAMPLE_INTERVAL_MS / 1000);
    Serial.println(F(" seconds"));
    
    // Show logging status
    if (sd_card_available) {
        Serial.print(F("Data Logging: CSV file "));
        Serial.println(CSV_FILENAME);
        Serial.println(F("Data will be logged to SD card AND displayed on serial"));
    } else {
        Serial.println(F("Data Logging: SD card not available"));
        Serial.println(F("Data will ONLY be displayed on serial monitor"));
    }
    
    if (gnss_initialization_successful) {
        Serial.println(F("\nWaiting for GNSS fix... (may take up to 90 seconds)"));
        if (gnss_time_available) {
            Serial.println(F("GNSS time is available - timestamps will be satellite-accurate"));
        } else {
            Serial.println(F("GNSS time not yet available - using RTC for now"));
        }
    }
    
    Serial.println(F("Press 's' for status, 'q' to quit demo, 't' to toggle timezone"));
    Serial.println(F("=============================================="));
}

// Arduino Main Loop
void loop() {
    uint32_t current_time = millis();
    
    // Check for demo timeout
    if ((current_time - demo_start_time) > (DEMO_DURATION_MINUTES * 60UL * 1000UL)) {
        demo_active = false;
    }
    
    if (!demo_active) {
        Serial.println(F("\n=== Demo Complete ==="));
        if (sd_card_available) {
            Serial.print(F("Data logged to: "));
            Serial.println(CSV_FILENAME);
        } else {
            Serial.println(F("Data was displayed on serial monitor only (no SD card)"));
        }
        Serial.println(F("System will halt. Reset to run again."));
        while (1) delay(1000);
    }
    
    // Handle serial commands
    if (Serial.available()) {
        char command = Serial.read();
        if (command == 's' || command == 'S') {
            printStatusReport();
        } else if (command == 'q' || command == 'Q') {
            demo_active = false;
        } else if (command == 't' || command == 'T') {
            isDST = !isDST;
            Serial.print(F("Timezone changed to: "));
            Serial.println(isDST ? "CDT (UTC-5)" : "CST (UTC-6)");
        }
    }
    
    // Collect integrated sensor data
    if (current_time - last_power_sample >= POWER_SAMPLE_INTERVAL_MS) {
        collectIntegratedData();
        last_power_sample = current_time;
    }
    
    // Periodic time synchronization
    if (gnss_initialization_successful && rtc_initialization_successful && 
        (current_time - last_time_sync >= TIME_SYNC_INTERVAL_MS)) {
        if (syncRTCWithGNSS()) {
            Serial.println(F("Periodic GNSS-RTC synchronization completed"));
        }
    }
    
    // Periodic status reports
    if (current_time - last_status_report >= STATUS_REPORT_INTERVAL_MS) {
        printStatusReport();
        last_status_report = current_time;
    }
    
    delay(100);  // Prevent excessive CPU usage
} 