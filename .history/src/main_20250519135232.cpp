#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_INA228.h>

// BH1750 Light Sensor
BH1750 lightMeter; // Uses default I2C address 0x23

// INA228 Current/Voltage Sensors
Adafruit_INA228 ina_solar;
Adafruit_INA228 ina_battery;
Adafruit_INA228 ina_load;

// Define I2C addresses for INA228 sensors
const uint8_t INA_SOLAR_ADDR = 0x40;
const uint8_t INA_BATTERY_ADDR = 0x42;
const uint8_t INA_LOAD_ADDR = 0x41;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // wait for serial port to connect. Needed for native USB
  }

  Serial.println(F("Starting sensor initialization..."));

  // Initialize I2C bus
  Wire.begin();

  // Initialize BH1750
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("BH1750 Light Sensor Initialized."));
  } else {
    Serial.println(F("Error initializing BH1750 Light Sensor!"));
  }

  // Initialize INA228 sensors
  Serial.println(F("Initializing INA228 sensors..."));

  if (ina_solar.begin(INA_SOLAR_ADDR)) {
    Serial.println(F("INA228 Solar Panel Monitor (0x40) Initialized."));
    // Configure INA228 settings if needed (using defaults for now)
    // Example: ina_solar.setShuntResistor_mOhm(100); // 100 mOhm shunt
    // ina_solar.setADCMode(INA228_MODE_CONT_SHUNT_BUS);
    // ina_solar.setADCRange(INA228_ADCRANGE_163MV); // or INA228_ADCRANGE_40MV
  } else {
    Serial.print(F("Failed to initialize INA228 Solar Panel Monitor at 0x"));
    Serial.println(INA_SOLAR_ADDR, HEX);
  }

  if (ina_battery.begin(INA_BATTERY_ADDR)) {
    Serial.println(F("INA228 Battery Power Monitor (0x42) Initialized."));
  } else {
    Serial.print(F("Failed to initialize INA228 Battery Power Monitor at 0x"));
    Serial.println(INA_BATTERY_ADDR, HEX);
  }

  if (ina_load.begin(INA_LOAD_ADDR)) {
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
  float lux = lightMeter.readLightLevel();

  Serial.println(F("--- Sensor Readings ---"));

  // BH1750 Data
  if (lux < 0) {
    Serial.println(F("Error reading from BH1750 or sensor not configured"));
  } else {
    Serial.print(F("Light Intensity: "));
    Serial.print(lux);
    Serial.println(F(" lx"));
  }
  Serial.println();

  // INA228 Solar Panel Monitor Data
  Serial.println(F("Solar Panel Monitor (0x40):"));
  Serial.print(F("  Bus Voltage:   ")); Serial.print(ina_solar.getBusVoltage_V()); Serial.println(F(" V"));
  Serial.print(F("  Shunt Voltage: ")); Serial.print(ina_solar.getShuntVoltage_mV()); Serial.println(F(" mV"));
  Serial.print(F("  Current:       ")); Serial.print(ina_solar.getCurrent_mA()); Serial.println(F(" mA"));
  Serial.print(F("  Power:         ")); Serial.print(ina_solar.getPower_mW()); Serial.println(F(" mW"));
  Serial.println();

  // INA228 Battery Power Monitor Data
  Serial.println(F("Battery Power Monitor (0x42):"));
  Serial.print(F("  Bus Voltage:   ")); Serial.print(ina_battery.getBusVoltage_V()); Serial.println(F(" V"));
  Serial.print(F("  Shunt Voltage: ")); Serial.print(ina_battery.getShuntVoltage_mV()); Serial.println(F(" mV"));
  Serial.print(F("  Current:       ")); Serial.print(ina_battery.getCurrent_mA()); Serial.println(F(" mA"));
  Serial.print(F("  Power:         ")); Serial.print(ina_battery.getPower_mW()); Serial.println(F(" mW"));
  Serial.println();

  // INA228 Load Power Monitor Data
  Serial.println(F("Load Power Monitor (0x41):"));
  Serial.print(F("  Bus Voltage:   ")); Serial.print(ina_load.getBusVoltage_V()); Serial.println(F(" V"));
  Serial.print(F("  Shunt Voltage: ")); Serial.print(ina_load.getShuntVoltage_mV()); Serial.println(F(" mV"));
  Serial.print(F("  Current:       ")); Serial.print(ina_load.getCurrent_mA()); Serial.println(F(" mA"));
  Serial.print(F("  Power:         ")); Serial.print(ina_load.getPower_mW()); Serial.println(F(" mW"));
  
  Serial.println(F("--------------------------------------------------"));
  Serial.println();

  delay(1000); 
} 