# Iridium Nano BLE Project

## Project Overview
The Iridium Nano BLE project integrates Bluetooth Low Energy capabilities with Iridium satellite communication and GNSS (Global Navigation Satellite System) for remote data transmission in areas without cellular coverage. The system enables data collection, storage, and transmission via both Bluetooth and satellite networks, making it ideal for remote monitoring applications.

## Current System Status
The current implementation includes:
- [x] Bluetooth connectivity for device control and configuration
- [x] Iridium satellite communication for remote data transmission
- [x] GNSS integration for location tracking
- [x] SD card data storage and management
- [x] Basic command interface via BLE
- [x] Signal quality reporting for both Iridium and GNSS systems

## Project Goals
1. [x] Improve user interface and command structure *(in progress)*
2. [ ] Enhance data management capabilities
3. [ ] Optimize power consumption for field deployment
4. [ ] Improve reliability and error handling
5. [ ] Add advanced features for remote monitoring applications

## Project Tasks and Progress

### 1. Command Interface Enhancement
**Status:** In Progress  
**Priority:** High  
**Complexity:** 7/10  
**Timeline:** 2025-05-10 to 2025-05-24

#### Subtasks:
- [x] 1.1 Menu Command Implementation - Add a dedicated 'M' command to display the menu options
- [x] 1.2 File Listing Command Refinement - Update the 'L' command to focus only on file listing
- [x] 1.3 System Status Command Implementation - Implement the 'ST' command to show Iridium and GNSS status
- [ ] 1.4 File Management Command Improvements - Enhance file operation commands with better feedback
- [ ] 1.5 New Configuration Commands - Add new commands for system configuration

### 2. Data Management Improvements
**Status:** Pending  
**Priority:** High  
**Complexity:** 8/10  
**Timeline:** 2025-05-25 to 2025-06-14

#### Requirements:
- [ ] Implement intelligent data buffering for satellite transmission
- [ ] Add configurable data logging intervals and formats
- [ ] Improve file naming convention and organization
- [ ] Implement data compression for satellite transmission
- [ ] Add data validation and error checking

### 3. Connectivity Enhancements
**Status:** Pending  
**Priority:** Medium  
**Complexity:** 7/10  
**Timeline:** 2025-06-15 to 2025-06-28

#### Requirements:
- [ ] Improve Bluetooth connection reliability
- [ ] Add reconnection capabilities after signal loss
- [ ] Implement more robust Iridium connection handling
- [ ] Add signal quality monitoring and adaptive transmission

### 4. Power Management Optimization
**Status:** Pending  
**Priority:** High  
**Complexity:** 9/10  
**Timeline:** 2025-06-29 to 2025-07-19

#### Requirements:
- [ ] Implement sleep modes for extended battery life
- [ ] Add power usage monitoring and reporting
- [ ] Create configurable power profiles for different deployment scenarios
- [ ] Optimize transmission scheduling to minimize power consumption

### 5. User Experience Improvements
**Status:** Pending  
**Priority:** Medium  
**Complexity:** 6/10  
**Timeline:** 2025-07-20 to 2025-08-02

#### Requirements:
- [ ] Improve status reporting and error messages
- [ ] Add progress indicators for long operations
- [ ] Implement better handling of transmission failures
- [ ] Add configuration storage and retrieval for persistent settings

### 6. Advanced Features Implementation
**Status:** Pending  
**Priority:** Low  
**Complexity:** 10/10  
**Timeline:** 2025-08-03 to 2025-08-30

#### Requirements:
- [ ] Geofencing capabilities using GNSS
- [ ] Scheduled data transmission windows
- [ ] Remote device configuration via Iridium
- [ ] Two-way communication for remote command execution
- [ ] Sensor integration framework for external data sources

## Technical Specifications
- Arduino/PlatformIO environment
- Artemis/Apollo3 microcontroller platform
- BLE communication protocol
- Iridium 9603N satellite modem
- u-blox GNSS receiver
- SD card storage
- Low power design target (<50mA avg. current draw)

## Success Criteria
- Successful end-to-end data transmission via both BLE and Iridium
- <50mA average power consumption in normal operation
- >95% successful transmission rate in field testing
- Intuitive command interface with comprehensive error handling
- Robust data storage with no data loss during power interruptions

## Progress Tracker
- [x] Initialize project in Task Master
- [x] Create comprehensive PRD
- [x] Generate task structure
- [x] Implement Menu Command (M)
- [x] Implement File Listing Command (L)
- [x] Implement System Status Command (ST)
- [ ] Complete Command Interface Enhancement task
- [ ] Complete Data Management Improvements task
- [ ] Complete Connectivity Enhancements task
- [ ] Complete Power Management Optimization task
- [ ] Complete User Experience Improvements task
- [ ] Complete Advanced Features Implementation task

# Iridium Nano BLE Project Documentation

This directory contains documentation for the Iridium Nano BLE project.

## Documentation Organization

* `planning/` - Planning documents and project requirements
  * `product_requirements.md` - Product Requirements Document (PRD)

* `progress/` - Progress tracking and task status
  * `task_dashboard.md` - Dashboard generated from Task Master data
  * `task_status.md` - Task status information

* `tasks/` - Individual task documentation (optional)

## Task Management System

This project uses Task Master to manage tasks. The task system consists of several components:

1. **Essential task data** (in the `tasks/` directory at the project root):
   * `tasks.json` - Primary task database
   * Individual task files (e.g., `task_001.txt`)

2. **Documentation files** (in this `docs/` directory):
   * Generated markdown files for human readability
   * Can be regenerated from the task data

For a detailed explanation of the task management system, see:
* [Task System Guide](task_system_guide.md)

## Common Task Master Commands

* `npx task-master list` - Show all tasks
* `npx task-master next` - Find next task to work on
* `npx task-master show <id>` - View details of a specific task

## Generating Documentation

To regenerate the task dashboard:
```
./scripts/task_dashboard.ps1
``` 