#include <IridiumSBD.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <string.h>

#define DIAGNOSTICS false
#define SD_CS_PIN 8 // SD card chip select pin 8
#define MAX_BUFFER_SIZE 95 // Maximum buffer size in bytes for Iridium transmission

// Global objects and variables
SFE_UBLOX_GNSS myGNSS;
IridiumSBD modem(Wire);
unsigned long lastUpdateTime = 0;
const long updateInterval = 5000; // Update every 5 seconds
File dataFile;
const char* dataFileName = "loc.txt";
unsigned int readingCount = 0; // Count total readings

// Function to format latitude and longitude
char formattedLat[8];
char formattedLon[9];
String finalLatStr;
String finalLonStr;

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
                // Pad with zeros *after* the existing digits, before null termination
                strncpy(formattedLon + 1, rawLonStr.substring(1).c_str(), rawLonStr.length() - 1);
                for (int i = 0; i < 7 - (rawLonStr.length() - 1); i++) {
                    formattedLon[1 + (rawLonStr.length() - 1) + i] = '0'; // Corrected padding
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

// Function to transmit data via Iridium with chunking
bool transmitViaIridium(const uint8_t* data, size_t dataSize) {
    // Maximum allowed payload size for Iridium SBD
    const size_t MAX_IRIDIUM_PAYLOAD = 95;
    
    Serial.println("Sending data via Iridium...");
    Serial.print("Total data size: ");
    Serial.print(dataSize);
    Serial.println(" bytes");
    
    if (dataSize > MAX_IRIDIUM_PAYLOAD) {
        Serial.print("Data exceeds maximum payload size, truncating to ");
        Serial.print(MAX_IRIDIUM_PAYLOAD);
        Serial.println(" bytes");
        dataSize = MAX_IRIDIUM_PAYLOAD;
    }
    
    // Create a buffer for potential responses
    uint8_t rxBuffer[50];
    size_t rxBufferSize = sizeof(rxBuffer);
    
    // Track transmission time
    unsigned long startTime = millis();
    
    // Send the data (truncated if necessary)
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
        Serial.print("Time elapsed: ");
        Serial.print(transmitTime / 1000.0);
        Serial.println(" seconds");
        return false;
    }
}

// Function to get SD card file size
size_t getFileSize() {
    File f = SD.open(dataFileName, FILE_READ);
    if (!f) {
        Serial.println("Failed to open file for size check");
        return 0;
    }
    size_t size = f.size();
    f.close();
    return size;
}

// Function to read file contents into buffer
bool readFileToBuffer(uint8_t* buffer, size_t bufferSize) {
    File f = SD.open(dataFileName, FILE_READ);
    if (!f) {
        Serial.println("Failed to open file for reading");
        return false;
    }
    
    size_t bytesRead = f.read(buffer, bufferSize);
    f.close();
    
    if (bytesRead != bufferSize) {
        Serial.println("Warning: Bytes read doesn't match expected size");
    }
    
    return true;
}

// Function to clear the data file
void clearDataFile() {
    SD.remove(dataFileName);
    dataFile = SD.open(dataFileName, FILE_WRITE);
    if (dataFile) {
        dataFile.close();
        readingCount = 0; // Reset reading counter
    } else {
        Serial.println("Error creating new data file");
    }
}

// Function to remove the first n bytes from the data file
void shiftFileContents(size_t bytesToRemove) {
    File sourceFile = SD.open(dataFileName, FILE_READ);
    File tempFile = SD.open("temp.txt", FILE_WRITE);
    
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
    SD.remove(dataFileName);
    
    // Since SD.rename() is not available, manually copy temp file to original file
    File newDataFile = SD.open(dataFileName, FILE_WRITE);
    File readTempFile = SD.open("temp.txt", FILE_READ);
    
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
    SD.remove("temp.txt");
    
    Serial.print("Removed ");
    Serial.print(bytesToRemove);
    Serial.println(" bytes from the data file");
}

void setup() {
    int signalQuality = -1;
    Serial.begin(115200);
    while (!Serial);
    Serial.println(F("Iridium SBD with GNSS and SD Card Data Logging"));
    Serial.println(F("Checking location every 5 seconds"));
    Serial.println(F("Will transmit when file size reaches 95 bytes"));

    // Initialize I2C buses
    Wire.begin();
    Wire1.begin();
    Wire.setClock(400000);
    Wire1.setClock(400000);

    // Initialize SD card
    Serial.print("Initializing SD card...");
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD card initialization failed!");
        while (1); // Don't proceed if SD card initialization fails
    }
    Serial.println("SD card initialized successfully");

    // Check if data file exists, create if not
    if (!SD.exists(dataFileName)) {
        dataFile = SD.open(dataFileName, FILE_WRITE);
        if (dataFile) {
            dataFile.close();
            Serial.println("Data file created");
        } else {
            Serial.println("Error creating data file");
            while (1); // Don't proceed if file creation fails
        }
    } else {
        // File already exists, get its size
        size_t existingSize = getFileSize();
        Serial.print("Existing data file found, size: ");
        Serial.print(existingSize);
        Serial.println(" bytes");
        
        if (existingSize > 0) {
            Serial.print("Need ");
            Serial.print(MAX_BUFFER_SIZE - existingSize);
            Serial.println(" more bytes before transmission");
        }
    }

    // Initialize GNSS
    if (!myGNSS.begin()) {
        Serial.println("GNSS not detected. Check wiring!");
        while (1);
    }
    Serial.println("GNSS initialized successfully");

    // Initialize Iridium modem
    if (!modem.isConnected()) {
        Serial.println(F("Iridium not connected! Check wiring."));
        while (1);
    }
    Serial.println(F("Iridium modem connected"));

    // Handle supercapacitor charging
    modem.enableSuperCapCharger(true);
    while (!modem.checkSuperCapCharger()) {
        Serial.println(F("Charging supercapacitors..."));
        delay(1000);
    }
    Serial.println(F("Supercapacitors charged"));

    // Initialize modem
    modem.enable9603Npower(true);
    modem.setPowerProfile(IridiumSBD::USB_POWER_PROFILE);

    int err = modem.begin();
    if (err != ISBD_SUCCESS) {
        Serial.print(F("Modem begin failed: "));
        Serial.println(err);
        return;
    }

    // Check signal quality
    err = modem.getSignalQuality(signalQuality);
    if (err == ISBD_SUCCESS) {
        Serial.print(F("Signal quality: "));
        Serial.println(signalQuality);
    } else {
        Serial.print(F("Signal check failed: "));
        Serial.println(err);
    }
    
    // Reset timing to start immediate readings
    lastUpdateTime = 0;
}

void loop() {
    unsigned long currentTime = millis();
    
    // Check if it's time to update
    if (currentTime - lastUpdateTime >= updateInterval) {
        lastUpdateTime = currentTime;
        
        // Get GNSS data
        myGNSS.checkUblox();
        if (myGNSS.getGnssFixOk()) {
            readingCount++;
            
            // Debug output
            Serial.println("\n------- GNSS Reading #" + String(readingCount) + " -------");
            Serial.print("Raw latitude: ");
            Serial.println(myGNSS.getLatitude());
            Serial.print("Raw longitude: ");
            Serial.println(myGNSS.getLongitude());

            formatLatLon(); // Format latitude and longitude

            int year = myGNSS.getYear();
            int twoDigitYear = year % 100;
            char formattedYear[3];
            snprintf(formattedYear, sizeof(formattedYear), "%02d", twoDigitYear);

            // Create data record with formatted raw values
            String dataRecord = finalLatStr +
                                "," +
                                finalLonStr +
                                " " +
                                String(formattedYear) +
                                String(myGNSS.getMonth()) +
                                String(myGNSS.getDay()) +
                                " " +
                                String(myGNSS.getHour()) +
                                String(myGNSS.getMinute()) +
                                "\n";

            Serial.print("New reading: ");
            Serial.print(dataRecord);
            
            // Calculate record size
            size_t recordSize = dataRecord.length();
            Serial.print("Record size: ");
            Serial.print(recordSize);
            Serial.println(" bytes");

            // Open the data file and append new data
            dataFile = SD.open(dataFileName, FILE_WRITE);
            if (dataFile) {
                dataFile.print(dataRecord);
                dataFile.close();
                Serial.println("Data written to SD card");
            } else {
                Serial.println("Error opening data file for writing");
            }

            // Check file size after writing
            size_t fileSize = getFileSize();
            Serial.print("Current file size: ");
            Serial.print(fileSize);
            Serial.println(" bytes");
            
            // Calculate remaining bytes before transmission
            if (fileSize < MAX_BUFFER_SIZE) {
                size_t bytesRemaining = MAX_BUFFER_SIZE - fileSize;
                Serial.print("Need ");
                Serial.print(bytesRemaining);
                Serial.print(" more bytes before transmission (about ");
                
                // Estimate how many more readings needed
                if (recordSize > 0) {
                    int readingsRemaining = (bytesRemaining + recordSize - 1) / recordSize; // Ceiling division
                    Serial.print(readingsRemaining);
                    Serial.print(" more readings, ");
                    
                    // Estimate time
                    int timeRemaining = readingsRemaining * (updateInterval / 1000);
                    Serial.print(timeRemaining);
                    Serial.println(" seconds)");
                } else {
                    Serial.println("? more readings)");
                }
            }

            // If file size exceeds MAX_BUFFER_SIZE, transmit data
            if (fileSize >= MAX_BUFFER_SIZE) {
                Serial.println("File size exceeds threshold. Preparing to transmit...");
                
                // Cap the transmission size at MAX_BUFFER_SIZE bytes
                size_t transmitSize = min(fileSize, (size_t)MAX_BUFFER_SIZE);
                
                // Allocate buffer to hold file data
                uint8_t* dataBuffer = new uint8_t[transmitSize + 1]; // +1 for null termination
                
                // Read contents into buffer
                File f = SD.open(dataFileName, FILE_READ);
                if (f) {
                    f.read(dataBuffer, transmitSize);
                    dataBuffer[transmitSize] = 0; // Ensure null termination
                    f.close();
                    
                    // Transmit data via Iridium
                    if (transmitViaIridium(dataBuffer, transmitSize)) {
                        // Transmission successful, shift file contents
                        shiftFileContents(transmitSize);
                        Serial.println("Data transmitted and file updated");
                    } else {
                        Serial.println("Transmission failed. Data kept for retry.");
                    }
                }
                
                // Free memory
                delete[] dataBuffer;
            }
        } else {
            Serial.println("No GNSS Fix - data not logged.");
        }
    }
    
    delay(1000); // Small delay to prevent CPU overload
}

#if DIAGNOSTICS
void ISBDConsoleCallback(IridiumSBD *device, char c) {
    Serial.write(c);
}

void ISBDDiagsCallback(IridiumSBD *device, char c) {
    Serial.write(c);
}
#endif 