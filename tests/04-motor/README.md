# Test 04 — IBT-2 motor PWM

## Purpose

Verify Motor1 PWM and direction GPIO on the IBT-2 H-bridge driver.

## Prerequisites

- **Disconnect NRF CSN from PA8** or rewire CSN to **PA4** before this test (PA8 is Motor1 PWM).
- Tie both enable pins to 3.3V (no spare EN GPIO on L432 MVP).

## Wiring — IBT-2 Motor 1

| IBT-2 pin | NUCLEO | Function |
|-----------|--------|----------|
| RPWM | **PA8** | TIM1_CH1 PWM @ **10 kHz** |
| LPWM | **PA9** | Direction GPIO |
| R_EN | **3.3V** | Tied high |
| L_EN | **3.3V** | Tied high |
| VCC | 12V (bench supply) | Motor power |
| GND | GND | Common ground |
| M+ / M− | Motor | |

Clock: **HSI 16 MHz**. TIM1: **PSC=15**, **ARR=99** → 10 kHz PWM. Duty ~70% during each 2 s phase.

## Expected behavior

- Motor runs ~2 s one direction, ~2 s reversed (direction toggles on PA9).
- LED **PB3** double-blinks at each direction change.

## Build

```bash
cd tests/04-motor
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/home/luceor/Desktop/wifeeder/cmake/toolchain-arm-none-eabi.cmake
cmake --build .
```

## Pass criteria

Audible/visible motor motion with direction reversal; no runaway if EN tied correctly.
