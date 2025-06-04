# SD Card Power Consumption Testing System

## Overview

The SD Card Power Consumption Testing System is a comprehensive tool designed to measure and analyze power consumption across all operational modes of SD card usage and system statuses. This system leverages the existing INA228 power measurement infrastructure from `main.cpp` to provide detailed insights into SD card power characteristics.

## Features

### 📊 Comprehensive Testing Coverage
- **11 SD Card Operational Modes**: From initialization to power-off states
- **9 System Status States**: Complete system lifecycle coverage
- **Statistical Analysis**: Multiple iterations for reliable data
- **Real-time Monitoring**: 20kHz sampling rate for precision

### 🔋 Power Analysis Capabilities
- **Multi-sensor Monitoring**: Solar, battery, and load power measurements
- **Peak Detection**: Identify power consumption spikes
- **Correlation Analysis**: Understand relationships between modes and statuses
- **Energy Calculations**: Total energy consumption per operation

### 📈 Advanced Reporting
- **CSV Data Export**: Machine-readable data for further analysis
- **Text Reports**: Human-readable detailed analysis
- **Summary Reports**: Executive-level insights and recommendations
- **Real-time Progress**: Live updates during testing

## Hardware Requirements

### Primary Components
- **SparkFun RedBoard Artemis Nano** (or compatible Arduino board)
- **3x INA228 Power Monitors** at I2C addresses:
  - `0x40` (Solar monitoring)
  - `0x44` (Battery monitoring) 
  - `0x41` (Load monitoring)
- **SD Card Module** with CS pin connected to pin 8
- **0.015Ω Shunt Resistors** for current sensing

### Wiring Configuration
```
INA228 Solar  -> I2C Address 0x40
INA228 Battery-> I2C Address 0x44  
INA228 Load   -> I2C Address 0x41
SD Card CS    -> Digital Pin 8
I2C SDA       -> SDA Pin
I2C SCL       -> SCL Pin
```

## Software Architecture

### Core Components

#### 1. Power Measurement Engine
- **Rapid Sampling**: 50μs intervals for high-resolution data
- **Multi-sensor Coordination**: Simultaneous readings from all INA228 sensors
- **Data Validation**: Automatic filtering of invalid measurements
- **Timing Precision**: Microsecond-level timestamp accuracy

#### 2. SD Card Mode Testing
Systematic testing of all SD operational modes:

| Mode | Description | Test Method |
|------|-------------|-------------|
| **Initialization** | SD card startup sequence | Monitor during `SD.begin()` |
| **File Open/Close** | File system operations | Alternating open/close cycles |
| **Data Writing** | Continuous write operations | Large data block writes |
| **Data Reading** | Continuous read operations | Sequential file reading |
| **File Flushing** | Cache synchronization | Force flush operations |
| **Metadata Interaction** | Directory and file info | File listing and info queries |
| **Active Idle** | Ready state, no operations | Monitor idle consumption |
| **Standby Idle** | Low-activity standby | Minimal SD activity |
| **Software Low Power** | Software-controlled low power | Reduced clock/activity |
| **Hardware Powered Off** | Physical power disconnect | No SD power consumption |
| **Rapid Read/Write** | High-speed data transfer | Burst read/write operations |

#### 3. System Status Testing
Comprehensive system state monitoring:

| Status | Description | Monitoring Focus |
|--------|-------------|------------------|
| **System Initializing** | Boot and setup phase | Startup power patterns |
| **Sensor Data Acquisition** | Reading sensor data | Sensor power overhead |
| **Preparing for SD Operation** | Pre-write preparation | Setup power costs |
| **Writing RAM Buffer to SD** | Data transfer phase | Transfer power spikes |
| **Finalizing SD Operations** | Post-write cleanup | Cleanup power usage |
| **Transitioning to Low Power** | Power state changes | Transition overhead |
| **MCU Idle** | Microcontroller idle | Base system consumption |
| **SD Read Operation** | Active data reading | Read-specific patterns |
| **Error Recovery** | Handling failures | Recovery power costs |

#### 4. Statistical Analysis Engine
- **Standard Deviation**: Measurement variability analysis
- **Peak Detection**: Identify consumption spikes > 10mW
- **Correlation Analysis**: Mode-status interaction effects
- **Energy Calculations**: Total energy consumption per operation
- **Trend Analysis**: Power consumption patterns over time

## Usage Guide

### Quick Start

1. **Hardware Setup**
   ```cpp
   // Ensure all INA228 sensors are connected at correct addresses
   // SD card connected to pin 8
   // I2C connections established
   ```

2. **Upload Firmware**
   ```bash
   # Using PlatformIO
   pio run --target upload
   
   # Using Arduino IDE
   # Select SparkFun RedBoard Artemis Nano
   # Upload sd_power_test.cpp
   ```

3. **Basic Testing**
   ```
   Serial Monitor Commands:
   'h' - Show help menu
   'a' - Run comprehensive test
   'v' - Validate test environment
   'r' - Generate detailed report
   's' - Generate summary report
   'c' - Clear test data
   ```

### Comprehensive Testing Procedure

#### Step 1: Environment Validation
```
> v
Validating test environment...
  Solar INA228: OK
  Battery INA228: OK  
  Load INA228: OK
  SD Card: OK
Test environment validation passed
```

#### Step 2: Run Full Test Suite
```
> a
=================================================
Starting Comprehensive SD Card Power Test
=================================================
Testing 11 SD modes
Testing 9 system statuses
Samples per test: 1000
Test iterations: 5

=== Test Iteration 1 of 5 ===
Testing: Initialization / System Initializing (Iteration 1)
Rapid sampling completed: 1000 samples in 52847 microseconds
...
```

#### Step 3: Generate Reports
```
> r  # Detailed technical report
> s  # Executive summary report
```

### Advanced Configuration

#### Modify Test Parameters
```cpp
// In sd_power_test_config.h
const uint16_t SAMPLES_PER_MODE_TEST = 2000;  // More samples
const uint16_t SAMPLE_INTERVAL_MICROSECONDS = 25;  // Higher rate
const uint16_t TEST_ITERATION_COUNT = 10;  // More iterations
```

#### Custom Test Modes
```cpp
// Add custom SD operations in test functions
bool testSDCustomMode() {
  // Your custom SD card operations here
  // Power sampling will capture consumption
  return true;
}
```

## Data Output Format

### CSV Data Structure
```csv
timestamp_us,sd_mode,system_status,solar_voltage,solar_current,solar_power,
battery_voltage,battery_current,battery_power,load_voltage,load_current,
load_power,total_power,sample_number,iteration,valid
```

### Report Format
```
=== SD CARD MODE ANALYSIS ===
Mode: Data Writing
  System Initializing: Avg=125.456mW, Peak=201.234mW, Min=98.123mW
  Writing RAM Buffer to SD: Avg=156.789mW, Peak=245.678mW, Min=112.456mW
  
=== POWER CONSUMPTION SUMMARY ===  
Highest Power Mode: Data Writing + Writing RAM Buffer to SD
  Average Power: 156.789 mW
Lowest Power Mode: Hardware Powered Off + MCU Idle  
  Average Power: 12.345 mW
Power Range: 12.345 - 156.789 mW
Power Ratio: 12.7:1
```

## Troubleshooting

### Common Issues

#### INA228 Not Responding
```
ERROR: Solar INA228 not responding
```
**Solution**: Check I2C connections and address configuration

#### SD Card Initialization Failed  
```
ERROR: SD card initialization failed
```
**Solution**: Verify SD card connection, format, and pin configuration

#### Invalid Measurements
```
WARNING: High number of invalid measurements detected
```
**Solution**: Check power supply stability and sensor connections

### Memory Considerations

The system uses significant RAM for data storage:
- **Test Data Array**: ~440KB for full test results
- **Statistics Arrays**: ~12KB for calculated statistics  
- **Buffers**: ~2KB for I/O operations

For memory-constrained systems, reduce `SAMPLES_PER_TEST` or `TEST_ITERATIONS`.

### Performance Optimization

#### High-Speed Sampling
- Uses `delayMicroseconds()` for precise timing
- I2C at 400kHz for faster sensor reads
- Optimized register access patterns
- Yield control to prevent system lockup

#### Data Processing
- Statistics calculated incrementally
- Memory-efficient data structures
- Optional memory optimization compilation flags

## Integration with Main System

### Shared Resources
The power testing system shares hardware configuration with `main.cpp`:
- **INA228 Addresses**: Identical to production system
- **Calibration Values**: Same shunt resistor specifications  
- **SD Card Configuration**: Same CS pin and settings
- **I2C Parameters**: Consistent communication settings

### Non-Interference Design
- **Standalone Operation**: No modification of `main.cpp` required
- **Independent Data Files**: Separate file naming to avoid conflicts
- **Isolated Testing**: Can run independently or alongside main system
- **Reset Capability**: Full data reset without affecting main system

## Scientific Methodology

### Statistical Validity
- **Multiple Iterations**: 5 default iterations per test for statistical significance
- **Large Sample Sizes**: 1000 samples per test for resolution
- **Validation Criteria**: Automatic filtering of invalid measurements
- **Standard Deviation**: Quantifies measurement variability

### Measurement Accuracy
- **20kHz Sampling Rate**: Captures fast power transients  
- **Microsecond Timestamps**: Precise temporal correlation
- **Multi-sensor Approach**: Comprehensive power analysis
- **Calibrated Sensors**: Uses same calibration as production system

### Reproducibility
- **Documented Configuration**: All parameters clearly defined
- **Standardized Procedures**: Consistent test execution
- **Environment Validation**: Pre-test system verification
- **Data Export**: Raw data available for independent analysis

## Future Enhancements

### Planned Features
- **Real-time Graphing**: Live power consumption visualization
- **Automated Optimization**: AI-driven power optimization suggestions
- **Network Logging**: Remote data collection and analysis
- **Custom Test Sequences**: User-defined test patterns

### Research Applications
- **Battery Life Modeling**: Long-term power consumption prediction
- **Thermal Analysis**: Power consumption vs. temperature correlation
- **Wear Leveling Impact**: SD card aging effects on power consumption
- **Frequency Analysis**: Power spectral density analysis

## Conclusion

The SD Card Power Consumption Testing System provides comprehensive insights into SD card power characteristics across all operational modes and system states. By leveraging existing infrastructure and providing detailed analysis capabilities, it enables data-driven optimization of power consumption in solar-powered data logging applications.

For technical support or questions, refer to the source code documentation or contact the development team. 