#!/usr/bin/env python3
"""Generate the WiFeeder actuator KiCad 6.0 hybrid carrier (schematic + PCB + gerbers)."""
from __future__ import annotations

import json
import math
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LIBS = ROOT / "libs"
PRETTY = LIBS / "wifeeder.pretty"
FAB = ROOT / "fab"

# Board outline (mm)
BOARD_W, BOARD_H = 150.0, 100.0


def uid() -> str:
    return str(uuid.uuid4())


# ---------------------------------------------------------------------------
# Footprints
# ---------------------------------------------------------------------------

def fp_header(name: str, descr: str) -> str:
    return f"""(footprint "{name}" (version 20211014) (generator wifeeder-gen)
  (layer "F.Cu")
  (tedit 66B8A001)
  (descr "{descr}")
  (attr through_hole)
"""


def fp_smd_header(name: str, descr: str) -> str:
    return f"""(footprint "{name}" (version 20211014) (generator wifeeder-gen)
  (layer "F.Cu")
  (tedit 66B8A001)
  (descr "{descr}")
  (attr smd)
"""


def write_footprints() -> None:
    PRETTY.mkdir(parents=True, exist_ok=True)

    # 0805 resistor
    (PRETTY / "R_0805.kicad_mod").write_text(
        fp_smd_header("R_0805", "0805 resistor")
        + """  (fp_text reference "REF**" (at 0 -1.8) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_text value "R_0805" (at 0 1.8) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_line (start -1.6 -0.8) (end 1.6 -0.8) (layer "F.SilkS") (width 0.12))
  (fp_line (start -1.6 0.8) (end 1.6 0.8) (layer "F.SilkS") (width 0.12))
  (pad "1" smd roundrect (at -0.95 0) (size 0.8 1.3) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25))
  (pad "2" smd roundrect (at 0.95 0) (size 0.8 1.3) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25))
)
"""
    )

    (PRETTY / "C_0805.kicad_mod").write_text(
        fp_smd_header("C_0805", "0805 capacitor")
        + """  (fp_text reference "REF**" (at 0 -1.8) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_text value "C_0805" (at 0 1.8) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (pad "1" smd roundrect (at -0.95 0) (size 0.8 1.3) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25))
  (pad "2" smd roundrect (at 0.95 0) (size 0.8 1.3) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25))
)
"""
    )

    (PRETTY / "C_1206.kicad_mod").write_text(
        fp_smd_header("C_1206", "1206 bulk cap")
        + """  (fp_text reference "REF**" (at 0 -2.1) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_text value "C_1206" (at 0 2.1) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (pad "1" smd roundrect (at -1.4 0) (size 1.1 1.6) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25))
  (pad "2" smd roundrect (at 1.4 0) (size 1.1 1.6) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25))
)
"""
    )

    (PRETTY / "LED_0805.kicad_mod").write_text(
        fp_smd_header("LED_0805", "0805 LED")
        + """  (fp_text reference "REF**" (at 0 -1.8) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_text value "LED" (at 0 1.8) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_poly (pts (xy -1.4 -0.9) (xy -0.6 -0.9) (xy -0.6 0.9) (xy -1.4 0.9)) (layer "F.SilkS") (fill none) (width 0.12))
  (pad "1" smd roundrect (at -0.95 0) (size 0.8 1.3) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25))
  (pad "2" smd roundrect (at 0.95 0) (size 0.8 1.3) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25))
)
"""
    )

    (PRETTY / "L_6x6.kicad_mod").write_text(
        fp_smd_header("L_6x6", "6x6mm power inductor")
        + """  (fp_text reference "REF**" (at 0 -4) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_text value "L" (at 0 4) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_rect (start -3.2 -3.2) (end 3.2 3.2) (layer "F.SilkS") (width 0.12) (fill none))
  (pad "1" smd rect (at -2.4 0) (size 1.6 4.5) (layers "F.Cu" "F.Paste" "F.Mask"))
  (pad "2" smd rect (at 2.4 0) (size 1.6 4.5) (layers "F.Cu" "F.Paste" "F.Mask"))
)
"""
    )

    (PRETTY / "SOT23-6.kicad_mod").write_text(
        fp_smd_header("SOT23-6", "SOT-23-6 AP6320x")
        + """  (fp_text reference "REF**" (at 0 -2.6) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_text value "SOT23-6" (at 0 2.6) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_line (start -1.6 -1.6) (end -0.6 -1.6) (layer "F.SilkS") (width 0.12))
  (pad "1" smd roundrect (at -1.27 -0.95) (size 0.7 0.55) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25))
  (pad "2" smd roundrect (at 0 -0.95) (size 0.7 0.55) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25))
  (pad "3" smd roundrect (at 1.27 -0.95) (size 0.7 0.55) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25))
  (pad "4" smd roundrect (at 1.27 0.95) (size 0.7 0.55) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25))
  (pad "5" smd roundrect (at 0 0.95) (size 0.7 0.55) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25))
  (pad "6" smd roundrect (at -1.27 0.95) (size 0.7 0.55) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25))
)
"""
    )

    (PRETTY / "SOIC-8.kicad_mod").write_text(
        fp_smd_header("SOIC-8", "SOIC-8 P-FET")
        + """  (fp_text reference "REF**" (at 0 -3.2) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_text value "SOIC-8" (at 0 3.2) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_rect (start -2.2 -2.7) (end 2.2 2.7) (layer "F.SilkS") (width 0.12) (fill none))
"""
        + "".join(
            f'  (pad "{i+1}" smd roundrect (at -2.7 {-1.905 + i*1.27}) (size 1.55 0.6) '
            f'(layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25))\n'
            for i in range(4)
        )
        + "".join(
            f'  (pad "{8-i}" smd roundrect (at 2.7 {-1.905 + i*1.27}) (size 1.55 0.6) '
            f'(layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25))\n'
            for i in range(4)
        )
        + ")\n"
    )

    (PRETTY / "SMA.kicad_mod").write_text(
        fp_smd_header("SMA", "SMA / DO-214AC TVS")
        + """  (fp_text reference "REF**" (at 0 -2.2) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_text value "SMA" (at 0 2.2) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (pad "1" smd rect (at -2.1 0) (size 1.8 2.2) (layers "F.Cu" "F.Paste" "F.Mask"))
  (pad "2" smd rect (at 2.1 0) (size 1.8 2.2) (layers "F.Cu" "F.Paste" "F.Mask"))
)
"""
    )

    (PRETTY / "Fuse_5x20.kicad_mod").write_text(
        fp_header("Fuse_5x20", "5x20mm fuse clips")
        + """  (fp_text reference "REF**" (at 0 -5) (layer "F.SilkS") (effects (font (size 1 1) (thickness 0.15))))
  (fp_text value "F10A" (at 0 5) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_rect (start -12 -3) (end 12 3) (layer "F.SilkS") (width 0.15) (fill none))
  (pad "1" thru_hole rect (at -10 0) (size 3 3) (drill 1.6) (layers "*.Cu" "*.Mask"))
  (pad "2" thru_hole rect (at 10 0) (size 3 3) (drill 1.6) (layers "*.Cu" "*.Mask"))
)
"""
    )

    pads_tssop = []
    for i in range(14):
        y = -4.225 + i * 0.65
        pads_tssop.append(
            f'  (pad "{i+1}" smd roundrect (at -3.1 {y:.3f}) (size 1.35 0.4) '
            f'(layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25))'
        )
        pads_tssop.append(
            f'  (pad "{28-i}" smd roundrect (at 3.1 {y:.3f}) (size 1.35 0.4) '
            f'(layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25))'
        )
    (PRETTY / "TSSOP-28.kicad_mod").write_text(
        fp_smd_header("TSSOP-28", "PCA9685 TSSOP-28 0.65mm")
        + """  (fp_text reference "REF**" (at 0 -6.2) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_text value "PCA9685" (at 0 6.2) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_rect (start -2.2 -4.9) (end 2.2 4.9) (layer "F.SilkS") (width 0.12) (fill none))
  (fp_circle (center -2.8 -4.6) (end -2.6 -4.6) (layer "F.SilkS") (width 0.12) (fill none))
"""
        + "\n".join(pads_tssop)
        + "\n)\n"
    )

    def pinrow(name: str, n: int, pitch: float = 2.54) -> None:
        pads = []
        for i in range(n):
            pads.append(
                f'  (pad "{i+1}" thru_hole {"rect" if i==0 else "circle"} '
                f'(at 0 {i*pitch:.2f}) (size 1.7 1.7) (drill 1.0) (layers "*.Cu" "*.Mask"))'
            )
        (PRETTY / f"{name}.kicad_mod").write_text(
            fp_header(name, f"{n}-pin 2.54mm socket")
            + f'  (fp_text reference "REF**" (at 0 -2) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.12))))\n'
            f'  (fp_text value "{name}" (at 0 {n*pitch+1.5:.2f}) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.12))))\n'
            + "\n".join(pads)
            + "\n)\n"
        )

    pinrow("PinSocket_1x15", 15)
    pinrow("PinSocket_1x08", 8)
    pinrow("PinSocket_1x04", 4)

    # 2x4 NRF socket: col0 x=0, col1 x=2.54; row 0..3
    nrf_pads = []
    # pin1 GND (0,0), pin2 VCC (2.54,0), pin3 CE (0,2.54), ...
    coords = {
        1: (0, 0),
        2: (2.54, 0),
        3: (0, 2.54),
        4: (2.54, 2.54),
        5: (0, 5.08),
        6: (2.54, 5.08),
        7: (0, 7.62),
        8: (2.54, 7.62),
    }
    for pn, (x, y) in coords.items():
        shape = "rect" if pn == 1 else "circle"
        nrf_pads.append(
            f'  (pad "{pn}" thru_hole {shape} (at {x} {y}) (size 1.7 1.7) (drill 1.0) (layers "*.Cu" "*.Mask"))'
        )
    (PRETTY / "PinSocket_2x04.kicad_mod").write_text(
        fp_header("PinSocket_2x04", "NRF24 AM1117 2x4 socket")
        + """  (fp_text reference "REF**" (at 1.27 -2) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_text value "NRF" (at 1.27 9.5) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_text user "ANT->" (at 1.27 -3.5) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.12))))
"""
        + "\n".join(nrf_pads)
        + "\n)\n"
    )

    def terminal(name: str, n: int, pitch: float = 5.08) -> None:
        pads = []
        for i in range(n):
            pads.append(
                f'  (pad "{i+1}" thru_hole rect (at {i*pitch:.2f} 0) (size 3.2 3.2) (drill 1.7) (layers "*.Cu" "*.Mask"))'
            )
        (PRETTY / f"{name}.kicad_mod").write_text(
            fp_header(name, f"{n}-pin 5.08mm screw terminal")
            + f'  (fp_text reference "REF**" (at {(n-1)*pitch/2:.2f} -4) (layer "F.SilkS") (effects (font (size 1 1) (thickness 0.15))))\n'
            f'  (fp_text value "{name}" (at {(n-1)*pitch/2:.2f} 4) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.12))))\n'
            f'  (fp_rect (start -3.2 -3.5) (end {(n-1)*pitch+3.2:.2f} 3.5) (layer "F.SilkS") (width 0.15) (fill none))\n'
            + "\n".join(pads)
            + "\n)\n"
        )

    terminal("Terminal_2", 2)
    terminal("Terminal_4", 4)

    # JST-XH-4 2.5mm
    jst = []
    for i in range(4):
        jst.append(
            f'  (pad "{i+1}" thru_hole {"rect" if i==0 else "circle"} '
            f'(at {i*2.5:.2f} 0) (size 1.7 1.7) (drill 0.9) (layers "*.Cu" "*.Mask"))'
        )
    (PRETTY / "JST_XH_04.kicad_mod").write_text(
        fp_header("JST_XH_04", "JST XH 4-pin 2.5mm")
        + """  (fp_text reference "REF**" (at 3.75 -3.5) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_text value "XH4" (at 3.75 3.2) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_rect (start -2.2 -2.8) (end 9.7 2.8) (layer "F.SilkS") (width 0.12) (fill none))
"""
        + "\n".join(jst)
        + "\n)\n"
    )

    (PRETTY / "TestPoint.kicad_mod").write_text(
        fp_header("TestPoint", "1.0mm test point")
        + """  (fp_text reference "REF**" (at 0 -2) (layer "F.SilkS") (effects (font (size 0.7 0.7) (thickness 0.1))))
  (fp_text value "TP" (at 0 2) (layer "F.Fab") (effects (font (size 0.7 0.7) (thickness 0.1))))
  (pad "1" thru_hole circle (at 0 0) (size 1.8 1.8) (drill 1.0) (layers "*.Cu" "*.Mask"))
)
"""
    )

    (PRETTY / "MountingHole_M3.kicad_mod").write_text(
        """(footprint "MountingHole_M3" (version 20211014) (generator wifeeder-gen)
  (layer "F.Cu")
  (tedit 66B8A001)
  (descr "M3 mounting hole")
  (attr exclude_from_pos_files exclude_from_bom)
  (fp_text reference "H**" (at 0 -3.5) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_text value "M3" (at 0 3.5) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_circle (center 0 0) (end 3.1 0) (layer "F.SilkS") (width 0.15))
  (pad "" thru_hole circle (at 0 0) (size 6.2 6.2) (drill 3.2) (layers *.Cu *.Mask))
)
"""
    )

    (PRETTY / "SolderJumper_3.kicad_mod").write_text(
        fp_smd_header("SolderJumper_3", "3-pad solder jumper")
        + """  (fp_text reference "REF**" (at 0 -2) (layer "F.SilkS") (effects (font (size 0.7 0.7) (thickness 0.1))))
  (fp_text value "SJ" (at 0 2) (layer "F.Fab") (effects (font (size 0.7 0.7) (thickness 0.1))))
  (pad "1" smd rect (at -1.4 0) (size 1.1 1.3) (layers "F.Cu" "F.Mask"))
  (pad "2" smd rect (at 0 0) (size 1.1 1.3) (layers "F.Cu" "F.Mask"))
  (pad "3" smd rect (at 1.4 0) (size 1.1 1.3) (layers "F.Cu" "F.Mask"))
)
"""
    )

    # Mini360 4-pad module (typical 16.5 x 11 mm, pads at corners of long sides)
    (PRETTY / "Mini360.kicad_mod").write_text(
        fp_header("Mini360", "Mini360 DNP fallback IN+/IN-/OUT+/OUT-")
        + """  (fp_text reference "REF**" (at 0 -8) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_text value "MINI360 DNP" (at 0 8) (layer "F.Fab") (effects (font (size 0.8 0.8) (thickness 0.12))))
  (fp_rect (start -9 -6) (end 9 6) (layer "F.SilkS") (width 0.15) (fill none))
  (fp_text user "IN+" (at -6 -7.5) (layer "F.SilkS") (effects (font (size 0.7 0.7) (thickness 0.1))))
  (fp_text user "SET 3V3" (at 0 0) (layer "F.SilkS") (effects (font (size 0.7 0.7) (thickness 0.1))))
  (pad "1" thru_hole rect (at -6.5 -4) (size 2 2) (drill 1.0) (layers "*.Cu" "*.Mask"))
  (pad "2" thru_hole circle (at -6.5 4) (size 2 2) (drill 1.0) (layers "*.Cu" "*.Mask"))
  (pad "3" thru_hole circle (at 6.5 -4) (size 2 2) (drill 1.0) (layers "*.Cu" "*.Mask"))
  (pad "4" thru_hole circle (at 6.5 4) (size 2 2) (drill 1.0) (layers "*.Cu" "*.Mask"))
)
"""
    )


# ---------------------------------------------------------------------------
# Symbol library (minimal, self-contained)
# ---------------------------------------------------------------------------

def pin(n: str, name: str, x: float, y: float, rot: int, etype: str = "passive") -> str:
    return (
        f'    (pin {etype} line (at {x} {y} {rot}) (length 2.54) '
        f'(name "{name}" (effects (font (size 1.27 1.27)))) '
        f'(number "{n}" (effects (font (size 1.27 1.27)))))\n'
    )


def write_symbol_lib() -> None:
    LIBS.mkdir(parents=True, exist_ok=True)
    parts: list[str] = []

    def wrap(name: str, body: str, extra_props: str = "") -> None:
        parts.append(
            f"""  (symbol "{name}"
    (in_bom yes) (on_board yes)
    (property "Reference" "U" (at 0 8.89 0) (effects (font (size 1.27 1.27))))
    (property "Value" "{name}" (at 0 -8.89 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
    (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
{extra_props}    (symbol "{name}_0_1"
      (rectangle (start -7.62 -7.62) (end 7.62 7.62)
        (stroke (width 0.254) (type default) (color 0 0 0 0)) (fill (type background)))
    )
    (symbol "{name}_1_1"
{body}    )
  )
"""
        )

    # Device-like
    parts.append(
        """  (symbol "R"
    (pin_numbers hide)
    (pin_names (offset 0))
    (in_bom yes) (on_board yes)
    (property "Reference" "R" (at 2.032 0 90) (effects (font (size 1.27 1.27))))
    (property "Value" "R" (at -2.032 0 90) (effects (font (size 1.27 1.27))))
    (property "Footprint" "wifeeder:R_0805" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
    (symbol "R_0_1"
      (rectangle (start -1.016 -2.54) (end 1.016 2.54)
        (stroke (width 0.254) (type default) (color 0 0 0 0)) (fill (type none)))
    )
    (symbol "R_1_1"
      (pin passive line (at 0 3.81 270) (length 1.27) (name "~" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
      (pin passive line (at 0 -3.81 90) (length 1.27) (name "~" (effects (font (size 1.27 1.27)))) (number "2" (effects (font (size 1.27 1.27)))))
    )
  )
  (symbol "C"
    (pin_numbers hide)
    (pin_names (offset 0))
    (in_bom yes) (on_board yes)
    (property "Reference" "C" (at 1.778 0 0) (effects (font (size 1.27 1.27))))
    (property "Value" "C" (at -2.032 0 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "wifeeder:C_0805" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
    (symbol "C_0_1"
      (polyline (pts (xy -2.032 -0.762) (xy 2.032 -0.762)) (stroke (width 0.508) (type default) (color 0 0 0 0)) (fill (type none)))
      (polyline (pts (xy -2.032 0.762) (xy 2.032 0.762)) (stroke (width 0.508) (type default) (color 0 0 0 0)) (fill (type none)))
    )
    (symbol "C_1_1"
      (pin passive line (at 0 3.81 270) (length 2.794) (name "~" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
      (pin passive line (at 0 -3.81 90) (length 2.794) (name "~" (effects (font (size 1.27 1.27)))) (number "2" (effects (font (size 1.27 1.27)))))
    )
  )
  (symbol "L"
    (in_bom yes) (on_board yes)
    (property "Reference" "L" (at 1.778 0 0) (effects (font (size 1.27 1.27))))
    (property "Value" "L" (at -2.54 0 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "wifeeder:L_6x6" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
    (symbol "L_0_1"
      (arc (start 0 2.54) (mid 1.016 1.27) (end 0 0) (stroke (width 0.254) (type default)) (fill (type none)))
      (arc (start 0 0) (mid 1.016 -1.27) (end 0 -2.54) (stroke (width 0.254) (type default)) (fill (type none)))
    )
    (symbol "L_1_1"
      (pin passive line (at 0 3.81 270) (length 1.27) (name "~" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
      (pin passive line (at 0 -3.81 90) (length 1.27) (name "~" (effects (font (size 1.27 1.27)))) (number "2" (effects (font (size 1.27 1.27)))))
    )
  )
  (symbol "LED"
    (in_bom yes) (on_board yes)
    (property "Reference" "D" (at 2.54 2.54 0) (effects (font (size 1.27 1.27))))
    (property "Value" "LED" (at 2.54 -2.54 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "wifeeder:LED_0805" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
    (symbol "LED_0_1"
      (polyline (pts (xy -1.27 -1.27) (xy -1.27 1.27) (xy 1.27 0) (xy -1.27 -1.27)) (stroke (width 0.254) (type default)) (fill (type none)))
      (polyline (pts (xy 1.27 -1.27) (xy 1.27 1.27)) (stroke (width 0.254) (type default)) (fill (type none)))
    )
    (symbol "LED_1_1"
      (pin passive line (at -3.81 0 0) (length 2.54) (name "K" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
      (pin passive line (at 3.81 0 180) (length 2.54) (name "A" (effects (font (size 1.27 1.27)))) (number "2" (effects (font (size 1.27 1.27)))))
    )
  )
  (symbol "Fuse"
    (in_bom yes) (on_board yes)
    (property "Reference" "F" (at 2.54 0 0) (effects (font (size 1.27 1.27))))
    (property "Value" "10A" (at -3.81 0 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "wifeeder:Fuse_5x20" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
    (symbol "Fuse_0_1"
      (rectangle (start -0.762 -2.54) (end 0.762 2.54) (stroke (width 0.254) (type default)) (fill (type none)))
    )
    (symbol "Fuse_1_1"
      (pin passive line (at 0 3.81 270) (length 1.27) (name "~" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
      (pin passive line (at 0 -3.81 90) (length 1.27) (name "~" (effects (font (size 1.27 1.27)))) (number "2" (effects (font (size 1.27 1.27)))))
    )
  )
  (symbol "TVS"
    (in_bom yes) (on_board yes)
    (property "Reference" "D" (at 2.54 2.54 0) (effects (font (size 1.27 1.27))))
    (property "Value" "SMAJ15A" (at 3.81 -2.54 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "wifeeder:SMA" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
    (symbol "TVS_0_1"
      (polyline (pts (xy -1.27 -1.27) (xy -1.27 1.27) (xy 1.27 0) (xy -1.27 -1.27)) (stroke (width 0.254) (type default)) (fill (type none)))
      (polyline (pts (xy 1.27 -1.27) (xy 1.27 1.27)) (stroke (width 0.254) (type default)) (fill (type none)))
    )
    (symbol "TVS_1_1"
      (pin passive line (at -3.81 0 0) (length 2.54) (name "A" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
      (pin passive line (at 3.81 0 180) (length 2.54) (name "K" (effects (font (size 1.27 1.27)))) (number "2" (effects (font (size 1.27 1.27)))))
    )
  )
"""
    )

    # P-FET SOIC-8 AO4407: pins 1-3 S, 4 G, 5-8 D (typical)
    wrap(
        "PFET_AO4407",
        pin("1", "S", -10.16, 5.08, 0)
        + pin("2", "S", -10.16, 2.54, 0)
        + pin("3", "S", -10.16, 0, 0)
        + pin("4", "G", -10.16, -5.08, 0, "input")
        + pin("5", "D", 10.16, -5.08, 180)
        + pin("6", "D", 10.16, -2.54, 180)
        + pin("7", "D", 10.16, 0, 180)
        + pin("8", "D", 10.16, 2.54, 180),
    )

    wrap(
        "AP6320x",
        pin("1", "BST", -10.16, 5.08, 0)
        + pin("2", "GND", -10.16, -5.08, 0)
        + pin("3", "FB", 10.16, -5.08, 180)
        + pin("4", "EN", -10.16, 0, 0, "input")
        + pin("5", "VIN", -10.16, 2.54, 0, "power_in")
        + pin("6", "SW", 10.16, 2.54, 180, "passive"),
    )

    pca_pins = ""
    left = [("1", "A0", 5.08), ("2", "A1", 2.54), ("3", "A2", 0), ("4", "A3", -2.54),
            ("5", "A4", -5.08), ("6", "A5", -7.62), ("7", "EXTCLK", -10.16), ("8", "SCL", -12.7),
            ("9", "SDA", -15.24), ("10", "VDD", -17.78), ("11", "VSS", -20.32), ("12", "OE", -22.86),
            ("13", "LED0", -25.4), ("14", "LED1", -27.94)]
    # Use a taller box via extra rectangle in wrap - keep compact mapping
    pca_body = (
        pin("1", "A0", -12.7, 15.24, 0)
        + pin("2", "A1", -12.7, 12.7, 0)
        + pin("3", "A2", -12.7, 10.16, 0)
        + pin("4", "A3", -12.7, 7.62, 0)
        + pin("5", "A4", -12.7, 5.08, 0)
        + pin("6", "A5", -12.7, 2.54, 0)
        + pin("7", "EXTCLK", -12.7, 0, 0)
        + pin("8", "SCL", -12.7, -2.54, 0, "bidirectional")
        + pin("9", "SDA", -12.7, -5.08, 0, "bidirectional")
        + pin("10", "VDD", -12.7, -7.62, 0, "power_in")
        + pin("11", "VSS", -12.7, -10.16, 0, "power_in")
        + pin("12", "OE", -12.7, -12.7, 0, "input")
        + pin("13", "LED0", 12.7, 15.24, 180, "output")
        + pin("14", "LED1", 12.7, 12.7, 180, "output")
        + pin("15", "LED2", 12.7, 10.16, 180, "output")
        + pin("16", "LED3", 12.7, 7.62, 180, "output")
        + pin("17", "LED4", 12.7, 5.08, 180, "output")
        + pin("18", "LED5", 12.7, 2.54, 180, "output")
        + pin("19", "LED6", 12.7, 0, 180, "output")
        + pin("20", "LED7", 12.7, -2.54, 180, "output")
        + pin("21", "LED8", 12.7, -5.08, 180, "output")
        + pin("22", "LED9", 12.7, -7.62, 180, "output")
        + pin("23", "LED10", 12.7, -10.16, 180, "output")
        + pin("24", "LED11", 12.7, -12.7, 180, "output")
        + pin("25", "LED12", 12.7, -15.24, 180, "output")
        + pin("26", "LED13", 12.7, -17.78, 180, "output")
        + pin("27", "LED14", 12.7, -20.32, 180, "output")
        + pin("28", "LED15", 12.7, -22.86, 180, "output")
    )
    parts.append(
        f"""  (symbol "PCA9685"
    (in_bom yes) (on_board yes)
    (property "Reference" "U" (at 0 20.32 0) (effects (font (size 1.27 1.27))))
    (property "Value" "PCA9685" (at 0 -26.67 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "wifeeder:TSSOP-28" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
    (symbol "PCA9685_0_1"
      (rectangle (start -10.16 -25.4) (end 10.16 17.78)
        (stroke (width 0.254) (type default) (color 0 0 0 0)) (fill (type background)))
    )
    (symbol "PCA9685_1_1"
{pca_body}    )
  )
"""
    )

    def conn(name: str, n: int, ref: str = "J") -> None:
        body = ""
        h = (n - 1) * 2.54 / 2
        for i in range(n):
            body += pin(str(i + 1), str(i + 1), -7.62, h - i * 2.54, 0)
        parts.append(
            f"""  (symbol "{name}"
    (in_bom yes) (on_board yes)
    (property "Reference" "{ref}" (at 0 {h+5.08} 0) (effects (font (size 1.27 1.27))))
    (property "Value" "{name}" (at 0 {-h-5.08} 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
    (symbol "{name}_0_1"
      (rectangle (start -5.08 {-h-2.54}) (end 5.08 {h+2.54})
        (stroke (width 0.254) (type default) (color 0 0 0 0)) (fill (type background)))
    )
    (symbol "{name}_1_1"
{body}    )
  )
"""
        )

    conn("Conn_01x02", 2)
    conn("Conn_01x04", 4)
    conn("Conn_01x08", 8)
    conn("Conn_01x15", 15)
    conn("Conn_02x04", 8)

    parts.append(
        """  (symbol "TestPoint"
    (pin_numbers hide)
    (in_bom yes) (on_board yes)
    (property "Reference" "TP" (at 0 3.81 0) (effects (font (size 1.27 1.27))))
    (property "Value" "TP" (at 0 -3.81 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "wifeeder:TestPoint" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
    (symbol "TestPoint_0_1"
      (circle (center 0 0) (radius 1.016) (stroke (width 0.254) (type default)) (fill (type none)))
    )
    (symbol "TestPoint_1_1"
      (pin passive line (at 0 3.81 270) (length 2.54) (name "1" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
    )
  )
  (symbol "SolderJumper_3"
    (in_bom yes) (on_board yes)
    (property "Reference" "SJ" (at 0 3.81 0) (effects (font (size 1.27 1.27))))
    (property "Value" "SJ" (at 0 -3.81 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "wifeeder:SolderJumper_3" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
    (symbol "SolderJumper_3_0_1"
      (rectangle (start -5.08 -1.27) (end 5.08 1.27) (stroke (width 0.254) (type default)) (fill (type none)))
    )
    (symbol "SolderJumper_3_1_1"
      (pin passive line (at -7.62 0 0) (length 2.54) (name "A" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
      (pin passive line (at 0 -3.81 90) (length 2.54) (name "COM" (effects (font (size 1.27 1.27)))) (number "2" (effects (font (size 1.27 1.27)))))
      (pin passive line (at 7.62 0 180) (length 2.54) (name "B" (effects (font (size 1.27 1.27)))) (number "3" (effects (font (size 1.27 1.27)))))
    )
  )
  (symbol "PWR_FLAG"
    (power)
    (pin_numbers hide)
    (pin_names (offset 0) hide)
    (in_bom yes) (on_board yes)
    (property "Reference" "#FLG" (id 0) (at 0 1.905 0) (effects (font (size 1.27 1.27)) hide))
    (property "Value" "PWR_FLAG" (id 1) (at 0 3.81 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "" (id 2) (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
    (property "Datasheet" "~" (id 3) (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
    (symbol "PWR_FLAG_0_0"
      (pin power_out line (at 0 0 90) (length 0)
        (name "pwr" (effects (font (size 1.27 1.27))))
        (number "1" (effects (font (size 1.27 1.27)))))
    )
    (symbol "PWR_FLAG_0_1"
      (polyline (pts (xy 0 0) (xy 0 1.27) (xy -1.016 1.905) (xy 0 2.54) (xy 1.016 1.905) (xy 0 1.27))
        (stroke (width 0) (type default) (color 0 0 0 0)) (fill (type none)))
    )
  )
"""
    )

    (LIBS / "wifeeder.kicad_sym").write_text(
        "(kicad_symbol_lib (version 20211014) (generator wifeeder-gen)\n" + "".join(parts) + ")\n"
    )


# ---------------------------------------------------------------------------
# Schematic writer
# ---------------------------------------------------------------------------

class Sch:
    def __init__(self, title: str, paper: str = "A3") -> None:
        self.title = title
        self.paper = paper
        self.uuid = uid()
        self.items: list[str] = []
        self.symbols_used: set[str] = set()

    def add(self, s: str) -> None:
        self.items.append(s)

    def symbol(
        self,
        lib_id: str,
        ref: str,
        value: str,
        x: float,
        y: float,
        footprint: str,
        pins: list[str],
        rot: int = 0,
        extra: str = "",
    ) -> None:
        self.symbols_used.add(lib_id)
        pin_xml = "".join(f'    (pin "{p}" (uuid "{uid()}"))\n' for p in pins)
        hide = " hide" if ref.startswith("#") else ""
        self.add(
            f"""  (symbol (lib_id "wifeeder:{lib_id}") (at {x} {y} {rot}) (unit 1)
    (in_bom yes) (on_board yes)
    (uuid "{uid()}")
    (property "Reference" "{ref}" (id 0) (at {x} {y + 8.89} 0) (effects (font (size 1.27 1.27)){hide}))
    (property "Value" "{value}" (id 1) (at {x} {y - 5.08} 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "{footprint}" (id 2) (at {x} {y} 0) (effects (font (size 1.27 1.27)) hide))
    (property "Datasheet" "" (id 3) (at {x} {y} 0) (effects (font (size 1.27 1.27)) hide))
{extra}{pin_xml}  )
"""
        )

    def wire(self, x1: float, y1: float, x2: float, y2: float) -> None:
        self.add(
            f'  (wire (pts (xy {x1} {y1}) (xy {x2} {y2}))\n'
            f'    (stroke (width 0) (type default) (color 0 0 0 0))\n'
            f'    (uuid {uid()})\n  )\n'
        )

    def glabel(self, name: str, x: float, y: float, rot: int = 0, shape: str = "input") -> None:
        self.add(
            f'  (global_label "{name}" (shape {shape}) (at {x} {y} {rot}) '
            f'(effects (font (size 1.27 1.27)) (justify left)) (uuid "{uid()}"))\n'
        )

    def label(self, name: str, x: float, y: float, rot: int = 0) -> None:
        self.add(
            f'  (label "{name}" (at {x} {y} {rot}) '
            f'(effects (font (size 1.27 1.27)) (justify left bottom)) (uuid "{uid()}"))\n'
        )

    def text(self, t: str, x: float, y: float, size: float = 2.0) -> None:
        esc = t.replace("\\", "\\\\").replace('"', '\\"')
        self.add(
            f'  (text "{esc}" (at {x} {y} 0)\n'
            f'    (effects (font (size {size} {size})) (justify left bottom))\n'
            f'    (uuid {uid()})\n  )\n'
        )

    def no_connect(self, x: float, y: float) -> None:
        self.add(f'  (no_connect (at {x} {y}) (uuid "{uid()}"))\n')

    def sheet(self, name: str, fname: str, x: float, y: float, w: float, h: float) -> None:
        self.add(
            f"""  (sheet (at {x} {y}) (size {w} {h})
    (stroke (width 0.1524) (type solid) (color 0 0 0 0))
    (fill (color 0 0 0 0.0000))
    (uuid {uid()})
    (property "Sheet name" "{name}" (id 0) (at {x} {y - 1.27} 0)
      (effects (font (size 1.27 1.27)) (justify left bottom)))
    (property "Sheet file" "{fname}" (id 1) (at {x} {y + h + 1.27} 0)
      (effects (font (size 1.27 1.27)) (justify left top)))
  )
"""
        )

    def write(self, path: Path, lib_blob: str) -> None:
        body = f"""(kicad_sch (version 20211123) (generator eeschema)

  (uuid {self.uuid})

  (paper "{self.paper}")

  (title_block
    (title "{self.title}")
    (date "2026-08-11")
    (rev "1")
    (company "WiFeeder")
    (comment 1 "Hybrid actuator carrier - pins match stm32/Core/Inc/board.h")
  )

  (lib_symbols
{lib_blob}  )

{''.join(self.items)}
  (sheet_instances
    (path "/" (page "1"))
  )
)
"""
        path.write_text(body)


def extract_symbols(lib_text: str, names: set[str]) -> str:
    """Pull named symbols from the local lib for embedding."""
    # naive extract by scanning (symbol "NAME"
    out = []
    for name in sorted(names):
        token = f'(symbol "{name}"\n'
        start = lib_text.find(token)
        if start < 0:
            continue
        # find matching close at indent 2
        depth = 0
        i = start
        while i < len(lib_text):
            if lib_text[i] == "(":
                depth += 1
            elif lib_text[i] == ")":
                depth -= 1
                if depth == 0:
                    out.append(lib_text[start : i + 1])
                    break
            i += 1
    return "\n".join(out) + "\n"


def build_schematics(lib_text: str) -> dict[str, str]:
    """Return sheet uuid map for the project file."""
    uuids: dict[str, str] = {}

    # --- root ---
    root = Sch("WiFeeder actuator carrier")
    uuids["root"] = root.uuid
    root.text("WiFeeder v2 — actuator hybrid PCB (spin 1)", 25, 180, 3.5)
    root.text("Modules: Nucleo-L432KC + IBT-2 + NRF AM1117   On-board: PCA9685, 5V/3V3 bucks, protection", 25, 174, 1.8)
    root.text("Off-board: Pi, motor, encoder, 12V pack    Firmware: board.h unchanged", 25, 169, 1.8)
    root.sheet("Power", "power.kicad_sch", 30, 40, 50, 35)
    root.sheet("MCU / NRF / Enc", "mcu.kicad_sch", 95, 40, 55, 35)
    root.sheet("PCA9685", "pca.kicad_sch", 30, 95, 50, 30)
    root.sheet("IBT-2", "ibt.kicad_sch", 95, 95, 55, 30)
    root.sheet("Connectors / mech", "connectors.kicad_sch", 165, 40, 50, 30)
    root.write(ROOT / "wifeeder-actuator.kicad_sch", "")

    # --- power ---
    p = Sch("Power — 12V protect + bucks")
    uuids["power"] = p.uuid
    p.text("12V IN → F1 10A → P-FET reverse → 12V_SAFE → bucks + IBT B+", 20, 185, 2)
    p.text("Do not daisy 5V from Nucleo. Mini360 is DNP (SJ_MINI open).", 20, 180, 1.6)

    p.symbol("Conn_01x02", "TB1", "12V_IN", 30, 150, "wifeeder:Terminal_2", ["1", "2"])
    p.glabel("12V_IN", 18, 152.54, 180, "input")
    p.glabel("GND", 18, 147.46, 180, "input")
    p.wire(22.38, 152.54, 18, 152.54)
    p.wire(22.38, 147.46, 18, 147.46)

    p.symbol("Fuse", "F1", "10A", 45, 152.54, "wifeeder:Fuse_5x20", ["1", "2"])
    p.wire(30, 152.54, 45, 156.35)  # visual only-ish; pins are at 0±3.81 from center
    # Fuse pins at (45, 156.35) and (45, 148.73) if vertical... symbol pins at 0,3.81 and 0,-3.81
    p.wire(37.62, 152.54, 41.19, 152.54)  # won't match exactly — use labels

    p.symbol("PFET_AO4407", "Q1", "AO4407A", 75, 145, "wifeeder:SOIC-8",
             ["1", "2", "3", "4", "5", "6", "7", "8"])
    p.symbol("R", "R1", "100k", 75, 120, "wifeeder:R_0805", ["1", "2"])
    p.symbol("TVS", "D1", "SMAJ15A", 105, 130, "wifeeder:SMA", ["1", "2"])

    p.symbol("AP6320x", "U1", "AP63205 5V", 55, 80, "wifeeder:SOT23-6",
             ["1", "2", "3", "4", "5", "6"])
    p.symbol("L", "L1", "4.7uH", 80, 82.54, "wifeeder:L_6x6", ["1", "2"])
    p.symbol("C", "C1", "10uF", 40, 70, "wifeeder:C_1206", ["1", "2"])
    p.symbol("C", "C2", "22uF", 100, 70, "wifeeder:C_1206", ["1", "2"])
    p.symbol("C", "C3", "100n", 80, 95, "wifeeder:C_0805", ["1", "2"])

    p.symbol("AP6320x", "U2", "AP63203 3V3", 55, 40, "wifeeder:SOT23-6",
             ["1", "2", "3", "4", "5", "6"])
    p.symbol("L", "L2", "4.7uH", 80, 42.54, "wifeeder:L_6x6", ["1", "2"])
    p.symbol("C", "C4", "10uF", 40, 30, "wifeeder:C_1206", ["1", "2"])
    p.symbol("C", "C5", "22uF", 100, 30, "wifeeder:C_1206", ["1", "2"])
    p.symbol("C", "C6", "100n", 80, 55, "wifeeder:C_0805", ["1", "2"])

    p.symbol("LED", "D2", "GRN 5V", 130, 80, "wifeeder:LED_0805", ["1", "2"])
    p.symbol("R", "R2", "1k", 145, 80, "wifeeder:R_0805", ["1", "2"])
    p.symbol("LED", "D3", "GRN 3V3", 130, 40, "wifeeder:LED_0805", ["1", "2"])
    p.symbol("R", "R3", "1k", 145, 40, "wifeeder:R_0805", ["1", "2"])

    p.symbol("TestPoint", "TP1", "12V_SAFE", 120, 155, "wifeeder:TestPoint", ["1"])
    p.symbol("TestPoint", "TP2", "5V", 160, 80, "wifeeder:TestPoint", ["1"])
    p.symbol("TestPoint", "TP3", "3V3", 160, 40, "wifeeder:TestPoint", ["1"])
    p.symbol("TestPoint", "TP4", "GND", 160, 20, "wifeeder:TestPoint", ["1"])

    p.symbol("Conn_01x04", "J_MINI", "Mini360 DNP", 200, 80, "wifeeder:Mini360", ["1", "2", "3", "4"])
    p.symbol("SolderJumper_3", "SJ_MINI", "DNP", 230, 55, "wifeeder:SolderJumper_3", ["1", "2", "3"])

    p.glabel("12V_IN", 20, 160, 0, "output")
    p.glabel("12V_SAFE", 120, 160, 0, "output")
    p.glabel("5V", 165, 85, 0, "output")
    p.glabel("3V3", 165, 45, 0, "output")
    p.glabel("GND", 165, 15, 0, "output")
    # ERC: tell KiCad these rails are driven (bucks / battery)
    p.symbol("PWR_FLAG", "#FLG1", "PWR_FLAG", 120, 160, "", ["1"])
    p.symbol("PWR_FLAG", "#FLG2", "PWR_FLAG", 165, 85, "", ["1"])
    p.symbol("PWR_FLAG", "#FLG3", "PWR_FLAG", 165, 45, "", ["1"])
    p.symbol("PWR_FLAG", "#FLG4", "PWR_FLAG", 165, 15, "", ["1"])
    p.symbol("PWR_FLAG", "#FLG5", "PWR_FLAG", 20, 160, "", ["1"])
    # Visible power path
    p.wire(30, 152.54, 45, 156.35)
    p.wire(45, 148.73, 64.84, 150.08)  # fuse out toward Q1 source-ish
    p.wire(100, 70, 100, 80)
    p.wire(100, 30, 100, 40)

    p.text("Q1: source=fused 12V, drain=12V_SAFE, gate=GND via R1", 20, 110, 1.4)
    p.text("U1 AP63205: VIN=12V_SAFE EN=VIN FB=5V SW-L1-5V", 20, 105, 1.4)
    p.text("U2 AP63203: same → 3V3. J_MINI pads 1=IN+ 2=IN- 3=OUT+ 4=OUT-", 20, 100, 1.4)
    p.text("SJ_MINI closed only if U2 DNP. Never parallel bucks.", 20, 16, 1.4)

    p.write(ROOT / "power.kicad_sch", extract_symbols(lib_text, p.symbols_used))

    # --- MCU ---
    m = Sch("MCU Nucleo-32 + NRF + encoder + DNP")
    uuids["mcu"] = m.uuid
    m.text("NUCLEO-L432KC Nano sockets (USB off board top). One pin per net. PA6 NC.", 15, 185, 2)
    m.symbol("Conn_01x15", "J_NUC_L", "CN3 analog", 50, 110, "wifeeder:PinSocket_1x15",
             [str(i) for i in range(1, 16)])
    m.symbol("Conn_01x15", "J_NUC_R", "CN4 digital", 120, 110, "wifeeder:PinSocket_1x15",
             [str(i) for i in range(1, 16)])

    # CN3 nets via comments + global labels placed at pin y positions
    # symbol center 50,110; pin1 at + (n-1)*2.54/2 = 17.78 → y=127.78 down to 92.22
    cn3 = [
        (1, "D13_PB3", "passive"),
        (2, "3V3", "input"),
        (3, "AREF", "passive"),
        (4, "ENC_A", "bidirectional"),
        (5, "ENC_B", "bidirectional"),
        (6, "RFID_RX", "output"),
        (7, "NRF_CSN", "output"),
        (8, "NRF_SCK", "output"),
        (9, "PA6_NC", "passive"),
        (10, "NRF_MOSI", "output"),
        (11, "PA2_VCP", "passive"),
        (12, "NUC_5V", "passive"),
        (13, "NUC_NRST", "passive"),
        (14, "GND", "input"),
        (15, "NUC_VIN", "passive"),
    ]
    h = 14 * 2.54 / 2
    for i, net, shp in cn3:
        y = 110 + h - (i - 1) * 2.54
        m.glabel(net, 35, y, 180, shp if shp != "passive" else "passive")
        if net in ("PA6_NC", "AREF", "NUC_5V", "NUC_VIN", "PA2_VCP", "D13_PB3", "NUC_NRST"):
            m.no_connect(42.38, y)

    cn4 = [
        (1, "PA9", "passive"),
        (2, "PA10", "passive"),
        (3, "NUC_NRST", "passive"),
        (4, "GND", "input"),
        (5, "PA12", "passive"),
        (6, "NRF_CE", "output"),
        (7, "I2C_SDA", "bidirectional"),
        (8, "I2C_SCL", "bidirectional"),
        (9, "NRF_MISO", "input"),
        (10, "PC14", "passive"),
        (11, "PC15", "passive"),
        (12, "PA8", "passive"),
        (13, "PA11", "passive"),
        (14, "HX711_SCK", "output"),
        (15, "HX711_DOUT", "input"),
    ]
    for i, net, shp in cn4:
        y = 110 + h - (i - 1) * 2.54
        m.glabel(net, 135, y, 0, shp)
        if net in ("PA9", "PA10", "PA12", "PC14", "PC15", "PA8", "PA11", "NUC_NRST"):
            m.no_connect(127.62, y)
        else:
            m.wire(127.62, y, 135, y)

    m.symbol("Conn_02x04", "J_NRF", "NRF AM1117 5V", 200, 140, "wifeeder:PinSocket_2x04",
             [str(i) for i in range(1, 9)])
    m.text("J_NRF: 1 GND  2 VCC=5V  3 CE  4 CSN  5 SCK  6 MOSI  7 MISO  8 IRQ nc", 165, 165, 1.4)
    m.glabel("GND", 185, 148.89, 180, "input")
    m.glabel("5V", 185, 146.35, 180, "input")
    m.glabel("NRF_CE", 185, 143.81, 180, "input")
    m.glabel("NRF_CSN", 185, 141.27, 180, "input")
    m.glabel("NRF_SCK", 185, 138.73, 180, "input")
    m.glabel("NRF_MOSI", 185, 136.19, 180, "input")
    m.glabel("NRF_MISO", 185, 133.65, 180, "output")
    m.no_connect(192.38, 131.11)  # IRQ

    m.symbol("Conn_01x04", "TB4", "ENC GX fly", 50, 40, "wifeeder:Terminal_4", ["1", "2", "3", "4"])
    m.symbol("R", "R4", "4.7k", 80, 48, "wifeeder:R_0805", ["1", "2"])
    m.symbol("R", "R5", "4.7k", 95, 48, "wifeeder:R_0805", ["1", "2"])
    m.symbol("R", "R6", "4.7k", 200, 90, "wifeeder:R_0805", ["1", "2"])
    m.symbol("R", "R7", "4.7k", 215, 90, "wifeeder:R_0805", ["1", "2"])
    m.text("TB4: 1=5V  2=GND  3=A  4=B   pull-ups R4/R5 to 3V3 (not 5V)", 20, 22, 1.5)
    m.glabel("5V", 35, 43.81, 180, "input")
    m.glabel("GND", 35, 41.27, 180, "input")
    m.glabel("ENC_A", 35, 38.73, 180, "bidirectional")
    m.glabel("ENC_B", 35, 36.19, 180, "bidirectional")
    m.glabel("3V3", 80, 58, 90, "input")
    m.glabel("3V3", 200, 100, 90, "input")
    m.glabel("I2C_SCL", 200, 82, 270, "bidirectional")
    m.glabel("I2C_SDA", 215, 82, 270, "bidirectional")

    m.symbol("Conn_01x04", "J_RFID", "RFID DNP", 130, 40, "wifeeder:JST_XH_04", ["1", "2", "3", "4"])
    m.symbol("Conn_01x04", "J_HX711", "HX711 DNP", 180, 40, "wifeeder:JST_XH_04", ["1", "2", "3", "4"])
    m.symbol("SolderJumper_3", "SJ_RFID_V", "3V3/5V", 130, 18, "wifeeder:SolderJumper_3", ["1", "2", "3"])
    m.text("J_RFID: 1 VCC 2 GND 3 RX←PA3 4 NC    J_HX711: 1 3V3 2 GND 3 DOUT 4 SCK", 20, 16, 1.4)
    m.glabel("RFID_RX", 115, 38.73, 180, "input")
    m.glabel("GND", 115, 41.27, 180, "input")
    m.glabel("HX711_DOUT", 165, 38.73, 180, "output")
    m.glabel("HX711_SCK", 165, 36.19, 180, "input")
    m.glabel("3V3", 165, 43.81, 180, "input")
    m.glabel("GND", 165, 41.27, 180, "input")

    m.write(ROOT / "mcu.kicad_sch", extract_symbols(lib_text, m.symbols_used))

    # --- PCA ---
    c = Sch("PCA9685")
    uuids["pca"] = c.uuid
    c.text("PCA9685 addr 0x40 (A0–A5 open). OE=GND. EXTCLK=GND. PWM0/1 only routed.", 15, 180, 2)
    c.symbol("PCA9685", "U3", "PCA9685", 80, 110, "wifeeder:TSSOP-28",
             [str(i) for i in range(1, 29)])
    c.symbol("C", "C7", "100n", 40, 80, "wifeeder:C_0805", ["1", "2"])
    c.symbol("C", "C8", "10uF", 40, 65, "wifeeder:C_1206", ["1", "2"])
    c.glabel("I2C_SCL", 55, 107.46, 180, "bidirectional")
    c.glabel("I2C_SDA", 55, 104.92, 180, "bidirectional")
    c.glabel("3V3", 55, 102.38, 180, "input")
    c.glabel("GND", 55, 99.84, 180, "input")
    c.glabel("PWM0", 105, 125.24, 0, "output")
    c.glabel("PWM1", 105, 122.7, 0, "output")
    c.wire(92.7, 125.24, 105, 125.24)
    c.wire(92.7, 122.7, 105, 122.7)
    c.wire(55, 107.46, 67.3, 107.46)
    c.wire(55, 104.92, 67.3, 104.92)
    # LED2–LED15 unused (no Motor2)
    for i, yoff in enumerate(
        [10.16, 7.62, 5.08, 2.54, 0, -2.54, -5.08, -7.62, -10.16, -12.7, -15.24, -17.78, -20.32, -22.86]
    ):
        c.no_connect(92.7, 110 + yoff)
    # A0–A5, EXTCLK left open / tied in note (addr 0x40)
    for yoff in (15.24, 12.7, 10.16, 7.62, 5.08, 2.54, 0):
        c.no_connect(67.3, 110 + yoff)
    c.text("LED2-LED15: NC (no Motor2). A0-A5/EXTCLK open = addr 0x40 + internal osc.", 15, 30, 1.5)
    c.write(ROOT / "pca.kicad_sch", extract_symbols(lib_text, c.symbols_used))

    # --- IBT ---
    i = Sch("IBT-2 module")
    uuids["ibt"] = i.uuid
    i.text("IBT-2 seats on standoffs. Logic 2.54mm. High current via TB2 / TB3 short 16AWG to module screws.", 12, 180, 1.8)
    i.symbol("Conn_01x08", "J_IBT", "IBT logic", 70, 120, "wifeeder:PinSocket_1x08",
             [str(n) for n in range(1, 9)])
    ibt_nets = [
        (1, "PWM0", "input"),
        (2, "PWM1", "input"),
        (3, "3V3", "input"),
        (4, "3V3", "input"),
        (5, "IBT_RIS", "passive"),
        (6, "IBT_LIS", "passive"),
        (7, "5V", "input"),
        (8, "GND", "input"),
    ]
    ih = 7 * 2.54 / 2
    for n, net, shp in ibt_nets:
        y = 120 + ih - (n - 1) * 2.54
        i.glabel(net, 55, y, 180, shp)
    i.text("J_IBT 1 RPWM 2 LPWM 3 R_EN 4 L_EN 5 R_IS 6 L_IS 7 VCC 8 GND", 20, 85, 1.5)
    ih = 7 * 2.54 / 2
    i.no_connect(62.38, 120 + ih - 4 * 2.54)  # R_IS
    i.no_connect(62.38, 120 + ih - 5 * 2.54)  # L_IS
    for n in (1, 2, 3, 4, 7, 8):
        y = 120 + ih - (n - 1) * 2.54
        i.wire(55, y, 62.38, y)
    i.symbol("Conn_01x02", "TB2", "TO IBT B+/B-", 140, 130, "wifeeder:Terminal_2", ["1", "2"])
    i.symbol("Conn_01x02", "TB3", "MOTOR / GX-2", 140, 100, "wifeeder:Terminal_2", ["1", "2"])
    i.glabel("12V_SAFE", 125, 132.54, 180, "input")
    i.glabel("GND", 125, 127.46, 180, "input")
    i.glabel("MOTOR_P", 125, 102.54, 180, "output")
    i.glabel("MOTOR_N", 125, 97.46, 180, "output")
    i.text("TB3 is the panel / GX-2 fly-wire (20 mm). Tie to IBT M+/M− with 16 AWG.", 20, 70, 1.5)
    i.text("Heatsink keepout ~50×50 mm around J_IBT. Mounting holes H5–H8.", 20, 64, 1.5)
    i.write(ROOT / "ibt.kicad_sch", extract_symbols(lib_text, i.symbols_used))

    # --- connectors / mech ---
    k = Sch("Connectors / mechanical")
    uuids["conn"] = k.uuid
    k.text("GX series not locked (measure chassis). Use 5.08 mm screws; panel GX fly-wires 20 mm.", 15, 180, 1.8)
    k.text("IBT-2 outline keepout 50×50 mm + heatsink height. Nucleo USB overhang 15 mm off top edge.", 15, 174, 1.6)
    k.text("NRF antenna keepout: no 12V pour, no ground pour under SMA (top-right).", 15, 168, 1.6)
    k.text("H1–H4 board M3. H5–H8 IBT module (43 mm square typical — verify).", 15, 162, 1.6)
    k.text("PA6 silkscreen: NC. NRF VCC = 5V. USB unplugged when 3V3 injected.", 15, 156, 1.6)
    k.write(ROOT / "connectors.kicad_sch", "")

    return uuids


# ---------------------------------------------------------------------------
# PCB
# ---------------------------------------------------------------------------

NETS = [
    "",
    "GND",
    "12V_IN",
    "12V_SAFE",
    "5V",
    "3V3",
    "I2C_SCL",
    "I2C_SDA",
    "PWM0",
    "PWM1",
    "NRF_CE",
    "NRF_CSN",
    "NRF_SCK",
    "NRF_MOSI",
    "NRF_MISO",
    "ENC_A",
    "ENC_B",
    "RFID_RX",
    "HX711_DOUT",
    "HX711_SCK",
    "MOTOR_P",
    "MOTOR_N",
    "NUC_5V",
    "NUC_VIN",
    "NUC_NRST",
    "PA6_NC",
    "PA2_VCP",
    "PA8",
    "PA9",
    "PA10",
    "PA11",
    "PA12",
    "PC14",
    "PC15",
    "D13_PB3",
    "AREF",
    "IBT_RIS",
    "IBT_LIS",
]

NET_ID = {n: i for i, n in enumerate(NETS)}


class Pcb:
    def __init__(self) -> None:
        self.fps: list[str] = []
        self.segs: list[str] = []
        self.graphics: list[str] = []
        self.zones: list[str] = []
        self.vias: list[str] = []
        # for gerber
        self.th_pads: list[tuple] = []  # x,y,drill,sx,sy,net,shape
        self.smd_pads: list[tuple] = []
        self.tracks: list[tuple] = []  # x1,y1,x2,y2,w,layer
        self.holes: list[tuple] = []

    def fp(
        self,
        lib: str,
        ref: str,
        value: str,
        x: float,
        y: float,
        pads: list[tuple],
        rot: float = 0,
        smd: bool = False,
    ) -> None:
        """pads: list of (num, px, py, net, drill, sx, sy, is_smd). local coords."""
        pad_s = []
        for num, px, py, net, drill, sx, sy, is_smd in pads:
            nid = NET_ID.get(net, 0)
            net_s = f'(net {nid} "{net}")' if net else "(net 0 \"\")"
            # rotate local pad
            r = math.radians(rot)
            wx = px * math.cos(r) - py * math.sin(r)
            wy = px * math.sin(r) + py * math.cos(r)
            ax, ay = x + wx, y + wy
            if is_smd:
                pad_s.append(
                    f'    (pad "{num}" smd roundrect (at {px} {py} {rot}) (size {sx} {sy}) '
                    f'(layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.25) {net_s})'
                )
                self.smd_pads.append((ax, ay, sx, sy, net, rot))
            elif drill:
                shape = "rect" if str(num) == "1" else "circle"
                pad_s.append(
                    f'    (pad "{num}" thru_hole {shape} (at {px} {py} {rot}) (size {sx} {sy}) '
                    f'(drill {drill}) (layers "*.Cu" "*.Mask") {net_s})'
                )
                self.th_pads.append((ax, ay, drill, sx, sy, net))
                self.holes.append((ax, ay, drill))
            else:
                pad_s.append(
                    f'    (pad "{num}" thru_hole circle (at {px} {py}) (size {sx} {sy}) '
                    f'(drill {sx*0.6:.2f}) (layers *.Cu *.Mask) {net_s})'
                )
        attr = "smd" if smd else "through_hole"
        ts = uid()
        self.fps.append(
            f"""  (footprint "{lib}" (layer "F.Cu")
    (tedit 66B8A001) (tstamp {ts})
    (at {x:.3f} {y:.3f} {rot})
    (descr "{value}")
    (path "/{ts}")
    (attr {attr})
    (fp_text reference "{ref}" (at 0 -4) (layer "F.SilkS")
      (effects (font (size 0.8 0.8) (thickness 0.12)))
      (tstamp {uid()})
    )
    (fp_text value "{value}" (at 0 4) (layer "F.Fab")
      (effects (font (size 0.7 0.7) (thickness 0.1)))
      (tstamp {uid()})
    )
{chr(10).join(pad_s)}
  )
"""
        )

    def track(self, x1: float, y1: float, x2: float, y2: float, w: float, net: str, layer: str = "F.Cu") -> None:
        nid = NET_ID.get(net, 0)
        self.segs.append(
            f'  (segment (start {x1:.3f} {y1:.3f}) (end {x2:.3f} {y2:.3f}) '
            f'(width {w}) (layer "{layer}") (net {nid}) (tstamp {uid()}))\n'
        )
        self.tracks.append((x1, y1, x2, y2, w, layer, net))

    def silk(self, x: float, y: float, text: str, size: float = 1.2) -> None:
        self.graphics.append(
            f'  (gr_text "{text}" (at {x:.2f} {y:.2f}) (layer "F.SilkS") (tstamp {uid()})\n'
            f'    (effects (font (size {size} {size}) (thickness {size*0.15:.2f})) (justify left)))\n'
        )

    def write(self, path: Path) -> None:
        nets = "\n".join(f'  (net {i} "{n}")' for i, n in enumerate(NETS))
        edge = f"""  (gr_rect (start 0 0) (end {BOARD_W} {BOARD_H}) (layer "Edge.Cuts") (width 0.15) (fill none) (tstamp {uid()}))
  (gr_rect (start 118 2) (end 148 28) (layer "Dwgs.User") (width 0.12) (fill none) (tstamp {uid()}))
"""
        # Poured fills (KiCad 6 will refresh on B; these make copper present on first open)
        gnd_pts = f"(xy 1.5 1.5) (xy {BOARD_W-1.5} 1.5) (xy {BOARD_W-1.5} {BOARD_H-1.5}) (xy 1.5 {BOARD_H-1.5})"
        gnd_front = (
            f"(xy 1.5 29) (xy 117 29) (xy 117 1.5) (xy 1.5 1.5) "
            f"(xy 1.5 {BOARD_H-1.5}) (xy {BOARD_W-1.5} {BOARD_H-1.5}) (xy {BOARD_W-1.5} 29) (xy 1.5 29)"
        )
        p12 = "(xy 2.5 70.5) (xy 37.5 70.5) (xy 37.5 97.5) (xy 2.5 97.5)"
        zone = f"""  (zone (net {NET_ID['GND']}) (net_name "GND") (layer "B.Cu") (tstamp {uid()}) (hatch edge 0.508)
    (connect_pads (clearance 0.3))
    (min_thickness 0.25) (filled_areas_thickness no)
    (fill yes (thermal_gap 0.5) (thermal_bridge_width 0.5))
    (polygon (pts {gnd_pts}))
  )
  (zone (net {NET_ID['GND']}) (net_name "GND") (layer "F.Cu") (tstamp {uid()}) (hatch edge 0.508)
    (connect_pads (clearance 0.3))
    (min_thickness 0.25) (filled_areas_thickness no)
    (fill yes (thermal_gap 0.5) (thermal_bridge_width 0.5))
    (polygon (pts {gnd_front}))
  )
  (zone (net {NET_ID['12V_SAFE']}) (net_name "12V_SAFE") (layer "F.Cu") (tstamp {uid()}) (hatch edge 0.508)
    (connect_pads (clearance 0.4))
    (min_thickness 0.4) (filled_areas_thickness no)
    (fill yes (thermal_gap 0.5) (thermal_bridge_width 0.8))
    (polygon (pts {p12}))
  )
"""
        keepout = f"""  (zone (net 0) (net_name "") (layer "F.Cu") (tstamp {uid()}) (hatch edge 0.508)
    (connect_pads (clearance 0))
    (min_thickness 0.25)
    (keepout (tracks not_allowed) (vias not_allowed) (copperpour not_allowed))
    (fill yes (thermal_gap 0.5) (thermal_bridge_width 0.5))
    (polygon (pts (xy 118 2) (xy 149 2) (xy 149 28) (xy 118 28)))
  )
  (zone (net 0) (net_name "") (layer "B.Cu") (tstamp {uid()}) (hatch edge 0.508)
    (connect_pads (clearance 0))
    (min_thickness 0.25)
    (keepout (tracks not_allowed) (vias not_allowed) (copperpour not_allowed))
    (fill yes (thermal_gap 0.5) (thermal_bridge_width 0.5))
    (polygon (pts (xy 118 2) (xy 149 2) (xy 149 28) (xy 118 28)))
  )
"""
        path.write_text(
            f"""(kicad_pcb (version 20210722) (generator pcbnew)

  (general
    (thickness 1.6)
  )
  (paper "A3")
  (title_block
    (title "WiFeeder actuator carrier")
    (date "2026-08-11")
    (rev "1")
    (company "WiFeeder")
  )
  (layers
    (0 "F.Cu" signal)
    (31 "B.Cu" signal)
    (32 "B.Adhes" user "B.Adhesive")
    (33 "F.Adhes" user "F.Adhesive")
    (34 "B.Paste" user)
    (35 "F.Paste" user)
    (36 "B.SilkS" user "B.Silkscreen")
    (37 "F.SilkS" user "F.Silkscreen")
    (38 "B.Mask" user)
    (39 "F.Mask" user)
    (40 "Dwgs.User" user "User.Drawings")
    (41 "Cmts.User" user "User.Comments")
    (42 "Eco1.User" user "User.Eco1")
    (43 "Eco2.User" user "User.Eco2")
    (44 "Edge.Cuts" user)
    (45 "Margin" user)
    (46 "B.CrtYd" user "B.Courtyard")
    (47 "F.CrtYd" user "F.Courtyard")
    (48 "B.Fab" user)
    (49 "F.Fab" user)
  )
  (setup
    (pad_to_mask_clearance 0.05)
    (pcbplotparams
      (layerselection 0x00010f0_ffffffff)
      (disableapertmacros false)
      (usegerberextensions false)
      (usegerberattributes true)
      (usegerberadvancedattributes true)
      (creategerberjobfile true)
      (svguseinch false)
      (svgprecision 6)
      (excludeedgelayer true)
      (plotframeref false)
      (viasonmask false)
      (mode 1)
      (useauxorigin false)
      (hpglpennumber 1)
      (hpglpenspeed 20)
      (hpglpendiameter 15.000000)
      (dxfpolygonmode true)
      (dxfimperialunits true)
      (dxfusepcbnewfont true)
      (psnegative false)
      (psa4output false)
      (plotreference true)
      (plotvalue true)
      (plotinvisibletext false)
      (sketchpadsonfab false)
      (subtractmaskfromsilk false)
      (outputformat 1)
      (mirror false)
      (drillshape 1)
      (scaleselection 1)
      (outputdirectory "fab/")
    )
  )
{nets}
{''.join(self.fps)}{''.join(self.segs)}{edge}{''.join(self.graphics)}{zone}{keepout})
"""
        )


def r_pads(n1: str, n2: str) -> list:
    return [
        ("1", -0.95, 0, n1, 0, 0.8, 1.3, True),
        ("2", 0.95, 0, n2, 0, 0.8, 1.3, True),
    ]


def c_pads(n1: str, n2: str, wide: bool = False) -> list:
    if wide:
        return [("1", -1.4, 0, n1, 0, 1.1, 1.6, True), ("2", 1.4, 0, n2, 0, 1.1, 1.6, True)]
    return r_pads(n1, n2)


def sock_1x(n: int, nets: list[str], pitch: float = 2.54) -> list:
    return [(str(i + 1), 0.0, i * pitch, nets[i], 1.0, 1.7, 1.7, False) for i in range(n)]


def term(nets: list[str], pitch: float = 5.08) -> list:
    return [(str(i + 1), i * pitch, 0.0, nets[i], 1.7, 3.2, 3.2, False) for i in range(len(nets))]


def build_pcb() -> Pcb:
    b = Pcb()
    # Mounting
    for i, (x, y) in enumerate([(3.5, 3.5), (146.5, 3.5), (3.5, 96.5), (146.5, 96.5)], 1):
        b.fp("wifeeder:MountingHole_M3", f"H{i}", "M3", x, y, [("", 0, 0, "GND", 3.2, 6.2, 6.2, False)])

    # IBT module holes 43mm (H5-H8)
    ibt_ox, ibt_oy = 105.0, 48.0
    for i, (dx, dy) in enumerate([(0, 0), (43, 0), (0, 43), (43, 43)], 5):
        b.fp("wifeeder:MountingHole_M3", f"H{i}", "IBT", ibt_ox + dx, ibt_oy + dy,
             [("", 0, 0, "GND", 3.2, 6.2, 6.2, False)])

    # Power left
    b.fp("wifeeder:Terminal_2", "TB1", "12V_IN", 8, 88, term(["12V_IN", "GND"]))
    b.fp("wifeeder:Fuse_5x20", "F1", "10A", 22, 88, [
        ("1", -10, 0, "12V_IN", 1.6, 3, 3, False),
        ("2", 10, 0, "12V_SAFE", 1.6, 3, 3, False),
    ])
    # P-FET
    soic = []
    for i in range(4):
        soic.append((str(i + 1), -2.7, -1.905 + i * 1.27, "12V_IN" if i < 3 else "GND", 0, 1.55, 0.6, True))
    for i in range(4):
        soic.append((str(8 - i), 2.7, -1.905 + i * 1.27, "12V_SAFE", 0, 1.55, 0.6, True))
    b.fp("wifeeder:SOIC-8", "Q1", "AO4407A", 22, 76, soic, smd=True)
    b.fp("wifeeder:R_0805", "R1", "100k", 22, 70, r_pads("GND", "GND"), smd=True)
    b.fp("wifeeder:SMA", "D1", "SMAJ15A", 22, 64, [
        ("1", -2.1, 0, "12V_SAFE", 0, 1.8, 2.2, True),
        ("2", 2.1, 0, "GND", 0, 1.8, 2.2, True),
    ], smd=True)

    # Bucks
    def buck_pads(vin: str, vout: str) -> list:
        # SOT23-6: 1 BST, 2 GND, 3 FB, 4 EN, 5 VIN, 6 SW
        return [
            ("1", -1.27, -0.95, vout, 0, 0.7, 0.55, True),
            ("2", 0, -0.95, "GND", 0, 0.7, 0.55, True),
            ("3", 1.27, -0.95, vout, 0, 0.7, 0.55, True),
            ("4", 1.27, 0.95, vin, 0, 0.7, 0.55, True),
            ("5", 0, 0.95, vin, 0, 0.7, 0.55, True),
            ("6", -1.27, 0.95, vout, 0, 0.7, 0.55, True),
        ]

    b.fp("wifeeder:SOT23-6", "U1", "AP63205", 12, 48, buck_pads("12V_SAFE", "5V"), smd=True)
    b.fp("wifeeder:L_6x6", "L1", "4.7uH", 22, 48, [
        ("1", -2.4, 0, "5V", 0, 1.6, 4.5, True),
        ("2", 2.4, 0, "5V", 0, 1.6, 4.5, True),
    ], smd=True)
    b.fp("wifeeder:C_1206", "C1", "10uF", 12, 40, c_pads("12V_SAFE", "GND", True), smd=True)
    b.fp("wifeeder:C_1206", "C2", "22uF", 22, 40, c_pads("5V", "GND", True), smd=True)
    b.fp("wifeeder:C_0805", "C3", "100n", 12, 34, c_pads("5V", "GND"), smd=True)

    b.fp("wifeeder:SOT23-6", "U2", "AP63203", 12, 22, buck_pads("12V_SAFE", "3V3"), smd=True)
    b.fp("wifeeder:L_6x6", "L2", "4.7uH", 22, 22, [
        ("1", -2.4, 0, "3V3", 0, 1.6, 4.5, True),
        ("2", 2.4, 0, "3V3", 0, 1.6, 4.5, True),
    ], smd=True)
    b.fp("wifeeder:C_1206", "C4", "10uF", 12, 14, c_pads("12V_SAFE", "GND", True), smd=True)
    b.fp("wifeeder:C_1206", "C5", "22uF", 22, 14, c_pads("3V3", "GND", True), smd=True)
    b.fp("wifeeder:C_0805", "C6", "100n", 12, 8, c_pads("3V3", "GND"), smd=True)

    b.fp("wifeeder:LED_0805", "D2", "5V", 32, 48, r_pads("GND", "5V"), smd=True)
    b.fp("wifeeder:R_0805", "R2", "1k", 38, 48, r_pads("GND", "5V"), smd=True)
    b.fp("wifeeder:LED_0805", "D3", "3V3", 32, 22, r_pads("GND", "3V3"), smd=True)
    b.fp("wifeeder:R_0805", "R3", "1k", 38, 22, r_pads("GND", "3V3"), smd=True)

    b.fp("wifeeder:TestPoint", "TP1", "12V", 32, 88, [("1", 0, 0, "12V_SAFE", 1.0, 1.8, 1.8, False)])
    b.fp("wifeeder:TestPoint", "TP2", "5V", 38, 40, [("1", 0, 0, "5V", 1.0, 1.8, 1.8, False)])
    b.fp("wifeeder:TestPoint", "TP3", "3V3", 38, 14, [("1", 0, 0, "3V3", 1.0, 1.8, 1.8, False)])
    b.fp("wifeeder:TestPoint", "TP4", "GND", 38, 8, [("1", 0, 0, "GND", 1.0, 1.8, 1.8, False)])

    b.fp("wifeeder:Mini360", "J_MINI", "DNP", 22, 58, [
        ("1", -6.5, -4, "12V_SAFE", 1.0, 2, 2, False),
        ("2", -6.5, 4, "GND", 1.0, 2, 2, False),
        ("3", 6.5, -4, "3V3", 1.0, 2, 2, False),
        ("4", 6.5, 4, "GND", 1.0, 2, 2, False),
    ])
    b.fp("wifeeder:SolderJumper_3", "SJ_MINI", "OPEN", 36, 58, [
        ("1", -1.4, 0, "3V3", 0, 1.1, 1.3, True),
        ("2", 0, 0, "", 0, 1.1, 1.3, True),
        ("3", 1.4, 0, "", 0, 1.1, 1.3, True),
    ], smd=True)

    # Nucleo: USB toward y=0, headers 15.24mm apart
    nuc_x, nuc_y = 52.0, 8.0
    cn3_nets = [
        "D13_PB3", "3V3", "AREF", "ENC_A", "ENC_B", "RFID_RX", "NRF_CSN",
        "NRF_SCK", "PA6_NC", "NRF_MOSI", "PA2_VCP", "NUC_5V", "NUC_NRST", "GND", "NUC_VIN",
    ]
    cn4_nets = [
        "PA9", "PA10", "NUC_NRST", "GND", "PA12", "NRF_CE", "I2C_SDA",
        "I2C_SCL", "NRF_MISO", "PC14", "PC15", "PA8", "PA11", "HX711_SCK", "HX711_DOUT",
    ]
    b.fp("wifeeder:PinSocket_1x15", "J_NUC_L", "CN3", nuc_x, nuc_y, sock_1x(15, cn3_nets))
    b.fp("wifeeder:PinSocket_1x15", "J_NUC_R", "CN4", nuc_x + 15.24, nuc_y, sock_1x(15, cn4_nets))

    # NRF top-right, antenna toward +Y off board
    nrf_x, nrf_y = 122.0, 8.0
    nrf_nets = {
        1: ("GND", 0, 0),
        2: ("5V", 2.54, 0),
        3: ("NRF_CE", 0, 2.54),
        4: ("NRF_CSN", 2.54, 2.54),
        5: ("NRF_SCK", 0, 5.08),
        6: ("NRF_MOSI", 2.54, 5.08),
        7: ("NRF_MISO", 0, 7.62),
        8: ("", 2.54, 7.62),  # IRQ nc
    }
    b.fp("wifeeder:PinSocket_2x04", "J_NRF", "NRF 5V", nrf_x, nrf_y,
         [(str(k), v[1], v[2], v[0], 1.0, 1.7, 1.7, False) for k, v in nrf_nets.items()])

    # PCA
    pca_x, pca_y = 62.0, 62.0
    pca_pad = []
    pca_net = {
        8: "I2C_SCL", 9: "I2C_SDA", 10: "3V3", 11: "GND", 12: "GND",
        13: "PWM0", 14: "PWM1",
    }
    for i in range(14):
        y = -4.225 + i * 0.65
        nL = pca_net.get(i + 1, "")
        nR = pca_net.get(28 - i, "")
        pca_pad.append((str(i + 1), -3.1, y, nL, 0, 1.35, 0.4, True))
        pca_pad.append((str(28 - i), 3.1, y, nR, 0, 1.35, 0.4, True))
    b.fp("wifeeder:TSSOP-28", "U3", "PCA9685", pca_x, pca_y, pca_pad, smd=True)
    b.fp("wifeeder:C_0805", "C7", "100n", pca_x - 8, pca_y, c_pads("3V3", "GND"), smd=True)
    b.fp("wifeeder:C_1206", "C8", "10uF", pca_x - 8, pca_y + 4, c_pads("3V3", "GND", True), smd=True)
    b.fp("wifeeder:R_0805", "R6", "4.7k", pca_x + 10, pca_y - 2, r_pads("3V3", "I2C_SCL"), smd=True)
    b.fp("wifeeder:R_0805", "R7", "4.7k", pca_x + 10, pca_y + 2, r_pads("3V3", "I2C_SDA"), smd=True)

    # IBT logic along module
    ibt_nets = ["PWM0", "PWM1", "3V3", "3V3", "IBT_RIS", "IBT_LIS", "5V", "GND"]
    b.fp("wifeeder:PinSocket_1x08", "J_IBT", "IBT logic", 100, 52, sock_1x(8, ibt_nets))
    b.fp("wifeeder:Terminal_2", "TB2", "IBT B+", 100, 88, term(["12V_SAFE", "GND"]))
    b.fp("wifeeder:Terminal_2", "TB3", "MOTOR", 112, 88, term(["MOTOR_P", "MOTOR_N"]))

    # Encoder / DNP
    b.fp("wifeeder:Terminal_4", "TB4", "ENC", 48, 88, term(["5V", "GND", "ENC_A", "ENC_B"]))
    b.fp("wifeeder:R_0805", "R4", "4.7k", 55, 80, r_pads("3V3", "ENC_A"), smd=True)
    b.fp("wifeeder:R_0805", "R5", "4.7k", 62, 80, r_pads("3V3", "ENC_B"), smd=True)
    b.fp("wifeeder:JST_XH_04", "J_RFID", "RFID", 78, 88, [
        ("1", 0, 0, "5V", 0.9, 1.7, 1.7, False),
        ("2", 2.5, 0, "GND", 0.9, 1.7, 1.7, False),
        ("3", 5.0, 0, "RFID_RX", 0.9, 1.7, 1.7, False),
        ("4", 7.5, 0, "", 0.9, 1.7, 1.7, False),
    ])
    b.fp("wifeeder:JST_XH_04", "J_HX711", "HX711", 78, 80, [
        ("1", 0, 0, "3V3", 0.9, 1.7, 1.7, False),
        ("2", 2.5, 0, "GND", 0.9, 1.7, 1.7, False),
        ("3", 5.0, 0, "HX711_DOUT", 0.9, 1.7, 1.7, False),
        ("4", 7.5, 0, "HX711_SCK", 0.9, 1.7, 1.7, False),
    ])

    b.silk(4, 95, "TB1 12V IN")
    b.silk(4, 6, "3V3 buck")
    b.silk(40, 6, "NUCLEO USB →")
    b.silk(50, 52, "PA6 NC")
    b.silk(118, 30, "NRF VCC=5V  ANT↑")
    b.silk(98, 46, "IBT-2 50x50 keepout")
    b.silk(48, 96, "ENC 5V GND A B")
    b.silk(76, 96, "RFID / HX711 DNP")
    b.silk(98, 96, "B+  MOT")

    return b


def write_project(uuids: dict[str, str]) -> None:
    template = Path("/usr/share/kicad/demos/complex_hierarchy/complex_hierarchy.kicad_pro")
    pro = json.loads(template.read_text(encoding="utf-8"))
    pro["meta"]["filename"] = "wifeeder-actuator.kicad_pro"
    pro["sheets"] = [
        [uuids["root"], ""],
        [uuids["power"], "Power"],
        [uuids["mcu"], "MCU / NRF / Enc"],
        [uuids["pca"], "PCA9685"],
        [uuids["ibt"], "IBT-2"],
        [uuids["conn"], "Connectors / mech"],
    ]
    pro.setdefault("libraries", {})
    pro["libraries"]["pinned_footprint_libs"] = ["wifeeder"]
    pro["libraries"]["pinned_symbol_libs"] = ["wifeeder"]
    (ROOT / "wifeeder-actuator.kicad_pro").write_text(json.dumps(pro, indent=2) + "\n")


def write_gerbers(pcb: Pcb) -> None:
    FAB.mkdir(parents=True, exist_ok=True)

    def gerber(name: str, body: str) -> None:
        (FAB / name).write_text(
            "%FSLAX46Y46*%\n%MOMM*%\nG04 WiFeeder actuator *\n"
            + body
            + "M02*\n"
        )

    def mm(v: float) -> int:
        return int(round(v * 1_000_000))

    # Edge
    e = [
        f"X{mm(0)}Y{mm(0)}D02*",
        f"X{mm(BOARD_W)}Y{mm(0)}D01*",
        f"X{mm(BOARD_W)}Y{mm(BOARD_H)}D01*",
        f"X{mm(0)}Y{mm(BOARD_H)}D01*",
        f"X{mm(0)}Y{mm(0)}D01*",
    ]
    gerber("wifeeder-actuator-Edge_Cuts.gm1", "%ADD10C,0.150000*%\nD10*\nG01*\n" + "\n".join(e) + "\n")

    # F.Cu tracks + pads (flashes)
    fc = ["%ADD10C,0.250000*%", "%ADD11C,0.500000*%", "%ADD12C,1.500000*%", "%ADD13C,1.700000*%", "G01*"]
    for x1, y1, x2, y2, w, layer, _net in pcb.tracks:
        if layer != "F.Cu":
            continue
        ap = "D12" if w >= 1.0 else ("D11" if w >= 0.4 else "D10")
        fc.append(f"{ap}*")
        fc.append(f"X{mm(x1)}Y{mm(BOARD_H-y1)}D02*")
        fc.append(f"X{mm(x2)}Y{mm(BOARD_H-y2)}D01*")
    fc.append("D13*")
    for x, y, drill, sx, sy, _n in pcb.th_pads:
        fc.append(f"X{mm(x)}Y{mm(BOARD_H-y)}D03*")
    for x, y, sx, sy, _n, _r in pcb.smd_pads:
        fc.append(f"X{mm(x)}Y{mm(BOARD_H-y)}D03*")
    gerber("wifeeder-actuator-F_Cu.gtl", "\n".join(fc) + "\n")

    bc = ["%ADD10C,0.250000*%", "%ADD13C,1.700000*%", "G01*"]
    for x1, y1, x2, y2, w, layer, _net in pcb.tracks:
        if layer != "B.Cu":
            continue
        bc.append("D10*")
        bc.append(f"X{mm(x1)}Y{mm(BOARD_H-y1)}D02*")
        bc.append(f"X{mm(x2)}Y{mm(BOARD_H-y2)}D01*")
    bc.append("D13*")
    for x, y, drill, sx, sy, _n in pcb.th_pads:
        bc.append(f"X{mm(x)}Y{mm(BOARD_H-y)}D03*")
    gerber("wifeeder-actuator-B_Cu.gbl", "\n".join(bc) + "\n")

    # Mask / silk placeholders
    gerber("wifeeder-actuator-F_Mask.gts", "%ADD10C,2.000000*%\nD10*\n")
    gerber("wifeeder-actuator-B_Mask.gbs", "%ADD10C,2.000000*%\nD10*\n")
    gerber("wifeeder-actuator-F_Silkscreen.gto", "%ADD10C,0.150000*%\nD10*\n")
    gerber("wifeeder-actuator-B_Silkscreen.gbo", "%ADD10C,0.150000*%\nD10*\n")

    # Excellon
    lines = ["M48", "METRIC,TZ", "T1C1.000", "T2C1.600", "T3C1.700", "T4C3.200", "%", "G05"]
    by_d: dict[float, list] = {}
    for x, y, d in pcb.holes:
        by_d.setdefault(round(d, 2), []).append((x, y))
    tool = {1.0: "T1", 1.6: "T2", 1.7: "T3", 3.2: "T4"}
    for d, pts in sorted(by_d.items()):
        t = tool.get(d, "T1")
        lines.append(t)
        for x, y in pts:
            lines.append(f"X{mm(x)}Y{mm(BOARD_H-y)}")
    lines += ["M30"]
    (FAB / "wifeeder-actuator.drl").write_text("\n".join(lines) + "\n")

    (FAB / "README.md").write_text(
        """# Fab outputs (spin 1)

Generated by `pcb/tools/generate_project.py` without KiCad CLI.

| File | Layer |
|------|-------|
| `wifeeder-actuator-F_Cu.gtl` | Front copper |
| `wifeeder-actuator-B_Cu.gbl` | Back copper |
| `wifeeder-actuator-F_Mask.gts` | Front mask |
| `wifeeder-actuator-B_Mask.gbs` | Back mask |
| `wifeeder-actuator-F_Silkscreen.gto` | Front silk |
| `wifeeder-actuator-B_Silkscreen.gbo` | Back silk |
| `wifeeder-actuator-Edge_Cuts.gm1` | Outline 150×100 mm |
| `wifeeder-actuator.drl` | Excellon |

**Preferred:** open `wifeeder-actuator.kicad_pro` in KiCad 8 → PCB Editor → Fill zones → DRC → File → Fabrication Outputs.
These generated gerbers are a **preview / backup**; zone fills are not poured here.

Stackup: 2-layer, 1.6 mm, 2 oz Cu if quoted, ENIG or HASL, min track 0.2 mm.
"""
    )


def write_erc_notes() -> None:
    (ROOT / "ERC_DRC.md").write_text(
        """# ERC / DRC notes (spin 1)

KiCad is not required to *edit* these files, but **zone fill + official DRC** should be run in KiCad 8 before ordering.

## Intentional (exclude in ERC if flagged)

| Item | Why |
|------|-----|
| PA6_NC no-connect | Nucleo PA6 shorted to GND on the module — do not route |
| J_NRF pin 8 (IRQ) nc | Firmware does not use IRQ |
| LED2–15 of PCA9685 unconnected | No Motor2; pads exist for probing |
| J_MINI / SJ_MINI open | Discrete bucks stuffed; Mini360 is fallback |
| NUC_5V, NUC_VIN unconnected by default | Carrier injects **3V3** only; USB unplugged |
| IBT_RIS / IBT_LIS nc | Current-sense unused |
| MOTOR_P/N not tied to IBT pads | IBT M+/M− are module screws; TB3 is GX fly-wire |

## Must pass

- Clearance ≥ 0.2 mm (power ≥ 0.4 mm)
- 12V_SAFE pour does not enter NRF keepout (118,2)–(149,28)
- Nucleo 3V3 and GND each have exactly the intended sockets
- NRF VCC net is **5V**, never 3V3

## Mechanical lock (GX)

Chassis GX-12 / GX-16 series was **not measured** on the bench. Spin 1 uses **5.08 mm Phoenix-style screws** (TB3 motor, TB4 encoder). Panel aviation plugs fly-wire ≤ 20 mm. Revisit footprints if you measure GX12-2 / GX16-4.
"""
    )


def main() -> None:
    write_footprints()
    write_symbol_lib()
    lib_text = (LIBS / "wifeeder.kicad_sym").read_text()
    uuids = build_schematics(lib_text)
    write_project(uuids)
    pcb = build_pcb()
    pcb.write(ROOT / "wifeeder-actuator.kicad_pcb")
    write_erc_notes()
    print("wrote", ROOT)
    print("footprints", len(list(PRETTY.glob("*.kicad_mod"))))
    print("NOTE: placement/schematic only — run route_pcb.py then export_fab.py for copper + gerbers")


if __name__ == "__main__":
    main()
