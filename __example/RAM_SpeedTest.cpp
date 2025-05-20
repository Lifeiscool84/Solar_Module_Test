#include <Arduino.h>
#include <Wire.h>

#include <string.h> // Ensure standard C string functions are included

const uint32_t I2C_CLOCK_SPEED = 400000;

// EEPROM Settings
// const uint8_t EEPROM_ADDR = 0x50; // Default I2C address for CAT24C512
const uint32_t EEPROM_SIZE_BYTES = 65536; // 64KB or 512 Kbit
const uint16_t EEPROM_PAGE_SIZE_BYTES = 128; // Page size for CAT24C512
const uint32_t EEPROM_WRITE_DELAY_MS = 5; // Typical write cycle time

// RAM Benchmark Settings
const size_t RAM_BUFFER_SIZE = 160 * 1024; // Increase to 160KB buffer
// const size_t RAM_TEST_SIZES[] = {1 * 1024, 4 * 1024, 16 * 1024, 64 * 1024, 128 * 1024}; // Removed - Testing max size only
// const size_t NUM_RAM_TEST_SIZES = sizeof(RAM_TEST_SIZES) / sizeof(RAM_TEST_SIZES[0]); // Removed

// Allocate static buffers in RAM
// WARNING: Allocating large static buffers reduces available RAM for dynamic allocation (heap/stack).
// Ensure total static allocation + stack + heap fits within the available 390KB RAM.
// 2x 128KB = 256KB. This leaves ~134KB for stack, heap, and other global/static variables.
uint8_t ramBufferPrimary[RAM_BUFFER_SIZE];
uint8_t ramBufferSecondary[RAM_BUFFER_SIZE];

// Function Prototypes
void timedEEPROMWriteTest(uint32_t dataSize);
void timedEEPROMReadTest(uint32_t dataSize);
void timedRAMWriteTest(size_t dataSize);
void timedRAMReadTest(size_t dataSize);
void timedRAMCopyTest(size_t dataSize);
void printSpeed(uint32_t duration_us, size_t dataSize);

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10); // Wait for serial connection
    }
    Serial.println("Artemis MicroMod EEPROM & RAM Benchmark Test");

    // Configure I2C
    Wire.begin();
    Wire.setClock(I2C_CLOCK_SPEED); // Set I2C clock speed

    /* // Removed - Block relies on removed headers and is not needed for RAM tests
    // Optional: Print I2C speed setting confirmation
    sfe_apollo3_settings_settings_active_t settings;
    settings.val = sfe_apollo3_settings_read(&settings);
    Serial.print("I2C Clock Speed set to: ");
    if(settings.i2cClockSpeedSetting == SFE_APOLLO3_SETTINGS_I2CCLOCK_400KHZ){
        Serial.println("400kHz");
    } else if (settings.i2cClockSpeedSetting == SFE_APOLLO3_SETTINGS_I2CCLOCK_100KHZ){
        Serial.println("100kHz");
    } else if (settings.i2cClockSpeedSetting == SFE_APOLLO3_SETTINGS_I2CCLOCK_1MHZ){
        Serial.println("1MHz");
    } else {
        Serial.println("Unknown");
    }
    */
    Serial.println("I2C Configured (Speed check code removed)"); // Added simple confirmation

    Serial.println("Starting EEPROM Benchmark...");
    // Initialize EEPROM tests here if needed, or handle in loop

    Serial.println("Starting RAM Benchmark...");
    Serial.println("-----------------------------");
    Serial.print("Allocated RAM Buffers: 2 x ");
    Serial.print(RAM_BUFFER_SIZE / 1024);
    Serial.println(" KB");
    Serial.println("-----------------------------");
}

void loop() {
    // --- EEPROM Benchmark ---
    Serial.println("--- EEPROM Benchmark Execution ---");
    uint32_t eepromTestSize = 1024; // Example: Test with 1KB for EEPROM
    Serial.println("Testing EEPROM with data size: " + String(eepromTestSize) + " bytes");

    timedEEPROMWriteTest(eepromTestSize);
    delay(100); // Small delay between tests
    timedEEPROMReadTest(eepromTestSize);

    Serial.println("--- EEPROM Benchmark Complete ---");
    Serial.println("---------------------------------");

    // --- RAM Benchmark ---
    Serial.println("--- RAM Benchmark Execution (Max Size Only) ---");
    Serial.println("Buffer & Test Size: " + String(RAM_BUFFER_SIZE / 1024) + " KB (" + String(RAM_BUFFER_SIZE) + " bytes)");
    Serial.println("---------------------------------------------------");

    // Removed loop - Testing only with RAM_BUFFER_SIZE
    size_t currentTestSize = RAM_BUFFER_SIZE;

    timedRAMWriteTest(currentTestSize);
    delay(50); // Small delay

    timedRAMReadTest(currentTestSize);
    delay(50); // Small delay

    timedRAMCopyTest(currentTestSize);
    delay(50); // Small delay

    Serial.println("--- RAM Benchmark Complete ---");
    Serial.println("------------------------------");

    Serial.println("Tests completed. Entering idle loop (delaying 10 seconds)...");
    delay(10000); // Delay before running tests again
}

// --- Helper Functions ---

// Function to print speed results
void printSpeed(uint32_t duration_us, size_t dataSize) {
    if (duration_us == 0) duration_us = 1; // Prevent division by zero
    float duration_s = (float)duration_us / 1000000.0f;
    float speed_bps = (float)dataSize / duration_s;
    float speed_kbps = speed_bps / 1024.0f;
    float speed_mbps = speed_kbps / 1024.0f;

    Serial.print("  Duration: "); Serial.print(duration_us); Serial.println(" us");
    Serial.print("  Speed: "); Serial.print(speed_bps, 2); Serial.println(" B/s");
    Serial.print("         "); Serial.print(speed_kbps, 2); Serial.println(" KB/s");
    Serial.print("         "); Serial.print(speed_mbps, 2); Serial.println(" MB/s");
}

// --- EEPROM Test Functions ---

void timedEEPROMWriteTest(uint32_t dataSize) {
    // ... existing code ...
}

void timedEEPROMReadTest(uint32_t dataSize) {
    // ... existing code ...
}

// --- RAM Test Functions ---

// Test writing to RAM buffer using memset
void timedRAMWriteTest(size_t dataSize) {
    if (dataSize > RAM_BUFFER_SIZE) {
        Serial.println("Error: dataSize exceeds RAM_BUFFER_SIZE in timedRAMWriteTest");
        return;
    }

    Serial.println("RAM Write Test (memset):");
    uint8_t pattern = 0xA5; // Arbitrary pattern to write

    // Disable interrupts for more accurate timing (optional, can have side effects)
    // noInterrupts();

    uint32_t startTime = micros();
    memset(ramBufferPrimary, pattern, dataSize); // Write pattern to the buffer
    uint32_t endTime = micros();

    // Re-enable interrupts if disabled
    // interrupts();

    uint32_t duration = endTime - startTime;
    printSpeed(duration, dataSize);
}

// Test reading from RAM buffer
void timedRAMReadTest(size_t dataSize) {
    if (dataSize > RAM_BUFFER_SIZE) {
        Serial.println("Error: dataSize exceeds RAM_BUFFER_SIZE in timedRAMReadTest");
        return;
    }

    Serial.println("RAM Read Test (byte-by-byte):");
    volatile uint8_t dummyRead; // Use volatile to prevent optimizer removing the read

    // Disable interrupts for more accurate timing (optional)
    // noInterrupts();

    uint32_t startTime = micros();
    for (size_t i = 0; i < dataSize; ++i) {
        dummyRead = ramBufferPrimary[i]; // Read each byte
    }
    uint32_t endTime = micros();

    // Re-enable interrupts if disabled
    // interrupts();

    uint32_t duration = endTime - startTime;
    printSpeed(duration, dataSize);
    (void)dummyRead; // Avoid unused variable warning
}

// Test copying between RAM buffers using memcpy
void timedRAMCopyTest(size_t dataSize) {
    if (dataSize > RAM_BUFFER_SIZE) {
        Serial.println("Error: dataSize exceeds RAM_BUFFER_SIZE in timedRAMCopyTest");
        return;
    }

    Serial.println("RAM Copy Test (memcpy):");

    // Optional: Fill source buffer first if needed, but don't time this part
    // memset(ramBufferPrimary, 0x5A, dataSize);

    // Disable interrupts for more accurate timing (optional)
    // noInterrupts();

    uint32_t startTime = micros();
    memcpy(ramBufferSecondary, ramBufferPrimary, dataSize); // Copy data between buffers
    uint32_t endTime = micros();

    // Re-enable interrupts if disabled
    // interrupts();

    uint32_t duration = endTime - startTime;
    printSpeed(duration, dataSize);

    // Optional: Verify copy integrity (can add significant overhead)
    // if (memcmp(ramBufferPrimary, ramBufferSecondary, dataSize) != 0) {
    //     Serial.println("  Error: RAM copy verification failed!");
    // } else {
    //     Serial.println("  RAM copy verified successfully.");
    // }
} 