/**
 * BluetoothTestProtocol.cpp
 * 
 * Generic BLE test protocol framework for sensor testing and data collection
 * Based on the Final_DEMO_Maggie.cpp Bluetooth implementation
 * 
 * This file provides a reusable framework for testing various sensors via Bluetooth
 * without requiring direct serial connection to a PC.
 * 
 * Features:
 * - Simple menu-based BLE interface
 * - File operations on SD card (list, send, delete)
 * - Extensible command structure for sensor-specific commands
 * - Data collection and storage to SD card
 * 
 * To adapt for a specific sensor:
 * 1. Add sensor-specific includes and configuration
 * 2. Implement the initializeSensor() function
 * 3. Implement the collectSensorData() function 
 * 4. Add sensor-specific commands to handleCommand() function
 */

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoBLE.h>
#include <Arduino.h>

// Constants for BLE
#define SERVICE_UUID "1101"
#define CHARACTERISTIC_UUID "2101"
#define CHARACTERISTIC_MAX_LENGTH 100
#define DELAY_BETWEEN_PACKETS 50
#define DELAY_BETWEEN_ACK_CHECKS 10
#define ACK_MESSAGE "ACK"
#define SD_CS_PIN 8  // SD card chip select pin

// File operation constants
#define FILE_EXTENSION ".txt"

// Global variables
BLEService testService(SERVICE_UUID);
BLEStringCharacteristic testCharacteristic(CHARACTERISTIC_UUID, BLERead | BLEWrite | BLENotify | BLEIndicate, CHARACTERISTIC_MAX_LENGTH);
File dataFile;
String currentFileName = "";
bool isCollectingData = false;
unsigned long dataCollectionStartTime = 0;
unsigned long dataCollectionDuration = 5 * 60 * 1000; // Default 5 minutes (configurable via BLE)
unsigned long lastDataUpdateTime = 0;
unsigned long dataUpdateInterval = 1000; // Default 1 second between data points (configurable via BLE)

// Function prototypes
void handleClient(BLEDevice central);
void listFiles(BLEDevice central);
void sendFile(BLEDevice central, String fileName);
void waitForAck(BLEDevice central);
void collectSensorData();
void initializeSensor();
bool handleCommand(String command);
void rotateDataFile();
void createNewDataFile();
String formatTimeValue(int value);

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    
    // Initialize I2C
    Wire.begin();
    Wire.setClock(400000);
    
    // Initialize BLE
    if (!BLE.begin()) {
        Serial.println("Starting BLE failed!");
        while (1);
    }
    
    // Initialize SD card
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("Starting SD failed!");
        while (1);
    }
    
    // Initialize the sensor
    initializeSensor();
    
    // Set up the BLE characteristics
    BLE.setLocalName("SensorTestUnit");
    BLE.setAdvertisedService(testService);
    testService.addCharacteristic(testCharacteristic);
    BLE.addService(testService);
    BLE.advertise();
    
    Serial.println("Bluetooth device active, waiting for connections...");
}

void loop() {
    BLEDevice central = BLE.central();
    
    if (central) {
        handleClient(central);
    } else if (isCollectingData) {
        // Only collect data when not connected and data collection is active
        unsigned long currentTime = millis();
        if (currentTime - lastDataUpdateTime >= dataUpdateInterval) {
            lastDataUpdateTime = currentTime;
            collectSensorData();
            rotateDataFile();
        }
    }
}

void handleClient(BLEDevice central) {
    // Close any open file when a client connects
    if (dataFile) {
        dataFile.close();
    }
    
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    
    isCollectingData = false; // Pause data collection during connection
    
    // Print the menu options
    String menuOptions = 
        "Menu Options:\n"
        "L - List Files\n"
        "D<filename> - Delete File\n"
        "S - Start Data Collection\n"
        "C - Clear SD Card\n"
        "R - Reboot System\n"
        "DT<minutes> - Set Data Collection Duration\n"
        "UI<milliseconds> - Set Update Interval\n";
    
    Serial.println(menuOptions);
    
    while (central.connected()) {
        if (testCharacteristic.written()) {
            String command = testCharacteristic.value();
            Serial.print("Received command: ");
            Serial.println(command);
            
            if (command == "L") {
                listFiles(central);
            } else if (command.startsWith("D") && command.length() > 1) {
                String filename = command.substring(1);
                if (SD.remove(filename)) {
                    testCharacteristic.writeValue("File deleted: " + filename);
                } else {
                    testCharacteristic.writeValue("Error deleting file");
                }
            } else if (command == "S") {
                createNewDataFile();
                isCollectingData = true;
                dataCollectionStartTime = millis();
                testCharacteristic.writeValue("Data collection started: " + currentFileName);
            } else if (command == "C") {
                // Clear the SD card
                File root = SD.open("/");
                while (true) {
                    File entry = root.openNextFile();
                    if (!entry) break;
                    
                    if (!entry.isDirectory()) {
                        SD.remove(entry.name());
                    }
                    entry.close();
                }
                root.close();
                testCharacteristic.writeValue("SD card cleared");
            } else if (command == "R") {
                testCharacteristic.writeValue("Rebooting...");
                delay(500);
                // Implement reboot command for your specific board
                NVIC_SystemReset(); // For Artemis/Apollo boards
                // For other boards you might use different reset methods
            } else if (command.startsWith("DT")) {
                // Set data collection duration in minutes
                int minutes = command.substring(2).toInt();
                if (minutes > 0) {
                    dataCollectionDuration = minutes * 60 * 1000;
                    testCharacteristic.writeValue("Data collection duration set to " + String(minutes) + " minutes");
                }
            } else if (command.startsWith("UI")) {
                // Set update interval in milliseconds
                int interval = command.substring(2).toInt();
                if (interval >= 10) { // Ensure reasonable lower limit
                    dataUpdateInterval = interval;
                    testCharacteristic.writeValue("Update interval set to " + String(interval) + " ms");
                }
            } else if (handleCommand(command)) {
                // Custom command handled by the derived implementation
            } else {
                // Assume it's a filename to read
                sendFile(central, command);
            }
        }
    }
    
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
}

void listFiles(BLEDevice central) {
    File root = SD.open("/");
    if (!root) {
        testCharacteristic.writeValue("Failed to open root directory");
        return;
    }
    
    File entry;
    while ((entry = root.openNextFile()) && testCharacteristic.subscribed()) {
        if (!entry.isDirectory()) {
            String filename = entry.name();
            String fileInfo = filename + " (" + String(entry.size()) + " bytes)";
            
            if (testCharacteristic.writeValue(fileInfo) == 0) {
                Serial.println("Write to characteristic failed");
            }
            delay(DELAY_BETWEEN_PACKETS);
            waitForAck(central);
        }
        entry.close();
    }
    root.close();
    testCharacteristic.writeValue("End of file list");
}

void sendFile(BLEDevice central, String fileName) {
    File file = SD.open(fileName.c_str());
    if (!file) {
        testCharacteristic.writeValue("Failed to open file: " + fileName);
        return;
    }
    
    // Send file size information first
    testCharacteristic.writeValue("File: " + fileName + " (" + String(file.size()) + " bytes)");
    delay(DELAY_BETWEEN_PACKETS);
    waitForAck(central);
    
    // Send file contents in chunks
    while (file.available() && testCharacteristic.subscribed()) {
        String data = "";
        for (int i = 0; i < 20 && file.available(); i++) {
            char c = file.read();
            data += c;
        }
        
        if (testCharacteristic.writeValue(data) == 0) {
            Serial.println("Write to characteristic failed");
        }
        delay(DELAY_BETWEEN_PACKETS);
        waitForAck(central);
    }
    
    file.close();
    testCharacteristic.writeValue("End of file");
}

void waitForAck(BLEDevice central) {
    unsigned long startTime = millis();
    while (central.connected() && (millis() - startTime < 2000)) {
        if (testCharacteristic.written() && testCharacteristic.value() == ACK_MESSAGE) {
            return;
        }
        delay(DELAY_BETWEEN_ACK_CHECKS);
    }
    // If we reach here, we timed out waiting for ACK
    Serial.println("Timed out waiting for ACK");
}

void createNewDataFile() {
    // Create a filename based on the current time
    // This implementation should be modified to use a real-time clock if available
    
    // Simple incrementing filename for example
    static int fileCounter = 0;
    currentFileName = "data_" + String(fileCounter++) + FILE_EXTENSION;
    
    // Open the file for writing
    dataFile = SD.open(currentFileName, FILE_WRITE);
    if (!dataFile) {
        Serial.println("Failed to create data file");
    } else {
        // Write a header to the file
        dataFile.println("Timestamp,SensorData");
        dataFile.flush();
    }
}

void rotateDataFile() {
    // Check if the current time is past the data collection start time plus the duration
    if (millis() - dataCollectionStartTime >= dataCollectionDuration) {
        if (dataFile) {
            dataFile.close();
        }
        
        createNewDataFile();
        dataCollectionStartTime = millis();
    }
}

String formatTimeValue(int value) {
    if (value < 10) {
        return "0" + String(value);
    } else {
        return String(value);
    }
}

// ===== IMPLEMENT THESE FUNCTIONS FOR YOUR SPECIFIC SENSOR =====

void initializeSensor() {
    // Implement initialization for your specific sensor
    // Example:
    // if (!sensor.begin()) {
    //     Serial.println("Failed to initialize sensor");
    //     while(1);
    // }
    
    Serial.println("Sensor initialized");
}

void collectSensorData() {
    // Implement data collection for your specific sensor
    if (!dataFile) {
        return;
    }
    
    // Example:
    // float value = sensor.readValue();
    // dataFile.print(millis());
    // dataFile.print(",");
    // dataFile.println(value);
    // dataFile.flush();
    
    // For this example, we'll just write a timestamp and random value
    dataFile.print(millis());
    dataFile.print(",");
    dataFile.println(random(100));
    dataFile.flush();
}

bool handleCommand(String command) {
    // Implement custom commands for your specific sensor
    // Return true if command was handled, false otherwise
    
    // Example:
    // if (command == "CALIBRATE") {
    //     sensor.calibrate();
    //     testCharacteristic.writeValue("Sensor calibrated");
    //     return true;
    // }
    
    return false; // Command not handled
} 