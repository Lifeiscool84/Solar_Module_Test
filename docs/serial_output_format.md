# Serial Output Format - Tabular Display

## Overview
The serial output has been updated to display data in a compact, tabular format similar to an Excel spreadsheet. This makes it much easier to track current and power changes over time.

## Technical Note
**Important**: The Arduino framework used by SparkFun Artemis Nano does not support `Serial.printf()` properly. The implementation uses traditional `Serial.print()` methods with manual formatting for proper alignment, as recommended by the [Adafruit INA228 documentation](https://learn.adafruit.com/adafruit-ina228-i2c-power-monitor/arduino).

## Format Description

### Header (displayed every 30 seconds)
```
=== SOLAR MONITORING DATA TABLE ===
Time     | Lux  |GPS|Sat| Solar (V/mA/mW/°C) | Battery (V/mA/mW/°C) | Load (V/mA/mW/°C)  |Stat
---------|------|---|---|--------------------|--------------------- |-------------------|----
```

### Data Rows (every second)
```
14:23:45 | 1234 |YES| 8|12.34/ 123.4/ 456.7/25.3|11.87/ -45.3/ -537.2/26.1|11.85/  78.9/ 934.5/24.8|CHRG
14:23:46 | 1235 |YES| 8|12.35/ 124.1/ 459.2/25.4|11.88/ -46.1/ -548.1/26.2|11.86/  79.2/ 937.8/24.9|CHRG
14:23:47 | 1236 |YES| 8|12.36/ 124.8/ 461.7/25.5|11.89/ -46.9/ -559.0/26.3|11.87/  80.1/ 941.2/25.0|CHRG
```

## Column Definitions

| Column | Width | Description | Example |
|--------|-------|-------------|---------|
| Time | 8 chars | HH:MM:SS format | `14:23:45` |
| Lux | 5 chars | Light sensor reading (right-aligned) | `1234` |
| GPS | 3 chars | GPS fix status | `YES`/` NO` |
| Sat | 2 chars | Satellite count (right-aligned) | ` 8` |
| Solar | ~20 chars | V/mA/mW/°C format | `12.34/ 123.4/ 456.7/25.3` |
| Battery | ~22 chars | V/mA/mW/°C format | `11.87/ -45.3/ -537.2/26.1` |
| Load | ~20 chars | V/mA/mW/°C format | `11.85/  78.9/ 934.5/24.8` |
| Stat | 4 chars | Battery status | `CHRG`/`DISC`/`IDLE` |

## Formatting Details

### Number Alignment
- **Light sensor**: Right-aligned in 5 characters
- **GPS satellites**: Right-aligned in 2 characters  
- **Current values**: Right-aligned with space padding
- **Power values**: Right-aligned with space padding for readability
- **Voltage/Temperature**: Fixed decimal places (2 for voltage, 1 for temp)

### Negative Value Handling
- **Battery current**: Negative values indicate charging
- **Battery power**: Negative values indicate energy flowing into battery
- **Proper spacing**: Negative signs are accounted for in alignment

## Battery Status Codes
- **CHRG**: Charging (current < -0.5 mA)
- **DISC**: Discharging (current > +0.5 mA)  
- **IDLE**: Idle (-0.5 ≤ current ≤ +0.5 mA)

## Power Sign Convention
- **Positive Power**: Battery discharging (energy flowing out)
- **Negative Power**: Battery charging (energy flowing in)
- **Formula**: P = V × I (preserving current sign)

## Additional Information

### Periodic Status (every 60 seconds)
```
STATUS: Reading #123 | Date: 2025-01-15 | GPS: 30.123456,-90.654321
```

### Power Verification (every 60 seconds)
```
POWER VERIFICATION: V*I vs Sensor
Solar: 456.7 vs 456.7 mW | Battery: -537.2 vs -537.2 mW | Load: 934.5 vs 934.5 mW
```

### Temperature Warnings
```
WARNING: High temperature detected: 65.2°C
WARNING: Low temperature detected: -12.3°C
Temp Range: 23.5°C to 27.8°C
```

## Benefits of New Format

1. **Compact**: One line per reading instead of 8+ lines
2. **Real-time tracking**: Easy to see value changes over time
3. **Excel-like**: Similar to watching CSV data populate
4. **Power monitoring**: Clear indication of charging/discharging
5. **Quick scanning**: Aligned columns for easy comparison
6. **Reduced clutter**: Less scrolling required
7. **Platform compatible**: Uses standard Arduino Serial.print() methods

## Example Session Output
```
SETUP COMPLETE - Entering main data collection loop
Data collection will run regardless of serial connection status

DATA FORMAT: Tabular display updates every second
Format: Time | Lux |GPS|Sat| V/mA/mW/°C for Solar|Battery|Load |Status
Battery Power: Negative=Charging, Positive=Discharging

=== SOLAR MONITORING DATA TABLE ===
Time     | Lux  |GPS|Sat| Solar (V/mA/mW/°C) | Battery (V/mA/mW/°C) | Load (V/mA/mW/°C)  |Stat
---------|------|---|---|--------------------|--------------------- |-------------------|----
14:23:45 | 1234 |YES| 8|12.34/ 123.4/ 456.7/25.3|11.87/ -45.3/ -537.2/26.1|11.85/  78.9/ 934.5/24.8|CHRG
14:23:46 | 1235 |YES| 8|12.35/ 124.1/ 459.2/25.4|11.88/ -46.1/ -548.1/26.2|11.86/  79.2/ 937.8/24.9|CHRG
14:23:47 | 1236 |YES| 8|12.36/ 124.8/ 461.7/25.5|11.89/ -46.9/ -559.0/26.3|11.87/  80.1/ 941.2/25.0|CHRG
STATUS: Reading #3 | Date: 2025-01-15 | GPS: 30.123456,-90.654321
14:23:48 | 1237 |YES| 8|12.37/ 125.5/ 464.2/25.6|11.90/ -47.7/ -570.0/26.4|11.88/  81.0/ 944.5/25.1|CHRG
```

## Troubleshooting

### Printf Issues
If you see format specifiers like `%5.0f` in the output instead of numbers, it means `printf()` is not working on your platform. This code uses the standard Arduino `Serial.print()` approach to avoid this issue.

### Alignment Issues  
The manual spacing ensures proper column alignment across different value ranges and positive/negative numbers. The formatting automatically adjusts spacing based on the magnitude of values. 