// Test program to demonstrate incremental filename creation
// This will create solar.csv, solar1.csv, solar2.csv, etc.
#include <SPI.h>
#include <SD.h>

const int chipSelect = 8; // Same as main program
char testFilename[20];

void setup() {
  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && (millis() - start < 5000)) {
    delay(10);
  }
  
  Serial.println("=== Solar Log Filename Test ===");
  
  pinMode(chipSelect, OUTPUT);
  digitalWrite(chipSelect, HIGH);
  delay(100);
  
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  
  Serial.println("SD card initialized successfully.");
  
  // Create multiple test files to demonstrate incremental naming
  for (int testRun = 0; testRun < 5; testRun++) {
    Serial.print("\n--- Test Run "); Serial.print(testRun + 1); Serial.println(" ---");
    
    // Find next available filename
    strcpy(testFilename, "solar.csv");
    int fileNumber = 0;
    
    while (SD.exists(testFilename)) {
      fileNumber++;
      snprintf(testFilename, sizeof(testFilename), "solar%d.csv", fileNumber);
    }
    
    // Create the file
    Serial.print("Creating: "); Serial.println(testFilename);
    File testFile = SD.open(testFilename, FILE_WRITE);
    if (testFile) {
      testFile.println("Timestamp_ms,Test_Data");
      testFile.print(millis()); testFile.println(",Test entry");
      testFile.close();
      Serial.print("✓ Successfully created "); Serial.println(testFilename);
    } else {
      Serial.print("✗ Failed to create "); Serial.println(testFilename);
    }
    
    delay(1000);
  }
  
  // List all created files
  Serial.println("\n--- Final File List ---");
  listSolarFiles();
}

void loop() {
  // Nothing to do in loop
  delay(5000);
  Serial.println("Test complete. Check SD card for files: solar.csv, solar1.csv, solar2.csv, etc.");
}

void listSolarFiles() {
  Serial.println("Solar log files on SD card:");
  
  // Check base file
  if (SD.exists("solar.csv")) {
    File file = SD.open("solar.csv", FILE_READ);
    if (file) {
      Serial.print("solar.csv - "); Serial.print(file.size()); Serial.println(" bytes");
      file.close();
    }
  }
  
  // Check numbered files
  int count = 0;
  for (int i = 1; i <= 20; i++) { // Check first 20 numbered files
    char filename[20];
    snprintf(filename, sizeof(filename), "solar%d.csv", i);
    
    if (SD.exists(filename)) {
      File file = SD.open(filename, FILE_READ);
      if (file) {
        Serial.print(filename); Serial.print(" - "); Serial.print(file.size()); Serial.println(" bytes");
        file.close();
        count++;
      }
    }
  }
  
  Serial.print("Total files: "); 
  Serial.println(SD.exists("solar.csv") ? count + 1 : count);
} 