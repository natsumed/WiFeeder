# WiFeeder v2 — Datasheet / product links

Store PDFs here if desired. Prefer links to avoid large binaries in git.

| Part | Notes | Link |
|------|-------|------|
| PN01007BRKT | 50 RPM 12 V DC gear motor with bracket | search vendor for `PN01007BRKT` |
| GTS06-OC-RA600A-2M | 600 P/R NPN OC, 6 mm shaft; factory cable often red/black/green/white; **bench pigtail is 2× red + 2× black** — ID by pin + meter | [GTS family datasheet (Radiomag)](https://www.radiomag.com.de/datasheets/gts-ab-a-english.pdf) |
| IBT-2 / BTS7960B | Dual half-bridge motor driver module | Infineon BTS7960 datasheet |
| NRF24L01+ PA+LNA + AM1117 | Adapter **VCC = 5 V** | Nordic nRF24L01+ PS |
| NUCLEO-L432KC | STM32L432KC | ST UM1956 |
| Mini360 | Adjustable buck — set **3.3 V** out before connecting MCU | module silk |
| HX711 | Load-cell ADC | Avia Semiconductor HX711 |
| RFID 125 kHz UART | EM4100-style `$A0112…` frames | module docs |

Encoder color codes differ by seller — always continuity-check against [`../CONNECTOR_MAP.md`](../CONNECTOR_MAP.md).
