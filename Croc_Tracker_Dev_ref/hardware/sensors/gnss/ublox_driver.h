#ifndef UBLOX_DRIVER_H
#define UBLOX_DRIVER_H

#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include "gnss_types.h"

/**
 * @file ublox_driver.h
 * @brief Hardware abstraction layer for u-blox GNSS modules
 * 
 * Extracted and modularized from main.txt solar monitoring system
 * Provides clean interface for GNSS operations across multi-sensor applications
 * 
 * Key Features:
 * - Hardware initialization and configuration
 * - Position reading with validation
 * - Fix quality assessment and error handling
 * - Integration with time synchronization systems
 * - Power management and performance optimization
 */

class UBLOX_Driver {
private:
    SFE_UBLOX_GNSS gnss;            // SparkFun u-blox GNSS library instance
    GNSSConfig config;              // Configuration settings
    GNSSValidationConfig validation; // Validation criteria
    bool is_initialized;            // Initialization status
    uint32_t init_start_time_ms;    // Time when initialization started
    uint32_t first_fix_time_ms;     // Time when first fix was achieved
    bool first_fix_achieved;        // Whether first fix has been achieved
    
    // Private helper methods
    bool isFixValid() const;
    bool updateGNSSData(GNSSData& data);
    float convertDOP(uint16_t dop_raw) const;
    bool validatePositionBounds(double lat, double lon, float alt) const;
    void resetFixTiming();
    
public:
    /**
     * @brief Constructor with default configuration
     */
    UBLOX_Driver();
    
    /**
     * @brief Constructor with custom configuration
     * @param cfg Custom GNSS configuration
     */
    explicit UBLOX_Driver(const GNSSConfig& cfg);
    
    /**
     * @brief Initialize GNSS hardware and set default configuration
     * @return GNSSStatus indicating success or specific error
     * 
     * Extracted from: bool initGNSS() in main.txt
     */
    GNSSStatus initialize();
    
    /**
     * @brief Read current position and navigation data
     * @param data Reference to GNSSData structure to fill
     * @return true if successful, false if GNSS communication failed
     * 
     * Extracted from: void readGNSSData(SensorData& data) in main.txt
     */
    bool readPosition(GNSSData& data);
    
    /**
     * @brief Validate GNSS fix quality against configured criteria
     * @param data GNSS data to validate
     * @return true if fix meets quality standards
     * 
     * Extracted from: bool validateGnssData() in main.txt
     */
    bool validateFix(const GNSSData& data);
    
    /**
     * @brief Update validation configuration
     * @param new_validation New validation criteria
     */
    void updateValidationConfig(const GNSSValidationConfig& new_validation);
    
    /**
     * @brief Get current validation configuration
     * @return Current GNSSValidationConfig
     */
    const GNSSValidationConfig& getValidationConfig() const;
    
    /**
     * @brief Check if GNSS is properly initialized and communicating
     * @return true if GNSS is ready for use
     */
    bool isReady() const;
    
    /**
     * @brief Check if valid fix is currently available
     * @return true if fix is valid according to validation criteria
     */
    bool hasValidFix();
    
    /**
     * @brief Get time since initialization started (useful for timeout detection)
     * @return milliseconds since initialize() was called
     */
    uint32_t getInitializationTime() const;
    
    /**
     * @brief Get time to first fix (0 if no fix achieved yet)
     * @return milliseconds from initialization to first valid fix
     */
    uint32_t getTimeToFirstFix() const;
    
    /**
     * @brief Enable or disable high precision mode if supported
     * @param enable true to enable high precision mode
     * @return true if successful
     */
    bool setHighPrecisionMode(bool enable);
    
    /**
     * @brief Set measurement rate for GNSS updates
     * @param rate_ms Measurement rate in milliseconds (default: 1000 = 1Hz)
     * @return true if successful
     */
    bool setMeasurementRate(uint16_t rate_ms);
    
    /**
     * @brief Set navigation rate (ratio of measurements to navigation solutions)
     * @param nav_rate Navigation rate (default: 1 = every measurement)
     * @return true if successful
     */
    bool setNavigationRate(uint16_t nav_rate);
    
    /**
     * @brief Set dynamic model for different use cases
     * @param model Dynamic model (portable, automotive, etc.)
     * @return true if successful
     */
    bool setDynamicModel(dynModel model = DYN_MODEL_PORTABLE);
    
    /**
     * @brief Print current GNSS status and position to Serial
     * 
     * Useful for debugging and monitoring
     */
    void printStatus() const;
    
    /**
     * @brief Print last position data in human-readable format
     * @param data GNSS data to print
     */
    void printPosition(const GNSSData& data) const;
    
    /**
     * @brief Get satellite information summary
     * @param satellites_used Reference to store satellites used count
     * @param satellites_visible Reference to store satellites visible count
     * @param hdop Reference to store HDOP value
     * @return true if data is available
     */
    bool getSatelliteInfo(uint8_t& satellites_used, uint8_t& satellites_visible, float& hdop);
    
    /**
     * @brief Get current fix type
     * @return GNSSFixType indicating current fix quality
     */
    GNSSFixType getCurrentFixType();
    
    /**
     * @brief Enable power save mode for battery operation
     * @param enable true to enable power save mode
     * @return true if successful
     */
    bool setPowerSaveMode(bool enable);
    
    /**
     * @brief Force a factory reset of the GNSS module
     * @return true if successful
     */
    bool factoryReset();
    
    /**
     * @brief Save current configuration to module flash memory
     * @return true if successful
     */
    bool saveConfiguration();
    
    /**
     * @brief Get underlying SparkFun library instance for advanced operations
     * @return Reference to SFE_UBLOX_GNSS object
     */
    SFE_UBLOX_GNSS& getRawDriver();
    
    /**
     * @brief Get driver configuration
     * @return Current GNSSConfig
     */
    const GNSSConfig& getConfig() const;
    
    /**
     * @brief Update driver configuration
     * @param new_config New configuration to apply
     */
    void updateConfig(const GNSSConfig& new_config);
    
    /**
     * @brief Check for communication with GNSS module
     * @param timeout_ms Maximum time to wait for response
     * @return true if module responds
     */
    bool checkCommunication(uint16_t timeout_ms = 1000);
    
    /**
     * @brief Get current position accuracy estimates
     * @param horizontal_acc_m Reference to store horizontal accuracy (meters)
     * @param vertical_acc_m Reference to store vertical accuracy (meters)
     * @return true if accuracy data is available
     */
    bool getAccuracyEstimates(float& horizontal_acc_m, float& vertical_acc_m);
    
    /**
     * @brief Get current time from GNSS (UTC)
     * @param time_data Reference to GNSSTimeData structure to fill
     * @return true if time data is valid
     */
    bool getGNSSTime(GNSSTimeData& time_data);
    
    /**
     * @brief Check if module supports high precision mode
     * @return true if high precision is available
     */
    bool supportsHighPrecision() const;
    
    /**
     * @brief Get module protocol version
     * @param version_high Reference to store high byte
     * @param version_low Reference to store low byte
     * @return true if successful
     */
    bool getProtocolVersion(uint8_t& version_high, uint8_t& version_low);
    
    /**
     * @brief Enable automatic message types for better data collection
     * @return true if successful
     */
    bool enableAutomaticMessages();
    
    /**
     * @brief Perform a module self-test
     * @return true if all tests pass
     */
    bool performSelfTest();
};

#endif // UBLOX_DRIVER_H 