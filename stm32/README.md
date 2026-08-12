# STM32 firmware (NUCLEO-L432KC)

Production firmware entry: `Core/Src/main.c` → `wifeeder_init()` / `wifeeder_poll()`.

## Drivers

| Module | Role |
|--------|------|
| `nrf24l01` | Bit-bang SPI (PA4 CSN, PB1 MISO) |
| `protocol` / `crc8` | 32-byte NRF packets |
| `rfid` | USART2 RX PA3, `$A0112OKD` frames |
| `motor` | IBT-2 TIM1/TIM16 PWM |
| `encoder` | TIM2 PA0/PA1 |
| `hx711` | PB4/PB5 |
| `flash_int` | Device ID (RAM + flash read) |
| `wifeeder` | Feed loop (STATUS / RFID / FEED_CMD / FEED_DONE) |

FreeRTOS sources are vendored under `Middlewares/Third_Party/FreeRTOS/` (task split later).

## Build

```bash
docker run --rm --user root \
  -v "$PWD:/workspace" -v /home/luceor/Desktop/wifeeder:/wifeeder \
  wifeeder-dev bash -c '
  export PATH=/opt/arm-gcc/bin:$PATH
  cd /workspace/stm32 && mkdir -p build_prod && cd build_prod
  cmake .. -DCMAKE_TOOLCHAIN_FILE=/wifeeder/cmake/toolchain-arm-none-eabi.cmake
  cmake --build . -j$(nproc)
'

# Flash
cp build_prod/wifeeder_v2_mcu.bin /media/$USER/NODE_L432KC/
```

Hardware bring-up tests: see [`../tests/README.md`](../tests/README.md).
