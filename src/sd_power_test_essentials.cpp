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
const uint8_t INA228_I2C_ADDRESS = 0x44;

// INA228 Register Addresses (8-bit)
const uint8_t INA228_REG_CONFIG = 0x00;
const uint8_t INA228_REG_VBUS = 0x05;
const uint8_t INA228_REG_CURRENT = 0x07;
const uint8_t INA228_REG_POWER = 0x08;
const uint8_t INA228_REG_SHUNTCAL = 0x02;
const uint8_t INA228_REG_DEVICEID = 0x3F;

// Test Configuration
const uint32_t TEST_STATE_DURATION_S = 60;
const uint32_t SENSOR_SAMPLING_INTERVAL_MS = 1000;
const int RAM_BUFFER_SIZE = 60;
const char CSV_FILENAME[] = "test.csv";

// Current sensing parameters
const float RSHUNT_OHMS = 0.0177186;        // Effective shunt resistance 
const float MAX_CURRENT_A = 20.0;           // Maximum expected current
const float CURRENT_LSB = MAX_CURRENT_A / 524288.0;  // 19-bit signed, so 2^19 = 524288

// Data Structure
struct PowerLogEntry {
    uint32_t timestamp_ms;
    float voltage_V;
    float current_mA;
    float power_mW;
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
bool ina228_writeRegister16(uint8_t reg, uint16_t value);
uint16_t ina228_readRegister16(uint8_t reg);
uint32_t ina228_readRegister24(uint8_t reg);
bool initializeINA228();
float ina228_readBusVoltage();
float ina228_readCurrent();
float ina228_readPower();
bool initializeCSVLogging();
const char* getTestStateName(TestState state);
bool appendPowerDataToCSV();
void logSingleEntryToCSV(const PowerLogEntry& entry, const char* testStateName);
void takePowerMeasurement();
bool initializeTestEnvironment();

// I2C Communication Functions
bool ina228_writeRegister16(uint8_t reg, uint16_t value) {
    Wire.beginTransmission(INA228_I2C_ADDRESS);
    Wire.write(reg);
    Wire.write((value >> 8) & 0xFF);  // MSB first
    Wire.write(value & 0xFF);         // LSB second
    return (Wire.endTransmission() == 0);
}

uint16_t ina228_readRegister16(uint8_t reg) {
    Wire.beginTransmission(INA228_I2C_ADDRESS);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return 0;  // Communication error
    }
    
    Wire.requestFrom(INA228_I2C_ADDRESS, (uint8_t)2);
    if (Wire.available() >= 2) {
        uint16_t value = ((uint16_t)Wire.read() << 8) | Wire.read();
        return value;
    }
    return 0;
}

uint32_t ina228_readRegister24(uint8_t reg) {
    Wire.beginTransmission(INA228_I2C_ADDRESS);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return 0;  // Communication error
    }
    
    Wire.requestFrom(INA228_I2C_ADDRESS, (uint8_t)3);
    if (Wire.available() >= 3) {
        uint32_t value = ((uint32_t)Wire.read() << 16) | 
                         ((uint32_t)Wire.read() << 8) | 
                         (uint32_t)Wire.read();
        return value;
    }
    return 0;
}

bool initializeINA228() {
    if (ENABLE_SERIAL_DEBUG) Serial.print(F("Initializing INA228... "));
    
    // First check if device is present by reading Device ID
    uint16_t deviceID = ina228_readRegister16(INA228_REG_DEVICEID);
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
    if (!ina228_writeRegister16(INA228_REG_CONFIG, config)) {
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
    
    if (!ina228_writeRegister16(INA228_REG_SHUNTCAL, shuntCal)) {
        if (ENABLE_SERIAL_DEBUG) Serial.println(F("Failed to set shunt calibration!"));
        return false;
    }
    
    // Give device time to start conversions
    delay(10);
    
    if (ENABLE_SERIAL_DEBUG) Serial.println(F("Success!"));
    return true;
}

float ina228_readBusVoltage() {
    uint32_t raw = ina228_readRegister24(INA228_REG_VBUS);
    // Bus voltage LSB = 195.3125 µV, right-justified in upper 20 bits
    return (float)((raw >> 4) * 195.3125e-6);  // Convert to volts
}

float ina228_readCurrent() {
    uint32_t raw = ina228_readRegister24(INA228_REG_CURRENT);
    // Sign extend if negative (bit 19 is sign bit for 20-bit value)
    if (raw & 0x80000) {  // Check bit 19 (sign bit)
        raw |= 0xFFF00000;  // Sign extend to 32-bit
    }
    
    int32_t signed_raw = (int32_t)raw;
    // Convert to current using calculated CURRENT_LSB
    return (float)(signed_raw * CURRENT_LSB * 1000.0);  // Convert to mA
}

float ina228_readPower() {
    uint32_t raw = ina228_readRegister24(INA228_REG_POWER);
    // Power LSB = 3.2 × CURRENT_LSB (from INA228 datasheet)
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
        logFile.println(F("TestRunID,TestState,EntryTimestamp_ms,Voltage_V,Current_mA,Power_mW"));
        Serial.println(F("CSV header written."));
    }
    
    logFile.close();
    return true;
}

bool appendPowerDataToCSV() {
    File32 logFile = SD.open(CSV_FILENAME, O_WRONLY | O_APPEND);
    if (!logFile) {
        Serial.println(F("ERROR: Could not open CSV file for appending."));
        return false;
    }
    
    const char* stateName = getTestStateName(currentTestState);
    
    for (int i = 0; i < bufferIndex; i++) {
        logFile.print(testRunID);
        logFile.print(",");
        logFile.print(stateName);
        logFile.print(",");
        logFile.print(powerDataBuffer[i].timestamp_ms);
        logFile.print(",");
        logFile.print(powerDataBuffer[i].voltage_V, 4);
        logFile.print(",");
        logFile.print(powerDataBuffer[i].current_mA, 4);
        logFile.print(",");
        logFile.println(powerDataBuffer[i].power_mW, 4);
    }
    
    logFile.close();
    bufferIndex = 0;
    return true;
}

void logSingleEntryToCSV(const PowerLogEntry& entry, const char* testStateName) {
    File32 logFile = SD.open(CSV_FILENAME, O_WRONLY | O_APPEND);
    if (!logFile) {
        Serial.println(F("ERROR: Could not open CSV file for single entry."));
        return;
    }
    
    logFile.print(testRunID);
    logFile.print(",");
    logFile.print(testStateName);
    logFile.print(",");
    logFile.print(entry.timestamp_ms);
    logFile.print(",");
    logFile.print(entry.voltage_V, 4);
    logFile.print(",");
    logFile.print(entry.current_mA, 4);
    logFile.print(",");
    logFile.println(entry.power_mW, 4);
    
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
        powerDataBuffer[bufferIndex].voltage_V = ina228_readBusVoltage();
        powerDataBuffer[bufferIndex].current_mA = ina228_readCurrent();
        powerDataBuffer[bufferIndex].power_mW = ina228_readPower();
        bufferIndex++;
    }
}

bool initializeTestEnvironment() {
    Serial.println(F("Initializing test environment..."));
    
    Serial.print(F("Initializing I2C bus... "));
    Wire.begin();
    Serial.println(F("Success!"));
    
    if (!initializeINA228()) {
        Serial.println(F("FATAL ERROR: Could not initialize INA228 sensor!"));
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
    Serial.println(F("=== TEST STATE 1: MCU Active, SD De-initialized ==="));
    currentTestState = TEST_MCU_ACTIVE_SD_DEINITIALIZED;
    testStateStartTime_ms = millis();
    bufferIndex = 0;
    
    SD.end();
    Serial.println(F("SD card de-initialized."));
    
    uint32_t nextSampleTime_ms = testStateStartTime_ms;
    
    while ((millis() - testStateStartTime_ms) < (TEST_STATE_DURATION_S * 1000)) {
        uint32_t now = millis();
        
        if (now >= nextSampleTime_ms) {
            takePowerMeasurement();
            Serial.print(F("  ["));
            Serial.print((now - testStateStartTime_ms) / 1000);
            Serial.print(F(" s] Baseline measurement taken. Buffer size: "));
            Serial.println(bufferIndex);
            nextSampleTime_ms = now + SENSOR_SAMPLING_INTERVAL_MS;
        }
        
        delay(10);
    }
    
    Serial.print(F("Test State 1 completed. Collected "));
    Serial.print(bufferIndex);
    Serial.println(F(" samples."));
}

void runTest_MCU_Active_SD_Idle_Standby() {
    Serial.println(F("=== TEST STATE 2: MCU Active, SD Idle Standby ==="));
    currentTestState = TEST_MCU_ACTIVE_SD_IDLE_STANDBY;
    testStateStartTime_ms = millis();
    
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println(F("ERROR: Could not re-initialize SD card for Test State 2!"));
        return;
    }
    Serial.println(F("SD card re-initialized for idle standby test."));
    
    uint32_t nextSampleTime_ms = testStateStartTime_ms;
    
    while ((millis() - testStateStartTime_ms) < (TEST_STATE_DURATION_S * 1000)) {
        uint32_t now = millis();
        
        if (now >= nextSampleTime_ms) {
            takePowerMeasurement();
            Serial.print(F("  ["));
            Serial.print((now - testStateStartTime_ms) / 1000);
            Serial.print(F(" s] Idle measurement taken. Buffer size: "));
            Serial.println(bufferIndex);
            nextSampleTime_ms = now + SENSOR_SAMPLING_INTERVAL_MS;
        }
        
        delay(10);
    }
    
    Serial.print(F("Test State 2 completed. Collected "));
    Serial.print(bufferIndex);
    Serial.println(F(" samples."));
}

void runTest_Sustained_SD_Write() {
    Serial.println(F("=== TEST STATE 3: Sustained SD Write (1Hz) ==="));
    currentTestState = TEST_SUSTAINED_SD_WRITE;
    testStateStartTime_ms = millis();
    
    uint32_t nextSampleTime_ms = testStateStartTime_ms;
    
    while ((millis() - testStateStartTime_ms) < (TEST_STATE_DURATION_S * 1000)) {
        uint32_t now = millis();
        
        if (now >= nextSampleTime_ms) {
            PowerLogEntry entry;
            entry.timestamp_ms = now;
            entry.voltage_V = ina228_readBusVoltage();
            entry.current_mA = ina228_readCurrent();
            entry.power_mW = ina228_readPower();
            logSingleEntryToCSV(entry, getTestStateName(currentTestState));
            
            Serial.print(F("  ["));
            Serial.print((now - testStateStartTime_ms) / 1000);
            Serial.println(F(" s] Sustained write entry logged directly to CSV."));
            nextSampleTime_ms = now + SENSOR_SAMPLING_INTERVAL_MS;
        }
        
        delay(10);
    }
    
    Serial.println(F("Test State 3 completed. All entries written directly to CSV."));
}

void runTest_Periodic_Batch_Write_Cycle() {
    Serial.println(F("=== TEST STATE 4: Periodic Batch Write Cycle ==="));
    currentTestState = TEST_PERIODIC_BATCH_WRITE_CYCLE;
    testStateStartTime_ms = millis();
    bufferIndex = 0;
    
    const uint32_t BATCH_WRITE_INTERVAL_MS = 30000;
    uint32_t nextBatchTime_ms = testStateStartTime_ms + BATCH_WRITE_INTERVAL_MS;
    uint32_t nextSampleTime_ms = testStateStartTime_ms;
    bool sdInitialized = true;
    
    while ((millis() - testStateStartTime_ms) < (TEST_STATE_DURATION_S * 1000)) {
        uint32_t now = millis();
        
        if (now >= nextBatchTime_ms && bufferIndex > 0) {
            if (!sdInitialized) {
                SD.begin(SD_CS_PIN);
                sdInitialized = true;
                Serial.println(F("  SD card re-initialized for batch write."));
            }
            
            Serial.print(F("  Writing "));
            Serial.print(bufferIndex);
            Serial.print(F(" buffered entries to CSV... "));
            appendPowerDataToCSV();
            Serial.println(F("Done."));
            
            SD.end();
            sdInitialized = false;
            Serial.println(F("  SD card de-initialized."));
            
            nextBatchTime_ms = now + BATCH_WRITE_INTERVAL_MS;
        }
        
        if (now >= nextSampleTime_ms) {
            takePowerMeasurement();
            Serial.print(F("  ["));
            Serial.print((now - testStateStartTime_ms) / 1000);
            Serial.print(F(" s] Buffering measurement. Buffer size: "));
            Serial.println(bufferIndex);
            nextSampleTime_ms = now + SENSOR_SAMPLING_INTERVAL_MS;
        }
        
        delay(10);
    }
    
    if (bufferIndex > 0) {
        if (!sdInitialized) {
            SD.begin(SD_CS_PIN);
        }
        appendPowerDataToCSV();
        Serial.print(F("Final batch write completed: "));
        Serial.print(bufferIndex);
        Serial.println(F(" entries."));
    }
    
    Serial.println(F("Test State 4 completed."));
}

// Main Arduino Functions
void setup() {
    if (ENABLE_SERIAL_DEBUG) {
        Serial.begin(115200);
        delay(2000);
        
        Serial.println(F("\n=========================================================="));
        Serial.println(F("    SD CARD POWER CONSUMPTION TEST - ESSENTIAL STATES"));
        Serial.println(F("=========================================================="));
        Serial.println();
        Serial.println(F("Purpose: Measure power consumption across four essential"));
        Serial.println(F("         SD card operational states with precise timing"));
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
        Serial.println(F("=== SD Card Power Test Essentials ==="));
        Serial.println(F("Starting essential power consumption testing..."));
    }
}

void loop() {
    runTest_MCU_Active_SD_Deinitialized();
    
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println(F("ERROR: Could not re-initialize SD card after Test State 1!"));
        while (1) delay(1000);
    }
    appendPowerDataToCSV();
    
    runTest_MCU_Active_SD_Idle_Standby();
    appendPowerDataToCSV();
    
    runTest_Sustained_SD_Write();
    
    runTest_Periodic_Batch_Write_Cycle();
    
    Serial.println(F("\n=== ALL TESTS COMPLETED ==="));
    Serial.print(F("Test Run ID: "));
    Serial.println(testRunID);
    Serial.println(F("Results saved to: test.csv"));
    Serial.println(F("System will halt. Reset to run again."));
    
    while (1) delay(1000);
} 