# Test 03 — NRF24L01+ SPI bring-up

## Purpose

Verify the NRF24L01+ responds to bit-bang SPI. Prints results on **USART1 PA9 @ 115200**.

## Wiring (current bench)

| NRF pin | STM32 | Notes |
|---------|-------|-------|
| VCC | **3.3 V** | **Never 5 V** — Nucleo 5V (~4.8 V) destroys PA/LNA; SPI may still work |
| GND | GND | Common ground |
| CE | **PB0** | |
| CSN | **PA4** | Production pin |
| SCK | **PA5** | |
| MOSI | **PA7** | |
| MISO | **PB1** | Do not use PA6 |
| IRQ | nc | |

UART debug: **PA9** → USB-serial RX (typically `/dev/ttyUSB0` at 115200).

## LED codes

| Pattern | Meaning |
|---------|---------|
| ON 5s / OFF 1s | **PASS** — STATUS=0x0E, RF_CH=0x4C |
| ON 3s / OFF 1s | Partial — STATUS OK, RF_CH bad |
| ON 2s / OFF 2s | FAIL — no SPI (0xFF) |

## Build / flash

```bash
cd tests/03-nrf && mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/home/luceor/Desktop/wifeeder/cmake/toolchain-arm-none-eabi.cmake
cmake --build .
cp test_nrf.bin /media/$USER/NODE_L432KC/
```

Serial:

```bash
stty -F /dev/ttyUSB0 115200 raw -echo
cat /dev/ttyUSB0
```
