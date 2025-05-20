# Iridium Nano BLE Project: Product Requirements Document

## Overview
The Iridium Nano BLE project integrates Bluetooth Low Energy capabilities with Iridium satellite communication and GNSS (Global Navigation Satellite System) for remote data transmission in areas without cellular coverage. The system enables data collection, storage, and transmission via both Bluetooth and satellite networks, making it ideal for remote monitoring applications.

## Current System Status
The current implementation includes:
- Bluetooth connectivity for device control and configuration
- Iridium satellite communication for remote data transmission
- GNSS integration for location tracking
- SD card data storage and management
- Basic command interface via BLE
- Signal quality reporting for both Iridium and GNSS systems

## Project Goals
1. Improve user interface and command structure
2. Enhance data management capabilities
3. Optimize power consumption for field deployment
4. Improve reliability and error handling
5. Add advanced features for remote monitoring applications

## Detailed Requirements

### Command Interface Enhancement
- Reorganize command structure to be more intuitive and consistent
- Add a dedicated menu command ("M") separate from file listing
- Add system status command ("ST") to show Iridium and GNSS status
- Improve file management commands with better feedback
- Add new commands for system configuration and diagnostics

### Data Management Improvements
- Implement intelligent data buffering for satellite transmission
- Add configurable data logging intervals and formats
- Improve file naming convention and organization
- Implement data compression for satellite transmission
- Add data validation and error checking

### Connectivity Enhancements
- Improve Bluetooth connection reliability
- Add reconnection capabilities after signal loss
- Implement more robust Iridium connection handling
- Add signal quality monitoring and adaptive transmission

### Power Management
- Implement sleep modes for extended battery life
- Add power usage monitoring and reporting
- Create configurable power profiles for different deployment scenarios
- Optimize transmission scheduling to minimize power consumption

### User Experience
- Improve status reporting and error messages
- Add progress indicators for long operations
- Implement better handling of transmission failures
- Add configuration storage and retrieval for persistent settings

### Advanced Features
- Geofencing capabilities using GNSS
- Scheduled data transmission windows
- Remote device configuration via Iridium
- Two-way communication for remote command execution
- Sensor integration framework for external data sources

## Technical Specifications
- Arduino/PlatformIO environment
- Artemis/Apollo3 microcontroller platform
- BLE communication protocol
- Iridium 9603N satellite modem
- u-blox GNSS receiver
- SD card storage
- Low power design target (<50mA avg. current draw)

## Timeline
1. Command Interface Enhancement - 2 weeks
2. Data Management Improvements - 3 weeks
3. Connectivity Enhancements - 2 weeks
4. Power Management - 3 weeks
5. User Experience Improvements - 2 weeks
6. Advanced Features - 4 weeks

## Success Criteria
- Successful end-to-end data transmission via both BLE and Iridium
- <50mA average power consumption in normal operation
- >95% successful transmission rate in field testing
- Intuitive command interface with comprehensive error handling
- Robust data storage with no data loss during power interruptions 