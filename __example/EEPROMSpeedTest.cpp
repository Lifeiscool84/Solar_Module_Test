// EEPROM Benchmark Test for Artemis MicroMod

#include <Arduino.h>
#include <EEPROM.h> // Use standard Arduino EEPROM library interface

// --- Benchmark Constants ---
const uint16_t BUFFER_SIZE = 64;      // Buffer for pattern generation (though writes are byte-by-byte)
uint8_t transferBuffer[BUFFER_SIZE];  // Not directly used for EEPROM write/read timing, but kept for structure similarity

// Smaller test sizes appropriate for typical EEPROM
const uint32_t testDataSizes[] = {
    64,           // 64 Bytes
    128,          // 128 Bytes
    256,          // 256 Bytes
    512,          // 512 Bytes
    1 * 1024      // 1 KB (if EEPROM is large enough)
    // Add more sizes if needed and if EEPROM capacity allows
};
const uint8_t numDataSizes = sizeof(testDataSizes) / sizeof(testDataSizes[0]);
uint32_t eepromSize = 0; // Will be determined in setup()

// --- Function Prototypes ---
void setup();
void loop();
bool timedEEPROMWriteTest(uint32_t dataSize);
bool timedEEPROMReadTest(uint32_t dataSize);

// --- Benchmark Functions ---

// Performs a timed EEPROM write test (byte-by-byte)
bool timedEEPROMWriteTest(uint32_t dataSize) {
    Serial.print("\\n--- EEPROM Write Test: Size=");
    Serial.print(dataSize);
    Serial.println(" bytes ---");

    if (dataSize > eepromSize) {
        Serial.println("  ERROR: Test size exceeds available EEPROM size.");
        return false;
    }

    Serial.print("  Writing data (byte-by-byte)...");

    // Pre-fill buffer with a pattern (optional, just for consistency if we verified)
    for (uint16_t i = 0; i < BUFFER_SIZE; i++) {
        transferBuffer[i] = (uint8_t)(i % 256);
    }

    unsigned long writeStartTime = micros();

    for (uint32_t addr = 0; addr < dataSize; addr++) {
        // Generate a simple value based on address for the test write
        uint8_t valueToWrite = (uint8_t)(addr % 256);
        EEPROM.write(addr, valueToWrite);

        // Progress indicator (less frequent due to slow writes)
        if (dataSize > 256 && addr > 0 && (addr % 64 == 0)) {
             Serial.print(".");
        }
    }
    // Note: EEPROM.write might buffer; a commit/end might be needed depending on core implementation
    // Standard Arduino API doesn't mandate an explicit commit after write, but some cores might need it.
    // Let's assume the write happens immediately or is handled by the library internally for now.

    unsigned long writeEndTime = micros();

    Serial.println(" done.");

    // Calculate and print speed
    unsigned long duration_us = writeEndTime - writeStartTime;
    if (duration_us == 0) duration_us = 1; // Avoid division by zero
    float speed_Bps = (float)dataSize * 1000000.0 / duration_us;
    // KB/s and MB/s are unlikely to be meaningful for EEPROM byte writes
    // float speed_KBps = speed_Bps / 1024.0;

    Serial.print("  Time: "); Serial.print(duration_us / 1000.0, 2); Serial.println(" ms");
    Serial.print("  Speed: "); Serial.print(speed_Bps, 2); Serial.println(" B/s");
    // Serial.print("         "); Serial.print(speed_KBps, 2); Serial.println(" KB/s");

    return true;
}

// Performs a timed EEPROM read test (byte-by-byte)
bool timedEEPROMReadTest(uint32_t dataSize) {
    Serial.print("\\n--- EEPROM Read Test: Size=");
    Serial.print(dataSize);
    Serial.println(" bytes ---");

     if (dataSize > eepromSize) {
        Serial.println("  ERROR: Test size exceeds available EEPROM size.");
        return false;
    }

    Serial.print("  Reading data (byte-by-byte)...");
    unsigned long readStartTime = micros();
    volatile uint8_t tempReadValue; // Use volatile to prevent read optimization

    for (uint32_t addr = 0; addr < dataSize; addr++) {
        tempReadValue = EEPROM.read(addr);

        // Optional: Verification against expected pattern (adds overhead)
        // uint8_t expectedValue = (uint8_t)(addr % 256);
        // if (tempReadValue != expectedValue) {
        //     Serial.print("\\n  ERROR: Verification failed at address "); Serial.print(addr);
        //     Serial.print("! Expected "); Serial.print(expectedValue);
        //     Serial.print(", Read "); Serial.println(tempReadValue);
        //     // Decide whether to stop or continue
        //     // return false;
        // }

         // Progress indicator
        if (dataSize > 256 && addr > 0 && (addr % 256 == 0)) {
             Serial.print(".");
        }
    }

    unsigned long readEndTime = micros();

    Serial.println(" done.");

    // Calculate and print speed
    unsigned long duration_us = readEndTime - readStartTime;
    if (duration_us == 0) duration_us = 1; // Avoid division by zero
    float speed_Bps = (float)dataSize * 1000000.0 / duration_us;
    float speed_KBps = speed_Bps / 1024.0;
    // MB/s unlikely to be meaningful for EEPROM
    // float speed_MBps = speed_KBps / 1024.0;

    Serial.print("  Time: "); Serial.print(duration_us / 1000.0, 2); Serial.println(" ms");
    Serial.print("  Speed: "); Serial.print(speed_Bps, 2); Serial.println(" B/s");
    Serial.print("         "); Serial.print(speed_KBps, 2); Serial.println(" KB/s");
    // Serial.print("         "); Serial.print(speed_MBps, 4); Serial.println(" MB/s");

    return true;
}


void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  unsigned long startMillis = millis();
   while (!Serial && (millis() - startMillis < 5000)) { // Wait max 5 seconds for serial
    delay(100);
  }
  Serial.println("\\n-- Artemis EEPROM Read/Write Benchmark --");

  // Initialize EEPROM (if needed) and get size.
  // The exact initialization might depend on the Apollo3 core implementation.
  // EEPROM.begin() might be required, or size might be known implicitly.
  // Let's assume EEPROM.length() works without an explicit begin, or begin is handled internally.
  eepromSize = EEPROM.length();
  Serial.print("Detected EEPROM Size: ");
  Serial.print(eepromSize);
  Serial.println(" bytes");

  if (eepromSize == 0) {
      Serial.println("ERROR: EEPROM size reported as 0. Cannot run tests.");
      while(1) delay(1);
  }
   // Some cores might require EEPROM.begin(eepromSize); here if length() wasn't enough.
   // If you get errors during write/read, try uncommenting/adding EEPROM.begin().
   // EEPROM.begin(eepromSize);


  Serial.println("Ready to start benchmarks...");
  delay(2000); // Pause before starting
}

void loop() {
    Serial.println("\\n=========================================");
    Serial.println("       Starting EEPROM Benchmark Run     ");
    Serial.println("=========================================");

    // Iterate through data sizes
    for (int j = 0; j < numDataSizes; j++) {
        uint32_t currentSize = testDataSizes[j];

        // Check if test size exceeds available EEPROM
        if (currentSize > eepromSize) {
            Serial.print("\\nSkipping test size "); Serial.print(currentSize);
            Serial.println(" bytes (larger than EEPROM capacity).");
            continue;
        }

        // Perform Write Test
        bool writeOk = timedEEPROMWriteTest(currentSize);

        // Perform Read Test (even if write failed, to see read speed)
        // If verification was enabled in read test, writeOk check might be needed.
        timedEEPROMReadTest(currentSize);


        delay(1000); // Pause briefly between size tests
    } // End data size loop

    Serial.println("\\n=========================================");
    Serial.println("       EEPROM Benchmark Run Complete     ");
    Serial.println("=========================================");
    Serial.println("Halting. Reset board to run again.");

    // Halt execution after one full run
    while(1) {
        delay(100);
    }
} 