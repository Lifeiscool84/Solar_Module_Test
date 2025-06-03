# Battery Calibration Changes

## Overview
Applied measured calibration values for the battery INA228 (address 0x44) to improve current measurement accuracy.

## Changes Made

### 1. **Calibration Constants**
```cpp
// Battery-specific calibration values (measured with 4-wire method)
const float EFFECTIVE_SHUNT_OHMS_BATTERY = 0.0177186f; // From V_INA_shunt / I_multimeter
const float BATTERY_INA228_ANOMALY_FACTOR = 2.18453f; // Correction factor for battery readings
const float MAX_CURRENT_BATTERY = 5.0f; // Keep original max current design
```

**Explanation:**
- `EFFECTIVE_SHUNT_OHMS_BATTERY`: Measured actual shunt resistance (17.7186 mΩ vs 15 mΩ programmed)
- `BATTERY_INA228_ANOMALY_FACTOR`: Correction factor derived from measurements
- Used in battery INA228 initialization instead of generic `SHUNT_OHMS_PROGRAMMED`

### 2. **Battery Current Correction Logic**
```cpp
// Apply battery-specific correction factor
float current_raw_from_chip = readCurrent(INA_BATTERY_ADDR, battery_current_lsb, rawCurrent, shiftedCurrent);
float current_final_battery = current_raw_from_chip * BATTERY_INA228_ANOMALY_FACTOR;
data.battery_current = current_final_battery; // Use corrected current
```

**Explanation:**
- Raw current from INA228 is multiplied by anomaly factor (2.18453)
- This should bring readings closer to actual measured current (~43mA target)

### 3. **Verification Calculations**
```cpp
// Calculate current from shunt voltage using effective resistance for comparison
float currentFromVshunt = (shuntV / 1000.0f) / EFFECTIVE_SHUNT_OHMS_BATTERY * 1000.0f;
```

**Explanation:**
- Independent calculation: I = V_shunt / R_effective
- Should match corrected current reading for validation
- Both should show ~43mA when measuring actual 43mA load

### 4. **Debug Output**
Added comprehensive debug output every 5 seconds showing:
- Shunt voltage (mV)
- Raw current from chip (mA)
- Final corrected current (mA)
- Calculated current from V_shunt/R_eff (mA)
- Effective shunt resistance (mΩ)
- Anomaly correction factor

## Expected Results
- **Before correction**: Battery current readings were inaccurate
- **After correction**: Battery current should read ~43mA when actual load is 43mA
- **Verification**: Both corrected current and V_shunt/R_eff calculation should match

## Validation
Monitor the debug output to verify:
1. Corrected current matches expected load current
2. V_shunt/R_eff calculation matches corrected current
3. Temperature readings are now realistic (20-40°C range)

## References
- Based on 4-wire measurement method
- Accounts for PCB trace resistance and contact resistance
- Specific to battery INA228 at address 0x44 