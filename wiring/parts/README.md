# Fritzing parts for WiFeeder v2

Installed by `python3 wiring/tools/build_wired_sketch.py` into `~/Documents/Fritzing/parts/user/`.

## Real graphics

| Part | Source |
|------|--------|
| `Adafruit_PCA9685` | [Adafruit Fritzing Library](https://github.com/adafruit/Fritzing-Library) |
| `IBT2_BTS7960_real` | Fritzing forum BTS7960/IBT-2 |
| `STM32_Nucleo-32_board` | Blacksocks Nucleo-32 |
| NRF24, Pi 2, gear-motor, LM2596, resistor, battery | Fritzing core |

## Labeled boxes

GX-2 motor plug (no faithful community part). Encoder is **`GTS06_encoder_wifeeder`** only — four pins VCC/GND/A/B for the factory pigtail. Do not also place GX-4.

## MVP nets

PCA9685 I2C (D4/D5) → PWM0/1 → IBT-2 → 1× motor; 1× encoder; NRF STM + Pi.  
See [`../CONNECTOR_MAP.md`](../CONNECTOR_MAP.md).
