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
const uint8_t INA228_REG_ADCCFG = 0x01; // ADC_CONFIG Register
const uint8_t INA228_REG_SHUNTCAL = 0x02;
const uint8_t INA228_REG_VSHUNT = 0x04;
const uint8_t INA228_REG_VBUS = 0x05;
// const uint8_t INA228_REG_DIETEMP = 0x06; // Not used in this version
const uint8_t INA228_REG_CURRENT = 0x07;
const uint8_t INA228_REG_POWER = 0x08;
// const uint8_t INA228_REG_ENERGY = 0x09; // Not used
// const uint8_t INA228_REG_CHARGE = 0x0A; // Not used
// const uint8_t INA228_REG_DIAGALRT = 0x0B; // Not used
const uint8_t INA228_REG_MFG_UID = 0x3E;
const uint8_t INA228_REG_DEVICE_ID = 0x3F; // Corrected name

// Global Shunt Resistor Value (used for Vshunt/Rshunt calculation in loop)
const float SHUNT_OHMS_PROGRAMMED = 0.015f;
//calculated from V_INA_shunt / I_multimeter 4wire method
const float EFFECTIVE_SHUNT_OHMS_BATTERY = 0.017718f; // From V_INA_shunt / I_multimeter

// Calibration and scaling variables
float solar_current_lsb = 0.0f;
float battery_current_lsb = 0.0f;
float load_current_lsb = 0.0f;

// Function prototypes
float initINA228(uint8_t address, const char* sensorName, float shuntOhms = SHUNT_OHMS_PROGRAMMED, float maxCurrent = 5.0f);
void inspectINA228Registers(uint8_t address, const char* sensorName);
float readBusVoltage(uint8_t address, uint32_t& rawRegValue, uint32_t& shiftedRegValue);
float readShuntVoltage(uint8_t address, int32_t& rawRegValue, int32_t& shiftedRegValue);
float readCurrent(uint8_t address, float current_lsb_value, int32_t& rawRegValue, int32_t& shiftedRegValue);
float readPower(uint8_t address, float current_lsb_value, uint32_t& rawRegValue);
uint32_t readRegister(uint8_t address, uint8_t reg, uint8_t numBytes);
uint16_t readRegister16(uint8_t address, uint8_t reg); // Helper for 16-bit registers

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10); 
  }

  Serial.println(F("Starting sensor initialization..."));
  Wire.begin();

  if (lightMeter.begin(BH1750_TO_GROUND)) {
    Serial.println(F("BH1750 Light Sensor Initialized."));
    lightMeter.start();
  } else {
    Serial.println(F("Error initializing BH1750 Light Sensor!"));
  }

  Serial.println(F("\nInitializing INA228 sensors and inspecting registers..."));

  solar_current_lsb = initINA228(INA_SOLAR_ADDR, "Solar", SHUNT_OHMS_PROGRAMMED, 5.0f);
  if (solar_current_lsb > 0.0f) {
    Serial.print(F("INA228 Solar (0x40) Init OK. Current LSB (A/bit): ")); Serial.println(solar_current_lsb, 10);
    inspectINA228Registers(INA_SOLAR_ADDR, "Solar");
  } else {
    Serial.println(F("Failed to initialize INA228 Solar Monitor (0x40)"));
  }

  battery_current_lsb = initINA228(INA_BATTERY_ADDR, "Battery", EFFECTIVE_SHUNT_OHMS_BATTERY, 5.0f);
  if (battery_current_lsb > 0.0f) {
    Serial.print(F("INA228 Battery (0x44) Init OK. Current LSB (A/bit): ")); Serial.println(battery_current_lsb, 10);
    inspectINA228Registers(INA_BATTERY_ADDR, "Battery");
  } else {
    Serial.println(F("Failed to initialize INA228 Battery Monitor (0x44)"));
  }

  load_current_lsb = initINA228(INA_LOAD_ADDR, "Load", SHUNT_OHMS_PROGRAMMED, 5.0f);
  if (load_current_lsb > 0.0f) {
    Serial.print(F("INA228 Load (0x41) Init OK. Current LSB (A/bit): ")); Serial.println(load_current_lsb, 10);
    inspectINA228Registers(INA_LOAD_ADDR, "Load");
  } else {
    Serial.println(F("Failed to initialize INA228 Load Monitor (0x41)"));
  }
  
  Serial.println(F("\nSensor initialization complete. Starting measurements."));
  Serial.println(F("--------------------------------------------------"));
}

void loop() {
  float lux = 0;
  if (lightMeter.hasValue()) {
    lux = lightMeter.getLux();
  }

  Serial.println(F("\n--- Sensor Readings ---"));
  Serial.print(F("Light Intensity: ")); Serial.print(lux); Serial.println(F(" lx\n"));

  // Variables to store raw and shifted register values for debugging
  uint32_t rawVBus, shiftedVBus;
  int32_t rawVShunt, shiftedVShunt;
  int32_t rawCurrent, shiftedCurrent;
  uint32_t rawPower;

  // INA228 Solar Panel Monitor Data
  if (solar_current_lsb > 0.0f) {
    Serial.println(F("Solar Panel Monitor (0x40):"));
    float busV = readBusVoltage(INA_SOLAR_ADDR, rawVBus, shiftedVBus);
    float shuntV = readShuntVoltage(INA_SOLAR_ADDR, rawVShunt, shiftedVShunt);
    float current = readCurrent(INA_SOLAR_ADDR, solar_current_lsb, rawCurrent, shiftedCurrent);
    float power = readPower(INA_SOLAR_ADDR, solar_current_lsb, rawPower);
    float currentFromVshunt = (shuntV / 1000.0f) / SHUNT_OHMS_PROGRAMMED * 1000.0f; // in mA

    Serial.print(F("  Raw VBUS Reg: 0x")); Serial.print(rawVBus, HEX); Serial.print(F(" (Shifted: ")); Serial.print(shiftedVBus); Serial.println(F(")"));
    Serial.print(F("  Bus Voltage:   ")); Serial.print(busV, 4); Serial.println(F(" V"));
    Serial.print(F("  Raw VSHUNT Reg: 0x")); Serial.print(rawVShunt, HEX); Serial.print(F(" (Shifted: ")); Serial.print(shiftedVShunt); Serial.println(F(")"));
    Serial.print(F("  Shunt Voltage: ")); Serial.print(shuntV, 4); Serial.println(F(" mV"));
    Serial.print(F("  Raw CURRENT Reg: 0x")); Serial.print(rawCurrent, HEX); Serial.print(F(" (Shifted: ")); Serial.print(shiftedCurrent); Serial.println(F(")"));
    Serial.print(F("  Current LSB (A/bit): ")); Serial.println(solar_current_lsb, 10);
    Serial.print(F("  Current:       ")); Serial.print(current, 4); Serial.println(F(" mA"));
    Serial.print(F("  Calc. I from Vshunt: ")); Serial.print(currentFromVshunt, 4); Serial.println(F(" mA"));
    Serial.print(F("  Raw POWER Reg: 0x")); Serial.println(rawPower, HEX); // CORRECTED LINE
    Serial.print(F("  Power:         ")); Serial.print(power, 4); Serial.println(F(" mW"));
    Serial.println();
  }

  // INA228 Battery Power Monitor Data
  if (battery_current_lsb > 0.0f) {
    Serial.println(F("Battery Power Monitor (0x44):"));
    float busV = readBusVoltage(INA_BATTERY_ADDR, rawVBus, shiftedVBus);
    float shuntV = readShuntVoltage(INA_BATTERY_ADDR, rawVShunt, shiftedVShunt);
    float current = readCurrent(INA_BATTERY_ADDR, battery_current_lsb, rawCurrent, shiftedCurrent);
    float power = readPower(INA_BATTERY_ADDR, battery_current_lsb, rawPower);
    float currentFromVshunt = (shuntV / 1000.0f) / SHUNT_OHMS_PROGRAMMED * 1000.0f; // in mA

    Serial.print(F("  Raw VBUS Reg: 0x")); Serial.print(rawVBus, HEX); Serial.print(F(" (Shifted: ")); Serial.print(shiftedVBus); Serial.println(F(")"));
    Serial.print(F("  Bus Voltage:   ")); Serial.print(busV, 4); Serial.println(F(" V"));
    Serial.print(F("  Raw VSHUNT Reg: 0x")); Serial.print(rawVShunt, HEX); Serial.print(F(" (Shifted: ")); Serial.print(shiftedVShunt); Serial.println(F(")"));
    Serial.print(F("  Shunt Voltage: ")); Serial.print(shuntV, 4); Serial.println(F(" mV"));
    Serial.print(F("  Raw CURRENT Reg: 0x")); Serial.print(rawCurrent, HEX); Serial.print(F(" (Shifted: ")); Serial.print(shiftedCurrent); Serial.println(F(")"));
    Serial.print(F("  Current LSB (A/bit): ")); Serial.println(battery_current_lsb, 10);
    Serial.print(F("  Current:       ")); Serial.print(current, 4); Serial.println(F(" mA"));
    Serial.print(F("  Calc. I from Vshunt: ")); Serial.print(currentFromVshunt, 4); Serial.println(F(" mA"));
    Serial.print(F("  Raw POWER Reg: 0x")); Serial.println(rawPower, HEX); // CORRECTED LINE
    Serial.print(F("  Power:         ")); Serial.print(power, 4); Serial.println(F(" mW"));
    Serial.println();
  }

  // INA228 Load Power Monitor Data
  if (load_current_lsb > 0.0f) {
    Serial.println(F("Load Power Monitor (0x41):"));
    float busV = readBusVoltage(INA_LOAD_ADDR, rawVBus, shiftedVBus);
    float shuntV = readShuntVoltage(INA_LOAD_ADDR, rawVShunt, shiftedVShunt);
    float current = readCurrent(INA_LOAD_ADDR, load_current_lsb, rawCurrent, shiftedCurrent);
    float power = readPower(INA_LOAD_ADDR, load_current_lsb, rawPower);
    float currentFromVshunt = (shuntV / 1000.0f) / SHUNT_OHMS_PROGRAMMED * 1000.0f; // in mA
    
    Serial.print(F("  Raw VBUS Reg: 0x")); Serial.print(rawVBus, HEX); Serial.print(F(" (Shifted: ")); Serial.print(shiftedVBus); Serial.println(F(")"));
    Serial.print(F("  Bus Voltage:   ")); Serial.print(busV, 4); Serial.println(F(" V"));
    Serial.print(F("  Raw VSHUNT Reg: 0x")); Serial.print(rawVShunt, HEX); Serial.print(F(" (Shifted: ")); Serial.print(shiftedVShunt); Serial.println(F(")"));
    Serial.print(F("  Shunt Voltage: ")); Serial.print(shuntV, 4); Serial.println(F(" mV"));
    Serial.print(F("  Raw CURRENT Reg: 0x")); Serial.print(rawCurrent, HEX); Serial.print(F(" (Shifted: ")); Serial.print(shiftedCurrent); Serial.println(F(")"));
    Serial.print(F("  Current LSB (A/bit): ")); Serial.println(load_current_lsb, 10);
    Serial.print(F("  Current:       ")); Serial.print(current, 4); Serial.println(F(" mA"));
    Serial.print(F("  Calc. I from Vshunt: ")); Serial.print(currentFromVshunt, 4); Serial.println(F(" mA"));
    Serial.print(F("  Raw POWER Reg: 0x")); Serial.println(rawPower, HEX); // CORRECTED LINE
    Serial.print(F("  Power:         ")); Serial.print(power, 4); Serial.println(F(" mW"));
  }
  
  Serial.println(F("--------------------------------------------------"));
  delay(5000); // Increased delay for readability
}

float initINA228(uint8_t address, const char* sensorName, float shuntOhms, float maxCurrent) {
  Serial.print(F("\nAttempting to initialize INA228: ")); Serial.println(sensorName);
  Wire.beginTransmission(address);
  if (Wire.endTransmission() != 0) {
    Serial.print(F("  ERROR: INA228 not found at address 0x")); Serial.println(address, HEX);
    return 0.0f; 
  }
  Serial.print(F("  Device found at 0x")); Serial.println(address, HEX);
  
  Wire.beginTransmission(address);
  Wire.write(INA228_REG_CONFIG);
  Wire.write((uint8_t)0x80); 
  Wire.write((uint8_t)0x00); 
  if (Wire.endTransmission() != 0) {
    Serial.print(F("  ERROR: Failed to send reset to INA228 0x")); Serial.println(address, HEX);
    return 0.0f;
  }
  Serial.println(F("  Device reset command sent."));
  delay(10); 
  
  float device_current_lsb = maxCurrent / 524288.0f; 
  uint16_t calibration = (uint16_t)(13107.2f * 1000000.0f * device_current_lsb * shuntOhms);
  Serial.print(F("  Calculated device_current_lsb (A/bit): ")); Serial.println(device_current_lsb, 10);
  Serial.print(F("  Calculated SHUNT_CAL value: 0x")); Serial.print(calibration, HEX); Serial.print(F(" (")); Serial.print(calibration); Serial.println(F(")"));
  
  Wire.beginTransmission(address);
  Wire.write(INA228_REG_SHUNTCAL);
  Wire.write((uint8_t)((calibration >> 8) & 0xFF)); 
  Wire.write((uint8_t)(calibration & 0xFF));       
  if (Wire.endTransmission() != 0) {
    Serial.print(F("  ERROR: Failed to write SHUNT_CAL to INA228 0x")); Serial.println(address, HEX);
    return 0.0f;
  }
  Serial.println(F("  SHUNT_CAL value written."));
  
  return device_current_lsb;
}

void inspectINA228Registers(uint8_t address, const char* sensorName) {
  Serial.print(F("Inspecting registers for INA228: ")); Serial.println(sensorName);
  
  uint16_t configReg = readRegister16(address, INA228_REG_CONFIG);
  Serial.print(F("  CONFIG (0x00)      : 0x")); Serial.println(configReg, HEX);
  Serial.print(F("    ADCRANGE (Bit 4) : ")); Serial.println((configReg >> 4) & 0x1);

  uint16_t adcConfigReg = readRegister16(address, INA228_REG_ADCCFG);
  Serial.print(F("  ADC_CONFIG (0x01)  : 0x")); Serial.println(adcConfigReg, HEX);
  Serial.print(F("    MODE (Bits 15-12): 0x")); Serial.println((adcConfigReg >> 12) & 0xF, HEX);
  Serial.print(F("    VBUSCT (Bits 11-9): 0x")); Serial.println((adcConfigReg >> 9) & 0x7, HEX);
  Serial.print(F("    VSHCT (Bits 8-6) : 0x")); Serial.println((adcConfigReg >> 6) & 0x7, HEX);
  Serial.print(F("    VTCT (Bits 5-3)  : 0x")); Serial.println((adcConfigReg >> 3) & 0x7, HEX);
  Serial.print(F("    AVG (Bits 2-0)   : 0x")); Serial.println(adcConfigReg & 0x7, HEX);


  uint16_t shuntCalReg = readRegister16(address, INA228_REG_SHUNTCAL);
  Serial.print(F("  SHUNT_CAL (0x02)   : 0x")); Serial.print(shuntCalReg, HEX);
  Serial.print(F(" (")); Serial.print(shuntCalReg); Serial.println(F(")"));

  uint16_t mfgID = readRegister16(address, INA228_REG_MFG_UID);
  Serial.print(F("  MANUFACTURER_ID (0x3E): 0x")); Serial.println(mfgID, HEX);
  
  uint16_t devID = readRegister16(address, INA228_REG_DEVICE_ID);
  Serial.print(F("  DEVICE_ID (0x3F)   : 0x")); Serial.println(devID, HEX);
  Serial.print(F("    DIEID (Bits 15-4): 0x")); Serial.println((devID >> 4) & 0xFFF, HEX);
  Serial.print(F("    REVID (Bits 3-0) : 0x")); Serial.println(devID & 0xF, HEX);
}

uint16_t readRegister16(uint8_t address, uint8_t reg) {
  return (uint16_t)readRegister(address, reg, 2);
}

float readBusVoltage(uint8_t address, uint32_t& rawRegValue, uint32_t& shiftedRegValue) {
  rawRegValue = readRegister(address, INA228_REG_VBUS, 3); 
  shiftedRegValue = rawRegValue >> 4; 
  return (float)shiftedRegValue * 195.3125f / 1000000.0f; 
}

float readShuntVoltage(uint8_t address, int32_t& rawRegValue, int32_t& shiftedRegValue) {
  uint32_t temp_raw = readRegister(address, INA228_REG_VSHUNT, 3);
  rawRegValue = (int32_t)temp_raw; // Store the 24-bit raw value before sign extension for debug print

  // Perform sign extension on a temporary variable if needed, or ensure rawRegValue is correctly representing the 24-bit signedness
  int32_t signed_raw_value = (int32_t)temp_raw; // Cast to int32_t
  if (signed_raw_value & 0x800000) { // Check 24th bit (bit 23 of 0-23 range)
    signed_raw_value |= 0xFF000000;   // Sign extend to 32 bits
  }
  // rawRegValue for printing should ideally be the value before sign extension if you want to see the "as read" 24 bits.
  // The value used for calculation IS the sign-extended one. Let's keep rawRegValue as the direct 24-bit read.

  shiftedRegValue = signed_raw_value >> 4; 
  return (float)shiftedRegValue * 0.0003125f;
}

float readCurrent(uint8_t address, float current_lsb_value, int32_t& rawRegValue, int32_t& shiftedRegValue) {
  if (current_lsb_value == 0.0f) {
    rawRegValue = 0; shiftedRegValue = 0;
    return 0.0f; 
  }
  uint32_t temp_raw = readRegister(address, INA228_REG_CURRENT, 3);
  rawRegValue = (int32_t)temp_raw; // Store the 24-bit raw value

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
    for (uint8_t i = 0; i < numBytes; i++) { 
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