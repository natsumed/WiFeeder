# Fab outputs (wifeeder-actuator)

Exported by `pcb/tools/export_fab.py` from KiCad 6.0 pcbnew (zone-filled copper).

| File | Layer |
|------|-------|
| `wifeeder-actuator-F_Cu.gtl` | Front copper |
| `wifeeder-actuator-B_Cu.gbl` | Back copper |
| `wifeeder-actuator-F_Mask.gts` | Front mask |
| `wifeeder-actuator-B_Mask.gbs` | Back mask |
| `wifeeder-actuator-F_Silkscreen.gto` | Front silk |
| `wifeeder-actuator-B_Silkscreen.gbo` | Back silk |
| `wifeeder-actuator-Edge_Cuts.gm1` | Outline 150×100 mm |
| `wifeeder-actuator.drl` | Excellon drill |

**Workflow:** `route_pcb.py` → `export_fab.py`. Re-run after any layout change.

Before ordering: open in KiCad → Inspect → Design Rules Checker (clearance ≥ 0.2 mm).

Stackup: 2-layer, 1.6 mm, 2 oz Cu if quoted, ENIG or HASL, min track 0.2 mm.
