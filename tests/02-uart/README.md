# Test 02 — UART debug

## Purpose

Verify serial output. Uses **USART1 TX on PA9** at **115200 8N1** with **HSI 16 MHz** (register-level, no HAL).

Production debug uses **LPUART1 on PA2** (ST-Link VCP). This test uses an external USB–serial adapter on **PA9** for a clear loopback-free check.

## Wiring

| Signal | NUCLEO pin | Arduino | Adapter |
|--------|------------|---------|---------|
| USART1 TX | **PA9** | D8* | RX (white) |
| GND | GND | — | GND |

\*On NUCLEO-L432KC Arduino silkscreen, **D9 = PA8**; PA9 is the adjacent pin used here as USART1 TX.

Optional: onboard LED **PB3** blinks briefly each message.

## Expected output

Every **5 seconds**:

```
=== NUCLEO-L432KC UART OK ===
```

## Serial terminal

```bash
sudo chmod 666 /dev/ttyUSB0   # or ttyACM0 if using PA2/LPUART in other tests
stty -F /dev/ttyUSB0 115200 raw -echo
cat /dev/ttyUSB0
```

## Build

```bash
cd tests/02-uart
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/home/luceor/Desktop/wifeeder/cmake/toolchain-arm-none-eabi.cmake
cmake --build .
```

## Pass criteria

Repeating banner on serial monitor at 115200 baud.
