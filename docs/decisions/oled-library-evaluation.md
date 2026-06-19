# OLED library decision: migration completed to U8g2

Minimum valid version: 1.0.155

## Status
This decision is now **closed**: OLED rendering uses **U8g2** as the active backend.

## Why this note changed
The previous evaluation compared SH1106/SSD1306Wire vs U8g2 and recommended postponing migration.
Runtime feedback and stability observations validated U8g2 for this project (fewer artifacts during page transitions and fewer random ghost pixels).

## Current baseline
- OLED backend: U8g2.
- Supported controllers through configuration: SH1106 and SSD1306.
- I2C address remains configurable (`0x3C` / `0x3D`) from `include/config.h`.

## Follow-up
Future work is tracked in:
- `docs/todo.md`
- `docs/todo.md`
