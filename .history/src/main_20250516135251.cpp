#include <Arduino.h>
#include <Wire.h>
#include <hp_BH1750.h>
#include <Adafruit_INA228.h>

// BH1750 Light Sensor
hp_BH1750 lightMeter; // Corrected type

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
  // Consider adding Wire.setWireTimeout(5000, true); for I2C stability

  // Initialize BH1750
  // Default address for hp_BH1750 is not automatically 0x23 in begin()
  if (lightMeter.begin(BH1750_TO_GROUND)) { // Use enum for address
    lightMeter.setQuality(BH1750_QUALITY_HIGH); // Set desired quality/mode
    // lightMeter.start(); // May be needed depending on library's continuous mode logic
    Serial.println(F("BH1750 Light Sensor Initialized (0x23)."));
  } else {
    Serial.println(F("Error initializing BH1750 Light Sensor at 0x23!"));
  }

  // Initialize INA228 sensors
  Serial.println(F("Initializing INA228 sensors..."));

  if (ina_solar.begin(INA_SOLAR_ADDR)) {
    Serial.println(F("INA228 Solar Panel Monitor (0x40) Initialized."));
    // ina_solar.setShunt(0.01); // Example: Set shunt resistor (e.g. 10mOhm)
  } else {
    Serial.print(F("Failed to initialize INA228 Solar Panel Monitor at 0x"));
    Serial.println(INA_SOLAR_ADDR, HEX);
  }

  if (ina_battery.begin(INA_BATTERY_ADDR)) {
    Serial.println(F("INA228 Battery Power Monitor (0x42) Initialized."));
    // ina_battery.setShunt(0.01);
  } else {
    Serial.print(F("Failed to initialize INA228 Battery Power Monitor at 0x"));
    Serial.println(INA_BATTERY_ADDR, HEX);
  }

  if (ina_load.begin(INA_LOAD_ADDR)) {
    Serial.println(F("INA228 Load Power Monitor (0x41) Initialized."));
    // ina_load.setShunt(0.01);
  } else {
    Serial.print(F("Failed to initialize INA228 Load Power Monitor at 0x"));
    Serial.println(INA_LOAD_ADDR, HEX);
  }
  
  Serial.println(F("Sensor initialization complete. Starting measurements."));
  Serial.println(F("--------------------------------------------------"));
}

void loop() {
  // Read Light Sensor
  float lux = lightMeter.getLux(); // Corrected function call

  Serial.println(F("--- Sensor Readings ---"));

  // BH1750 Data
  // The hp_BH1750 library might return specific values for errors or saturation.
  // For now, we'll check if it's a typical negative error, but consult lib docs for specifics.
  if (lightMeter.saturated()) {
      Serial.println(F("Light Intensity: Saturated (>65535 lx)"));
  } else if (lux < 0) { // Or specific error codes from hp_BH1750 if available
    Serial.println(F("Error reading from BH1750 or sensor not configured"));
  } else {
    Serial.print(F("Light Intensity: "));
    Serial.print(lux);
    Serial.println(F(" lx"));
  }
  Serial.println();

  // INA228 Solar Panel Monitor Data
  Serial.println(F("Solar Panel Monitor (0x40):"));
  Serial.print(F("  Bus Voltage:   ")); Serial.print(ina_solar.readBusVoltage()); Serial.println(F(" V"));
  Serial.print(F("  Shunt Voltage: ")); Serial.print(ina_solar.readShuntVoltage() * 1000.0); Serial.println(F(" mV"));
  Serial.print(F("  Current:       ")); Serial.print(ina_solar.readCurrent() * 1000.0); Serial.println(F(" mA"));
  Serial.print(F("  Power:         ")); Serial.print(ina_solar.readPower() * 1000.0); Serial.println(F(" mW"));
  Serial.println();

  // INA228 Battery Power Monitor Data
  Serial.println(F("Battery Power Monitor (0x42):"));
  Serial.print(F("  Bus Voltage:   ")); Serial.print(ina_battery.readBusVoltage()); Serial.println(F(" V"));
  Serial.print(F("  Shunt Voltage: ")); Serial.print(ina_battery.readShuntVoltage() * 1000.0); Serial.println(F(" mV"));
  Serial.print(F("  Current:       ")); Serial.print(ina_battery.readCurrent() * 1000.0); Serial.println(F(" mA"));
  Serial.print(F("  Power:         ")); Serial.print(ina_battery.readPower() * 1000.0); Serial.println(F(" mW"));
  Serial.println();

  // INA228 Load Power Monitor Data
  Serial.println(F("Load Power Monitor (0x41):"));
  Serial.print(F("  Bus Voltage:   ")); Serial.print(ina_load.readBusVoltage()); Serial.println(F(" V"));
  Serial.print(F("  Shunt Voltage: ")); Serial.print(ina_load.readShuntVoltage() * 1000.0); Serial.println(F(" mV"));
  Serial.print(F("  Current:       ")); Serial.print(ina_load.readCurrent() * 1000.0); Serial.println(F(" mA"));
  Serial.print(F("  Power:         ")); Serial.print(ina_load.readPower() * 1000.0); Serial.println(F(" mW"));
  
  Serial.println(F("--------------------------------------------------"));
  Serial.println();

  delay(1000); 
} 