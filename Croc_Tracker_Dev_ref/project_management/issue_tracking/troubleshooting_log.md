# Troubleshooting Log for Iridium Nano BLE Project

*Last Updated: Moment of generation*

## 1. Introduction

This document logs the troubleshooting steps, errors encountered, attempts made, and solutions found during the development of the Iridium Nano BLE project. The project is developed for an Artemis-based microcontroller, likely utilizing Mbed OS (an RTOS), and built using PlatformIO.

## 2. SPI and SD Card Functionality (From "Reverting SPI.h Changes for SD..." Chat)

### Problem Description
The SD card functionality (reading/writing) ceased to work after modifications were made to the SPI (Serial Peripheral Interface) library, which were initially intended to support Bluetooth functionalities.

### Observed Errors
- SD card read/write operations failed (specific error messages not detailed in summary, but implied).
- The `SPI.h` and `SPI.cpp` files had been significantly altered from their original MbedSPI versions.

### Cause
The original MbedSPI implementation, which is hardware-specific for the Artemis/Mbed environment, was replaced with a simplified or generic SPI version that was not fully compatible with the hardware's requirements for SD card communication.

### Attempts Made
- Analysis of the modified `SPI.h` and `SPI.cpp` files.
- Comparison with the original MbedSPI library versions.

### Solution/Resolution
- The `SPI.h` and `SPI.cpp` files were reverted to their original MbedSPI versions.
- A new function was introduced, likely within the SPI context or SD card driver, to handle bit order correctly, as this can be a critical factor in SPI communication.

### Key Learnings
- Modifying low-level hardware communication libraries like SPI requires a thorough understanding of the target hardware (Artemis) and the underlying OS/framework (Mbed OS).
- Generic library replacements often miss hardware-specific optimizations or functionalities crucial for peripherals like SD cards.
- Always version control critical library files before modification. Reverting to a known-good state is much easier with proper versioning.

## 3. Task Management Tooling Issues

### Problem Description
Initial attempts to use Task Master tools (`mcp_taskmaster_get_tasks`, `mcp_taskmaster_initialize_project`) for project and task management failed.

### Observed Errors
- **`mcp_taskmaster_get_tasks`**: Reported that `tasks.json` was not found.
- **`mcp_taskmaster_initialize_project`**: The tool execution failed. The specific error message for this failure was not detailed in the summary, but it prevented project initialization via the tool.

### Cause
- The `tasks.json` file, which is essential for Task Master operations, was not present in the expected location.
- The `initialize_project` tool might have failed due to incorrect project root path configuration, environment issues, or other unmet prerequisites for the tool.

### Attempts Made
- Executed `mcp_taskmaster_get_tasks` to list tasks.
- Executed `mcp_taskmaster_initialize_project` to set up the Task Master structure.

### Solution/Resolution
- Due to the immediate issues with the Task Master tools, a manual approach to task tracking was adopted.
- Project progress and task status were managed by directly editing the `docs/progress/task_progress.md` file.

### Key Learnings
- When automated project management tools present initial setup challenges, having a manual fallback (like a simple markdown file) can prevent delays and maintain project momentum.
- For tools like Task Master, ensure that all prerequisite files (e.g., `tasks.json`) are in place and that the tool is configured with the correct project paths and environment.

## 4. `src/main.cpp` Build and Lint Errors

### Problem Description
Persistent build failures occurred when compiling `src/main.cpp` using `pio run`. These were accompanied by a large number of linter errors, primarily indicating undefined identifiers.

### Observed Build Errors (Specific Fixes Attempted/Made)
- **`ISBD_ERROR_DEFAULT` undefined:** This constant was used in the code but lacked a definition.
    - *Status:* Unresolved during the logged interactions. Needs to be defined or included from the correct IridiumSBD library header.
- **`createNewDataFile` return type mismatch:** The function was defined as `void` but used in contexts expecting a `bool`.
    - *Attempt/Solution:* The function prototype and definition were changed to return `bool`.
- **`myGNSS.setAutoNAVPVT(true);` typo:** Incorrect function name used.
    - *Attempt/Solution:* Corrected to `myGNSS.setAutoNAVPVAT(true);`.
- **`validateFileOperation(dataFile.flush(), ...)` incorrect usage:** `dataFile.flush()` returns `void` and cannot be directly used as a boolean argument for `validateFileOperation`.
    - *Attempt/Solution:* `dataFile.flush()` was called directly in `collectSensorData`, and its success was assumed or handled implicitly.

### Observed Linter Errors (Widespread Undefined Identifiers)
A significant number of linter errors pointed to undefined identifiers, suggesting a core issue with the build environment's ability to recognize standard and library-specific types. These included:
- **Arduino Core & BLE Types:** `BLEService`, `BLEStringCharacteristic`, `BLERead`, `BLEWrite`, `BLENotify`, `BLEIndicate`, `File` (Arduino SD library's `File`), `String` (Arduino `String` class), `BLEDevice`, `BLE` (global BLE object).
- **External Library Types:** `IridiumSBD` (from IridiumSBD library), `SFE_UBLOX_GNSS` (from SparkFun u-blox GNSS library).
- **Arduino Core Functions/Objects:** `Wire`, `Wire1` (for I2C communication), `fabs` (math function), `millis` (timing function).
- **Cascading Syntax/Scoping Issues:** Many subsequent errors like "expected a ';'", "identifier 'X' is undefined", and "this declaration has no storage class or type specifier" were likely consequences of these initial type definition failures.

### Cause of Widespread Linter Errors
The primary cause for the multitude of "identifier undefined" errors is likely related to the PlatformIO project configuration (`platformio.ini`):
- **Incorrect Framework/Board Setup:** The Mbed OS framework for Artemis might not be correctly configured to expose Arduino-compatible APIs and types.
- **Missing Library Dependencies:** Necessary libraries (e.g., `ArduinoBLE`, SD library, IridiumSBD, SparkFun u-blox GNSS) might not be correctly listed in `lib_deps` in `platformio.ini`, or PlatformIO might be failing to locate them.
- **Incorrect Include Paths:** Even if libraries are present, the compiler might not be finding their header files.
- **PlatformIO Core or Toolchain Issues:** Less likely, but possible issues with the PlatformIO installation or the compiler toolchain.

### Errors in Other Files
- Build failures also indicated errors in `main_temp.cpp` and `main.full.cpp` (e.g., 'central' not declared, 'transmissionProgress' redeclared). These files were not the focus of active development during these sessions but contributed to the overall build failure.

### Attempts Made (General for Build/Lint Issues)
- Sequentially implementing features and running `pio run` after changes.
- Analyzing terminal output for error messages.
- Applying specific fixes for identified errors (as listed above).

### Solution/Resolution (for Widespread Linter Errors)
- *Status:* Largely unresolved during the logged interactions. The fundamental issue of undefined core Arduino and library types needs to be addressed by thoroughly reviewing and correcting the `platformio.ini` file, ensuring:
    1.  Correct `board` and `framework` (e.g., `framework = arduino` or `framework = mbed` with an Arduino core layer if applicable for Artemis).
    2.  All necessary libraries (`ArduinoBLE`, SD, IridiumSBD, SparkFun_u-blox_GNSS_Arduino_Library, etc.) are included in `lib_deps`.
    3.  PlatformIO's library manager can correctly download and link these libraries.
    4.  `#include` directives in `main.cpp` are correct and point to existing headers.

### Key Learnings for Build/Lint Issues
- **Environment is Key:** In embedded development, especially with complex setups like Artemis/Mbed/PlatformIO, the project configuration (`platformio.ini`) is critical. Most widespread "undefined identifier" errors trace back to this.
- **Isolate Errors:** Errors in unrelated or old files (like `main_temp.cpp`) can obscure issues in the active development file. Consider excluding them from the build temporarily if they are not relevant.
- **Systematic Dependency Check:** When core types are missing, systematically verify library inclusion, header paths, and framework compatibility.
- **Incremental Fixes:** Address one type of error at a time. Fixing the core type definitions often resolves many subsequent cascading errors.
- **Consult Documentation:** Refer to PlatformIO documentation and library examples for the specific board/framework to ensure correct setup.

## 5. General Development Context & Learnings

- **Development Environment:** Artemis-based microcontroller, Mbed OS (inferred), PlatformIO build system.
- **Process:** Iterative development with frequent builds and error analysis. This is standard but highlights the need for a stable build environment.
- **Importance of `platformio.ini`:** This file is central to resolving most of the persistent build and lint issues. Ensuring correct board, framework, library dependencies, and build flags is paramount.
- **Handling Pre-existing Errors:** The project appears to have started with, or accumulated, a number of errors in auxiliary files (`main_temp.cpp`, `main.full.cpp`). It's often beneficial to clean these up or exclude them from builds to focus on current development tasks.

## 6. Future Troubleshooting Steps (Recommendations)

1.  **Prioritize `platformio.ini` Review:**
    *   Verify `board` and `framework` settings for Artemis with Arduino compatibility (likely via Mbed's Arduino core).
    *   Ensure all required libraries (`IridiumSBD`, `SparkFun_u-blox_GNSS_Arduino_Library`, `ArduinoBLE`, and any SD card library) are correctly specified in `lib_deps`.
    *   Clean and rebuild the project (`pio run -t clean`, then `pio run`) after `platformio.ini` changes.
2.  **Address `ISBD_ERROR_DEFAULT`:** Locate where this constant should be defined (likely in an `IridiumSBD` header or needs to be added manually if it's a custom extension) and ensure it's properly included.
3.  **Verify Standard Includes:** Double-check that standard Arduino headers like `Arduino.h`, `Wire.h`, `SPI.h`, `SD.h`, and `ArduinoBLE.h` are included in `main.cpp` and are being correctly resolved by the compiler.
4.  **Isolate `main_temp.cpp` and `main.full.cpp`:** If these files are not currently needed, temporarily remove them from the `src` directory or exclude them from the build via `platformio.ini`'s `src_filter` option to reduce build noise.
5.  **Incremental Compilation:** Comment out sections of `main.cpp` to identify precisely which lines or library usages trigger specific "undefined identifier" errors, which can help pinpoint missing includes or library issues.

## 6. Bluetooth Communication Issues (nRF Connect Log Analysis - 2025-05-11)

### Problem Description
Analysis of an nRF Connect log (dated 2025-05-11) from a smartphone client revealed several issues with how the Artemis device handles BLE communication, leading to slow responses, incomplete messages, and potentially missing initial messages.

### Observed Behavior & Issues from Log
1.  **`waitForAck()` Delays:**
    *   **Symptom:** Significant 2-second delays were observed between chunks of data sent for commands like `ST` (Status) and `L` (List Files). For example, the `ST` command output, which is chunked, had noticeable pauses between each part.
    *   **Cause:** The `waitForAck(BLEDevice central)` function waits for an application-level "ACK" string from the client. Standard BLE clients like nRF Connect do not send this custom ACK by default after receiving a notification or indication. The BLE protocol itself ensures reliable delivery at the link layer. Thus, `waitForAck()` was likely timing out consistently.
    *   **Impact:** Made multi-part responses very slow. Could also hide initial messages if `waitForAck()` was called before the client was fully ready or if subsequent messages were sent too quickly.

2.  **Incomplete Menu (`M`) and Help (`H`) Responses:**
    *   **Symptom:** The responses for the `M` (Menu) and `H` (Help) commands, which can be quite long, appeared truncated in the nRF Connect log.
    *   **Cause:** Unlike the `ST` command, the `getFormattedMenu()` and `getCommandHelp()` functions did not have manual chunking logic to split their long strings into MTU-compatible segments for `testCharacteristic.writeValue()`. The `ArduinoBLE` library might not robustly handle fragmentation for very long strings sent via a single `writeValue` call on notify/indicate characteristics. The MTU was negotiated to 242 bytes, meaning payloads should be around 239 bytes max (MTU - 3 bytes ATT overhead).
    *   **Impact:** Users received incomplete menu and help information.

3.  **Missing Initial Message for `SQ` (Signal Quality) Command:**
    *   **Symptom:** The log for the `SQ` command did not show the preliminary "Checking Iridium signal quality..." message that the code intends to send before the actual signal quality result.
    *   **Cause:** The result message might have been sent too quickly after the preliminary message, or the `waitForAck()` call immediately after the preliminary message (if one existed there, not explicitly confirmed for SQ in this log but pattern suggests it) could have caused issues.
    *   **Impact:** User does not see the initial feedback that the command is being processed.

4.  **`printf`-style Format Specifiers in Output (e.g., `%3.1f`):**
    *   **Symptom:** The nRF Connect log showed literal `printf`-style format specifiers (e.g., `%3.1f`) in the output for the `ST` command (HDOP, free space values).
    *   **Cause:** This indicates the firmware running on the device when the log was captured was an older version of `src/main.cpp` that used `snprintf` or similar with these formatters directly in the string, and these weren't being replaced by actual values as expected (or the `String` formatting was different).
    *   **Note:** The version of `src/main.cpp` reviewed alongside this log (as of 2025-05-11) uses `String(float_value, 1)` for formatting, which is correct.
    *   **Impact:** Garbled status information for the user in the older firmware version.

5.  **Initial Welcome/Status Messages Reliability:**
    *   **Symptom:** While the log shows the initial welcome/status messages were received after connection, the `waitForAck()` calls after them could have made their appearance unreliable or delayed.
    *   **Cause:** Similar to point 1, `waitForAck()` timeouts.
    *   **Impact:** Potentially delayed or inconsistent delivery of initial crucial information to the user.

6.  **`sendFile()` Command:**
    *   **Observation (Proactive Analysis):** The `sendFile()` function also uses `waitForAck()` after sending each 20-byte chunk of the file.
    *   **Potential Impact:** File transfers would be extremely slow due to the repeated 2-second timeouts.

### Planned Solutions for `src/main.cpp`
1.  **Remove `waitForAck()`:**
    *   Remove calls to `waitForAck(central)` after `testCharacteristic.writeValue()` in `handleClient()` for the initial welcome message and status.
    *   Remove `waitForAck(central)` from the loop in `listFiles(BLEDevice central)`.
    *   Remove `waitForAck(central)` from the loop in `sendFile(BLEDevice central, String fileName)`.
    *   Rely on the inherent reliability of BLE notifications/indications.

2.  **Implement Manual Chunking for Long Messages:**
    *   Modify `handleClient()` to manually chunk the output of `getFormattedMenu()` (for `M` command) and `getCommandHelp()` (for `H` command) using a loop and `substring()`, similar to how the `ST` command's response is handled. Each chunk should be ~CHARACTERISTIC_MAX_LENGTH or less (respecting MTU). A small `delay(DELAY_BETWEEN_PACKETS)` should be kept between chunks.

3.  **Ensure Visibility of `SQ` Preliminary Message:**
    *   In `handleCommand()`, for the `SQ` case, ensure a `delay(DELAY_BETWEEN_PACKETS);` is present *after* `testCharacteristic.writeValue("Checking Iridium signal quality...");` and *before* the actual signal quality is sent.

4.  **Firmware Versioning:**
    *   No immediate code change for the `%3.1f` issue as the current code seems correct. However, this highlights the importance of ensuring the device is always flashed with the latest, verified firmware. This incident will be noted as a reminder.

5.  **Consider `DELAY_BETWEEN_PACKETS`:**
    *   The existing `DELAY_BETWEEN_PACKETS` (50ms) is likely sufficient for most cases once `waitForAck` is removed. If issues with message overrun persist on slower BLE clients, this value could be slightly increased. For now, it will be kept at 50ms.

## 7. nRF Connect Log Display: Hex vs. Text (2025-05-11 Log Analysis)

### Observation
When sending commands like "ST" to the device, the nRF Connect log (specifically lines starting with "I" for "Indication received") shows the response payload as a series of hexadecimal values, for example:
`I 21:10:12.125 Indication received from ... value: (0x) 3D-3D-3D-3D-3D-20-43-4F-4D-50-52...`

However, subsequent lines in the log (often starting with "A" for "Application") correctly interpret and display this same data as human-readable text:
`A 21:10:12.125 "(0x) 3D-3D-3... "===== COMPREHENSIVE SYSTEM S..." received`

### Explanation
- **Hex is Raw Data:** The hexadecimal values (`3D-3D-3D...`) are the raw bytes of the text string being sent by the Artemis device. For example, `0x3D` is `=`, `0x20` is a space, `0x43` is `C`.
- **Firmware Sends Text:** The `src/main.cpp` code correctly uses `testCharacteristic.writeValue("some string")` to send human-readable text. It does not explicitly convert strings to hex for transmission.
- **nRF Connect's Logging Behavior:**
    - BLE diagnostic tools like nRF Connect often log incoming raw packet data in hexadecimal format. This is a common way to display arbitrary byte streams and is useful for low-level debugging.
    - The tool *also* attempts to interpret this data. The fact that other log lines (e.g., the "A ... received" lines) show the correct text string ("===== COMPREHENSIVE SYSTEM S...") confirms that nRF Connect *is* receiving and understanding the data as text.
    - The display of raw hex for "Indication received" lines is a logging choice/verbosity level within nRF Connect and does not indicate an error in the data being sent by the Artemis device.
- **Client-Side Display Settings:** Most BLE tools, including nRF Connect, allow users to select how characteristic values are displayed (e.g., as Hex, UTF-8 Text, Decimal). If the primary display for a characteristic in the nRF Connect UI shows hex, it can usually be changed. Sometimes, for "unknown" characteristics or if the "Parse known characteristics" option is disabled (as mentioned in Nordic DevZone posts like [this one](https://devzone.nordicsemi.com/f/nordic-q-a/22523/writing-hex-values-to-characteristics-using-nrf-connect/88597)), the default log or UI display might be hex.

### Was it Supposed to Happen This Way?
- **Yes, for the logging tool's raw data view.** It's standard for debug logs to show raw byte payloads.
- **Yes, the firmware is correctly sending text.**
- **Yes, the nRF Connect application correctly interprets the text.**

The overall communication of textual data is working as intended. The display of hex in certain log lines is part of the tool's detailed logging, not an indication that the device is sending incorrect data.

### Resolution/Action
- **No firmware code change is required** for this specific observation. The device is sending correct textual data.
- **User Understanding:** Recognize that the hex display in "Indication received" lines is a raw data view, and the interpreted text is available in other log lines or by changing display settings in the nRF Connect UI for the characteristic.

## 8. Arduino/PlatformIO Forward Declaration Issues

### Problem Description
The project exhibits numerous "identifier undefined" or "incomplete type" errors in the IDE (VSCode/Cursor), particularly for standard Arduino types and functions, even though the code may still compile successfully with `pio run`.

### Observed Errors
- **Undefined Identifiers:** Standard Arduino types (`String`, `File`) and objects (`Serial`, `Wire`) appear as undefined.
- **Incomplete Types:** Forward-declared classes like `BLEDevice` marked as "incomplete type" in function parameters.
- **Missing Standard Functions:** Common Arduino functions (`millis()`, `delay()`) flagged as undefined.
- **"Expected a ';'" Cascade Errors:** A sequence of "expected a ';'" errors following an initial undefined identifier error.

### Cause
These issues commonly occur due to:

1. **IntelliSense Configuration Gaps:** The IDE's IntelliSense (code analysis) engine cannot find the proper header paths or has incomplete knowledge of Arduino/framework-specific types.

2. **Header Inclusion Order:** Arduino header files need to be included in a specific order for proper type resolution.

3. **Missing Forward Declarations:** Some libraries expect pre-declared types that are normally provided by the Arduino core.

4. **Framework Version Mismatches:** The IDE might be using different header files than the actual compiler during `pio run`.

5. **Missing `c_cpp_properties.json` Configuration:** Incomplete include paths or configuration in the VSCode C/C++ extension settings.

### Attempts Made
- Analysis of linter errors from main.cpp
- Comparison of included header files against required dependencies

### Solution/Resolution
A systematic approach to resolve these issues involves:

1. **Update `c_cpp_properties.json`:**
   - Ensure it includes paths to both Arduino core headers and library headers
   - For PlatformIO projects, it should include paths like:
     ```json
     "includePath": [
         "${workspaceFolder}/**",
         "${env:HOME}/.platformio/packages/framework-arduino*/cores/arduino",
         "${env:HOME}/.platformio/packages/framework-arduino*/libraries/**",
         "${env:HOME}/.platformio/packages/framework-mbed*/cores/arduino",
         "${env:HOME}/.platformio/lib/**"
     ]
     ```

2. **Fix Header Inclusion Order:**
   - Arduino.h should typically be included first
   - Library-specific headers should follow
   - Example correct order:
     ```cpp
     #include <Arduino.h>
     #include <Wire.h>
     #include <SPI.h>
     #include <SD.h>
     #include <ArduinoBLE.h>
     // Project-specific headers
     ```

3. **Add Explicit Forward Declarations:**
   - For persistent "incomplete type" errors, add forward declarations before function prototypes:
     ```cpp
     // Forward declarations
     class BLEDevice;
     class IridiumSBD;
     
     // Function prototypes
     void handleClient(BLEDevice central);
     bool transmitViaIridium(const uint8_t* data, size_t dataSize);
     ```

4. **Regenerate IntelliSense Configuration:**
   - Use PlatformIO's "Rebuild IntelliSense Index" command
   - In VSCode: Ctrl+Shift+P > "PlatformIO: Rebuild IntelliSense Index"

5. **Double-Check Library Compatibility:**
   - Ensure libraries are compatible with the chosen board and framework
   - Check for version mismatches or conflicting dependencies

### Key Learnings
- **Separation of IDE Analysis vs. Actual Compilation:** IDE linter errors don't always mean compilation will fail. PlatformIO's compilation process often succeeds even when the IDE shows errors.
- **IntelliSense Configuration is Critical:** For proper IDE support, correct include paths are essential.
- **Forward Declarations Help IDE Analysis:** Adding explicit forward declarations improves code navigation and error highlighting in the IDE without affecting compilation.
- **Library Management:** Keep libraries up-to-date and aligned with the framework version to minimize compatibility issues.

### Specific Fix Examples for Iridium Nano Project
For the specific errors in `src/main.cpp`, these solutions would help:

1. **For undefined `BLEDevice`, `BLEService`, etc.:**
   ```cpp
   // Add before function prototypes
   class BLEDevice;
   class BLEService;
   class BLEStringCharacteristic;
   ```

2. **For undefined `String`, `File`, etc.:**
   ```cpp
   // Ensure this comes first
   #include <Arduino.h>
   ```

3. **For undefined `Wire`, `millis()`, etc.:**
   ```cpp
   // Check platformio.ini has Arduino framework
   framework = arduino
   // Or explicitly for Mbed-based boards
   framework = mbed
   ``` 