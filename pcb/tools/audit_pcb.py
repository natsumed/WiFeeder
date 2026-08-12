#!/usr/bin/env python3
"""Audit wifeeder-actuator.kicad_pcb connectivity and routing quality."""

from __future__ import annotations

import argparse
import math
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_PCB = ROOT / "wifeeder-actuator.kicad_pcb"

POWER_NETS = {
    "GND",
    "12V_IN",
    "12V_SAFE",
    "5V",
    "3V3",
    "NUC_5V",
    "NUC_VIN",
    "MOTOR_P",
    "MOTOR_N",
    "PA6_NC",
    "IBT_RIS",
    "IBT_LIS",
}
OPTIONAL_NETS = {"RFID_RX", "HX711_DOUT", "HX711_SCK"}
# Nucleo GPIO not used by firmware on spin 1 — no copper required
UNUSED_GPIO = {
    "PA9", "PA10", "PA11", "PA12", "PA8", "PC14", "PC15",
    "D13_PB3", "AREF", "PA2_VCP", "NUC_NRST",
}


def parse_pcb_text(path: Path) -> dict:
    pcb = path.read_text()
    nets = {int(n): name for n, name in re.findall(r'\(net (\d+) "([^"]*)"\)', pcb)}

    pads: list[tuple] = []
    for block in re.split(r"(?=\(footprint )", pcb):
        if not block.startswith("(footprint"):
            continue
        ref_m = re.search(r'reference "([^"]+)"', block)
        at_m = re.search(r"\(at ([\d.-]+) ([\d.-]+)", block)
        if not ref_m or not at_m:
            continue
        ref = ref_m.group(1)
        fx, fy = float(at_m.group(1)), float(at_m.group(2))
        for pm in re.finditer(r'\(pad "([^"]*)"([\s\S]*?\))\s*\n', block):
            num = pm.group(1)
            body = pm.group(2)
            at_m = re.search(r"\(at ([\d.-]+) ([\d.-]+)(?: ([\d.-]+))?\)", body)
            net_m = re.search(r'\(net (\d+)(?: "([^"]*)")?\)', body)
            if not at_m:
                continue
            px, py = float(at_m.group(1)), float(at_m.group(2))
            rot = float(at_m.group(3) or 0)
            net_id = int(net_m.group(1)) if net_m else 0
            r = math.radians(rot)
            x = fx + px * math.cos(r) - py * math.sin(r)
            y = fy + px * math.sin(r) + py * math.cos(r)
            pads.append((ref, num, nets.get(net_id, ""), x, y, net_id))

    segs: list[tuple] = []
    for m in re.finditer(
        r'\(segment \(start ([\d.-]+) ([\d.-]+)\) \(end ([\d.-]+) ([\d.-]+)\)[^\n]*\(net (\d+)\)',
        pcb,
    ):
        x1, y1, x2, y2, net_id = (
            float(m.group(1)),
            float(m.group(2)),
            float(m.group(3)),
            float(m.group(4)),
            int(m.group(5)),
        )
        segs.append(((x1, y1), (x2, y2), net_id))
        if abs(x1 - x2) < 0.01 and abs(y1 - y2) < 0.01:
            print(f"WARNING: zero-length segment at ({x1},{y1}) net {nets.get(net_id, net_id)}")

    vias = len(re.findall(r"\(via ", pcb))
    return {"nets": nets, "pads": pads, "segs": segs, "vias": vias}


def dist_point_segment(px: float, py: float, x1: float, y1: float, x2: float, y2: float) -> float:
    dx, dy = x2 - x1, y2 - y1
    if abs(dx) < 1e-9 and abs(dy) < 1e-9:
        return math.hypot(px - x1, py - y1)
    t = max(0.0, min(1.0, ((px - x1) * dx + (py - y1) * dy) / (dx * dx + dy * dy)))
    return math.hypot(px - (x1 + t * dx), py - (y1 + t * dy))


def audit(path: Path, tol: float = 0.8) -> int:
    data = parse_pcb_text(path)
    nets, pads, segs = data["nets"], data["pads"], data["segs"]
    pads_by_net: dict[int, list] = defaultdict(list)
    for ref, num, net, x, y, net_id in pads:
        pads_by_net[net_id].append((ref, num, net, x, y))

    print(f"PCB: {path}")
    print(f"  segments: {len(segs)}  vias: {data['vias']}  pads: {len(pads)}")
    print(f"  nets by segment count:")
    for net_id, cnt in Counter(s[2] for s in segs).most_common(12):
        print(f"    {nets.get(net_id, net_id)}: {cnt}")

    unrouted = []
    for ref, num, net, x, y, net_id in pads:
        if not net or net in POWER_NETS or net in OPTIONAL_NETS or net in UNUSED_GPIO:
            continue
        ok = False
        for (x1, y1), (x2, y2), sn in segs:
            if sn != net_id:
                continue
            if dist_point_segment(x, y, x1, y1, x2, y2) <= tol:
                ok = True
                break
        if not ok:
            unrouted.append((ref, num, net, round(x, 2), round(y, 2)))

    print(f"\n  signal pads not within {tol} mm of routed trace: {len(unrouted)}")
    for item in sorted(unrouted):
        print(f"    {item[0]:10} pad {item[1]:>2} {item[2]:12} @ ({item[3]}, {item[4]})")

    return 0 if not unrouted else 1


def main() -> int:
    parser = argparse.ArgumentParser(description="Audit PCB routing connectivity")
    parser.add_argument("pcb", nargs="?", default=str(DEFAULT_PCB))
    parser.add_argument("--tol", type=float, default=0.8, help="pad-to-trace tolerance mm")
    args = parser.parse_args()
    return audit(Path(args.pcb), args.tol)


if __name__ == "__main__":
    sys.exit(main())
