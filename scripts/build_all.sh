#!/usr/bin/env bash
# Build all WiFeeder v2 firmware artifacts that do not need hardware.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
V1_TC="${WIFEEDER_TOOLCHAIN:-/home/luceor/Desktop/wifeeder/cmake/toolchain-arm-none-eabi.cmake}"
IMAGE="${WIFEEDER_DOCKER_IMAGE:-wifeeder-dev}"

echo "== Raspberry Pi host (native stub) =="
cmake -S "$ROOT/raspberry" -B "$ROOT/raspberry/build" -DWIFEEDER_BUILD_TESTS=ON
cmake --build "$ROOT/raspberry/build" -j"$(nproc)"
"$ROOT/raspberry/build/test_diet"

echo "== STM32 production + hardware tests (Docker) =="
docker run --rm --user root \
  -v "$ROOT:/workspace" \
  -v "$(dirname "$V1_TC")/..:/wifeeder" \
  "$IMAGE" bash -c "
set -e
export PATH=/opt/arm-gcc/bin:\$PATH
TC=/wifeeder/cmake/toolchain-arm-none-eabi.cmake
cd /workspace/stm32
mkdir -p build_prod && cd build_prod
cmake .. -DCMAKE_TOOLCHAIN_FILE=\$TC
cmake --build . -j\$(nproc)
for t in 01-led 02-uart 03-nrf 04-motor 05-encoder 06-rfid 07-hx711; do
  cd /workspace/tests/\$t
  mkdir -p build && cd build
  cmake .. -DCMAKE_TOOLCHAIN_FILE=\$TC
  cmake --build . -j\$(nproc)
done
"

echo "== Pi NRF tools (native) =="
cmake -S "$ROOT/tests/08-nrf-pi" -B "$ROOT/tests/08-nrf-pi/build"
cmake --build "$ROOT/tests/08-nrf-pi/build" -j"$(nproc)"

echo "All software builds completed. Flash .bin files when hardware arrives."
