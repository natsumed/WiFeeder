#!/usr/bin/env python3
"""Export fabrication gerbers and drill from wifeeder-actuator.kicad_pcb."""

from __future__ import annotations

import sys
from pathlib import Path

import pcbnew

ROOT = Path(__file__).resolve().parents[1]
PCB_PATH = ROOT / "wifeeder-actuator.kicad_pcb"
FAB_DIR = ROOT / "fab"
PREFIX = "wifeeder-actuator"

# pcbnew prepends project name; plot stem should be the layer suffix only
LAYER_EXPORTS = [
    (pcbnew.F_Cu, "F_Cu", ".gtl"),
    (pcbnew.B_Cu, "B_Cu", ".gbl"),
    (pcbnew.F_SilkS, "F_Silkscreen", ".gto"),
    (pcbnew.B_SilkS, "B_Silkscreen", ".gbo"),
    (pcbnew.F_Mask, "F_Mask", ".gts"),
    (pcbnew.B_Mask, "B_Mask", ".gbs"),
    (pcbnew.Edge_Cuts, "Edge_Cuts", ".gm1"),
]


def fill_zones(board: pcbnew.BOARD) -> None:
    zones = board.Zones()
    for zone in zones:
        zone.SetNeedRefill(True)
    pcbnew.ZONE_FILLER(board).Fill(zones)


def export_gerbers(board: pcbnew.BOARD, out_dir: Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    plot = pcbnew.PLOT_CONTROLLER(board)
    opts = plot.GetPlotOptions()
    opts.SetOutputDirectory(str(out_dir))
    opts.SetPlotFrameRef(False)
    opts.SetUseGerberAttributes(True)

    for layer, stem, suffix in LAYER_EXPORTS:
        plot.SetLayer(layer)
        plot.OpenPlotfile(stem, pcbnew.PLOT_FORMAT_GERBER, stem)
        plot.PlotLayer()
        plot.ClosePlot()
        src = out_dir / f"{PREFIX}-{stem}.gbr"
        dst = out_dir / f"{PREFIX}-{stem}{suffix}"
        if not src.exists():
            raise FileNotFoundError(f"expected gerber {src}")
        if dst.exists():
            dst.unlink()
        src.rename(dst)


def export_drill(board: pcbnew.BOARD, out_dir: Path) -> None:
    writer = pcbnew.EXCELLON_WRITER(board)
    writer.SetMapFileFormat(False)
    writer.CreateDrillandMapFilesSet(str(out_dir), True, False)
    mapping = {
        f"{PREFIX}-PTH.drl": f"{PREFIX}.drl",
        f"{PREFIX}-NPTH.drl": f"{PREFIX}-NPTH.drl",
    }
    for src_name, dst_name in mapping.items():
        src = out_dir / src_name
        dst = out_dir / dst_name
        if src.exists():
            if dst.exists() and dst != src:
                dst.unlink()
            if dst != src:
                src.rename(dst)


def main() -> int:
    board = pcbnew.LoadBoard(str(PCB_PATH))
    fill_zones(board)
    board.Save(str(PCB_PATH))

    for stale in FAB_DIR.glob(f"{PREFIX}-wifeeder-*"):
        stale.unlink()
    for stale in FAB_DIR.glob("*.gbr"):
        stale.unlink()

    export_gerbers(board, FAB_DIR)
    try:
        export_drill(board, FAB_DIR)
    except Exception as exc:
        print(f"WARNING: drill export failed: {exc}", file=sys.stderr)

    print(f"Exported gerbers to {FAB_DIR}")
    for path in sorted(FAB_DIR.glob(f"{PREFIX}*")):
        if path.suffix in {".gtl", ".gbl", ".gto", ".gbo", ".gts", ".gbs", ".gm1", ".drl"}:
            print(f"  {path.name} ({path.stat().st_size} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
