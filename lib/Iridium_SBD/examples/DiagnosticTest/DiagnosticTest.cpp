#include <Arduino.h>
#include <IridiumSBD.h>
#include <time.h>

#define IridiumSerial Serial3  // Change this to match your setup
#define DIAGNOSTICS true      // Enable diagnostics for detailed feedback

IridiumSBD modem(IridiumSerial);

// Function declarations
void printDiagnosticInfo();

// Diagnostic callbacks
void ISBDConsoleCallback(IridiumSBD *device, char c) {
  Serial.write(c);
}

void ISBDDiagsCallback(IridiumSBD *device, char c) {
  Serial.write(c);
}

void printDiagnosticInfo() {
  Serial.println("\n=== Iridium Module Diagnostic Test ===");
  Serial.println("1. Checking basic communication...");
  
  // Test 1: Basic Communication
  Serial.println("\nInitializing modem...");
  int err = modem.begin();
  if (err != ISBD_SUCCESS) {
    Serial.print("FAILED: Begin failed with error ");
    Serial.println(err);
    if (err == ISBD_NO_MODEM_DETECTED)
      Serial.println("ERROR: No modem detected - Check wiring!");
    return;
  }
  Serial.println("PASSED: Basic communication established");

  // Test 2: Firmware Version
  Serial.println("\n2. Checking firmware version...");
  char version[12];
  err = modem.getFirmwareVersion(version, sizeof(version));
  if (err != ISBD_SUCCESS) {
    Serial.print("FAILED: Firmware version check failed with error ");
    Serial.println(err);
  } else {
    Serial.print("PASSED: Firmware Version: ");
    Serial.println(version);
  }

  // Test 3: Signal Quality
  Serial.println("\n3. Checking signal quality...");
  int signalQuality = -1;
  err = modem.getSignalQuality(signalQuality);
  if (err != ISBD_SUCCESS) {
    Serial.print("FAILED: Signal quality check failed with error ");
    Serial.println(err);
  } else {
    Serial.print("PASSED: Signal Quality: ");
    Serial.print(signalQuality);
    Serial.println("/5");
    if (signalQuality < 2) {
      Serial.println("WARNING: Signal quality is below recommended level (2)");
    }
  }

  // Test 4: Network Status
  Serial.println("\n4. Checking network status...");
  struct tm t;
  err = modem.getSystemTime(t);
  if (err != ISBD_SUCCESS) {
    Serial.print("FAILED: Network check failed with error ");
    Serial.println(err);
    if (err == ISBD_NO_NETWORK)
      Serial.println("ERROR: No network connection - Check antenna and signal!");
  } else {
    Serial.println("PASSED: Network connection established");
    Serial.print("System Time: ");
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "%04d-%02d-%02d %02d:%02d:%02d",
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
             t.tm_hour, t.tm_min, t.tm_sec);
    Serial.println(timeStr);
  }

  // Test 5: Message Sending (Optional - requires credits)
  Serial.println("\n5. Testing message sending (Optional)...");
  Serial.println("Sending test message...");
  err = modem.sendSBDText("Test message from diagnostic program");
  if (err != ISBD_SUCCESS) {
    Serial.print("FAILED: Message sending failed with error ");
    Serial.println(err);
    if (err == ISBD_SENDRECEIVE_TIMEOUT)
      Serial.println("ERROR: Timeout - Check signal quality and antenna!");
  } else {
    Serial.println("PASSED: Test message sent successfully");
  }

  // Test 6: Message Reception
  Serial.println("\n6. Checking for waiting messages...");
  int waitingMessages = modem.getWaitingMessageCount();
  if (waitingMessages >= 0) {
    Serial.print("PASSED: Message check successful. ");
    Serial.print(waitingMessages);
    Serial.println(" messages waiting.");
  } else {
    Serial.println("FAILED: Message check failed");
  }

  Serial.println("\n=== Diagnostic Test Complete ===");
}

void setup() {
  // Start console and satellite modem serial ports
  Serial.begin(115200);
  while (!Serial);  // Wait for serial port to be ready
  
  IridiumSerial.begin(19200);
  
  // Print welcome message
  Serial.println("Iridium Module Diagnostic Program");
  Serial.println("=================================");
  
  // Run diagnostic tests
  printDiagnosticInfo();
}

void loop() {
  // Nothing to do in loop
  delay(1000);
} 