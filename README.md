# MeteoHub S3

> **Minimum supported version: 1.9.0**

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

## Key Features (v1.9.x Highlights)

* **Compact binary history storage (v1.4.0+)**
  Per-day binary files (`/history/YYYY/MM/YYYY-MM-DD.bin`) with a file header for forward compatibility, plus daily `.stats` files. Direct record access, far smaller than CSV. CSV is now export-only; legacy CSV files are migrated automatically on first boot.

* **Fast history display (v1.8.0)**
  Sequential block reads (no per-record seek) and an SD SPI frequency ladder (20/10/4/1 MHz, verified by a write+read-back test) dramatically speed up history loading, with a safe fallback.

* **History page: period selection & comparison**
  Pick a window (24h / 48h / 7d / 30d / today / custom range), compare two periods, optional per-metric synthesis line, an inverted "Zoom" scale control, and a real-time toggle.

* **System page (hub)**
  OTA firmware update, NeoLED brightness (persisted in NVS), CSV/config exports, and access to the File manager and Logs. The main menu is streamlined to four entries: Dashboard, Statistics, History, System.

* **Data quality & sensor robustness (v1.6.x)**
  I2C reads are validated, retried, and the bus auto-recovers on repeated failures; failed reads are not stored. Outliers are removed from charts (temporal coherence) and statistics (robust median/MAD) while raw data is preserved.

* **UDP log monitoring (v1.9.0)**
  App and ESP-core logs (WiFi, I2C, watchdog…) are broadcast over UDP so you can follow them wirelessly (e.g. in Tabby), no serial cable required. Configurable in `config.h`.

* **Reliable SD writes**
  Safe write operations with explicit `flush()` and mutex protection to reduce the risk of file corruption.

## Usage

See the [User Guide](docs/user_guide.md) for details on the History page, the System page, chart scaling, exports, and system operation.

## SD Card Notes

To minimize the risk of data corruption:

1. Format the SD card using **FAT32** (32 KB allocation size recommended).
2. Properly shut down the device before removing the card, even though data is protected by `flush()` operations after each write.
3. Back up the `/history` folder before a major firmware upgrade (the binary format is migrated automatically, but a backup is good practice).

---

### Configuration Notes

`config.h` is compiled into the firmware; the web assets in `data/` are embedded at build time (`scripts/embed_web_files.py`). The effective configuration can be exported from the **System** page or via `GET /api/config/export`. Runtime settings such as NeoLED brightness are stored in NVS.
