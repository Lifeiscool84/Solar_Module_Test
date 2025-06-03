# Battery Monitoring System Improvements

## Overview
The battery monitoring system has been updated to use a more intuitive and accurate measurement approach based on direct calculations from shunt voltage and effective resistance.

## Changes Made

### 1. Direct Current Calculation
**Previous approach:** Used INA228 chip calibration with correction factors
- Applied `BATTERY_INA228_ANOMALY_FACTOR = 2.18453f` to compensate for parasitic resistance
- Complex correction logic with multiple calibration steps

**New approach:** Direct calculation from measured values
- Current (mA) = Shunt Voltage (mV) / Effective Resistance (ohms)
- Uses `EFFECTIVE_SHUNT_OHMS_BATTERY = 0.0177186f` (measured with 4-wire method)
- Eliminates need for correction factors

### 2. Proper Power Sign Convention
**Previous approach:** Power was always positive regardless of current direction

**New approach:** Power preserves sign from current
- Positive current (discharging) → Positive power
- Negative current (charging) → Negative power
- Formula: P = V × I (preserving sign)

### 3. Updated Battery Status Logic
**Previous logic:**
```cpp
if (data.battery_current > 0) {
    Serial.println(F("Battery Status: CHARGING"));
} else if (data.battery_current < 0) {
    Serial.println(F("Battery Status: DISCHARGING"));
}
```

**New logic:**
```cpp
if (data.battery_current > 0.5) {
    Serial.println(F("Battery Status: DISCHARGING"));
} else if (data.battery_current < -0.5) {
    Serial.println(F("Battery Status: CHARGING"));
}
```

### 4. Enhanced Debug Output
The debug output now shows:
- Effective resistance value
- Direct calculation vs chip calculation comparison
- Clear battery status indication
- Simplified debugging information

## Technical Details

### Effective Resistance Measurement
- Measured using 4-wire method: `EFFECTIVE_SHUNT_OHMS_BATTERY = 0.0177186f`
- Accounts for parasitic resistance in PCB traces and connections
- More accurate than nominal 0.015Ω shunt resistor value

### Current Calculation
```cpp
// Direct calculation from shunt voltage and effective resistance
data.battery_current = shuntV / EFFECTIVE_SHUNT_OHMS_BATTERY;
```

### Power Calculation
```cpp
// Power preserving sign convention
data.battery_power = data.battery_voltage * data.battery_current;
```

## Benefits

1. **Simplicity**: Eliminates complex correction factor calculations
2. **Accuracy**: Uses measured effective resistance for better precision
3. **Intuitive**: Sign convention matches physical energy flow
4. **Maintainability**: Clear, straightforward calculation approach
5. **Transparency**: Easy to understand and verify measurements

## File Changes
- `src/main.cpp`: Updated battery measurement logic in `readAllSensors()`
- Removed `BATTERY_INA228_ANOMALY_FACTOR` constant
- Updated battery status detection logic
- Enhanced debug output for better monitoring

## Verification
The system continues to show chip-calculated values alongside direct calculations for comparison and validation purposes during the debug output cycle. 