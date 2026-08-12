# Test 01 — LED (PB3)

## Purpose

Verify the NUCLEO-L432KC boots and GPIO works. No external wiring.

## Wiring

| Signal | NUCLEO pin | Notes |
|--------|------------|-------|
| LED LD3 | **PB3** | Onboard green LED |

## Expected behavior

Green LED blinks **2 s ON / 2 s OFF** continuously.

## Build

```bash
cd tests/01-led
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/home/luceor/Desktop/wifeeder/cmake/toolchain-arm-none-eabi.cmake
cmake --build .
```

Output: `test_led.elf`, `test_led.bin`

## Flash

Copy `test_led.bin` to the ST-Link mass-storage drive (`NODE_L432KC`) or use OpenOCD (see `tests/README.md`).

## Pass criteria

LED blinks at ~2 s period with no external connections.
