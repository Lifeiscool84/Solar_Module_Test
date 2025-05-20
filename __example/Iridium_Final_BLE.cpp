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
#define MIN_GNSS_SATS 4              // Minimum satellites for reliable GPS fix
#define GNSS_HDOP_THRESHOLD 5.0      // Maximum HDOP value for reliable positioning
#define DATA_VALIDATION_RETRIES 3    // Number of retries for validating data

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
#define ISBD_ERROR_DEFAULT       999  // Default error code when not specified

// Global variables
BLEService testService(SERVICE_UUID);
BLEStringCharacteristic testCharacteristic(CHARACTERISTIC_UUID, BLERead | BLEWrite | BLENotify | BLEIndicate, CHARACTERISTIC_MAX_LENGTH);
File dataFile;
String currentFileName = DATA_FILENAME;

// Core state variables for improved state management 
enum DataCollectionState { NOT_COLLECTING, COLLECTING_ACTIVE, COLLECTING_PAUSED };
DataCollectionState currentDataCollectionState = NOT_COLLECTING;
bool isIridiumReady = false;   // Tracks if Iridium modem has sufficient signal quality
bool isGNSSFixed = false;      // Tracks if GNSS module has a valid position fix
bool isCollectingData = false; // Legacy state variable - kept for backwards compatibility

// Add state variables for confirmation prompts
enum ConfirmationState { NO_CONFIRMATION_PENDING, CONFIRM_CLEAR_SD, CONFIRM_DELETE_FILE };
ConfirmationState pendingConfirmation = NO_CONFIRMATION_PENDING;
String pendingDeleteFilename = ""; // Store filename when delete confirmation is pending

// Command alias definitions for improved UI
struct CommandAlias {
    const char* shortCmd;
    const char* longCmd;
    const char* description;
};

// Command mapping table - pairs short commands with their descriptive equivalents
const CommandAlias COMMAND_ALIASES[] = {
    {"M", "MENU", "Show Menu"},
    {"L", "LIST", "List Files"},
    {"S", "START_LOG", "Start Data Collection"},
    {"X", "STOP_LOG", "Stop Data Collection"},
    {"ST", "STATUS", "Show System Status"},
    {"C", "CLEAR_ALL", "Clear SD Card"},
    {"R", "REBOOT", "Reboot System"},
    {"SQ", "SIGNAL", "Check Signal Quality"},
    {"T", "TRANSMIT", "Transmit Data File via Iridium"},
    {"VD", "VALIDATE", "Validate Data File"},
    {"DT", "DURATION", "Set Data Collection Duration (minutes)"},
    {"UI", "INTERVAL", "Set Update Interval (ms)"},
    {"P", "MESSAGE", "Send Quick Message via Iridium"},
    {"H", "HELP", "Show Help Information"},
    {"QT", "QUICK_TRACK", "Activate Quick Tracking Preset"}
};
const int NUM_COMMAND_ALIASES = sizeof(COMMAND_ALIASES) / sizeof(CommandAlias);

// Preset configuration structure
struct PresetConfig {
    const char* name;
    const char* description;
    unsigned long dataInterval;
    unsigned long dataDuration;
};

// Define the QUICK_TRACK preset
const PresetConfig QUICK_TRACK_PRESET = {
    "QUICK_TRACK",
    "Optimized for rapid data collection with frequent updates",
    2000,               // 2-second interval between data points
    10 * 60 * 1000      // 10 minutes duration
};

// Helper function to lookup command by either short or long version
String getNormalizedCommand(const String& inputCommand) {
    // First, extract the base command (before any parameters)
    String baseCommand = inputCommand;
    int paramIndex = inputCommand.indexOf(' ');
    if (paramIndex > 0) {
        baseCommand = inputCommand.substring(0, paramIndex);
    }
    
    // For commands with inline parameters (e.g., "DT15" or "DURATION15")
    String prefix = "";
    String params = "";
    for (const CommandAlias& alias : COMMAND_ALIASES) {
        // If it's already a short command that might have parameters, extract them
        if (inputCommand.startsWith(alias.shortCmd) && strlen(alias.shortCmd) < inputCommand.length()) {
            prefix = alias.shortCmd;
            params = inputCommand.substring(strlen(alias.shortCmd));
            break;
        }
        // If it's a long command that might have parameters, extract them
        else if (inputCommand.startsWith(alias.longCmd) && strlen(alias.longCmd) < inputCommand.length()) {
            prefix = alias.shortCmd; // Return short command with parameters
            params = inputCommand.substring(strlen(alias.longCmd));
            break;
        }
    }
    
    // If we found parameters, reconstruct the normalized command
    if (prefix.length() > 0) {
        return prefix + params;
    }
    
    // For full command matching (without parameters)
    for (const CommandAlias& alias : COMMAND_ALIASES) {
        if (baseCommand.equalsIgnoreCase(alias.longCmd)) {
            // If long command matches, return the short equivalent
            return alias.shortCmd;
        }
    }
    
    // Return original if no match found
    return inputCommand;
}

// Helper function to get a formatted menu with both short and long commands
String getFormattedMenu() {
    String menu = "===== Satellite GPS Tracker - Command Menu =====\n\n";
    
    // Group commands by category
    menu += "DATA COLLECTION:\n";
    menu += "  S/START_LOG - Start Data Collection\n";
    menu += "  X/STOP_LOG  - Stop Data Collection\n";
    menu += "  DT/DURATION - Set Data Collection Duration (minutes)\n";
    menu += "    Example: DT5 or DURATION5 (sets for 5 minutes)\n";
    menu += "  UI/INTERVAL - Set Update Interval (ms)\n";
    menu += "    Example: UI5000 or INTERVAL5000 (sets for 5000ms)\n";
    menu += "  QT/QUICK_TRACK - Activate Quick Tracking Preset\n\n";
    
    menu += "SYSTEM STATUS:\n";
    menu += "  ST/STATUS   - Show System Status\n";
    menu += "  SQ/SIGNAL   - Check Signal Quality\n";
    menu += "  R/REBOOT    - Reboot System\n\n";
    
    menu += "FILE MANAGEMENT:\n";
    menu += "  L/LIST      - List Files\n";
    menu += "  C/CLEAR_ALL - Clear SD Card (requires confirmation)\n";
    menu += "  D/DELETE    - Delete a specific file (requires confirmation)\n";
    menu += "    Example: D<filename> or DELETE <filename>\n\n";
    
    menu += "IRIDIUM COMMUNICATION:\n";
    menu += "  T/TRANSMIT  - Transmit Data File via Iridium\n";
    menu += "  P/MESSAGE   - Send Quick Message via Iridium\n";
    menu += "    Example: P<text> or MESSAGE <text>\n";
    menu += "  VD/VALIDATE - Validate Data File\n\n";
    
    menu += "HELP:\n";
    menu += "  M/MENU      - Show this Menu\n";
    menu += "  H/HELP      - Show Help Information\n";
    menu += "    Example: H<command> or HELP <command> for specific help\n\n";
    
    menu += "TIP: You can view file contents by entering the filename\n";
    menu += "TIP: Commands requiring confirmation will prompt you for 'YES'\n";
    menu += "================================================\n";
    
    return menu;
}

// Function to get detailed help text for a specific command
String getCommandHelp(const String& command) {
    String helpText = "";
    String normalizedCmd = getNormalizedCommand(command);
    
    // General help if no specific command requested
    if (normalizedCmd == "H") {
        helpText = "===== HELP INFORMATION =====\n\n";
        helpText += "To use this device, enter commands via the Bluetooth terminal.\n";
        helpText += "Commands can be entered in short form (e.g., 'S') or long form (e.g., 'START_LOG').\n\n";
        
        helpText += "For help with a specific command, enter:\n";
        helpText += "  H<command> or HELP <command>\n";
        helpText += "  Example: 'HELP STATUS' or 'H ST'\n\n";
        
        helpText += "Use MENU or M to see all available commands.\n";
        helpText += "================================================\n";
        return helpText;
    }
    
    // Command-specific help
    if (normalizedCmd == "M") {
        helpText = "===== MENU COMMAND HELP =====\n";
        helpText += "Usage: M or MENU\n\n";
        helpText += "Displays the main menu with all available commands organized by category.\n";
        helpText += "The menu shows both short and long forms of commands with descriptions.\n";
    }
    else if (normalizedCmd == "L") {
        helpText = "===== LIST COMMAND HELP =====\n";
        helpText += "Usage: L or LIST\n\n";
        helpText += "Lists all files stored on the SD card with their sizes.\n";
        helpText += "To view the contents of a file, enter the filename.\n";
    }
    else if (normalizedCmd == "S") {
        helpText = "===== START_LOG COMMAND HELP =====\n";
        helpText += "Usage: S or START_LOG\n\n";
        helpText += "Begins data collection from GNSS and other sensors.\n";
        helpText += "Data is stored in " + currentFileName + " by default.\n\n";
        helpText += "Related commands:\n";
        helpText += "- X/STOP_LOG: Stops data collection\n";
        helpText += "- DT/DURATION: Sets duration in minutes (e.g., DT5)\n";
        helpText += "- UI/INTERVAL: Sets update interval in ms (e.g., UI5000)\n";
    }
    else if (normalizedCmd == "X") {
        helpText = "===== STOP_LOG COMMAND HELP =====\n";
        helpText += "Usage: X or STOP_LOG\n\n";
        helpText += "Stops ongoing data collection and closes the data file.\n";
        helpText += "Displays the total duration of data collection.\n";
    }
    else if (normalizedCmd == "ST") {
        helpText = "===== STATUS COMMAND HELP =====\n";
        helpText += "Usage: ST or STATUS\n\n";
        helpText += "Displays comprehensive system status including:\n";
        helpText += "- Iridium modem signal strength and readiness\n";
        helpText += "- GNSS fix status and coordinates (if available)\n";
        helpText += "- Data collection state\n";
        helpText += "- SD card status\n";
    }
    else if (normalizedCmd == "C") {
        helpText = "===== CLEAR_ALL COMMAND HELP =====\n";
        helpText += "Usage: C or CLEAR_ALL\n\n";
        helpText += "CAUTION: This command deletes ALL files on the SD card.\n";
        helpText += "You will be prompted to confirm this action by typing 'YES'.\n";
        helpText += "Any other response will cancel the operation.\n";
    }
    else if (normalizedCmd.startsWith("D")) {
        helpText = "===== DELETE COMMAND HELP =====\n";
        helpText += "Usage: D<filename> or DELETE <filename>\n";
        helpText += "Example: DIRID_DAT.TXT or DELETE IRID_DAT.TXT\n\n";
        helpText += "Deletes a specific file from the SD card.\n";
        helpText += "You will be prompted to confirm this action by typing 'YES'.\n";
        helpText += "Any other response will cancel the operation.\n";
        helpText += "\nFormat requirements:\n";
        helpText += "- Filenames must follow 8.3 format (8 char name, 3 char extension)\n";
        helpText += "- SD file system is case-insensitive but shows uppercase\n";
        helpText += "- Valid examples: IRID_DAT.TXT, DATA01.CSV, LOG1.TXT\n";
        helpText += "- Invalid examples: mylongfilename.txt, data.csv.bak\n";
        helpText += "\nNotes:\n";
        helpText += "- Use the LIST command (L) to see available files\n";
        helpText += "- For short form (D), no space between command and filename\n";
        helpText += "- For long form (DELETE), include a space after the command\n";
        helpText += "- Deletion is permanent and cannot be undone\n";
    }
    else if (normalizedCmd == "R") {
        helpText = "===== REBOOT COMMAND HELP =====\n";
        helpText += "Usage: R or REBOOT\n\n";
        helpText += "Restarts the device immediately.\n";
        helpText += "This can be useful if the system becomes unresponsive.\n";
    }
    else if (normalizedCmd == "SQ") {
        helpText = "===== SIGNAL COMMAND HELP =====\n";
        helpText += "Usage: SQ or SIGNAL\n\n";
        helpText += "Checks the Iridium satellite signal quality on a scale of 0-5.\n";
        helpText += "A minimum signal of " + String(MIN_SIGNAL_QUALITY) + " is required for reliable transmission.\n\n";
        helpText += "If signal is low:\n";
        helpText += "1. Move to an outdoor location with clear sky view\n";
        helpText += "2. Ensure the antenna has proper orientation\n";
        helpText += "3. Wait for satellite to come into view (may take several minutes)\n";
    }
    else if (normalizedCmd == "T") {
        helpText = "===== TRANSMIT COMMAND HELP =====\n";
        helpText += "Usage: T or TRANSMIT\n\n";
        helpText += "Transmits data via Iridium satellite network.\n";
        helpText += "Maximum transmission size: " + String(MAX_BUFFER_SIZE) + " bytes per message.\n\n";
        helpText += "Requirements:\n";
        helpText += "- Iridium signal quality of at least " + String(MIN_SIGNAL_QUALITY) + "/5\n";
        helpText += "- Valid data file available\n\n";
        helpText += "After successful transmission, sent data is removed from the file.\n";
    }
    else if (normalizedCmd == "VD") {
        helpText = "===== VALIDATE COMMAND HELP =====\n";
        helpText += "Usage: VD or VALIDATE\n\n";
        helpText += "Validates the integrity of the data file.\n";
        helpText += "Checks format, structure, and record counts.\n";
        helpText += "Use before transmission to ensure data quality.\n";
    }
    else if (normalizedCmd.startsWith("DT")) {
        helpText = "===== DURATION COMMAND HELP =====\n";
        helpText += "Usage: DT<minutes> or DURATION<minutes>\n";
        helpText += "Example: DT5 or DURATION5\n\n";
        helpText += "Sets the duration of data collection in minutes.\n";
        helpText += "The default duration is 5 minutes.\n";
        helpText += "Valid range: 1-1440 minutes (24 hours maximum)\n";
        helpText += "Value must be a positive integer.\n";
        helpText += "\nNotes:\n";
        helpText += "- No space between command and value (use 'DT5', not 'DT 5')\n";
        helpText += "- Duration affects battery life and storage space\n";
        helpText += "- For durations longer than 60 minutes, ensure power source is stable\n";
    }
    else if (normalizedCmd.startsWith("UI")) {
        helpText = "===== INTERVAL COMMAND HELP =====\n";
        helpText += "Usage: UI<milliseconds> or INTERVAL<milliseconds>\n";
        helpText += "Example: UI5000 or INTERVAL5000\n\n";
        helpText += "Sets the interval between data points in milliseconds.\n";
        helpText += "Default interval is 10,000 ms (10 seconds).\n";
        helpText += "Valid range: 1000-300000 ms (1 second to 5 minutes)\n";
        helpText += "Value must be a positive integer.\n";
        helpText += "\nNotes:\n";
        helpText += "- No space between command and value (use 'UI5000', not 'UI 5000')\n";
        helpText += "- Shorter intervals increase data resolution but consume more storage\n";
        helpText += "- Intervals below 1000ms may cause system instability\n";
        helpText += "- Recommended minimum: 5000ms for stable satellite communication\n";
    }
    else if (normalizedCmd.startsWith("P")) {
        helpText = "===== MESSAGE COMMAND HELP =====\n";
        helpText += "Usage: P<message> or MESSAGE <message>\n";
        helpText += "Example: PHello World or MESSAGE Hello World\n\n";
        helpText += "Sends a short text message via Iridium satellite network.\n";
        helpText += "Maximum message length: " + String(MAX_BUFFER_SIZE) + " characters.\n";
        helpText += "Supported characters: A-Z, a-z, 0-9, standard punctuation.\n";
        helpText += "\nRequirements:\n";
        helpText += "- Iridium signal quality of at least " + String(MIN_SIGNAL_QUALITY) + "/5\n";
        helpText += "- Message format: Text only (no binary data)\n";
        helpText += "\nNotes:\n";
        helpText += "- For short form (P), no space between command and message\n";
        helpText += "- For long form (MESSAGE), include a space after the command\n";
        helpText += "- Message transmission may take 5-20 seconds depending on signal quality\n";
        helpText += "- Unicode/special characters may not transmit correctly\n";
    }
    else if (normalizedCmd == "QT") {
        helpText = "===== QUICK_TRACK PRESET HELP =====\n";
        helpText += "Usage: QT or QUICK_TRACK\n\n";
        helpText += "Activates the Quick Tracking preset configuration:\n";
        helpText += "- Sets interval to " + String(QUICK_TRACK_PRESET.dataInterval) + "ms (2 seconds)\n";
        helpText += "- Sets duration to " + String(QUICK_TRACK_PRESET.dataDuration / 60000) + " minutes\n";
        helpText += "- Automatically starts data collection\n\n";
        helpText += "Perfect for rapid data collection scenarios when you need\n";
        helpText += "frequent position updates and quick deployment.\n\n";
        helpText += "After activation:\n";
        helpText += "- Use 'X' to stop tracking early if needed\n";
        helpText += "- Use 'ST' to monitor status\n";
        helpText += "- Data will be saved in " + currentFileName + "\n";
    }
    else {
        // Command not found
        helpText = "Help topic not found for: " + command + "\n";
        helpText += "Use HELP without parameters to see general help information.\n";
        helpText += "Use MENU to see all available commands.\n";
    }
    
    return helpText;
}

unsigned long dataCollectionStartTime = 0;
unsigned long dataCollectionDuration = 5 * 60 * 1000; // Default 5 minutes (configurable via BLE)
unsigned long lastDataUpdateTime = 0;
unsigned long dataUpdateInterval = 10000; // Default 10 seconds between data points (configurable via BLE)

// Transmission tracking variables
int transmissionProgressValue = 0; // Store the current progress percentage

// Iridium and GNSS specific variables
IridiumSBD modem(Wire);
SFE_UBLOX_GNSS myGNSS;
int signalQuality = -1;
unsigned int readingCount = 0;
char formattedLat[8];
char formattedLon[9];
String finalLatStr;
String finalLonStr;

// Global variable to store the currently connected central
BLEDevice connectedCentral; // This will hold the current connected device

// Function prototypes
void handleClient(BLEDevice central);
void listFiles(BLEDevice central);
void sendFile(BLEDevice central, String fileName);
void waitForAck(BLEDevice central);
void collectSensorData();
void initializeSensor();
bool handleCommand(String command);
void rotateDataFile();
bool createNewDataFile();
String formatTimeValue(int value);
void formatLatLon();
bool transmitViaIridium(const uint8_t* data, size_t dataSize);
size_t getFileSize(const char* filename);
void shiftFileContents(const char* filename, size_t bytesToRemove);
bool validateGnssData();
bool validateFileOperation(bool result, const char* errorMsg, int errorCode = 0);
bool checkDataIntegrity(const uint8_t* data, size_t dataSize);
int calculateChecksum(const uint8_t* data, size_t dataSize);
void logError(const char* context, int errorCode);
void transmissionProgress(BLEDevice central, int progress, const String& statusMsg = "");
void updateTransmissionProgress(BLEDevice central, int progress, const String& statusMsg = "");
void sendMessage(const String& message);  // Helper that uses the global connectedCentral

// Function to convert decimal degrees to degrees, minutes, seconds format
String formatDMS(float decimalDegrees, bool isLatitude) {
    // Determine if positive or negative
    bool isNegative = (decimalDegrees < 0);
    decimalDegrees = fabs(decimalDegrees);
    
    // Extract degrees, minutes, seconds
    int degrees = (int)decimalDegrees;
    float minutesFloat = (decimalDegrees - degrees) * 60.0;
    int minutes = (int)minutesFloat;
    float seconds = (minutesFloat - minutes) * 60.0;
    
    // Format the output string
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%d° %d' %.1f\" %c", 
             degrees, minutes, seconds, 
             isLatitude ? (isNegative ? 'S' : 'N') : (isNegative ? 'W' : 'E'));
    
    return String(buffer);
}

// Function to convert decimal degrees to degrees decimal minutes format
String formatDDM(float decimalDegrees, bool isLatitude) {
    // Determine if positive or negative
    bool isNegative = (decimalDegrees < 0);
    decimalDegrees = fabs(decimalDegrees);
    
    // Extract degrees and decimal minutes
    int degrees = (int)decimalDegrees;
    float decimalMinutes = (decimalDegrees - degrees) * 60.0;
    
    // Format the output string
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%d° %.4f' %c", 
             degrees, decimalMinutes, 
             isLatitude ? (isNegative ? 'S' : 'N') : (isNegative ? 'W' : 'E'));
    
    return String(buffer);
}

// Helper functions for message formatting
String getSuccessMessage(const String& action, const String& details = "", const String& nextStep = "") {
    String message = "Success: " + action;
    if (details.length() > 0) {
        message += "\n" + details;
    }
    if (nextStep.length() > 0) {
        message += "\n→ " + nextStep;
    }
    return message;
}

String getFailureMessage(const String& action, const String& reason = "", const String& solution = "") {
    String message = "Error: " + action;
    if (reason.length() > 0) {
        message += "\n" + reason;
    }
    if (solution.length() > 0) {
        message += "\n→ " + solution;
    }
    return message;
}

String getWarningMessage(const String& situation, const String& impact = "", const String& recommendation = "") {
    String message = "Warning: " + situation;
    if (impact.length() > 0) {
        message += "\n" + impact;
    }
    if (recommendation.length() > 0) {
        message += "\n→ " + recommendation;
    }
    return message;
}

// Function to get human-readable GNSS coordinates
String getHumanReadableCoordinates(long rawLatitude, long rawLongitude) {
    // Convert from millionths of a degree to degrees
    float latitude = rawLatitude / 10000000.0;
    float longitude = rawLongitude / 10000000.0;
    
    String dmsFormat = "DMS: " + formatDMS(latitude, true) + ", " + formatDMS(longitude, false);
    String ddmFormat = "DDM: " + formatDDM(latitude, true) + ", " + formatDDM(longitude, false);
    
    return dmsFormat + "\n" + ddmFormat;
}

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
    BLE.setLocalName("Satellite GPS Tracker");
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
        // Update the state for better state management
        if (currentDataCollectionState == COLLECTING_PAUSED) {
            // Resume data collection after BLE disconnect
            currentDataCollectionState = COLLECTING_ACTIVE;
        }
        
        // Only collect data when not connected and data collection is active
        unsigned long currentTime = millis();
        if (currentTime - lastDataUpdateTime >= dataUpdateInterval) {
            lastDataUpdateTime = currentTime;
            collectSensorData();
        }
    }
}

void handleClient(BLEDevice central) {
    // Store the current central in the global variable for use by other functions
    connectedCentral = central;
    
    // Close any open file when a client connects
    if (dataFile) {
        dataFile.close();
    }
    
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    
    // Update state management
    if (isCollectingData) {
        // If we were collecting data, pause that activity during connection
        currentDataCollectionState = COLLECTING_PAUSED;
    }
    isCollectingData = false; // Pause data collection during connection
    
    // Send welcome message and initial status
    String welcomeMsg = 
        "Welcome to Satellite GPS Tracker!\n"
        "This device collects GPS data for satellite transmission.\n";
    testCharacteristic.writeValue(welcomeMsg);
    delay(DELAY_BETWEEN_PACKETS);
    
    // Show initial system status
    // Show Iridium connection status
    int iridiumSignal = -1;
    modem.getSignalQuality(iridiumSignal);
    bool iridiumReady = iridiumSignal >= MIN_SIGNAL_QUALITY;
    isIridiumReady = iridiumReady; // Update global state variable
    
    // Check GNSS status
    myGNSS.checkUblox(); // Make sure we have fresh data
    bool gnssFixed = myGNSS.getGnssFixOk();
    isGNSSFixed = gnssFixed; // Update global state variable
    
    // Update data collection state
    currentDataCollectionState = isCollectingData ? COLLECTING_PAUSED : NOT_COLLECTING;
    
    // Show quick status
    String quickStatus = "SYSTEM STATUS:\n";
    quickStatus += "- Iridium: " + String(iridiumReady ? "READY" : "NOT READY") + "\n";
    quickStatus += "- GNSS: " + String(gnssFixed ? "FIXED" : "NO FIX") + "\n";
    
    // Show more detailed collection state
    String collectionStateMsg;
    switch (currentDataCollectionState) {
        case COLLECTING_ACTIVE:
            collectionStateMsg = "ACTIVE";
            break;
        case COLLECTING_PAUSED:
            collectionStateMsg = "PAUSED";
            break;
        case NOT_COLLECTING:
        default:
            collectionStateMsg = "INACTIVE";
            break;
    }
    quickStatus += "- Data Collection: " + collectionStateMsg + "\n";
    quickStatus += "- SD Card: READY";
    
    testCharacteristic.writeValue(quickStatus);
    delay(DELAY_BETWEEN_PACKETS);
    
    // Get updated menu with both short and descriptive commands
    String menuOptions = getFormattedMenu();
    
    Serial.println(menuOptions);
    testCharacteristic.writeValue(menuOptions);
    
    while (central.connected()) {
        if (testCharacteristic.written()) {
            String originalCommand = testCharacteristic.value();
            Serial.print("Received command: ");
            Serial.println(originalCommand);
            
            // Normalize the command (convert long form to short if needed)
            String command = getNormalizedCommand(originalCommand);
            
            // If command was translated, log the conversion
            if (command != originalCommand) {
                Serial.print("Normalized to: ");
            Serial.println(command);
            }
            
            if (command == "M") {
                // Show menu options
                // testCharacteristic.writeValue(menuOptions); // Original call
                // Manually chunk and send menuOptions
                const int CHUNK_SIZE_MENU = CHARACTERISTIC_MAX_LENGTH - 5; // Max length, leaving some buffer
                int startPosMenu = 0;
                while (startPosMenu < menuOptions.length()) {
                    int endPosMenu = min(startPosMenu + CHUNK_SIZE_MENU, (int)menuOptions.length());
                    String chunkMenu = menuOptions.substring(startPosMenu, endPosMenu);
                    testCharacteristic.writeValue(chunkMenu);
                    delay(DELAY_BETWEEN_PACKETS);
                    startPosMenu = endPosMenu;
                }
            } else if (command == "L") {
                // List files on the SD card
                listFiles(central);
            } else if (command == "ST") {
                // Get system status, enhanced with more comprehensive information
                int iridiumSignal = -1;
                modem.getSignalQuality(iridiumSignal);
                bool iridiumReady = iridiumSignal >= MIN_SIGNAL_QUALITY;
                isIridiumReady = iridiumReady; // Update global state variable
                
                // Check GNSS status
                myGNSS.checkUblox(); // Make sure we have fresh data
                bool gnssFixed = myGNSS.getGnssFixOk();
                isGNSSFixed = gnssFixed; // Update global state variable
                
                // Get SD card status and available space
                uint32_t totalSpace = 0;
                uint32_t usedSpace = 0;
                
                // Calculate free space on SD card
                File root = SD.open("/");
                if (root) {
                    File entry;
                    while (entry = root.openNextFile()) {
                        if (!entry.isDirectory()) {
                            usedSpace += entry.size();
                        }
                        entry.close();
                    }
                    root.close();
                }
                
                // Estimated space for a microSD (will be approximate)
                totalSpace = 1024 * 1024 * 1024; // Assume 1GB as default
                uint32_t freeSpace = totalSpace - usedSpace;
                
                // Format free space in human-readable format
                String freeSpaceStr;
                if (freeSpace > 1024 * 1024 * 1024) {
                    freeSpaceStr = String(freeSpace / (1024.0 * 1024.0 * 1024.0), 1) + " GB";
                } else if (freeSpace > 1024 * 1024) {
                    freeSpaceStr = String(freeSpace / (1024.0 * 1024.0), 1) + " MB";
                } else {
                    freeSpaceStr = String(freeSpace / 1024.0, 1) + " KB";
                }
                
                // Prepare comprehensive status response
                String statusMsg = "===== COMPREHENSIVE SYSTEM STATUS =====\n\n";
                
                // 1. DATA COLLECTION STATUS
                statusMsg += "DATA COLLECTION:\n";
                
                // Handle the switch cases with proper variable initialization to avoid jumps over initialization
                if (currentDataCollectionState == COLLECTING_ACTIVE) {
                    String collectionStateMsg = "ACTIVE";
                    // Add elapsed time and remaining time if duration is set
                    unsigned long elapsed = millis() - dataCollectionStartTime;
                    String elapsedStr = String(elapsed / 60000) + "m " + String((elapsed % 60000) / 1000) + "s";
                    statusMsg += "- Status: " + collectionStateMsg + " (Running for " + elapsedStr + ")\n";
                    
                    if (dataCollectionDuration > 0) {
                        unsigned long remaining = (dataCollectionDuration > elapsed) ? 
                            (dataCollectionDuration - elapsed) : 0;
                        String remainingStr = String(remaining / 60000) + "m " + String((remaining % 60000) / 1000) + "s";
                        statusMsg += "- Remaining: " + remainingStr + "\n";
                } else {
                        statusMsg += "- Remaining: No limit set\n";
                    }
                }
                else if (currentDataCollectionState == COLLECTING_PAUSED) {
                    String collectionStateMsg = "PAUSED";
                    statusMsg += "- Status: " + collectionStateMsg + "\n";
                }
                else if (currentDataCollectionState == NOT_COLLECTING) {
                    String collectionStateMsg = "INACTIVE";
                    statusMsg += "- Status: " + collectionStateMsg + "\n";
                }
                
                statusMsg += "- Interval: " + String(dataUpdateInterval) + "ms (" + 
                           String(dataUpdateInterval / 1000.0, 1) + "s)\n";
                statusMsg += "- Duration: " + String(dataCollectionDuration / 60000) + " min\n";
                statusMsg += "- Data points: " + String(readingCount) + "\n";
                statusMsg += "- File: " + currentFileName + "\n\n";
                
                // 2. SATELLITE COMMUNICATIONS
                statusMsg += "SATELLITE COMMUNICATIONS:\n";
                statusMsg += "- Iridium Signal: " + String(iridiumSignal) + "/5";
                if (iridiumSignal >= MIN_SIGNAL_QUALITY) {
                    statusMsg += " (GOOD)\n";
                } else if (iridiumSignal > 0) {
                    statusMsg += " (WEAK - Move to open sky)\n";
                } else {
                    statusMsg += " (NO SIGNAL - Check antenna)\n";
                }
                
                // 3. GNSS INFORMATION
                statusMsg += "\nGNSS INFORMATION:\n";
                statusMsg += "- Fix: " + String(gnssFixed ? "YES" : "NO") + "\n";
                
                // Add satellite count if available
                int numSats = myGNSS.getSIV();
                float hdop = myGNSS.getHorizontalDOP();
                
                statusMsg += "- Satellites: " + String(numSats) + " (";
                if (numSats >= MIN_GNSS_SATS) {
                    statusMsg += "GOOD";
                } else if (numSats > 0) {
                    statusMsg += "MARGINAL";
                } else {
                    statusMsg += "POOR";
                }
                statusMsg += ")\n";
                
                statusMsg += "- Position accuracy: ";
                if (hdop <= 1.0) {
                    statusMsg += "Excellent";
                } else if (hdop <= 2.0) {
                    statusMsg += "Good";
                } else if (hdop <= 5.0) {
                    statusMsg += "Moderate";
                } else if (hdop <= 10.0) {
                    statusMsg += "Fair";
                } else {
                    statusMsg += "Poor";
                }
                statusMsg += " (HDOP: " + String(hdop, 1) + ")\n";
                
                // If GNSS has a fix, show human-readable coordinates
                if (gnssFixed) {
                    long lat = myGNSS.getLatitude();
                    long lon = myGNSS.getLongitude();
                    
                    statusMsg += "- Coordinates:\n  " + 
                               formatDMS(lat / 10000000.0, true) + "\n  " + 
                               formatDMS(lon / 10000000.0, false) + "\n";
                    
                    // Add speed and altitude if available
                    long speed = myGNSS.getGroundSpeed();
                    long alt = myGNSS.getAltitudeMSL();
                    
                    // Convert from mm/s to km/h
                    float speedKmh = speed * 0.0036; // mm/s to km/h conversion
                    
                    statusMsg += "- Speed: " + String(speedKmh, 1) + " km/h\n";
                    statusMsg += "- Altitude: " + String(alt / 1000.0, 1) + " m\n";
                } else {
                    statusMsg += "- Coordinates: Not available (No fix)\n";
                }
                
                // 4. STORAGE INFORMATION
                statusMsg += "\nSTORAGE STATUS:\n";
                statusMsg += "- SD Card: READY\n";
                statusMsg += "- Free space: " + freeSpaceStr + "\n";
                
                // Estimate recording capacity
                // Approx 100 bytes per reading
                unsigned long bytesPerHour = (3600000 / dataUpdateInterval) * 100;
                float hoursRemaining = freeSpace / (float)bytesPerHour;
                
                statusMsg += "- Est. recording capacity: ";
                if (hoursRemaining > 24) {
                    statusMsg += String(hoursRemaining / 24, 1) + " days\n";
                } else {
                    statusMsg += String(hoursRemaining, 1) + " hours\n";
                }
                
                // 5. SYSTEM INFORMATION
                statusMsg += "\nSYSTEM INFORMATION:\n";
                statusMsg += "- Uptime: " + String(millis() / 3600000) + "h " + 
                           String((millis() % 3600000) / 60000) + "m\n";
                
                // End of status report
                statusMsg += "\n===========================================\n";
                statusMsg += "Use HELP <command> for command instructions\n";
                
                // Split and send in chunks due to BLE size limits
                // Send the status message in chunks
                const int CHUNK_SIZE_STATUS = CHARACTERISTIC_MAX_LENGTH - 5; // Safe size for BLE characteristic
                int startPosStatus = 0;
                while (startPosStatus < statusMsg.length()) {
                    int endPosStatus = min(startPosStatus + CHUNK_SIZE_STATUS, (int)statusMsg.length());
                    String chunkStatus = statusMsg.substring(startPosStatus, endPosStatus);
                    testCharacteristic.writeValue(chunkStatus);
                    delay(DELAY_BETWEEN_PACKETS);
                    // Removed: waitForAck(central); // Let link layer handle reliability
                    startPosStatus = endPosStatus;
                }
            } else if (command.startsWith("D") && command.length() > 1) {
                // Request confirmation before deleting file
                pendingDeleteFilename = command.substring(1);
                pendingConfirmation = CONFIRM_DELETE_FILE;
                testCharacteristic.writeValue("WARNING: Are you sure you want to delete '" + pendingDeleteFilename + "'?");
                testCharacteristic.writeValue("Type 'YES' to confirm or any other text to cancel.");
            } else if (command == "S") {
                // Start data collection
                if (!isCollectingData) {
                    // Create a new data file if needed or reset the existing one
                    if (!createNewDataFile()) {
                        testCharacteristic.writeValue("Error: Failed to create data file");
                        testCharacteristic.writeValue("Check SD card status with 'ST' command");
                        return;
                    }
                    
                isCollectingData = true;
                    currentDataCollectionState = COLLECTING_ACTIVE;
                dataCollectionStartTime = millis();
                    
                    // Write CSV header if it's a new file
                    if (dataFile.size() == 0) {
                        dataFile.println("Latitude,Longitude,Altitude,Satellites,Time,Checksum");
                    }
                    
                    // Update the user on status and what to do next
                    testCharacteristic.writeValue("Success: Data collection started");
                    testCharacteristic.writeValue("File: " + currentFileName + ", Duration: " + String(dataCollectionDuration / 60000) + " minutes");
                    testCharacteristic.writeValue("Use 'X' to stop recording or 'ST' to check status");
                } else {
                    // Already collecting data - provide a helpful message
                    testCharacteristic.writeValue("Warning: Data collection already active");
                    testCharacteristic.writeValue("Started " + String((millis() - dataCollectionStartTime) / 1000) + " seconds ago");
                    testCharacteristic.writeValue("Use 'X' to stop current recording first if needed");
                }
            } else if (command == "X") {
                // Stop data collection
                if (isCollectingData) {
                    isCollectingData = false;
                    currentDataCollectionState = NOT_COLLECTING;
                    
                    // Close the data file to ensure all data is saved
                    if (dataFile) {
                        dataFile.close();
                    }
                    
                    unsigned long duration = (millis() - dataCollectionStartTime) / 1000; // Convert to seconds
                    String durationStr = "";
                    
                    // Format duration in a human-readable way
                    if (duration >= 3600) {
                        durationStr = String(duration / 3600) + "h " + String((duration % 3600) / 60) + "m " + String(duration % 60) + "s";
                    } else if (duration >= 60) {
                        durationStr = String(duration / 60) + "m " + String(duration % 60) + "s";
                    } else {
                        durationStr = String(duration) + "s";
                    }
                    
                    testCharacteristic.writeValue("Success: Data collection stopped");
                    testCharacteristic.writeValue("Duration: " + durationStr);
                    testCharacteristic.writeValue("Data saved to: " + currentFileName);
                    testCharacteristic.writeValue("Use 'T' to transmit data or 'VD' to validate");
                } else {
                    testCharacteristic.writeValue("Warning: No data collection in progress");
                    testCharacteristic.writeValue("Nothing to stop");
                    testCharacteristic.writeValue("Use 'S' to start data collection");
                }
            } else if (command == "C") {
                // Clear the SD card - Request confirmation instead of immediate action
                pendingConfirmation = CONFIRM_CLEAR_SD;
                testCharacteristic.writeValue("WARNING: This will delete ALL files on the SD card");
                testCharacteristic.writeValue("This action cannot be undone");
                testCharacteristic.writeValue("Type 'YES' to confirm or any other text to cancel");
            } else if (pendingConfirmation != NO_CONFIRMATION_PENDING && originalCommand == "YES") {
                // Handle confirmation responses
                if (pendingConfirmation == CONFIRM_CLEAR_SD) {
                    // Clear the SD card (original implementation)
                File root = SD.open("/");
                    
                    if (!root) {
                        testCharacteristic.writeValue("Error: SD card may be missing or corrupted");
                        testCharacteristic.writeValue("Check that SD card is properly inserted");
                        pendingConfirmation = NO_CONFIRMATION_PENDING;
                        return;
                    }
                    
                    testCharacteristic.writeValue("Clearing all files from SD card...");
                    
                    // Count total files for progress reporting
                    int totalFileCount = 0;
                    File countFile = root;
                while (true) {
                        File entry = countFile.openNextFile();
                        if (!entry) {
                            break;
                        }
                        if (!entry.isDirectory()) {
                            totalFileCount++;
                        }
                        entry.close();
                    }
                    
                    // Reset to start of directory
                    root.rewindDirectory();
                    
                    // Delete all files from SD card
                    int filesDeleted = 0;
                    int lastProgress = 0;
                    
                    File file = root;
                    while (true) {
                        File entry = file.openNextFile();
                        if (!entry) {
                            break;
                        }
                    
                    if (!entry.isDirectory()) {
                            String fileName = entry.name();
                            entry.close(); // Close the file before deleting
                            
                            // Show progress for large number of files
                            if (SD.remove(fileName)) {
                                filesDeleted++;
                                
                                // Show progress updates for larger file counts
                                if (totalFileCount > 5) {
                                    int progress = (filesDeleted * 100) / totalFileCount;
                                    if (progress >= lastProgress + 20) { // Update every 20%
                                        lastProgress = progress;
                                        testCharacteristic.writeValue("Clearing files: " + String(progress) + "% (" + 
                                                                    String(filesDeleted) + "/" + String(totalFileCount) + ")");
                                    }
                                }
                            }
                        } else {
                    entry.close();
                }
                    }
                    
                root.close();
                    
                    if (filesDeleted > 0) {
                        testCharacteristic.writeValue("Success: SD card cleared");
                        testCharacteristic.writeValue(String(filesDeleted) + " files deleted");
                        testCharacteristic.writeValue("Use 'L' to verify files are gone or 'S' to start new data collection");
                    } else {
                        testCharacteristic.writeValue("Success: SD card is already empty");
                        testCharacteristic.writeValue("No files to delete");
                        testCharacteristic.writeValue("Use 'S' to start new data collection");
                    }
                    
                    pendingConfirmation = NO_CONFIRMATION_PENDING;
                } else if (pendingConfirmation == CONFIRM_DELETE_FILE) {
                    // Delete the file with the name we stored earlier
                    if (pendingDeleteFilename.length() > 0) {
                        testCharacteristic.writeValue("Deleting file: " + pendingDeleteFilename);
                        
                        // Get file size before deletion to report in the confirmation
                        File fileToDelete = SD.open(pendingDeleteFilename);
                        size_t fileSize = 0;
                        
                        if (fileToDelete) {
                            fileSize = fileToDelete.size();
                            fileToDelete.close();
                        }
                        
                        // For larger files, show a progress indicator (simulated since delete is atomic)
                        if (fileSize > 10000) { // Only for files >10KB
                            testCharacteristic.writeValue("Processing file... (0%)");
                            delay(250); // Delay to show progress
                            testCharacteristic.writeValue("Processing file... (25%)");
                            delay(250);
                            testCharacteristic.writeValue("Processing file... (50%)");
                            delay(250);
                            testCharacteristic.writeValue("Processing file... (75%)");
                            delay(250);
                        }
                        
                        if (SD.remove(pendingDeleteFilename)) {
                            // Format file size in a readable way
                            String sizeStr;
                            if (fileSize > 1000000) {
                                sizeStr = String(fileSize / 1000000.0, 1) + " MB";
                            } else if (fileSize > 1000) {
                                sizeStr = String(fileSize / 1000.0, 1) + " KB";
                            } else {
                                sizeStr = String(fileSize) + " bytes";
                            }
                            
                            testCharacteristic.writeValue("Success: File deleted");
                            testCharacteristic.writeValue(pendingDeleteFilename + " (" + sizeStr + ")");
                            testCharacteristic.writeValue("Use 'L' to view remaining files");
                        } else {
                            testCharacteristic.writeValue("Failure: Failed to delete file");
                            testCharacteristic.writeValue("Check filename and try again");
                        }
                    } else {
                        testCharacteristic.writeValue("Failure: No filename specified");
                        testCharacteristic.writeValue("Use D followed by filename, e.g., DIRID_DAT.TXT");
                    }
                    pendingDeleteFilename = "";
                    pendingConfirmation = NO_CONFIRMATION_PENDING;
                } else if (pendingConfirmation != NO_CONFIRMATION_PENDING) {
                    // Any response other than "YES" cancels the operation
                    testCharacteristic.writeValue("Operation cancelled. No changes were made.");
                    pendingDeleteFilename = "";
                    pendingConfirmation = NO_CONFIRMATION_PENDING;
            } else if (command == "R") {
                testCharacteristic.writeValue("Rebooting...");
                delay(500);
                // Implement reboot command for your specific board
                am_hal_reset_control(AM_HAL_RESET_CONTROL_TPIU_RESET, 0); // For Artemis/Apollo boards
                }
            } else if (command.startsWith("DT")) {
                // Extract the numeric part
                String durationStr = command.substring(2);
                int duration = durationStr.toInt();
                
                // Validate duration range
                if (duration <= 0) {
                    testCharacteristic.writeValue("Failure: Duration must be a positive number");
                    testCharacteristic.writeValue("Example: DT5 for 5 minutes");
                } else if (duration > 1440) {
                    testCharacteristic.writeValue("Failure: Maximum duration is 1440 minutes (24 hours)");
                    testCharacteristic.writeValue("Enter a value between 1 and 1440");
                } else {
                // Set data collection duration in minutes
                    dataCollectionDuration = duration * 60 * 1000;  // Convert to milliseconds
                    testCharacteristic.writeValue("Success: Duration set");
                    testCharacteristic.writeValue(String(duration) + " minutes");
                    testCharacteristic.writeValue("Use 'S' to start data collection with this duration");
                }
            } else if (command.startsWith("UI")) {
                // Extract the numeric part
                String intervalStr = command.substring(2);
                int interval = intervalStr.toInt();
                
                // Validate interval range
                if (interval <= 0) {
                    testCharacteristic.writeValue("Failure: Interval must be a positive number");
                    testCharacteristic.writeValue("Example: UI5000 for 5 seconds");
                } else if (interval < 1000) {
                    testCharacteristic.writeValue("Warning: Values below 1000ms may cause instability");
                    testCharacteristic.writeValue("Setting to minimum safe value (1000ms)");
                    dataUpdateInterval = 1000;
                } else if (interval > 300000) {
                    testCharacteristic.writeValue("Warning: Maximum interval is 300000ms (5 minutes)");
                    testCharacteristic.writeValue("Setting to maximum value (300000ms)");
                    dataUpdateInterval = 300000;
                } else {
                    // Set data update interval in milliseconds
                    dataUpdateInterval = interval;
                    testCharacteristic.writeValue("Success: Interval set");
                    testCharacteristic.writeValue(String(interval) + " ms between data points");
                    testCharacteristic.writeValue("Use 'S' to start data collection with this interval");
                }
            }
            else if (command == "QT" || command == "QUICK_TRACK") {
                // Apply the QUICK_TRACK preset configuration
                dataUpdateInterval = QUICK_TRACK_PRESET.dataInterval;
                dataCollectionDuration = QUICK_TRACK_PRESET.dataDuration;
                
                // Provide feedback about the preset
                testCharacteristic.writeValue("Activated " + String(QUICK_TRACK_PRESET.name) + " preset");
                testCharacteristic.writeValue("Interval: " + String(QUICK_TRACK_PRESET.dataInterval) + "ms, Duration: " + 
                                           String(QUICK_TRACK_PRESET.dataDuration / 60000) + " minutes");
                
                // Automatically start data collection
                if (!isCollectingData) {
                    // Create or open data file
                    if (!SD.exists(currentFileName)) {
                        if (!createNewDataFile()) {
                            testCharacteristic.writeValue("Failed to create data file");
                            testCharacteristic.writeValue("Check SD card status");
        return;
                        }
                    } else {
                        dataFile = SD.open(currentFileName, FILE_WRITE);
                        if (!dataFile) {
                            testCharacteristic.writeValue("Failed to open data file");
                            testCharacteristic.writeValue("Try rebooting the device");
                            return;
                        }
                        // Seek to end to append
                        dataFile.seek(dataFile.size());
                    }
                    
                    // Begin collecting data
                    isCollectingData = true;
                    currentDataCollectionState = COLLECTING_ACTIVE;
                    dataCollectionStartTime = millis();
                    lastDataUpdateTime = dataCollectionStartTime;
                    
                    testCharacteristic.writeValue("Quick tracking started automatically");
                    testCharacteristic.writeValue("Recording to: " + currentFileName);
                    testCharacteristic.writeValue("Use 'X' to stop or 'ST' to check status");
                } else {
                    // Already collecting data - update parameters on-the-fly
                    testCharacteristic.writeValue("Updated collection parameters for active session");
                    testCharacteristic.writeValue("New interval: " + String(dataUpdateInterval) + "ms");
                    testCharacteristic.writeValue("Use 'X' to stop when finished");
                }
            } else if (command.startsWith("H")) { // Handle H and H<command>
                String helpArg = "";
                if (command.length() > 1) { // Check if there's an argument after H
                    helpArg = originalCommand.substring(originalCommand.indexOf(command.substring(1))); // Get the argument from original command
                } else {
                    helpArg = command; // Just "H"
                }
                String helpText = getCommandHelp(helpArg);
                // Manually chunk and send helpText
                const int CHUNK_SIZE_HELP = CHARACTERISTIC_MAX_LENGTH - 5; // Max length, leaving some buffer
                int startPosHelp = 0;
                while (startPosHelp < helpText.length()) {
                    int endPosHelp = min(startPosHelp + CHUNK_SIZE_HELP, (int)helpText.length());
                    String chunkHelp = helpText.substring(startPosHelp, endPosHelp);
                    testCharacteristic.writeValue(chunkHelp);
                    delay(DELAY_BETWEEN_PACKETS);
                    startPosHelp = endPosHelp;
                }
            }
        }
    }
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
    }
    
    file.close();
    testCharacteristic.writeValue("End of file");
}

void waitForAck(BLEDevice central) {
    unsigned long startTime = millis();
    while (central.connected() && (millis() - startTime < 2000)) {
        // If a new value is written by the client, assume it's an ACK or next command.
        // This makes it more responsive than waiting for a specific "ACK" string.
        if (testCharacteristic.written()) { 
            // Optional: check if testCharacteristic.value() is "ACK" if strict ACK is ever needed
            return;
        }
        delay(DELAY_BETWEEN_ACK_CHECKS);
    }
    // If we reach here, we timed out waiting for ACK
    Serial.println("Timed out waiting for ACK");
}

bool createNewDataFile() {
    // Open the data file for writing
    dataFile = SD.open(currentFileName, FILE_WRITE);
    if (!dataFile) {
        Serial.println("Failed to create data file");
        return false;
    } else {
        // Write a header to the file
        bool headerWritten = dataFile.println("Latitude,Longitude,Date,Time,CheckSum");
        dataFile.flush();
        return headerWritten;
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
    
    // Enable additional GNSS data for validation
    myGNSS.setAutoNAVPVAT(true);      // Enable automatic NAVPVT messages
    myGNSS.setAutoPVT(true);         // Enable navigation position velocity time solution
    myGNSS.setAutoHPPOSLLH(true);    // Enable high precision geodetic position
    
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
        if (!validateFileOperation(createNewDataFile(), "Failed to create data file", 0)) {
            return;
        }
    }
    
    // Get GNSS data
    myGNSS.checkUblox();
    
    // Validate GNSS data before logging
    if (myGNSS.getGnssFixOk() && validateGnssData()) {
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
        
        // Calculate and append checksum for data integrity
        uint8_t buffer[dataRecord.length()];
        dataRecord.getBytes(buffer, dataRecord.length());
        int checksum = calculateChecksum(buffer, dataRecord.length() - 1); // Don't include newline in checksum
        
        // Write to SD card with validation
        if (validateFileOperation(dataFile.print(dataRecord), "Failed to write data record", 0)) {
            // Call flush and handle separately since it returns void
            dataFile.flush(); // Flush returns void, so we can't validate return value directly
            
            // Timestamp for last successful write
            lastDataUpdateTime = millis();
        Serial.print("Data point recorded: ");
        Serial.print(dataRecord);
        }
        
        // Rotate file if needed
        rotateDataFile();
    } else {
        Serial.println("GNSS data validation failed - data not logged");
    }
}

bool validateGnssData() {
    // Check for minimum satellite count
    int numSatellites = myGNSS.getSIV();
    if (numSatellites < MIN_GNSS_SATS) {
        Serial.print("Insufficient satellites: ");
        Serial.print(numSatellites);
        Serial.print(" (minimum ");
        Serial.print(MIN_GNSS_SATS);
        Serial.println(")");
        return false;
    }
    
    // Check horizontal dilution of precision (HDOP)
    // Lower values indicate better precision
    float hdop = myGNSS.getHorizontalDOP() / 100.0; // HDOP is reported in cm
    if (hdop > GNSS_HDOP_THRESHOLD) {
        Serial.print("Poor HDOP value: ");
        Serial.print(hdop);
        Serial.print(" (threshold ");
        Serial.print(GNSS_HDOP_THRESHOLD);
        Serial.println(")");
        return false;
    }
    
    // Check for valid speed and heading
    // Can be used to filter out erratic readings
    long speed = myGNSS.getGroundSpeed();
    long heading = myGNSS.getHeading();
    if (speed < 0 || heading < 0 || heading > 360000) { // Speed in mm/s, heading in degrees * 10^-5
        Serial.println("Invalid speed or heading values");
        return false;
    }
    
    return true;
}

bool validateFileOperation(bool result, const char* errorMsg, int errorCode) {
    if (!result) {
        Serial.println(errorMsg);
        if (errorCode != 0) {
            Serial.print("Error code: ");
            Serial.println(errorCode);
        }
        return false;
    }
    return true;
}

bool handleCommand(String command) {
    // Handle Iridium-specific commands
    if (command == "SQ") {
        // Check signal quality with debug info
        testCharacteristic.writeValue("Checking Iridium signal quality...");
        delay(DELAY_BETWEEN_PACKETS); // Ensure this message has a chance to be sent
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
            
            // Validate data before transmitting
            if (!checkDataIntegrity(dataBuffer, transmitSize)) {
                testCharacteristic.writeValue("Error: Data validation failed. Cannot transmit.");
                delete[] dataBuffer;
                return true;
            }
            
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
        
        // Validate message before transmitting
        if (!checkDataIntegrity(messageBuffer, messageLength)) {
            testCharacteristic.writeValue("Error: Message validation failed. Cannot transmit.");
            delete[] messageBuffer;
            return true;
        }
        
        // Transmit via Iridium
        if (transmitViaIridium(messageBuffer, messageLength)) {
            testCharacteristic.writeValue("Message transmitted successfully!");
        } else {
            testCharacteristic.writeValue("Message transmission failed");
        }
        
        delete[] messageBuffer;
        return true;
    } else if (command == "VD") {
        // New command to manually validate data file
        testCharacteristic.writeValue("Validating data file integrity...");
        
        // Close the file if it's open
        if (dataFile) {
            dataFile.close();
        }
        
        File f = SD.open(currentFileName, FILE_READ);
        if (!f) {
            testCharacteristic.writeValue("Error: Could not open data file");
            return true;
        }
        
        // Count total lines and valid records
        int totalLines = 0;
        int validRecords = 0;
        String line;
        
        while (f.available()) {
            char c = f.read();
            if (c == '\n') {
                totalLines++;
                
                // Skip header line
                if (totalLines == 1) {
                    line = "";
                    continue;
                }
                
                // Check if line has expected format (4 CSV fields)
                int commaCount = 0;
                for (unsigned int i = 0; i < line.length(); i++) {
                    if (line[i] == ',') commaCount++;
                }
                
                if (commaCount >= 3) {
                    validRecords++;
                }
                
                line = "";
            } else {
                line += c;
            }
        }
        
        f.close();
        
        // Report validation results
        String validationResult = "Data file validation: " + String(validRecords) + " valid records out of " + 
                                String(totalLines - 1) + " total records.";
        testCharacteristic.writeValue(validationResult);
        Serial.println(validationResult);
        
        if (validRecords < totalLines - 1) {
            testCharacteristic.writeValue("Warning: Some records may be corrupt");
        } else {
            testCharacteristic.writeValue("All records validated successfully");
        }
        
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
    // Validate data integrity before transmission
    if (!checkDataIntegrity(data, dataSize)) {
        Serial.println("Data integrity check failed. Aborting transmission.");
        return false;
    }
    
    // Cap at maximum payload size
    if (dataSize > MAX_BUFFER_SIZE) {
        dataSize = MAX_BUFFER_SIZE;
    }
    
    // Create a buffer for potential responses
    uint8_t rxBuffer[50];
    size_t rxBufferSize = sizeof(rxBuffer);
    
    // Track transmission time
    unsigned long startTime = millis();
    
    // Add retry mechanism for failed transmissions
    int retries = 0;
    int err = ISBD_ERROR_DEFAULT;
    
    // Show initial progress indicator
    updateTransmissionProgress(connectedCentral, 5, "Starting transmission...");
    
    while (retries < CONNECTION_RETRIES) {
        // Check signal quality before attempting transmission
        int signalQuality = -1;
        
        // Update progress - checking signal quality
        updateTransmissionProgress(connectedCentral, 10, "Checking signal quality...");
        modem.getSignalQuality(signalQuality);
        
        if (signalQuality < MIN_SIGNAL_QUALITY) {
            Serial.print("Signal quality too low for reliable transmission: ");
            Serial.print(signalQuality);
            Serial.println("/5. Waiting before retry...");
            updateTransmissionProgress(connectedCentral, 15, "Signal low (" + String(signalQuality) + "/5). Waiting...");
            delay(5000); // Wait before retry
            retries++;
            continue;
        }
        
        // Update progress - preparing to send
        updateTransmissionProgress(connectedCentral, 25, "Signal good (" + String(signalQuality) + "/5). Preparing data...");
        
        // Send the data - this is where most of the time is spent
        updateTransmissionProgress(connectedCentral, 30, "Connecting to satellite...");
        
        // Start a timer to update progress during the potentially long sendReceiveSBDBinary operation
        unsigned long lastProgressUpdate = millis();
        bool isCancelled = false;
        
        // Set up a thread-safe way to update progress during the long operation
        auto progressCallback = [&lastProgressUpdate, &isCancelled]() {
            // Check if it's time for a progress update (every 3 seconds)
            if (millis() - lastProgressUpdate > 3000) {
                // Calculate a new progress value that increases gradually from 30 to 90
                int progressStage = (millis() - lastProgressUpdate) / 3000;
                int newProgress = 30 + min(progressStage * 5, 60); // Cap at 90%
                
                updateTransmissionProgress(connectedCentral, newProgress, "Transmitting data...");
                lastProgressUpdate = millis();
                
                // Check if user wants to cancel (could check for a button press or a BLE command)
                if (testCharacteristic.written() && testCharacteristic.value() == "CANCEL") {
                    isCancelled = true;
                    return false; // Signal to abort
                }
            }
            return true; // Continue operation
        };
        
        // Set the callback before transmission
        // Note: This is conceptual. The actual implementation depends on if the IridiumSBD library 
        // supports callbacks or interrupt-based progress updates
        // modem.setCallback(progressCallback);
        
        // Send data with periodic progress updates
        updateTransmissionProgress(connectedCentral, 40, "Initiating satellite transmission...");
        err = modem.sendReceiveSBDBinary(data, dataSize, rxBuffer, rxBufferSize);
        
        // If the transmission was successful
        if (err == ISBD_SUCCESS) {
            break; // Transmission successful
        }
        
        // Log the specific error
        logError("Iridium transmission", err);
        
        // Update progress with error information
        updateTransmissionProgress(connectedCentral, 40, "Error: " + String(err) + ". Retrying...");
        
        // Determine if error is recoverable
        if (err == ISBD_SENDRECEIVE_TIMEOUT || err == ISBD_PROTOCOL_ERROR) {
            retries++;
            Serial.print("Retrying transmission (");
            Serial.print(retries);
            Serial.print("/");
            Serial.print(CONNECTION_RETRIES);
            Serial.println(")...");
            delay(5000); // Wait before retry
        } else {
            // Non-recoverable error
            Serial.println("Non-recoverable error. Aborting transmission.");
            updateTransmissionProgress(connectedCentral, 0, "Failed: Unrecoverable error " + String(err));
            break;
        }
    }
    
    // Calculate transmission time
    unsigned long transmitTime = millis() - startTime;
    
    if (err == ISBD_SUCCESS) {
        Serial.print("Data transmitted successfully in ");
        Serial.print(transmitTime / 1000.0);
        Serial.println(" seconds");
        
        // Final progress update - success
        updateTransmissionProgress(connectedCentral, 100, "Success! Transmitted in " + String(transmitTime / 1000.0) + "s");
        
        return true;
    } else {
        Serial.print("Iridium transmission failed with error code: ");
        Serial.println(err);
        
        // Final progress update - failure
        updateTransmissionProgress(connectedCentral, 0, "Failed after " + String(transmitTime / 1000.0) + "s (Error: " + String(err) + ")");
        
        return false;
    }
}

bool checkDataIntegrity(const uint8_t* data, size_t dataSize) {
    // Basic validation - ensure data isn't empty or NULL
    if (data == NULL || dataSize == 0) {
        Serial.println("Invalid data: NULL or empty");
        return false;
    }
    
    // Check for valid ASCII characters if this is text data
    // Skip for binary data as needed
    bool isTextData = true;
    for (size_t i = 0; i < dataSize; i++) {
        // Check if character is outside printable ASCII range and not control character
        if ((data[i] < 32 || data[i] > 126) && 
            data[i] != '\r' && data[i] != '\n' && data[i] != '\t') {
            isTextData = false;
            break;
        }
    }
    
    if (isTextData) {
        // For text data, check that we have line delimiters and valid CSV format
        bool hasDelimiter = false;
        for (size_t i = 0; i < dataSize; i++) {
            if (data[i] == ',' || data[i] == '\n') {
                hasDelimiter = true;
                break;
            }
        }
        
        if (!hasDelimiter) {
            Serial.println("CSV data validation failed: No delimiters found");
            return false;
        }
    }
    
    return true;
}

int calculateChecksum(const uint8_t* data, size_t dataSize) {
    // Simple checksum algorithm (CRC-8)
    int crc = 0;
    for (size_t i = 0; i < dataSize; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc & 0xFF;
}

void logError(const char* context, int errorCode) {
    Serial.print("ERROR in ");
    Serial.print(context);
    Serial.print(": Code ");
    Serial.print(errorCode);
    
    // Translate error codes to human-readable messages
    switch (errorCode) {
        case ISBD_SUCCESS:
            Serial.println(" (Success)");
            break;
        case ISBD_ALREADY_AWAKE:
            Serial.println(" (Device already awake)");
            break;
        case ISBD_SERIAL_FAILURE:
            Serial.println(" (Serial communication failure)");
            break;
        case ISBD_PROTOCOL_ERROR:
            Serial.println(" (Protocol error)");
            break;
        case ISBD_CANCELLED:
            Serial.println(" (Cancelled by user)");
            break;
        case ISBD_NO_MODEM_DETECTED:
            Serial.println(" (No modem detected)");
            break;
        case ISBD_SBDIX_FATAL_ERROR:
            Serial.println(" (SBDIX fatal error)");
            break;
        case ISBD_SENDRECEIVE_TIMEOUT:
            Serial.println(" (Send/receive timeout)");
            break;
        case ISBD_RX_OVERFLOW:
            Serial.println(" (RX buffer overflow)");
            break;
        case ISBD_REENTRANT:
            Serial.println(" (Reentrant error)");
            break;
        case ISBD_IS_ASLEEP:
            Serial.println(" (Device is asleep)");
            break;
        case ISBD_NO_SLEEP_PIN:
            Serial.println(" (No sleep pin defined)");
            break;
        case ISBD_NO_NETWORK:
            Serial.println(" (No network available)");
            break;
        case ISBD_MSG_TOO_LONG:
            Serial.println(" (Message too long)");
            break;
        default:
            Serial.println(" (Unknown error)");
            break;
    }
    
    // If we have a BLE connection, also send error via BLE
    if (testCharacteristic.subscribed()) {
        char errorMessage[60];
        snprintf(errorMessage, sizeof(errorMessage), "Error in %s: Code %d", context, errorCode);
        testCharacteristic.writeValue(errorMessage);
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

// Implementation of updateTransmissionProgress
void updateTransmissionProgress(BLEDevice central, int progress, const String& statusMsg) {
    // Update the global progress value
    transmissionProgressValue = progress;
    
    // Call the transmissionProgress function to display the progress
    transmissionProgress(central, progress, statusMsg);
}

// Implementation of the transmissionProgress function
void transmissionProgress(BLEDevice central, int progress, const String& statusMsg) {
    // Format progress message
    String progressMsg = "Progress: [";
    
    // Create a simple progress bar
    const int totalBars = 10;
    int filledBars = (progress * totalBars) / 100;
    
    for (int i = 0; i < totalBars; i++) {
        if (i < filledBars) {
            progressMsg += "=";
        } else {
            progressMsg += " ";
        }
    }
    
    progressMsg += "] " + String(progress) + "%";
    
    // Add status message if provided
    if (statusMsg.length() > 0) {
        progressMsg += " - " + statusMsg;
    }
    
    // Send the progress update to the BLE client
    testCharacteristic.writeValue(progressMsg);
    delay(DELAY_BETWEEN_PACKETS);
}

void sendMessage(const String& message) {
    // Use the BLE characteristic to send a message
    testCharacteristic.writeValue(message);
    delay(DELAY_BETWEEN_PACKETS);  // Add a delay to ensure reliable transmission
}

void listFiles(BLEDevice central) {
    File root = SD.open("/");
    if (!root) {
        testCharacteristic.writeValue(getFailureMessage(
            "read directory",
            "SD card may be missing or corrupted",
            "Check SD card is properly inserted"));
        return;
    }
    
    int fileCount = 0;
    size_t totalSize = 0;
    
    testCharacteristic.writeValue("=== Files on SD Card ===");
    
    File entry;
    while ((entry = root.openNextFile()) && testCharacteristic.subscribed()) {
        if (!entry.isDirectory()) {
            fileCount++;
            size_t fileSize = entry.size();
            totalSize += fileSize;
            
            // Format file size in a readable way
            String sizeStr;
            if (fileSize > 1000000) {
                sizeStr = String(fileSize / 1000000.0, 1) + " MB";
            } else if (fileSize > 1000) {
                sizeStr = String(fileSize / 1000.0, 1) + " KB";
            } else {
                sizeStr = String(fileSize) + " bytes";
            }
            
            String filename = entry.name();
            String fileInfo = filename + " (" + sizeStr + ")";
            
            if (testCharacteristic.writeValue(fileInfo) == 0) {
                Serial.println("Write to characteristic failed");
            }
            delay(DELAY_BETWEEN_PACKETS);
        }
        entry.close();
    }
    
    root.close();
    
    // Format total size
    String totalSizeStr;
    if (totalSize > 1000000) {
        totalSizeStr = String(totalSize / 1000000.0, 1) + " MB";
    } else if (totalSize > 1000) {
        totalSizeStr = String(totalSize / 1000.0, 1) + " KB";
    } else {
        totalSizeStr = String(totalSize) + " bytes";
    }
    
    if (fileCount > 0) {
        testCharacteristic.writeValue("=== Total: " + String(fileCount) + " files (" + totalSizeStr + ") ===");
        testCharacteristic.writeValue("→ TIP: Type a filename to view contents, or 'VD' to validate data files");
    } else {
        testCharacteristic.writeValue("No files found on SD card");
        testCharacteristic.writeValue("→ TIP: Use 'S' to start data collection");
    }
} 