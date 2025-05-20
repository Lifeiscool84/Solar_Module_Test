#include <Arduino.h> // Include the main Arduino library for core functions and types
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_INA228.h>
#include <stdio.h> // Required for snprintf
#include <math.h>  // For isnan, isinf

// --- Pin Definitions ---
// Define the Chip Select pin for your SD card module
// D0 is common for ESP8266 (maps to GPIO16). For standard Arduinos (Uno, Mega, etc.) use 10 or 4.
// For ESP32, common pins are 5, 15, 22 etc. depending on the board.
//D0 is correct for the Artemis MicroMod
const int chipSelect = D0;

// --- INA228 Configuration ---
const uint8_t INA228_ADDRESS = 0x40; // I2C address of the INA228 sensor

// --- Logging Configuration ---
const char* LOG_FILENAME = "Current.txt"; // File name on the SD card
const unsigned long LOG_INTERVAL = 2000; // Log data every 2000 ms (2 seconds)

// --- Global Objects ---
Adafruit_INA228 ina228 = Adafruit_INA228(); // Create an instance of the INA228 library
File dataFile; // File object for SD card operations

// --- Timing Variable ---
unsigned long lastLogTime = 0; // Stores the time (in ms) of the last log event

// --- Custom Float to String Conversion ---
/**
 * Convert a float to string with given precision.
 * bufferSize must be large enough to hold result plus null terminator.
 */
void floatToChar(float value, int decimals, char* buffer, int bufferSize) {
  if (!buffer || bufferSize < 2) {
    return; // Invalid buffer
  }

  // Handle special float cases
  if (isnan(value)) {
    strncpy(buffer, "nan", bufferSize -1); // Leave space for null term
    buffer[bufferSize - 1] = '\0';
    return;
  }
  if (isinf(value)) {
    if (value > 0) strncpy(buffer, "inf", bufferSize -1);
    else           strncpy(buffer, "-inf", bufferSize -1);
    buffer[bufferSize - 1] = '\0';
    return;
  }

  bool negative = (value < 0.0f);
  if (negative) {
    value = -value;  // Work with positive magnitude
  }

  // Extract integer part
  // Use double for intermediate calculations for better precision handling
  double valueDouble = (double)value;
  unsigned long intPart = (unsigned long)(valueDouble);
  double fractional = valueDouble - (double)intPart;

  // Construct integer portion into buffer using Arduino String temporarily
  String temp = negative ? "-" + String(intPart) : String(intPart);

  // If decimals > 0, process fractional part
  if (decimals > 0) {
    temp += ".";

    // Scale fractional up to "decimals" places
    // Using pow() might be more robust than repeated multiplication for precision
    double scaledFractional = fractional * pow(10.0, decimals);
    unsigned long fracInt = (unsigned long)(scaledFractional + 0.5); // Round

    // Build zero-padded fractional part
    String fracString = String(fracInt);
    String padding = "";
    for (int i = fracString.length(); i < decimals; ++i) {
        padding += "0";
    }
    temp += padding;
    temp += fracString;
  }

  // Copy result into the provided buffer
  strncpy(buffer, temp.c_str(), bufferSize - 1);
  // Ensure null terminator
  buffer[bufferSize - 1] = '\0'; 
}

// --- Function Prototypes (Standard Arduino structure doesn't strictly require these here) ---
void setup();
void loop();

// --- Setup Function ---
void setup() {
  // Initialize Serial communication
  Serial.begin(115200);
  unsigned long startMillis = millis();
   while (!Serial && (millis() - startMillis < 5000)) { // Wait max 5 seconds for serial
    delay(100);
  }
  Serial.println("\n--- INA228 SD Card Logger ---");

  // Initialize SD Card Chip Select pin
  pinMode(chipSelect, OUTPUT);
  digitalWrite(chipSelect, HIGH); // Deselect card initially
  delay(10);

  // Initialize SD card
  Serial.print("Initializing SD card (CS Pin: "); Serial.print(chipSelect); Serial.print(")...");
  if (!SD.begin(chipSelect)) {
    Serial.println(" Card failed, or not present! Check wiring and card. Halting.");
    while (1) delay(1); // Stop execution
  }
  Serial.println(" SD card initialized.");

  // Initialize I2C communication
  Wire.begin();

  // Initialize INA228 Sensor
  Serial.print("Initializing INA228 sensor at I2C address 0x");
  Serial.print(INA228_ADDRESS, HEX);
  Serial.print("...");
  if (!ina228.begin(INA228_ADDRESS)) {
    Serial.println(" Failed to find INA228 chip! Check wiring and address. Halting.");
    while (1) delay(1); // Stop execution
  }
  Serial.println(" INA228 sensor found.");

  // *** ADDED: Set shunt resistor value and max expected current ***
  // IMPORTANT: Replace 0.015 with your actual shunt resistance in Ohms
  // IMPORTANT: Replace 5.0 with a realistic maximum current in Amps for your setup
  float shunt_res_ohms = 0.015; 
  float max_expected_amps = 5.0; 
  Serial.print("Setting shunt resistor = "); Serial.print(shunt_res_ohms, 3); Serial.print(" Ohms, Max current = "); Serial.print(max_expected_amps); Serial.println(" A");
  ina228.setShunt(shunt_res_ohms, max_expected_amps);

  // Configure INA228 Sensor Settings
  Serial.println("Configuring INA28 settings:");

  // Set Averaging Count (Number of samples averaged for each measurement)
  ina228.setAveragingCount(INA228_COUNT_256);
  Serial.print("  - Averaging Count set to: "); Serial.println(ina228.getAveragingCount());

  // Set ADC Conversion Time for both Shunt Current and Bus Voltage
  // Note: The library requires setting these individually.
  ina228.setCurrentConversionTime(INA228_TIME_2074_us);
  ina228.setVoltageConversionTime(INA228_TIME_2074_us);
  // Verify the settings by reading them back
  Serial.print("  - Current Conversion Time set to: "); Serial.print(ina228.getCurrentConversionTime() * 1.0); Serial.println(" enum value"); // Print enum value
  Serial.print("  - Voltage Conversion Time set to: "); Serial.print(ina228.getVoltageConversionTime() * 1.0); Serial.println(" enum value"); // Print enum value
  // Note: To print the actual microsecond value, you'd need a mapping function or lookup table
  // based on the ina228_conv_time_t enum definition.

  // Check if the log file exists, if not, create it and write the header
  Serial.print("Checking/Preparing log file: "); Serial.println(LOG_FILENAME);
  if (!SD.exists(LOG_FILENAME)) {
    Serial.println("  Log file not found. Creating and writing header...");
    // Use FILE_WRITE for initial creation/overwrite if header needed
    dataFile = SD.open(LOG_FILENAME, FILE_WRITE); 
    if (dataFile) {
      // Write header row (CSV format) - Use print for pieces, println for final newline
      // Corrected units based on library implementation and data analysis
      dataFile.print("Timestamp(s),Current(mA),BusVoltage(V),ShuntVoltage(mV),Power(mW),Energy(J),DieTemp(C)"); 
      dataFile.println(); // Add newline
      dataFile.close(); // Close the file immediately after writing header
      Serial.println("  Header written successfully.");
    } else {
      Serial.println("  ERROR: Could not create log file! Check SD card. Halting.");
       while (1) delay(1); // Stop execution
    }
  } else {
      Serial.println("  Log file found. Appending data.");
  }

  Serial.println("Setup complete. Starting measurements every ");
  Serial.print(LOG_INTERVAL / 1000.0); Serial.println(" seconds.");
  lastLogTime = millis(); // Initialize last log time to start logging immediately
}

// --- Loop Function ---
void loop() {
  // Check if it's time to log data based on the interval
  unsigned long currentTime = millis();
  if (currentTime - lastLogTime >= LOG_INTERVAL) {
    lastLogTime = currentTime; // Update the last log time for the next interval

    // --- Read Measurements from INA228 ---
    float current_A = ina228.readCurrent();
    float busVoltage_V = ina228.readBusVoltage();
    float shuntVoltage_mV = ina228.readShuntVoltage() * 1000.0f; 
    float power_W = ina228.readPower();
    float energy_J = ina228.readEnergy(); 
    float dieTemp_C = ina228.readDieTemp();

    // --- Prepare and Print Data Piecewise --- 
    char tempBuffer[20]; // Temporary buffer for float-to-string conversion
    unsigned long timestamp_s = currentTime / 1000; // Timestamp in whole seconds

    // --- Print to Serial Monitor --- 
    Serial.print(timestamp_s);
    
    Serial.print(",");
    floatToChar(current_A, 6, tempBuffer, sizeof(tempBuffer)); // Use custom function
    Serial.print(tempBuffer);
    
    Serial.print(",");
    floatToChar(busVoltage_V, 3, tempBuffer, sizeof(tempBuffer)); // Use custom function
    Serial.print(tempBuffer);
    
    Serial.print(",");
    floatToChar(shuntVoltage_mV, 4, tempBuffer, sizeof(tempBuffer)); // Use custom function
    Serial.print(tempBuffer);
    
    Serial.print(",");
    floatToChar(power_W, 4, tempBuffer, sizeof(tempBuffer)); // Keep formatting, but unit changed in header
    Serial.print(tempBuffer);
    
    Serial.print(",");
    floatToChar(energy_J, 4, tempBuffer, sizeof(tempBuffer)); // Use custom function
    Serial.print(tempBuffer);
    
    Serial.print(",");
    floatToChar(dieTemp_C, 2, tempBuffer, sizeof(tempBuffer)); // Use custom function
    Serial.print(tempBuffer);
    
    Serial.println(); // Print final newline

    // --- Log Data to SD Card (Piecewise) ---
    // Use explicit flags for append mode as FILE_APPEND is not defined in this core
    dataFile = SD.open(LOG_FILENAME, O_RDWR | O_CREAT | O_APPEND);
    if (dataFile) {
      dataFile.print(timestamp_s);
      
      dataFile.print(",");
      floatToChar(current_A, 6, tempBuffer, sizeof(tempBuffer)); // Use custom function
      dataFile.print(tempBuffer);
      
      dataFile.print(",");
      floatToChar(busVoltage_V, 3, tempBuffer, sizeof(tempBuffer)); // Use custom function
      dataFile.print(tempBuffer);
      
      dataFile.print(",");
      floatToChar(shuntVoltage_mV, 4, tempBuffer, sizeof(tempBuffer)); // Use custom function
      dataFile.print(tempBuffer);
      
      dataFile.print(",");
      floatToChar(power_W, 4, tempBuffer, sizeof(tempBuffer)); // Keep formatting, but unit changed in header
      dataFile.print(tempBuffer);
      
      dataFile.print(",");
      floatToChar(energy_J, 4, tempBuffer, sizeof(tempBuffer)); // Use custom function
      dataFile.print(tempBuffer);
      
      dataFile.print(",");
      floatToChar(dieTemp_C, 2, tempBuffer, sizeof(tempBuffer)); // Use custom function
      dataFile.print(tempBuffer);
      
      dataFile.println(); // Print final newline

      if (dataFile.getWriteError()) { // Check for write errors specifically
        Serial.println("  ERROR: SD card write error!");
      }

      dataFile.close(); // *** CRITICAL: Close the file immediately to flush data to card ***
    } else {
      Serial.print("  ERROR: Could not open '");
      Serial.print(LOG_FILENAME);
      Serial.println("' for appending! Check SD card.");
      // Consider adding error indication
    }
  }

  // The loop will continue checking the time. 
  // No delay() needed here as the timing is handled by millis() check.
  // This allows the microcontroller to potentially perform other tasks 
  // if needed between logging intervals (though none are added here).
}
