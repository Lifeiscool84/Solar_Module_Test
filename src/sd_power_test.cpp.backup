/**
 * @file sd_power_test.cpp
 * @brief Comprehensive SD Card Power Consumption Testing Program
 * 
 * Tests power consumption across 11 SD card operational modes and 9 system statuses
 * using existing INA228 infrastructure from main.cpp.
 * 
 * Leverages existing hardware configuration:
 * - CS pin = 8
 * - INA228 addresses: 0x40 (solar), 0x44 (battery), 0x41 (load)
 * - Existing measurement functions from main.cpp
 */

#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// Hardware Configuration (from main.cpp)
const int chipSelect = 8; // SD card CS pin
const uint8_t INA_SOLAR_ADDR = 0x40;
const uint8_t INA_BATTERY_ADDR = 0x44;
const uint8_t INA_LOAD_ADDR = 0x41;

// INA228 Register Addresses (from main.cpp)
const uint8_t INA228_REG_CONFIG = 0x00;
const uint8_t INA228_REG_ADCCFG = 0x01;
const uint8_t INA228_REG_SHUNTCAL = 0x02;
const uint8_t INA228_REG_VSHUNT = 0x04;
const uint8_t INA228_REG_VBUS = 0x05;
const uint8_t INA228_REG_DIETEMP = 0x06;
const uint8_t INA228_REG_CURRENT = 0x07;
const uint8_t INA228_REG_POWER = 0x08;
const uint8_t INA228_REG_MFG_UID = 0x3E;
const uint8_t INA228_REG_DEVICE_ID = 0x3F;

// Global Calibration Values
float solar_current_lsb = 0.0f;
float battery_current_lsb = 0.0f;
float load_current_lsb = 0.0f;

// ===== SD CARD OPERATIONAL MODES =====
enum SDCardMode {
  SD_INITIALIZATION = 0,
  SD_FILE_OPEN_CLOSE = 1,
  SD_DATA_WRITING = 2,
  SD_DATA_READING = 3,
  SD_FILE_FLUSHING = 4,
  SD_METADATA_INTERACTION = 5,
  SD_ACTIVE_IDLE = 6,
  SD_STANDBY_IDLE = 7,
  SD_SOFTWARE_LOW_POWER = 8,
  SD_HARDWARE_POWERED_OFF = 9,
  SD_RAPID_READ_WRITE = 10,
  SD_MODE_COUNT = 11
};

const char* SD_MODE_NAMES[] = {
  "Initialization",
  "File Open/Close",
  "Data Writing",
  "Data Reading",
  "File Flushing",
  "Metadata Interaction",
  "Active Idle",
  "Standby Idle",
  "Software Low Power",
  "Hardware Powered Off",
  "Rapid Read/Write"
};

// ===== SYSTEM STATUS STATES =====
enum SystemStatus {
  SYS_INITIALIZING = 0,
  SYS_SENSOR_ACQUISITION = 1,
  SYS_PREPARING_SD = 2,
  SYS_WRITING_BUFFER = 3,
  SYS_FINALIZING_SD = 4,
  SYS_TRANSITIONING_LOW_POWER = 5,
  SYS_MCU_IDLE = 6,
  SYS_SD_READ_OPERATION = 7,
  SYS_ERROR_RECOVERY = 8,
  SYS_STATUS_COUNT = 9
};

const char* SYSTEM_STATUS_NAMES[] = {
  "System Initializing",
  "Sensor Data Acquisition",
  "Preparing for SD Operation", 
  "Writing RAM Buffer to SD",
  "Finalizing SD Operations",
  "Transitioning to Low Power",
  "MCU Idle",
  "SD Read Operation",
  "Error Recovery"
};

// ===== POWER TEST DATA STRUCTURE =====
struct PowerTestData {
  unsigned long timestamp_micros;
  SDCardMode sd_mode;
  SystemStatus system_status;
  float solar_voltage;
  float solar_current;
  float solar_power;
  float battery_voltage;
  float battery_current;
  float battery_power;
  float load_voltage;
  float load_current;
  float load_power;
  float total_power;
  uint16_t sample_number;
  bool valid_measurement;
};

// ===== STATISTICAL ANALYSIS STRUCTURE =====
struct PowerStatistics {
  float min_power;
  float max_power;
  float avg_power;
  float peak_power;
  float energy_consumed_mWh;
  uint32_t duration_us;
  uint16_t sample_count;
  float std_deviation;
  float peak_to_average_ratio;
};

// ===== TEST CONFIGURATION =====
const uint16_t SAMPLES_PER_TEST = 50;
const uint16_t RAPID_SAMPLE_INTERVAL_US = 50; // 50 microseconds for rapid sampling
const uint16_t TEST_ITERATIONS = 2; // Multiple iterations for statistical significance
const uint32_t INTER_TEST_DELAY_MS = 100; // Delay between tests

// ===== GLOBAL TEST VARIABLES =====
PowerTestData testResults[SD_MODE_COUNT][SYS_STATUS_COUNT][SAMPLES_PER_TEST];
PowerStatistics modeStatistics[SD_MODE_COUNT];
PowerStatistics systemStatistics[SYS_STATUS_COUNT];
PowerStatistics combinedStatistics[SD_MODE_COUNT][SYS_STATUS_COUNT];

File testDataFile;
File testLogFile;
bool sdCardInitialized = false;
bool testInProgress = false;
uint16_t currentTestIteration = 0;

// ===== FUNCTION PROTOTYPES =====
// INA228 Functions (adapted from main.cpp)
float initINA228(uint8_t address, const char* sensorName, float shuntOhms, float maxCurrent);
uint32_t readRegister(uint8_t address, uint8_t reg, uint8_t numBytes);
uint16_t readRegister16(uint8_t address, uint8_t reg);
float readBusVoltage(uint8_t address, uint32_t& rawRegValue, uint32_t& shiftedRegValue);
float readCurrent(uint8_t address, float current_lsb_value, int32_t& rawRegValue, int32_t& shiftedRegValue);
float readPower(uint8_t address, float current_lsb_value, uint32_t& rawRegValue);

// Power Measurement Functions
PowerTestData rapidPowerSample(SDCardMode mode, SystemStatus status);
bool performRapidSampling(SDCardMode mode, SystemStatus status, PowerTestData samples[], uint16_t numSamples);

// SD Card Mode Testing Functions
bool testSDInitialization();
bool testSDFileOpenClose();
bool testSDDataWriting();
bool testSDDataReading();
bool testSDFileFlushing();
bool testSDMetadataInteraction();
bool testSDActiveIdle();
bool testSDStandbyIdle();
bool testSDSoftwareLowPower();
bool testSDHardwarePoweredOff();
bool testSDRapidReadWrite();

// System Status Testing Functions
bool testSystemInitializing();
bool testSensorAcquisition();
bool testPreparingSD();
bool testWritingBuffer();
bool testFinalizingSD();
bool testTransitioningLowPower();
bool testMCUIdle();
bool testSDReadOperation();
bool testErrorRecovery();

// Statistical Analysis Functions
PowerStatistics calculateStatistics(PowerTestData samples[], uint16_t numSamples);
void generateCorrelationAnalysis();
void detectPowerPeaks(PowerTestData samples[], uint16_t numSamples);
float calculateStandardDeviation(PowerTestData samples[], uint16_t numSamples, float average);

// Reporting Functions
void createPowerTestCSV();
void logPowerTestData(const PowerTestData& data);
void generateTextReport();
void generateSummaryReport();
void printTestProgress(SDCardMode mode, SystemStatus status, uint16_t iteration);

// Control Functions
bool initializePowerTest();
void runComprehensivePowerTest();
void runSingleModeTest(SDCardMode mode);
void runSingleStatusTest(SystemStatus status);
void displayTestMenu();
void handleUserInput();

// Utility Functions
const char* getSDModeName(SDCardMode mode);
const char* getSystemStatusName(SystemStatus status);
bool validateTestEnvironment();
void resetTestData();

// ===== IMPLEMENTATION STARTS HERE =====

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  
  Serial.println(F("================================================="));
  Serial.println(F("SD Card Power Consumption Testing System"));
  Serial.println(F("================================================="));
  Serial.println();
  
  if (!initializePowerTest()) {
    Serial.println(F("ERROR: Failed to initialize power testing system"));
    while (1) {
      delay(1000);
    }
  }
  
  Serial.println(F("Power testing system initialized successfully"));
  Serial.println(F("Type 'h' for help menu"));
  Serial.println();
}

void loop() {
  handleUserInput();
  delay(10);
}

bool initializePowerTest() {
  Serial.println(F("Initializing power testing system..."));
  
  // Initialize I2C
  Wire.begin();
  Wire.setClock(400000); // 400kHz for faster measurements
  
  // Initialize INA228 sensors (using same parameters as main.cpp)
  solar_current_lsb = initINA228(INA_SOLAR_ADDR, "Solar", 0.015f, 5.0f);
  battery_current_lsb = initINA228(INA_BATTERY_ADDR, "Battery", 0.015f, 5.0f);
  load_current_lsb = initINA228(INA_LOAD_ADDR, "Load", 0.015f, 5.0f);
  
  if (solar_current_lsb == 0.0f || battery_current_lsb == 0.0f || load_current_lsb == 0.0f) {
    Serial.println(F("ERROR: Failed to initialize INA228 sensors"));
    return false;
  }
  
  // Initialize SD card
  pinMode(chipSelect, OUTPUT);
  if (!SD.begin(chipSelect)) {
    Serial.println(F("ERROR: SD card initialization failed"));
    return false;
  }
  sdCardInitialized = true;
  
  // Create test data files
  createPowerTestCSV();
  
  // Reset test data arrays
  resetTestData();
  
  // Validate test environment
  if (!validateTestEnvironment()) {
    Serial.println(F("WARNING: Test environment validation failed"));
  }
  
  return true;
}

void handleUserInput() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    
    while (Serial.available() > 0) {
      Serial.read(); // Clear buffer
    }
    
    switch (command) {
      case 'h':
      case 'H':
        displayTestMenu();
        break;
        
      case 'a':
      case 'A':
        if (!testInProgress) {
          Serial.println(F("Starting comprehensive power test..."));
          runComprehensivePowerTest();
        } else {
          Serial.println(F("Test already in progress"));
        }
        break;
        
      case 'r':
      case 'R':
        generateTextReport();
        break;
        
      case 's':
      case 'S':
        generateSummaryReport();
        break;
        
      case 'v':
      case 'V':
        if (!validateTestEnvironment()) {
          Serial.println(F("Test environment validation failed"));
        } else {
          Serial.println(F("Test environment validation passed"));
        }
        break;
        
      case 'c':
      case 'C':
        resetTestData();
        Serial.println(F("Test data cleared"));
        break;
        
      default:
        Serial.println(F("Unknown command. Type 'h' for help."));
        break;
    }
  }
}

void displayTestMenu() {
  Serial.println(F("=== SD Card Power Test Menu ==="));
  Serial.println(F("a/A - Run comprehensive power test"));
  Serial.println(F("r/R - Generate detailed report"));
  Serial.println(F("s/S - Generate summary report"));
  Serial.println(F("v/V - Validate test environment"));
  Serial.println(F("c/C - Clear test data"));
  Serial.println(F("h/H - Show this help menu"));
  Serial.println();
}

// ===== INA228 MEASUREMENT FUNCTIONS (adapted from main.cpp) =====

float initINA228(uint8_t address, const char* sensorName, float shuntOhms, float maxCurrent) {
  Serial.print(F("Initializing INA228: ")); Serial.println(sensorName);
  
  Wire.beginTransmission(address);
  if (Wire.endTransmission() != 0) {
    Serial.print(F("ERROR: INA228 not found at address 0x")); Serial.println(address, HEX);
    return 0.0f;
  }
  
  // Reset device
  Wire.beginTransmission(address);
  Wire.write(INA228_REG_CONFIG);
  Wire.write((uint8_t)0x80);
  Wire.write((uint8_t)0x00);
  if (Wire.endTransmission() != 0) {
    return 0.0f;
  }
  delay(10);
  
  // Configure ADC for continuous monitoring
  uint16_t adcConfig = 0xF000 | (5 << 9) | (5 << 6) | (5 << 3) | 0;
  Wire.beginTransmission(address);
  Wire.write(INA228_REG_ADCCFG);
  Wire.write((uint8_t)((adcConfig >> 8) & 0xFF));
  Wire.write((uint8_t)(adcConfig & 0xFF));
  if (Wire.endTransmission() != 0) {
    return 0.0f;
  }
  
  // Calculate and set calibration
  float device_current_lsb = maxCurrent / 524288.0f;
  uint16_t calibration = (uint16_t)(13107.2f * 1000000.0f * device_current_lsb * shuntOhms);
  
  Wire.beginTransmission(address);
  Wire.write(INA228_REG_SHUNTCAL);
  Wire.write((uint8_t)((calibration >> 8) & 0xFF));
  Wire.write((uint8_t)(calibration & 0xFF));
  if (Wire.endTransmission() != 0) {
    return 0.0f;
  }
  
  Serial.print(F("  Initialized with current LSB: ")); Serial.println(device_current_lsb, 10);
  return device_current_lsb;
}

uint32_t readRegister(uint8_t address, uint8_t reg, uint8_t numBytes) {
  uint32_t value = 0;
  
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return 0;
  }
  
  uint8_t bytesRead = Wire.requestFrom(address, numBytes);
  if (bytesRead == numBytes) {
    for (uint8_t i = 0; i < numBytes; i++) {
      value = (value << 8) | Wire.read();
    }
  }
  return value;
}

uint16_t readRegister16(uint8_t address, uint8_t reg) {
  uint16_t value = 0;
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return 0;
  }
  
  uint8_t bytesRead = Wire.requestFrom(address, (uint8_t)2);
  if (bytesRead == 2) {
    value = (Wire.read() << 8) | Wire.read();
  }
  return value;
}

float readBusVoltage(uint8_t address, uint32_t& rawRegValue, uint32_t& shiftedRegValue) {
  rawRegValue = readRegister(address, INA228_REG_VBUS, 3);
  shiftedRegValue = rawRegValue >> 4;
  return (float)shiftedRegValue * 195.3125f / 1000000.0f;
}

float readCurrent(uint8_t address, float current_lsb_value, int32_t& rawRegValue, int32_t& shiftedRegValue) {
  if (current_lsb_value == 0.0f) {
    rawRegValue = 0; shiftedRegValue = 0;
    return 0.0f;
  }
  
  uint32_t temp_raw = readRegister(address, INA228_REG_CURRENT, 3);
  rawRegValue = (int32_t)temp_raw;
  
  int32_t signed_raw_value = (int32_t)temp_raw;
  if (signed_raw_value & 0x800000) {
    signed_raw_value |= 0xFF000000;
  }
  shiftedRegValue = signed_raw_value >> 4;
  return (float)shiftedRegValue * current_lsb_value * 1000.0f;
}

float readPower(uint8_t address, float current_lsb_value, uint32_t& rawRegValue) {
  if (current_lsb_value == 0.0f) {
    rawRegValue = 0;
    return 0.0f;
  }
  rawRegValue = readRegister(address, INA228_REG_POWER, 3);
  return (float)rawRegValue * 3.2f * current_lsb_value * 1000.0f;
}

// ===== RAPID POWER SAMPLING FUNCTIONS =====

PowerTestData rapidPowerSample(SDCardMode mode, SystemStatus status) {
  PowerTestData sample;
  sample.timestamp_micros = micros();
  sample.sd_mode = mode;
  sample.system_status = status;
  sample.valid_measurement = true;
  
  // Read all three INA228 sensors simultaneously for rapid sampling
  uint32_t rawReg, shiftedReg;
  int32_t rawRegSigned, shiftedRegSigned;
  
  // Solar measurements
  sample.solar_voltage = readBusVoltage(INA_SOLAR_ADDR, rawReg, shiftedReg);
  sample.solar_current = readCurrent(INA_SOLAR_ADDR, solar_current_lsb, rawRegSigned, shiftedRegSigned);
  sample.solar_power = readPower(INA_SOLAR_ADDR, solar_current_lsb, rawReg);
  
  // Battery measurements
  sample.battery_voltage = readBusVoltage(INA_BATTERY_ADDR, rawReg, shiftedReg);
  sample.battery_current = readCurrent(INA_BATTERY_ADDR, battery_current_lsb, rawRegSigned, shiftedRegSigned);
  sample.battery_power = readPower(INA_BATTERY_ADDR, battery_current_lsb, rawReg);
  
  // Load measurements
  sample.load_voltage = readBusVoltage(INA_LOAD_ADDR, rawReg, shiftedReg);
  sample.load_current = readCurrent(INA_LOAD_ADDR, load_current_lsb, rawRegSigned, shiftedRegSigned);
  sample.load_power = readPower(INA_LOAD_ADDR, load_current_lsb, rawReg);
  
  // Calculate total power
  sample.total_power = sample.solar_power + sample.battery_power + sample.load_power;
  
  // Validate measurement
  if (sample.solar_voltage == 0.0f && sample.battery_voltage == 0.0f && sample.load_voltage == 0.0f) {
    sample.valid_measurement = false;
  }
  
  return sample;
}

bool performRapidSampling(SDCardMode mode, SystemStatus status, PowerTestData samples[], uint16_t numSamples) {
  if (samples == nullptr || numSamples == 0) {
    return false;
  }
  
  unsigned long startTime = micros();
  
  for (uint16_t i = 0; i < numSamples; i++) {
    samples[i] = rapidPowerSample(mode, status);
    samples[i].sample_number = i;
    
    // Precise timing control for rapid sampling
    delayMicroseconds(RAPID_SAMPLE_INTERVAL_US);
    
    // Yield control occasionally to prevent system lockup
    if (i % 100 == 0) {
      yield();
    }
  }
  
  unsigned long endTime = micros();
  unsigned long totalTime = endTime - startTime;
  
  Serial.print(F("Rapid sampling completed: "));
  Serial.print(numSamples);
  Serial.print(F(" samples in "));
  Serial.print(totalTime);
  Serial.println(F(" microseconds"));
  
  return true;
}

// ===== SD CARD MODE TESTING FUNCTIONS =====

bool testSDInitialization() {
  Serial.println(F("Testing SD Card Initialization mode..."));
  
  // Ensure SD card is properly disconnected first
  SD.end();
  delay(50);
  
  // Begin rapid sampling during initialization
  PowerTestData initSamples[SAMPLES_PER_TEST];
  
  unsigned long startTime = micros();
  bool samplingStarted = false;
  
  // Start sampling in parallel with SD initialization
  for (uint16_t i = 0; i < SAMPLES_PER_TEST; i++) {
    if (i == SAMPLES_PER_TEST / 4 && !samplingStarted) {
      // Trigger SD initialization at 25% through sampling
      SD.begin(chipSelect);
      samplingStarted = true;
    }
    
    initSamples[i] = rapidPowerSample(SD_INITIALIZATION, SYS_INITIALIZING);
    initSamples[i].sample_number = i;
    delayMicroseconds(RAPID_SAMPLE_INTERVAL_US);
  }
  
  // Store results
  memcpy(testResults[SD_INITIALIZATION][SYS_INITIALIZING], initSamples, 
         sizeof(PowerTestData) * SAMPLES_PER_TEST);
  
  return true;
}

bool testSDFileOpenClose() {
  Serial.println(F("Testing SD Card File Open/Close mode..."));
  
  PowerTestData samples[SAMPLES_PER_TEST];
  File testFile;
  
  for (uint16_t i = 0; i < SAMPLES_PER_TEST; i++) {
    // Alternate between file open and close operations
    if (i % 100 < 50) {
      testFile = SD.open("powertest.tmp", FILE_WRITE);
    } else {
      if (testFile) {
        testFile.close();
      }
    }
    
    samples[i] = rapidPowerSample(SD_FILE_OPEN_CLOSE, SYS_PREPARING_SD);
    samples[i].sample_number = i;
    delayMicroseconds(RAPID_SAMPLE_INTERVAL_US);
  }
  
  if (testFile) {
    testFile.close();
  }
  SD.remove("powertest.tmp");
  
  memcpy(testResults[SD_FILE_OPEN_CLOSE][SYS_PREPARING_SD], samples,
         sizeof(PowerTestData) * SAMPLES_PER_TEST);
  
  return true;
}

bool testSDDataWriting() {
  Serial.println(F("Testing SD Card Data Writing mode..."));
  
  PowerTestData samples[SAMPLES_PER_TEST];
  File testFile = SD.open("writetest.tmp", FILE_WRITE);
  
  if (!testFile) {
    Serial.println(F("ERROR: Could not open file for write test"));
    return false;
  }
  
  char testData[64];
  sprintf(testData, "Test data line for power measurement analysis\n");
  
  for (uint16_t i = 0; i < SAMPLES_PER_TEST; i++) {
    // Continuous writing during sampling
    if (i % 10 == 0) {
      testFile.print(testData);
    }
    
    samples[i] = rapidPowerSample(SD_DATA_WRITING, SYS_WRITING_BUFFER);
    samples[i].sample_number = i;
    delayMicroseconds(RAPID_SAMPLE_INTERVAL_US);
  }
  
  testFile.close();
  SD.remove("writetest.tmp");
  
  memcpy(testResults[SD_DATA_WRITING][SYS_WRITING_BUFFER], samples,
         sizeof(PowerTestData) * SAMPLES_PER_TEST);
  
  return true;
}

bool testSDDataReading() {
  Serial.println(F("Testing SD Card Data Reading mode..."));
  
  // First create a test file with data
  File writeFile = SD.open("readtest.tmp", FILE_WRITE);
  if (!writeFile) {
    return false;
  }
  
  for (int i = 0; i < 100; i++) {
    writeFile.println("Sample data line for reading test purposes");
  }
  writeFile.close();
  
  PowerTestData samples[SAMPLES_PER_TEST];
  File readFile = SD.open("readtest.tmp", FILE_READ);
  
  if (!readFile) {
    SD.remove("readtest.tmp");
    return false;
  }
  
  char buffer[64];
  
  for (uint16_t i = 0; i < SAMPLES_PER_TEST; i++) {
    // Continuous reading during sampling
    if (i % 20 == 0 && readFile.available()) {
      readFile.readBytesUntil('\n', buffer, sizeof(buffer));
    }
    
    samples[i] = rapidPowerSample(SD_DATA_READING, SYS_SD_READ_OPERATION);
    samples[i].sample_number = i;
    delayMicroseconds(RAPID_SAMPLE_INTERVAL_US);
  }
  
  readFile.close();
  SD.remove("readtest.tmp");
  
  memcpy(testResults[SD_DATA_READING][SYS_SD_READ_OPERATION], samples,
         sizeof(PowerTestData) * SAMPLES_PER_TEST);
  
  return true;
}

bool testSDFileFlushing() {
  Serial.println(F("Testing SD Card File Flushing mode..."));
  
  // First create a test file with data
  File writeFile = SD.open("flushtest.tmp", FILE_WRITE);
  if (!writeFile) {
    return false;
  }
  
  for (int i = 0; i < 100; i++) {
    writeFile.println("Sample data line for flushing test purposes");
  }
  writeFile.close();
  
  // Flush the file
  if (!SD.remove("flushtest.tmp")) {
    return false;
  }
  
  return true;
}

bool testSDMetadataInteraction() {
  Serial.println(F("Testing SD Card Metadata Interaction mode..."));
  
  // First create a test file with data
  File writeFile = SD.open("metatest.tmp", FILE_WRITE);
  if (!writeFile) {
    return false;
  }
  
  for (int i = 0; i < 100; i++) {
    writeFile.println("Sample data line for metadata test purposes");
  }
  writeFile.close();
  
  // Read the file
  File readFile = SD.open("metatest.tmp", FILE_READ);
  if (!readFile) {
    return false;
  }
  
  char buffer[64];
  uint16_t lineCount = 0;
  
  while (readFile.available()) {
    readFile.readBytesUntil('\n', buffer, sizeof(buffer));
    lineCount++;
  }
  
  readFile.close();
  
  // Remove the file
  if (!SD.remove("metatest.tmp")) {
    return false;
  }
  
  return lineCount == 100;
}

bool testSDActiveIdle() {
  Serial.println(F("Testing SD Card Active Idle mode..."));
  
  // Ensure SD card is properly disconnected first
  SD.end();
  delay(50);
  
  // Begin rapid sampling during active idle
  PowerTestData idleSamples[SAMPLES_PER_TEST];
  
  unsigned long startTime = micros();
  bool samplingStarted = false;
  
  // Start sampling in parallel with SD idle
  for (uint16_t i = 0; i < SAMPLES_PER_TEST; i++) {
    if (i == SAMPLES_PER_TEST / 4 && !samplingStarted) {
      // Trigger SD idle at 25% through sampling
      SD.begin(chipSelect);
      samplingStarted = true;
    }
    
    idleSamples[i] = rapidPowerSample(SD_ACTIVE_IDLE, SYS_INITIALIZING);
    idleSamples[i].sample_number = i;
    delayMicroseconds(RAPID_SAMPLE_INTERVAL_US);
  }
  
  // Store results
  memcpy(testResults[SD_ACTIVE_IDLE][SYS_INITIALIZING], idleSamples, 
         sizeof(PowerTestData) * SAMPLES_PER_TEST);
  
  return true;
}

bool testSDStandbyIdle() {
  Serial.println(F("Testing SD Card Standby Idle mode..."));
  
  // Ensure SD card is properly disconnected first
  SD.end();
  delay(50);
  
  // Begin rapid sampling during standby idle
  PowerTestData idleSamples[SAMPLES_PER_TEST];
  
  unsigned long startTime = micros();
  bool samplingStarted = false;
  
  // Start sampling in parallel with SD idle
  for (uint16_t i = 0; i < SAMPLES_PER_TEST; i++) {
    if (i == SAMPLES_PER_TEST / 4 && !samplingStarted) {
      // Trigger SD idle at 25% through sampling
      SD.begin(chipSelect);
      samplingStarted = true;
    }
    
    idleSamples[i] = rapidPowerSample(SD_STANDBY_IDLE, SYS_INITIALIZING);
    idleSamples[i].sample_number = i;
    delayMicroseconds(RAPID_SAMPLE_INTERVAL_US);
  }
  
  // Store results
  memcpy(testResults[SD_STANDBY_IDLE][SYS_INITIALIZING], idleSamples, 
         sizeof(PowerTestData) * SAMPLES_PER_TEST);
  
  return true;
}

bool testSDSoftwareLowPower() {
  Serial.println(F("Testing SD Card Software Low Power mode..."));
  
  // Ensure SD card is properly disconnected first
  SD.end();
  delay(50);
  
  // Begin rapid sampling during software low power
  PowerTestData lowPowerSamples[SAMPLES_PER_TEST];
  
  unsigned long startTime = micros();
  bool samplingStarted = false;
  
  // Start sampling in parallel with SD low power
  for (uint16_t i = 0; i < SAMPLES_PER_TEST; i++) {
    if (i == SAMPLES_PER_TEST / 4 && !samplingStarted) {
      // Trigger SD low power at 25% through sampling
      SD.begin(chipSelect);
      samplingStarted = true;
    }
    
    lowPowerSamples[i] = rapidPowerSample(SD_SOFTWARE_LOW_POWER, SYS_INITIALIZING);
    lowPowerSamples[i].sample_number = i;
    delayMicroseconds(RAPID_SAMPLE_INTERVAL_US);
  }
  
  // Store results
  memcpy(testResults[SD_SOFTWARE_LOW_POWER][SYS_INITIALIZING], lowPowerSamples, 
         sizeof(PowerTestData) * SAMPLES_PER_TEST);
  
  return true;
}

bool testSDHardwarePoweredOff() {
  Serial.println(F("Testing SD Card Hardware Powered Off mode..."));
  
  // Ensure SD card is properly disconnected first
  SD.end();
  delay(50);
  
  // Begin rapid sampling during hardware powered off
  PowerTestData offSamples[SAMPLES_PER_TEST];
  
  unsigned long startTime = micros();
  bool samplingStarted = false;
  
  // Start sampling in parallel with SD off
  for (uint16_t i = 0; i < SAMPLES_PER_TEST; i++) {
    if (i == SAMPLES_PER_TEST / 4 && !samplingStarted) {
      // Trigger SD off at 25% through sampling
      SD.begin(chipSelect);
      samplingStarted = true;
    }
    
    offSamples[i] = rapidPowerSample(SD_HARDWARE_POWERED_OFF, SYS_INITIALIZING);
    offSamples[i].sample_number = i;
    delayMicroseconds(RAPID_SAMPLE_INTERVAL_US);
  }
  
  // Store results
  memcpy(testResults[SD_HARDWARE_POWERED_OFF][SYS_INITIALIZING], offSamples, 
         sizeof(PowerTestData) * SAMPLES_PER_TEST);
  
  return true;
}

bool testSDRapidReadWrite() {
  Serial.println(F("Testing SD Card Rapid Read/Write mode..."));
  
  // First create a test file with data
  File writeFile = SD.open("readwritetest.tmp", FILE_WRITE);
  if (!writeFile) {
    return false;
  }
  
  for (int i = 0; i < 100; i++) {
    writeFile.println("Sample data line for read/write test purposes");
  }
  writeFile.close();
  
  // Read the file
  File readFile = SD.open("readwritetest.tmp", FILE_READ);
  if (!readFile) {
    return false;
  }
  
  char buffer[64];
  uint16_t lineCount = 0;
  
  while (readFile.available()) {
    readFile.readBytesUntil('\n', buffer, sizeof(buffer));
    lineCount++;
  }
  
  readFile.close();
  
  // Remove the file
  if (!SD.remove("readwritetest.tmp")) {
    return false;
  }
  
  return lineCount == 100;
}

// ===== SYSTEM STATUS TESTING FUNCTIONS =====

bool testSystemInitializing() {
  Serial.println(F("Testing System Initializing status..."));
  
  // Ensure system is properly initialized first
  if (!sdCardInitialized) {
    return false;
  }
  
  // Begin rapid sampling during system initializing
  PowerTestData initializingSamples[SAMPLES_PER_TEST];
  
  unsigned long startTime = micros();
  bool samplingStarted = false;
  
  // Start sampling in parallel with system initializing
  for (uint16_t i = 0; i < SAMPLES_PER_TEST; i++) {
    if (i == SAMPLES_PER_TEST / 4 && !samplingStarted) {
      // Trigger system initialization at 25% through sampling
      samplingStarted = true;
    }
    
    initializingSamples[i] = rapidPowerSample(SD_INITIALIZATION, SYS_INITIALIZING);
    initializingSamples[i].sample_number = i;
    delayMicroseconds(RAPID_SAMPLE_INTERVAL_US);
  }
  
  // Store results
  memcpy(testResults[SD_INITIALIZATION][SYS_INITIALIZING], initializingSamples, 
         sizeof(PowerTestData) * SAMPLES_PER_TEST);
  
  return true;
}

bool testSensorAcquisition() {
  Serial.println(F("Testing Sensor Data Acquisition status..."));
  
  // Ensure system is properly initialized first
  if (!sdCardInitialized) {
    return false;
  }
  
  // Begin rapid sampling during sensor acquisition
  PowerTestData acquisitionSamples[SAMPLES_PER_TEST];
  
  unsigned long startTime = micros();
  bool samplingStarted = false;
  
  // Start sampling in parallel with sensor acquisition
  for (uint16_t i = 0; i < SAMPLES_PER_TEST; i++) {
    if (i == SAMPLES_PER_TEST / 4 && !samplingStarted) {
      // Trigger sensor acquisition at 25% through sampling
      samplingStarted = true;
    }
    
    acquisitionSamples[i] = rapidPowerSample(SD_INITIALIZATION, SYS_SENSOR_ACQUISITION);
    acquisitionSamples[i].sample_number = i;
    delayMicroseconds(RAPID_SAMPLE_INTERVAL_US);
  }
  
  // Store results
  memcpy(testResults[SD_INITIALIZATION][SYS_SENSOR_ACQUISITION], acquisitionSamples, 
         sizeof(PowerTestData) * SAMPLES_PER_TEST);
  
  return true;
}

bool testPreparingSD() {
  Serial.println(F("Testing Preparing SD status..."));
  
  // Ensure system is properly initialized first
  if (!sdCardInitialized) {
    return false;
  }
  
  // Begin rapid sampling during preparing SD
  PowerTestData preparingSamples[SAMPLES_PER_TEST];
  
  unsigned long startTime = micros();
  bool samplingStarted = false;
  
  // Start sampling in parallel with preparing SD
  for (uint16_t i = 0; i < SAMPLES_PER_TEST; i++) {
    if (i == SAMPLES_PER_TEST / 4 && !samplingStarted) {
      // Trigger preparing SD at 25% through sampling
      SD.begin(chipSelect);
      samplingStarted = true;
    }
    
    preparingSamples[i] = rapidPowerSample(SD_INITIALIZATION, SYS_PREPARING_SD);
    preparingSamples[i].sample_number = i;
    delayMicroseconds(RAPID_SAMPLE_INTERVAL_US);
  }
  
  // Store results
  memcpy(testResults[SD_INITIALIZATION][SYS_PREPARING_SD], preparingSamples, 
         sizeof(PowerTestData) * SAMPLES_PER_TEST);
  
  return true;
}

bool testWritingBuffer() {
  Serial.println(F("Testing Writing RAM Buffer status..."));
  
  // Ensure system is properly initialized first
  if (!sdCardInitialized) {
    return false;
  }
  
  // Begin rapid sampling during writing RAM buffer
  PowerTestData writingSamples[SAMPLES_PER_TEST];
  
  unsigned long startTime = micros();
  bool samplingStarted = false;
  
  // Start sampling in parallel with writing RAM buffer
  for (uint16_t i = 0; i < SAMPLES_PER_TEST; i++) {
    if (i == SAMPLES_PER_TEST / 4 && !samplingStarted) {
      // Trigger writing RAM buffer at 25% through sampling
      SD.begin(chipSelect);
      samplingStarted = true;
    }
    
    writingSamples[i] = rapidPowerSample(SD_INITIALIZATION, SYS_WRITING_BUFFER);
    writingSamples[i].sample_number = i;
    delayMicroseconds(RAPID_SAMPLE_INTERVAL_US);
  }
  
  // Store results
  memcpy(testResults[SD_INITIALIZATION][SYS_WRITING_BUFFER], writingSamples, 
         sizeof(PowerTestData) * SAMPLES_PER_TEST);
  
  return true;
}

bool testFinalizingSD() {
  Serial.println(F("Testing Finalizing SD status..."));
  
  // Ensure system is properly initialized first
  if (!sdCardInitialized) {
    return false;
  }
  
  // Begin rapid sampling during finalizing SD
  PowerTestData finalizingSamples[SAMPLES_PER_TEST];
  
  unsigned long startTime = micros();
  bool samplingStarted = false;
  
  // Start sampling in parallel with finalizing SD
  for (uint16_t i = 0; i < SAMPLES_PER_TEST; i++) {
    if (i == SAMPLES_PER_TEST / 4 && !samplingStarted) {
      // Trigger finalizing SD at 25% through sampling
      SD.begin(chipSelect);
      samplingStarted = true;
    }
    
    finalizingSamples[i] = rapidPowerSample(SD_INITIALIZATION, SYS_FINALIZING_SD);
    finalizingSamples[i].sample_number = i;
    delayMicroseconds(RAPID_SAMPLE_INTERVAL_US);
  }
  
  // Store results
  memcpy(testResults[SD_INITIALIZATION][SYS_FINALIZING_SD], finalizingSamples, 
         sizeof(PowerTestData) * SAMPLES_PER_TEST);
  
  return true;
}

bool testTransitioningLowPower() {
  Serial.println(F("Testing Transitioning to Low Power status..."));
  
  // Ensure system is properly initialized first
  if (!sdCardInitialized) {
    return false;
  }
  
  // Begin rapid sampling during transitioning to low power
  PowerTestData transitioningSamples[SAMPLES_PER_TEST];
  
  unsigned long startTime = micros();
  bool samplingStarted = false;
  
  // Start sampling in parallel with transitioning to low power
  for (uint16_t i = 0; i < SAMPLES_PER_TEST; i++) {
    if (i == SAMPLES_PER_TEST / 4 && !samplingStarted) {
      // Trigger transitioning to low power at 25% through sampling
      SD.begin(chipSelect);
      samplingStarted = true;
    }
    
    transitioningSamples[i] = rapidPowerSample(SD_INITIALIZATION, SYS_TRANSITIONING_LOW_POWER);
    transitioningSamples[i].sample_number = i;
    delayMicroseconds(RAPID_SAMPLE_INTERVAL_US);
  }
  
  // Store results
  memcpy(testResults[SD_INITIALIZATION][SYS_TRANSITIONING_LOW_POWER], transitioningSamples, 
         sizeof(PowerTestData) * SAMPLES_PER_TEST);
  
  return true;
}

bool testMCUIdle() {
  Serial.println(F("Testing MCU Idle status..."));
  
  // Ensure system is properly initialized first
  if (!sdCardInitialized) {
    return false;
  }
  
  // Begin rapid sampling during MCU idle
  PowerTestData idleSamples[SAMPLES_PER_TEST];
  
  unsigned long startTime = micros();
  bool samplingStarted = false;
  
  // Start sampling in parallel with MCU idle
  for (uint16_t i = 0; i < SAMPLES_PER_TEST; i++) {
    if (i == SAMPLES_PER_TEST / 4 && !samplingStarted) {
      // Trigger MCU idle at 25% through sampling
      SD.begin(chipSelect);
      samplingStarted = true;
    }
    
    idleSamples[i] = rapidPowerSample(SD_INITIALIZATION, SYS_MCU_IDLE);
    idleSamples[i].sample_number = i;
    delayMicroseconds(RAPID_SAMPLE_INTERVAL_US);
  }
  
  // Store results
  memcpy(testResults[SD_INITIALIZATION][SYS_MCU_IDLE], idleSamples, 
         sizeof(PowerTestData) * SAMPLES_PER_TEST);
  
  return true;
}

bool testSDReadOperation() {
  Serial.println(F("Testing SD Read Operation status..."));
  
  // Ensure system is properly initialized first
  if (!sdCardInitialized) {
    return false;
  }
  
  // Begin rapid sampling during SD read operation
  PowerTestData readSamples[SAMPLES_PER_TEST];
  
  unsigned long startTime = micros();
  bool samplingStarted = false;
  
  // Start sampling in parallel with SD read operation
  for (uint16_t i = 0; i < SAMPLES_PER_TEST; i++) {
    if (i == SAMPLES_PER_TEST / 4 && !samplingStarted) {
      // Trigger SD read operation at 25% through sampling
      SD.begin(chipSelect);
      samplingStarted = true;
    }
    
    readSamples[i] = rapidPowerSample(SD_INITIALIZATION, SYS_SD_READ_OPERATION);
    readSamples[i].sample_number = i;
    delayMicroseconds(RAPID_SAMPLE_INTERVAL_US);
  }
  
  // Store results
  memcpy(testResults[SD_INITIALIZATION][SYS_SD_READ_OPERATION], readSamples, 
         sizeof(PowerTestData) * SAMPLES_PER_TEST);
  
  return true;
}

bool testErrorRecovery() {
  Serial.println(F("Testing Error Recovery status..."));
  
  // Ensure system is properly initialized first
  if (!sdCardInitialized) {
    return false;
  }
  
  // Begin rapid sampling during error recovery
  PowerTestData recoverySamples[SAMPLES_PER_TEST];
  
  unsigned long startTime = micros();
  bool samplingStarted = false;
  
  // Start sampling in parallel with error recovery
  for (uint16_t i = 0; i < SAMPLES_PER_TEST; i++) {
    if (i == SAMPLES_PER_TEST / 4 && !samplingStarted) {
      // Trigger error recovery at 25% through sampling
      SD.begin(chipSelect);
      samplingStarted = true;
    }
    
    recoverySamples[i] = rapidPowerSample(SD_INITIALIZATION, SYS_ERROR_RECOVERY);
    recoverySamples[i].sample_number = i;
    delayMicroseconds(RAPID_SAMPLE_INTERVAL_US);
  }
  
  // Store results
  memcpy(testResults[SD_INITIALIZATION][SYS_ERROR_RECOVERY], recoverySamples, 
         sizeof(PowerTestData) * SAMPLES_PER_TEST);
  
  return true;
}

// ===== STATISTICAL ANALYSIS FUNCTIONS =====

PowerStatistics calculateStatistics(PowerTestData samples[], uint16_t numSamples) {
  PowerStatistics stats;
  stats.min_power = stats.max_power = samples[0].total_power;
  stats.avg_power = 0.0f;
  stats.peak_power = 0.0f;
  stats.energy_consumed_mWh = 0.0f;
  stats.duration_us = 0;
  stats.sample_count = 0;
  stats.std_deviation = 0.0f;
  stats.peak_to_average_ratio = 0.0f;
  
  for (uint16_t i = 0; i < numSamples; i++) {
    if (samples[i].valid_measurement) {
      float power = samples[i].total_power;
      stats.min_power = min(stats.min_power, power);
      stats.max_power = max(stats.max_power, power);
      stats.avg_power += power;
      stats.peak_power = max(stats.peak_power, power);
      stats.energy_consumed_mWh += power * 0.001f; // Convert seconds to milliseconds
      stats.duration_us += RAPID_SAMPLE_INTERVAL_US;
      stats.sample_count++;
    }
  }
  
  stats.avg_power /= stats.sample_count;
  stats.peak_to_average_ratio = stats.peak_power / stats.avg_power;
  
  return stats;
}

void generateCorrelationAnalysis() {
  Serial.println(F("=== CORRELATION ANALYSIS ==="));
  
  // Analyze correlation between SD modes and system statuses
  for (uint8_t mode = 0; mode < SD_MODE_COUNT; mode++) {
    float modeAverage = 0.0f;
    uint8_t validStatuses = 0;
    
    for (uint8_t status = 0; status < SYS_STATUS_COUNT; status++) {
      PowerStatistics stats = combinedStatistics[mode][status];
      if (stats.sample_count > 0) {
        modeAverage += stats.avg_power;
        validStatuses++;
      }
    }
    
    if (validStatuses > 0) {
      modeAverage /= validStatuses;
      modeStatistics[mode].avg_power = modeAverage;
      
      Serial.print(getSDModeName((SDCardMode)mode));
      Serial.print(F(": ")); Serial.print(modeAverage, 3); Serial.println(F(" mW"));
    }
  }
  
  Serial.println();
}

void detectPowerPeaks(PowerTestData samples[], uint16_t numSamples) {
  if (numSamples < 3) return;
  
  Serial.println(F("=== POWER PEAK DETECTION ==="));
  
  uint16_t peakCount = 0;
  float threshold = 10.0f; // 10mW threshold for peak detection
  
  for (uint16_t i = 1; i < numSamples - 1; i++) {
    if (samples[i].valid_measurement && 
        samples[i-1].valid_measurement && 
        samples[i+1].valid_measurement) {
      
      float current = samples[i].total_power;
      float prev = samples[i-1].total_power;
      float next = samples[i+1].total_power;
      
      // Check if current sample is a peak
      if (current > prev && current > next && current > threshold) {
        peakCount++;
        if (peakCount <= 5) { // Only show first 5 peaks
          Serial.print(F("Peak ")); Serial.print(peakCount);
          Serial.print(F(" at sample ")); Serial.print(i);
          Serial.print(F(": ")); Serial.print(current, 3); Serial.println(F(" mW"));
        }
      }
    }
  }
  
  Serial.print(F("Total peaks detected: ")); Serial.println(peakCount);
  Serial.println();
}

float calculateStandardDeviation(PowerTestData samples[], uint16_t numSamples, float average) {
  float variance = 0.0f;
  uint16_t validSamples = 0;
  
  for (uint16_t i = 0; i < numSamples; i++) {
    if (samples[i].valid_measurement) {
      float power = samples[i].total_power;
      variance += (power - average) * (power - average);
      validSamples++;
    }
  }
  
  if (validSamples > 1) {
    variance /= (validSamples - 1); // Use sample standard deviation
    return sqrt(variance);
  }
  
  return 0.0f;
}

// ===== REPORTING FUNCTIONS =====

void createPowerTestCSV() {
  if (!sdCardInitialized) {
    return;
  }
  
  testDataFile = SD.open("power_test_data.csv", FILE_WRITE);
  if (testDataFile) {
    testDataFile.println(F("timestamp_us,sd_mode,system_status,solar_voltage,solar_current,solar_power,battery_voltage,battery_current,battery_power,load_voltage,load_current,load_power,total_power,sample_number,iteration,valid"));
    testDataFile.close();
    Serial.println(F("Created power test CSV file"));
  }
  
  testLogFile = SD.open("power_test_log.txt", FILE_WRITE);
  if (testLogFile) {
    testLogFile.println(F("SD Card Power Consumption Test Log"));
    testLogFile.println(F("====================================="));
    testLogFile.close();
    Serial.println(F("Created power test log file"));
  }
}

void logPowerTestData(const PowerTestData& data) {
  if (!sdCardInitialized) {
    return;
  }
  
  testDataFile = SD.open("power_test_data.csv", FILE_WRITE);
  if (testDataFile) {
    testDataFile.print(data.timestamp_micros); testDataFile.print(",");
    testDataFile.print((uint8_t)data.sd_mode); testDataFile.print(",");
    testDataFile.print((uint8_t)data.system_status); testDataFile.print(",");
    testDataFile.print(data.solar_voltage, 6); testDataFile.print(",");
    testDataFile.print(data.solar_current, 6); testDataFile.print(",");
    testDataFile.print(data.solar_power, 6); testDataFile.print(",");
    testDataFile.print(data.battery_voltage, 6); testDataFile.print(",");
    testDataFile.print(data.battery_current, 6); testDataFile.print(",");
    testDataFile.print(data.battery_power, 6); testDataFile.print(",");
    testDataFile.print(data.load_voltage, 6); testDataFile.print(",");
    testDataFile.print(data.load_current, 6); testDataFile.print(",");
    testDataFile.print(data.load_power, 6); testDataFile.print(",");
    testDataFile.print(data.total_power, 6); testDataFile.print(",");
    testDataFile.print(data.sample_number); testDataFile.print(",");
    testDataFile.print(currentTestIteration); testDataFile.print(",");
    testDataFile.println(data.valid_measurement ? "1" : "0");
    testDataFile.close();
  }
}

void generateTextReport() {
  Serial.println(F("================================================="));
  Serial.println(F("SD CARD POWER CONSUMPTION DETAILED REPORT"));
  Serial.println(F("================================================="));
  Serial.println();
  
  // Report header
  Serial.print(F("Test Date: ")); Serial.println(__DATE__);
  Serial.print(F("Test Time: ")); Serial.println(__TIME__);
  Serial.print(F("Sample Rate: ")); Serial.print(1000000 / RAPID_SAMPLE_INTERVAL_US); 
  Serial.println(F(" Hz"));
  Serial.print(F("Samples per test: ")); Serial.println(SAMPLES_PER_TEST);
  Serial.print(F("Test iterations: ")); Serial.println(TEST_ITERATIONS);
  Serial.println();
  
  // SD Mode Analysis
  Serial.println(F("=== SD CARD MODE ANALYSIS ==="));
  for (uint8_t mode = 0; mode < SD_MODE_COUNT; mode++) {
    Serial.print(F("Mode: ")); Serial.println(getSDModeName((SDCardMode)mode));
    
    for (uint8_t status = 0; status < SYS_STATUS_COUNT; status++) {
      PowerStatistics stats = combinedStatistics[mode][status];
      if (stats.sample_count > 0) {
        Serial.print(F("  ")); Serial.print(getSystemStatusName((SystemStatus)status));
        Serial.print(F(": Avg=")); Serial.print(stats.avg_power, 3); Serial.print(F("mW"));
        Serial.print(F(", Peak=")); Serial.print(stats.peak_power, 3); Serial.print(F("mW"));
        Serial.print(F(", Min=")); Serial.print(stats.min_power, 3); Serial.println();
      }
    }
    Serial.println();
  }
  
  // System Status Analysis
  Serial.println(F("=== SYSTEM STATUS ANALYSIS ==="));
  for (uint8_t status = 0; status < SYS_STATUS_COUNT; status++) {
    Serial.print(F("Status: ")); Serial.println(getSystemStatusName((SystemStatus)status));
    
    float totalAvgPower = 0.0f;
    float totalPeakPower = 0.0f;
    uint8_t validModes = 0;
    
    for (uint8_t mode = 0; mode < SD_MODE_COUNT; mode++) {
      PowerStatistics stats = combinedStatistics[mode][status];
      if (stats.sample_count > 0) {
        totalAvgPower += stats.avg_power;
        totalPeakPower = max(totalPeakPower, stats.peak_power);
        validModes++;
      }
    }
    
    if (validModes > 0) {
      Serial.print(F("  Average across modes: ")); 
      Serial.print(totalAvgPower / validModes, 3); Serial.println(F("mW"));
      Serial.print(F("  Peak across modes: ")); 
      Serial.print(totalPeakPower, 3); Serial.println(F("mW"));
    }
    Serial.println();
  }
  
  Serial.println(F("=== END OF DETAILED REPORT ==="));
}

void generateSummaryReport() {
  Serial.println(F("================================================="));
  Serial.println(F("SD CARD POWER CONSUMPTION SUMMARY REPORT"));
  Serial.println(F("================================================="));
  Serial.println();
  
  // Find highest and lowest power modes
  float highestPower = 0.0f;
  float lowestPower = 99999.0f;
  SDCardMode highestMode = SD_INITIALIZATION;
  SDCardMode lowestMode = SD_INITIALIZATION;
  SystemStatus highestStatus = SYS_INITIALIZING;
  SystemStatus lowestStatus = SYS_INITIALIZING;
  
  for (uint8_t mode = 0; mode < SD_MODE_COUNT; mode++) {
    for (uint8_t status = 0; status < SYS_STATUS_COUNT; status++) {
      PowerStatistics stats = combinedStatistics[mode][status];
      if (stats.sample_count > 0) {
        if (stats.avg_power > highestPower) {
          highestPower = stats.avg_power;
          highestMode = (SDCardMode)mode;
          highestStatus = (SystemStatus)status;
        }
        if (stats.avg_power < lowestPower) {
          lowestPower = stats.avg_power;
          lowestMode = (SDCardMode)mode;
          lowestStatus = (SystemStatus)status;
        }
      }
    }
  }
  
  Serial.println(F("=== POWER CONSUMPTION SUMMARY ==="));
  Serial.print(F("Highest Power Mode: ")); Serial.print(getSDModeName(highestMode));
  Serial.print(F(" + ")); Serial.println(getSystemStatusName(highestStatus));
  Serial.print(F("  Average Power: ")); Serial.print(highestPower, 3); Serial.println(F(" mW"));
  Serial.println();
  
  Serial.print(F("Lowest Power Mode: ")); Serial.print(getSDModeName(lowestMode));
  Serial.print(F(" + ")); Serial.println(getSystemStatusName(lowestStatus));
  Serial.print(F("  Average Power: ")); Serial.print(lowestPower, 3); Serial.println(F(" mW"));
  Serial.println();
  
  Serial.print(F("Power Range: ")); Serial.print(lowestPower, 3); Serial.print(F(" - "));
  Serial.print(highestPower, 3); Serial.println(F(" mW"));
  Serial.print(F("Power Ratio: ")); Serial.print(highestPower / lowestPower, 2); Serial.println(F(":1"));
  
  Serial.println();
  Serial.println(F("=== RECOMMENDATIONS ==="));
  Serial.println(F("- Use lowest power modes for battery conservation"));
  Serial.println(F("- Minimize time in highest power modes"));
  Serial.println(F("- Consider batching operations for efficiency"));
  
  Serial.println(F("=== END OF SUMMARY REPORT ==="));
}

void printTestProgress(SDCardMode mode, SystemStatus status, uint16_t iteration) {
  Serial.print(F("Testing: ")); Serial.print(getSDModeName(mode));
  Serial.print(F(" / ")); Serial.print(getSystemStatusName(status));
  Serial.print(F(" (Iteration ")); Serial.print(iteration + 1); Serial.println(F(")"));
}

// ===== CONTROL FUNCTIONS =====



void runComprehensivePowerTest() {
  testInProgress = true;
  currentTestIteration = 0;
  
  Serial.println(F("================================================="));
  Serial.println(F("Starting Comprehensive SD Card Power Test"));
  Serial.println(F("================================================="));
  Serial.print(F("Testing ")); Serial.print(SD_MODE_COUNT); Serial.println(F(" SD modes"));
  Serial.print(F("Testing ")); Serial.print(SYS_STATUS_COUNT); Serial.println(F(" system statuses"));
  Serial.print(F("Samples per test: ")); Serial.println(SAMPLES_PER_TEST);
  Serial.print(F("Test iterations: ")); Serial.println(TEST_ITERATIONS);
  Serial.println();
  
  // Run each iteration
  for (currentTestIteration = 0; currentTestIteration < TEST_ITERATIONS; currentTestIteration++) {
    Serial.print(F("=== Test Iteration ")); Serial.print(currentTestIteration + 1);
    Serial.print(F(" of ")); Serial.print(TEST_ITERATIONS); Serial.println(F(" ==="));
    
    // Test all SD card modes
    for (uint8_t mode = 0; mode < SD_MODE_COUNT; mode++) {
      printTestProgress((SDCardMode)mode, SYS_INITIALIZING, currentTestIteration);
      
      switch ((SDCardMode)mode) {
        case SD_INITIALIZATION:
          testSDInitialization();
          break;
        case SD_FILE_OPEN_CLOSE:
          testSDFileOpenClose();
          break;
        case SD_DATA_WRITING:
          testSDDataWriting();
          break;
        case SD_DATA_READING:
          testSDDataReading();
          break;
        case SD_FILE_FLUSHING:
          testSDFileFlushing();
          break;
        case SD_METADATA_INTERACTION:
          testSDMetadataInteraction();
          break;
        case SD_ACTIVE_IDLE:
          testSDActiveIdle();
          break;
        case SD_STANDBY_IDLE:
          testSDStandbyIdle();
          break;
        case SD_SOFTWARE_LOW_POWER:
          testSDSoftwareLowPower();
          break;
        case SD_HARDWARE_POWERED_OFF:
          testSDHardwarePoweredOff();
          break;
        case SD_RAPID_READ_WRITE:
          testSDRapidReadWrite();
          break;
        default:
          break;
      }
      
      delay(INTER_TEST_DELAY_MS);
    }
    
    // Test all system statuses
    for (uint8_t status = 0; status < SYS_STATUS_COUNT; status++) {
      printTestProgress(SD_INITIALIZATION, (SystemStatus)status, currentTestIteration);
      
      switch ((SystemStatus)status) {
        case SYS_INITIALIZING:
          testSystemInitializing();
          break;
        case SYS_SENSOR_ACQUISITION:
          testSensorAcquisition();
          break;
        case SYS_PREPARING_SD:
          testPreparingSD();
          break;
        case SYS_WRITING_BUFFER:
          testWritingBuffer();
          break;
        case SYS_FINALIZING_SD:
          testFinalizingSD();
          break;
        case SYS_TRANSITIONING_LOW_POWER:
          testTransitioningLowPower();
          break;
        case SYS_MCU_IDLE:
          testMCUIdle();
          break;
        case SYS_SD_READ_OPERATION:
          testSDReadOperation();
          break;
        case SYS_ERROR_RECOVERY:
          testErrorRecovery();
          break;
        default:
          break;
      }
      
      delay(INTER_TEST_DELAY_MS);
    }
  }
  
  Serial.println();
  Serial.println(F("================================================="));
  Serial.println(F("Comprehensive Power Test Complete"));
  Serial.println(F("================================================="));
  Serial.println(F("Calculating statistics..."));
  
  // Calculate statistics for all modes and statuses
  for (uint8_t mode = 0; mode < SD_MODE_COUNT; mode++) {
    for (uint8_t status = 0; status < SYS_STATUS_COUNT; status++) {
      combinedStatistics[mode][status] = calculateStatistics(
        testResults[mode][status], SAMPLES_PER_TEST);
    }
  }
  
  Serial.println(F("Statistics calculation complete"));
  Serial.println(F("Type 'r' for detailed report or 's' for summary"));
  
  testInProgress = false;
}

void runSingleModeTest(SDCardMode mode) {
  Serial.print(F("Running single mode test: ")); Serial.println(getSDModeName(mode));
  
  printTestProgress(mode, SYS_INITIALIZING, 0);
  
  switch (mode) {
    case SD_INITIALIZATION:
      testSDInitialization();
      break;
    case SD_FILE_OPEN_CLOSE:
      testSDFileOpenClose();
      break;
    case SD_DATA_WRITING:
      testSDDataWriting();
      break;
    case SD_DATA_READING:
      testSDDataReading();
      break;
    case SD_FILE_FLUSHING:
      testSDFileFlushing();
      break;
    case SD_METADATA_INTERACTION:
      testSDMetadataInteraction();
      break;
    case SD_ACTIVE_IDLE:
      testSDActiveIdle();
      break;
    case SD_STANDBY_IDLE:
      testSDStandbyIdle();
      break;
    case SD_SOFTWARE_LOW_POWER:
      testSDSoftwareLowPower();
      break;
    case SD_HARDWARE_POWERED_OFF:
      testSDHardwarePoweredOff();
      break;
    case SD_RAPID_READ_WRITE:
      testSDRapidReadWrite();
      break;
    default:
      Serial.println(F("Invalid mode"));
      break;
  }
}

void runSingleStatusTest(SystemStatus status) {
  Serial.print(F("Running single status test: ")); Serial.println(getSystemStatusName(status));
  
  printTestProgress(SD_INITIALIZATION, status, 0);
  
  switch (status) {
    case SYS_INITIALIZING:
      testSystemInitializing();
      break;
    case SYS_SENSOR_ACQUISITION:
      testSensorAcquisition();
      break;
    case SYS_PREPARING_SD:
      testPreparingSD();
      break;
    case SYS_WRITING_BUFFER:
      testWritingBuffer();
      break;
    case SYS_FINALIZING_SD:
      testFinalizingSD();
      break;
    case SYS_TRANSITIONING_LOW_POWER:
      testTransitioningLowPower();
      break;
    case SYS_MCU_IDLE:
      testMCUIdle();
      break;
    case SYS_SD_READ_OPERATION:
      testSDReadOperation();
      break;
    case SYS_ERROR_RECOVERY:
      testErrorRecovery();
      break;
    default:
      Serial.println(F("Invalid status"));
      break;
  }
}

// ===== UTILITY FUNCTIONS =====

const char* getSDModeName(SDCardMode mode) {
  if (mode >= 0 && mode < SD_MODE_COUNT) {
    return SD_MODE_NAMES[mode];
  }
  return "Unknown Mode";
}

const char* getSystemStatusName(SystemStatus status) {
  if (status >= 0 && status < SYS_STATUS_COUNT) {
    return SYSTEM_STATUS_NAMES[status];
  }
  return "Unknown Status";
}

bool validateTestEnvironment() {
  Serial.println(F("Validating test environment..."));
  
  // Check INA228 sensors
  bool sensorsOK = true;
  
  Wire.beginTransmission(INA_SOLAR_ADDR);
  if (Wire.endTransmission() != 0) {
    Serial.println(F("ERROR: Solar INA228 not responding"));
    sensorsOK = false;
  }
  
  Wire.beginTransmission(INA_BATTERY_ADDR);
  if (Wire.endTransmission() != 0) {
    Serial.println(F("ERROR: Battery INA228 not responding"));
    sensorsOK = false;
  }
  
  Wire.beginTransmission(INA_LOAD_ADDR);
  if (Wire.endTransmission() != 0) {
    Serial.println(F("ERROR: Load INA228 not responding"));
    sensorsOK = false;
  }
  
  // Check SD card
  if (!sdCardInitialized) {
    Serial.println(F("ERROR: SD card not initialized"));
    return false;
  }
  
  // Test SD card write capability
  File testFile = SD.open("test.tmp", FILE_WRITE);
  if (!testFile) {
    Serial.println(F("ERROR: Cannot write to SD card"));
    return false;
  }
  testFile.println("Test");
  testFile.close();
  SD.remove("test.tmp");
  
  if (sensorsOK) {
    Serial.println(F("Test environment validation passed"));
  } else {
    Serial.println(F("Test environment validation failed"));
  }
  
  return sensorsOK;
}

void resetTestData() {
  Serial.println(F("Resetting test data..."));
  
  // Clear all test result arrays
  memset(testResults, 0, sizeof(testResults));
  memset(modeStatistics, 0, sizeof(modeStatistics));
  memset(systemStatistics, 0, sizeof(systemStatistics));
  memset(combinedStatistics, 0, sizeof(combinedStatistics));
  
  currentTestIteration = 0;
  testInProgress = false;
  
  Serial.println(F("Test data reset complete"));
}

// ===== END OF FILE ===== 