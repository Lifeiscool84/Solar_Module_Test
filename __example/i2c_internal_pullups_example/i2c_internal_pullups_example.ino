/**
 * @file i2c_internal_pullups_example.ino
 * @brief Example demonstrating how to configure Apollo3 internal I2C pull-ups.
 *
 * This sketch shows how to use the Ambiq HAL functions to explicitly set the
 * internal pull-up resistor strength for the I2C pins (SDA and SCL) before
 * initializing the Wire library.
 *
 * Note: While internal pull-ups exist, it is generally recommended to use
 * appropriate EXTERNAL pull-up resistors (e.g., 2.2kOhm - 4.7kOhm) for
 * reliable I2C communication, especially at higher speeds (400kHz) or on
 * longer buses. This example primarily serves to demonstrate the HAL configuration.
 */

#include <Arduino.h>
#include <Wire.h>          // Standard Arduino I2C library
#include <am_hal_gpio.h>   // Required for HAL GPIO configuration functions and types

// Define the I2C pins for the default Wire interface (IOM0) on most Artemis boards
const int I2C_SCL_PIN = 5;
const int I2C_SDA_PIN = 6;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for Serial port to connect
  delay(1000);
  Serial.println("I2C Internal Pull-up Configuration Example");

  // --- Start I2C Pull-up Configuration ---

  // 1. Choose the desired pull-up resistor strength.
  //    Only specific pads support these stronger pull-ups (incl. IOM0 SCL/SDA: 5/6).
  //    Uncomment the desired value and comment out the others.
  //    Using AM_HAL_GPIO_PIN_PULLUP_NONE effectively disables the internal pull-up.

  // am_hal_gpio_pullup_t pullup_strength = AM_HAL_GPIO_PIN_PULLUP_NONE; // No internal pull-up
  // am_hal_gpio_pullup_t pullup_strength = AM_HAL_GPIO_PIN_PULLUP_1_5K; // 1.5 kOhm
  am_hal_gpio_pullup_t pullup_strength = AM_HAL_GPIO_PIN_PULLUP_6K;   // 6 kOhm (Often a reasonable choice if using internal)
  // am_hal_gpio_pullup_t pullup_strength = AM_HAL_GPIO_PIN_PULLUP_12K;  // 12 kOhm
  // am_hal_gpio_pullup_t pullup_strength = AM_HAL_GPIO_PIN_PULLUP_24K;  // 24 kOhm

  Serial.print("Configuring I2C pins with pull-up setting: ");
  switch(pullup_strength) {
    case AM_HAL_GPIO_PIN_PULLUP_NONE: Serial.println("NONE"); break;
    case AM_HAL_GPIO_PIN_PULLUP_1_5K: Serial.println("1.5K"); break;
    case AM_HAL_GPIO_PIN_PULLUP_6K:   Serial.println("6K");   break;
    case AM_HAL_GPIO_PIN_PULLUP_12K:  Serial.println("12K");  break;
    case AM_HAL_GPIO_PIN_PULLUP_24K:  Serial.println("24K");  break;
    default:                          Serial.println("Unknown (Weak?)"); break; // Should not happen for I2C capable pins if using above defines
  }

  // 2. Configure SCL Pad (Pad 5 for Wire/IOM0)
  //    Start with the default Board Support Package (BSP) configuration for the pin's function (IOM0 SCL)
  am_hal_gpio_pincfg_t scl_cfg = g_AM_BSP_GPIO_IOM0_SCL;
  //    Modify only the pull-up setting
  scl_cfg.ePullup = pullup_strength;
  //    Apply the modified configuration to the pin
  am_hal_gpio_pinconfig(I2C_SCL_PIN, &scl_cfg);
  Serial.println(" - SCL Pad configured.");

  // 3. Configure SDA Pad (Pad 6 for Wire/IOM0)
  //    Start with the default BSP configuration for the pin's function (IOM0 SDA)
  am_hal_gpio_pincfg_t sda_cfg = g_AM_BSP_GPIO_IOM0_SDA;
  //    Modify only the pull-up setting
  sda_cfg.ePullup = pullup_strength;
  //    Apply the modified configuration to the pin
  am_hal_gpio_pinconfig(I2C_SDA_PIN, &sda_cfg);
  Serial.println(" - SDA Pad configured.");

  // --- End I2C Pull-up Configuration ---

  // 4. Initialize the Wire library AFTER configuring the pins
  Serial.println("Initializing Wire library...");
  Wire.begin();
  Serial.println("Wire library initialized.");

  Serial.println("\nStarting I2C Scanner...");
}

void loop() {
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    // Wire.endTransmission() returns
    // 0 if device acknowledges
    // 1 if data too long to fit in transmit buffer (shouldn't happen here)
    // 2 if received NACK on transmit of address (device not found)
    // 3 if received NACK on transmit of data (shouldn't happen here)
    // 4 other error
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      nDevices++;
    }
    else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
    // Ignore error code 2 (NACK / device not present)
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found");
  else
    Serial.println("Scan complete.");

  delay(5000); // Wait 5 seconds for next scan
} 