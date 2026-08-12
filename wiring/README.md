# WiFeeder v2 — Fritzing wiring pack

**Production (less wiring):** hybrid actuator PCB in [`../pcb/`](../pcb/). This Fritzing pack is the **bench / no-PCB** layout.

Main sketch: **[`wifeeder-v2.fzz`](wifeeder-v2.fzz)**  
**Per-block sketches:** [`blocks/`](blocks/) (power, PCA9685, motor, encoder, NRF STM, NRF Pi)  
Pin tables: **[`CONNECTOR_MAP.md`](CONNECTOR_MAP.md)**  
Preview exports: [`exports/wifeeder-v2-breadboard.png`](exports/wifeeder-v2-breadboard.png), [`exports/wifeeder-v2-schematic.png`](exports/wifeeder-v2-schematic.png)

Covers STM32 actuator (Nucleo + PCA9685 + IBT-2 + one motor + one 4-wire encoder pigtail + NRF) and Raspberry Pi host NRF, matching [`../HARDWARE.md`](../HARDWARE.md).

## Install Fritzing (this Linux machine)

**System package (installed):**

```bash
fritzing          # /usr/bin/fritzing  — also in the app menu
```

Packages: `fritzing`, `fritzing-data`, `fritzing-parts` (apt, 0.9.6).

Optional newer binary (not required): `~/Applications/fritzing/Fritzing` from the CD-548 release tarball.

## Open the sketch

1. Open `wiring/wifeeder-v2.fzz` for the full MVP, or a file under `wiring/blocks/` for one island.
2. **Main sketch (bench):** **USB Nucleo** feeds **X=3V3** and **Z=5V**. Mini360 **OUT is off X** until a meter shows 3.30 V (unset pot ≈ 10 V killed the first Nucleo + IBT 74HC244). **Never** put Nucleo/Pi 5 V on NRF VCC.
3. **MVP modules:** PCA9685 (I2C D4/D5) → IBT-2 → **one** motor; **one** encoder; NRF STM + Pi. Battery → IBT B± stays direct.
4. Rebuild all sketches: `python3 wiring/tools/build_wired_sketch.py`
5. Previews: [`exports/wifeeder-v2-breadboard.png`](exports/wifeeder-v2-breadboard.png)

Details: [`CONNECTOR_MAP.md`](CONNECTOR_MAP.md), block index: [`blocks/README.md`](blocks/README.md).

## Rebuild parts / exports

```bash
# Needs Nucleo SVG tree at /tmp/Fritzing-components (or re-clone Blacksocks/Fritzing-components)
# and Fritzing core parts under ~/Applications/fritzing/fritzing-parts
python3 wiring/tools/build_wiring_pack.py
```

## Legend (see also `blocks/`)

**Rule:** one jumper per Nucleo pin; daisy via the breadboard (same X/W/Y/Z + columns on **every** block). PCA right header only (not C2).

| Island | Sketch | Contents |
|--------|--------|----------|
| Power | `blocks/01-power.fzz` | 12 V battery, Mini360 → Nucleo 3V3/GND |
| PWM | `blocks/02-pca9685.fzz` | Nucleo D4/D5 ↔ PCA9685; power/OE from Buck |
| Drive | `blocks/03-motor.fzz` | IBT-2, GX-2 → PN01007BRKT |
| Sense | `blocks/04-encoder.fzz` | 4-wire encoder pigtail; pull-ups on cols 40/41 |
| STM RF | `blocks/05-nrf-stm.fzz` | Nucleo + NRF (**VCC=3.3 V**) |
| Pi RF | `blocks/06-nrf-pi.fzz` | Raspberry Pi + NRF (**VCC=3.3 V pin 1**) |

## Verification checklist

- [ ] Continuity: motor GX-2; encoder **red/black/green/white** ↔ Z / Y / col 40 / col 41
- [ ] NRF VCC ≈ **3.3 V** on STM and Pi (not 5 V)
- [ ] Encoder A/B idle high (~3.3 V) with pull-ups; toggle when shaft turns
- [ ] No wire on **PA6**
