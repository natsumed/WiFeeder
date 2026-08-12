# Contributing to WiFeeder

## Branch model

| Branch | Purpose |
|--------|---------|
| `main` | Always releasable / documented stable baseline |
| `develop` | Day-to-day integration |
| `feature/*`, `fix/*`, `chore/*`, `pcb/*`, `docs/*` | Short-lived work branches |

Promotion path:

1. Branch from `develop`
2. Open a pull request into `develop`
3. When a slice is stable, open a PR from `develop` into `main`
4. Tag releases on `main` as `vX.Y.Z` (create `release/*` only when cutting a numbered release)

## Pull requests

- Use the PR template (summary, hardware impact, test plan).
- Keep PRs focused; prefer squash merge for feature work.
- CI must pass (`host` + `stm32` jobs). Do not flash hardware in CI.
- Call out NRF **3.3 V** power changes explicitly — Nucleo 5 V damages PA/LNA modules.

## Local checks

```bash
# Host (native stub, no Pi SPI required)
cmake -S raspberry -B raspberry/build -DRASPBERRY_PI=OFF -DWIFEEDER_BUILD_TESTS=ON
cmake --build raspberry/build -j"$(nproc)"

# STM32 firmware
cmake -S stm32 -B stm32/build \
  -DCMAKE_TOOLCHAIN_FILE="$(pwd)/cmake/toolchain-arm-none-eabi.cmake"
cmake --build stm32/build -j"$(nproc)"
```

## Commit style

Prefer short, imperative subjects that explain *why* (e.g. “Fix NRF CE GPIO init on L432”). Group unrelated changes into separate commits or PRs.

## Repository monitoring

Watch the Actions and Security tabs for CI failures and Dependabot alerts.
