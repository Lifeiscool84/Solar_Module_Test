# SparkFun Artemis/Apollo3 Blue Microcontroller Troubleshooting Guide

This document provides a specialized troubleshooting approach for the SparkFun Artemis family of development boards based on the Apollo3 Blue microcontroller, focusing on common issues and solutions when developing with PlatformIO, Arduino IDE, or Ambiq SDK.

## Table of Contents
1. [Development Environment Setup](#1-development-environment-setup)
2. [Build and Compilation Errors](#2-build-and-compilation-errors)
3. [Upload and Connectivity Problems](#3-upload-and-connectivity-problems)
4. [Power Management Issues](#4-power-management-issues)
5. [Apollo3-Specific Peripheral Problems](#5-apollo3-specific-peripheral-problems)
6. [BLE Communication Challenges](#6-ble-communication-challenges)
7. [Advanced Debugging Techniques](#7-advanced-debugging-techniques)
8. [Common Library Integration Issues](#8-common-library-integration-issues)

## 1. Development Environment Setup

### PlatformIO Configuration

#### Common Issues
- Missing board definition
- Incorrect framework selection
- Unable to find Apollo3 toolchain

#### Solution Steps
1. **Correct platformio.ini Setup:**
   ```ini
   [env:redboard_artemis_atp]
   platform = apollo3blue
   board = redboard_artemis_atp
   framework = arduino
   
   ; If using SparkFun's Artemis Nano:
   ; board = sparkfun_artemis_nano
   
   ; For advanced users with Ambiq SDK:
   ; framework = mbed
   ```

2. **Install Apollo3 Platform:**
   - Run `pio platform install apollo3blue` if not automatically installed
   - Verify installation with `pio platform list`

3. **Update PlatformIO Registry:**
   - Run `pio pkg update` to ensure latest board definitions
   - Check that Apollo3 toolchain is properly installed

4. **Fix c_cpp_properties.json for IntelliSense:**
   - Add Apollo3-specific include paths for proper code analysis:
   ```json
   "includePath": [
       "${workspaceFolder}/**",
       "${env:HOME}/.platformio/platforms/apollo3blue/*/cores/arduino",
       "${env:HOME}/.platformio/packages/framework-arduinoapollo3/cores/arduino",
       "${env:HOME}/.platformio/packages/framework-arduinoapollo3/libraries/**"
   ]
   ```

### Ambiq SDK Setup Issues

#### Common Issues
- Missing ARM toolchain
- Incorrect path configuration
- Build errors from mismatched SDK versions

#### Solution Steps
1. **Verify ARM Toolchain:**
   - Install GNU Arm Embedded Toolchain (9-2019-q4 or compatible)
   - Add toolchain to your PATH environment variable

2. **Ambiq SDK Configuration:**
   - Follow SparkFun's guide for setting up the Ambiq Apollo3 SDK
   - Verify AMBIQ_SDK environment variable points to SDK root
   - Ensure python3 and required packages are installed

3. **Serial Uploader Tool:**
   - Check that the Artemis SVL Uploader is properly installed
   - Test with basic example: `artemis_svl_uploader.py -f blink.bin`

## 2. Build and Compilation Errors

### Forward Declaration Issues

#### Common Issues
- "Undefined identifiers" for standard Arduino types (`String`, `File`)
- "Incomplete type" errors for classes like `BLEDevice`
- "Expected a ';'" cascade errors

#### Solution Steps
1. **Add Proper Forward Declarations:**
   ```cpp
   // Forward declarations for BLE
   class BLEDevice;
   class BLEService;
   class BLECharacteristic;
   class BLEStringCharacteristic;
   
   // Function prototypes using these types
   void handleClient(BLEDevice central);
   ```

2. **Correct Include Order:**
   ```cpp
   #include <Arduino.h>    // Always include first
   #include <Wire.h>       // Core hardware interfaces
   #include <SPI.h>
   #include <ArduinoBLE.h> // Functionality libraries
   
   // Project-specific includes
   #include "my_header.h"
   ```

3. **Apollo3-Specific Headers:**
   - Include the required Apollo3 headers for core functions:
   ```cpp
   #include "am_mcu_apollo.h" // For Artemis/Apollo hardware access
   ```

### Apollo3-Specific Compiler Errors

#### Common Issues
- Memory allocation failures
- Ambiguous overloaded functions
- Initialization order issues

#### Solution Steps
1. **Memory Model Configuration:**
   - Adjust memory model in platformio.ini if needed:
   ```ini
   build_flags = 
       -D ARM_MATH_CM4
       -mfloat-abi=hard
       -mfpu=fpv4-sp-d16
   ```

2. **Fix Common Apollo3 Errors:**
   - Handle namespace conflicts with ambiq/mbed libraries
   - Verify correct Arduino-style function signatures
   - Check for memory alignment issues with DMA

3. **Artemis BSP Version Compatibility:**
   - Ensure SparkFun_Apollo3 BSP version is compatible with your code
   - Check changelog for breaking changes between versions

## 3. Upload and Connectivity Problems

### Serial Connection Issues

#### Common Issues
- Unable to detect Artemis board
- "No device found" during upload
- Serial timeout during programming

#### Solution Steps
1. **Check USB-Serial Driver:**
   - Artemis boards use CH340 USB-to-serial chipset
   - Install CH340 drivers appropriate for your OS:
     - Windows: CH341SER.EXE
     - macOS: CH34xVCPDriver
     - Linux: Built-in kernel driver usually works

2. **Reset Before Upload:**
   - Artemis sometimes needs manual reset before upload
   - Press RESET button just before initiating upload
   - Try the double-tap reset technique (tap reset twice quickly)

3. **Upload Settings:**
   - Add specific upload settings to platformio.ini:
   ```ini
   upload_speed = 921600
   upload_port = COM3  # Adjust to your port
   upload_protocol = custom
   upload_command = python $PROJECT_PACKAGES_DIR/framework-arduinoapollo3/tools/artemis_svl_uploader.py -f ${SOURCE} -b ${upload_speed}
   ```

4. **Check SVL Bootloader:**
   - Ensure bootloader is intact (indicated by blue LED on reset)
   - If bootloader is corrupted, use Segger J-Link or another Artemis to restore

### JTAG/SWD Debugging Connection

#### Common Issues
- Unable to connect with JTAG/SWD
- "Error in initializing ST-LINK device"
- Debugger can't find target

#### Solution Steps
1. **J-Link Configuration:**
   - Install J-Link software specific to Apollo3
   - Configure correct target (Cortex-M4 for Apollo3)
   - Use initialization sequence specific to Apollo3:
   ```
   device AMAPH1KK-KBR
   speed 4000
   ```

2. **SWD Connection Verification:**
   - Check physical connections (SWDIO, SWCLK, GND)
   - Verify correct pinout for your specific Artemis board
   - Artemis Nano uses different pin placement than RedBoard Artemis

3. **Debug Settings in platformio.ini:**
   ```ini
   debug_tool = jlink
   debug_init_break = tbreak setup
   ```

## 4. Power Management Issues

### Power Consumption Problems

#### Common Issues
- Higher than expected power draw
- Power-down modes not working
- Failure to wake from sleep

#### Solution Steps
1. **Apollo3 Power Modes Configuration:**
   ```cpp
   // Configure sleep mode
   am_hal_pwrctrl_low_power_init();
   
   // Enter deep sleep
   am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
   
   // Wake-up sources
   am_hal_gpio_pin_config(pinNumber, AM_HAL_GPIO_PINCFG_INPUT);
   am_hal_gpio_interrupt_control(AM_HAL_GPIO_INT_CHANNEL_0, 
                              AM_HAL_GPIO_INT_CTRL_ENABLE, 
                              &pinConfigStruct);
   ```

2. **Clock and Timer Configuration:**
   - Set appropriate clock speeds using Apollo3 HAL
   - Use the built-in timer functions for wake-up
   - Configure RTC for precise timing control

3. **Peripheral Power Management:**
   - Disable unused peripherals explicitly
   - Proper power-down sequence for each peripheral:
   ```cpp
   // Disable ADC
   am_hal_adc_disable(adc_handle);
   
   // Power down BLE radio
   am_hal_ble_power_control(ble_handle, AM_HAL_BLE_POWER_OFF);
   ```

### Brown-Out and Reset Issues

#### Common Issues
- Unexpected resets during operation
- Brown-out detection triggering incorrectly
- Voltage regulator issues

#### Solution Steps
1. **Power Supply Quality:**
   - Use stable power supply with sufficient current capability
   - Add bulk capacitors (10-100µF) near VIN pin
   - Add decoupling capacitors (0.1µF) near power pins

2. **Brown-Out Detection Configuration:**
   - Configure BoD threshold appropriate for your application
   - Implement graceful shutdown on low voltage detection

3. **Debug Reset Cause:**
   ```cpp
   void checkResetCause() {
     uint32_t resetCause = am_hal_reset_control(AM_HAL_RESET_CONTROL_GET_STATUS, 0);
     
     Serial.print("Reset cause: 0x");
     Serial.println(resetCause, HEX);
     
     if (resetCause & AM_HAL_RESET_POR)
       Serial.println("Power-on reset");
     if (resetCause & AM_HAL_RESET_BOD)
       Serial.println("Brown-out reset");
     if (resetCause & AM_HAL_RESET_SWPOR)
       Serial.println("Software POR reset");
     if (resetCause & AM_HAL_RESET_WDT)
       Serial.println("Watchdog timer reset");
     
     // Clear reset status
     am_hal_reset_control(AM_HAL_RESET_CONTROL_CLEAR, 0);
   }
   ```

## 5. Apollo3-Specific Peripheral Problems

### I2C Interface Issues

#### Common Issues
- I2C bus hangs or freezes
- IOM resource conflicts
- Multiple I2C buses configuration errors

#### Solution Steps
1. **Apollo3 I2C Configuration:**
   - Apollo3 has 6 IOM (I/O Master) modules that can be I2C or SPI
   - Configure I2C correctly using Wire or HAL:
   ```cpp
   // Using Arduino Wire
   Wire.begin();  // Default I2C bus
   Wire.setClock(400000);  // 400 kHz
   
   // For second I2C bus
   Wire1.begin();
   Wire1.setClock(400000);
   ```

2. **IOM Resource Management:**
   - Track which IOM is used for which interface
   - Avoid conflicts between SPI and I2C on the same IOM
   - Artemis Nano predefined pin assignments:
     - I2C0: SDA=25, SCL=27
     - I2C1: SDA=17, SCL=16

3. **Bus Recovery Procedure:**
   - Reset I2C bus if it gets stuck:
   ```cpp
   void resetI2CBus() {
     // Reinitialize the Wire interface
     Wire.end();
     delay(10);
     Wire.begin();
     Wire.setClock(400000);
   }
   ```

### SPI Interface Issues

#### Common Issues
- SD card communication failures
- SPI clock speed issues
- Conflicting SPI implementations

#### Solution Steps
1. **Apollo3 SPI Configuration:**
   - Configure SPI using appropriate IOM module
   - Set correct SPI mode and clock speed for your peripherals:
   ```cpp
   // Standard Arduino SPI
   SPI.begin();
   SPI.setClockDivider(SPI_CLOCK_DIV4);
   SPI.setDataMode(SPI_MODE0);
   
   // For SD card specific fixes
   SD.begin(SD_CS_PIN, SPI_HALF_SPEED);
   ```

2. **Avoid MbedSPI Conflicts:**
   - The Artemis Arduino core can have conflicts between Arduino SPI and MbedSPI
   - If you modified SPI.h/SPI.cpp, keep original versions as backups
   - For SD card compatibility, sometimes standard Arduino SPI works better

3. **Chip Select Management:**
   - Handle CS pins manually for better control
   - Ensure proper timing between CS and SPI transactions
   ```cpp
   digitalWrite(CS_PIN, LOW);
   SPI.transfer(cmd);
   SPI.transfer(data, len);
   digitalWrite(CS_PIN, HIGH);
   ```

### ADC and Analog Interface Issues

#### Common Issues
- Inaccurate ADC readings
- Analog reference issues
- ADC saturation or noise problems

#### Solution Steps
1. **Apollo3 ADC Configuration:**
   - Configure ADC precision and reference:
   ```cpp
   // Arduino style
   analogReadResolution(14);  // Apollo3 has 14-bit ADC
   
   // HAL style
   am_hal_adc_config_t ADCConfig;
   am_hal_adc_initialize(0, &adc_handle);
   am_hal_adc_configure(adc_handle, &ADCConfig);
   ```

2. **ADC Noise Reduction:**
   - Add hardware filtering (RC low-pass filter)
   - Implement software averaging for stable readings
   ```cpp
   int getAverageAnalogReading(int pin, int samples) {
     long sum = 0;
     for (int i = 0; i < samples; i++) {
       sum += analogRead(pin);
       delay(1);
     }
     return sum / samples;
   }
   ```

3. **Reference Voltage Considerations:**
   - Apollo3 ADC reference is tied to supply voltage
   - For precise measurements, consider external reference

## 6. BLE Communication Challenges

### ArduinoBLE Library Issues

#### Common Issues
- BLE initialization failures
- Connection stability problems
- Characteristic notification issues

#### Solution Steps
1. **Proper BLE Initialization:**
   ```cpp
   if (!BLE.begin()) {
     Serial.println("Starting BLE failed!");
     
     // Apollo3-specific reset for BLE
     am_hal_ble_power_control(ble_handle, AM_HAL_BLE_POWER_OFF);
     delay(100);
     am_hal_ble_power_control(ble_handle, AM_HAL_BLE_POWER_ON);
     
     if (!BLE.begin()) {
       while (1) {
         delay(1000);
         Serial.println("Retrying BLE init...");
       }
     }
   }
   ```

2. **BLE Connection Management:**
   - Set appropriate connection parameters
   - Implement robust disconnection handling:
   ```cpp
   void blePeripheralConnectHandler(BLEDevice central) {
     // Connection event handling
     digitalWrite(LED_PIN, HIGH);
   }
   
   void blePeripheralDisconnectHandler(BLEDevice central) {
     // Clean disconnection handling
     digitalWrite(LED_PIN, LOW);
     // Restart advertising when disconnected
     BLE.advertise();
   }
   
   // Register event handlers
   BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
   BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);
   ```

3. **Characteristic Notification Fixes:**
   - Break long messages into chunks to avoid MTU limits
   - Add delays between notifications
   ```cpp
   void sendLongMessage(const String& message) {
     const int chunkSize = 20; // Adjust based on MTU
     for (int i = 0; i < message.length(); i += chunkSize) {
       String chunk = message.substring(i, min(i + chunkSize, message.length()));
       myCharacteristic.writeValue(chunk);
       delay(50); // Small delay between chunks
     }
   }
   ```

4. **Apollo3-Specific BLE Configuration:**
   - Configure BLE power levels appropriately
   - Set advertising parameters for optimal performance
   ```cpp
   BLE.setAdvertisingInterval(100); // 100ms interval
   BLE.setConnectionInterval(100, 200); // Min 100ms, max 200ms
   ```

### BLE Debugging

#### Common Issues
- Difficult to diagnose connection issues
- Unknown BLE state
- Poor range or signal strength

#### Solution Steps
1. **BLE State Monitoring:**
   ```cpp
   void printBLEState() {
     if (BLE.advertising()) {
       Serial.println("BLE is advertising");
     } else if (BLE.connected()) {
       Serial.println("BLE is connected");
     } else {
       Serial.println("BLE is initialized but idle");
     }
   }
   ```

2. **Use External BLE Analyzer:**
   - nRF Connect app for Android/iOS
   - Wireshark with BLE sniffing hardware
   - Ellisys Bluetooth Analyzer for professional debugging

3. **RadioFruit BLE Command Mode:**
   - Some Artemis boards support direct AT command mode
   - Enter command mode for diagnostic information
   - Check signal strength and connection quality

## 7. Advanced Debugging Techniques

### Apollo3 Hardware Debugging

#### Techniques
1. **JLINK Integration:**
   - Use Segger J-Link with Apollo3 target
   - Set up SWD connection
   - Example platformio.ini configuration:
   ```ini
   debug_tool = jlink
   debug_init_break = tbreak setup
   debug_load_mode = modified
   debug_init_cmds =
     target remote $DEBUG_PORT
     monitor reset
     monitor halt
   ```

2. **LED Status Patterns:**
   - Define clear error code patterns
   - Example Apollo3 RGB LED handler:
   ```cpp
   void setErrorPattern(int errorCode) {
     // Red blinks for error code 
     for (int i = 0; i < errorCode; i++) {
       digitalWrite(LED_RED_PIN, HIGH);
       delay(200);
       digitalWrite(LED_RED_PIN, LOW);
       delay(200);
     }
     delay(1000); // Pause between patterns
   }
   ```

3. **Watchdog Implementation:**
   ```cpp
   void setupWatchdog(uint32_t timeout_ms) {
     am_hal_wdt_config_t wdt_config;
     wdt_config.ui32Config = AM_HAL_WDT_ENABLE_RESET | AM_HAL_WDT_ENABLE_INTERRUPT;
     wdt_config.ui32IntValue = timeout_ms * 32768 / 1000; // Convert ms to WDT ticks
     wdt_config.ui32ResetValue = wdt_config.ui32IntValue + 16; // Reset slightly after interrupt
     am_hal_wdt_init(&wdt_config);
     am_hal_wdt_start();
   }
   
   void feedWatchdog() {
     am_hal_wdt_restart();
   }
   ```

### Apollo3 Software Debugging

#### Techniques
1. **Memory Analysis:**
   - Monitor memory usage on Apollo3
   ```cpp
   void printMemoryUsage() {
     extern unsigned int __heap_start;
     extern unsigned int __heap_end;
     extern char *__brkval;
     
     Serial.print("Heap usage: ");
     Serial.print((uint32_t)__brkval - (uint32_t)&__heap_start);
     Serial.print(" / ");
     Serial.print((uint32_t)&__heap_end - (uint32_t)&__heap_start);
     Serial.println(" bytes");
   }
   ```

2. **Custom Assertion Macro:**
   ```cpp
   #define ASSERT(condition, message) \
     do { \
       if (!(condition)) { \
         Serial.print(F("Assertion failed in ")); \
         Serial.print(__FILE__); \
         Serial.print(F(", line ")); \
         Serial.print(__LINE__); \
         Serial.print(F(": ")); \
         Serial.println(F(message)); \
         while (1) { \
           digitalWrite(LED_RED_PIN, HIGH); \
           delay(100); \
           digitalWrite(LED_RED_PIN, LOW); \
           delay(100); \
         } \
       } \
     } while (0)
   ```

3. **Function Execution Timing:**
   ```cpp
   uint32_t measureExecutionTime(void (*func)()) {
     uint32_t start = micros();
     func();
     uint32_t end = micros();
     return end - start;
   }
   ```

## 8. Common Library Integration Issues

### SparkFun Library Compatibility

#### Common Issues
- Version conflicts between libraries
- Initialization failures 
- Configuration mismatches

#### Solution Steps
1. **SparkFun Apollo3 Library Updates:**
   - Use PlatformIO Library Manager to install latest versions
   - Check compatibility notes for each library
   - Add correct dependencies in platformio.ini:
   ```ini
   lib_deps =
     sparkfun/SparkFun BNO080 Cortex Based IMU
     sparkfun/SparkFun u-blox GNSS Arduino Library
   ```

2. **I2C Address Conflicts:**
   - Map I2C addresses used in your project
   - Use I2C multiplexer if needed
   - Use alternate I2C bus (Wire1) for conflicting devices

3. **Apollo3 Hardware Abstraction Layer:**
   - Some libraries may bypass Arduino API
   - Check for direct HAL calls that might conflict
   - Ensure libraries are specifically compatible with Apollo3

### Iridium and Satellite Communication Libraries

#### Common Issues
- Power supply issues for transmitters
- I/O pin conflicts
- Timeout and synchronization problems

#### Solution Steps
1. **Power Management for Transmitters:**
   - Ensure adequate power supply for transmission current spikes
   - Consider separate power supply for satellite modem
   - Implement proper power-up sequence:
   ```cpp
   // Enable power to Iridium modem
   digitalWrite(IRIDIUM_PWR_PIN, HIGH);
   delay(1000); // Allow time to stabilize
   
   // Initialize modem
   modem.setPowerProfile(IridiumSBD::USB_POWER_PROFILE);
   modem.begin();
   ```

2. **I/O Management with Apollo3:**
   - Verify sleep/wake functionality with Apollo3 power modes
   - Use appropriate GPIO interrupts for modem signals
   ```cpp
   // Configure ring indicator as interrupt
   am_hal_gpio_pin_config(RING_PIN, AM_HAL_GPIO_PINCFG_INPUT_PULLUP);
   attachInterrupt(digitalPinToInterrupt(RING_PIN), ringIndicatorISR, FALLING);
   ```

3. **Timeout and Synchronization:**
   - Implement appropriate timeouts for satellite operations
   - Use non-blocking state machine for long operations
   ```cpp
   // Non-blocking state machine example
   enum IridiumState {
     IDLE,
     INIT,
     CHECKING_SIGNAL,
     SENDING,
     RECEIVING,
     ERROR
   };
   
   IridiumState currentState = IDLE;
   unsigned long stateEntryTime = 0;
   
   void updateIridiumStateMachine() {
     unsigned long now = millis();
     
     switch (currentState) {
       case IDLE:
         // Handle idle state
         break;
         
       case CHECKING_SIGNAL:
         if (now - stateEntryTime > 10000) {
           // Timeout after 10 seconds of checking signal
           currentState = ERROR;
           break;
         }
         
         // Check signal quality
         int signalQuality = -1;
         int err = modem.getSignalQuality(signalQuality);
         if (err == ISBD_SUCCESS) {
           if (signalQuality > 2) {
             // Signal is good enough, move to sending
             currentState = SENDING;
             stateEntryTime = now;
           }
         }
         break;
         
       // Other states...
     }
   }
   ```

## Appendix: SparkFun Artemis Hardware Reference

### Board-Specific Pinouts and Features

#### Artemis Nano
- **Processor:** Apollo3 Blue, 48MHz (96MHz Burst)
- **I2C:** SDA on pin 25, SCL on pin 27 (Wire)
         SDA on pin 17, SCL on pin 16 (Wire1)
- **SPI:** MOSI on pin 11, MISO on pin 12, SCK on pin 13
- **Serial:** TX on pin 1, RX on pin 0
- **LEDs:** Blue LED on pin 19
- **Flash:** 1MB internal flash
- **SRAM:** 384KB
- **Special Features:** 
  - Ultra-low power consumption (~30µA/MHz)
  - BLE radio built-in
  - 10 ADC pins
  - PDM microphone input capability

#### RedBoard Artemis
- **Additional Features:**
  - Qwiic/STEMMA QT connector (I2C)
  - Multiple form factor options
  - Enhanced power regulation

#### Power Considerations
- Supply voltage: 3.3V (regulated from USB)
- Absolute maximum: 3.6V
- Minimum operating: 1.8V
- Deep sleep current: < 2.5µA

## Resources

### Documentation
- [SparkFun Artemis Development Guide](https://learn.sparkfun.com/tutorials/artemis-development-with-arduino)
- [Ambiq Apollo3 Datasheet](https://cdn.sparkfun.com/assets/learn_tutorials/9/2/2/Apollo3-Blue-SoC-Datasheet.pdf)
- [SparkFun Artemis Hookup Guide](https://learn.sparkfun.com/tutorials/hookup-guide-for-the-sparkfun-redboard-artemis)

### Tools
- [Artemis SVL Uploader](https://github.com/sparkfun/Apollo3_Uploader_ASB)
- [Ambiq Apollo3 SDK](https://github.com/sparkfun/AmbiqSuiteSDK)
- [J-Link Software](https://www.segger.com/downloads/jlink/)

### Community Resources
- [SparkFun Forum - Artemis Section](https://forum.sparkfun.com/viewforum.php?f=168)
- [GitHub - SparkFun Artemis Arduino Core](https://github.com/sparkfun/Arduino_Apollo3)

---

This guide serves as a specialized reference for troubleshooting SparkFun Artemis/Apollo3 Blue microcontroller projects. For any issues not covered here, refer to the SparkFun forums or GitHub repositories for additional community-based solutions. 