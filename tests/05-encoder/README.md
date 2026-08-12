# Test 05 — Encoder (TIM2)

## Purpose

Verify quadrature encoder interface on Encoder 1 (**600 P/R**, TIM2 ×4).

## Wiring

Factory 4-wire pigtail (no separate GX-4). Pull-ups 4.7 kΩ A/B → 3V3 on the breadboard.

Pigtail is **2× red + 2× black** — identify nets by molded pin + meter ([`wiring/CONNECTOR_MAP.md`](../../wiring/CONNECTOR_MAP.md)), then:

| Encoder net | NUCLEO / rail | Timer |
|-------------|---------------|-------|
| A | **PA0** (A0) via BB col 40 | TIM2_CH1 |
| B | **PA1** (A1) via BB col 41 | TIM2_CH2 |
| VCC | BB **Z** 5 V | — |
| GND | BB **Y** | — |

## Expected behavior

Rotate the encoder shaft: each count change causes a short **PB3** LED blink. TIM2 free-runs as 16-bit counter.

## Build

```bash
cd tests/05-encoder
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/home/luceor/Desktop/wifeeder/cmake/toolchain-arm-none-eabi.cmake
cmake --build .
```

## Pass criteria

LED blinks when A/B quadrature edges occur; counter increments/decrements with rotation direction.
