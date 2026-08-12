# Test 06 — 134 kHz RFID reader

## Purpose

Verify USART2 RX and parse 27-byte ASCII frames from the v1 134 kHz reader protocol.

## Frame format

27-byte ASCII: `$A0112OKD` + 15-digit decimal tag + 2-digit XOR checksum (hex nibbles).

Checksum: start with `'A'`, XOR bytes `[2..23]`, compare to hex digits at `[24..25]`.

Ported from v1 `MCU/Drivers/RfId/Src/rfid_134khz.c`.

## Wiring

| Reader | NUCLEO | UART |
|--------|--------|------|
| TX (data out) | **PA3** (USART2_RX) | 9600 8N1 |
| GND | GND | |
| VCC | Per reader spec | Often 5V |

## Expected behavior

On valid tag read:
1. Short LED blinks encode tag ID bits (MSB first).
2. Three long blinks (500 ms) confirm successful parse.

## Build

```bash
cd tests/06-rfid
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/home/luceor/Desktop/wifeeder/cmake/toolchain-arm-none-eabi.cmake
cmake --build .
```

## Pass criteria

Present a known tag; LED pattern repeats for each valid frame with correct checksum.
