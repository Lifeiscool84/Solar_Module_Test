// INA228 Temperature Test Program
// Upload this to test temperature monitoring functionality
#include <Wire.h>

// INA228 addresses
const uint8_t INA_SOLAR_ADDR = 0x40;
const uint8_t INA_BATTERY_ADDR = 0x44;
const uint8_t INA_LOAD_ADDR = 0x41;

// INA228 registers
const uint8_t INA228_REG_DIETEMP = 0x06;
const uint8_t INA228_REG_ADCCFG = 0x01;

uint16_t readRegister16(uint8_t address, uint8_t reg) {
  uint16_t value = 0;
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) == 0) {
    if (Wire.requestFrom(address, (uint8_t)2) == 2) {
      value = (Wire.read() << 8) | Wire.read();
    }
  }
  return value;
}

float readTemperature(uint8_t address) {
  uint16_t rawValue = readRegister16(address, INA228_REG_DIETEMP);
  // INA228 temperature conversion from datasheet:
  // Temperature (°C) = rawValue * 7.8125e-3
  // No offset needed - direct conversion from raw value
  return (float)rawValue * 0.0078125f;
}

void setup() {
  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && (millis() - start < 5000)) {
    delay(10);
  }
  
  Serial.println("=== INA228 Temperature Test ===");
  Wire.begin();
  
  // Configure each INA228 for temperature monitoring
  uint8_t addresses[] = {INA_SOLAR_ADDR, INA_BATTERY_ADDR, INA_LOAD_ADDR};
  const char* names[] = {"Solar", "Battery", "Load"};
  
  for (int i = 0; i < 3; i++) {
    Serial.print("Configuring "); Serial.print(names[i]); 
    Serial.print(" (0x"); Serial.print(addresses[i], HEX); Serial.print(")...");
    
    // Configure ADC for continuous temperature monitoring
    // MODE = 0xC (continuous temperature only)
    uint16_t adcConfig = 0xC000 | (5 << 3); // Temperature conversion time = 1052 us
    Wire.beginTransmission(addresses[i]);
    Wire.write(INA228_REG_ADCCFG);
    Wire.write((uint8_t)((adcConfig >> 8) & 0xFF));
    Wire.write((uint8_t)(adcConfig & 0xFF));
    
    if (Wire.endTransmission() == 0) {
      Serial.println(" SUCCESS");
    } else {
      Serial.println(" FAILED");
    }
  }
  
  Serial.println("Waiting 2 seconds for temperature readings to stabilize...");
  delay(2000);
}

void loop() {
  Serial.println("\n--- Temperature Readings ---");
  
  uint8_t addresses[] = {INA_SOLAR_ADDR, INA_BATTERY_ADDR, INA_LOAD_ADDR};
  const char* names[] = {"Solar", "Battery", "Load"};
  
  for (int i = 0; i < 3; i++) {
    float temp = readTemperature(addresses[i]);
    Serial.print(names[i]); Serial.print(" (0x"); 
    Serial.print(addresses[i], HEX); Serial.print("): ");
    Serial.print(temp, 2); Serial.println("°C");
  }
  
  delay(2000); // Read every 2 seconds
} 