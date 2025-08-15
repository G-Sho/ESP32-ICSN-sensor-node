# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Development Commands

### Building and Uploading
- `python -m platformio run` - Build the project for ESP32
- `python -m platformio run --target upload` - Build and upload to ESP32 (COM3)
- `python -m platformio device monitor` - Open serial monitor (115200 baud)
- `python -m platformio run --target uploadfs` - Upload filesystem data (LittleFS)

### Testing and Development
- Serial commands available when device is running:
  - `send_interest` - Send INTEREST packet via ESP-NOW
  - `read_sensor` - Simulate sensor data transmission
  - `help` - Show available commands

## Project Architecture

This is an **Information-Centric Sensor Network (ICSN)** implementation for ESP32 using ESP-NOW protocol. The project implements a clean architecture pattern with the following key layers:

### Core Components
- **ESP_NOWController** (`lib/ICSN/controller/`): Main controller bridging ESP-NOW hardware with business logic
- **UseCaseInteractor** (`lib/ICSN/use_case/`): Core business logic implementing ICSN protocol
- **Data Repositories** (`lib/ICSN/infrastructure/data_access/`): LRU-based caching for FIB, PIT, and CS tables

### Data Flow
1. **ESP-NOW packets** received → converted to `InputData`
2. **UseCaseInteractor** processes based on signal type (INTEREST/DATA/INVALID)
3. **Repository pattern** manages routing tables (FIB), pending interests (PIT), and content store (CS)
4. **OutputData** generated → converted to `ESP_NOWControlData` → sent via ESP-NOW

### Configuration
- System configuration in `data/config.json` with table sizes, TTL, hop thresholds
- Configuration loaded at startup via `loadSystemConfig()`
- LittleFS filesystem used for configuration storage

### Signal Types
- **INTEREST**: Request for specific content (name-based routing)
- **DATA**: Content response packets
- **INVALID**: Error/unknown packet type

### Key Data Structures
- `ESP_NOWControlData`: Application-layer packet format with multiple TX addresses
- `CommunicationData`: Wire protocol format for ESP-NOW transmission
- `InputData`/`OutputData`: Clean architecture boundaries for business logic

### Hardware Configuration
- ESP32 development board
- WiFi STA mode, Channel 1
- ESP-NOW for mesh communication
- Optional DHT/BME280 sensors supported

## Important Notes
- Project uses C++11 standard
- No external mesh libraries (painlessMesh commented out)
- TaskScheduler for periodic operations
- Clean separation between hardware (ESP-NOW) and protocol logic (ICSN)
- All string operations use fixed-size buffers for embedded safety