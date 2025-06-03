// Comparison test between our implementation and Adafruit library approach
// Upload this to verify temperature readings match between implementations
#include <Wire.h>

// INA228 addresses and registers
const uint8_t INA_SOLAR_ADDR = 0x40;
const uint8_t INA228_REG_DIETEMP = 0x06;

// Our implementation
uint16_t readRegister16_Our(uint8_t address, uint8_t reg) {
  uint16_t value = 0;
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) == 0) {
    if (Wire.requestFrom(address, (uint8_t)2) == 2) {
      value = (Wire.read() << 8) | Wire.read(); // MSBFIRST
    }
  }
  return value;
}

float readTemperature_Our(uint8_t address) {
  uint16_t rawValue = readRegister16_Our(address, INA228_REG_DIETEMP);
  return (float)rawValue * 0.0078125f; // Our corrected formula
}

// Adafruit library equivalent approach
float readTemperature_Adafruit(uint8_t address) {
  uint16_t rawValue = readRegister16_Our(address, INA228_REG_DIETEMP);
  return (float)rawValue * 7.8125 / 1000.0; // Adafruit formula
}

void setup() {
  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && (millis() - start < 5000)) {
    delay(10);
  }
  
  Serial.println("=== INA228 Temperature Implementation Comparison ===");
  Wire.begin();
  delay(1000);
}

void loop() {
  Serial.println("\n--- Temperature Comparison ---");
  
  // Read raw register value
  uint16_t rawValue = readRegister16_Our(INA_SOLAR_ADDR, INA228_REG_DIETEMP);
  
  // Calculate using both approaches
  float temp_our = readTemperature_Our(INA_SOLAR_ADDR);
  float temp_adafruit = readTemperature_Adafruit(INA_SOLAR_ADDR);
  
  Serial.print("Raw Register Value: 0x"); Serial.print(rawValue, HEX);
  Serial.print(" ("); Serial.print(rawValue); Serial.println(")");
  
  Serial.print("Our Formula:      "); Serial.print(temp_our, 4); Serial.println("°C");
  Serial.print("Adafruit Formula: "); Serial.print(temp_adafruit, 4); Serial.println("°C");
  Serial.print("Difference:       "); Serial.print(abs(temp_our - temp_adafruit), 6); Serial.println("°C");
  
  // Expected results for validation
  Serial.println("\nExpected Results:");
  Serial.println("Raw 0x1000 (4096) = ~32.0°C");
  Serial.println("Raw 0x1900 (6400) = ~50.0°C");
  Serial.println("Raw 0x0C80 (3200) = ~25.0°C");
  
  if (rawValue > 0) {
    if (temp_our >= 15.0 && temp_our <= 60.0) {
      Serial.println("✓ Temperature reading looks reasonable");
    } else {
      Serial.println("✗ Temperature reading seems incorrect");
    }
  } else {
    Serial.println("✗ No valid temperature data");
  }
  
  delay(3000);
} 