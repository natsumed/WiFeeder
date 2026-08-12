# Actuator carrier connectors (spin 1)

Electrical nets match [`../stm32/Core/Inc/board.h`](../stm32/Core/Inc/board.h).  
This table **replaces breadboard columns** in [`../wiring/CONNECTOR_MAP.md`](../wiring/CONNECTOR_MAP.md) when the PCB is used.

## Mechanical lock

GX aviation series was **not measured**. Spin 1 uses **5.08 mm screw terminals**. Panel GX-12/16 plugs fly-wire ≤ 20 mm to TB3 / TB4. IBT-2 **B+/B−/M+/M−** stay on the module screws; TB2/TB3 are adjacent 16 AWG landing points.

IBT keepout: **50×50 mm** + stock heatsink. Nucleo USB overhangs the **top** edge (~15 mm). NRF antenna keepout: no 12 V / pour under SMA (`Dwgs.User` box).

---

## TB1 — 12 V in

| Pin | Net |
|-----|-----|
| 1 | 12V_IN (pre-fuse) |
| 2 | GND |

## TB2 — to IBT B+/B−

| Pin | Net |
|-----|-----|
| 1 | 12V_SAFE (after fuse + P-FET) |
| 2 | GND |

## TB3 — motor / GX-2 fly-wire

| Pin | Net | GX-2 |
|-----|-----|------|
| 1 | MOTOR_P | pin 1 M+ |
| 2 | MOTOR_N | pin 2 M− |

Tie to IBT **M+/M−** with short 16 AWG (or use IBT screws as the only motor connection).

## TB4 — encoder 4-wire pigtail

Factory encoder cable + plug. Land the four flying leads here (no separate GX-4 + encoder body).

| Pin | Net | Pigtail |
|-----|-----|---------|
| 1 | 5V | VCC after meter ID (photo pin colors are 2× red + 2× black) |
| 2 | GND | GND after meter ID |
| 3 | ENC_A (PA0) + 4.7 kΩ to 3V3 | A after meter ID |
| 4 | ENC_B (PA1) + 4.7 kΩ to 3V3 | B after meter ID |

## J_NRF — AM1117 adapter 2×4 (VCC = **5 V**)

| Pin | NRF | MCU |
|-----|-----|-----|
| 1 | GND | GND |
| 2 | VCC | **5V** |
| 3 | CE | PB0 / D3 |
| 4 | CSN | PA4 / A3 |
| 5 | SCK | PA5 / A4 |
| 6 | MOSI | PA7 / A6 |
| 7 | MISO | PB1 / D6 |
| 8 | IRQ | nc |

## J_IBT — logic (2.54 mm)

| Pin | IBT | Net |
|-----|-----|-----|
| 1 | RPWM | PCA PWM0 |
| 2 | LPWM | PCA PWM1 |
| 3 | R_EN | 3V3 |
| 4 | L_EN | 3V3 |
| 5 | R_IS | nc |
| 6 | L_IS | nc |
| 7 | VCC | 5V |
| 8 | GND | GND |

## J_NUC_L / J_NUC_R — Nucleo-32 (Nano 15+15, 0.6")

CN3 (left, USB at top): D13, **3V3**, AREF, A0, A1, A2/PA3 RFID, A3 CSN, A4 SCK, **A5/PA6 NC**, A6 MOSI, A7, 5V (open), NRST, GND, VIN (open).

CN4 (right): D1, D0, NRST, GND, D2, D3 CE, D4 SDA, D5 SCL, D6 MISO, D7, D8, D9, D10, D11 HX711 SCK, D12 HX711 DOUT.

## J_RFID / J_HX711 — JST-XH-4 (DNP)

| J_RFID | Net | J_HX711 | Net |
|--------|-----|---------|-----|
| 1 VCC | 5V (or 3V3 via SJ_RFID_V) | 1 | 3V3 |
| 2 GND | GND | 2 | GND |
| 3 RX | PA3 | 3 DOUT | PB4 |
| 4 NC | — | 4 SCK | PB5 |

## Test points

TP1 `12V_SAFE` · TP2 `5V` · TP3 `3V3` · TP4 `GND`
