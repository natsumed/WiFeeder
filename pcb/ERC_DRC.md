# ERC / DRC notes (spin 1)

KiCad is not required to *edit* these files, but **zone fill + official DRC** should be run in KiCad 8 before ordering.

## Intentional (exclude in ERC if flagged)

| Item | Why |
|------|-----|
| PA6_NC no-connect | Nucleo PA6 shorted to GND on the module — do not route |
| J_NRF pin 8 (IRQ) nc | Firmware does not use IRQ |
| LED2–15 of PCA9685 unconnected | No Motor2; pads exist for probing |
| J_MINI / SJ_MINI open | Discrete bucks stuffed; Mini360 is fallback |
| NUC_5V, NUC_VIN unconnected by default | Carrier injects **3V3** only; USB unplugged |
| IBT_RIS / IBT_LIS nc | Current-sense unused |
| MOTOR_P/N not tied to IBT pads | IBT M+/M− are module screws; TB3 is GX fly-wire |

## Must pass

- Clearance ≥ 0.2 mm (power ≥ 0.4 mm)
- 12V_SAFE pour does not enter NRF keepout (118,2)–(149,28)
- Nucleo 3V3 and GND each have exactly the intended sockets
- NRF VCC net is **5V**, never 3V3

## Mechanical lock (GX)

Chassis GX-12 / GX-16 series was **not measured** on the bench. Spin 1 uses **5.08 mm Phoenix-style screws** (TB3 motor, TB4 encoder). Panel aviation plugs fly-wire ≤ 20 mm. Revisit footprints if you measure GX12-2 / GX16-4.
