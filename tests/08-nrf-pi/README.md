# Test 08 — NRF24L01+ on Raspberry Pi 2 Model B

## Purpose

Pi-side SPI bring-up. Code uses **BCM** GPIO numbers (`/dev/spidev0.0` + sysfs **GPIO25**).

## Warning: ignore WiringPi / Pi4J pinout images

Many Google diagrams (including Pi4J) label pins as `GPIO 25` meaning **WiringPi pin 25**, **not** Broadcom BCM 25.

| Scheme | “GPIO 25” means | Physical pin |
|--------|-----------------|--------------|
| **BCM** (Linux, our code, `raspi-gpio`) | BCM 25 | **22** |
| **WiringPi / Pi4J** (that Google image) | WiringPi 25 | **37** (= BCM 26!) |

Same trap on SPI labels in that image (`GPIO 12 MOSI` = WiringPi 12 = physical 19 = **BCM 10**).

**Always wire by physical header pin number (1–40).** Those never change.

## Wiring (physical pins only) — Pi 2 Model B 40-pin

```
NRF pin     →  Pi header pin     (BCM, for reference)
────────────────────────────────────────────────────
VCC         →  pin 1 (3.3V)      NEVER pin 2/4 — 5V kills PA/LNA
GND         →  pin 6  (GND)
MOSI        →  pin 19            (BCM 10)
MISO        →  pin 21            (BCM 9)
SCK         →  pin 23            (BCM 11)
CSN         →  pin 24            (BCM 8 = SPI0 CE0 → /dev/spidev0.0)
CE          →  pin 22            (BCM 25)   ← NOT pin 37
IRQ         →  leave open
```

| NRF | Header pin | Notes |
|-----|------------|--------|
| VCC | **1** | **3.3 V** — never 5 V on module VCC |
| GND | **6** | |
| MOSI | **19** | |
| MISO | **21** | |
| SCK | **23** | |
| CSN | **24** | must be `spidev0.0` |
| CE | **22** | BCM 25 — **not** pin 37 |

Pin **37** is BCM **26** — do not use for CE with this test.

## Problem we hit: only `/dev/spidev0.1`

v1 **MCP2515 CAN** overlay stole SPI CE0 and GPIO25. Comment out in `/boot/config.txt`:

```
# dtoverlay=mcp2515-can0,...
# dtoverlay=spi-bcm2835-overlay
```

Keep `dtparam=spi=on`, then `sudo reboot`. Confirm:

```bash
ls /dev/spidev0.0
```

## Get code / build / run

```bash
# on PC
scp -r wifeeder-v2/tests/08-nrf-pi pi@<ip>:~/

# on Pi
cd ~/08-nrf-pi
mkdir -p build
g++ -std=c++17 -O2 -Wall -o build/test_nrf_tx test_nrf_tx.cpp
g++ -std=c++17 -O2 -Wall -o build/test_nrf_rx test_nrf_rx.cpp
sudo ./build/test_nrf_tx
```

### Pass / fail

| Output | Meaning |
|--------|---------|
| `PASS: NRF SPI OK` + `RF_CH=0x4C` | Good |
| `STATUS=0xFF` | MISO open / wrong wire |
| `STATUS=0x00` `RF_CH=0x00` | No chip response — check 3.3V, pins 19/21/23/24, CE on **22** |
