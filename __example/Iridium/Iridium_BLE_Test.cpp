/**
 * Iridium_BLE_Test.cpp
 * 
 * Bluetooth test protocol implementation for Iridium satellite communication
 * Based on the generic BluetoothTestProtocol.cpp framework
 * 
 * This example demonstrates how to adapt the generic Bluetooth test protocol
 * for Iridium SBD satellite communication testing.
 */

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoBLE.h>
#include <Arduino.h>
#include <IridiumSBD.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>

// For Artemis/Apollo boards reset function
#include "am_mcu_apollo.h"

// Constants for BLE
#define SERVICE_UUID "1102"
#define CHARACTERISTIC_UUID "2102"
#define CHARACTERISTIC_MAX_LENGTH 100
#define DELAY_BETWEEN_PACKETS 50
#define DELAY_BETWEEN_ACK_CHECKS 10
#define ACK_MESSAGE "ACK"
#define SD_CS_PIN 8  // SD card chip select pin
#define DIAGNOSTICS false

// File operation constants
#define FILE_EXTENSION ".txt"
#define DATA_FILENAME "IRID_DAT.TXT"  // Follows 8.3 convention: 8 char name, 3 char extension
#define MAX_BUFFER_SIZE 95 // Maximum buffer size in bytes for Iridium transmission

// Debug and timeout constants
#define MODEM_CONNECT_TIMEOUT 60000  // 60 seconds timeout for modem initialization
#define CONNECTION_RETRIES 3         // Number of retries for satellite connection
#define MIN_SIGNAL_QUALITY 2         // Minimum signal quality required (0-5 scale)

// Define IridiumSBD error codes if not defined by the library
#define ISBD_SUCCESS             0
#define ISBD_ALREADY_AWAKE       1
#define ISBD_SERIAL_FAILURE      2
#define ISBD_PROTOCOL_ERROR      3
#define ISBD_CANCELLED           4
#define ISBD_NO_MODEM_DETECTED   5
#define ISBD_SBDIX_FATAL_ERROR   6
#define ISBD_SENDRECEIVE_TIMEOUT 7
#define ISBD_RX_OVERFLOW         8
#define ISBD_REENTRANT           9
#define ISBD_IS_ASLEEP           10
#define ISBD_NO_SLEEP_PIN        11
#define ISBD_NO_NETWORK          12
#define ISBD_MSG_TOO_LONG        13

// Global variables
BLEService testService(SERVICE_UUID);
BLEStringCharacteristic testCharacteristic(CHARACTERISTIC_UUID, BLERead | BLEWrite | BLENotify | BLEIndicate, CHARACTERISTIC_MAX_LENGTH);
File dataFile;
String currentFileName = DATA_FILENAME;
bool isCollectingData = false;
unsigned long dataCollectionStartTime = 0;
unsigned long dataCollectionDuration = 5 * 60 * 1000; // Default 5 minutes (configurable via BLE)
unsigned long lastDataUpdateTime = 0;
unsigned long dataUpdateInterval = 10000; // Default 10 seconds between data points (configurable via BLE)

// Iridium and GNSS specific variables
IridiumSBD modem(Wire);
SFE_UBLOX_GNSS myGNSS;
int signalQuality = -1;
unsigned int readingCount = 0;
char formattedLat[8];
char formattedLon[9];
String finalLatStr;
String finalLonStr;

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
void formatLatLon();
bool transmitViaIridium(const uint8_t* data, size_t dataSize);
size_t getFileSize(const char* filename);
void shiftFileContents(const char* filename, size_t bytesToRemove);

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    
    // Initialize I2C buses
    Wire.begin();
    Wire1.begin();
    Wire.setClock(400000);
    Wire1.setClock(400000);
    
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
    BLE.setLocalName("IridiumTestUnit");
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
        "Iridium Test Menu:\n"
        "L - List Files\n"
        "D<filename> - Delete File\n"
        "S - Start Data Collection\n"
        "C - Clear SD Card\n"
        "R - Reboot System\n"
        "DT<minutes> - Set Data Collection Duration\n"
        "UI<milliseconds> - Set Update Interval\n"
        "SQ - Check Signal Quality\n"
        "T - Transmit Data File via Iridium\n"
        "P<message> - Send Quick Message via Iridium\n";
    
    Serial.println(menuOptions);
    testCharacteristic.writeValue(menuOptions);
    
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
                am_hal_reset_control(AM_HAL_RESET_CONTROL_TPIU_RESET, 0); // For Artemis/Apollo boards
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
                if (interval >= 1000) { // Ensure reasonable lower limit for Iridium testing
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
    // Open the data file for writing
    dataFile = SD.open(currentFileName, FILE_WRITE);
    if (!dataFile) {
        Serial.println("Failed to create data file");
    } else {
        // Write a header to the file
        dataFile.println("Latitude,Longitude,Date,Time");
        dataFile.flush();
    }
}

void rotateDataFile() {
    // Ensure we don't have too large a file for Iridium transmission
    if (getFileSize(currentFileName.c_str()) > MAX_BUFFER_SIZE * 2) {
        if (dataFile) {
            dataFile.close();
        }
        
        // Create a backup of the current file with timestamp
        // Use a shorter naming format that's 8.3 compatible
        unsigned long timestamp = millis() / 1000; // Use seconds for shorter value
        String backupName = "IR" + String(timestamp % 100000).substring(0, 5) + FILE_EXTENSION;
        
        // Rename by copying (since SD library doesn't have rename)
        File sourceFile = SD.open(currentFileName, FILE_READ);
        File destFile = SD.open(backupName, FILE_WRITE);
        
        if (sourceFile && destFile) {
            while (sourceFile.available()) {
                destFile.write(sourceFile.read());
            }
            sourceFile.close();
            destFile.close();
            
            // Remove original and create a new one
            SD.remove(currentFileName);
            createNewDataFile();
        }
    }
}

String formatTimeValue(int value) {
    if (value < 10) {
        return "0" + String(value);
    } else {
        return String(value);
    }
}

// ===== IRIDIUM-SPECIFIC FUNCTIONS =====

void initializeSensor() {
    // Initialize GNSS
    Serial.println("Initializing GNSS module...");
    if (!myGNSS.begin()) {
        Serial.println("GNSS not detected. Check wiring!");
        testCharacteristic.writeValue("ERROR: GNSS not detected. Check wiring!");
        return;
    }
    Serial.println("GNSS initialized successfully");
    testCharacteristic.writeValue("GNSS initialized successfully");
    
    // Configure GNSS
    myGNSS.setI2COutput(COM_TYPE_UBX);
    
    // Initialize Iridium modem
    Serial.println("Checking Iridium modem connection...");
    if (!modem.isConnected()) {
        Serial.println("Iridium not connected! Check wiring.");
        testCharacteristic.writeValue("ERROR: Iridium not connected! Check wiring.");
        return;
    }
    Serial.println("Iridium modem connected");
    testCharacteristic.writeValue("Iridium modem connected");
    
    // Handle supercapacitor charging
    Serial.println("Charging supercapacitors...");
    modem.enableSuperCapCharger(true);
    int chargeAttempts = 0;
    while (!modem.checkSuperCapCharger() && chargeAttempts < 10) {
        Serial.println("Charging supercapacitors...");
        delay(1000);
        chargeAttempts++;
    }
    
    if (chargeAttempts >= 10) {
        Serial.println("Warning: Supercapacitors may not be fully charged");
        testCharacteristic.writeValue("Warning: Supercapacitors may not be fully charged");
    } else {
        Serial.println("Supercapacitors charged");
        testCharacteristic.writeValue("Supercapacitors charged");
    }
    
    // Initialize modem with timeout
    Serial.println("Initializing Iridium modem...");
    testCharacteristic.writeValue("Initializing Iridium modem... (may take time)");
    modem.enable9603Npower(true);
    modem.setPowerProfile(IridiumSBD::USB_POWER_PROFILE);
    
    // Add timeout mechanism for modem initialization
    unsigned long startTime = millis();
    Serial.println("Attempting to begin Iridium modem communication...");
    testCharacteristic.writeValue("Connecting to Iridium network...");
    
    int err = ISBD_SENDRECEIVE_TIMEOUT; // Default error code
    
    // Use timeout for satellite connection attempts
    while ((millis() - startTime) < MODEM_CONNECT_TIMEOUT) {
        err = modem.begin();
        if (err == ISBD_SUCCESS) {
            break;
        }
        
        // Check if we're likely indoors
        int signalQuality = -1;
        modem.getSignalQuality(signalQuality);
        
        if (signalQuality == 0) {
            Serial.println("No signal detected. If indoors, please move device outside for satellite connection.");
            testCharacteristic.writeValue("No signal. Move device outdoors for satellite connection.");
            delay(5000); // Wait before retry
        } else {
            Serial.print("Low signal quality: ");
            Serial.println(signalQuality);
            testCharacteristic.writeValue("Low signal quality: " + String(signalQuality) + "/5. Move to location with clear sky view.");
            delay(5000); // Wait before retry
        }
        
        Serial.print("Modem begin failed: ");
        Serial.print(err);
        Serial.print(" - Retrying... (");
        Serial.print((millis() - startTime) / 1000);
        Serial.println(" seconds elapsed)");
    }
    
    if (err != ISBD_SUCCESS) {
        Serial.print("Modem begin failed after timeout: ");
        Serial.println(err);
        testCharacteristic.writeValue("ERROR: Iridium connection failed. If indoors, move outside for satellite connection.");
        return;
    }
    
    // Check signal quality now that we're connected
    int signalQuality = -1;
    err = modem.getSignalQuality(signalQuality);
    if (err != ISBD_SUCCESS) {
        Serial.println("Signal quality check failed");
        testCharacteristic.writeValue("Signal quality check failed");
    } else {
        Serial.print("Signal quality: ");
        Serial.print(signalQuality);
        Serial.println("/5");
        
        if (signalQuality < MIN_SIGNAL_QUALITY) {
            Serial.println("WARNING: Signal quality too low for reliable transmission.");
            Serial.println("For better signal: 1) Move outdoors 2) Ensure antenna has clear view of sky");
            testCharacteristic.writeValue("Warning: Low signal (" + String(signalQuality) + "/5). Move outdoors with clear sky view.");
        } else {
            testCharacteristic.writeValue("Signal quality: " + String(signalQuality) + "/5");
        }
    }
    
    Serial.println("Iridium modem and GNSS initialized successfully");
    testCharacteristic.writeValue("System ready! Iridium and GNSS initialized.");
}

void collectSensorData() {
    // Ensure the dataFile is open
    if (!dataFile) {
        createNewDataFile();
    }
    
    // Get GNSS data
    myGNSS.checkUblox();
    if (myGNSS.getGnssFixOk()) {
        readingCount++;
        
        // Format latitude and longitude
        formatLatLon();
        
        // Format date and time
        int year = myGNSS.getYear();
        int twoDigitYear = year % 100;
        char formattedYear[3];
        snprintf(formattedYear, sizeof(formattedYear), "%02d", twoDigitYear);
        
        // Create data record
        String dataRecord = finalLatStr +
                           "," +
                           finalLonStr +
                           "," +
                           String(formattedYear) +
                           formatTimeValue(myGNSS.getMonth()) +
                           formatTimeValue(myGNSS.getDay()) +
                           "," +
                           formatTimeValue(myGNSS.getHour()) +
                           formatTimeValue(myGNSS.getMinute()) +
                           formatTimeValue(myGNSS.getSecond()) +
                           "\n";
        
        // Write to SD card
        dataFile.print(dataRecord);
        dataFile.flush();
        
        Serial.print("Data point recorded: ");
        Serial.print(dataRecord);
        
        // Rotate file if needed
        rotateDataFile();
    } else {
        Serial.println("No GNSS fix - data not logged");
    }
}

bool handleCommand(String command) {
    // Handle Iridium-specific commands
    if (command == "SQ") {
        // Check signal quality with debug info
        testCharacteristic.writeValue("Checking Iridium signal quality...");
        int signalQuality = -1;
        int err = modem.getSignalQuality(signalQuality);
        if (err == ISBD_SUCCESS) {
            String qualityMsg = "Signal quality: " + String(signalQuality) + "/5";
            
            // Add helpful troubleshooting advice based on signal strength
            if (signalQuality == 0) {
                qualityMsg += " - No signal detected. If indoors, move outside for satellite connection.";
            } else if (signalQuality < MIN_SIGNAL_QUALITY) {
                qualityMsg += " - Low signal. For better results: 1) Move outdoors 2) Ensure antenna has clear view of sky";
            } else {
                qualityMsg += " - Good signal. Transmission should be reliable.";
            }
            
            Serial.println(qualityMsg);
            testCharacteristic.writeValue(qualityMsg);
            return true;
        } else {
            String errorMsg = "Signal check failed: " + String(err);
            if (err == ISBD_SENDRECEIVE_TIMEOUT) {
                errorMsg += " - Connection timed out. If indoors, move outside.";
            } else if (err == ISBD_PROTOCOL_ERROR) {
                errorMsg += " - Protocol error. Check modem connection.";
            }
            
            Serial.println(errorMsg);
            testCharacteristic.writeValue(errorMsg);
            return true;
        }
    } else if (command == "T") {
        // Transmit the current data file via Iridium
        testCharacteristic.writeValue("Preparing to transmit data via Iridium...");
        
        // Close the file if it's open
        if (dataFile) {
            dataFile.close();
        }
        
        // Get the file size
        size_t fileSize = getFileSize(currentFileName.c_str());
        if (fileSize == 0) {
            testCharacteristic.writeValue("Error: Data file is empty");
            return true;
        }
        
        // Cap the transmission size
        size_t transmitSize = min(fileSize, (size_t)MAX_BUFFER_SIZE);
        testCharacteristic.writeValue("File size: " + String(fileSize) + " bytes, will transmit: " + String(transmitSize) + " bytes");
        
        // Allocate buffer for data
        uint8_t* dataBuffer = new uint8_t[transmitSize + 1];
        if (!dataBuffer) {
            testCharacteristic.writeValue("Error: Failed to allocate memory");
            return true;
        }
        
        // Read the file into the buffer
        File f = SD.open(currentFileName, FILE_READ);
        if (f) {
            f.read(dataBuffer, transmitSize);
            dataBuffer[transmitSize] = 0; // Null terminate
            f.close();
            
            // Transmit via Iridium
            testCharacteristic.writeValue("Transmitting via Iridium...");
            if (transmitViaIridium(dataBuffer, transmitSize)) {
                testCharacteristic.writeValue("Transmission successful!");
                // After successful transmission, shift the file contents
                shiftFileContents(currentFileName.c_str(), transmitSize);
                testCharacteristic.writeValue("Data file updated after transmission");
            } else {
                testCharacteristic.writeValue("Transmission failed");
            }
            
            delete[] dataBuffer;
            return true;
        } else {
            testCharacteristic.writeValue("Error: Failed to open data file");
            delete[] dataBuffer;
            return true;
        }
    } else if (command.startsWith("P") && command.length() > 1) {
        // Quick message transmission
        String message = command.substring(1);
        testCharacteristic.writeValue("Sending message: " + message);
        
        // Convert string to buffer
        size_t messageLength = message.length();
        uint8_t* messageBuffer = new uint8_t[messageLength + 1];
        message.getBytes(messageBuffer, messageLength + 1);
        
        // Transmit via Iridium
        if (transmitViaIridium(messageBuffer, messageLength)) {
            testCharacteristic.writeValue("Message transmitted successfully!");
        } else {
            testCharacteristic.writeValue("Message transmission failed");
        }
        
        delete[] messageBuffer;
        return true;
    }
    
    return false; // Command not handled
}

void formatLatLon() {
    String rawLatStr = String(myGNSS.getLatitude());
    String rawLonStr = String(myGNSS.getLongitude());

    // Format Latitude to 7 digits
    if (rawLatStr.length() >= 7) {
        strncpy(formattedLat, rawLatStr.substring(0, 7).c_str(), 7);
        formattedLat[7] = '\0';
    } else {
        strcpy(formattedLat, "0000000");
        for (int i = 0; i < 7 - rawLatStr.length(); i++) {
            formattedLat[i] = '0';
        }
        strncat(formattedLat, rawLatStr.c_str(), 7 - (7 - rawLatStr.length()));
        formattedLat[7] = '\0';
    }

    // Format Longitude to 8 digits (including sign)
    if (rawLonStr.length() >= 7) {
        if (rawLonStr.startsWith("-")) {
            formattedLon[0] = '-';
            if (rawLonStr.length() >= 8) {
                strncpy(formattedLon + 1, rawLonStr.substring(1, 8).c_str(), 7);
                formattedLon[8] = '\0';
            } else {
                // Pad with zeros after the existing digits
                strncpy(formattedLon + 1, rawLonStr.substring(1).c_str(), rawLonStr.length() - 1);
                for (int i = 0; i < 7 - (rawLonStr.length() - 1); i++) {
                    formattedLon[1 + (rawLonStr.length() - 1) + i] = '0';
                }
                formattedLon[8] = '\0';
            }
        } else {
            strncpy(formattedLon, rawLonStr.substring(0, 7).c_str(), 7);
            formattedLon[7] = '\0';
            formattedLon[8] = '\0';  // Ensure null termination
        }
    } else {
        if (rawLonStr.startsWith("-")) {
            formattedLon[0] = '-';
            strcpy(formattedLon + 1, "0000000");
            for (int i = 0; i < 7 - (rawLonStr.length() - 1); i++) {
                formattedLon[1 + i] = '0';
            }
            strncat(formattedLon, rawLonStr.substring(1).c_str(), 8 - strlen(formattedLon));
            formattedLon[8] = '\0';
        } else {
            strcpy(formattedLon, "0000000");
            for (int i = 0; i < 7 - rawLonStr.length(); i++) {
                formattedLon[i] = '0';
            }
            strncat(formattedLon, rawLonStr.c_str(), 8 - strlen(formattedLon));
            formattedLon[8] = '\0';
        }
    }

    finalLatStr = String(formattedLat);
    finalLonStr = String(formattedLon);
}

bool transmitViaIridium(const uint8_t* data, size_t dataSize) {
    // Cap at maximum payload size
    if (dataSize > MAX_BUFFER_SIZE) {
        dataSize = MAX_BUFFER_SIZE;
    }
    
    // Create a buffer for potential responses
    uint8_t rxBuffer[50];
    size_t rxBufferSize = sizeof(rxBuffer);
    
    // Track transmission time
    unsigned long startTime = millis();
    
    // Send the data
    int err = modem.sendReceiveSBDBinary(data, dataSize, rxBuffer, rxBufferSize);
    
    // Calculate transmission time
    unsigned long transmitTime = millis() - startTime;
    
    if (err == ISBD_SUCCESS) {
        Serial.print("Data transmitted successfully in ");
        Serial.print(transmitTime / 1000.0);
        Serial.println(" seconds");
        return true;
    } else {
        Serial.print("Iridium transmission failed with error code: ");
        Serial.println(err);
        return false;
    }
}

size_t getFileSize(const char* filename) {
    File f = SD.open(filename, FILE_READ);
    if (!f) {
        return 0;
    }
    size_t size = f.size();
    f.close();
    return size;
}

// Function to remove the first n bytes from the data file
void shiftFileContents(const char* filename, size_t bytesToRemove) {
    File sourceFile = SD.open(filename, FILE_READ);
    File tempFile = SD.open("TEMP.TXT", FILE_WRITE);  // Use uppercase for consistency
    
    if (!sourceFile || !tempFile) {
        Serial.println("Error opening files for shifting");
        return;
    }
    
    // Skip the bytes we want to remove
    sourceFile.seek(bytesToRemove);
    
    // Copy the remaining content to the temp file
    while (sourceFile.available()) {
        tempFile.write(sourceFile.read());
    }
    
    sourceFile.close();
    tempFile.close();
    
    // Replace the original file with the temp file
    SD.remove(filename);
    
    // Manually copy temp file to original file
    File newDataFile = SD.open(filename, FILE_WRITE);
    File readTempFile = SD.open("TEMP.TXT", FILE_READ);  // Use uppercase for consistency
    
    if (!readTempFile || !newDataFile) {
        Serial.println("Error opening files for manual rename");
        return;
    }
    
    // Copy content from temp file to new file
    while (readTempFile.available()) {
        newDataFile.write(readTempFile.read());
    }
    
    readTempFile.close();
    newDataFile.close();
    
    // Delete the temporary file
    SD.remove("TEMP.TXT");  // Use uppercase for consistency
    
    Serial.print("Removed ");
    Serial.print(bytesToRemove);
    Serial.println(" bytes from the data file");
}

#if DIAGNOSTICS
void ISBDConsoleCallback(IridiumSBD *device, char c) {
    Serial.write(c);
}

void ISBDDiagsCallback(IridiumSBD *device, char c) {
    Serial.write(c);
}
#endif 