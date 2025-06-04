# SD Card Power Consumption Testing System

## Quick Start

### 🚀 Overview
Comprehensive power consumption testing for SD card operations across 11 operational modes and 9 system statuses using existing INA228 infrastructure.

### 📋 Requirements
- SparkFun RedBoard Artemis Nano
- 3x INA228 power monitors (addresses 0x40, 0x44, 0x41)
- SD card connected to pin 8
- PlatformIO or Arduino IDE

### ⚡ Quick Setup

1. **Upload the firmware:**
   ```bash
   pio run --target upload
   ```

2. **Open Serial Monitor (115200 baud):**
   ```
   Serial Monitor Commands:
   'h' - Help menu
   'a' - Run comprehensive test  
   'v' - Validate environment
   'r' - Detailed report
   's' - Summary report
   'c' - Clear data
   ```

3. **Run a complete test:**
   ```
   > v  (validate environment)
   > a  (run comprehensive test)
   > s  (view summary report)
   ```

### 📊 What It Tests

**SD Card Modes (11):**
- Initialization, File Open/Close, Data Writing/Reading
- File Flushing, Metadata Operations, Idle States
- Low Power Modes, Hardware Power Off

**System Statuses (9):**
- System Init, Sensor Acquisition, SD Preparation
- Buffer Writing, SD Finalization, Power Transitions
- MCU Idle, Read Operations, Error Recovery

### 📈 Output Data

**Generated Files:**
- `sd_power_data.csv` - Raw measurement data
- `sd_power_log.txt` - Test execution log  
- Console reports with statistics and recommendations

**Key Metrics:**
- Power consumption (mW) per mode/status combination
- Peak detection and energy calculations
- Statistical analysis with standard deviation
- Power optimization recommendations

### 🔧 Configuration

Modify test parameters in `src/sd_power_test_config.h`:
```cpp
const uint16_t SAMPLES_PER_MODE_TEST = 1000;    // Samples per test
const uint16_t SAMPLE_INTERVAL_MICROSECONDS = 50; // 20kHz sampling
const uint16_t TEST_ITERATION_COUNT = 5;        // Test repetitions
```

### 📖 Documentation

Full documentation: `docs/SD_Power_Testing_Guide.md`

### 🛠️ Integration

- **Standalone**: Runs independently of main.cpp
- **Non-interfering**: Uses same hardware without conflicts
- **Data isolation**: Separate files, no main.cpp modification needed

### 🐛 Troubleshooting

**Common Issues:**
- `INA228 not responding` → Check I2C connections
- `SD initialization failed` → Verify CS pin 8 connection
- `Invalid measurements` → Check power supply stability

**Memory Usage:** ~440KB RAM for full test data. Reduce sample count if needed.

### 📊 Sample Output
```
=== POWER CONSUMPTION SUMMARY ===
Highest Power Mode: Data Writing + Writing RAM Buffer to SD
  Average Power: 156.789 mW
Lowest Power Mode: Hardware Powered Off + MCU Idle  
  Average Power: 12.345 mW
Power Ratio: 12.7:1

=== RECOMMENDATIONS ===
- Use lowest power modes for battery conservation
- Minimize time in highest power modes  
- Consider batching operations for efficiency
```

---
**Status**: ✅ Ready for production testing
**Last Updated**: January 2025
**Platform**: Arduino/PlatformIO 