# Quick Reference - Artemis Nano Fixes

## Issue 1: Serial RX Not Working (Hardware Fix)
**Problem**: Serial input completely non-functional  
**File**: `C:\\Users\\[username]\\.platformio\\packages\\framework-arduinoapollo3\\variants\\SFE_ARTEMIS_NANO\\variant.cpp`  
**Fix**: Comment out `UART Serial1(SERIAL1_TX, SERIAL1_RX);`  
**Commands**: `pio run --target clean` then rebuild

## Issue 2: Data Collection Doesn't Start Without Serial Monitor
**Problem**: Device hangs during startup when no serial monitor connected  
**Root Cause**: `showEnhancedRealTimeDisplay()` function had infinite `while(Serial)` loop  
**Fix**: Added multiple timeout mechanisms (5s, 30s, 60s)  
**Result**: Data collection starts within 10 seconds regardless of serial connection

## GNSS Time Synchronization (January 2025)
**Feature**: Automatic RTC sync with GPS time
**Frequency**: Every 2 hours when GPS fix available  
**Manual Command**: `g` in real-time display mode
**Benefits**: Accurate timestamps, compensates for RTC drift
**Timezone**: CST/CDT support with `z` command toggle

## Timestamp Format Update (January 2025)
**Change**: Separated timestamp into Date and Time columns
**Date Format**: YYYY-MM-DD  
**Time Format**: HH:MM:SS (24-hour military time)
**CSV Header**: `Date,Time,Lux,Latitude,Longitude,GPS_Fix,GPS_Sats,...`
**Benefits**: Better spreadsheet compatibility and data analysis

## GPS Integration Added
**Library**: SparkFun u-blox GNSS Arduino Library v2.2.25
**Data**: Latitude/Longitude in decimal degrees (6 decimal places)
**Validation**: Requires 4+ satellites and HDOP < 5.0
**CSV**: Added GPS_Fix, GPS_Sats, Latitude, Longitude columns

## Power Calculation Fix (January 2025)
**Problem**: Battery power didn't match V×I calculation
**Root Cause**: Current correction factor not applied to power reading
**Fix**: Applied `BATTERY_INA228_ANOMALY_FACTOR = 2.18453f` to power calculation
**Verification**: Added periodic power math verification in debug output

## Key Command Reference
### Real-Time Display Commands
- `+N` / `-N` - Adjust time by N seconds (e.g., +10, -5)
- `z` - Toggle timezone (CST ↔ CDT)
- `d` - Toggle debug mode
- `g` - Manual GNSS time sync
- `c` - Continue to main program

### Timeout Behaviors
- **No Serial Monitor**: Auto-continues in 5 seconds
- **User Inactive**: Auto-continues after 60 seconds of no input
- **Maximum Session**: Auto-continues after 30 seconds total
- **GNSS Time Sync**: Retries every 2 hours when GPS available

## Key Files Modified
- `src/main.cpp` - Main implementation
- `docs/ARTEMIS_SERIAL_RX_FIX.md` - Detailed documentation
- Hardware fix: Apollo3 framework variant.cpp file

## Key Timeout Behaviors
- **No Serial Monitor**: Auto-continue after 5 seconds
- **Serial Connected, No Input**: Auto-continue after 30 seconds  
- **User Active, Then Idle**: Auto-continue after 60 seconds
- **Manual Continue**: Press 'c' to continue immediately

## CSV Format (Current)
```
Date,Time,Lux,Latitude,Longitude,GPS_Fix,GPS_Sats,Solar_V,Solar_mA,Solar_mW,Solar_Temp,Battery_V,Battery_mA,Battery_mW,Battery_Temp,Load_V,Load_mA,Load_mW,Load_Temp
```

## Time Adjustment Commands
- `+N` - Add N seconds (e.g., +10)
- `-N` - Subtract N seconds (e.g., -5)  
- `z` - Toggle timezone (CST/CDT)
- `d` - Toggle debug mode
- `c` - Continue to data collection

## Build Status
- **Flash Usage**: 19.2% (189,184 bytes from 983,040 bytes)
- **RAM Usage**: 11.7% (45,904 bytes from 393,216 bytes)
- **Compilation**: ✅ Success with no errors

## CSV Header Format
```
ISO_Timestamp,Lux,Latitude,Longitude,GPS_Fix,GPS_Sats,Solar_V,Solar_mA,Solar_mW,Solar_Temp,Battery_V,Battery_mA,Battery_mW,Battery_Temp,Load_V,Load_mA,Load_mW,Load_Temp
```

## Critical Commands
- `pio run --target clean` - Clear build cache after variant.cpp fix
- `pio run` - Compile and build project

## Status Messages to Watch For
- "SETUP COMPLETE - Entering main data collection loop"
- "DATA COLLECTION ACTIVE - Reading #X at [timestamp]"  
- "SD CARD STATUS: ACTIVE - Logging to solarX.csv"

## Testing Serial Connection
**With Monitor**: Should see time adjustment interface with timeout
**Without Monitor**: Should start data collection within 10 seconds  
**Serial Disconnect**: Data collection continues, logged to SD card

## Problem
SparkFun RedBoard Artemis Nano - Serial input not working (Serial.available() always returns 0)

## Solution
**File to edit**: `C:\\Users\\[username]\\.platformio\\packages\\framework-arduinoapollo3\\variants\\SFE_ARTEMIS_NANO\\variant.cpp`

**Change**: Comment out this line:
```cpp
//UART Serial1(SERIAL1_TX, SERIAL1_RX);
```

## Steps
1. Backup `variant.cpp`
2. Comment out the Serial1 line
3. `pio run --target clean`
4. `pio run`
5. Test with minimal serial echo code

## Test Code
```cpp
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Type any character:");
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    Serial.print("Received: ");
    Serial.println(c);
  }
  delay(10);
}
```

## Status
✅ **VERIFIED WORKING** - January 2025

📋 **Full Documentation**: See `docs/ARTEMIS_SERIAL_RX_FIX.md` 