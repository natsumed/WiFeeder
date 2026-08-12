# WiFeeder v2 — Hardware Test Suite

Standalone bring-up programs for **NUCLEO-L432KC** (STM32L432KC) and **Raspberry Pi** NRF link. Each STM32 test is self-contained bare-metal code (register-level where possible) with its own `CMakeLists.txt`.

Authoritative pin map: [`wiring/CONNECTOR_MAP.md`](../wiring/CONNECTOR_MAP.md) · production firmware: [`stm32/Core/Inc/board.h`](../stm32/Core/Inc/board.h).

## Test index

| # | Directory | Target | Purpose |
|---|-----------|--------|---------|
| 01 | `01-led/` | `test_led.elf` | PB3 onboard LED blink |
| 02 | `02-uart/` | `test_uart.elf` | USART1 PA9 @ 115200 |
| 03 | `03-nrf/` | `test_nrf.elf` | NRF24 SPI (CSN=**PA4**, CE=PB0) |
| 04 | `04-motor/` | `test_motor.elf` | PCA9685 I2C → IBT-2 (PWM0/1) |
| 05 | `05-encoder/` | `test_encoder.elf` | TIM2 encoder PA0/PA1 |
| 06 | `06-rfid/` | `test_rfid.elf` | USART2 PA3 RFID `$A0112OKD` |
| 07 | `07-hx711/` | `test_hx711.elf` | HX711 bit-bang PB4/PB5 |
| 08 | `08-nrf-pi/` | `test_nrf_tx` / `test_nrf_rx` | Pi spidev + GPIO25 CE |
| 09 | `09-nrf-link/` | `test_nrf_link_stm` + `test_nrf_link` | STM32 ↔ Pi PING/PONG (**CH 76**) |
| 10 | `10-nrf-rf/` | `test_nrf_rf_carrier` / `test_nrf_rf_rpd` | RF energy (CONT_WAVE / RPD) |

Future unit tests (diet GTest ports) live under `tests/unit/`.

## Locked pin map (MVP production)

| Function | Pin | Notes |
|----------|-----|-------|
| LED | **PB3** | Nucleo LD3 |
| Debug UART | **PA2** | LPUART1 → ST-Link VCP |
| NRF SCK / MOSI / MISO | PA5 / PA7 / **PB1** | Bit-bang SPI; never PA6 |
| NRF CSN / CE | **PA4** / PB0 | **VCC=3.3 V** — never Nucleo 5V |
| PCA9685 SCL / SDA | **PB6** / **PB7** | I2C bit-bang (D5/D4) |
| Motor1 | PCA **PWM0/PWM1** → IBT RPWM/LPWM | EN→3.3V; OE→GND |
| Encoder 1 A/B | **PA0** / **PA1** | TIM2 ×4; 4-wire pigtail (green/white); **600 P/R** |
| RFID RX | **PA3** | USART2 — deferred in MVP wiring |
| HX711 DOUT / SCK | **PB4** / **PB5** | Deferred in MVP wiring |

**Board notes:** PA6 shorted — never use as MISO. One jumper per Nucleo pin (power daisy from Buck/NRF). Motor2 TIM16 on PB6/PB7 is **retired** for MVP.

## Shared build infrastructure

- `cmake/stm32_test.cmake` — CPU flags, `STM32L486xx`, startup, linker, `.bin` post-build
- `common/test_utils.c` — LED PB3 helpers, `delay_ms`, `blink_pattern`
- Startup: `stm32/startup/startup_stm32l486xx.s`
- Linker: `stm32/linker/STM32L432KC_FLASH.ld`

## Build an STM32 test

```bash
cd tests/04-motor
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/home/luceor/Desktop/wifeeder/cmake/toolchain-arm-none-eabi.cmake
cmake --build . -j$(nproc)
```

Outputs: `test_motor.elf`, `test_motor.bin`.

## Flash

**Mass storage:** copy `.bin` to `NODE_L432KC` drive.

**OpenOCD (Docker):**

```bash
sg docker -c 'docker run --rm --user root --privileged \
  -v /dev/bus/usb:/dev/bus/usb -v /home/luceor/Desktop/wifeeder-v2:/workspace \
  wifeeder-dev bash -c "
  openocd -f interface/stlink.cfg -f target/stm32l4x.cfg \
    -c \"init\" -c \"program /workspace/tests/04-motor/build/test_motor.elf verify reset exit\"
"'
```

## Serial debug

```bash
# LPUART1 PA2 (ST-Link VCP) — typically /dev/ttyACM0
stty -F /dev/ttyACM0 115200 raw -echo
cat /dev/ttyACM0
```

Test 02 uses **USART1 TX on PA9** with an external USB–serial adapter instead.

## Recommended bench order

1. **01-led** — power & clock
2. **02-uart** — serial sanity
3. **03-nrf** — SPI register R/W (**VCC=3.3 V**)
4. **10-nrf-rf** then **09-nrf-link** — energy then PING/PONG (**CH 76**)
5. **04-motor** (PCA9685) → **05-encoder**
6. **06-rfid** / **07-hx711** when those peripherals are wired

## Phase gate

Component tests PASS on hardware before claiming E2E feed. Production MCU is a bare-metal superloop in `stm32/Core/Src/wifeeder.c` (FreeRTOS later).
