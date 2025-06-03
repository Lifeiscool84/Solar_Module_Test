// Simple SD Card Test Program
// Upload this first to verify your SD card is working
#include <SPI.h>
#include <SD.h>

const int chipSelect = 8; // Same as main program

void setup() {
  Serial.begin(115200);
  // Wait for serial connection
  unsigned long start = millis();
  while (!Serial && (millis() - start < 5000)) {
    delay(10);
  }
  
  Serial.println("=== SD Card Test Program ===");
  Serial.print("CS Pin: "); Serial.println(chipSelect);
  
  pinMode(chipSelect, OUTPUT);
  digitalWrite(chipSelect, HIGH);
  delay(100);
  
  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println(" FAILED!");
    Serial.println("Check:");
    Serial.println("1. SD card inserted properly");
    Serial.println("2. Wiring connections");
    Serial.println("3. CS pin number (currently: 8)");
    return;
  }
  Serial.println(" SUCCESS!");
  
  // Test write
  Serial.print("Testing write...");
  File testFile = SD.open("test.txt", FILE_WRITE);
  if (testFile) {
    testFile.println("Hello SD Card!");
    testFile.println("Test data: 123.45");
    testFile.close();
    Serial.println(" SUCCESS!");
  } else {
    Serial.println(" FAILED!");
    return;
  }
  
  // Test read
  Serial.print("Testing read...");
  testFile = SD.open("test.txt", FILE_READ);
  if (testFile) {
    Serial.println(" SUCCESS!");
    Serial.println("File contents:");
    while (testFile.available()) {
      Serial.write(testFile.read());
    }
    testFile.close();
  } else {
    Serial.println(" FAILED!");
    return;
  }
  
  // Clean up
  SD.remove("test.txt");
  Serial.println("Test file removed.");
  Serial.println("=== SD Card Test PASSED ===");
  Serial.println("Your SD card is working correctly!");
}

void loop() {
  // Do nothing
} 