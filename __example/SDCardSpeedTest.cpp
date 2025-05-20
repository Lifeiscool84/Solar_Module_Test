#include <SPI.h>
#include <SD.h>
#include <Arduino.h>

const int chipSelect = D0;

// --- Benchmark Constants ---
const char* TEST_FILENAME = "sd_bench.dat";
const uint16_t BUFFER_SIZE = 512; // Buffer for read/write operations
uint8_t transferBuffer[BUFFER_SIZE];

const uint32_t testDataSizes[] = {
    100 * 1024,    // 100 KB
    500 * 1024,    // 500 KB
    1 * 1024 * 1024 // 1 MB
};
const uint8_t numDataSizes = sizeof(testDataSizes) / sizeof(testDataSizes[0]);

// --- Function Prototypes ---
void setup();
void loop();
bool timedSDWriteTest(const char* fileName, uint32_t dataSize);
bool timedSDReadTest(const char* fileName, uint32_t dataSize);


// --- Benchmark Functions ---

// Performs a timed SD card write test
bool timedSDWriteTest(const char* fileName, uint32_t dataSize) {
    Serial.print("--- SD Write Test: Size=");
    Serial.print(dataSize / 1024);
    Serial.println(" KB ---");

    // Remove old test file if it exists
    if (SD.exists(fileName)) {
        Serial.print("  Removing old file...");
        if (!SD.remove(fileName)) {
            Serial.println(" failed.");
            return false;
        }
        Serial.println(" done.");
    }

    // Open the file for writing
    File dataFile = SD.open(fileName, FILE_WRITE);
    if (!dataFile) {
        Serial.println("  ERROR: Could not open file for writing.");
        return false;
    }
    Serial.print("  Writing data...");

    // Fill buffer with a simple pattern for consistency
    for (uint16_t i = 0; i < BUFFER_SIZE; i++) {
        transferBuffer[i] = (uint8_t)(i % 256);
    }

    unsigned long writeStartTime = micros();
    uint32_t bytesRemaining = dataSize;
    uint32_t bytesWrittenTotal = 0;

    while (bytesRemaining > 0) {
        uint16_t bytesToWrite = (bytesRemaining >= BUFFER_SIZE) ? BUFFER_SIZE : (uint16_t)bytesRemaining;

        size_t bytesWritten = dataFile.write(transferBuffer, bytesToWrite);

        if (bytesWritten != bytesToWrite) {
            Serial.print("  ERROR: Write failed! Expected ");
            Serial.print(bytesToWrite);
            Serial.print(", wrote ");
            Serial.println(bytesWritten);
            dataFile.close();
            return false;
        }

        bytesRemaining -= bytesWritten;
        bytesWrittenTotal += bytesWritten;

        // Progress indicator roughly every 64KB
        if (bytesWrittenTotal > 0 && (bytesWrittenTotal % (64 * 1024)) < bytesToWrite) {
             Serial.print(".");
        }
    }
    // Ensure data is flushed to card before closing and timing
    dataFile.flush();
    unsigned long writeEndTime = micros();
    dataFile.close();

    Serial.println(" done.");

    // Calculate and print speed
    unsigned long duration_us = writeEndTime - writeStartTime;
    if (duration_us == 0) duration_us = 1; // Avoid division by zero
    float speed_Bps = (float)dataSize * 1000000.0 / duration_us;
    float speed_KBps = speed_Bps / 1024.0;
    float speed_MBps = speed_KBps / 1024.0;

    Serial.print("  Time: "); Serial.print(duration_us / 1000.0, 2); Serial.println(" ms");
    Serial.print("  Speed: "); Serial.print(speed_Bps, 2); Serial.println(" B/s");
    Serial.print("         "); Serial.print(speed_KBps, 2); Serial.println(" KB/s");
    Serial.print("         "); Serial.print(speed_MBps, 4); Serial.println(" MB/s");

    return true;
}

// Performs a timed SD card read test
bool timedSDReadTest(const char* fileName, uint32_t dataSize) {
    Serial.print("--- SD Read Test: Size=");
    Serial.print(dataSize / 1024);
    Serial.println(" KB ---");

    // Open the file for reading
    File dataFile = SD.open(fileName, FILE_READ);
    if (!dataFile) {
        Serial.println("  ERROR: Could not open file for reading.");
        return false;
    }

    // Check if file size matches expected data size
    if (dataFile.size() != dataSize) {
         Serial.print("  ERROR: File size mismatch! Expected ");
         Serial.print(dataSize);
         Serial.print(", got ");
         Serial.println(dataFile.size());
         dataFile.close();
         // Proceeding with read test anyway, but flag it.
         // return false; // Optionally return false here
    }


    Serial.print("  Reading data...");
    unsigned long readStartTime = micros();
    uint32_t bytesRemaining = dataSize; // Read based on expected size
    uint32_t bytesReadTotal = 0;

    while (bytesRemaining > 0) {
        // Determine bytes to read, ensure it doesn't exceed remaining or buffer size
        uint16_t bytesToRead = (bytesRemaining >= BUFFER_SIZE) ? BUFFER_SIZE : (uint16_t)bytesRemaining;

        // Read into buffer
        int bytesRead = dataFile.read(transferBuffer, bytesToRead);

        if (bytesRead <= 0) { // Error or end of file reached unexpectedly
            Serial.print("  ERROR: Read failed or unexpected EOF. Read ");
            Serial.print(bytesRead);
            Serial.print(" bytes. Total read: ");
            Serial.println(bytesReadTotal);
            dataFile.close();
            return false;
        }

        // --- Basic Verification (Optional - adds overhead) ---
        // for (uint16_t i = 0; i < bytesRead; i++) {
        //     if (transferBuffer[i] != (uint8_t)((bytesReadTotal + i) % 256)) {
        //         Serial.println("  ERROR: Data verification failed!");
        //         dataFile.close();
        //         return false;
        //     }
        // }
        // --- End Verification ---

        bytesRemaining -= bytesRead;
        bytesReadTotal += bytesRead;

        // Progress indicator roughly every 64KB
        if (bytesReadTotal > 0 && (bytesReadTotal % (64 * 1024)) < bytesRead) {
             Serial.print(".");
        }
    }
    unsigned long readEndTime = micros();
    dataFile.close();

    Serial.println(" done.");

    // Calculate and print speed
    unsigned long duration_us = readEndTime - readStartTime;
    if (duration_us == 0) duration_us = 1; // Avoid division by zero
    float speed_Bps = (float)bytesReadTotal * 1000000.0 / duration_us; // Use actual bytes read
    float speed_KBps = speed_Bps / 1024.0;
    float speed_MBps = speed_KBps / 1024.0;

    Serial.print("  Time: "); Serial.print(duration_us / 1000.0, 2); Serial.println(" ms");
    Serial.print("  Speed: "); Serial.print(speed_Bps, 2); Serial.println(" B/s");
    Serial.print("         "); Serial.print(speed_KBps, 2); Serial.println(" KB/s");
    Serial.print("         "); Serial.print(speed_MBps, 4); Serial.println(" MB/s");

    // Basic check if we read the amount we expected
    if (bytesReadTotal != dataSize) {
        Serial.print("  WARNING: Read total (");
        Serial.print(bytesReadTotal);
        Serial.print(") doesn't match expected size (");
        Serial.print(dataSize);
        Serial.println(").");
        // return false; // Optionally return false here
    }


    return true;
}


void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  unsigned long startMillis = millis();
   while (!Serial && (millis() - startMillis < 5000)) { // Wait max 5 seconds for serial
    delay(100);
  }
  Serial.println("-- SD Card Read/Write Benchmark --");


  // Set the chipSelect pin as an output and deselect the card initially
  pinMode(chipSelect, OUTPUT);
  digitalWrite(chipSelect, HIGH);
  delay(10); // Small delay before initialization

  Serial.print("Initializing SD card (CS=D"); Serial.print(chipSelect); Serial.print(")..."); // Assuming D0 definition maps to a number

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println(" Card failed, or not present!");
    // don't do anything more:
    while (1) delay(1); // Halt
  }
  Serial.println(" card initialized.");
  Serial.println("Ready to start benchmarks...");
  delay(2000); // Pause before starting
}

void loop() {
    Serial.println("=========================================");
    Serial.println("         Starting Benchmark Run          ");
    Serial.println("=========================================");

    // Iterate through data sizes
    for (int j = 0; j < numDataSizes; j++) {
        uint32_t currentSize = testDataSizes[j];

        // Perform Write Test
        bool writeOk = timedSDWriteTest(TEST_FILENAME, currentSize);

        // Perform Read Test only if write seemed successful
        if (writeOk) {
            timedSDReadTest(TEST_FILENAME, currentSize);
        } else {
            Serial.println("Skipping Read Test due to Write Failure.");
        }

        delay(1000); // Pause briefly between size tests
    } // End data size loop

    Serial.println("=========================================");
    Serial.println("         Benchmark Run Complete          ");
    Serial.println("=========================================");
    Serial.println("Halting. Reset board to run again.");

    // Halt execution after one full run
    while(1) {
        delay(100);
    }
}