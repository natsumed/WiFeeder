#!/usr/bin/env python3
"""Quick KiCad connectivity check for wifeeder-actuator.kicad_pcb."""

import sys
from pathlib import Path

import pcbnew

PCB = Path(__file__).resolve().parents[1] / "wifeeder-actuator.kicad_pcb"


def main() -> int:
    board = pcbnew.LoadBoard(str(PCB))
    board.BuildConnectivity()
    uc = board.GetConnectivity().GetUnconnectedCount()
    tracks = len(list(board.GetTracks()))
    print(f"{PCB.name}: unconnected={uc} tracks+vias={tracks}")
    return 0 if uc == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
