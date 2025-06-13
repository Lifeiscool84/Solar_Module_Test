#!/usr/bin/env python3
"""
GNSS Power Tracking Analysis Tool
Designed for analyzing CSV data from the Solar Module Test GNSS Power Demo

USAGE:
    Method 1 (Auto-detect CSV): python analysis/gnss_power_analysis.py
    Method 2 (Specify file):    python analysis/gnss_power_analysis.py path/to/file.csv

The script automatically looks for gnss_power_demo.csv in:
1. Same directory as the script (analysis/)
2. Parent directory (project root)

Supports the new 3-sensor CSV format:
Solar_Voltage_V,Solar_Current_mA,Solar_Power_mW,Battery_Voltage_V,Battery_Current_mA,Battery_Power_mW,Load_Voltage_V,Load_Current_mA,Load_Power_mW

OUTPUTS:
- analysis_output/gnss_power_analysis.png (main charts)
- analysis_output/power_correlation_matrix.png
- analysis_output/voltage_current_characteristics.png
- analysis_output/gnss_power_analysis_report.txt

Author: Solar Module Test Project
Date: January 2025
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import os
import sys
from pathlib import Path
from datetime import datetime
import json

# Set visualization style
plt.style.use('seaborn-v0_8')
sns.set_palette("husl")

def load_gnss_power_data(csv_path):
    """Load and validate GNSS Power Demo CSV data"""
    print(f"Loading GNSS Power Demo data from: {csv_path}")
    
    if not os.path.exists(csv_path):
        raise FileNotFoundError(f"CSV file not found: {csv_path}")
    
    # Load CSV data
    df = pd.read_csv(csv_path)
    print(f"Loaded {len(df)} records")
    print(f"Columns: {list(df.columns)}")
    
    # Convert timestamp if present
    if 'System_Millis_ms' in df.columns:
        df['Timestamp_s'] = df['System_Millis_ms'] / 1000.0
        df['Relative_Time_s'] = df['Timestamp_s'] - df['Timestamp_s'].iloc[0]
    
    # Convert numeric columns
    numeric_columns = [col for col in df.columns if any(x in col for x in ['Voltage', 'Current', 'Power', 'Latitude', 'Longitude', 'Altitude', 'HDOP'])]
    for col in numeric_columns:
        df[col] = pd.to_numeric(df[col], errors='coerce')
    
    # Detect format and convert if needed
    if 'Solar_Power_mW' not in df.columns:
        print("INFO: Old format detected - converting to 3-sensor format")
        # Handle old format conversion
        # Try different column name variations for battery power
        battery_power_col = None
        for col in ['Batt_Power_mW', 'Batt_Power_HW_mW', 'Battery_Power_mW']:
            if col in df.columns:
                battery_power_col = col
                break
        
        if battery_power_col:
            # Map old battery data to new format
            df['Battery_Power_mW'] = df[battery_power_col]
            df['Battery_Voltage_V'] = df.get('Batt_Voltage_V', 0)
            df['Battery_Current_mA'] = df.get('Batt_Current_mA', 0)
        
        # Try different column name variations for load power
        load_power_col = None
        for col in ['Load_Power_mW', 'Load_Power_HW_mW']:
            if col in df.columns:
                load_power_col = col
                break
        
        if load_power_col:
            # Load data already exists
            df['Load_Power_mW'] = df[load_power_col]
            df['Load_Voltage_V'] = df.get('Load_Voltage_V', 0)
            df['Load_Current_mA'] = df.get('Load_Current_mA', 0)
        else:
            # Create load data as copy of battery (for legacy compatibility)
            df['Load_Power_mW'] = df.get('Battery_Power_mW', 0)
            df['Load_Voltage_V'] = df.get('Battery_Voltage_V', 0)
            df['Load_Current_mA'] = df.get('Battery_Current_mA', 0)
        
        # Add placeholder solar data
        df['Solar_Power_mW'] = 0.0
        df['Solar_Voltage_V'] = 0.0
        df['Solar_Current_mA'] = 0.0
        
        print("INFO: Format conversion complete")
    
    # Remove invalid records
    initial_count = len(df)
    power_columns = ['Solar_Power_mW', 'Battery_Power_mW', 'Load_Power_mW']
    df.dropna(subset=[col for col in power_columns if col in df.columns], inplace=True)
    final_count = len(df)
    
    if initial_count != final_count:
        print(f"Removed {initial_count - final_count} invalid records")
    
    print(f"Final dataset: {final_count} valid records")
    return df

def analyze_power_flow(df):
    """Analyze power flow between Solar -> Battery -> Load"""
    print("\n=== Power Flow Analysis ===")
    
    analysis = {}
    
    # Overall statistics
    analysis['solar'] = {
        'avg_power_mW': df['Solar_Power_mW'].mean(),
        'max_power_mW': df['Solar_Power_mW'].max(),
        'min_power_mW': df['Solar_Power_mW'].min(),
        'total_energy_mWh': df['Solar_Power_mW'].sum() / 3600.0,  # Assuming 1Hz sampling
    }
    
    analysis['battery'] = {
        'avg_power_mW': df['Battery_Power_mW'].mean(),
        'max_power_mW': df['Battery_Power_mW'].max(),
        'min_power_mW': df['Battery_Power_mW'].min(),
        'total_energy_mWh': df['Battery_Power_mW'].sum() / 3600.0,
    }
    
    analysis['load'] = {
        'avg_power_mW': df['Load_Power_mW'].mean(),
        'max_power_mW': df['Load_Power_mW'].max(),
        'min_power_mW': df['Load_Power_mW'].min(),
        'total_energy_mWh': df['Load_Power_mW'].sum() / 3600.0,
    }
    
    # Calculate system efficiency
    solar_total = df['Solar_Power_mW'].sum()
    load_total = df['Load_Power_mW'].sum()
    
    if solar_total > 0:
        analysis['system_efficiency_pct'] = (load_total / solar_total) * 100.0
    else:
        analysis['system_efficiency_pct'] = 0.0
    
    # Power correlations
    analysis['correlations'] = {
        'solar_vs_battery': df['Solar_Power_mW'].corr(df['Battery_Power_mW']),
        'battery_vs_load': df['Battery_Power_mW'].corr(df['Load_Power_mW']),
        'solar_vs_load': df['Solar_Power_mW'].corr(df['Load_Power_mW'])
    }
    
    return analysis

def analyze_gnss_power_correlation(df):
    """Analyze correlation between GNSS operation and power consumption"""
    print("\n=== GNSS Power Correlation Analysis ===")
    
    analysis = {}
    
    # Check if GNSS data is available
    if 'GNSS_Valid' not in df.columns:
        print("WARNING: No GNSS validity data found")
        return {}
    
    # Split data by GNSS status
    gnss_active = df[df['GNSS_Valid'] == True]
    gnss_inactive = df[df['GNSS_Valid'] == False]
    
    if len(gnss_active) == 0 or len(gnss_inactive) == 0:
        print("WARNING: Insufficient GNSS state variation for analysis")
        return {}
    
    analysis['gnss_active'] = {
        'count': len(gnss_active),
        'avg_load_power_mW': gnss_active['Load_Power_mW'].mean(),
        'avg_battery_power_mW': gnss_active['Battery_Power_mW'].mean(),
        'avg_satellites': gnss_active['Satellites_Used'].mean() if 'Satellites_Used' in df.columns else 0
    }
    
    analysis['gnss_inactive'] = {
        'count': len(gnss_inactive),
        'avg_load_power_mW': gnss_inactive['Load_Power_mW'].mean(),
        'avg_battery_power_mW': gnss_inactive['Battery_Power_mW'].mean(),
        'avg_satellites': 0
    }
    
    # Calculate GNSS power consumption estimate
    gnss_power_estimate = analysis['gnss_active']['avg_load_power_mW'] - analysis['gnss_inactive']['avg_load_power_mW']
    analysis['estimated_gnss_power_mW'] = max(0, gnss_power_estimate)
    
    return analysis

def generate_visualizations(df, output_dir):
    """Generate comprehensive visualizations for GNSS Power Demo data"""
    print(f"\n=== Generating Visualizations ===")
    
    os.makedirs(output_dir, exist_ok=True)
    
    # Figure 1: Power vs Time (3 sensors)
    plt.figure(figsize=(15, 10))
    
    plt.subplot(2, 2, 1)
    if 'Relative_Time_s' in df.columns:
        time_col = 'Relative_Time_s'
        time_data = df[time_col]
        time_label = 'Relative Time (s)'
    else:
        time_data = range(len(df))
        time_label = 'Sample Index'
    
    plt.plot(time_data, df['Solar_Power_mW'], label='Solar', linewidth=2, alpha=0.8)
    plt.plot(time_data, df['Battery_Power_mW'], label='Battery', linewidth=2, alpha=0.8)
    plt.plot(time_data, df['Load_Power_mW'], label='Load', linewidth=2, alpha=0.8)
    plt.xlabel(time_label)
    plt.ylabel('Power (mW)')
    plt.title('Power Consumption vs Time')
    plt.legend()
    plt.grid(True, alpha=0.3)
    
    # Figure 2: Power Distribution
    plt.subplot(2, 2, 2)
    power_data = [df['Solar_Power_mW'], df['Battery_Power_mW'], df['Load_Power_mW']]
    power_labels = ['Solar', 'Battery', 'Load']
    plt.boxplot(power_data, tick_labels=power_labels)
    plt.ylabel('Power (mW)')
    plt.title('Power Distribution by Sensor')
    plt.grid(True, alpha=0.3)
    
    # Figure 3: GNSS Position Plot (if available)
    plt.subplot(2, 2, 3)
    if 'Latitude_deg' in df.columns and 'Longitude_deg' in df.columns:
        valid_gps = df[(df['Latitude_deg'] != 0) & (df['Longitude_deg'] != 0)]
        if len(valid_gps) > 0:
            plt.scatter(valid_gps['Longitude_deg'], valid_gps['Latitude_deg'], 
                       c=valid_gps['Load_Power_mW'], cmap='viridis', alpha=0.7)
            plt.colorbar(label='Load Power (mW)')
            plt.xlabel('Longitude (deg)')
            plt.ylabel('Latitude (deg)')
            plt.title('GNSS Position vs Power Consumption')
        else:
            plt.text(0.5, 0.5, 'No valid GNSS data', ha='center', va='center', transform=plt.gca().transAxes)
    else:
        plt.text(0.5, 0.5, 'GNSS data not available', ha='center', va='center', transform=plt.gca().transAxes)
    
    # Figure 4: System Efficiency
    plt.subplot(2, 2, 4)
    if 'System_Efficiency_pct' in df.columns:
        plt.plot(time_data, df['System_Efficiency_pct'], linewidth=2, color='green')
        plt.xlabel(time_label)
        plt.ylabel('Efficiency (%)')
        plt.title('System Efficiency vs Time')
        plt.grid(True, alpha=0.3)
    else:
        # Calculate efficiency on the fly
        # For legacy data without solar, show load/battery efficiency
        if df['Solar_Power_mW'].sum() > 0:
            efficiency = (df['Load_Power_mW'] / df['Solar_Power_mW'].replace(0, np.nan)) * 100
            efficiency_title = 'Solar to Load Efficiency vs Time'
        else:
            efficiency = (df['Load_Power_mW'] / df['Battery_Power_mW'].replace(0, np.nan)) * 100
            efficiency_title = 'Battery to Load Efficiency vs Time'
        
        plt.plot(time_data, efficiency, linewidth=2, color='green')
        plt.xlabel(time_label)
        plt.ylabel('Efficiency (%)')
        plt.title(efficiency_title)
        plt.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'gnss_power_analysis.png'), dpi=300, bbox_inches='tight')
    plt.close()
    
    # Generate additional detailed plots
    generate_detailed_analysis_plots(df, output_dir)
    
    print(f"Visualizations saved to: {output_dir}")

def generate_detailed_analysis_plots(df, output_dir):
    """Generate additional detailed analysis plots"""
    
    # Power correlation matrix
    plt.figure(figsize=(10, 8))
    power_columns = ['Solar_Power_mW', 'Battery_Power_mW', 'Load_Power_mW']
    correlation_matrix = df[power_columns].corr()
    sns.heatmap(correlation_matrix, annot=True, cmap='coolwarm', center=0, 
                square=True, linewidths=0.5, cbar_kws={"shrink": .8})
    plt.title('Power Sensor Correlation Matrix')
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'power_correlation_matrix.png'), dpi=300, bbox_inches='tight')
    plt.close()
    
    # Voltage vs Current scatter plots
    fig, axes = plt.subplots(1, 3, figsize=(18, 6))
    
    sensors = [('Solar', 'Solar_Voltage_V', 'Solar_Current_mA'),
               ('Battery', 'Battery_Voltage_V', 'Battery_Current_mA'),
               ('Load', 'Load_Voltage_V', 'Load_Current_mA')]
    
    for i, (name, voltage_col, current_col) in enumerate(sensors):
        axes[i].scatter(df[voltage_col], df[current_col], alpha=0.6, s=20)
        axes[i].set_xlabel(f'{name} Voltage (V)')
        axes[i].set_ylabel(f'{name} Current (mA)')
        axes[i].set_title(f'{name} Sensor: V-I Characteristic')
        axes[i].grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'voltage_current_characteristics.png'), dpi=300, bbox_inches='tight')
    plt.close()

def generate_summary_report(df, power_analysis, gnss_analysis, output_dir):
    """Generate a comprehensive text summary report"""
    report_path = os.path.join(output_dir, 'gnss_power_analysis_report.txt')
    
    with open(report_path, 'w') as f:
        f.write("="*60 + "\n")
        f.write("GNSS POWER TRACKING ANALYSIS REPORT\n")
        f.write("="*60 + "\n")
        f.write(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write(f"Dataset: {len(df)} measurements\n\n")
        
        # Data Summary
        f.write("DATA SUMMARY\n")
        f.write("-"*20 + "\n")
        if 'Relative_Time_s' in df.columns:
            duration_s = df['Relative_Time_s'].max()
            f.write(f"Recording Duration: {duration_s:.1f} seconds ({duration_s/60:.1f} minutes)\n")
        f.write(f"Total Measurements: {len(df)}\n")
        f.write(f"Sampling Rate: ~1 Hz\n\n")
        
        # Power Analysis
        f.write("POWER FLOW ANALYSIS\n")
        f.write("-"*20 + "\n")
        for sensor, data in power_analysis.items():
            if sensor == 'correlations' or sensor == 'system_efficiency_pct':
                continue
            f.write(f"{sensor.title()} Sensor:\n")
            f.write(f"  Average Power: {data['avg_power_mW']:.2f} mW\n")
            f.write(f"  Peak Power: {data['max_power_mW']:.2f} mW\n")
            f.write(f"  Minimum Power: {data['min_power_mW']:.2f} mW\n")
            f.write(f"  Total Energy: {data['total_energy_mWh']:.4f} mWh\n\n")
        
        f.write(f"System Efficiency: {power_analysis.get('system_efficiency_pct', 0):.2f}%\n\n")
        
        # Power Correlations
        f.write("POWER CORRELATIONS\n")
        f.write("-"*20 + "\n")
        corr = power_analysis.get('correlations', {})
        f.write(f"Solar vs Battery: {corr.get('solar_vs_battery', 0):.3f}\n")
        f.write(f"Battery vs Load: {corr.get('battery_vs_load', 0):.3f}\n")
        f.write(f"Solar vs Load: {corr.get('solar_vs_load', 0):.3f}\n\n")
        
        # GNSS Analysis
        if gnss_analysis:
            f.write("GNSS POWER CORRELATION\n")
            f.write("-"*20 + "\n")
            f.write(f"GNSS Active Periods: {gnss_analysis.get('gnss_active', {}).get('count', 0)} measurements\n")
            f.write(f"GNSS Inactive Periods: {gnss_analysis.get('gnss_inactive', {}).get('count', 0)} measurements\n")
            f.write(f"Estimated GNSS Power: {gnss_analysis.get('estimated_gnss_power_mW', 0):.2f} mW\n\n")
        
        # System Performance
        f.write("SYSTEM PERFORMANCE METRICS\n")
        f.write("-"*30 + "\n")
        
        # Calculate additional metrics
        if 'Solar_Power_mW' in df.columns and df['Solar_Power_mW'].sum() > 0:
            solar_utilization = (df['Load_Power_mW'].sum() / df['Solar_Power_mW'].sum()) * 100
            f.write(f"Solar Power Utilization: {solar_utilization:.2f}%\n")
        
        if 'Battery_Power_mW' in df.columns:
            battery_efficiency = (df['Load_Power_mW'].sum() / df['Battery_Power_mW'].sum()) * 100 if df['Battery_Power_mW'].sum() > 0 else 0
            f.write(f"Battery to Load Efficiency: {battery_efficiency:.2f}%\n")
        
        f.write("\n" + "="*60 + "\n")
        f.write("END OF REPORT\n")
        f.write("="*60 + "\n")
    
    print(f"Summary report saved to: {report_path}")

def main():
    """Main analysis function"""
    # Get script directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Check for command line argument first, then default to gnss_power_demo.csv
    if len(sys.argv) == 2:
        csv_path = sys.argv[1]
        print(f"Using CSV file from command line: {csv_path}")
    else:
        # Look for gnss_power_demo.csv in the same directory as the script
        default_csv = os.path.join(script_dir, "gnss_power_demo.csv")
        if os.path.exists(default_csv):
            csv_path = default_csv
            print(f"Using default CSV file: {csv_path}")
        else:
            # Look for gnss_power_demo.csv in the parent directory (where main.cpp saves it)
            parent_csv = os.path.join(os.path.dirname(script_dir), "gnss_power_demo.csv")
            if os.path.exists(parent_csv):
                csv_path = parent_csv
                print(f"Using CSV file from parent directory: {csv_path}")
            else:
                print("ERROR: No CSV file found!")
                print(f"Looked for:")
                print(f"  - {default_csv}")
                print(f"  - {parent_csv}")
                print()
                print("Usage Options:")
                print("1. python analysis/gnss_power_analysis.py  (auto-finds gnss_power_demo.csv)")
                print("2. python analysis/gnss_power_analysis.py <csv_file_path>  (specify file)")
                print()
                print("Make sure gnss_power_demo.csv exists in the project root or analysis/ folder")
                sys.exit(1)
    
    output_dir = "analysis_output"
    
    try:
        # Load and analyze data
        df = load_gnss_power_data(csv_path)
        power_analysis = analyze_power_flow(df)
        gnss_analysis = analyze_gnss_power_correlation(df)
        
        # Generate outputs
        generate_visualizations(df, output_dir)
        generate_summary_report(df, power_analysis, gnss_analysis, output_dir)
        
        # Print summary to console
        print("\n" + "="*50)
        print("ANALYSIS COMPLETE")
        print("="*50)
        print(f"Dataset: {len(df)} measurements")
        print(f"Solar Average Power: {power_analysis['solar']['avg_power_mW']:.2f} mW")
        print(f"Battery Average Power: {power_analysis['battery']['avg_power_mW']:.2f} mW")
        print(f"Load Average Power: {power_analysis['load']['avg_power_mW']:.2f} mW")
        print(f"System Efficiency: {power_analysis.get('system_efficiency_pct', 0):.2f}%")
        
        if gnss_analysis and 'estimated_gnss_power_mW' in gnss_analysis:
            print(f"Estimated GNSS Power: {gnss_analysis['estimated_gnss_power_mW']:.2f} mW")
        
        print(f"\nOutputs saved to: {os.path.abspath(output_dir)}/")
        print("- gnss_power_analysis.png")
        print("- power_correlation_matrix.png")
        print("- voltage_current_characteristics.png") 
        print("- gnss_power_analysis_report.txt")
        
    except Exception as e:
        print(f"ERROR: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main() 