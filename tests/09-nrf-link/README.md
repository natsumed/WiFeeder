# Test 09 — STM32 ↔ Raspberry Pi NRF24 link

## Purpose

End-to-end radio test: **Pi sends `PING`, STM32 replies `PONG`**.

| Side | Role |
|------|------|
| STM32 (`stm32/`) | RX peer — listen forever, reply to each PING |
| Pi (`pi/`) | TX client — send N PINGs, count PONGs |

Radio params (both sides) — production ESB:
- Channel **76** (`0x4C`) — matches `PROTOCOL.md` / prod firmware
- Address **`0xE7E7E7E7E7`**
- **1 Mbps**, max power (`RF_SETUP=0x07`), **8-byte** payload (link test)
- **Auto-ACK on** (`TX_DS` ⇒ peer heard us; `MAX_RT` ⇒ no RF)

Lab note: some SI24R1 clones behave better on channel **64**; use `sweep` only for diagnosis — production default remains **76**.

SPI-OK does not imply RF-OK. Start at **≥10 cm** with antennas attached.

## Bring-up checklist (before claiming PASS)

- [ ] **VCC = 3.3 V** on STM and Pi (Buck / Pi pin 1). **Never** Nucleo or Pi 5 V on module VCC — that is ~4.8 V and permanently kills PA/LNA while SPI still looks OK.
- [ ] Antennas attached on both modules
- [ ] Modules ≥10 cm apart; shared GND if co-located supplies
- [ ] Test **10** carrier/RPD sees energy on CH 76
- [ ] Then test **09** PING/PONG
- [ ] Both sides RF_CH readback = 76

## Wiring

### STM32 (same pins as test 03)

| NRF | Nucleo-32 label | MCU |
|-----|-----------------|-----|
| **VCC** | **3.3 V** (Buck / BB X) | Never Nucleo **5V** (~4.8 V) |
| GND | GND | — |
| CE | **D3** | PB0 |
| CSN | **A3** | PA4 |
| SCK | A4 | PA5 |
| MOSI | A6 | PA7 |
| MISO | D6 | PB1 |

### Pi 2 Model B (physical pins)

| NRF | Header pin |
|-----|------------|
| **VCC** | **1 (3.3 V)** — never pin 2/4 (5 V) on module VCC |
| GND | 6 |
| MOSI | 19 |
| MISO | 21 |
| SCK | 23 |
| CSN | 24 (`/dev/spidev0.0`) |
| CE | **22** (BCM GPIO25) |

Need `/dev/spidev0.0` (disable MCP2515 CAN overlay — see `tests/08-nrf-pi/README.md`).

**Do not** put 5 V on module VCC. SPI can still look fine (`STATUS=0x0E`) after the PA/LNA is already dead.
## 1. Build & flash STM32

```bash
cd tests/09-nrf-link/stm32
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/home/luceor/Desktop/wifeeder/cmake/toolchain-arm-none-eabi.cmake
cmake --build .

openocd -f interface/stlink.cfg -f target/stm32l4x.cfg \
  -c "init" -c "program test_nrf_link_stm.elf verify reset exit"
```

STM32 LED: 3 short blinks at boot, then a short blink on each answered PING.

### OpenOCD probe cells (after ~1 s run)

| Address | Meaning |
|---------|---------|
| `0x20000000` | `0xA55A0009` listening; `0xDEAD0001` SPI/init fail |
| `0x20000004` | `(CONFIG<<16)\|(RF_SETUP<<8)\|RF_CH` — expect RF_CH **76** (`0x4C`) |
| `0x20000008` | RX-loop counter (should climb) |
| `0x2000000C` | `pings \| (pongs<<8) \| (rxany<<16)` |

```bash
openocd -f interface/stlink.cfg -f target/stm32l4x.cfg \
  -c "init" -c "halt" -c "mdw 0x20000000 4" -c "resume" -c "shutdown"
```

## 2. Build & run Pi

```bash
scp -r tests/09-nrf-link/pi pi@<ip>:~/09-nrf-link-pi
# on Pi:
cd ~/09-nrf-link-pi
g++ -std=c++17 -O2 -Wall -o test_nrf_link test_nrf_link.cpp
sudo ./test_nrf_link          # 10 rounds
sudo ./test_nrf_link 20       # 20 rounds
sudo ./test_nrf_link sweep    # hunt channel if crystal skew suspected
```

Each PING prints `[ACK TX_DS]` (STM heard the PING) or `[MAX_RT no peer]` (no RF to STM).

## Pass / fail

| Output | Meaning |
|--------|---------|
| `PASS: STM32 <-> Pi NRF link OK` | ACK + PONG |
| `PARTIAL` + `ACK` | RF OK, PONG path flaky |
| `MAX_RT` every time | No RF at STM — see below |
| Local `FAIL: local NRF config mismatch` | Fix Pi wiring (test 08) |
| STM magic `0xDEAD0001` | Fix STM SPI (test 03) |

### If every PING is `MAX_RT` (lab symptom with PA+LNA adapters)

STM probe should look like:
- `0x20000000` = `0xA55A0009` (listening)
- `0x20000004` = `0x000F00xx` (CONFIG RX + RF_SETUP + CH)
- `0x2000000C` = `0` → **no packets / no RPD** (RF not arriving)

1. **VCC = 3.3 V** on *both* sides. Nucleo/Pi **5 V on VCC** destroys PA/LNA (SPI still OK).
2. **Separate modules ~0.5–1 m** (PA+LNA at 5 cm can saturate even at PA_MIN).
3. Antennas screwed on (you already have this).
4. Optional: 10–100 µF across adapter VCC–GND.
## Nucleo ↔ Nucleo (no Pi / no UART)

| Role | ELF | LED at boot |
|------|-----|-------------|
| RX peer | `test_nrf_link_rx.elf` | 3 short blinks; blink on each PING |
| TX client | `test_nrf_link_tx.elf` | 2 long blinks; solid LED = all PONGs OK |

Flash RX first (leave that board powered), then flash TX on the other:

```bash
openocd -f interface/stlink.cfg -f target/stm32l4x.cfg \
  -c "init" -c "program test_nrf_link_rx.elf verify reset exit"
# swap USB to the other Nucleo (or use a 2nd cable)
openocd -f interface/stlink.cfg -f target/stm32l4x.cfg \
  -c "init" -c "program test_nrf_link_tx.elf verify reset exit"
# read TX result
openocd -f interface/stlink.cfg -f target/stm32l4x.cfg \
  -c "init" -c "halt" -c "mdw 0x20000000 4" -c "resume" -c "shutdown"
```

TX magic: `0xA55A00FF` PASS, `0xA55A00FE` partial, `0xA55A00F0` fail.  
CE on **D3** for both (not hard-tied to 3.3V). **VCC = 3.3 V**.
