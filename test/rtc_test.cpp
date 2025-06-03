// RTC Test Program for RV8803
// Upload this to test RTC functionality before using in main program
#include <Wire.h>
#include <SparkFun_RV8803.h>

RV8803 rtc;

void setup() {
  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && (millis() - start < 5000)) {
    delay(10);
  }
  
  Serial.println("=== RV8803 RTC Test Program ===");
  Wire.begin();
  
  if (rtc.begin() == false) {
    Serial.println("ERROR: RTC initialization failed!");
    Serial.println("Check wiring and I2C connections.");
    return;
  }
  
  Serial.println("RTC initialized successfully!");
  
  // Set timezone to Central Standard Time (CST = UTC-6)
  rtc.setTimeZoneQuarterHours(-24); // -6 hours in quarter hours
  Serial.println("Time zone set to CST (UTC-6)");
  
  // Option to set to compiler time
  Serial.println("\nWould you like to set RTC to compiler time? (y/n)");
  Serial.println("You have 10 seconds to respond...");
  
  unsigned long timeout = millis();
  while (millis() - timeout < 10000) {
    if (Serial.available()) {
      char response = Serial.read();
      if (response == 'y' || response == 'Y') {
        if (rtc.setToCompilerTime()) {
          Serial.println("RTC set to compiler time successfully!");
        } else {
          Serial.println("Failed to set compiler time!");
        }
        break;
      } else if (response == 'n' || response == 'N') {
        Serial.println("Using existing RTC time...");
        break;
      }
    }
    delay(100);
  }
  
  Serial.println("\nContinuing with RTC readings...");
}

void loop() {
  if (rtc.updateTime() == false) {
    Serial.println("RTC update failed!");
    delay(1000);
    return;
  }
  
  // Display various time formats
  Serial.println("\n--- RTC Time Readings ---");
  
  // Basic time components
  Serial.print("Date: "); Serial.print(rtc.getMonth()); Serial.print("/");
  Serial.print(rtc.getDate()); Serial.print("/"); Serial.println(rtc.getYear() + 2000);
  
  Serial.print("Time: "); Serial.print(rtc.getHours()); Serial.print(":");
  if (rtc.getMinutes() < 10) Serial.print("0");
  Serial.print(rtc.getMinutes()); Serial.print(":");
  if (rtc.getSeconds() < 10) Serial.print("0");
  Serial.println(rtc.getSeconds());
  
  // String formats
  char buffer[50];
  rtc.stringDateUSA(buffer, sizeof(buffer));
  Serial.print("Date (USA): "); Serial.println(buffer);
  
  rtc.stringTime(buffer, sizeof(buffer));
  Serial.print("Time: "); Serial.println(buffer);
  
  rtc.stringTime8601(buffer, sizeof(buffer));
  Serial.print("ISO 8601: "); Serial.println(buffer);
  
  rtc.stringTime8601TZ(buffer, sizeof(buffer));
  Serial.print("ISO 8601 TZ: "); Serial.println(buffer);
  
  // Time zone info
  int8_t tz = rtc.getTimeZoneQuarterHours();
  Serial.print("Time Zone: UTC"); 
  if (tz < 0) Serial.print(tz / 4.0f, 1);
  else if (tz > 0) { Serial.print("+"); Serial.print(tz / 4.0f, 1); }
  else Serial.print("+0");
  Serial.println();
  
  delay(2000); // Update every 2 seconds
} 