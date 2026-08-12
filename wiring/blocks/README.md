# WiFeeder v2 — wiring blocks

Each island has its **own breadboard** using the **same rails/columns** as [`../wifeeder-v2.fzz`](../wifeeder-v2.fzz) / [`../CONNECTOR_MAP.md`](../CONNECTOR_MAP.md). Wire one block at a time.

**NRF VCC = 3.3 V** (rail **X**). Never Nucleo/Pi 5 V.

```bash
python3 wiring/tools/build_wired_sketch.py
```

| File | Block | Breadboard use |
|------|-------|----------------|
| [`01-power.fzz`](01-power.fzz) | Power | USB Nucleo → **X/Z**; Mini360 IN only (OUT off X) |
| [`02-pca9685.fzz`](02-pca9685.fzz) | PWM | X/W → PCA; cols **30/31** I2C |
| [`03-motor.fzz`](03-motor.fzz) | Motor | **Z** → IBT VCC; X → EN; cols **33/34** PWM; BAT→B± direct |
| [`04-encoder.fzz`](04-encoder.fzz) | Encoder | 4-wire pigtail: red→**Z**, black→**Y**, green/white→**40/41** + pull-ups to X |
| [`05-nrf-stm.fzz`](05-nrf-stm.fzz) | RF STM | **X** → NRF VCC; cols **45–49** SPI |
| [`06-nrf-pi.fzz`](06-nrf-pi.fzz) | RF Pi | Pi pin1 → **X**; same cols **45–49** |

PCA: right header VCC/`44` GND/`48` — never C2 pads `7`/`8`.
