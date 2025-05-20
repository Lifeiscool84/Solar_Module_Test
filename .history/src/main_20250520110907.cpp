#include <Wire.h>
#include <hp_BH1750.h>

// BH1750 Light Sensor
hp_BH1750 lightMeter; // Uses default I2C address 0x23

// Define I2C addresses for INA228 sensors
const uint8_t INA_SOLAR_ADDR = 0x40;
const uint8_t INA_BATTERY_ADDR = 0x44;
const uint8_t INA_LOAD_ADDR = 0x41;

// INA228 register addresses
const uint8_t INA228_REG_CONFIG = 0x00;
const uint8_t INA228_REG_SHUNTCAL = 0x02;
const uint8_t INA228_REG_VSHUNT = 0x04;
const uint8_t INA228_REG_VBUS = 0x05;
const uint8_t INA228_REG_DIETEMP = 0x06;
const uint8_t INA228_REG_CURRENT = 0x07;
const uint8_t INA228_REG_POWER = 0x08;
const uint8_t INA228_REG_ENERGY = 0x09;
const uint8_t INA228_REG_CHARGE = 0x0A;
const uint8_t INA228_REG_DIAGALRT = 0x0B;
const uint8_t INA228_REG_MFG_UID = 0x3E;
const uint8_t INA228_REG_DVC_UID = 0x3F;

// Calibration and scaling variables
float current_lsb = 0.0001; // 100uA per bit

// Function prototypes
bool initINA228(uint8_t address, float shuntOhms = 0.1, float maxCurrent = 5.0);
float readBusVoltage(uint8_t address);
float readShuntVoltage(uint8_t address);
float readCurrent(uint8_t address);
float readPower(uint8_t address);
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

  if (initINA228(INA_SOLAR_ADDR, 0.1, 5.0)) {
    Serial.println(F("INA228 Solar Panel Monitor (0x40) Initialized."));
  } else {
    Serial.print(F("Failed to initialize INA228 Solar Panel Monitor at 0x"));
    Serial.println(INA_SOLAR_ADDR, HEX);
  }

  if (initINA228(INA_BATTERY_ADDR, 0.1, 5.0)) {
    Serial.println(F("INA228 Battery Power Monitor (0x42) Initialized."));
  } else {
    Serial.print(F("Failed to initialize INA228 Battery Power Monitor at 0x"));
    Serial.println(INA_BATTERY_ADDR, HEX);
  }

  if (initINA228(INA_LOAD_ADDR, 0.1, 5.0)) {
    Serial.println(F("INA228 Load Power Monitor (0x41) Initialized."));
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
  Serial.println(F("Solar Panel Monitor (0x40):"));
  Serial.print(F("  Bus Voltage:   ")); Serial.print(readBusVoltage(INA_SOLAR_ADDR)); Serial.println(F(" V"));
  Serial.print(F("  Shunt Voltage: ")); Serial.print(readShuntVoltage(INA_SOLAR_ADDR)); Serial.println(F(" mV"));
  Serial.print(F("  Current:       ")); Serial.print(readCurrent(INA_SOLAR_ADDR)); Serial.println(F(" mA"));
  Serial.print(F("  Power:         ")); Serial.print(readPower(INA_SOLAR_ADDR)); Serial.println(F(" mW"));
  Serial.println();

  // INA228 Battery Power Monitor Data
  Serial.println(F("Battery Power Monitor (0x42):"));
  Serial.print(F("  Bus Voltage:   ")); Serial.print(readBusVoltage(INA_BATTERY_ADDR)); Serial.println(F(" V"));
  Serial.print(F("  Shunt Voltage: ")); Serial.print(readShuntVoltage(INA_BATTERY_ADDR)); Serial.println(F(" mV"));
  Serial.print(F("  Current:       ")); Serial.print(readCurrent(INA_BATTERY_ADDR)); Serial.println(F(" mA"));
  Serial.print(F("  Power:         ")); Serial.print(readPower(INA_BATTERY_ADDR)); Serial.println(F(" mW"));
  Serial.println();

  // INA228 Load Power Monitor Data
  Serial.println(F("Load Power Monitor (0x41):"));
  Serial.print(F("  Bus Voltage:   ")); Serial.print(readBusVoltage(INA_LOAD_ADDR)); Serial.println(F(" V"));
  Serial.print(F("  Shunt Voltage: ")); Serial.print(readShuntVoltage(INA_LOAD_ADDR)); Serial.println(F(" mV"));
  Serial.print(F("  Current:       ")); Serial.print(readCurrent(INA_LOAD_ADDR)); Serial.println(F(" mA"));
  Serial.print(F("  Power:         ")); Serial.print(readPower(INA_LOAD_ADDR)); Serial.println(F(" mW"));
  
  Serial.println(F("--------------------------------------------------"));
  Serial.println();

  delay(1000); 
}

bool initINA228(uint8_t address, float shuntOhms, float maxCurrent) {
  // Check if the device is responding
  Wire.beginTransmission(address);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  
  // Reset the device
  Wire.beginTransmission(address);
  Wire.write(INA228_REG_CONFIG);
  Wire.write((uint8_t)0x80); // Set reset bit in MSB, explicitly cast to uint8_t
  Wire.write((uint8_t)0x00); // Lower byte, explicitly cast to uint8_t
  Wire.endTransmission();
  delay(10); // Wait for reset to complete
  
  // Calculate calibration value
  current_lsb = maxCurrent / 32768.0; // Set LSB to get closest to max current with 15-bit resolution
  uint32_t calibration = (uint32_t)((0.00032768 / (current_lsb * shuntOhms)) * 8192);
  
  // Write calibration value to SHUNTCAL register
  Wire.beginTransmission(address);
  Wire.write(INA228_REG_SHUNTCAL);
  Wire.write((uint8_t)((calibration >> 8) & 0xFF)); // MSB, explicitly cast to uint8_t
  Wire.write((uint8_t)(calibration & 0xFF));         // LSB, explicitly cast to uint8_t
  if (Wire.endTransmission() != 0) {
    return false;
  }
  
  return true;
}

float readBusVoltage(uint8_t address) {
  uint32_t value = readRegister(address, INA228_REG_VBUS, 3);
  // Calculate voltage in volts (195.3125 uV per LSB)
  return (float)(value >> 4) * 195.3125 / 1000000.0;
}

float readShuntVoltage(uint8_t address) {
  int32_t value = (int32_t)readRegister(address, INA228_REG_VSHUNT, 3);
  // Sign extend 24-bit value to 32-bit
  if (value & 0x800000) {
    value |= 0xFF000000;
  }
  // Calculate shunt voltage in mV (312.5 nV per LSB)
  return (float)value * 0.0003125;
}

float readCurrent(uint8_t address) {
  int32_t value = (int32_t)readRegister(address, INA228_REG_CURRENT, 3);
  // Sign extend 24-bit value to 32-bit
  if (value & 0x800000) {
    value |= 0xFF000000;
  }
  // Calculate current in mA
  return (float)value * current_lsb * 1000.0;
}

float readPower(uint8_t address) {
  uint32_t value = readRegister(address, INA228_REG_POWER, 3);
  // Calculate power in mW (power LSB = 3.2 * current_lsb)
  return (float)value * 3.2 * current_lsb * 1000.0;
}

uint32_t readRegister(uint8_t address, uint8_t reg, uint8_t numBytes) {
  uint32_t value = 0;
  
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission(false); // Keep connection active
  
  Wire.requestFrom(address, numBytes);
  
  if (Wire.available() >= numBytes) {
    for (int i = 0; i < numBytes; i++) {
      uint8_t byte = Wire.read();
      value = (value << 8) | byte;
    }
  }
  
  return value;
} 