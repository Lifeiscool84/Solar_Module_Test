/*
 * multi_sensor_template.cpp
 * 
 * Template for integrating multiple sensors in the Croc Tracker project
 * 
 * This template demonstrates best practices for:
 * - Multi-sensor initialization and management
 * - Synchronized data collection
 * - Error handling and recovery
 * - Data logging and CSV export
 * - Power management integration
 * 
 * Usage:
 * 1. Copy this template to your firmware directory
 * 2. Replace [SENSOR_X] placeholders with actual sensor names
 * 3. Update I2C addresses and sensor configurations
 * 4. Customize data structures for your specific sensors
 * 5. Implement sensor-specific measurement functions
 * 
 * Author: [Your Name]
 * Date: [Date]
 * Version: 1.0
 */

#include <Wire.h>
#include <SPI.h>
#include <SdFat.h>

// Include sensor driver headers
#include "hardware/sensors/ina228/ina228_driver.h"
// #include "hardware/sensors/[sensor_2]/[sensor_2]_driver.h"
// #include "hardware/sensors/[sensor_3]/[sensor_3]_driver.h"

// Include utility headers
#include "libraries/utilities/csv_logger.h"
#include "hardware/storage/sd_power_manager.h"

// ==================== CONFIGURATION SECTION ====================

// Debug Configuration
const bool ENABLE_SERIAL_DEBUG = true;  // Set to false for production

// Hardware Configuration
const uint8_t SD_CS_PIN = 8;

// Sensor I2C Addresses
const uint8_t [SENSOR_1]_I2C_ADDRESS = 0x40;  // Replace with actual address
const uint8_t [SENSOR_2]_I2C_ADDRESS = 0x44;  // Replace with actual address
const uint8_t [SENSOR_3]_I2C_ADDRESS = 0x41;  // Replace with actual address

// Test Configuration
const uint32_t MEASUREMENT_INTERVAL_MS = 1000;  // 1 Hz sampling
const uint32_t TEST_DURATION_S = 300;           // 5 minutes
const int RAM_BUFFER_SIZE = 60;                 // Buffer 60 samples
const char CSV_FILENAME[] = "multi_sensor_data.csv";

// ==================== DATA STRUCTURES ====================

// Multi-sensor data structure
struct MultiSensorData {
    uint32_t timestamp_ms;
    
    // Sensor 1 data (e.g., Solar monitoring)
    float sensor1_voltage_V;
    float sensor1_current_mA;
    float sensor1_power_mW;
    
    // Sensor 2 data (e.g., Battery monitoring)
    float sensor2_voltage_V;
    float sensor2_current_mA;
    float sensor2_power_mW;
    
    // Sensor 3 data (e.g., Load monitoring)
    float sensor3_voltage_V;
    float sensor3_current_mA;
    float sensor3_power_mW;
    
    // Calculated values
    float power_balance_mW;        // Solar - Battery - Load
    float efficiency_percent;      // (Load / Solar) * 100
    
    // Data quality flags
    bool sensor1_valid;
    bool sensor2_valid;
    bool sensor3_valid;
    bool all_sensors_valid;
};

// System state enumeration
enum SystemState {
    SYSTEM_INIT = 0,
    SYSTEM_IDLE,
    SYSTEM_MEASURING,
    SYSTEM_ERROR,
    SYSTEM_SHUTDOWN
};

// ==================== GLOBAL VARIABLES ====================

SdFat SD;
MultiSensorData dataBuffer[RAM_BUFFER_SIZE];
int bufferIndex = 0;
uint32_t testRunID = 0;
SystemState currentState = SYSTEM_INIT;
uint32_t lastMeasurementTime = 0;
uint32_t testStartTime = 0;

// Sensor status tracking
bool sensor1_initialized = false;
bool sensor2_initialized = false;
bool sensor3_initialized = false;

// ==================== SENSOR MANAGEMENT FUNCTIONS ====================

bool initializeAllSensors() {
    if (ENABLE_SERIAL_DEBUG) {
        Serial.println(F("=== MULTI-SENSOR INITIALIZATION ==="));
    }
    
    // Initialize I2C bus
    Wire.begin();
    delay(100);  // Allow I2C bus to stabilize
    
    // Initialize Sensor 1
    if (ENABLE_SERIAL_DEBUG) Serial.print(F("Initializing Sensor 1 ([SENSOR_1])... "));
    sensor1_initialized = [sensor_1]_init([SENSOR_1]_I2C_ADDRESS, "Sensor1");
    if (ENABLE_SERIAL_DEBUG) {
        Serial.println(sensor1_initialized ? F("Success!") : F("Failed!"));
    }
    
    // Initialize Sensor 2
    if (ENABLE_SERIAL_DEBUG) Serial.print(F("Initializing Sensor 2 ([SENSOR_2])... "));
    sensor2_initialized = [sensor_2]_init([SENSOR_2]_I2C_ADDRESS, "Sensor2");
    if (ENABLE_SERIAL_DEBUG) {
        Serial.println(sensor2_initialized ? F("Success!") : F("Failed!"));
    }
    
    // Initialize Sensor 3
    if (ENABLE_SERIAL_DEBUG) Serial.print(F("Initializing Sensor 3 ([SENSOR_3])... "));
    sensor3_initialized = [sensor_3]_init([SENSOR_3]_I2C_ADDRESS, "Sensor3");
    if (ENABLE_SERIAL_DEBUG) {
        Serial.println(sensor3_initialized ? F("Success!") : F("Failed!"));
    }
    
    // Check overall initialization status
    bool allSensorsOK = sensor1_initialized && sensor2_initialized && sensor3_initialized;
    
    if (ENABLE_SERIAL_DEBUG) {
        Serial.print(F("Multi-sensor initialization: "));
        Serial.println(allSensorsOK ? F("SUCCESS") : F("PARTIAL/FAILED"));
        Serial.print(F("Active sensors: "));
        Serial.print(sensor1_initialized + sensor2_initialized + sensor3_initialized);
        Serial.println(F("/3"));
    }
    
    return allSensorsOK;
}

bool validateSensorCommunication() {
    if (ENABLE_SERIAL_DEBUG) {
        Serial.println(F("=== SENSOR COMMUNICATION VALIDATION ==="));
    }
    
    bool allValid = true;
    
    // Validate Sensor 1
    if (sensor1_initialized) {
        bool valid = [sensor_1]_validateDevice([SENSOR_1]_I2C_ADDRESS);
        if (ENABLE_SERIAL_DEBUG) {
            Serial.print(F("Sensor 1 communication: "));
            Serial.println(valid ? F("OK") : F("FAILED"));
        }
        allValid &= valid;
    }
    
    // Validate Sensor 2
    if (sensor2_initialized) {
        bool valid = [sensor_2]_validateDevice([SENSOR_2]_I2C_ADDRESS);
        if (ENABLE_SERIAL_DEBUG) {
            Serial.print(F("Sensor 2 communication: "));
            Serial.println(valid ? F("OK") : F("FAILED"));
        }
        allValid &= valid;
    }
    
    // Validate Sensor 3
    if (sensor3_initialized) {
        bool valid = [sensor_3]_validateDevice([SENSOR_3]_I2C_ADDRESS);
        if (ENABLE_SERIAL_DEBUG) {
            Serial.print(F("Sensor 3 communication: "));
            Serial.println(valid ? F("OK") : F("FAILED"));
        }
        allValid &= valid;
    }
    
    return allValid;
}

// ==================== DATA COLLECTION FUNCTIONS ====================

MultiSensorData takeSynchronizedMeasurement() {
    MultiSensorData data = {0};  // Initialize all fields to zero
    data.timestamp_ms = millis();
    
    // Read Sensor 1 (if available)
    if (sensor1_initialized) {
        data.sensor1_voltage_V = [sensor_1]_readVoltage([SENSOR_1]_I2C_ADDRESS);
        data.sensor1_current_mA = [sensor_1]_readCurrent([SENSOR_1]_I2C_ADDRESS);
        data.sensor1_power_mW = [sensor_1]_readPower([SENSOR_1]_I2C_ADDRESS);
        data.sensor1_valid = (data.sensor1_voltage_V > 0.1);  // Basic validity check
    }
    
    // Read Sensor 2 (if available)
    if (sensor2_initialized) {
        data.sensor2_voltage_V = [sensor_2]_readVoltage([SENSOR_2]_I2C_ADDRESS);
        data.sensor2_current_mA = [sensor_2]_readCurrent([SENSOR_2]_I2C_ADDRESS);
        data.sensor2_power_mW = [sensor_2]_readPower([SENSOR_2]_I2C_ADDRESS);
        data.sensor2_valid = (data.sensor2_voltage_V > 0.1);  // Basic validity check
    }
    
    // Read Sensor 3 (if available)
    if (sensor3_initialized) {
        data.sensor3_voltage_V = [sensor_3]_readVoltage([SENSOR_3]_I2C_ADDRESS);
        data.sensor3_current_mA = [sensor_3]_readCurrent([SENSOR_3]_I2C_ADDRESS);
        data.sensor3_power_mW = [sensor_3]_readPower([SENSOR_3]_I2C_ADDRESS);
        data.sensor3_valid = (data.sensor3_voltage_V > 0.1);  // Basic validity check
    }
    
    // Calculate derived values
    data.power_balance_mW = data.sensor1_power_mW - data.sensor2_power_mW - data.sensor3_power_mW;
    
    if (data.sensor1_power_mW > 0.1) {
        data.efficiency_percent = (data.sensor3_power_mW / data.sensor1_power_mW) * 100.0;
    } else {
        data.efficiency_percent = 0.0;
    }
    
    // Set overall validity flag
    data.all_sensors_valid = data.sensor1_valid && data.sensor2_valid && data.sensor3_valid;
    
    return data;
}

void addDataToBuffer(const MultiSensorData& data) {
    if (bufferIndex < RAM_BUFFER_SIZE) {
        dataBuffer[bufferIndex] = data;
        bufferIndex++;
        
        if (ENABLE_SERIAL_DEBUG) {
            Serial.print(F("Data buffered ("));
            Serial.print(bufferIndex);
            Serial.print(F("/"));
            Serial.print(RAM_BUFFER_SIZE);
            Serial.println(F(")"));
        }
    } else {
        if (ENABLE_SERIAL_DEBUG) {
            Serial.println(F("WARNING: Buffer full! Consider flushing to SD card."));
        }
    }
}

// ==================== CSV LOGGING FUNCTIONS ====================

bool initializeCSVLogging() {
    if (ENABLE_SERIAL_DEBUG) Serial.print(F("Initializing CSV logging... "));
    
    bool fileExists = SD.exists(CSV_FILENAME);
    
    File32 logFile = SD.open(CSV_FILENAME, O_WRONLY | O_CREAT | O_APPEND);
    if (!logFile) {
        if (ENABLE_SERIAL_DEBUG) Serial.println(F("Failed!"));
        return false;
    }
    
    // Write CSV header if file is new
    if (!fileExists) {
        logFile.println(F("TestRunID,Timestamp_ms,S1_Voltage_V,S1_Current_mA,S1_Power_mW,S1_Valid,S2_Voltage_V,S2_Current_mA,S2_Power_mW,S2_Valid,S3_Voltage_V,S3_Current_mA,S3_Power_mW,S3_Valid,Power_Balance_mW,Efficiency_Percent,All_Valid"));
        if (ENABLE_SERIAL_DEBUG) Serial.print(F("Header written... "));
    }
    
    logFile.close();
    if (ENABLE_SERIAL_DEBUG) Serial.println(F("Success!"));
    return true;
}

bool flushBufferToCSV() {
    if (bufferIndex == 0) return true;  // Nothing to flush
    
    if (ENABLE_SERIAL_DEBUG) {
        Serial.print(F("Flushing "));
        Serial.print(bufferIndex);
        Serial.print(F(" entries to CSV... "));
    }
    
    File32 logFile = SD.open(CSV_FILENAME, O_WRONLY | O_APPEND);
    if (!logFile) {
        if (ENABLE_SERIAL_DEBUG) Serial.println(F("Failed to open file!"));
        return false;
    }
    
    // Write all buffered data
    for (int i = 0; i < bufferIndex; i++) {
        const MultiSensorData& data = dataBuffer[i];
        
        // Write CSV row
        logFile.print(testRunID); logFile.print(",");
        logFile.print(data.timestamp_ms); logFile.print(",");
        logFile.print(data.sensor1_voltage_V, 4); logFile.print(",");
        logFile.print(data.sensor1_current_mA, 4); logFile.print(",");
        logFile.print(data.sensor1_power_mW, 4); logFile.print(",");
        logFile.print(data.sensor1_valid ? 1 : 0); logFile.print(",");
        logFile.print(data.sensor2_voltage_V, 4); logFile.print(",");
        logFile.print(data.sensor2_current_mA, 4); logFile.print(",");
        logFile.print(data.sensor2_power_mW, 4); logFile.print(",");
        logFile.print(data.sensor2_valid ? 1 : 0); logFile.print(",");
        logFile.print(data.sensor3_voltage_V, 4); logFile.print(",");
        logFile.print(data.sensor3_current_mA, 4); logFile.print(",");
        logFile.print(data.sensor3_power_mW, 4); logFile.print(",");
        logFile.print(data.sensor3_valid ? 1 : 0); logFile.print(",");
        logFile.print(data.power_balance_mW, 4); logFile.print(",");
        logFile.print(data.efficiency_percent, 2); logFile.print(",");
        logFile.print(data.all_sensors_valid ? 1 : 0);
        logFile.println();
    }
    
    logFile.close();
    
    // Reset buffer
    bufferIndex = 0;
    
    if (ENABLE_SERIAL_DEBUG) Serial.println(F("Success!"));
    return true;
}

// ==================== MAIN SYSTEM FUNCTIONS ====================

void setup() {
    if (ENABLE_SERIAL_DEBUG) {
        Serial.begin(115200);
        delay(2000);
        
        Serial.println(F("\n============================================"));
        Serial.println(F("    CROC TRACKER MULTI-SENSOR MONITOR"));
        Serial.println(F("============================================"));
        Serial.println();
    }
    
    testRunID = millis();
    testStartTime = millis();
    
    if (ENABLE_SERIAL_DEBUG) {
        Serial.print(F("Test Run ID: "));
        Serial.println(testRunID);
        Serial.print(F("Measurement Interval: "));
        Serial.print(MEASUREMENT_INTERVAL_MS);
        Serial.println(F(" ms"));
        Serial.print(F("Test Duration: "));
        Serial.print(TEST_DURATION_S);
        Serial.println(F(" seconds"));
        Serial.println();
    }
    
    // Initialize sensors
    if (!initializeAllSensors()) {
        if (ENABLE_SERIAL_DEBUG) {
            Serial.println(F("WARNING: Not all sensors initialized successfully!"));
            Serial.println(F("Continuing with available sensors..."));
        }
    }
    
    // Validate sensor communication
    if (!validateSensorCommunication()) {
        if (ENABLE_SERIAL_DEBUG) {
            Serial.println(F("WARNING: Sensor communication issues detected!"));
        }
    }
    
    // Initialize SD card
    if (ENABLE_SERIAL_DEBUG) Serial.print(F("Initializing SD card... "));
    if (!SD.begin(SD_CS_PIN)) {
        if (ENABLE_SERIAL_DEBUG) Serial.println(F("Failed!"));
        currentState = SYSTEM_ERROR;
        return;
    }
    if (ENABLE_SERIAL_DEBUG) Serial.println(F("Success!"));
    
    // Initialize CSV logging
    if (!initializeCSVLogging()) {
        if (ENABLE_SERIAL_DEBUG) Serial.println(F("Failed to initialize CSV logging!"));
        currentState = SYSTEM_ERROR;
        return;
    }
    
    // Start measurements
    currentState = SYSTEM_MEASURING;
    lastMeasurementTime = millis();
    
    if (ENABLE_SERIAL_DEBUG) {
        Serial.println(F("=== STARTING MULTI-SENSOR MONITORING ==="));
        Serial.println();
    }
}

void loop() {
    uint32_t currentTime = millis();
    
    // Check if test duration exceeded
    if ((currentTime - testStartTime) > (TEST_DURATION_S * 1000)) {
        if (currentState != SYSTEM_SHUTDOWN) {
            if (ENABLE_SERIAL_DEBUG) {
                Serial.println(F("=== TEST COMPLETED ==="));
                Serial.println(F("Flushing remaining data and shutting down..."));
            }
            
            flushBufferToCSV();
            currentState = SYSTEM_SHUTDOWN;
            
            if (ENABLE_SERIAL_DEBUG) {
                Serial.print(F("Test Run ID: "));
                Serial.println(testRunID);
                Serial.println(F("Data saved to: "));
                Serial.println(CSV_FILENAME);
                Serial.println(F("System halted. Reset to restart."));
            }
        }
        
        // Halt system
        delay(1000);
        return;
    }
    
    // Handle state machine
    switch (currentState) {
        case SYSTEM_MEASURING:
            // Check if it's time for next measurement
            if ((currentTime - lastMeasurementTime) >= MEASUREMENT_INTERVAL_MS) {
                // Take synchronized measurement
                MultiSensorData data = takeSynchronizedMeasurement();
                addDataToBuffer(data);
                
                if (ENABLE_SERIAL_DEBUG) {
                    Serial.print(F("["));
                    Serial.print((currentTime - testStartTime) / 1000);
                    Serial.print(F("s] S1:"));
                    Serial.print(data.sensor1_power_mW, 1);
                    Serial.print(F("mW S2:"));
                    Serial.print(data.sensor2_power_mW, 1);
                    Serial.print(F("mW S3:"));
                    Serial.print(data.sensor3_power_mW, 1);
                    Serial.print(F("mW Bal:"));
                    Serial.print(data.power_balance_mW, 1);
                    Serial.print(F("mW Eff:"));
                    Serial.print(data.efficiency_percent, 1);
                    Serial.println(F("%"));
                }
                
                lastMeasurementTime = currentTime;
                
                // Flush buffer when nearly full
                if (bufferIndex >= (RAM_BUFFER_SIZE - 5)) {
                    flushBufferToCSV();
                }
            }
            break;
            
        case SYSTEM_ERROR:
            if (ENABLE_SERIAL_DEBUG) {
                Serial.println(F("SYSTEM ERROR: Check hardware and reset."));
            }
            delay(5000);
            break;
            
        case SYSTEM_SHUTDOWN:
            // System halted - do nothing
            delay(1000);
            break;
            
        default:
            currentState = SYSTEM_ERROR;
            break;
    }
    
    delay(10);  // Small delay to prevent excessive loop iterations
}