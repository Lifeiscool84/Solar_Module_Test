# RTC Integration - RV8803 Real-Time Clock

## Overview
Added SparkFun RV8803 real-time clock support to provide accurate timestamps for solar monitoring data logging.

## Features Added

### 1. **Time Zone Support**
- Default: Central Standard Time (CST, UTC-6)
- Supports Central Daylight Time (CDT, UTC-5) 
- Configurable via serial interface

### 2. **Serial Time Setting Interface**
When connected via serial, the system provides time setting options during startup:
- **'s'** - Set time to compiler time (automatic)
- **'t'** - Set custom time (manual input)
- **'z'** - Toggle between CST/CDT time zones
- **'c'** - Continue with current RTC time

### 3. **Battery-Backed Operation**
- RTC maintains time when system is powered off (requires coin cell battery)
- Automatically uses stored time if no serial interaction
- No time setting required after initial setup

### 4. **Enhanced Data Logging**
- **ISO 8601 Timestamps**: Standard format (YYYY-MM-DDTHH:MM:SS)
- **Dual Time Logging**: Both RTC timestamp and millis() for compatibility
- **Updated CSV Format**: 
  ```
  ISO_Timestamp,Timestamp_ms,Lux,Solar_V,Solar_mA,Solar_mW,Solar_Temp,...
  ```

## Usage Instructions

### Initial Setup
1. Upload the main program
2. Connect via serial monitor
3. When prompted, choose time setting option:
   - For quick setup: Press 's' to use compiler time
   - For precise time: Press 't' and enter date/time manually
   - For time zone: Press 'z' to toggle CST/CDT

### Normal Operation
- System automatically uses RTC time without serial connection
- Time persists through power cycles (with coin cell battery)
- Timestamps appear in both serial output and SD card logs

### Time Zone Management
- **CST (Central Standard Time)**: UTC-6 hours (winter)
- **CDT (Central Daylight Time)**: UTC-5 hours (summer)
- Toggle between zones using 'z' command during startup

## Hardware Requirements
- SparkFun RV8803 RTC breakout board
- CR1225 coin cell battery (for time keeping during power off)
- I2C connection to Arduino (same bus as other sensors)

## Testing
Use the provided test program (`test/rtc_test.cpp`) to verify:
- RTC initialization
- Time setting functionality
- Time zone configuration
- Various timestamp formats

## CSV Log Format Changes
**Before:**
```
Timestamp_ms,Lux,Solar_V,Solar_mA,...
3477,0.00,0.0637,-0.124,...
```

**After:**
```
ISO_Timestamp,Timestamp_ms,Lux,Solar_V,Solar_mA,...
2024-01-15T14:30:15,3477,0.00,0.0637,-0.124,...
```

## Troubleshooting
- **"RTC initialization failed"**: Check I2C wiring and power connections
- **"RTC appears to be reset"**: Install coin cell battery and set time
- **Time zone issues**: Use 'z' command to toggle between CST/CDT
- **Timestamp errors**: Verify RTC is updating with serial output

The RTC provides professional-grade timestamping for long-term solar monitoring studies and data analysis. 