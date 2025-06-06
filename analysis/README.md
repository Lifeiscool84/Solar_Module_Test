# Final Power Analysis Report Generator

This Python script is the definitive tool for analyzing the power consumption data from the `sd_power_test_essentials.cpp` sketch. It processes the generated `TEST.CSV` file, incorporates the critical findings from the "busy-wait" vs. "I/O-wait" investigation, and produces a comprehensive, accurate report in both Markdown and PDF formats.

## How to Use

### 1. Prerequisites

First, ensure you have the required Python libraries installed. This script requires `pandas`, `matplotlib`, and `fpdf`.

```bash
pip install pandas matplotlib fpdf
```

### 2. Run the Analysis

Execute the script from the root of your project directory, providing the path to your `TEST.CSV` data file as a command-line argument.

```bash
python analysis/final_report_generator.py test/TEST.CSV
```

### 3. Review the Output

After the script runs, you will find two new files in the `analysis/` directory:

*   `FINAL_ANALYSIS_REPORT.md`: A detailed Markdown report with data tables, analysis, and conclusions.
*   `FINAL_ANALYSIS_REPORT.pdf`: A professionally formatted PDF version of the same report, suitable for sharing or archiving.

This tool consolidates all previous analyses into a single, authoritative report. 