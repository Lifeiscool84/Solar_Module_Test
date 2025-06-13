# GNSS Power Analysis Tool - Usage Guide

## How to Run the Analysis Script

The `gnss_power_analysis.py` script has been updated to automatically find and analyze your GNSS power data.

### Method 1: Auto-Detection (Recommended)
```bash
python analysis/gnss_power_analysis.py
```

The script automatically looks for `gnss_power_demo.csv` in:
1. Same directory as the script (`analysis/`)
2. Parent directory (project root)

### Method 2: Specify CSV File
```bash
python analysis/gnss_power_analysis.py path/to/your/file.csv
```

## What the Script Does

1. **Loads CSV Data**: Reads GNSS power tracking data with 3-sensor format
2. **Power Flow Analysis**: Analyzes Solar → Battery → Load power flow
3. **GNSS Correlation**: Correlates GNSS operation with power consumption
4. **Generates Visualizations**: Creates comprehensive charts and graphs
5. **Summary Report**: Produces detailed text analysis

## Outputs Generated

All outputs are saved to `analysis_output/`:

- `gnss_power_analysis.png` - Main analysis charts (power vs time, distributions, efficiency)
- `power_correlation_matrix.png` - Power sensor correlation heatmap
- `voltage_current_characteristics.png` - V-I characteristics for all sensors
- `gnss_power_analysis_report.txt` - Detailed text summary report

## CSV Format Supported

The script supports the new 3-sensor CSV format from `main.cpp`:
```
Timestamp_24H,System_Millis_ms,GNSS_Valid,Latitude_deg,Longitude_deg,Altitude_m,Satellites_Used,HDOP,Fix_Type,Solar_Voltage_V,Solar_Current_mA,Solar_Power_mW,Battery_Voltage_V,Battery_Current_mA,Battery_Power_mW,Load_Voltage_V,Load_Current_mA,Load_Power_mW,GNSS_Power_Est_mW,System_Efficiency_pct
```

## Example Analysis Results

Based on recent data:
- **Dataset**: 1718 measurements over 30 minutes
- **Battery Average**: 117.42 mW
- **Load Average**: 93.55 mW  
- **Battery-to-Load Efficiency**: 79.67%
- **GNSS Active Periods**: 318 measurements
- **GNSS Inactive Periods**: 1400 measurements

## Dependencies

Make sure you have the required Python packages:
```bash
pip install pandas matplotlib seaborn numpy
```

## Troubleshooting

If you get "No CSV file found" error:
1. Make sure `gnss_power_demo.csv` exists in project root or `analysis/` folder
2. Run the GNSS power demo first to generate data
3. Check file permissions

For pandas/matplotlib errors:
```bash
pip install --upgrade pandas matplotlib seaborn
``` 