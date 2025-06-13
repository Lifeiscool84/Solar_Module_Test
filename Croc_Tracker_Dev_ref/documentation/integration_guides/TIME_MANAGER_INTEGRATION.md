# TimeManager Service Integration Guide

## Overview

This document provides comprehensive guidance for integrating the TimeManager service into the Croc_Tracker_Dev_ref project. The TimeManager provides coordinated time management between RTC and GNSS modules, extracting and modernizing functionality from `main.txt` to create a professional service-layer architecture.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Function Mapping](#function-mapping)
3. [Integration Steps](#integration-steps)
4. [Configuration](#configuration)
5. [Service Patterns](#service-patterns)
6. [Error Handling](#error-handling)
7. [Performance Considerations](#performance-considerations)
8. [Testing and Validation](#testing-and-validation)
9. [Troubleshooting](#troubleshooting)
10. [Future Enhancements](#future-enhancements)

## Architecture Overview

### Design Principles

The TimeManager follows service-layer architecture principles:

- **Service Coordination**: Orchestrates RTC and GNSS drivers for time synchronization
- **Policy Management**: Implements automatic sync policies and fallback strategies
- **Error Isolation**: Provides robust error handling with detailed status reporting
- **Configuration Management**: Flexible configuration with validation and runtime updates
- **Statistics Tracking**: Comprehensive monitoring and reporting capabilities

### Service Layer Architecture

```
Application Layer
    ↓
TimeManager Service ← Configuration & Policies
    ↓         ↓
RTC Driver  GNSS Driver ← Hardware Abstraction Layer
    ↓         ↓
RV8803      u-blox Module ← Hardware Layer
```

## Function Mapping

### Original Code Extraction

| Original Function (main.txt) | TimeManager Method | Enhancement |
|------------------------------|-------------------|-------------|
| `bool syncRTCWithGNSSLocal()` | `TimeManagerStatus syncWithGNSS()` | Professional error handling, timezone management |
| `void checkPeriodicGNSSTimeSync()` | `TimeManagerStatus periodicSync()` | Configurable intervals, comprehensive statistics |

### Enhanced Functionality

The TimeManager provides significant enhancements over original implementations:

1. **Service Layer Architecture**: Clean separation between hardware drivers and application logic
2. **Professional Error Handling**: Detailed status codes instead of simple boolean returns
3. **Configuration Management**: Runtime configuration updates with validation
4. **Statistics Tracking**: Comprehensive sync monitoring and success rate tracking
5. **Policy Management**: Flexible sync intervals and fallback strategies

## Integration Steps

### Step 1: Driver Initialization

```cpp
#include "firmware/reference_implementations/time_manager.h"

// Initialize hardware drivers first
RV8803_Driver rtc_driver;
UBLOX_Driver gnss_driver;

void setup() {
    // Initialize I2C
    Wire.begin();
    
    // Initialize drivers
    RTCStatus rtc_status = rtc_driver.initialize();
    GNSSStatus gnss_status = gnss_driver.initialize();
    
    // Drivers must be ready before TimeManager
    if (rtc_status != RTCStatus::SUCCESS) {
        // Handle RTC initialization failure
    }
    
    if (gnss_status != GNSSStatus::SUCCESS) {
        // GNSS can fail, TimeManager supports fallback
        Serial.println("GNSS unavailable, using RTC-only mode");
    }
}
```

### Step 2: TimeManager Configuration

```cpp
// Configure TimeManager service
TimeManagerConfig config;
config.sync_interval_ms = 2UL * 60UL * 60UL * 1000UL;  // 2 hours
config.enable_automatic_sync = true;
config.require_gnss_validation = true;
config.fallback_to_rtc = true;
config.default_timezone = TimeZone::CST;
config.sync_timeout_ms = 30000;
config.max_sync_retries = 3;

TimeManager time_manager(config);
```

### Step 3: Service Initialization

```cpp
TimeManagerStatus tm_status = time_manager.initialize(rtc_driver, gnss_driver);
if (tm_status != TimeManagerStatus::SUCCESS) {
    Serial.print("TimeManager initialization failed: ");
    Serial.println(TimeManager::getStatusString(tm_status));
    // Handle initialization failure
}
```

### Step 4: Periodic Operations

```cpp
void loop() {
    // Check for periodic sync (call frequently)
    TimeManagerStatus sync_result = time_manager.periodicSync();
    
    // Get current time when needed
    RTCTimestamp current_time;
    if (time_manager.getCurrentTime(current_time)) {
        // Use current_time.iso8601_string
    }
    
    // Monitor statistics
    const TimeSyncStats& stats = time_manager.getStatistics();
    
    delay(1000);  // Application main loop
}
```

## Configuration

### TimeManagerConfig Structure

```cpp
struct TimeManagerConfig {
    uint32_t sync_interval_ms;      // 2 hours default
    bool enable_automatic_sync;     // true
    bool require_gnss_validation;   // true
    bool fallback_to_rtc;          // true
    TimeZone default_timezone;      // CST/CDT
    uint16_t sync_timeout_ms;       // 30 seconds
    uint8_t max_sync_retries;       // 3 attempts
};
```

### Configuration Validation

The TimeManager validates configuration parameters:

- **Sync Interval**: 5 minutes to 24 hours
- **Timeout**: 1 second to 5 minutes
- **Retries**: 0 to 10 attempts

### Runtime Configuration Updates

```cpp
// Update configuration during runtime
TimeManagerConfig new_config = time_manager.getConfig();
new_config.sync_interval_ms = 1UL * 60UL * 60UL * 1000UL;  // 1 hour
time_manager.updateConfig(new_config);
```

## Service Patterns

### 1. Automatic Periodic Sync

```cpp
// Enable automatic sync every 2 hours
time_manager.setAutomaticSync(true);
time_manager.setSyncInterval(2UL * 60UL * 60UL * 1000UL);

// In main loop - just call periodicSync()
void loop() {
    time_manager.periodicSync();  // Handles timing internally
    // Your application code
}
```

### 2. Manual Sync Control

```cpp
// Disable automatic sync for manual control
time_manager.setAutomaticSync(false);

// Force sync when needed
if (some_condition) {
    TimeManagerStatus result = time_manager.syncWithGNSS();
    if (result == TimeManagerStatus::SUCCESS) {
        Serial.println("Time synchronized successfully");
    }
}
```

### 3. Fallback Strategy

```cpp
// Configure graceful degradation
TimeManagerConfig config;
config.fallback_to_rtc = true;           // Allow RTC-only operation
config.require_gnss_validation = false;  // Less strict GNSS requirements

// TimeManager will automatically fallback when GNSS unavailable
```

### 4. Timezone Management

```cpp
// Set timezone
time_manager.setDefaultTimezone(TimeZone::CST);

// Toggle between CST/CDT
TimeZone new_tz = time_manager.toggleTimezone();

// Get current timezone
TimeZone current_tz = time_manager.getDefaultTimezone();
```

## Error Handling

### TimeManagerStatus Enum

```cpp
enum class TimeManagerStatus : uint8_t {
    SUCCESS = 0,
    RTC_NOT_AVAILABLE = 1,
    GNSS_NOT_AVAILABLE = 2,
    GNSS_NO_FIX = 3,
    GNSS_INVALID_TIME = 4,
    RTC_SET_FAILED = 5,
    COMMUNICATION_ERROR = 6,
    INVALID_CONFIGURATION = 7,
    SYNC_IN_PROGRESS = 8,
    SYNC_NOT_NEEDED = 9,
    INITIALIZATION_FAILED = 10
};
```

### Error Handling Patterns

```cpp
// Handle specific error conditions
TimeManagerStatus result = time_manager.syncWithGNSS();
switch (result) {
    case TimeManagerStatus::SUCCESS:
        // Sync successful
        break;
        
    case TimeManagerStatus::GNSS_NO_FIX:
        Serial.println("GNSS has no fix - waiting for satellites");
        break;
        
    case TimeManagerStatus::RTC_SET_FAILED:
        Serial.println("RTC communication error");
        // Try RTC driver reset
        break;
        
    default:
        Serial.print("Sync failed: ");
        Serial.println(TimeManager::getStatusString(result));
        break;
}
```

### Graceful Degradation

```cpp
// Check service health
if (!time_manager.isReady()) {
    Serial.println("TimeManager not ready - using fallback time");
    // Use alternative time source or cached time
}

// Monitor driver availability
const TimeSyncStats& stats = time_manager.getStatistics();
if (!stats.is_gnss_available) {
    Serial.println("Operating in RTC-only mode");
}
```

## Performance Considerations

### Memory Usage

- **TimeManager Service**: ~200 bytes RAM
- **Configuration**: ~32 bytes
- **Statistics**: ~48 bytes
- **Total**: ~280 bytes additional RAM

### CPU Usage

- **Periodic Check**: ~1ms every 5 seconds (configurable)
- **GNSS Sync**: ~100-500ms (depends on GNSS response time)
- **RTC Operations**: ~1-5ms

### Timing Considerations

```cpp
// Optimal call frequency for periodicSync()
void loop() {
    // Call every 1-5 seconds for responsive sync timing
    time_manager.periodicSync();
    
    // Avoid calling more frequently than every 100ms
    delay(1000);
}
```

### Power Management

```cpp
// For battery applications - adjust sync frequency
TimeManagerConfig low_power_config;
low_power_config.sync_interval_ms = 4UL * 60UL * 60UL * 1000UL;  // 4 hours
low_power_config.sync_timeout_ms = 15000;  // Faster timeout
low_power_config.max_sync_retries = 1;     // Fewer retries
```

## Testing and Validation

### Unit Testing Approach

```cpp
void test_timemanager_basic() {
    TimeManager tm;
    
    // Test initialization
    TimeManagerStatus status = tm.initialize(rtc_driver, gnss_driver);
    assert(status == TimeManagerStatus::SUCCESS);
    
    // Test configuration
    assert(tm.isReady());
    
    // Test sync operations
    status = tm.syncWithGNSS();
    // Validate based on GNSS availability
}
```

### Integration Testing

```cpp
void test_timemanager_integration() {
    // Test with various driver states
    test_rtc_only_mode();
    test_gnss_unavailable();
    test_gnss_no_fix();
    test_successful_sync();
    
    // Test configuration changes
    test_timezone_toggle();
    test_interval_changes();
    
    // Test error recovery
    test_sync_failures();
    test_driver_recovery();
}
```

### Field Testing

1. **GNSS Reception**: Test in various environments (indoor/outdoor)
2. **Long-term Operation**: Monitor sync success rates over days
3. **Power Cycles**: Test recovery after system restart
4. **Time Accuracy**: Validate synchronized time accuracy

## Troubleshooting

### Common Issues

#### 1. TimeManager Initialization Fails

**Symptoms**: `INITIALIZATION_FAILED` status

**Causes**:
- RTC driver not initialized
- Invalid configuration
- I2C communication issues

**Solutions**:
```cpp
// Verify driver initialization
if (!rtc_driver.isReady()) {
    Serial.println("RTC driver not ready");
    // Re-initialize RTC
}

// Check configuration
if (!TimeManager::validateConfig(config)) {
    Serial.println("Invalid configuration");
    // Use default configuration
}
```

#### 2. GNSS Sync Never Succeeds

**Symptoms**: Continuous `GNSS_NO_FIX` or `GNSS_NOT_AVAILABLE`

**Causes**:
- Poor satellite reception
- GNSS module not initialized
- Validation criteria too strict

**Solutions**:
```cpp
// Check GNSS status
if (!gnss_driver.hasValidFix()) {
    Serial.println("Waiting for GNSS fix...");
    // Move to area with better sky view
}

// Reduce validation requirements temporarily
TimeManagerConfig config = time_manager.getConfig();
config.require_gnss_validation = false;
time_manager.updateConfig(config);
```

#### 3. RTC Time Keeps Resetting

**Symptoms**: Time resets after power cycle

**Causes**:
- RTC battery low
- RTC communication issues
- Incorrect timezone handling

**Solutions**:
```cpp
// Check RTC validity
if (!rtc_driver.validateStoredTime()) {
    Serial.println("RTC time invalid - setting to compiler time");
    rtc_driver.forceCompilerTime();
}

// Monitor RTC status
rtc_driver.printStatus();
```

### Diagnostic Tools

```cpp
void diagnose_timemanager() {
    Serial.println("=== TimeManager Diagnostics ===");
    
    // Service status
    time_manager.printStatus();
    time_manager.printStatistics();
    
    // Driver status
    Serial.println("\n=== Driver Status ===");
    rtc_driver.printStatus();
    gnss_driver.printStatus();
    
    // Hardware tests
    Serial.println("\n=== Hardware Tests ===");
    test_rtc_communication();
    test_gnss_communication();
}
```

## Future Enhancements

### Planned Features

1. **NTP Synchronization**: Internet-based time sync option
2. **Time Zone Database**: Support for multiple time zones
3. **Sync History**: Log of sync events for analysis
4. **Power Optimization**: Advanced power management modes
5. **External Time Sources**: Support for additional time references

### Extension Points

```cpp
// Future: Custom time source interface
class TimeSource {
public:
    virtual bool getTime(RTCTimestamp& time) = 0;
    virtual bool isAvailable() = 0;
};

// Future: Multiple time source support
class AdvancedTimeManager : public TimeManager {
    void addTimeSource(TimeSource* source);
    void setTimeSources(const std::vector<TimeSource*>& sources);
};
```

### Integration with Other Services

```cpp
// Future: Event-driven architecture
class TimeEventHandler {
public:
    virtual void onTimeSync(bool successful) = 0;
    virtual void onTimeZoneChange(TimeZone new_tz) = 0;
    virtual void onSyncFailure(TimeManagerStatus error) = 0;
};

// Register for time events
time_manager.registerEventHandler(&my_handler);
```

## Migration Guide

### From Legacy Code

If migrating from `main.txt` implementation:

1. **Replace Function Calls**:
   ```cpp
   // Old
   if (syncRTCWithGNSSLocal()) {
       // Success
   }
   
   // New
   TimeManagerStatus result = time_manager.syncWithGNSS();
   if (result == TimeManagerStatus::SUCCESS) {
       // Success
   }
   ```

2. **Periodic Sync Migration**:
   ```cpp
   // Old
   checkPeriodicGNSSTimeSync();
   
   // New
   time_manager.periodicSync();  // Handles timing internally
   ```

3. **Time Reading Migration**:
   ```cpp
   // Old
   updateRTCTimestamp(sensor_data);
   
   // New
   RTCTimestamp timestamp;
   if (time_manager.getCurrentTime(timestamp)) {
       // Use timestamp.iso8601_string
   }
   ```

## Conclusion

The TimeManager service provides a professional, service-layer approach to time synchronization that enhances the original functionality from `main.txt` while maintaining compatibility with existing hardware drivers. The service architecture enables robust error handling, flexible configuration, and comprehensive monitoring for production embedded systems.

The integration patterns described in this guide ensure reliable time management across various operational scenarios while providing the foundation for future enhancements and multi-sensor integration. 