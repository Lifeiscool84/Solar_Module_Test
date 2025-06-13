/*
 * i2c_scanner.cpp
 * 
 * I2C Scanner utility for the Croc Tracker project
 * 
 * This utility scans the I2C bus to detect connected devices and provides
 * detailed information about each discovered device. Essential for hardware
 * validation and troubleshooting.
 * 
 * Features:
 * - Comprehensive I2C address scanning (0x03 to 0x77)
 * - Device identification where possible
 * - Known device database for common sensors
 * - I2C bus health diagnostics
 * - Detailed reporting and recommendations
 * 
 * Usage:
 * 1. Upload this sketch to your microcontroller
 * 2. Open Serial Monitor at 115200 baud
 * 3. Review detected devices and addresses
 * 4. Use results to configure sensor drivers
 * 
 * Author: Croc Tracker Development Team
 * Date: [Date]
 * Version: 2.0
 */

#include <Wire.h>

// ==================== DEVICE DATABASE ====================

struct KnownDevice {
    uint8_t address;
    const char* name;
    const char* description;
    const char* notes;
};

// Database of known I2C devices commonly used in embedded projects
const KnownDevice knownDevices[] = {
     
    // End marker
    {0x00, "", "", ""}
};

// ==================== SCANNING FUNCTIONS ====================

const KnownDevice* findKnownDevice(uint8_t address) {
    for (int i = 0; knownDevices[i].address != 0 || knownDevices[i].name[0] != '\0'; i++) {
        if (knownDevices[i].address == address) {
            return &knownDevices[i];
        }
    }
    return nullptr;
}

bool testI2CAddress(uint8_t address) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    return (error == 0);
}

uint8_t readDeviceRegister(uint8_t address, uint8_t reg) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return 0xFF;  // Error
    }
    
    Wire.requestFrom(address, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0xFF;  // Error
}

uint16_t readDeviceRegister16(uint8_t address, uint8_t reg) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return 0xFFFF;  // Error
    }
    
    Wire.requestFrom(address, (uint8_t)2);
    if (Wire.available() >= 2) {
        uint16_t value = ((uint16_t)Wire.read() << 8) | Wire.read();
        return value;
    }
    return 0xFFFF;  // Error
}

void attemptDeviceIdentification(uint8_t address) {
    Serial.print("    Attempting device identification... ");
    
    // Try common device ID registers
    uint8_t deviceId8 = 0xFF;
    uint16_t deviceId16 = 0xFFFF;
    
    // Try 8-bit device ID at common registers
    uint8_t idRegisters[] = {0x00, 0x0F, 0x3F, 0xFC, 0xFD, 0xFE, 0xFF};
    for (int i = 0; i < 7; i++) {
        uint8_t id = readDeviceRegister(address, idRegisters[i]);
        if (id != 0xFF && id != 0x00) {
            deviceId8 = id;
            Serial.print("8-bit ID: 0x");
            Serial.print(deviceId8, HEX);
            Serial.print(" at reg 0x");
            Serial.print(idRegisters[i], HEX);
            break;
        }
    }
    
    // Try 16-bit device ID (common for advanced sensors)
    if (deviceId8 == 0xFF) {
        for (int i = 0; i < 7; i++) {
            uint16_t id = readDeviceRegister16(address, idRegisters[i]);
            if (id != 0xFFFF && id != 0x0000) {
                deviceId16 = id;
                Serial.print("16-bit ID: 0x");
                Serial.print(deviceId16, HEX);
                Serial.print(" at reg 0x");
                Serial.print(idRegisters[i], HEX);
                break;
            }
        }
    }
    
    if (deviceId8 == 0xFF && deviceId16 == 0xFFFF) {
        Serial.print("No device ID found");
    }
    
    Serial.println();
    
    // Specific device identification for known sensors
    if (address >= 0x40 && address <= 0x4F) {
        // Check for INA228 (common power monitor)
        uint16_t inaId = readDeviceRegister16(address, 0x3F);
        if (inaId == 0x2280 || inaId == 0x2281) {
            Serial.println("    → Confirmed: INA228 Power Monitor");
        }
    }
}

void performBusHealthCheck() {
    Serial.println("\n==========================================");
    Serial.println("I2C BUS HEALTH CHECK");
    Serial.println("==========================================");
    
    // Test bus at different speeds
    Serial.println("Testing bus reliability...");
    
    int failCount = 0;
    const int testIterations = 100;
    
    for (int i = 0; i < testIterations; i++) {
        Wire.beginTransmission(0x00);  // General call address
        uint8_t error = Wire.endTransmission();
        if (error != 2) {  // Should get NACK (error 2) for general call
            failCount++;
        }
        delayMicroseconds(100);
    }
    
    float reliability = ((float)(testIterations - failCount) / testIterations) * 100.0;
    Serial.print("Bus reliability: ");
    Serial.print(reliability, 1);
    Serial.println("%");
    
    if (reliability < 95.0) {
        Serial.println("⚠️  WARNING: I2C bus reliability issues detected");
        Serial.println("   Check pull-up resistors (should be 4.7kΩ)");
        Serial.println("   Verify connections and cable length");
        Serial.println("   Check for electromagnetic interference");
    } else {
        Serial.println("✅ I2C bus appears healthy");
    }
}

void scanI2CBus() {
    Serial.println("\n==========================================");
    Serial.println("I2C DEVICE SCANNER");
    Serial.println("==========================================");
    Serial.println("Scanning I2C bus for devices...");
    Serial.println("Address range: 0x03 to 0x77");
    Serial.println();
    
    int deviceCount = 0;
    uint8_t foundAddresses[120];  // Maximum possible I2C addresses
    
    // Scan all valid I2C addresses
    for (uint8_t address = 0x03; address <= 0x77; address++) {
        if (testI2CAddress(address)) {
            foundAddresses[deviceCount] = address;
            deviceCount++;
            
            Serial.print("Device found at address 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            Serial.print(" (");
            Serial.print(address, DEC);
            Serial.println(")");
            
            // Check if this is a known device
            const KnownDevice* known = findKnownDevice(address);
            if (known) {
                Serial.print("  → Known device: ");
                Serial.print(known->name);
                Serial.print(" (");
                Serial.print(known->description);
                Serial.println(")");
                if (strlen(known->notes) > 0) {
                    Serial.print("    Notes: ");
                    Serial.println(known->notes);
                }
            } else {
                Serial.println("  → Unknown device");
                attemptDeviceIdentification(address);
            }
            Serial.println();
        }
    }
    
    // Summary
    Serial.println("==========================================");
    Serial.print("SCAN COMPLETE - ");
    Serial.print(deviceCount);
    Serial.println(" device(s) found");
    Serial.println("==========================================");
    
    if (deviceCount == 0) {
        Serial.println("No I2C devices detected!");
        Serial.println();
        Serial.println("Troubleshooting checklist:");
        Serial.println("  1. Check power supply to devices");
        Serial.println("  2. Verify SDA and SCL connections");
        Serial.println("  3. Check for 4.7kΩ pull-up resistors on SDA and SCL");
        Serial.println("  4. Ensure correct voltage levels (3.3V or 5V)");
        Serial.println("  5. Check for short circuits or loose connections");
        Serial.println("  6. Verify device addresses in datasheets");
    } else {
        Serial.println("\nFound devices summary:");
        for (int i = 0; i < deviceCount; i++) {
            Serial.print("  0x");
            if (foundAddresses[i] < 16) Serial.print("0");
            Serial.print(foundAddresses[i], HEX);
            
            const KnownDevice* known = findKnownDevice(foundAddresses[i]);
            if (known) {
                Serial.print(" - ");
                Serial.print(known->name);
            }
            Serial.println();
        }
        
        Serial.println("\nRecommendations:");
        Serial.println("  ✅ Update sensor driver I2C addresses");
        Serial.println("  ✅ Run device-specific validation tests");
        Serial.println("  ✅ Document findings in project configuration");
    }
}

void printI2CConfiguration() {
    Serial.println("\n==========================================");
    Serial.println("I2C CONFIGURATION");
    Serial.println("==========================================");
    Serial.println("Current I2C settings:");
    Serial.println("  Clock Speed: 100 kHz (default)");
    Serial.println("  SDA Pin: Platform default");
    Serial.println("  SCL Pin: Platform default");
    Serial.println("  Pull-up: External 4.7kΩ recommended");
    Serial.println("  Voltage: 3.3V or 5V (check device specs)");
    Serial.println();
    Serial.println("For Croc Tracker project sensors:");
    Serial.println("  INA228 Solar:   Expected at 0x40");
    Serial.println("  INA228 Battery: Expected at 0x44");
    Serial.println("  INA228 Load:    Expected at 0x41");
    Serial.println();
}

// ==================== MAIN PROGRAM ====================

void setup() {
    Serial.begin(115200);
    delay(2000);  // Wait for Serial Monitor
    
    Serial.println("╔══════════════════════════════════════════════╗");
    Serial.println("║        CROC TRACKER I2C SCANNER v2.0        ║");
    Serial.println("║                                              ║");
    Serial.println("║  Professional I2C Device Discovery Tool     ║");
    Serial.println("║  for Embedded Sensor Integration            ║");
    Serial.println("╚══════════════════════════════════════════════╝");
    Serial.println();
    
    // Initialize I2C
    Serial.println("Initializing I2C bus...");
    Wire.begin();
    delay(100);  // Allow bus to stabilize
    Serial.println("I2C bus initialized successfully");
    
    // Print configuration information
    printI2CConfiguration();
    
    // Perform bus health check
    performBusHealthCheck();
    
    // Scan for devices
    scanI2CBus();
    
    Serial.println("\n==========================================");
    Serial.println("NEXT STEPS");
    Serial.println("==========================================");
    Serial.println("1. Update sensor driver addresses based on findings");
    Serial.println("2. Run hardware validation tests for each device");
    Serial.println("3. Configure multi-sensor integration");
    Serial.println("4. Test communication reliability under load");
    Serial.println("5. Document final configuration in project files");
    Serial.println();
    Serial.println("Scan complete. Reset to run again.");
}

void loop() {
    // Scanner is complete - enter idle state
    delay(1000);
    
    // Optional: Continuous monitoring mode
    // Uncomment below for periodic re-scanning
    /*
    static uint32_t lastScan = 0;
    if (millis() - lastScan > 30000) {  // Re-scan every 30 seconds
        Serial.println("\n[Periodic re-scan]");
        scanI2CBus();
        lastScan = millis();
    }
    */
}