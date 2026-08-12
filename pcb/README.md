# WiFeeder actuator carrier (hybrid, spin 1)

KiCad **6.0** project (matches this machine): [`wifeeder-actuator.kicad_pro`](wifeeder-actuator.kicad_pro)

Replaces the Fritzing breadboard, PCA9685 breakout, Mini360, and jumper harness.
Firmware pins are unchanged — see [`../stm32/Core/Inc/board.h`](../stm32/Core/Inc/board.h) and
[`../wiring/CONNECTOR_MAP.md`](../wiring/CONNECTOR_MAP.md).

| On this PCB | Plug-in modules | Off-board |
|-------------|-----------------|-----------|
| PCA9685 + passives | NUCLEO-L432KC | Raspberry Pi + NRF |
| Fixed 5 V + 3.3 V bucks | IBT-2 (BTS7960) | PN01007BRKT + GTS06 |
| Enc / I2C pull-ups, fuse, TVS, P-FET | NRF24 + AM1117 (2×4) | 12 V pack |
| Screw / JST connectors | Mini360 **DNP fallback only** | — |

**Do not** chip-down STM32, BTS7960, or nRF24 on this spin.

## Open / regenerate

```bash
# Open in KiCad 6.0 (installed as /usr/bin/kicad)
kicad pcb/wifeeder-actuator.kicad_pro

# After open: PCB editor → B (fill zones) → Inspect → Design Rules Checker
# Schematic: Inspect → Electrical Rules Checker

# Regenerate schematic + placement (no copper — does NOT route)
python3 pcb/tools/generate_project.py
python3 pcb/tools/route_pcb.py      # power + signal routing
python3 pcb/tools/verify_pcb.py     # KiCad connectivity (unconnected must be 0)
python3 pcb/tools/export_fab.py     # zone fill + gerbers
python3 pcb/tools/audit_pcb.py      # pad-to-trace check
```

Fritzing remains the **no-PCB bench** pack under [`../wiring/`](../wiring/).

## Power (read before first apply)

1. No modules seated. Apply 12 V to **TB1**. Measure **TP5V ≈ 5.00 V**, **TP3V3 ≈ 3.30 V**.
2. Seat Nucleo with **USB unplugged** (carrier feeds **3V3 → Nucleo 3V3**).
3. Seat NRF adapter: **VCC is 5 V** (not 3.3 V).
4. Seat IBT-2. Wire **TB2** (12V_SAFE / GND) to IBT **B+ / B−** with short 16 AWG (module has no underside power pins). Motor: IBT **M+/M−** and/or **TB3**.
5. Encoder on **TB4** (5 V VCC, 3.3 V pull-ups on A/B).

Mini360 (`J_MINI`) is **do not populate** unless the AP63203 is omitted and `SJ_MINI` is closed.

## Sheets

| File | Contents |
|------|----------|
| `wifeeder-actuator.kicad_sch` | Root / hierarchy |
| `power.kicad_sch` | 12 V protect, 5 V / 3.3 V bucks, Mini360 DNP |
| `mcu.kicad_sch` | Nucleo-32 sockets, NRF, encoder, RFID/HX711 DNP |
| `pca.kicad_sch` | PCA9685 chip-down |
| `ibt.kicad_sch` | IBT-2 logic + power screws |
| `connectors.kicad_sch` | Mechanical notes / mounting |

Pinouts: [`CONNECTORS.md`](CONNECTORS.md).

## Fab

- 2-layer, 1.6 mm, 2 oz Cu preferred, 100×150 mm, ENIG or HASL
- Gerbers: [`fab/`](fab/)
- Keepout: no 12 V under NRF antenna (top-right)

## Bring-up

1. Rails only (above)
2. Nucleo LED blink
3. PCA ACK `0x40`
4. [`../tests/04-motor`](../tests/04-motor/)
5. NRF tests 10 then 09, CH **76**, antennas, ≥10 cm
6. Encoder **2400** counts/rev
