#!/usr/bin/env python3
"""
Power Comparison Analysis Tool
Analyzes test.csv with dual power calculation methods
Compares hardware INA228 power register vs manual V×I calculation

Usage: Simply place this script in the same folder as test.csv and run:
    python power_comparison_analysis.py
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from pathlib import Path
import sys
import os

def analyze_power_comparison(csv_file="test.csv"):
    """
    Analyze the power comparison data from SD card test
    
    New CSV format with 11 columns:
    TestRunID,TestState,EntryTimestamp_ms,Batt_Voltage_V,Batt_Current_mA,
    Batt_Power_HW_mW,Batt_Power_Calc_mW,Load_Voltage_V,Load_Current_mA,
    Load_Power_HW_mW,Load_Power_Calc_mW
    """
    
    # Get the directory where this script is located
    script_dir = Path(__file__).parent
    csv_path = script_dir / csv_file
    
    # Check if file exists in the same directory as the script
    if not csv_path.exists():
        print(f"❌ Error: {csv_file} not found in {script_dir}!")
        print("Please copy test.csv to the analysis folder and try again.")
        print(f"Expected location: {csv_path}")
        return None
    
    try:
        # Read CSV data
        print(f"🔋 SD Card Power Analysis - Dual Calculation Comparison")
        print("=" * 55)
        print(f"📊 Loading data from {csv_path}...")
        df = pd.read_csv(csv_path)
        
        print(f"✅ Loaded {len(df)} measurements")
        print(f"📈 Test states: {', '.join(df['TestState'].unique())}")
        print(f"🔋 Test runs: {', '.join(map(str, df['TestRunID'].unique()))}")
        print()
        
        # Calculate power differences
        df['Batt_Power_Diff_mW'] = df['Batt_Power_HW_mW'] - df['Batt_Power_Calc_mW']
        df['Load_Power_Diff_mW'] = df['Load_Power_HW_mW'] - df['Load_Power_Calc_mW']
        
        # Calculate relative errors (%) - handle division by zero
        df['Batt_Power_Error_Pct'] = np.where(df['Batt_Power_HW_mW'] != 0, 
                                              (df['Batt_Power_Diff_mW'] / df['Batt_Power_HW_mW']) * 100, 0)
        df['Load_Power_Error_Pct'] = np.where(df['Load_Power_HW_mW'] != 0, 
                                              (df['Load_Power_Diff_mW'] / df['Load_Power_HW_mW']) * 100, 0)
        
        # Overall statistics
        print("=== POWER CALCULATION COMPARISON SUMMARY ===")
        print("\n📊 BATTERY SENSOR (0x44):")
        print(f"  Hardware Power:   {df['Batt_Power_HW_mW'].mean():.3f} ± {df['Batt_Power_HW_mW'].std():.3f} mW")
        print(f"  Calculated Power: {df['Batt_Power_Calc_mW'].mean():.3f} ± {df['Batt_Power_Calc_mW'].std():.3f} mW")
        print(f"  Mean Difference:  {df['Batt_Power_Diff_mW'].mean():.3f} ± {df['Batt_Power_Diff_mW'].std():.3f} mW")
        print(f"  Mean Error:       {df['Batt_Power_Error_Pct'].mean():.2f} ± {df['Batt_Power_Error_Pct'].std():.2f} %")
        
        print("\n📊 LOAD SENSOR (0x41):")
        print(f"  Hardware Power:   {df['Load_Power_HW_mW'].mean():.3f} ± {df['Load_Power_HW_mW'].std():.3f} mW")
        print(f"  Calculated Power: {df['Load_Power_Calc_mW'].mean():.3f} ± {df['Load_Power_Calc_mW'].std():.3f} mW")
        print(f"  Mean Difference:  {df['Load_Power_Diff_mW'].mean():.3f} ± {df['Load_Power_Diff_mW'].std():.3f} mW")
        print(f"  Mean Error:       {df['Load_Power_Error_Pct'].mean():.2f} ± {df['Load_Power_Error_Pct'].std():.2f} %")
        
        # State-by-state analysis
        print("\n=== POWER CONSUMPTION BY TEST STATE ===")
        for state in sorted(df['TestState'].unique()):
            state_data = df[df['TestState'] == state]
            print(f"\n🔹 {state}:")
            print(f"  Battery: {state_data['Batt_Power_HW_mW'].mean():.3f} ± {state_data['Batt_Power_HW_mW'].std():.3f} mW")
            print(f"  Load:    {state_data['Load_Power_HW_mW'].mean():.3f} ± {state_data['Load_Power_HW_mW'].std():.3f} mW")
            print(f"  Batt Error: {state_data['Batt_Power_Error_Pct'].mean():.2f} ± {state_data['Batt_Power_Error_Pct'].std():.2f} %")
            print(f"  Load Error: {state_data['Load_Power_Error_Pct'].mean():.2f} ± {state_data['Load_Power_Error_Pct'].std():.2f} %")
            print(f"  Samples: {len(state_data)}")
        
        # Sensor comparison
        print("\n=== BATTERY vs LOAD SENSOR COMPARISON ===")
        df['Voltage_Diff_V'] = df['Batt_Voltage_V'] - df['Load_Voltage_V']
        df['Current_Diff_mA'] = df['Batt_Current_mA'] - df['Load_Current_mA']
        df['Power_Diff_HW_mW'] = df['Batt_Power_HW_mW'] - df['Load_Power_HW_mW']
        
        print(f"  Voltage Difference:  {df['Voltage_Diff_V'].mean():.4f} ± {df['Voltage_Diff_V'].std():.4f} V")
        print(f"  Current Difference:  {df['Current_Diff_mA'].mean():.3f} ± {df['Current_Diff_mA'].std():.3f} mA")
        print(f"  Power Difference:    {df['Power_Diff_HW_mW'].mean():.3f} ± {df['Power_Diff_HW_mW'].std():.3f} mW")
        
        # Generate plots if matplotlib available
        try:
            # Try different style options
            try:
                plt.style.use('seaborn-v0_8')
            except:
                try:
                    plt.style.use('seaborn')
                except:
                    plt.style.use('default')
            
            fig, axes = plt.subplots(2, 2, figsize=(15, 10))
            fig.suptitle('Power Calculation Method Comparison', fontsize=16, fontweight='bold')
            
            # Plot 1: Hardware vs Calculated Power (Battery)
            axes[0,0].scatter(df['Batt_Power_HW_mW'], df['Batt_Power_Calc_mW'], alpha=0.6, c='blue', s=20)
            min_val = min(df['Batt_Power_HW_mW'].min(), df['Batt_Power_Calc_mW'].min())
            max_val = max(df['Batt_Power_HW_mW'].max(), df['Batt_Power_Calc_mW'].max())
            axes[0,0].plot([min_val, max_val], [min_val, max_val], 'r--', label='Perfect Match', linewidth=2)
            axes[0,0].set_xlabel('Hardware Power (mW)')
            axes[0,0].set_ylabel('Calculated Power (mW)')
            axes[0,0].set_title('Battery: HW vs Calc Power')
            axes[0,0].legend()
            axes[0,0].grid(True, alpha=0.3)
            
            # Plot 2: Hardware vs Calculated Power (Load)
            axes[0,1].scatter(df['Load_Power_HW_mW'], df['Load_Power_Calc_mW'], alpha=0.6, c='green', s=20)
            min_val = min(df['Load_Power_HW_mW'].min(), df['Load_Power_Calc_mW'].min())
            max_val = max(df['Load_Power_HW_mW'].max(), df['Load_Power_Calc_mW'].max())
            axes[0,1].plot([min_val, max_val], [min_val, max_val], 'r--', label='Perfect Match', linewidth=2)
            axes[0,1].set_xlabel('Hardware Power (mW)')
            axes[0,1].set_ylabel('Calculated Power (mW)')
            axes[0,1].set_title('Load: HW vs Calc Power')
            axes[0,1].legend()
            axes[0,1].grid(True, alpha=0.3)
            
            # Plot 3: Power Error Distribution
            axes[1,0].hist(df['Batt_Power_Error_Pct'], bins=20, alpha=0.7, label='Battery', color='blue', density=True)
            axes[1,0].hist(df['Load_Power_Error_Pct'], bins=20, alpha=0.7, label='Load', color='green', density=True)
            axes[1,0].set_xlabel('Power Calculation Error (%)')
            axes[1,0].set_ylabel('Density')
            axes[1,0].set_title('Power Calculation Error Distribution')
            axes[1,0].legend()
            axes[1,0].grid(True, alpha=0.3)
            
            # Plot 4: Battery vs Load Power Comparison
            axes[1,1].scatter(df['Batt_Power_HW_mW'], df['Load_Power_HW_mW'], alpha=0.6, c='purple', s=20)
            min_val = min(df['Batt_Power_HW_mW'].min(), df['Load_Power_HW_mW'].min())
            max_val = max(df['Batt_Power_HW_mW'].max(), df['Load_Power_HW_mW'].max())
            axes[1,1].plot([min_val, max_val], [min_val, max_val], 'r--', label='Perfect Match', linewidth=2)
            axes[1,1].set_xlabel('Battery Power (mW)')
            axes[1,1].set_ylabel('Load Power (mW)')
            axes[1,1].set_title('Battery vs Load Power')
            axes[1,1].legend()
            axes[1,1].grid(True, alpha=0.3)
            
            plt.tight_layout()
            
            # Save plots in the same directory as the script
            output_file = script_dir / f"power_comparison_analysis_{df['TestRunID'].iloc[0]}.png"
            plt.savefig(output_file, dpi=300, bbox_inches='tight')
            print(f"\n📊 Analysis plots saved to: {output_file}")
            
            # Only show plots if running interactively
            if hasattr(sys, 'ps1') or not sys.stdin.isatty():
                plt.show()
            
        except ImportError:
            print("\n📊 Matplotlib not available - skipping plots")
            print("   Install with: pip install matplotlib seaborn")
        except Exception as e:
            print(f"\n⚠️ Error generating plots: {e}")
        
        # Validation conclusions
        print("\n=== VALIDATION CONCLUSIONS ===")
        
        # Check if power calculations are reasonable
        max_batt_error = abs(df['Batt_Power_Error_Pct']).max()
        max_load_error = abs(df['Load_Power_Error_Pct']).max()
        
        print(f"📈 Maximum calculation errors:")
        print(f"   Battery: {max_batt_error:.2f}%")
        print(f"   Load: {max_load_error:.2f}%")
        
        if max_batt_error < 5 and max_load_error < 5:
            print("✅ Power calculations are EXCELLENT - errors < 5%")
        elif max_batt_error < 10 and max_load_error < 10:
            print("✅ Power calculations are GOOD - errors < 10%")
        else:
            print("⚠️ Power calculations may need review - high errors detected")
        
        # Check sensor consistency
        voltage_consistency = abs(df['Voltage_Diff_V'].mean()) < 0.1
        current_consistency = abs(df['Current_Diff_mA'].mean()) < 1.0
        
        if voltage_consistency and current_consistency:
            print("✅ Dual sensors are CONSISTENT")
        else:
            print("⚠️ Dual sensors show differences - check hardware setup")
        
        # Save summary report
        summary_file = script_dir / f"power_analysis_summary_{df['TestRunID'].iloc[0]}.txt"
        with open(summary_file, 'w') as f:
            f.write("SD Card Power Analysis Summary\n")
            f.write("=" * 30 + "\n\n")
            f.write(f"Data file: {csv_file}\n")
            f.write(f"Test Run ID: {df['TestRunID'].iloc[0]}\n")
            f.write(f"Total measurements: {len(df)}\n")
            f.write(f"Test states: {', '.join(df['TestState'].unique())}\n\n")
            
            f.write("Power Calculation Validation:\n")
            f.write(f"Battery max error: {max_batt_error:.2f}%\n")
            f.write(f"Load max error: {max_load_error:.2f}%\n\n")
            
            f.write("Average Power by State:\n")
            for state in sorted(df['TestState'].unique()):
                state_data = df[df['TestState'] == state]
                f.write(f"{state}:\n")
                f.write(f"  Battery: {state_data['Batt_Power_HW_mW'].mean():.3f} mW\n")
                f.write(f"  Load: {state_data['Load_Power_HW_mW'].mean():.3f} mW\n")
        
        print(f"\n📄 Summary report saved to: {summary_file}")
        
        return df
        
    except Exception as e:
        print(f"❌ Error analyzing data: {e}")
        import traceback
        traceback.print_exc()
        return None

def main():
    """Main function - automatically detects and processes test.csv"""
    
    print("🔋 SD Card Power Analysis - Automatic Processing")
    print("=" * 50)
    
    # Check if test.csv exists in the same directory
    script_dir = Path(__file__).parent
    csv_file = script_dir / "test.csv"
    
    if csv_file.exists():
        print(f"✅ Found test.csv in {script_dir}")
        print("🚀 Starting automatic analysis...\n")
        
        df = analyze_power_comparison("test.csv")
        
        if df is not None:
            print("\n" + "=" * 50)
            print("✅ Analysis completed successfully!")
            print("\n📋 Generated files:")
            print(f"  📊 Plots: power_comparison_analysis_{df['TestRunID'].iloc[0]}.png")
            print(f"  📄 Summary: power_analysis_summary_{df['TestRunID'].iloc[0]}.txt")
            print("\n💡 Key takeaways:")
            print("  - Compare hardware vs calculated power methods")
            print("  - Check error percentages for sensor validation")
            print("  - Monitor battery vs load sensor differences")
            print("  - Use this data to validate INA228 configuration")
        else:
            print("❌ Analysis failed - check the data file format")
    else:
        print(f"❌ test.csv not found in {script_dir}")
        print("\n📋 Instructions:")
        print("1. Copy test.csv from your SD card to this analysis folder")
        print("2. Run this script again: python power_comparison_analysis.py")
        print(f"3. Expected location: {csv_file}")
        return False
    
    return True

if __name__ == "__main__":
    main() 