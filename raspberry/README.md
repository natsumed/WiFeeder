# WiFeeder v2 — Raspberry Pi Host

Host controller for WiFeeder v2: NRF24L01+ link to STM32 actuators, diet calculation, JSON database, and MQTT uplink.

Station serial number: **WCN-A100-0001X**

## Features

- 32-byte NRF protocol (`0x01`–`0x08`) with CRC-8 poly `0x07` (matches STM32)
- `nrf_link` API (init/start/stop/sendto/recv) modeled on v1 `walink`
- LINEAR_FORCED diet engine ported from v1 `diet.cpp`
- JSON file database (`wifeeder-data.json`) with mutex
- MQTT via libmosquittopp (optional stub without it)
- Desktop Linux build uses NRF stub (logs packets, no SPI)

## Dependencies

**Required**

- CMake ≥ 3.16, C++17 compiler, pthread

**Optional**

- `jsoncpp` — used when found; otherwise embedded `min_json`
- `libmosquittopp` — real MQTT; otherwise stub logs to stdout
- `libgtest-dev` — GoogleTest runner for `test_diet`

On Raspberry Pi OS:

```bash
sudo apt update
sudo apt install -y cmake g++ libjsoncpp-dev libmosquittopp-dev
```

## Build

### Desktop (stub NRF, diet unit tests)

```bash
cd raspberry
cmake -B build -DWIFEEDER_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

### Raspberry Pi (SPI + GPIO)

Enable SPI and reboot:

```bash
sudo raspi-config   # Interface Options → SPI → Enable
# or:
echo dtparam=spi=on | sudo tee -a /boot/config.txt
sudo reboot
```

Build with Pi defines:

```bash
cd raspberry
cmake -B build -DRASPBERRY_PI=ON
cmake --build build
```

## Run

Copy seed data (first run):

```bash
cp config/wifeeder-data.json ./wifeeder-data.json
# or set WIFEEDER_DATA=/etc/wifeeder-data.json
```

Start host:

```bash
./build/wifeeder_host
```

Environment variables:

| Variable | Default | Description |
|----------|---------|-------------|
| `WIFEEDER_DATA` | `./wifeeder-data.json` | Animal/diet JSON database path |

## Wiring (NRF24L01+ → Raspberry Pi)

| NRF pin | Pi connection |
|---------|----------------|
| VCC | 3.3V (use adapter with regulator) |
| GND | GND |
| CE | GPIO25 (BCM) |
| CSN | SPI0 CE0 |
| SCK | SCLK (pin 23) |
| MOSI | MOSI (pin 19) |
| MISO | MISO (pin 21) |
| IRQ | not connected |

SPI device: `/dev/spidev0.0`

Radio settings match STM32 and `PROTOCOL.md`:

- Channel 76 (2.476 GHz)
- 1 Mbps, 32-byte payload
- Address `0xE7E7E7E7E7`

## Thread model

1. **NRF RX thread** (`nrf_link`) — receives packets, auto-ACK
2. **Feed thread** — `RFID_TAG` → diet `get_portion` → `FEED_CMD`; `FEED_DONE` → log + MQTT
3. **Status thread** — `STATUS` heartbeat + 5 min offline detection
4. **Daily reset thread** — ~23:50 daily_reset for all animals

## Message flow

See [PROTOCOL.md](../PROTOCOL.md) and [docs/wifeeder-v2-final-plan.md](../docs/wifeeder-v2-final-plan.md).

```
STM32 --RFID_TAG(0x02)--> Host --lookup DB--> diet.get_portion()
Host --FEED_CMD(0x03)--> STM32 --dispense--> FEED_DONE(0x04) --> Host log + MQTT
STM32 --STATUS(0x01)--> Host (every 5s heartbeat)
```

## Unit tests

```bash
./build/test_diet
```

Tests cover portion calculation, grams↔revs conversion (16 rev calibration reference), and `daily_reset` without hardware.

## License

Portions derived from WiFeeder v1 (CodinTek / Mohamed HNEZLI).
