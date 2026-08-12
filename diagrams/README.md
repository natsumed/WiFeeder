# WiFeeder v2 — Mermaid Diagrams

This directory contains Mermaid `.mmd` files extracted from the project documentation, plus **PLAN-*** diagrams from the implementation plan.

## How to View

### Method 1 — VS Code (Recommended)

1. Open any `.mmd` file
2. VS Code will prompt to install the Mermaid extension — accept it
3. Or press `Ctrl+K V` to open the Mermaid Preview

### Method 2 — Mermaid Live Editor

1. Go to https://mermaid.live/
2. Paste the content of any `.mmd` file
3. See the diagram rendered instantly

### Method 3 — CLI (requires Node.js)

```bash
npm install -g @mermaid-js/mermaid-cli
mmdc -i diagram.mmd -o diagram.png -w 1200
```

## File Index

### Implementation plan (authoritative for pins / port)

| File | Source | Content |
|------|--------|---------|
| `PLAN-01-system-context.mmd` | docs/wifeeder-v2-final-plan.md | Cloud ↔ Pi ↔ STM32 |
| `PLAN-02-phase-roadmap.mmd` | docs/wifeeder-v2-final-plan.md | Phase gates |
| `PLAN-03-pinmap-l432kc.mmd` | docs/wifeeder-v2-final-plan.md | Locked L432KC pin map |
| `PLAN-04-feed-sequence.mmd` | docs/wifeeder-v2-final-plan.md | RFID → diet → feed → MQTT |
| `PLAN-05-freertos-tasks.mmd` | docs/wifeeder-v2-final-plan.md | Radio / Wifeeder / Cmd / Iwdg |
| `PLAN-06-v1-port-matrix.mmd` | docs/wifeeder-v2-final-plan.md | Port / replace / drop from v1 |
| `PLAN-07-protocol-map.mmd` | docs/wifeeder-v2-final-plan.md | v1 CAN types → v2 NRF types |

### Extracted from docs

| File | Source | Content |
|------|--------|---------|
| `ARCHITECTURE-01.mmd` … `09` | ARCHITECTURE.md | System overview, flows, topology |
| `HARDWARE-01.mmd` … `09` | HARDWARE.md | Pin maps & wiring (**L432KC-corrected**) |
| `PROTOCOL-01.mmd` … `10` | PROTOCOL.md | Communication protocol |
| `SOFTWARE-01.mmd` … `08` | SOFTWARE.md | Code architecture |
| `TESTING-01.mmd` … `04` | TESTING.md | Test plans |
| `DEPLOYMENT-01.mmd` … `02` | DEPLOYMENT.md | Deployment flows |
| `README-01.mmd` … `02` | README.md | Project overview |
| `TROUBLESHOOTING-01.mmd` … `03` | TROUBLESHOOTING.md | Diagnostics |
| `BILL_OF_MATERIALS-01.mmd` … `03` | BILL_OF_MATERIALS.md | Hardware diagrams |

> **Pin authority:** Prefer `PLAN-03-pinmap-l432kc.mmd` and [`docs/wifeeder-v2-final-plan.md`](../docs/wifeeder-v2-final-plan.md) §2 over any older diagram that cites PB8–PB11, PC0/PC1, TIM3/TIM4, or PA6 as NRF MISO.
