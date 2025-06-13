/**
 * @file gnss_basic_usage.cpp
 * @brief Basic usage example for UBLOX_Driver class
 * 
 * Demonstrates proper initialization, configuration, and data reading
 * from u-blox GNSS modules using the professional driver implementation
 * 
 * Hardware Required:
 * - u-blox GNSS module (ZED-F9P, NEO-M8P, NEO-M9N, etc.)
 * - I2C connection to microcontroller
 * - Clear view of sky for satellite reception
 */

#include <Arduino.h>
#include <Wire.h>
#include "../ublox_driver.h"

// Global GNSS driver instance
UBLOX_Driver gnss_driver;

// Configuration variables
bool gnss_enabled = true;
unsigned long last_position_read = 0;
unsigned long last_status_print = 0;
const unsigned long POSITION_READ_INTERVAL = 5000;   // Read position every 5 seconds
const unsigned long STATUS_PRINT_INTERVAL = 30000;   // Print status every 30 seconds

// Function prototypes
void setup();
void loop();
void initializeGNSS();
void readAndDisplayGNSSData();
void demonstrateGNSSFeatures();
void printMenu();
void handleSerialCommands();

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    Serial.println(F("========================================"));
    Serial.println(F("GNSS Driver Basic Usage Example"));
    Serial.println(F("Extracted from main.txt Implementation"));
    Serial.println(F("========================================"));
    
    // Initialize I2C
    Wire.begin();
    
    // Initialize GNSS
    initializeGNSS();
    
    // Print usage menu
    printMenu();
    
    Serial.println(F("Setup complete. Starting GNSS monitoring..."));
}

void loop() {
    unsigned long current_time = millis();
    
    // Handle serial commands
    handleSerialCommands();
    
    // Read GNSS position periodically
    if (gnss_enabled && (current_time - last_position_read >= POSITION_READ_INTERVAL)) {
        readAndDisplayGNSSData();
        last_position_read = current_time;
    }
    
    // Print status periodically
    if (current_time - last_status_print >= STATUS_PRINT_INTERVAL) {
        Serial.println(F("\n--- Periodic Status Update ---"));
        gnss_driver.printStatus();
        last_status_print = current_time;
    }
    
    delay(100);  // Small delay to prevent overwhelming the serial output
}

void initializeGNSS() {
    Serial.println(F("\n=== GNSS Initialization ==="));
    
    // Create custom configuration (optional)
    GNSSConfig config;
    config.measurement_rate_ms = 1000;      // 1 Hz updates
    config.navigation_rate = 1;             // Every measurement
    config.enable_high_precision = true;    // Enable if supported
    
    // Set validation criteria
    config.validation.min_satellites = 4;   // Minimum 4 satellites
    config.validation.max_hdop = 5.0f;      // Maximum HDOP of 5.0
    config.validation.max_age_ms = 5000;    // Data valid for 5 seconds
    config.validation.require_time_valid = false;  // Don't require time validity
    
    // Update driver configuration
    gnss_driver.updateConfig(config);
    
    // Initialize the GNSS driver
    GNSSStatus status = gnss_driver.initialize();
    
    switch (status) {
        case GNSSStatus::SUCCESS:
            Serial.println(F("✓ GNSS initialization successful"));
            gnss_enabled = true;
            break;
            
        case GNSSStatus::DEVICE_NOT_FOUND:
            Serial.println(F("✗ GNSS device not found - check wiring"));
            gnss_enabled = false;
            break;
            
        case GNSSStatus::COMMUNICATION_ERROR:
            Serial.println(F("✗ GNSS communication error"));
            gnss_enabled = false;
            break;
            
        case GNSSStatus::CONFIGURATION_FAILED:
            Serial.println(F("✗ GNSS configuration failed"));
            gnss_enabled = false;
            break;
            
        default:
            Serial.println(F("✗ Unknown GNSS initialization error"));
            gnss_enabled = false;
            break;
    }
    
    if (gnss_enabled) {
        Serial.println(F("GNSS driver ready for position readings"));
        Serial.print(F("Time since initialization: "));
        Serial.print(gnss_driver.getInitializationTime());
        Serial.println(F(" ms"));
    }
    
    Serial.println(F("===========================\n"));
}

void readAndDisplayGNSSData() {
    if (!gnss_enabled || !gnss_driver.isReady()) {
        Serial.println(F("GNSS not ready for data reading"));
        return;
    }
    
    GNSSData gnss_data;
    
    if (gnss_driver.readPosition(gnss_data)) {
        Serial.println(F("\n--- GNSS Position Reading ---"));
        
        Serial.print(F("Timestamp: ")); Serial.println(gnss_data.last_update_ms);
        Serial.print(F("Fix Status: ")); 
        Serial.println(gnss_data.is_fixed ? F("FIXED") : F("NO FIX"));
        
        Serial.print(F("Validation: ")); 
        Serial.println(gnss_data.validation_passed ? F("PASSED") : F("FAILED"));
        
        Serial.print(F("Satellites Used: ")); Serial.println(gnss_data.satellites_used);
        Serial.print(F("HDOP: ")); Serial.println(gnss_data.hdop, 2);
        Serial.print(F("Fix Type: ")); Serial.println((int)gnss_data.fix_type);
        
        if (gnss_data.position.position_valid) {
            Serial.println(F("--- Position Data ---"));
            Serial.print(F("Latitude:  ")); Serial.println(gnss_data.position.latitude_deg, 7);
            Serial.print(F("Longitude: ")); Serial.println(gnss_data.position.longitude_deg, 7);
            Serial.print(F("Altitude:  ")); Serial.print(gnss_data.position.altitude_m, 2); Serial.println(F(" m"));
            
            Serial.print(F("H.Accuracy: ")); 
            Serial.print(gnss_data.position.horizontal_accuracy_m, 2); 
            Serial.println(F(" m"));
            
            Serial.print(F("V.Accuracy: ")); 
            Serial.print(gnss_data.position.vertical_accuracy_m, 2); 
            Serial.println(F(" m"));
        }
        
        if (gnss_data.time.time_valid) {
            Serial.println(F("--- Time Data ---"));
            Serial.print(F("UTC Time: "));
            Serial.print(gnss_data.time.year); Serial.print(F("/"));
            Serial.print(gnss_data.time.month); Serial.print(F("/"));
            Serial.print(gnss_data.time.day); Serial.print(F(" "));
            Serial.print(gnss_data.time.hour); Serial.print(F(":"));
            Serial.print(gnss_data.time.minute); Serial.print(F(":"));
            Serial.println(gnss_data.time.second);
        }
        
        // Display time to first fix if achieved
        if (gnss_data.time_to_first_fix_ms > 0) {
            Serial.print(F("Time to First Fix: "));
            Serial.print(gnss_data.time_to_first_fix_ms);
            Serial.println(F(" ms"));
        }
        
        Serial.println(F("----------------------------"));
        
    } else {
        Serial.println(F("Failed to read GNSS position data"));
    }
}

void demonstrateGNSSFeatures() {
    if (!gnss_enabled) {
        Serial.println(F("GNSS not available for feature demonstration"));
        return;
    }
    
    Serial.println(F("\n=== GNSS Feature Demonstration ==="));
    
    // 1. Communication test
    Serial.print(F("Communication Test: "));
    bool comm_ok = gnss_driver.checkCommunication(2000);
    Serial.println(comm_ok ? F("PASS") : F("FAIL"));
    
    // 2. Protocol version
    uint8_t ver_high, ver_low;
    if (gnss_driver.getProtocolVersion(ver_high, ver_low)) {
        Serial.print(F("Protocol Version: "));
        Serial.print(ver_high); Serial.print(F("."));
        Serial.println(ver_low);
    }
    
    // 3. Self-test
    Serial.print(F("Self-Test: "));
    bool self_test_ok = gnss_driver.performSelfTest();
    Serial.println(self_test_ok ? F("PASS") : F("FAIL"));
    
    // 4. Satellite information
    uint8_t sats_used, sats_visible;
    float hdop;
    if (gnss_driver.getSatelliteInfo(sats_used, sats_visible, hdop)) {
        Serial.print(F("Satellite Info - Used: "));
        Serial.print(sats_used);
        Serial.print(F(", Visible: "));
        Serial.print(sats_visible);
        Serial.print(F(", HDOP: "));
        Serial.println(hdop, 2);
    }
    
    // 5. Accuracy estimates
    float h_acc, v_acc;
    if (gnss_driver.getAccuracyEstimates(h_acc, v_acc)) {
        Serial.print(F("Current Accuracy - H: "));
        Serial.print(h_acc, 2); Serial.print(F(" m, V: "));
        Serial.print(v_acc, 2); Serial.println(F(" m"));
    }
    
    // 6. High precision support
    Serial.print(F("High Precision Supported: "));
    Serial.println(gnss_driver.supportsHighPrecision() ? F("YES") : F("NO"));
    
    Serial.println(F("================================="));
}

void printMenu() {
    Serial.println(F("\n=== GNSS Driver Commands ==="));
    Serial.println(F("s - Print current status"));
    Serial.println(F("p - Read and print position"));
    Serial.println(F("f - Demonstrate features"));
    Serial.println(F("v - Update validation config"));
    Serial.println(F("r - Reset GNSS module"));
    Serial.println(F("c - Save configuration"));
    Serial.println(F("h - Print this help menu"));
    Serial.println(F("================================"));
}

void handleSerialCommands() {
    if (Serial.available()) {
        char command = Serial.read();
        Serial.read(); // consume newline
        
        switch (command) {
            case 's':
            case 'S':
                Serial.println(F("\n=== Current Status ==="));
                gnss_driver.printStatus();
                break;
                
            case 'p':
            case 'P':
                Serial.println(F("\n=== Manual Position Read ==="));
                readAndDisplayGNSSData();
                break;
                
            case 'f':
            case 'F':
                demonstrateGNSSFeatures();
                break;
                
            case 'v':
            case 'V': {
                Serial.println(F("\n=== Update Validation Config ==="));
                GNSSValidationConfig new_validation;
                new_validation.min_satellites = 6;     // Higher requirement
                new_validation.max_hdop = 3.0f;        // Stricter HDOP
                new_validation.max_age_ms = 3000;      // Fresher data
                new_validation.require_time_valid = true;
                
                gnss_driver.updateValidationConfig(new_validation);
                Serial.println(F("Updated validation criteria (stricter)"));
                break;
            }
            
            case 'r':
            case 'R':
                Serial.println(F("\n=== Factory Reset ==="));
                Serial.println(F("Performing factory reset..."));
                if (gnss_driver.factoryReset()) {
                    Serial.println(F("Factory reset successful"));
                } else {
                    Serial.println(F("Factory reset failed"));
                }
                break;
                
            case 'c':
            case 'C':
                Serial.println(F("\n=== Save Configuration ==="));
                if (gnss_driver.saveConfiguration()) {
                    Serial.println(F("Configuration saved to GNSS module"));
                } else {
                    Serial.println(F("Failed to save configuration"));
                }
                break;
                
            case 'h':
            case 'H':
                printMenu();
                break;
                
            default:
                Serial.println(F("Unknown command. Type 'h' for help."));
                break;
        }
    }
} 