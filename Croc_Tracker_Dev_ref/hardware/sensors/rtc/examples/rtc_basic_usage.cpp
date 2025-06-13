/**
 * @file rtc_basic_usage.cpp
 * @brief Basic usage example for RV8803_Driver
 * 
 * Demonstrates how to use the professional RV8803 driver that was extracted
 * from main.txt and modernized for the Croc_Tracker_Dev_ref project structure.
 * 
 * This example shows:
 * - Driver initialization
 * - Reading timestamps
 * - Timezone management
 * - Time adjustments
 * - Error handling
 */

#include <Arduino.h>
#include "../rv8803_driver.h"

// Create RTC driver instance
RV8803_Driver rtc_driver;

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000); // Wait for Serial or timeout
    
    Serial.println(F("=== RV8803 Driver Basic Usage Example ==="));
    Serial.println(F("Extracted from main.txt and modernized"));
    Serial.println();
    
    // Initialize I2C
    Wire.begin();
    
    // Initialize RTC driver
    Serial.print(F("Initializing RTC driver... "));
    RTCStatus status = rtc_driver.initialize();
    
    switch (status) {
        case RTCStatus::SUCCESS:
            Serial.println(F("SUCCESS!"));
            break;
        case RTCStatus::DEVICE_NOT_FOUND:
            Serial.println(F("FAILED - Device not found"));
            return;
        case RTCStatus::COMMUNICATION_ERROR:
            Serial.println(F("FAILED - Communication error"));
            return;
        case RTCStatus::INVALID_TIME:
            Serial.println(F("WARNING - Invalid time stored"));
            break;
        case RTCStatus::COMPILER_TIME_SET_FAILED:
            Serial.println(F("WARNING - Failed to set compiler time"));
            break;
    }
    
    // Show initial status
    Serial.println();
    rtc_driver.printStatus();
    
    // Demonstrate timezone operations
    Serial.println();
    Serial.println(F("=== Timezone Operations ==="));
    Serial.print(F("Current timezone: "));
    Serial.println((rtc_driver.getTimezone() == TimeZone::CST) ? "CST" : "CDT");
    
    // Toggle timezone
    TimeZone new_tz = rtc_driver.toggleTimezone();
    Serial.print(F("After toggle: "));
    Serial.println((new_tz == TimeZone::CST) ? "CST" : "CDT");
    
    // Toggle back
    rtc_driver.toggleTimezone();
    
    // Demonstrate timestamp reading
    Serial.println();
    Serial.println(F("=== Timestamp Reading ==="));
    RTCTimestamp timestamp;
    if (rtc_driver.getTimestamp(timestamp)) {
        Serial.print(F("Date: ")); Serial.println(timestamp.date_str);
        Serial.print(F("Time: ")); Serial.println(timestamp.time_str);
        Serial.print(F("Timezone: "));
        Serial.println((timestamp.timezone == TimeZone::CST) ? "CST" : "CDT");
        Serial.print(F("millis(): ")); Serial.println(timestamp.timestamp_ms);
        Serial.print(F("Valid: ")); Serial.println(timestamp.rtc_valid ? "Yes" : "No");
    } else {
        Serial.println(F("Failed to read timestamp"));
    }
    
    // Demonstrate time adjustment
    Serial.println();
    Serial.println(F("=== Time Adjustment Example ==="));
    Serial.println(F("Adjusting time forward by 10 seconds..."));
    
    TimeAdjustment adjustment;
    adjustment.type = TimeAdjustment::ADD_SECONDS;
    adjustment.seconds = 10;
    
    if (rtc_driver.applyTimeAdjustment(adjustment)) {
        Serial.println(F("Time adjustment successful!"));
        
        // Show new time
        if (rtc_driver.getTimestamp(timestamp)) {
            Serial.print(F("New time: "));
            Serial.print(timestamp.date_str);
            Serial.print(F(" "));
            Serial.println(timestamp.time_str);
        }
        
        // Adjust back
        adjustment.type = TimeAdjustment::SUBTRACT_SECONDS;
        rtc_driver.applyTimeAdjustment(adjustment);
        Serial.println(F("Time restored"));
    } else {
        Serial.println(F("Time adjustment failed"));
    }
    
    Serial.println();
    Serial.println(F("=== Setup Complete ==="));
    Serial.println(F("Loop will demonstrate continuous timestamp reading"));
}

void loop() {
    // Demonstrate continuous timestamp reading
    static unsigned long last_reading = 0;
    
    if (millis() - last_reading >= 5000) { // Every 5 seconds
        RTCTimestamp timestamp;
        
        Serial.print(F("RTC: "));
        if (rtc_driver.getTimestamp(timestamp)) {
            Serial.print(timestamp.date_str);
            Serial.print(F(" "));
            Serial.print(timestamp.time_str);
            Serial.print(F(" ("));
            Serial.print((timestamp.timezone == TimeZone::CST) ? "CST" : "CDT");
            Serial.println(F(")"));
        } else {
            Serial.println(F("Read failed"));
        }
        
        last_reading = millis();
    }
    
    // Handle user commands
    if (Serial.available()) {
        char cmd = Serial.read();
        Serial.read(); // Clear newline
        
        switch (cmd) {
            case 's':
            case 'S':
                rtc_driver.printStatus();
                break;
                
            case 't':
            case 'T':
                rtc_driver.toggleTimezone();
                Serial.println(F("Timezone toggled"));
                break;
                
            case '+':
                {
                    TimeAdjustment adj;
                    adj.type = TimeAdjustment::ADD_SECONDS;
                    adj.seconds = 60; // Add 1 minute
                    if (rtc_driver.applyTimeAdjustment(adj)) {
                        Serial.println(F("Added 1 minute"));
                    }
                }
                break;
                
            case '-':
                {
                    TimeAdjustment adj;
                    adj.type = TimeAdjustment::SUBTRACT_SECONDS;
                    adj.seconds = 60; // Subtract 1 minute
                    if (rtc_driver.applyTimeAdjustment(adj)) {
                        Serial.println(F("Subtracted 1 minute"));
                    }
                }
                break;
                
            case 'h':
            case 'H':
                Serial.println(F("Commands: s=status, t=toggle timezone, +=add minute, -=subtract minute"));
                break;
        }
    }
    
    delay(100);
} 