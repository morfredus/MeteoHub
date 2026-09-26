# MeteoHub S3

*Read in another language: **English** (this document) · [Français](README.fr.md).*

[![Version](https://img.shields.io/badge/version-1.46.0-blue)](CHANGELOG.md)

> **Minimum supported version: 1.9.0**

## Full Documentation

* [Documentation Index](docs/index.md)

## Getting Started

* [Beginner Guide](docs/beginner/index.md)

## Overview

MeteoHub S3 is a PlatformIO project for ESP32-S3 centered around an OLED dashboard (SH1106/SSD1306 using U8g2). It displays indoor sensor data, outdoor metrics received over ESP-NOW from the remote probe, weather forecasts, system logs, and device status through both the OLED interface and the integrated web dashboard.

## Hardware Requirements

Two builds are supported, one PlatformIO environment each:

| | `esp32-s3-oled` (original) | `esp32-s3-supermini` (simplified) |
|---|---|---|
| Board | ESP32-S3 DevKitC-1 N16R8 (16 MB / 8 MB PSRAM) | ESP32-S3 Super Mini (4 MB / 2 MB PSRAM), same as the probe |
| Display | 1.3" SH1106 OLED (I2C) | 0.96" JMD0.96D-1 SSD1306 OLED (I2C) |
| Controls | HW-040 rotary encoder + Back and Confirm buttons | one push button (short = next, long = menu / confirm) |
| Sensors | AHT20 + BMP280 | AHT20 + BMP280 |
| SD card | SPI module with card detect | SPI module (card detect optional) |

Optional SD card for long-term data storage (recommended: FAT32, 4-32 GB).
Wiring: [docs/hardware_wiring.md](docs/hardware_wiring.md).

## Building the Project

* Install PlatformIO in Visual Studio Code
* Select your board's environment: `esp32-s3-oled` or `esp32-s3-supermini` (first flash of the Super Mini over USB: its 4 MB partition table cannot be applied by OTA)
* Build:

```bash
platformio run
```

* Upload:

```bash
platformio run --target upload
```

## Key Features (v1.9.x Highlights)

* **Compact binary history storage**
  Symmetric indoor/outdoor layout: per-day binary files under `/history/indoor/YYYY/MM/…` and `/history/outdoor/YYYY/MM/…`, with a file header for forward compatibility, plus daily `.stats` files. Direct record access, far smaller than CSV. CSV is export-only. A full wipe is available from the System page (`POST /api/history/clear`).

* **Fast history display (v1.8.0)**
  Sequential block reads (no per-record seek) and an SD SPI frequency ladder (20/10/4/1 MHz, verified by a write+read-back test) dramatically speed up history loading, with a safe fallback.

* **History page (view-only)**
  Source (Outdoor / Indoor / **both** on one time axis) and period (24h / 48h / 7d / 30d / today / custom range); the time axis adapts to the chosen period. Optional per-metric synthesis line, an inverted "Zoom" scale control, and a real-time toggle. Deep analysis lives in morfAnalytics.

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
