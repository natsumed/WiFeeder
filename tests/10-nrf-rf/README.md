# Test 10 — NRF RF energy (Nucleo ↔ Nucleo)

## Purpose

Diagnose **air energy**, not packet decode. PING/PONG (test 09) can fail for many reasons; this only asks: *does the RX module see 2.4 GHz power on-channel?*

| Role | ELF | Behavior |
|------|-----|----------|
| Carrier | `test_nrf_rf_carrier.elf` | CONT_WAVE + continuous payload blasts, CE held high |
| RPD | `test_nrf_rf_rpd.elf` | RX mode, sample reg `0x09` (RPD) for ~4 s |

Same pins / **3.3 V VCC** as test 09. Channel **76**, `RF_SETUP=0x07`.

## Checklist before run

- [ ] Both modules **VCC = 3.3 V** (never Nucleo 5V)
- [ ] Antennas on
- [ ] ≥10 cm spacing
- [ ] Shared ground if using separate supplies
- [ ] Carrier CE high (`IDR` bit set)
- [ ] RPD hits climb when carrier runs

## Probe cells (`mdw 0x20000000 4`)

### Carrier

| Addr | Meaning |
|------|---------|
| `[0]` | `0xA55A0010` OK · `0xDEAD0001` SPI · `0xDEAD0002` **CE stuck low** |
| `[1]` | `(CONFIG<<16)\|(SETUP<<8)\|CH` |
| `[2]` | CE pin IDR (must be `1`) |
| `[3]` | blast count (climbing) |

### RPD listener

| Addr | Meaning |
|------|---------|
| `[0]` | `0xA55A0011` sampling · `0xA55A00FF` **energy seen** · `0xA55A00F0` none |
| `[1]` | radio snapshot |
| `[2]` | samples |
| `[3]` | `rpd_hits \| (rx_dr<<16)` |

## Interpret

| Result | Likely cause |
|--------|----------------|
| Either side `DEAD0002` | CE not wired to D3/PB0 (SPI can still work) |
| Carrier OK, RPD `F0` both directions | No RF — check 3.3 V, antenna, or already-dead PA (prior 5 V abuse) |
| RPD `FF` one way only | One module TX or RX path bad — swap modules |
| RPD `FF` both ways | Energy OK → return to test 09 (address/CRC/timing) |
