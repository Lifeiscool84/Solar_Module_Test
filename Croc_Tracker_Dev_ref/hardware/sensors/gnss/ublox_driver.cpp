/**
 * @file ublox_driver.cpp
 * @brief Implementation of UBLOX_Driver class for u-blox GNSS modules
 * 
 * Professional driver implementation extracted from main.txt solar monitoring system
 * Provides reliable GNSS functionality for multi-sensor embedded applications
 * 
 * Key Features:
 * - Robust initialization with timeout handling
 * - Position validation against configurable criteria
 * - Fix quality assessment and monitoring
 * - Power management and performance optimization
 * - Professional error handling and status reporting
 */

#include "ublox_driver.h"

// Constructor with default configuration
UBLOX_Driver::UBLOX_Driver() : 
    config(),
    validation(),
    is_initialized(false),
    init_start_time_ms(0),
    first_fix_time_ms(0),
    first_fix_achieved(false)
{
    // Initialize with default validation settings from gnss_types.h
    validation = GNSSValidationConfig();
}

// Constructor with custom configuration
UBLOX_Driver::UBLOX_Driver(const GNSSConfig& cfg) :
    config(cfg),
    validation(cfg.validation),
    is_initialized(false),
    init_start_time_ms(0),
    first_fix_time_ms(0),
    first_fix_achieved(false)
{
    // Use provided validation configuration
}

// Initialize GNSS hardware and set default configuration
// Extracted from: bool initGNSS() in main.txt
GNSSStatus UBLOX_Driver::initialize() {
    Serial.println(F("Initializing GNSS module..."));
    
    init_start_time_ms = millis();
    resetFixTiming();
    
    // Attempt to initialize the GNSS module
    if (!gnss.begin()) {
        Serial.println(F("GNSS not detected. Check wiring!"));
        is_initialized = false;
        return GNSSStatus::DEVICE_NOT_FOUND;
    }
    
    Serial.println(F("GNSS detected successfully"));
    
    // Test basic communication
    if (!checkCommunication(2000)) {
        Serial.println(F("GNSS communication test failed"));
        is_initialized = false;
        return GNSSStatus::COMMUNICATION_ERROR;
    }
    
    // Configure GNSS based on main.txt implementation
    Serial.println(F("Configuring GNSS module..."));
    
    try {
        // Set I2C output format to UBX (from main.txt)
        gnss.setI2COutput(COM_TYPE_UBX);
        
        // Enable automatic message types for better data collection (from main.txt)
        if (!enableAutomaticMessages()) {
            Serial.println(F("Warning: Could not enable all automatic messages"));
        }
        
        // Set measurement rate if different from default
        if (config.measurement_rate_ms != 1000) {
            if (!setMeasurementRate(config.measurement_rate_ms)) {
                Serial.println(F("Warning: Could not set measurement rate"));
            }
        }
        
        // Set navigation rate if different from default
        if (config.navigation_rate != 1) {
            if (!setNavigationRate(config.navigation_rate)) {
                Serial.println(F("Warning: Could not set navigation rate"));
            }
        }
        
        // Enable high precision mode if supported and requested
        if (config.enable_high_precision && supportsHighPrecision()) {
            if (!setHighPrecisionMode(true)) {
                Serial.println(F("Warning: Could not enable high precision mode"));
            }
        }
        
        // Set dynamic model to portable (suitable for most applications)
        if (!setDynamicModel(DYN_MODEL_PORTABLE)) {
            Serial.println(F("Warning: Could not set dynamic model"));
        }
        
    } catch (...) {
        Serial.println(F("GNSS configuration failed"));
        is_initialized = false;
        return GNSSStatus::CONFIGURATION_FAILED;
    }
    
    // Final communication test
    delay(100);  // Allow configuration to take effect
    if (!checkCommunication(1000)) {
        Serial.println(F("GNSS post-configuration communication failed"));
        is_initialized = false;
        return GNSSStatus::COMMUNICATION_ERROR;
    }
    
    is_initialized = true;
    Serial.println(F("GNSS initialized successfully"));
    
    return GNSSStatus::SUCCESS;
}

// Read current position and navigation data
// Extracted from: void readGNSSData(SensorData& data) in main.txt
bool UBLOX_Driver::readPosition(GNSSData& data) {
    if (!is_initialized) {
        return false;
    }
    
    // Clear previous data and set defaults (from main.txt logic)
    data = GNSSData();  // Reset to default state
    data.last_update_ms = millis();
    
    // Get fresh GNSS data (from main.txt)
    gnss.checkUblox();
    
    // Update all GNSS data fields
    if (!updateGNSSData(data)) {
        return false;
    }
    
    // Check if we have a valid GPS fix (from main.txt logic)
    data.gnss_fix_ok = gnss.getGnssFixOk();
    
    if (data.gnss_fix_ok && validateFix(data)) {
        data.is_fixed = true;
        data.validation_passed = true;
        data.position.position_valid = true;
        
        // Track first fix timing
        if (!first_fix_achieved) {
            first_fix_achieved = true;
            first_fix_time_ms = millis();
            data.time_to_first_fix_ms = first_fix_time_ms - init_start_time_ms;
            Serial.print(F("First GNSS fix achieved in "));
            Serial.print(data.time_to_first_fix_ms);
            Serial.println(F(" ms"));
        } else {
            data.time_to_first_fix_ms = first_fix_time_ms - init_start_time_ms;
        }
        
        // Convert from millionths of a degree to degrees (from main.txt)
        data.position.latitude_deg = gnss.getLatitude() / 10000000.0;
        data.position.longitude_deg = gnss.getLongitude() / 10000000.0;
        data.position.altitude_m = gnss.getAltitudeMSL() / 1000.0f;  // Convert mm to meters
        
        // Get accuracy estimates
        data.position.horizontal_accuracy_m = gnss.getHorizontalAccuracy() / 1000.0f;  // Convert mm to meters
        data.position.vertical_accuracy_m = gnss.getVerticalAccuracy() / 1000.0f;      // Convert mm to meters
        data.position.timestamp_ms = data.last_update_ms;
        
    } else {
        data.is_fixed = false;
        data.validation_passed = false;
        data.position.position_valid = false;
        
        // Still collect satellite count even without fix (from main.txt)
        data.satellites_used = gnss.getSIV();
        data.satellites_visible = gnss.getSIV();  // u-blox doesn't separate these easily
    }
    
    return true;
}

// Validate GNSS fix quality against configured criteria
// Extracted from: bool validateGnssData() in main.txt
bool UBLOX_Driver::validateFix(const GNSSData& data) {
    // Check for minimum satellite count requirement (from main.txt)
    if (data.satellites_used < validation.min_satellites) {
        return false;
    }
    
    // Check horizontal dilution of precision (HDOP) (from main.txt)
    // Lower values indicate better precision
    if (data.hdop > validation.max_hdop) {
        return false;
    }
    
    // Check data age
    uint32_t age_ms = millis() - data.last_update_ms;
    if (age_ms > validation.max_age_ms) {
        return false;
    }
    
    // Check if time is valid if required
    if (validation.require_time_valid && !data.time.time_valid) {
        return false;
    }
    
    // Check position bounds
    if (!validatePositionBounds(data.position.latitude_deg, data.position.longitude_deg, data.position.altitude_m)) {
        return false;
    }
    
    return true;
}

// Check if valid fix is currently available
bool UBLOX_Driver::hasValidFix() {
    if (!is_initialized) {
        return false;
    }
    
    gnss.checkUblox();
    
    if (!gnss.getGnssFixOk()) {
        return false;
    }
    
    // Quick validation check
    uint8_t satellites = gnss.getSIV();
    if (satellites < validation.min_satellites) {
        return false;
    }
    
    float hdop = convertDOP(gnss.getHorizontalDOP());
    if (hdop > validation.max_hdop) {
        return false;
    }
    
    return true;
}

// Update validation configuration
void UBLOX_Driver::updateValidationConfig(const GNSSValidationConfig& new_validation) {
    validation = new_validation;
}

// Get current validation configuration
const GNSSValidationConfig& UBLOX_Driver::getValidationConfig() const {
    return validation;
}

// Check if GNSS is properly initialized and communicating
bool UBLOX_Driver::isReady() const {
    return is_initialized;
}

// Get time since initialization started
uint32_t UBLOX_Driver::getInitializationTime() const {
    if (init_start_time_ms == 0) {
        return 0;
    }
    return millis() - init_start_time_ms;
}

// Get time to first fix
uint32_t UBLOX_Driver::getTimeToFirstFix() const {
    if (!first_fix_achieved) {
        return 0;
    }
    return first_fix_time_ms - init_start_time_ms;
}

// Enable or disable high precision mode
bool UBLOX_Driver::setHighPrecisionMode(bool enable) {
    if (!is_initialized) {
        return false;
    }
    
    return gnss.setHighPrecisionMode(enable);
}

// Set measurement rate for GNSS updates
bool UBLOX_Driver::setMeasurementRate(uint16_t rate_ms) {
    if (!is_initialized) {
        return false;
    }
    
    if (gnss.setMeasurementRate(rate_ms)) {
        config.measurement_rate_ms = rate_ms;
        return true;
    }
    return false;
}

// Set navigation rate
bool UBLOX_Driver::setNavigationRate(uint16_t nav_rate) {
    if (!is_initialized) {
        return false;
    }
    
    if (gnss.setNavigationRate(nav_rate)) {
        config.navigation_rate = nav_rate;
        return true;
    }
    return false;
}

// Set dynamic model
bool UBLOX_Driver::setDynamicModel(dynModel model) {
    if (!is_initialized) {
        return false;
    }
    
    return gnss.setDynamicModel(model);
}

// Print current GNSS status and position
void UBLOX_Driver::printStatus() const {
    if (!is_initialized) {
        Serial.println(F("GNSS Driver: NOT INITIALIZED"));
        return;
    }
    
    Serial.println(F("=== GNSS Driver Status ==="));
    Serial.print(F("Initialized: ")); Serial.println(is_initialized ? F("YES") : F("NO"));
    Serial.print(F("First Fix Achieved: ")); Serial.println(first_fix_achieved ? F("YES") : F("NO"));
    
    if (first_fix_achieved) {
        Serial.print(F("Time to First Fix: ")); Serial.print(getTimeToFirstFix()); Serial.println(F(" ms"));
    }
    
    Serial.print(F("Time Since Init: ")); Serial.print(getInitializationTime()); Serial.println(F(" ms"));
    
    // Current status
    uint8_t sats_used, sats_visible;
    float hdop;
    if (getSatelliteInfo(sats_used, sats_visible, hdop)) {
        Serial.print(F("Satellites Used: ")); Serial.println(sats_used);
        Serial.print(F("HDOP: ")); Serial.println(hdop, 2);
    }
    
    Serial.print(F("Fix Valid: ")); Serial.println(hasValidFix() ? F("YES") : F("NO"));
    Serial.println(F("========================"));
}

// Print position data in human-readable format
void UBLOX_Driver::printPosition(const GNSSData& data) const {
    Serial.println(F("=== GNSS Position Data ==="));
    Serial.print(F("Fix Valid: ")); Serial.println(data.is_fixed ? F("YES") : F("NO"));
    Serial.print(F("Validation Passed: ")); Serial.println(data.validation_passed ? F("YES") : F("NO"));
    
    if (data.position.position_valid) {
        Serial.print(F("Latitude: ")); Serial.println(data.position.latitude_deg, 7);
        Serial.print(F("Longitude: ")); Serial.println(data.position.longitude_deg, 7);
        Serial.print(F("Altitude: ")); Serial.print(data.position.altitude_m, 2); Serial.println(F(" m"));
        Serial.print(F("H.Accuracy: ")); Serial.print(data.position.horizontal_accuracy_m, 2); Serial.println(F(" m"));
        Serial.print(F("V.Accuracy: ")); Serial.print(data.position.vertical_accuracy_m, 2); Serial.println(F(" m"));
    }
    
    Serial.print(F("Satellites: ")); Serial.println(data.satellites_used);
    Serial.print(F("HDOP: ")); Serial.println(data.hdop, 2);
    Serial.print(F("Fix Type: ")); Serial.println((int)data.fix_type);
    Serial.println(F("========================="));
}

// Get satellite information summary
bool UBLOX_Driver::getSatelliteInfo(uint8_t& satellites_used, uint8_t& satellites_visible, float& hdop) {
    if (!is_initialized) {
        return false;
    }
    
    gnss.checkUblox();
    satellites_used = gnss.getSIV();
    satellites_visible = satellites_used;  // u-blox doesn't easily separate these
    hdop = convertDOP(gnss.getHorizontalDOP());
    
    return true;
}

// Get current fix type
GNSSFixType UBLOX_Driver::getCurrentFixType() {
    if (!is_initialized) {
        return GNSSFixType::NO_FIX;
    }
    
    gnss.checkUblox();
    uint8_t fix_type = gnss.getFixType();
    
    switch (fix_type) {
        case 0: return GNSSFixType::NO_FIX;
        case 1: return GNSSFixType::DEAD_RECKONING;
        case 2: return GNSSFixType::FIX_2D;
        case 3: return GNSSFixType::FIX_3D;
        case 4: return GNSSFixType::GNSS_DEAD_RECKONING;
        case 5: return GNSSFixType::TIME_ONLY;
        default: return GNSSFixType::NO_FIX;
    }
}

// Enable power save mode
bool UBLOX_Driver::setPowerSaveMode(bool enable) {
    if (!is_initialized) {
        return false;
    }
    
    return gnss.powerSaveMode(enable);
}

// Factory reset
bool UBLOX_Driver::factoryReset() {
    if (!is_initialized) {
        return false;
    }
    
    gnss.factoryReset();
    delay(2000);  // Allow time for reset
    
    // Re-initialize after reset
    resetFixTiming();
    return (initialize() == GNSSStatus::SUCCESS);
}

// Save configuration
bool UBLOX_Driver::saveConfiguration() {
    if (!is_initialized) {
        return false;
    }
    
    return gnss.saveConfiguration();
}

// Get underlying driver
SFE_UBLOX_GNSS& UBLOX_Driver::getRawDriver() {
    return gnss;
}

// Get driver configuration
const GNSSConfig& UBLOX_Driver::getConfig() const {
    return config;
}

// Update driver configuration
void UBLOX_Driver::updateConfig(const GNSSConfig& new_config) {
    config = new_config;
    validation = new_config.validation;
}

// Check communication
bool UBLOX_Driver::checkCommunication(uint16_t timeout_ms) {
    return gnss.isConnected(timeout_ms);
}

// Get accuracy estimates
bool UBLOX_Driver::getAccuracyEstimates(float& horizontal_acc_m, float& vertical_acc_m) {
    if (!is_initialized) {
        return false;
    }
    
    gnss.checkUblox();
    horizontal_acc_m = gnss.getHorizontalAccuracy() / 1000.0f;  // Convert mm to meters
    vertical_acc_m = gnss.getVerticalAccuracy() / 1000.0f;      // Convert mm to meters
    
    return true;
}

// Get GNSS time
bool UBLOX_Driver::getGNSSTime(GNSSTimeData& time_data) {
    if (!is_initialized) {
        return false;
    }
    
    gnss.checkUblox();
    
    time_data.year = gnss.getYear();
    time_data.month = gnss.getMonth();
    time_data.day = gnss.getDay();
    time_data.hour = gnss.getHour();
    time_data.minute = gnss.getMinute();
    time_data.second = gnss.getSecond();
    time_data.nanoseconds = gnss.getNanosecond();
    
    time_data.time_valid = gnss.getTimeValid();
    time_data.time_confirmed = gnss.getConfirmedTime();
    
    return time_data.time_valid;
}

// Check if high precision is supported
bool UBLOX_Driver::supportsHighPrecision() const {
    // This would need to be determined based on module type or protocol version
    // For now, assume most modern u-blox modules support it
    return true;
}

// Get protocol version
bool UBLOX_Driver::getProtocolVersion(uint8_t& version_high, uint8_t& version_low) {
    if (!is_initialized) {
        return false;
    }
    
    version_high = gnss.getProtocolVersionHigh();
    version_low = gnss.getProtocolVersionLow();
    
    return (version_high != 255 && version_low != 255);
}

// Enable automatic messages (from main.txt)
bool UBLOX_Driver::enableAutomaticMessages() {
    if (!is_initialized) {
        return false;
    }
    
    bool success = true;
    
    // Enable additional GNSS data for validation (from main.txt)
    success &= gnss.setAutoNAVPVAT(true);      // Enable automatic NAVPVT messages
    success &= gnss.setAutoPVT(true);          // Enable navigation position velocity time solution
    success &= gnss.setAutoHPPOSLLH(true);     // Enable high precision geodetic position
    
    return success;
}

// Perform self-test
bool UBLOX_Driver::performSelfTest() {
    if (!is_initialized) {
        return false;
    }
    
    // Basic communication test
    if (!checkCommunication(2000)) {
        return false;
    }
    
    // Check if we can read basic data
    gnss.checkUblox();
    uint8_t satellites = gnss.getSIV();
    uint16_t hdop_raw = gnss.getHorizontalDOP();
    
    // Basic sanity checks
    if (satellites > 50 || hdop_raw == 0xFFFF) {  // Invalid readings
        return false;
    }
    
    return true;
}

// Private helper methods

// Check if current fix is valid
bool UBLOX_Driver::isFixValid() const {
    if (!is_initialized) {
        return false;
    }
    
    return gnss.getGnssFixOk();
}

// Update GNSS data structure
bool UBLOX_Driver::updateGNSSData(GNSSData& data) {
    if (!is_initialized) {
        return false;
    }
    
    gnss.checkUblox();
    
    // Fix information
    data.fix_type = getCurrentFixType();
    data.satellites_used = gnss.getSIV();
    data.satellites_visible = data.satellites_used;  // u-blox limitation
    data.hdop = convertDOP(gnss.getHorizontalDOP());
    data.pdop = convertDOP(gnss.getPDOP());
    
    // Quality flags
    data.gnss_fix_ok = gnss.getGnssFixOk();
    
    // Get time data
    getGNSSTime(data.time);
    
    return true;
}

// Convert DOP from raw format
float UBLOX_Driver::convertDOP(uint16_t dop_raw) const {
    // u-blox DOP is reported in units of 0.01, so divide by 100
    return dop_raw / 100.0f;
}

// Validate position bounds
bool UBLOX_Driver::validatePositionBounds(double lat, double lon, float alt) const {
    using namespace GNSSConstants;
    
    if (lat < MIN_VALID_LATITUDE || lat > MAX_VALID_LATITUDE) {
        return false;
    }
    
    if (lon < MIN_VALID_LONGITUDE || lon > MAX_VALID_LONGITUDE) {
        return false;
    }
    
    if (alt > MAX_VALID_ALTITUDE || alt < -1000.0f) {  // Below 1km below sea level seems unreasonable
        return false;
    }
    
    return true;
}

// Reset fix timing
void UBLOX_Driver::resetFixTiming() {
    first_fix_achieved = false;
    first_fix_time_ms = 0;
} 