# WiFeeder v2 — Connector & pin map (MVP + PCA9685)

Authoritative wiring for [`wifeeder-v2.fzz`](wifeeder-v2.fzz).  
Per-block sketches: [`blocks/`](blocks/).

**MVP scope:** one **PN01007BRKT** motor, one factory-cabled **GTS06** encoder (4 flying leads), **PCA9685** PWM driver, NRF link. Motor2 / Enc2 / HX711 / RFID deferred.

**Bench layout:** the main `.fzz` **and every block** use the **same** breadboard map (rails X/W/Y/Z + columns below). Open one block at a time to wire that island.

---

## Hard rule: one jumper per Nucleo pin

Nucleo header pins accept **exactly one** physical jumper. Do not stack wires on 3V3, 5V, GND, A0, or A1.

| Nucleo pin | Single connection |
|------------|-------------------|
| 3V3 | → Breadboard **X** (Nucleo **USB** supplies 3.3 V — do **not** inject Mini360) |
| GND (analog `1001`) | ← Breadboard **W** (GND) |
| GND (digital `1103`) | → NRF GND only |
| 5V | → Breadboard **Z** only (IBT / encoder — **not** NRF) |
| D5 / D4 | → BB cols **30** / **31** (then PCA SCL/SDA) |
| D3 / A3 / A4 / A6 / D6 | → BB cols **45–49** (then NRF SPI) |
| A0 / A1 | ← BB cols **40** / **41** (Enc A/B + pull-ups) |

**Daisy hubs:** breadboard rails (not stacked on Nucleo). Battery → IBT B± stays **direct** (high current).

### What went wrong (2026-08-11) — Mini360 + our old map

The Mini360 is an **adjustable** buck. The pot shipped at ~**10 V**. Our old sketch treated OUT as “3V3” and wired **OUT → rail X → Nucleo 3V3** and **IBT R_EN/L_EN**. That put ~10 V on the STM32 3.3 V rail (smoke) and on the IBT logic pins (74HC244 crater).

**Wiring fault (ours):** using Mini360 as the MCU 3.3 V source with no measure-before-connect.  
**Not a broken Mini360:** set the pot to **3.30 V** and tape it. Do **not** reconnect OUT to X until then.

**New bench rule:** Nucleo **USB** supplies the logic rails. Mini360 **IN** may sit on the battery so you can set the pot; **OUT+ stays off X**.

**Bring-up** (new Nucleo + new IBT; USB first, battery last):

1. Mini360 **alone**: 12 V on IN, meter on OUT. Set **3.30 V**, tape the pot. Leave OUT disconnected.
2. Nucleo **USB only**. Meter: **3V3 = 3.3 V**, **5V ≈ 4.8 V**.
3. **Nucleo 3V3 → X**, **5V → Z**, **GND → W**, **W↔Y**. Meter X and Z before anything else.
4. IBT **VCC** (silk) → **Z**, **GND** → **W**. Meter the IBT VCC pin. **B+ still open.**
5. PCA VCC → **X** (right header, not V+/C2). EN → **X**. Encoder on Z/Y/40/41.
6. **Last:** fused battery → IBT **B±** only.

If **X** or **Z** is ever > 5.5 V, kill power. Do not reuse the smoked Nucleo or the holed IBT.

---

## Breadboard hub map (main + every `blocks/0N-*.fzz`)

Fritzing **Generic Bajillion Hole** part:

| Rail / column | Net |
|---------------|-----|
| **X** | 3V3 (**from Nucleo 3V3 / USB**) — NRF VCC, PCA, EN, pull-ups |
| **W** | GND (from Nucleo GND) |
| **Y** | GND (jumpered to W) |
| **Z** | 5V (from Nucleo 5V once) — IBT logic + encoder **only** |
| Col **30** | I2C SCL (Nucleo D5 ↔ PCA SCL) |
| Col **31** | I2C SDA (Nucleo D4 ↔ PCA SDA) |
| Col **33** | PWM0 → IBT RPWM |
| Col **34** | PWM1 → IBT LPWM |
| Col **40** | Enc A + pull-up + Nucleo A0 |
| Col **41** | Enc B + pull-up + Nucleo A1 |
| Col **45** | NRF CE |
| Col **46** | NRF CSN |
| Col **47** | NRF SCK |
| Col **48** | NRF MOSI |
| Col **49** | NRF MISO |

Pi ↔ NRF is a **separate** breadboard (`06-nrf-pi.fzz`) with the **same column numbers** (45–49). Pi pin 1 feeds rail **X**; never pin 2/4.

---

## Why PCA9685

Direct TIM PWM on the Nucleo crowded the headers. **PCA9685** drives IBT-2 RPWM/LPWM so the MCU only needs **SCL + SDA**.

Use the **right logic header** only. Never wire the C2 electrolytic pads (`connector7` / `connector8`).

| Nucleo | MCU | via BB | PCA9685 (right header) |
|--------|-----|--------|------------------------|
| **D5** | **PB6** I2C1_SCL | col 30 | SCL `connector46` |
| **D4** | **PB7** I2C1_SDA | col 31 | SDA `connector45` |
| — | — | rail X | VCC `connector44` |
| — | — | rail W | GND `connector48` |
| — | — | rail W | **OE `connector47`** |

PCA9685 address: default `0x40` (A0–A5 open).

| PCA9685 ch | via BB | → IBT-2 |
|------------|--------|---------|
| **PWM0** | col 33 | RPWM (M1) |
| **PWM1** | col 34 | LPWM (M1) |

IBT-2 **R_EN** and **L_EN** tied to breadboard **3V3 (X)** (not Nucleo 3V3).

---

## Motor GX-2 (IBT-2 M+/M− → PN01007BRKT)

| GX pin | Net |
|--------|-----|
| 1 | M+ |
| 2 | M− |

## Encoder — one 4-wire pigtail (already on the encoder)

The bench encoder is **not** a separate GTS06 body plus a loose GX-4. It arrives as one assembly: encoder + cable + plug, with **four flying leads** on the mating pigtail (photo 2026-08-11).

**Do not use wire color to pick VCC / GND / A / B.** The pigtail is **two red + two black** hookup wires. Identify by the **molded pin numbers** on the plug (solder-cup side in the photo), then confirm with a meter.

| Molded pin (photo) | Pigtail color | Function | Breadboard |
|--------------------|---------------|----------|------------|
| **1** | Black | **measure** (not color) | — |
| **2** | Red | **measure** | — |
| **3** | Red | **measure** | — |
| **4** | Black | **measure** | — |

After the meter check, land:

| Net | Breadboard |
|-----|------------|
| VCC | rail **Z** (5 V) |
| GND | rail **Y** |
| Phase A | col **40** → **PA0 / A0** |
| Phase B | col **41** → **PA1 / A1** |

**Identify (do this once, then label the four wires):**

1. **Best:** unplug the pigtail. On the encoder-side cable the factory GTS06 colors are usually **red = VCC**, **black = GND**, **green = A**, **white = B** (some sellers swap green/white). Continuity from each pigtail pin to those colors. Write the map on the plug.
2. **Only the 4 leads:** mark them P1–P4 from the molded numbers. Find VCC–GND with a DMM (lowest resistance pair, often a few hundred Ω to a few kΩ, polarity-sensitive). Confirm by applying **5 V through a 470 Ω–1 kΩ series resistor** — the encoder should draw ~10–30 mA. If current is ~0, wrong pair; if >50 mA, disconnect. The other two wires are A and B: 4.7 kΩ to 3V3 each, rotate the shaft, both must toggle. Swap A/B if TIM2 counts the wrong way.

**NPN OC:** 4.7 kΩ pull-ups on BB cols **40/41** → rail **X** (3V3). Do not stack a second jumper on Nucleo A0/A1. There is **no** fifth “C / index” wire.

---

## NUCLEO-L432KC (MVP)

| Label | MCU | Destination |
|-------|-----|-------------|
| **5V** | — | BB **Z** only (IBT / Enc). **Do not** feed NRF |
| GND | — | BB **W** only |
| GND (D-side) | — | NRF GND only |
| **3V3** | — | → BB **X** (Nucleo USB is the source). NRF VCC from **X** |
| D5 | PB6 | BB col 30 → PCA **SCL** |
| D4 | PB7 | BB col 31 → PCA **SDA** |
| D3 | PB0 | BB col 45 → NRF CE |
| A3 | PA4 | BB col 46 → NRF CSN |
| A4 | PA5 | BB col 47 → NRF SCK |
| A6 | PA7 | BB col 48 → NRF MOSI |
| D6 | PB1 | BB col 49 → NRF MISO |
| A0 / A1 | PA0 / PA1 | BB cols 40 / 41 (Enc) |
| **A5** | **PA6** | **Do not use** (shorted) |

Freed vs old map: **PA8/PA9** no longer drive the motor (PWM is on PCA9685).

---

## IBT-2 (single channel / one BTS7960 half of dual board)

| IBT-2 | Connects to |
|-------|-------------|
| B+ / B− | 12 V battery (**direct**, not via BB) |
| R_EN, L_EN | BB **X** (3.3 V) |
| RPWM / LPWM | BB cols **33** / **34** ← PCA PWM0/1 |
| VCC | BB **Z** (5V) |
| GND | BB **W** |
| M+ / M− | GX-2 → motor |
| 5V OUT | unused in BB layout (encoder uses rail Z) |

---

## Raspberry Pi ↔ NRF (unchanged)

| NRF | Pi |
|-----|-----|
| VCC | pin **1 (3.3 V)** — never pin 2/4 5 V on module VCC |
| GND | 6 |
| SCK/MOSI/MISO/CSN/CE | 23 / 19 / 21 / 24 / 22 (GPIO25) |

---

## Power (breadboard hubs — not Nucleo stacking)

1. Nucleo **USB** on. **3V3 → X**, **5V → Z**, **GND → W**; jumper **W↔Y**  
2. Battery → IBT **B±** only (high current off the BB). Mini360 **IN** may share that battery so you can set the pot.  
3. **Mini360 OUT+ is not on X** until a meter reads **3.30 V** (then you may jumper OUT→X and unplug USB for battery-only 3.3 V)  
4. BB **X** → PCA VCC, IBT EN, pull-ups, NRF VCC  
5. BB **W/Y** → PCA GND/OE, IBT GND, encoder GND  
6. BB **Z** → IBT VCC / Enc VCC (**not** NRF)

### Hard rule: NRF VCC = 3.3 V

Nucleo **5V** is ~4.8 V. nRF24L01+ abs max is **3.6 V**. 5 V on module VCC kills PA/LNA permanently; SPI still reads `STATUS=0x0E` (see Aug 2026 bench report). Do not use 5 V “because AM1117” unless you have **measured 3.3 V on the chip VCC pin**.
