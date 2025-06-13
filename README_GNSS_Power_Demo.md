# GNSS Power Tracking Demonstration

## Overview

Professional demonstration of integrated GNSS tracking with power consumption monitoring, utilizing the complete driver architecture from `Croc_Tracker_Dev_ref`. This demo showcases real-world multi-sensor coordination with precise timestamping, power analysis, and comprehensive data logging.

## Hardware Requirements

### Core Components
- **SparkFun RedBoard Artemis Nano** - Primary microcontroller
- **RV8803 RTC Module** - Precision timekeeping (I2C)
- **u-blox GNSS Module** - GPS/GNSS positioning (I2C)
- **2x INA228 Power Sensors** - High-precision power monitoring
  - Battery sensor: I2C address `0x44`
  - Load sensor: I2C address `0x41`
- **SD Card Module** - Data logging storage
- **Clear sky view** - Required for GNSS signal reception

### Power Monitoring Setup
```
Battery → INA228(0x44) → System Load → INA228(0x41) → Components
```

## Professional Driver Integration

### Architecture Components Used
- **RV8803_Driver** - RTC hardware abstraction (`hardware/sensors/rtc/`)
- **UBLOX_Driver** - GNSS hardware abstraction (`hardware/sensors/gnss/`)
- **TimeManager** - Service coordination layer (`firmware/reference_implementations/`)
- **Professional Error Handling** - RTCStatus, GNSSStatus, TimeManagerStatus enums
- **Advanced Data Structures** - RTCTimestamp, GNSSData, TimeSyncStats

### Service Architecture Benefits
- **Coordinated Time Synchronization** - RTC and GNSS working together
- **Professional Error Handling** - Comprehensive status reporting
- **Graceful Degradation** - System continues if GNSS unavailable
- **Configuration Management** - Runtime adjustable parameters
- **Statistics Tracking** - Performance monitoring and analysis

## Demo Functionality

### Core Operations
1. **Multi-Sensor Initialization**
   - RTC driver startup with status validation
   - GNSS driver initialization with fallback handling
   - TimeManager service coordination setup
   - Triple INA228 power sensor configuration (Solar/Battery/Load)

2. **Integrated Data Collection**
   - GNSS position tracking every 5 seconds
   - Power monitoring every 1 second
   - Timestamped logging via TimeManager
   - Real-time power consumption analysis

3. **Professional Data Logging**
   - CSV format with comprehensive headers
   - ISO8601 timestamps for data correlation
   - GNSS position data (lat/lon/alt/HDOP)
   - Triple power measurements (solar + battery + load)
   - Calculated efficiency and GNSS power estimation

### Data Output Format
```csv
Timestamp_ISO8601,System_Millis_ms,GNSS_Valid,Latitude_deg,Longitude_deg,Altitude_m,Satellites_Used,HDOP,Fix_Type,Solar_Voltage_V,Solar_Current_mA,Solar_Power_mW,Battery_Voltage_V,Battery_Current_mA,Battery_Power_mW,Load_Voltage_V,Load_Current_mA,Load_Power_mW,GNSS_Power_Est_mW,System_Efficiency_pct
```

## Usage Instructions

### 1. Hardware Setup
- Connect all I2C devices to shared bus (SDA/SCL)
- Install SD card in module
- Ensure GNSS module has clear sky view
- Verify all I2C addresses match configuration

### 2. Compilation & Upload
```bash
# In PlatformIO
pio run --target upload --environment artemis_nano
```

### 3. Operation
- Demo runs for 30 minutes automatically
- Serial commands available:
  - `s` - Display status report
  - `q` - Quit demo early
- Data logged to `gnss_power_demo.csv`

### 4. Expected Output
```
=======================================================
    GNSS POWER TRACKING DEMONSTRATION
    Professional Multi-Sensor Integration
=======================================================

=== RTC Driver Initialization ===
RTC driver initialized successfully
...
=== GNSS Driver Initialization ===
GNSS driver initialized successfully
...
=== TimeManager Service Initialization ===
TimeManager service ready
...
=== Starting GNSS Power Tracking Demo ===
Demo Duration: 30 minutes
GNSS Sampling: Every 5 seconds
Power Sampling: Every 1 second
Data Logging: gnss_power_demo.csv
```

## Data Analysis Capabilities

### Real-time Monitoring
- **GNSS Fix Success Rate** - Percentage of successful position fixes
- **Power Consumption Trends** - Battery vs load power analysis
- **System Efficiency** - Power conversion efficiency calculation
- **Time Sync Statistics** - RTC/GNSS synchronization performance

### Power Analysis Features
- **Baseline Power Measurement** - System power without GNSS active
- **GNSS Power Estimation** - Calculated GNSS module consumption
- **Efficiency Calculation** - Load/Battery power ratio
- **Trend Analysis** - Power consumption over time

### CSV Data Processing
Perfect for post-analysis with:
- **Excel/LibreOffice** - Direct CSV import
- **Python/Pandas** - Data science analysis
- **MATLAB** - Engineering analysis
- **R** - Statistical analysis

## Configuration Options

### Demo Parameters
```cpp
const uint32_t GNSS_SAMPLE_INTERVAL_MS = 5000;    // GNSS reading frequency
const uint32_t POWER_SAMPLE_INTERVAL_MS = 1000;   // Power monitoring frequency  
const uint32_t STATUS_REPORT_INTERVAL_MS = 30000; // Status report frequency
const uint32_t DEMO_DURATION_MINUTES = 30;        // Total demo duration
```

### TimeManager Configuration
```cpp
TimeManagerConfig tm_config;
tm_config.sync_interval_ms = 10UL * 60UL * 1000UL;  // 10 minutes for demo
tm_config.enable_automatic_sync = true;
tm_config.fallback_to_rtc = true;
```

## Professional Architecture Benefits

### Compared to Monolithic Code
- **Modularity** - Individual driver testing and validation
- **Reusability** - Drivers usable in other projects
- **Maintainability** - Clean separation of concerns
- **Error Handling** - Professional status reporting
- **Documentation** - Self-documenting code structure

### Service Layer Advantages
- **Coordination** - TimeManager handles RTC/GNSS interaction
- **Configuration** - Runtime parameter adjustment
- **Statistics** - Performance monitoring built-in
- **Fallback** - Graceful degradation strategies

## Troubleshooting

### Common Issues
1. **RTC Initialization Failed**
   - Check I2C connections
   - Verify RTC power supply
   - Confirm I2C address (default: 0x32)

2. **GNSS No Fix**
   - Ensure clear sky view
   - Wait up to 90 seconds for cold start
   - Check antenna connection

3. **Power Sensor Errors**
   - Verify I2C addresses (0x40 Solar, 0x44 Battery, 0x41 Load)
   - Check shunt resistor connections
   - Confirm sensor power supply

### Serial Debug Output
- Enable verbose debugging if needed
- Status reports every 30 seconds
- Real-time error reporting

## Integration with Existing Projects

### Using Professional Drivers
```cpp
#include "Croc_Tracker_Dev_ref/hardware/sensors/rtc/rv8803_driver.h"
#include "Croc_Tracker_Dev_ref/hardware/sensors/gnss/ublox_driver.h"
#include "Croc_Tracker_Dev_ref/firmware/reference_implementations/time_manager.h"

RV8803_Driver rtc;
UBLOX_Driver gnss;
TimeManager time_mgr;

// Initialize with professional error handling
RTCStatus rtc_status = rtc.initialize();
GNSSStatus gnss_status = gnss.initialize();
TimeManagerStatus tm_status = time_mgr.initialize(rtc, gnss);
```

## File Organization

### Demo Files
- `src/gnss_power_tracking_demo.cpp` - Main demonstration
- `README_GNSS_Power_Demo.md` - This documentation

### Professional Drivers (Reference)
- `Croc_Tracker_Dev_ref/hardware/sensors/rtc/` - RTC driver implementation
- `Croc_Tracker_Dev_ref/hardware/sensors/gnss/` - GNSS driver implementation  
- `Croc_Tracker_Dev_ref/firmware/reference_implementations/time_manager.*` - Service coordination

### Legacy Files (For Reference)
- `src/main.txt` - Original function implementations
- `src/sd_power_test_essentials.cpp` - Basic power monitoring reference

## Performance Specifications

### Memory Usage
- **RAM Overhead**: ~368 bytes for TimeManager service
- **Flash Overhead**: ~8KB for complete driver architecture
- **Data Structure**: 128 bytes per log entry

### Timing Performance
- **Periodic Sync Check**: <1ms execution time
- **GNSS Position Read**: 100-500ms depending on fix quality
- **Power Sensor Read**: <10ms per sensor
- **SD Card Write**: 20-50ms per entry

## Future Enhancements

### Possible Extensions
- **Bluetooth Data Streaming** - Real-time data transmission
- **Web Interface** - Browser-based monitoring
- **Advanced Analytics** - Machine learning power prediction
- **Multi-GNSS Support** - GPS + GLONASS + Galileo
- **Low Power Modes** - Sleep/wake power optimization

### Integration Opportunities
- **Solar Panel Monitoring** - Renewable energy tracking
- **Environmental Sensors** - Temperature/humidity correlation
- **Communication Modules** - Remote data transmission
- **Display Integration** - Real-time status display

---

*This demonstration showcases the professional embedded systems development approach using modular drivers, service coordination, and comprehensive error handling. The architecture serves as a foundation for production-ready GNSS tracking systems.* 