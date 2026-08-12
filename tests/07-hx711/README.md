# Test 07 — HX711 load cell

## Purpose

Verify bit-bang read of 24-bit HX711 data on **PB4/PB5** (not PC0/PC1 — those pins do not exist on L432KC).

## Wiring

| HX711 | NUCLEO | Notes |
|-------|--------|-------|
| DOUT | **PB4** | Input |
| SCK | **PB5** | Output |
| VCC / GND | 3.3V / GND | |
| E+ / E− / A+ / A− | Load cell | Per cell wiring |

## Expected behavior

- Polls HX711 when DOUT goes low.
- On significant weight change (>5000 counts), LED blinks **1–15** times based on magnitude.

## Build

```bash
cd tests/07-hx711
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/home/luceor/Desktop/wifeeder/cmake/toolchain-arm-none-eabi.cmake
cmake --build .
```

## Pass criteria

Apply load to cell; LED blink count changes with weight steps.
