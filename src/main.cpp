#include <Wire.h>
#include <hp_BH1750.h>

// BH1750 Light Sensor
hp_BH1750 lightMeter; // Uses default I2C address 0x23

// Define I2C addresses for INA228 sensors
const uint8_t INA_SOLAR_ADDR = 0x40;
const uint8_t INA_BATTERY_ADDR = 0x44; // Note: Your original serial print said 0x42 for battery
const uint8_t INA_LOAD_ADDR = 0x41;

// INA228 register addresses
const uint8_t INA228_REG_CONFIG = 0x00;
const uint8_t INA228_REG_SHUNTCAL = 0x02;
const uint8_t INA228_REG_VSHUNT = 0x04;
const uint8_t INA228_REG_VBUS = 0x05;
// const uint8_t INA228_REG_DIETEMP = 0x06; // Not used in this version
const uint8_t INA228_REG_CURRENT = 0x07;
const uint8_t INA228_REG_POWER = 0x08;
// const uint8_t INA228_REG_ENERGY = 0x09; // Not used
// const uint8_t INA228_REG_CHARGE = 0x0A; // Not used
// const uint8_t INA228_REG_DIAGALRT = 0x0B; // Not used
// const uint8_t INA228_REG_MFG_UID = 0x3E; // Not used
// const uint8_t INA228_REG_DVC_UID = 0x3F; // Not used

// Calibration and scaling variables - These will be properly set by initINA228
float solar_current_lsb = 0.0f;
float battery_current_lsb = 0.0f;
float load_current_lsb = 0.0f;

// Function prototypes
// MODIFIED: initINA228 now returns float (the calculated current_lsb)
float initINA228(uint8_t address, float shuntOhms = 0.015f, float maxCurrent = 5.0f);
float readBusVoltage(uint8_t address);
float readShuntVoltage(uint8_t address);
float readCurrent(uint8_t address, float current_lsb_value);
float readPower(uint8_t address, float current_lsb_value);
uint32_t readRegister(uint8_t address, uint8_t reg, uint8_t numBytes);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // wait for serial port to connect. Needed for native USB
  }

  Serial.println(F("Starting sensor initialization..."));

  // Initialize I2C bus
  Wire.begin();

  // Initialize BH1750
  if (lightMeter.begin(BH1750_TO_GROUND)) { // 0x23
    Serial.println(F("BH1750 Light Sensor Initialized."));
    lightMeter.start(); // Start continuous measurement
  } else {
    Serial.println(F("Error initializing BH1750 Light Sensor!"));
  }

  // Initialize INA228 sensors
  Serial.println(F("Initializing INA228 sensors..."));

  solar_current_lsb = initINA228(INA_SOLAR_ADDR, 0.015f, 5.0f);
  battery_current_lsb = initINA228(INA_BATTERY_ADDR, 0.015f, 5.0f); // Assuming 0.015 shunt, 5A max for all
  load_current_lsb = initINA228(INA_LOAD_ADDR, 0.015f, 5.0f);

  if (solar_current_lsb > 0.0f) { // Check if LSB is valid (not 0.0f indicating error)
    Serial.println(F("INA228 Solar Panel Monitor (0x40) Initialized."));
    Serial.print(F("  Solar Current LSB (A/bit): ")); Serial.println(solar_current_lsb, 8);
  } else {
    Serial.print(F("Failed to initialize INA228 Solar Panel Monitor at 0x"));
    Serial.println(INA_SOLAR_ADDR, HEX);
  }

  if (battery_current_lsb > 0.0f) {
    Serial.println(F("INA228 Battery Power Monitor (0x44) Initialized.")); // Corrected address in log if INA_BATTERY_ADDR is 0x44
    Serial.print(F("  Battery Current LSB (A/bit): ")); Serial.println(battery_current_lsb, 8);
  } else {
    Serial.print(F("Failed to initialize INA228 Battery Power Monitor at 0x"));
    Serial.println(INA_BATTERY_ADDR, HEX);
  }

  if (load_current_lsb > 0.0f) {
    Serial.println(F("INA228 Load Power Monitor (0x41) Initialized."));
    Serial.print(F("  Load Current LSB (A/bit): ")); Serial.println(load_current_lsb, 8);
  } else {
    Serial.print(F("Failed to initialize INA228 Load Power Monitor at 0x"));
    Serial.println(INA_LOAD_ADDR, HEX);
  }
  
  Serial.println(F("Sensor initialization complete. Starting measurements."));
  Serial.println(F("--------------------------------------------------"));
}

void loop() {
  // Read Light Sensor
  float lux = 0;
  if (lightMeter.hasValue()) {
    lux = lightMeter.getLux();
  }

  Serial.println(F("--- Sensor Readings ---"));

  // BH1750 Data
  Serial.print(F("Light Intensity: "));
  Serial.print(lux);
  Serial.println(F(" lx"));
  Serial.println();

  // INA228 Solar Panel Monitor Data
  if (solar_current_lsb > 0.0f) {
    Serial.println(F("Solar Panel Monitor (0x40):"));
    Serial.print(F("  Bus Voltage:   ")); Serial.print(readBusVoltage(INA_SOLAR_ADDR), 4); Serial.println(F(" V"));
    Serial.print(F("  Shunt Voltage: ")); Serial.print(readShuntVoltage(INA_SOLAR_ADDR), 4); Serial.println(F(" mV"));
    Serial.print(F("  Current:       ")); Serial.print(readCurrent(INA_SOLAR_ADDR, solar_current_lsb), 4); Serial.println(F(" mA"));
    Serial.print(F("  Power:         ")); Serial.print(readPower(INA_SOLAR_ADDR, solar_current_lsb), 4); Serial.println(F(" mW"));
    Serial.println();
  }

  // INA228 Battery Power Monitor Data
  if (battery_current_lsb > 0.0f) {
    Serial.println(F("Battery Power Monitor (0x44):")); // Corrected address
    Serial.print(F("  Bus Voltage:   ")); Serial.print(readBusVoltage(INA_BATTERY_ADDR), 4); Serial.println(F(" V"));
    Serial.print(F("  Shunt Voltage: ")); Serial.print(readShuntVoltage(INA_BATTERY_ADDR), 4); Serial.println(F(" mV"));
    Serial.print(F("  Current:       ")); Serial.print(readCurrent(INA_BATTERY_ADDR, battery_current_lsb), 4); Serial.println(F(" mA"));
    Serial.print(F("  Power:         ")); Serial.print(readPower(INA_BATTERY_ADDR, battery_current_lsb), 4); Serial.println(F(" mW"));
    Serial.println();
  }

  // INA228 Load Power Monitor Data
  if (load_current_lsb > 0.0f) {
    Serial.println(F("Load Power Monitor (0x41):"));
    Serial.print(F("  Bus Voltage:   ")); Serial.print(readBusVoltage(INA_LOAD_ADDR), 4); Serial.println(F(" V"));
    Serial.print(F("  Shunt Voltage: ")); Serial.print(readShuntVoltage(INA_LOAD_ADDR), 4); Serial.println(F(" mV"));
    Serial.print(F("  Current:       ")); Serial.print(readCurrent(INA_LOAD_ADDR, load_current_lsb), 4); Serial.println(F(" mA"));
    Serial.print(F("  Power:         ")); Serial.print(readPower(INA_LOAD_ADDR, load_current_lsb), 4); Serial.println(F(" mW"));
  }
  
  Serial.println(F("--------------------------------------------------"));
  Serial.println();

  delay(3000); 
}

// MODIFIED: Returns float (calculated current_lsb in Amps/bit) or 0.0f on error
float initINA228(uint8_t address, float shuntOhms, float maxCurrent) {
  Wire.beginTransmission(address);
  if (Wire.endTransmission() != 0) {
    Serial.print(F("INA228 not found at address 0x")); Serial.println(address, HEX);
    return 0.0f; 
  }
  
  // Reset the device
  Wire.beginTransmission(address);
  Wire.write(INA228_REG_CONFIG);
  Wire.write((uint8_t)0x80); // MSB: Set reset bit (bit 15)
  Wire.write((uint8_t)0x00); // LSB
  if (Wire.endTransmission() != 0) {
    Serial.print(F("Failed to send reset to INA228 0x")); Serial.println(address, HEX);
    return 0.0f;
  }
  delay(10); // Wait for reset to complete and first conversion cycle
  
  // CORRECTED: device_current_lsb calculation (Amps per bit)
  // For a 20-bit signed current register, the range is effectively +/- 2^19.
  float device_current_lsb = maxCurrent / 524288.0f; // 2^19 = 524288

  // CORRECTED: SHUNT_CAL calculation (assuming ADCRANGE=0, which is default after reset)
  // Datasheet (ina228 (2).pdf, pg 31, Eq. 2): SHUNT_CAL = 13107.2 * 10^6 * CURRENT_LSB * R_SHUNT
  // The SHUNT_CAL register (0x02) is 15 bits (bits 14-0).
  uint16_t calibration = (uint16_t)(13107.2f * 1000000.0f * device_current_lsb * shuntOhms);
  
  // Write calibration value to SHUNTCAL register (MSB first)
  Wire.beginTransmission(address);
  Wire.write(INA228_REG_SHUNTCAL);
  Wire.write((uint8_t)((calibration >> 8) & 0xFF)); // MSB of SHUNT_CAL
  Wire.write((uint8_t)(calibration & 0xFF));       // LSB of SHUNT_CAL
  if (Wire.endTransmission() != 0) {
    Serial.print(F("Failed to write SHUNT_CAL to INA228 0x")); Serial.println(address, HEX);
    return 0.0f;
  }
  
  return device_current_lsb; // Return the calculated current LSB in Amps/bit
}

float readBusVoltage(uint8_t address) {
  uint32_t raw_value = readRegister(address, INA228_REG_VBUS, 3); // Reads 24 bits
  // VBUS is in bits 23-4 of the 24-bit register (so upper 20 bits of the 24-bit field). Value is unsigned.
  // Datasheet (ina228 (2).pdf, pg 5): VBUS LSB = 195.3125 µV
  uint32_t bus_voltage_data = raw_value >> 4; // Isolate the 20-bit data
  return (float)bus_voltage_data * 195.3125f / 1000000.0f; // Result in Volts
}

float readShuntVoltage(uint8_t address) {
  int32_t raw_value = (int32_t)readRegister(address, INA228_REG_VSHUNT, 3); // Reads 24 bits
  // Sign extend 24-bit value to 32-bit based on bit 23 (MSB of the 24-bit field)
  if (raw_value & 0x800000) { 
    raw_value |= 0xFF000000;
  }
  // VSHUNT is in bits 23-4 of the 24-bit register (so upper 20 bits of the 24-bit field).
  // CORRECTED: Shift to get the 20-bit signed data
  int32_t shunt_voltage_data = raw_value >> 4; 
  // LSB is 312.5 nV (for ADCRANGE=0, default). For mV: LSB_mV = 312.5 nV / 1,000,000 (nV/mV) = 0.0003125 mV
  // (Datasheet ina228 (2).pdf, pg 5 & 25)
  return (float)shunt_voltage_data * 0.0003125f; // Result in mV
}

float readCurrent(uint8_t address, float current_lsb_value) {
  if (current_lsb_value == 0.0f) {
    // Serial.print(F("Error: current_lsb_value is 0 for address 0x")); Serial.println(address, HEX);
    return 0.0f; // Avoid division by zero or using uninitialized/error LSB
  }

  int32_t raw_value = (int32_t)readRegister(address, INA228_REG_CURRENT, 3); // Reads 24 bits
  // Sign extend 24-bit value to 32-bit based on bit 23
  if (raw_value & 0x800000) { 
    raw_value |= 0xFF000000;
  }
  // CURRENT is in bits 23-4 of the 24-bit register (so upper 20 bits of the 24-bit field).
  // CORRECTED: Shift to get the 20-bit signed data
  int32_t current_data = raw_value >> 4;
  // current_lsb_value is in Amps/bit
  // Datasheet (ina228 (2).pdf, pg 31, Eq. 4): Current[A] = CURRENT_REGISTER_VALUE * CURRENT_LSB
  // CURRENT_REGISTER_VALUE is `current_data` here.
  return (float)current_data * current_lsb_value * 1000.0f; // Result in mA
}

float readPower(uint8_t address, float current_lsb_value) {
  if (current_lsb_value == 0.0f) {
    // Serial.print(F("Error: current_lsb_value is 0 for address 0x")); Serial.println(address, HEX);
    return 0.0f; // Avoid issues with uninitialized/error LSB
  }

  uint32_t power_register_value = readRegister(address, INA228_REG_POWER, 3); // Reads 24 bits
  // POWER register is 24 bits (0-23), unsigned. No shift needed for this register value itself.
  // Datasheet (ina228 (2).pdf, pg 31, Eq. 5): Power[W] = 3.2 * CURRENT_LSB * POWER_REGISTER_VALUE
  // current_lsb_value is in Amps/bit
  // Result in mW: Power[W] * 1000
  return (float)power_register_value * 3.2f * current_lsb_value * 1000.0f; // Result in mW
}

uint32_t readRegister(uint8_t address, uint8_t reg, uint8_t numBytes) {
  uint32_t value = 0;
  
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) { 
    Serial.print(F("readRegister: Failed to send reg address 0x")); Serial.print(reg, HEX);
    Serial.print(F(" to device 0x")); Serial.println(address, HEX);
    return 0; 
  }
  
  uint8_t bytesRead = Wire.requestFrom(address, numBytes);
  
  if (bytesRead == numBytes) {
    for (uint8_t i = 0; i < numBytes; i++) { // Use uint8_t for loop counter with numBytes
      value = (value << 8) | Wire.read();
    }
  } else {
    Serial.print(F("readRegister: Failed to read ")); Serial.print(numBytes);
    Serial.print(F(" bytes from reg 0x")); Serial.print(reg, HEX);
    Serial.print(F(" on device 0x")); Serial.println(address, HEX);
    Serial.print(F("  Bytes actually read: ")); Serial.println(bytesRead);
    return 0; 
  }
  
  return value;
}