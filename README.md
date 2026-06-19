# MeteoHub S3

> **Minimum supported version: 1.1.3**

## Full Documentation

* [Documentation Index](docs/index.md)

## Getting Started

* [Beginner Guide](docs/beginner/index.md)

## Overview

MeteoHub S3 is a PlatformIO project for ESP32-S3 centered around an OLED dashboard (SH1106/SSD1306 using U8g2). It displays local sensor data, weather forecasts, system logs, and device status through both the OLED interface and the integrated web dashboard.

## Hardware Requirements

* OLED display (SH1106 or SSD1306, I2C)
* HW-040 rotary encoder module (encoder with push button)
* Back and Confirm buttons
* AHT20 and BMP280 sensors
* Optional SD card for long-term data storage (recommended: FAT32, 4-32 GB)

## Building the Project

* Install PlatformIO in Visual Studio Code
* Select the `esp32-s3-oled` environment
* Build:

```bash
platformio run
```

* Upload:

```bash
platformio run --target upload
```

## Key Features (v1.1.x Highlights)

* **Improved SD Card Reliability (v1.1.2)**
  Safe write operations with explicit `flush()` calls and Mutex protection to reduce the risk of file corruption.

* **Standard C++ Engine (v1.1.0)**
  Core web engine refactored to use `std::string`, providing improved memory stability and maintainability.

* **Advanced File Management**
  Upload, download, and delete files directly from the web interface.

* **Robust OTA Updates**
  Secure wireless firmware update system.

* **Advanced Charts**
  Three scaling modes (Fixed, Dynamic, and Mixed) for temperature, humidity, and pressure visualization.

## Usage

See the [User Guide](docs/user_guide.md) for detailed information about chart scaling modes, file management, and system operation.

## SD Card Notes

To minimize the risk of data corruption:

1. Format the SD card using **FAT32** (32 KB allocation size recommended).
2. Properly shut down the device before removing the card, even though data is protected by `flush()` operations after each write.

---

### Configuration Notes

Any changes made to `config.h` are automatically injected into the web interface during the build process.
