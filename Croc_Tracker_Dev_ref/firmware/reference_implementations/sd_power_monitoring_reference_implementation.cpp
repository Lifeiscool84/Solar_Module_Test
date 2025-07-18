/*
 * sd_power_test_essentials.cpp
 * 
 * Simplified SD card power consumption test using direct Wire communication
 * to avoid Apollo3 library compatibility issues.
 */

#include <Wire.h>
#include <SPI.h>
#include <SdFat.h>

// Debug Configuration - Set to false for battery operation
const bool ENABLE_SERIAL_DEBUG = true;  // Change to false for standalone operation

// Hardware Configuration
const uint8_t SD_CS_PIN = 8;
// Three INA228 sensors (addresses from user's main.txt)
const uint8_t INA228_SOLAR_I2C_ADDRESS = 0x40;   // Solar panel monitoring
const uint8_t INA228_BATTERY_I2C_ADDRESS = 0x44; // Battery monitoring (primary)
const uint8_t INA228_LOAD_I2C_ADDRESS = 0x41;    // Load monitoring (after 3.3V regulator)

// INA228 Register Addresses (8-bit)
const uint8_t INA228_REG_CONFIG = 0x00;
const uint8_t INA228_REG_VBUS = 0x05;
const uint8_t INA228_REG_CURRENT = 0x07;
const uint8_t INA228_REG_POWER = 0x08;
const uint8_t INA228_REG_ENERGY = 0x09;     // Energy accumulation register
const uint8_t INA228_REG_CHARGE = 0x0A;     // Charge accumulation register
const uint8_t INA228_REG_DIAGALRT = 0x0B;   // Diagnostic and alert register
const uint8_t INA228_REG_SHUNTCAL = 0x02;
const uint8_t INA228_REG_DEVICEID = 0x3F;

// Test Configuration
const uint32_t TEST_STATE_DURATION_S = 60;
const uint32_t SENSOR_SAMPLING_INTERVAL_MS = 1000;
const int RAM_BUFFER_SIZE = 60;
const char CSV_FILENAME[] = "test.csv";

// Current sensing parameters
const float RSHUNT_OHMS = 0.0177186;        // Effective shunt resistance 
const float MAX_CURRENT_A = 5.0;            // Maximum expected current (corrected from 20.0)
const float CURRENT_LSB = MAX_CURRENT_A / 524288.0;  // 19-bit signed, so 2^19 = 524288

// Data Structure (Updated for three sensors based on user's main.txt)
struct PowerLogEntry {
    uint32_t timestamp_ms;
    // Solar sensor (0x40) measurements
    float solar_voltage_V;
    float solar_current_mA;
    float solar_power_hw_mW;       // Hardware power register reading
    float solar_power_calc_mW;     // Manual V×I calculation
    // Battery sensor (0x44) measurements
    float battery_voltage_V;
    float battery_current_mA;
    float battery_power_hw_mW;     // Hardware power register reading
    float battery_power_calc_mW;   // Manual V×I calculation
    // Load sensor (0x41) measurements  
    float load_voltage_V;
    float load_current_mA;
    float load_power_hw_mW;        // Hardware power register reading
    float load_power_calc_mW;      // Manual V×I calculation
};

enum TestState {
    TEST_MCU_ACTIVE_SD_DEINITIALIZED = 1,
    TEST_MCU_ACTIVE_SD_IDLE_STANDBY = 2,
    TEST_SUSTAINED_SD_WRITE = 3,
    TEST_PERIODIC_BATCH_WRITE_CYCLE = 4
};

// Global Variables
SdFat SD;
PowerLogEntry powerDataBuffer[RAM_BUFFER_SIZE];
int bufferIndex = 0;
uint32_t testRunID = 0;
uint32_t testStateStartTime_ms = 0;
TestState currentTestState;

// Forward Declarations
bool ina228_writeRegister16(uint8_t address, uint8_t reg, uint16_t value);
uint16_t ina228_readRegister16(uint8_t address, uint8_t reg);
uint32_t ina228_readRegister24(uint8_t address, uint8_t reg);
bool initializeINA228(uint8_t address, const char* sensorName);
float ina228_readBusVoltage(uint8_t address);
float ina228_readCurrent(uint8_t address);
float ina228_readPower(uint8_t address);
bool initializeCSVLogging();
const char* getTestStateName(TestState state);
bool appendPowerDataToCSV();
void logSingleEntryToCSV(const PowerLogEntry& entry, const char* testStateName);
void takePowerMeasurement();
bool initializeTestEnvironment();

// I2C Communication Functions
bool ina228_writeRegister16(uint8_t address, uint8_t reg, uint16_t value) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write((value >> 8) & 0xFF);  // MSB first
    Wire.write(value & 0xFF);         // LSB second
    return (Wire.endTransmission() == 0);
}

uint16_t ina228_readRegister16(uint8_t address, uint8_t reg) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return 0;  // Communication error
    }
    
    Wire.requestFrom(address, (uint8_t)2);
    if (Wire.available() >= 2) {
        uint16_t value = ((uint16_t)Wire.read() << 8) | Wire.read();
        return value;
    }
    return 0;
}

uint32_t ina228_readRegister24(uint8_t address, uint8_t reg) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return 0;  // Communication error
    }
    
    Wire.requestFrom(address, (uint8_t)3);
    if (Wire.available() >= 3) {
        uint32_t value = ((uint32_t)Wire.read() << 16) | 
                         ((uint32_t)Wire.read() << 8) | 
                         (uint32_t)Wire.read();
        return value;
    }
    return 0;
}

bool initializeINA228(uint8_t address, const char* sensorName) {
    if (ENABLE_SERIAL_DEBUG) {
        Serial.print(F("Initializing INA228 "));
        Serial.print(sensorName);
        Serial.print(F(" (0x"));
        Serial.print(address, HEX);
        Serial.print(F(")... "));
    }
    
    // First check if device is present by reading Device ID
    uint16_t deviceID = ina228_readRegister16(address, INA228_REG_DEVICEID);
    if (ENABLE_SERIAL_DEBUG) {
        Serial.print(F("Device ID: 0x"));
        Serial.print(deviceID, HEX);
        Serial.print(F(" "));
    }
    
    // INA228 should return 0x2280 or 0x2281 (TI forum confirms both exist)
    if (deviceID != 0x2280 && deviceID != 0x2281) {
        if (ENABLE_SERIAL_DEBUG) {
            Serial.print(F("Failed! Expected 0x2280 or 0x2281, got 0x"));
            Serial.println(deviceID, HEX);
        }
        return false;
    }
    
    // Configure device for continuous measurement
    // Bit 15-12: RESET=0, RSVD=0, DELAY=00 (no delay)
    // Bit 11-9: ADCRANGE=0 (±163.84 mV range)  
    // Bit 8-6: BUSCT=100 (1052 µs conversion time)
    // Bit 5-3: SHUNTCT=100 (1052 µs conversion time)  
    // Bit 2-0: AVERAGES=000 (no averaging)
    // Bit 15-14: MODE=11 (continuous shunt and bus)
    uint16_t config = 0x4000 | (0x4 << 6) | (0x4 << 3) | 0x03;  // 0x4233
    if (!ina228_writeRegister16(address, INA228_REG_CONFIG, config)) {
        if (ENABLE_SERIAL_DEBUG) Serial.println(F("Failed to set configuration!"));
        return false;
    }
    
    // Calculate and set shunt calibration
    // SHUNT_CAL = 13107.2 × 10^6 × CURRENT_LSB × RSHUNT
    float shuntCal_f = 13107.2e6 * CURRENT_LSB * RSHUNT_OHMS;
    uint16_t shuntCal = (uint16_t)(shuntCal_f + 0.5);  // Round to nearest integer
    
    if (ENABLE_SERIAL_DEBUG) {
        Serial.print(F("SHUNT_CAL: "));
        Serial.print(shuntCal);
        Serial.print(F(" "));
    }
    
    if (!ina228_writeRegister16(address, INA228_REG_SHUNTCAL, shuntCal)) {
        if (ENABLE_SERIAL_DEBUG) Serial.println(F("Failed to set shunt calibration!"));
        return false;
    }
    
    // Give device time to start conversions
    delay(10);
    
    if (ENABLE_SERIAL_DEBUG) Serial.println(F("Success!"));
    return true;
}

float ina228_readBusVoltage(uint8_t address) {
    uint32_t raw = ina228_readRegister24(address, INA228_REG_VBUS);
    // Bus voltage LSB = 195.3125 µV, right-justified in upper 20 bits
    return (float)((raw >> 4) * 195.3125e-6);  // Convert to volts
}

float ina228_readCurrent(uint8_t address) {
    uint32_t raw = ina228_readRegister24(address, INA228_REG_CURRENT);
    
    // Cast to signed 32-bit for proper sign extension
    int32_t signed_raw = (int32_t)raw;
    
    // Sign extend if negative (bit 23 is sign bit for 24-bit value)
    if (signed_raw & 0x800000) {  // Check bit 23 (correct sign bit)
        signed_raw |= 0xFF000000;  // Sign extend to 32-bit
    }
    
    // Right shift by 4 bits as required by INA228 datasheet
    signed_raw >>= 4;
    
    // Convert to current using calculated CURRENT_LSB
    return (float)(signed_raw * CURRENT_LSB * 1000.0);  // Convert to mA
}

float ina228_readPower(uint8_t address) {
    uint32_t raw = ina228_readRegister24(address, INA228_REG_POWER);
    // Power LSB = 3.2 × CURRENT_LSB (from INA228 datasheet)
    // CRITICAL FIX: Power register should NOT be right-shifted by 4
    // Only current and voltage registers need the >>4 shift
    float powerLSB = 3.2 * CURRENT_LSB;
    return (float)(raw * powerLSB * 1000.0);  // Convert to mW
}

// CSV Logging Functions
bool initializeCSVLogging() {
    bool fileExists = SD.exists(CSV_FILENAME);
    
    File32 logFile = SD.open(CSV_FILENAME, O_WRONLY | O_CREAT | O_APPEND);
    if (!logFile) {
        Serial.println(F("ERROR: Could not open CSV file for logging."));
        return false;
    }
    
    if (!fileExists) {
        logFile.println(F("TestRunID,TestState,EntryTimestamp_ms,Solar_Voltage_V,Solar_Current_mA,Solar_Power_HW_mW,Solar_Power_Calc_mW,Batt_Voltage_V,Batt_Current_mA,Batt_Power_HW_mW,Batt_Power_Calc_mW,Load_Voltage_V,Load_Current_mA,Load_Power_HW_mW,Load_Power_Calc_mW"));
        Serial.println(F("CSV header written for three-sensor system."));
    }
    
    logFile.close();
    return true;
}

bool appendPowerDataToCSV() {
    File32 logFile = SD.open(CSV_FILENAME, O_WRONLY | O_APPEND);
    if (!logFile) {
        if (ENABLE_SERIAL_DEBUG) Serial.println(F("ERROR: Could not open CSV file for appending."));
        return false;
    }
    
    const char* stateName = getTestStateName(currentTestState);
    
    // Write all buffered entries (updated for three sensors)
    for (int i = 0; i < bufferIndex; i++) {
        logFile.print(testRunID);
        logFile.print(",");
        logFile.print(stateName);
        logFile.print(",");
        logFile.print(powerDataBuffer[i].timestamp_ms);
        logFile.print(",");
        // Solar sensor data
        logFile.print(powerDataBuffer[i].solar_voltage_V, 4);
        logFile.print(",");
        logFile.print(powerDataBuffer[i].solar_current_mA, 4);
        logFile.print(",");
        logFile.print(powerDataBuffer[i].solar_power_hw_mW, 4);
        logFile.print(",");
        logFile.print(powerDataBuffer[i].solar_power_calc_mW, 4);
        logFile.print(",");
        // Battery sensor data
        logFile.print(powerDataBuffer[i].battery_voltage_V, 4);
        logFile.print(",");
        logFile.print(powerDataBuffer[i].battery_current_mA, 4);
        logFile.print(",");
        logFile.print(powerDataBuffer[i].battery_power_hw_mW, 4);
        logFile.print(",");
        logFile.print(powerDataBuffer[i].battery_power_calc_mW, 4);
        logFile.print(",");
        // Load sensor data
        logFile.print(powerDataBuffer[i].load_voltage_V, 4);
        logFile.print(",");
        logFile.print(powerDataBuffer[i].load_current_mA, 4);
        logFile.print(",");
        logFile.print(powerDataBuffer[i].load_power_hw_mW, 4);
        logFile.print(",");
        logFile.print(powerDataBuffer[i].load_power_calc_mW, 4);
        logFile.println();
    }
    
    logFile.close();
    
    if (ENABLE_SERIAL_DEBUG) {
        Serial.print(F("Written "));
        Serial.print(bufferIndex);
        Serial.println(F(" entries to CSV."));
    }
    
    // Reset buffer after successful write
    bufferIndex = 0;
    return true;
}

void logSingleEntryToCSV(const PowerLogEntry& entry, const char* testStateName) {
    File32 logFile = SD.open(CSV_FILENAME, O_WRONLY | O_APPEND);
    if (!logFile) {
        if (ENABLE_SERIAL_DEBUG) Serial.println(F("ERROR: Could not open CSV file for single entry."));
        return;
    }
    
    logFile.print(testRunID);
    logFile.print(",");
    logFile.print(testStateName);
    logFile.print(",");
    logFile.print(entry.timestamp_ms);
    logFile.print(",");
    // Solar sensor data
    logFile.print(entry.solar_voltage_V, 4);
    logFile.print(",");
    logFile.print(entry.solar_current_mA, 4);
    logFile.print(",");
    logFile.print(entry.solar_power_hw_mW, 4);
    logFile.print(",");
    logFile.print(entry.solar_power_calc_mW, 4);
    logFile.print(",");
    // Battery sensor data
    logFile.print(entry.battery_voltage_V, 4);
    logFile.print(",");
    logFile.print(entry.battery_current_mA, 4);
    logFile.print(",");
    logFile.print(entry.battery_power_hw_mW, 4);
    logFile.print(",");
    logFile.print(entry.battery_power_calc_mW, 4);
    logFile.print(",");
    // Load sensor data
    logFile.print(entry.load_voltage_V, 4);
    logFile.print(",");
    logFile.print(entry.load_current_mA, 4);
    logFile.print(",");
    logFile.print(entry.load_power_hw_mW, 4);
    logFile.print(",");
    logFile.print(entry.load_power_calc_mW, 4);
    logFile.println();
    
    logFile.close();
}

// Utility Functions
const char* getTestStateName(TestState state) {
    switch (state) {
        case TEST_MCU_ACTIVE_SD_DEINITIALIZED: return "MCU_Active_SD_Deinitialized";
        case TEST_MCU_ACTIVE_SD_IDLE_STANDBY: return "MCU_Active_SD_Idle_Standby";
        case TEST_SUSTAINED_SD_WRITE: return "Sustained_SD_Write";
        case TEST_PERIODIC_BATCH_WRITE_CYCLE: return "Periodic_Batch_Write_Cycle";
        default: return "Unknown";
    }
}

void takePowerMeasurement() {
    if (bufferIndex < RAM_BUFFER_SIZE) {
        powerDataBuffer[bufferIndex].timestamp_ms = millis();
        
        // Solar sensor (0x40) measurements
        powerDataBuffer[bufferIndex].solar_voltage_V = ina228_readBusVoltage(INA228_SOLAR_I2C_ADDRESS);
        powerDataBuffer[bufferIndex].solar_current_mA = ina228_readCurrent(INA228_SOLAR_I2C_ADDRESS);
        powerDataBuffer[bufferIndex].solar_power_hw_mW = ina228_readPower(INA228_SOLAR_I2C_ADDRESS);
        powerDataBuffer[bufferIndex].solar_power_calc_mW = powerDataBuffer[bufferIndex].solar_voltage_V * powerDataBuffer[bufferIndex].solar_current_mA;
        
        // Battery sensor (0x44) measurements
        powerDataBuffer[bufferIndex].battery_voltage_V = ina228_readBusVoltage(INA228_BATTERY_I2C_ADDRESS);
        powerDataBuffer[bufferIndex].battery_current_mA = ina228_readCurrent(INA228_BATTERY_I2C_ADDRESS);
        powerDataBuffer[bufferIndex].battery_power_hw_mW = ina228_readPower(INA228_BATTERY_I2C_ADDRESS);
        powerDataBuffer[bufferIndex].battery_power_calc_mW = powerDataBuffer[bufferIndex].battery_voltage_V * powerDataBuffer[bufferIndex].battery_current_mA;
        
        // Load sensor (0x41) measurements
        powerDataBuffer[bufferIndex].load_voltage_V = ina228_readBusVoltage(INA228_LOAD_I2C_ADDRESS);
        powerDataBuffer[bufferIndex].load_current_mA = ina228_readCurrent(INA228_LOAD_I2C_ADDRESS);
        powerDataBuffer[bufferIndex].load_power_hw_mW = ina228_readPower(INA228_LOAD_I2C_ADDRESS);
        powerDataBuffer[bufferIndex].load_power_calc_mW = powerDataBuffer[bufferIndex].load_voltage_V * powerDataBuffer[bufferIndex].load_current_mA;
        
        bufferIndex++;
    }
}

bool initializeTestEnvironment() {
    Serial.println(F("Initializing test environment..."));
    
    Serial.print(F("Initializing I2C bus... "));
    Wire.begin();
    Serial.println(F("Success!"));
    
    // Initialize solar sensor (0x40)
    if (!initializeINA228(INA228_SOLAR_I2C_ADDRESS, "Solar")) {
        Serial.println(F("FATAL ERROR: Could not initialize INA228 Solar sensor!"));
        return false;
    }
    
    // Initialize battery sensor (0x44) (primary sensor)
    if (!initializeINA228(INA228_BATTERY_I2C_ADDRESS, "Battery")) {
        Serial.println(F("FATAL ERROR: Could not initialize INA228 Battery sensor!"));
        return false;
    }
    
    // Initialize load sensor (0x41) (after 3.3V regulator)
    if (!initializeINA228(INA228_LOAD_I2C_ADDRESS, "Load")) {
        Serial.println(F("FATAL ERROR: Could not initialize INA228 Load sensor!"));
        return false;
    }
    
    Serial.print(F("Waking and initializing SD card..."));
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println(F("FATAL ERROR: Could not initialize SD card!"));
        return false;
    }
    Serial.println(F("SD card initialized successfully."));
    
    if (!initializeCSVLogging()) {
        SD.end();
        Serial.println(F("FATAL ERROR: Could not initialize SD card logging!"));
        return false;
    }
    
    return true;
}

// Test State Implementations
void runTest_MCU_Active_SD_Deinitialized() {
    if (ENABLE_SERIAL_DEBUG) Serial.println(F("=== TEST STATE 1: MCU Active, SD De-initialized ==="));
    currentTestState = TEST_MCU_ACTIVE_SD_DEINITIALIZED;
    testStateStartTime_ms = millis();
    bufferIndex = 0;
    
    SD.end();
    if (ENABLE_SERIAL_DEBUG) Serial.println(F("SD card de-initialized."));
    
    uint32_t nextSampleTime_ms = testStateStartTime_ms;
    
    while ((millis() - testStateStartTime_ms) < (TEST_STATE_DURATION_S * 1000)) {
        uint32_t now = millis();
        
        if (now >= nextSampleTime_ms) {
            takePowerMeasurement();
            if (ENABLE_SERIAL_DEBUG) {
                Serial.print(F("  ["));
                Serial.print((now - testStateStartTime_ms) / 1000);
                Serial.print(F(" s] Baseline measurement taken. Buffer size: "));
                Serial.println(bufferIndex);
            }
            nextSampleTime_ms = now + SENSOR_SAMPLING_INTERVAL_MS;
        }
        
        delay(10);
    }
    
    if (ENABLE_SERIAL_DEBUG) {
        Serial.print(F("Test State 1 completed. Collected "));
        Serial.print(bufferIndex);
        Serial.println(F(" samples."));
    }
}

void runTest_MCU_Active_SD_Idle_Standby() {
    if (ENABLE_SERIAL_DEBUG) Serial.println(F("=== TEST STATE 2: MCU Active, SD Idle Standby ==="));
    currentTestState = TEST_MCU_ACTIVE_SD_IDLE_STANDBY;
    testStateStartTime_ms = millis();
    bufferIndex = 0;  // CRITICAL: Reset buffer for new test state
    
    if (!SD.begin(SD_CS_PIN)) {
        if (ENABLE_SERIAL_DEBUG) Serial.println(F("ERROR: Could not re-initialize SD card for Test State 2!"));
        return;
    }
    if (ENABLE_SERIAL_DEBUG) Serial.println(F("SD card re-initialized for idle standby test."));
    
    uint32_t nextSampleTime_ms = testStateStartTime_ms;
    
    while ((millis() - testStateStartTime_ms) < (TEST_STATE_DURATION_S * 1000)) {
        uint32_t now = millis();
        
        if (now >= nextSampleTime_ms) {
            takePowerMeasurement();
            if (ENABLE_SERIAL_DEBUG) {
                Serial.print(F("  ["));
                Serial.print((now - testStateStartTime_ms) / 1000);
                Serial.print(F(" s] Idle measurement taken. Buffer size: "));
                Serial.println(bufferIndex);
            }
            nextSampleTime_ms = now + SENSOR_SAMPLING_INTERVAL_MS;
        }
        
        delay(10);
    }
    
    if (ENABLE_SERIAL_DEBUG) {
        Serial.print(F("Test State 2 completed. Collected "));
        Serial.print(bufferIndex);
        Serial.println(F(" samples."));
    }
}

void runTest_Sustained_SD_Write() {
    if (ENABLE_SERIAL_DEBUG) Serial.println(F("=== TEST STATE 3: Sustained SD Write (1Hz) ==="));
    currentTestState = TEST_SUSTAINED_SD_WRITE;
    testStateStartTime_ms = millis();
    
    uint32_t nextSampleTime_ms = testStateStartTime_ms;
    
    while ((millis() - testStateStartTime_ms) < (TEST_STATE_DURATION_S * 1000)) {
        uint32_t now = millis();
        
        if (now >= nextSampleTime_ms) {
            PowerLogEntry entry;
            entry.timestamp_ms = now;
            
            // Solar sensor measurements
            entry.solar_voltage_V = ina228_readBusVoltage(INA228_SOLAR_I2C_ADDRESS);
            entry.solar_current_mA = ina228_readCurrent(INA228_SOLAR_I2C_ADDRESS);
            entry.solar_power_hw_mW = ina228_readPower(INA228_SOLAR_I2C_ADDRESS);
            entry.solar_power_calc_mW = entry.solar_voltage_V * entry.solar_current_mA;
            
            // Battery sensor measurements
            entry.battery_voltage_V = ina228_readBusVoltage(INA228_BATTERY_I2C_ADDRESS);
            entry.battery_current_mA = ina228_readCurrent(INA228_BATTERY_I2C_ADDRESS);
            entry.battery_power_hw_mW = ina228_readPower(INA228_BATTERY_I2C_ADDRESS);
            entry.battery_power_calc_mW = entry.battery_voltage_V * entry.battery_current_mA;
            
            // Load sensor measurements
            entry.load_voltage_V = ina228_readBusVoltage(INA228_LOAD_I2C_ADDRESS);
            entry.load_current_mA = ina228_readCurrent(INA228_LOAD_I2C_ADDRESS);
            entry.load_power_hw_mW = ina228_readPower(INA228_LOAD_I2C_ADDRESS);
            entry.load_power_calc_mW = entry.load_voltage_V * entry.load_current_mA;
            
            logSingleEntryToCSV(entry, getTestStateName(currentTestState));
            
            if (ENABLE_SERIAL_DEBUG) {
                Serial.print(F("  ["));
                Serial.print((now - testStateStartTime_ms) / 1000);
                Serial.println(F(" s] Sustained write entry logged directly to CSV."));
            }
            nextSampleTime_ms = now + SENSOR_SAMPLING_INTERVAL_MS;
        }
        
        delay(10);
    }
    
    if (ENABLE_SERIAL_DEBUG) Serial.println(F("Test State 3 completed. All entries written directly to CSV."));
}

void runTest_Periodic_Batch_Write_Cycle() {
    if (ENABLE_SERIAL_DEBUG) Serial.println(F("=== TEST STATE 4: Periodic Batch Write Cycle ==="));
    currentTestState = TEST_PERIODIC_BATCH_WRITE_CYCLE;
    testStateStartTime_ms = millis();
    bufferIndex = 0;
    
    if (ENABLE_SERIAL_DEBUG) Serial.println(F("  Test State 4 initialized. SD card will remain active throughout test."));
    
    const uint32_t BATCH_WRITE_INTERVAL_MS = 30000;
    uint32_t nextBatchTime_ms = testStateStartTime_ms + BATCH_WRITE_INTERVAL_MS;
    uint32_t nextSampleTime_ms = testStateStartTime_ms;
    
    if (ENABLE_SERIAL_DEBUG) {
        Serial.print(F("  First batch write scheduled at: "));
        Serial.print(BATCH_WRITE_INTERVAL_MS / 1000);
        Serial.println(F(" seconds"));
    }
    
    while ((millis() - testStateStartTime_ms) < (TEST_STATE_DURATION_S * 1000)) {
        uint32_t now = millis();
        
        // Handle batch writes every 30 seconds
        if (now >= nextBatchTime_ms && bufferIndex > 0) {
            if (ENABLE_SERIAL_DEBUG) {
                Serial.print(F("  [BATCH WRITE] Writing "));
                Serial.print(bufferIndex);
                Serial.print(F(" buffered entries to CSV... "));
            }
            
            delay(50);  // Timing buffer for SD operations (Issue #2 prevention)
            
            if (appendPowerDataToCSV()) {
                if (ENABLE_SERIAL_DEBUG) Serial.println(F("Success!"));
            } else {
                if (ENABLE_SERIAL_DEBUG) Serial.println(F("Failed!"));
            }
            
            nextBatchTime_ms = now + BATCH_WRITE_INTERVAL_MS;
            
            if (ENABLE_SERIAL_DEBUG) {
                Serial.print(F("  Next batch write scheduled in "));
                Serial.print(BATCH_WRITE_INTERVAL_MS / 1000);
                Serial.println(F(" seconds"));
            }
        }
        
        // Handle measurement sampling
        if (now >= nextSampleTime_ms) {
            takePowerMeasurement();
            if (ENABLE_SERIAL_DEBUG) {
                Serial.print(F("  ["));
                Serial.print((now - testStateStartTime_ms) / 1000);
                Serial.print(F(" s] Measurement buffered. Buffer size: "));
                Serial.println(bufferIndex);
            }
            nextSampleTime_ms = now + SENSOR_SAMPLING_INTERVAL_MS;
        }
        
        delay(10);
    }
    
    // Final batch write for any remaining data
    if (bufferIndex > 0) {
        if (ENABLE_SERIAL_DEBUG) {
            Serial.print(F("  [FINAL BATCH] Writing remaining "));
            Serial.print(bufferIndex);
            Serial.print(F(" entries... "));
        }
        
        delay(50);  // Timing buffer for SD operations
        
        if (appendPowerDataToCSV()) {
            if (ENABLE_SERIAL_DEBUG) Serial.println(F("Success!"));
        } else {
            if (ENABLE_SERIAL_DEBUG) Serial.println(F("Failed!"));
        }
    }
    
    if (ENABLE_SERIAL_DEBUG) Serial.println(F("Test State 4 completed successfully."));
}

// Main Arduino Functions
void setup() {
    if (ENABLE_SERIAL_DEBUG) {
        Serial.begin(115200);
        delay(2000);
        
        Serial.println(F("\n=========================================================="));
        Serial.println(F("    SD CARD POWER CONSUMPTION TEST - ESSENTIAL STATES"));
        Serial.println(F("             WITH TRIPLE INA228 SENSOR MONITORING"));
        Serial.println(F("=========================================================="));
        Serial.println();
        Serial.println(F("Purpose: Measure power consumption across four essential"));
        Serial.println(F("         SD card operational states with precise timing"));
        Serial.println(F("         Includes SOLAR, BATTERY and LOAD sensor monitoring"));
        Serial.println();
        
        Serial.println(F("--- THREE-SENSOR CONFIGURATION ---"));
        Serial.println(F("SOLAR Sensor (0x40):   Solar panel power monitoring"));
        Serial.println(F("BATTERY Sensor (0x44): Direct battery monitoring"));
        Serial.println(F("LOAD Sensor (0x41):    Post-3.3V regulator monitoring"));
        Serial.println(F("Power Calculation:      Hardware register + Manual V×I"));
        Serial.println(F("Data Validation:        Compare both power methods"));
        Serial.println(F("Power Balance:          Track Solar vs Battery+Load"));
        Serial.println();
        
        Serial.println(F("--- CONFIGURATION SUMMARY ---"));
    }
    
    testRunID = millis();
    
    if (ENABLE_SERIAL_DEBUG) {
        Serial.print(F("Test Run ID: "));
        Serial.println(testRunID);
        Serial.print(F("Test State Duration:    "));
        Serial.print(TEST_STATE_DURATION_S);
        Serial.println(F(" seconds"));
        Serial.print(F("Sampling Interval:      "));
        Serial.print(SENSOR_SAMPLING_INTERVAL_MS);
        Serial.print(F(" ms ("));
        Serial.print(1000.0 / SENSOR_SAMPLING_INTERVAL_MS);
        Serial.println(F(" Hz)"));
        Serial.print(F("RAM Buffer Size:        "));
        Serial.print(RAM_BUFFER_SIZE);
        Serial.println(F(" samples"));
        Serial.print(F("SD Card CS Pin:         "));
        Serial.println(SD_CS_PIN);
        Serial.print(F("CSV Filename:           "));
        Serial.println(CSV_FILENAME);
        Serial.println();
    }
    
    if (!initializeTestEnvironment()) {
        if (ENABLE_SERIAL_DEBUG) {
            Serial.println(F("FATAL ERROR: Failed to initialize test environment!"));
            Serial.println(F("Check hardware connections and reset system."));
        }
        while (1) delay(1000);
    }
    
    if (ENABLE_SERIAL_DEBUG) {
        Serial.println(F("=== THREE SENSOR VALIDATION TEST ==="));
        Serial.println(F("Comparing solar, battery, and load sensor readings..."));
        
        // Take a few sample readings to compare all three sensors
        for (int i = 0; i < 3; i++) {
            delay(1000);  // 1 second between readings
            
            float solar_voltage = ina228_readBusVoltage(INA228_SOLAR_I2C_ADDRESS);
            float solar_current = ina228_readCurrent(INA228_SOLAR_I2C_ADDRESS);
            float solar_power_hw = ina228_readPower(INA228_SOLAR_I2C_ADDRESS);
            float solar_power_calc = solar_voltage * solar_current;  // Manual V×I calculation
            
            float batt_voltage = ina228_readBusVoltage(INA228_BATTERY_I2C_ADDRESS);
            float batt_current = ina228_readCurrent(INA228_BATTERY_I2C_ADDRESS);
            float batt_power_hw = ina228_readPower(INA228_BATTERY_I2C_ADDRESS);
            float batt_power_calc = batt_voltage * batt_current;  // Manual V×I calculation
            
            float load_voltage = ina228_readBusVoltage(INA228_LOAD_I2C_ADDRESS);
            float load_current = ina228_readCurrent(INA228_LOAD_I2C_ADDRESS);
            float load_power_hw = ina228_readPower(INA228_LOAD_I2C_ADDRESS);
            float load_power_calc = load_voltage * load_current;  // Manual V×I calculation
            
            Serial.print(F("Sample ")); Serial.print(i+1); Serial.println(F(":"));
            Serial.print(F("  SOLAR:   ")); Serial.print(solar_voltage, 3); Serial.print(F("V, "));
            Serial.print(solar_current, 2); Serial.print(F("mA"));
            Serial.println();
            Serial.print(F("    Power HW: ")); Serial.print(solar_power_hw, 2); Serial.print(F("mW, "));
            Serial.print(F("Calc: ")); Serial.print(solar_power_calc, 2); Serial.print(F("mW, "));
            Serial.print(F("Diff: ")); Serial.print(abs(solar_power_hw - solar_power_calc), 2); Serial.println(F("mW"));
            
            Serial.print(F("  BATTERY: ")); Serial.print(batt_voltage, 3); Serial.print(F("V, "));
            Serial.print(batt_current, 2); Serial.print(F("mA"));
            Serial.println();
            Serial.print(F("    Power HW: ")); Serial.print(batt_power_hw, 2); Serial.print(F("mW, "));
            Serial.print(F("Calc: ")); Serial.print(batt_power_calc, 2); Serial.print(F("mW, "));
            Serial.print(F("Diff: ")); Serial.print(abs(batt_power_hw - batt_power_calc), 2); Serial.println(F("mW"));
            
            Serial.print(F("  LOAD:    ")); Serial.print(load_voltage, 3); Serial.print(F("V, "));
            Serial.print(load_current, 2); Serial.print(F("mA"));
            Serial.println();
            Serial.print(F("    Power HW: ")); Serial.print(load_power_hw, 2); Serial.print(F("mW, "));
            Serial.print(F("Calc: ")); Serial.print(load_power_calc, 2); Serial.print(F("mW, "));
            Serial.print(F("Diff: ")); Serial.print(abs(load_power_hw - load_power_calc), 2); Serial.println(F("mW"));
            
            // Calculate power balance (Solar input vs Battery+Load output)
            float power_balance = solar_power_hw - (batt_power_hw + load_power_hw);
            
            Serial.print(F("  POWER BALANCE: Solar=")); Serial.print(solar_power_hw, 2); Serial.print(F("mW, "));
            Serial.print(F("Batt+Load=")); Serial.print(batt_power_hw + load_power_hw, 2); Serial.print(F("mW, "));
            Serial.print(F("Balance=")); Serial.print(power_balance, 2); Serial.println(F("mW"));
            Serial.println();
        }
        
        Serial.println(F("=== SD Card Power Test Essentials ==="));
        Serial.println(F("Starting essential power consumption testing..."));
    }
}

void loop() {
    // Test State 1: MCU Active, SD De-initialized (uses RAM buffering)
    runTest_MCU_Active_SD_Deinitialized();
    
    // Re-initialize SD card and write Test State 1 data
    if (!SD.begin(SD_CS_PIN)) {
        if (ENABLE_SERIAL_DEBUG) Serial.println(F("ERROR: Could not re-initialize SD card after Test State 1!"));
        while (1) delay(1000);
    }
    if (bufferIndex > 0) {
        appendPowerDataToCSV();
        if (ENABLE_SERIAL_DEBUG) Serial.println(F("Test State 1 data written to CSV."));
    }
    
    // Test State 2: MCU Active, SD Idle Standby (uses RAM buffering)
    runTest_MCU_Active_SD_Idle_Standby();
    if (bufferIndex > 0) {
        appendPowerDataToCSV();
        if (ENABLE_SERIAL_DEBUG) Serial.println(F("Test State 2 data written to CSV."));
    }
    
    // Test State 3: Sustained SD Write (writes directly to CSV, no buffering)
    runTest_Sustained_SD_Write();
    // No appendPowerDataToCSV() call needed - data already written directly
    
    // Test State 4: Periodic Batch Write Cycle (handles its own CSV writing)
    runTest_Periodic_Batch_Write_Cycle();
    // No appendPowerDataToCSV() call needed - handles its own batch writing
    
    if (ENABLE_SERIAL_DEBUG) {
        Serial.println(F("\n=== ALL TESTS COMPLETED ==="));
        Serial.print(F("Test Run ID: "));
        Serial.println(testRunID);
        Serial.println(F("Results saved to: test.csv"));
        Serial.println(F("System will halt. Reset to run again."));
    }
    
    while (1) delay(1000);
} 