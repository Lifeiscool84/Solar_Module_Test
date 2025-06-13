/*
 * sensor_validation_template.cpp
 * 
 * Template for validating sensor hardware and communication
 * 
 * This template provides a comprehensive sensor validation framework that:
 * - Tests I2C communication reliability
 * - Validates device identification
 * - Checks data reading functionality
 * - Measures timing and performance
 * - Generates validation reports
 * 
 * Usage:
 * 1. Copy this template for each new sensor
 * 2. Update sensor-specific definitions
 * 3. Implement sensor-specific test functions
 * 4. Run validation before integration
 * 
 * Author: [Your Name]
 * Date: [Date]
 * Version: 1.0
 */

#include <Wire.h>

// ==================== SENSOR CONFIGURATION ====================

// Replace these with actual sensor values
#define SENSOR_NAME "TEMPLATE_SENSOR"
#define SENSOR_I2C_ADDRESS 0x40
#define SENSOR_DEVICE_ID_REG 0x3F
#define SENSOR_EXPECTED_ID 0x2280
#define SENSOR_CONFIG_REG 0x00
#define SENSOR_DATA_REG 0x05

// Test Configuration
#define VALIDATION_SAMPLES 100
#define TIMING_TEST_ITERATIONS 1000
#define STABILITY_TEST_DURATION_MS 60000  // 1 minute

// ==================== VALIDATION RESULTS ====================

struct ValidationResults {
    // Communication Tests
    bool i2c_communication_ok;
    bool device_id_valid;
    uint16_t actual_device_id;
    
    // Data Reading Tests
    bool data_reading_ok;
    uint32_t successful_reads;
    uint32_t failed_reads;
    float success_rate_percent;
    
    // Timing Tests
    float avg_read_time_us;
    float max_read_time_us;
    float min_read_time_us;
    
    // Stability Tests
    bool stability_test_passed;
    uint32_t stability_errors;
    float data_variance;
    
    // Overall Result
    bool validation_passed;
    char summary[256];
};

// ==================== UTILITY FUNCTIONS ====================

bool writeRegister16(uint8_t address, uint8_t reg, uint16_t value) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write((value >> 8) & 0xFF);
    Wire.write(value & 0xFF);
    return (Wire.endTransmission() == 0);
}

uint16_t readRegister16(uint8_t address, uint8_t reg) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return 0xFFFF;  // Error indicator
    }
    
    Wire.requestFrom(address, (uint8_t)2);
    if (Wire.available() >= 2) {
        uint16_t value = ((uint16_t)Wire.read() << 8) | Wire.read();
        return value;
    }
    return 0xFFFF;  // Error indicator
}

float readSensorData(uint8_t address) {
    // Replace with actual sensor data reading implementation
    uint16_t raw = readRegister16(address, SENSOR_DATA_REG);
    if (raw == 0xFFFF) return -999.0;  // Error indicator
    
    // Convert raw data to meaningful value (sensor-specific)
    return (float)raw * 0.001;  // Example conversion
}

// ==================== VALIDATION TEST FUNCTIONS ====================

bool testI2CCommunication(uint8_t address) {
    Serial.print("Testing I2C communication... ");
    
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    
    bool success = (error == 0);
    Serial.println(success ? "PASS" : "FAIL");
    
    if (!success) {
        Serial.print("  I2C Error Code: ");
        Serial.println(error);
    }
    
    return success;
}

bool testDeviceIdentification(uint8_t address, ValidationResults* results) {
    Serial.print("Testing device identification... ");
    
    uint16_t deviceId = readRegister16(address, SENSOR_DEVICE_ID_REG);
    results->actual_device_id = deviceId;
    
    bool valid = (deviceId == SENSOR_EXPECTED_ID);
    Serial.println(valid ? "PASS" : "FAIL");
    
    Serial.print("  Expected ID: 0x");
    Serial.print(SENSOR_EXPECTED_ID, HEX);
    Serial.print(", Actual ID: 0x");
    Serial.println(deviceId, HEX);
    
    return valid;
}

bool testDataReading(uint8_t address, ValidationResults* results) {
    Serial.print("Testing data reading reliability... ");
    
    uint32_t successful = 0;
    uint32_t failed = 0;
    
    for (int i = 0; i < VALIDATION_SAMPLES; i++) {
        float data = readSensorData(address);
        if (data != -999.0) {
            successful++;
        } else {
            failed++;
        }
        
        if (i % 20 == 0) {
            Serial.print(".");
        }
        delay(10);
    }
    
    results->successful_reads = successful;
    results->failed_reads = failed;
    results->success_rate_percent = ((float)successful / VALIDATION_SAMPLES) * 100.0;
    
    bool passed = (results->success_rate_percent >= 95.0);
    Serial.println(passed ? " PASS" : " FAIL");
    
    Serial.print("  Success Rate: ");
    Serial.print(results->success_rate_percent, 1);
    Serial.print("% (");
    Serial.print(successful);
    Serial.print("/");
    Serial.print(VALIDATION_SAMPLES);
    Serial.println(")");
    
    return passed;
}

bool testReadingTiming(uint8_t address, ValidationResults* results) {
    Serial.print("Testing reading timing performance... ");
    
    float totalTime = 0;
    float maxTime = 0;
    float minTime = 999999;
    
    for (int i = 0; i < TIMING_TEST_ITERATIONS; i++) {
        uint32_t startTime = micros();
        float data = readSensorData(address);
        uint32_t endTime = micros();
        
        float readTime = endTime - startTime;
        totalTime += readTime;
        
        if (readTime > maxTime) maxTime = readTime;
        if (readTime < minTime) minTime = readTime;
        
        if (i % 200 == 0) {
            Serial.print(".");
        }
    }
    
    results->avg_read_time_us = totalTime / TIMING_TEST_ITERATIONS;
    results->max_read_time_us = maxTime;
    results->min_read_time_us = minTime;
    
    bool passed = (results->avg_read_time_us < 5000);  // Less than 5ms average
    Serial.println(passed ? " PASS" : " FAIL");
    
    Serial.print("  Average: ");
    Serial.print(results->avg_read_time_us, 1);
    Serial.print("μs, Max: ");
    Serial.print(results->max_read_time_us, 1);
    Serial.print("μs, Min: ");
    Serial.print(results->min_read_time_us, 1);
    Serial.println("μs");
    
    return passed;
}

bool testDataStability(uint8_t address, ValidationResults* results) {
    Serial.print("Testing data stability over time... ");
    
    const int numSamples = 60;  // 1 minute at 1Hz
    float samples[numSamples];
    uint32_t errors = 0;
    uint32_t startTime = millis();
    
    // Collect samples
    for (int i = 0; i < numSamples; i++) {
        float data = readSensorData(address);
        if (data == -999.0) {
            errors++;
            samples[i] = 0;  // Use 0 for failed reads
        } else {
            samples[i] = data;
        }
        
        if (i % 10 == 0) {
            Serial.print(".");
        }
        
        delay(1000);  // 1 second interval
    }
    
    // Calculate variance
    float mean = 0;
    for (int i = 0; i < numSamples; i++) {
        mean += samples[i];
    }
    mean /= numSamples;
    
    float variance = 0;
    for (int i = 0; i < numSamples; i++) {
        variance += (samples[i] - mean) * (samples[i] - mean);
    }
    variance /= numSamples;
    
    results->stability_errors = errors;
    results->data_variance = variance;
    
    bool passed = (errors <= 2) && (variance < 1.0);  // Allow 2 errors max, low variance
    Serial.println(passed ? " PASS" : " FAIL");
    
    Serial.print("  Errors: ");
    Serial.print(errors);
    Serial.print("/");
    Serial.print(numSamples);
    Serial.print(", Variance: ");
    Serial.println(variance, 6);
    
    return passed;
}

// ==================== MAIN VALIDATION FUNCTION ====================

ValidationResults validateSensor(uint8_t address) {
    ValidationResults results = {0};  // Initialize all fields to zero/false
    
    Serial.println("\n==========================================");
    Serial.print("SENSOR VALIDATION: ");
    Serial.println(SENSOR_NAME);
    Serial.print("I2C Address: 0x");
    Serial.println(address, HEX);
    Serial.println("==========================================");
    
    // Test 1: I2C Communication
    Serial.println("\n1. I2C Communication Test");
    results.i2c_communication_ok = testI2CCommunication(address);
    
    if (!results.i2c_communication_ok) {
        strcpy(results.summary, "FAILED: I2C communication error");
        return results;
    }
    
    // Test 2: Device Identification
    Serial.println("\n2. Device Identification Test");
    results.device_id_valid = testDeviceIdentification(address, &results);
    
    // Test 3: Data Reading Reliability
    Serial.println("\n3. Data Reading Reliability Test");
    results.data_reading_ok = testDataReading(address, &results);
    
    // Test 4: Reading Timing Performance
    Serial.println("\n4. Reading Timing Performance Test");
    bool timing_ok = testReadingTiming(address, &results);
    
    // Test 5: Data Stability Over Time
    Serial.println("\n5. Data Stability Test");
    results.stability_test_passed = testDataStability(address, &results);
    
    // Overall Assessment
    results.validation_passed = results.i2c_communication_ok &&
                               results.device_id_valid &&
                               results.data_reading_ok &&
                               timing_ok &&
                               results.stability_test_passed;
    
    // Generate Summary
    if (results.validation_passed) {
        strcpy(results.summary, "PASSED: All validation tests successful");
    } else {
        strcpy(results.summary, "FAILED: One or more validation tests failed");
    }
    
    return results;
}

void printValidationReport(const ValidationResults& results) {
    Serial.println("\n==========================================");
    Serial.println("VALIDATION REPORT");
    Serial.println("==========================================");
    
    Serial.print("Sensor: ");
    Serial.println(SENSOR_NAME);
    Serial.print("I2C Address: 0x");
    Serial.println(SENSOR_I2C_ADDRESS, HEX);
    Serial.print("Overall Result: ");
    Serial.println(results.validation_passed ? "PASS" : "FAIL");
    Serial.println();
    
    Serial.println("Test Results:");
    Serial.print("  I2C Communication: ");
    Serial.println(results.i2c_communication_ok ? "PASS" : "FAIL");
    
    Serial.print("  Device ID: ");
    Serial.print(results.device_id_valid ? "PASS" : "FAIL");
    Serial.print(" (Expected: 0x");
    Serial.print(SENSOR_EXPECTED_ID, HEX);
    Serial.print(", Actual: 0x");
    Serial.print(results.actual_device_id, HEX);
    Serial.println(")");
    
    Serial.print("  Data Reading: ");
    Serial.print(results.data_reading_ok ? "PASS" : "FAIL");
    Serial.print(" (Success Rate: ");
    Serial.print(results.success_rate_percent, 1);
    Serial.println("%)");
    
    Serial.print("  Timing Performance: ");
    Serial.print("Average ");
    Serial.print(results.avg_read_time_us, 1);
    Serial.println("μs");
    
    Serial.print("  Data Stability: ");
    Serial.print(results.stability_test_passed ? "PASS" : "FAIL");
    Serial.print(" (Errors: ");
    Serial.print(results.stability_errors);
    Serial.print(", Variance: ");
    Serial.print(results.data_variance, 6);
    Serial.println(")");
    
    Serial.println();
    Serial.print("Summary: ");
    Serial.println(results.summary);
    Serial.println("==========================================");
}

// ==================== ARDUINO SETUP AND LOOP ====================

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("SENSOR VALIDATION FRAMEWORK");
    Serial.println("===========================");
    Serial.println();
    
    // Initialize I2C
    Wire.begin();
    delay(100);
    
    // Run validation
    ValidationResults results = validateSensor(SENSOR_I2C_ADDRESS);
    
    // Print report
    printValidationReport(results);
    
    // Recommendations
    Serial.println("\nRECOMMENDATIONS:");
    if (results.validation_passed) {
        Serial.println("✓ Sensor is ready for integration");
        Serial.println("✓ Proceed with driver development");
        Serial.println("✓ Add to multi-sensor configuration");
    } else {
        Serial.println("✗ Hardware validation required");
        Serial.println("✗ Check connections and power supply");
        Serial.println("✗ Verify I2C address and pull-up resistors");
        Serial.println("✗ Consult troubleshooting documentation");
    }
}

void loop() {
    // Validation complete - system halted
    delay(10000);
    
    // Optional: Continuous monitoring mode
    // Uncomment below for continuous sensor monitoring
    /*
    static uint32_t lastCheck = 0;
    if (millis() - lastCheck > 10000) {  // Check every 10 seconds
        Serial.println("Monitoring sensor...");
        float data = readSensorData(SENSOR_I2C_ADDRESS);
        Serial.print("Current reading: ");
        Serial.println(data);
        lastCheck = millis();
    }
    */
}