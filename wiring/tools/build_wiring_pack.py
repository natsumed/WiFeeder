#!/usr/bin/env python3
"""
Build WiFeeder v2 Fritzing wiring pack:
  - package community/core parts as .fzpz
  - synthesize custom .fzpz (IBT-2, GX-2/4, NRF AM1117, Mini360, RFID, encoder)
  - write wifeeder-v2.fzz (parts + netlist notes)
  - write breadboard + schematic SVG exports
"""
from __future__ import annotations

import os
import re
import shutil
import zipfile
from pathlib import Path
from textwrap import dedent
from xml.sax.saxutils import escape

ROOT = Path(__file__).resolve().parents[1]  # wiring/
PARTS = ROOT / "parts"
EXPORTS = ROOT / "exports"
FZ_HOME = Path.home() / "Applications" / "fritzing"
FZ_PARTS = FZ_HOME / "fritzing-parts"


def ensure_dirs() -> None:
    PARTS.mkdir(parents=True, exist_ok=True)
    EXPORTS.mkdir(parents=True, exist_ok=True)
    (PARTS / "src").mkdir(exist_ok=True)


def make_pin_svg(name: str, pins: list[str], width: float = 120, view: str = "breadboard") -> str:
    """Simple rectangular part SVG with labeled pins along the bottom."""
    n = max(1, len(pins))
    pitch = 10.0
    body_w = max(width, n * pitch + 20)
    body_h = 48.0 if view == "breadboard" else 56.0
    h = body_h + 28
    pin_y = body_h + 4
    rect_fill = "#2b6cb0" if view == "breadboard" else "#ffffff"
    stroke = "#1a202c"
    text_fill = "#ffffff" if view == "breadboard" else "#000000"
    lines = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{body_w}mm" height="{h}mm" '
        f'viewBox="0 0 {body_w} {h}" version="1.1">',
        f'<rect id="board" x="2" y="2" width="{body_w - 4}" height="{body_h}" '
        f'rx="2" fill="{rect_fill}" stroke="{stroke}" stroke-width="0.8"/>',
        f'<text x="{body_w / 2}" y="16" text-anchor="middle" font-family="sans-serif" '
        f'font-size="6" fill="{text_fill}">{escape(name)}</text>',
    ]
    for i, pin in enumerate(pins):
        x = 12 + i * pitch
        cid = f"connector{i}"
        lines.append(
            f'<circle id="{cid}pin" cx="{x}" cy="{pin_y}" r="1.6" fill="#d69e2e" stroke="{stroke}" stroke-width="0.3"/>'
        )
        if view == "schematic":
            lines.append(
                f'<rect id="{cid}terminal" x="{x - 0.5}" y="{pin_y - 0.5}" width="1" height="1" fill="none"/>'
            )
        lines.append(
            f'<text x="{x}" y="{body_h - 4}" text-anchor="middle" font-family="sans-serif" '
            f'font-size="3.2" fill="{text_fill}">{escape(pin)}</text>'
        )
        if view == "pcb":
            lines.append(
                f'<circle id="{cid}pad" cx="{x}" cy="{pin_y}" r="1.8" fill="none" stroke="#f7931e" stroke-width="0.4"/>'
            )
    lines.append("</svg>")
    return "\n".join(lines)


def make_fzp(module_id: str, title: str, pins: list[str], family: str, description: str) -> str:
    connectors = []
    for i, pin in enumerate(pins):
        cid = f"connector{i}"
        connectors.append(
            dedent(
                f"""\
          <connector id="{cid}" type="male" name="{escape(pin)}">
           <description>{escape(pin)}</description>
           <views>
            <breadboardView><p svgId="{cid}pin" layer="breadboard"/></breadboardView>
            <schematicView><p svgId="{cid}pin" terminalId="{cid}terminal" layer="schematic"/></schematicView>
            <pcbView>
             <p svgId="{cid}pad" layer="copper0"/>
             <p svgId="{cid}pad" layer="copper1"/>
            </pcbView>
           </views>
          </connector>"""
            )
        )
    conn_xml = "\n".join(connectors)
    bb = f"breadboard/{module_id}_breadboard.svg"
    sch = f"schematic/{module_id}_schematic.svg"
    pcb = f"pcb/{module_id}_pcb.svg"
    icon = f"icon/{module_id}_icon.svg"
    return dedent(
        f"""\
        <?xml version="1.0" encoding="UTF-8"?>
        <module fritzingVersion="0.9.6" moduleId="{module_id}" referenceFile="{module_id}.fzp">
         <version>1</version>
         <author>WiFeeder v2</author>
         <title>{escape(title)}</title>
         <label>U</label>
         <date>2026-08-03</date>
         <tags><tag>wifeeder</tag></tags>
         <properties>
          <property name="family">{escape(family)}</property>
          <property name="variant">wifeeder-v2</property>
         </properties>
         <description>{escape(description)}</description>
         <views>
          <iconView><layers image="{icon}"><layer layerId="icon"/></layers></iconView>
          <breadboardView><layers image="{bb}"><layer layerId="breadboard"/></layers></breadboardView>
          <schematicView><layers image="{sch}"><layer layerId="schematic"/></layers></schematicView>
          <pcbView>
           <layers image="{pcb}">
            <layer layerId="copper0"/><layer layerId="silkscreen"/><layer layerId="copper1"/>
           </layers>
          </pcbView>
         </views>
         <connectors>
        {conn_xml}
         </connectors>
        </module>
        """
    ).lstrip()


def write_fzpz(module_id: str, title: str, pins: list[str], family: str, description: str) -> Path:
    out = PARTS / f"{module_id}.fzpz"
    fzp = make_fzp(module_id, title, pins, family, description)
    svgs = {
        f"svg.breadboard.{module_id}_breadboard.svg": make_pin_svg(title, pins, view="breadboard"),
        f"svg.schematic.{module_id}_schematic.svg": make_pin_svg(title, pins, view="schematic"),
        f"svg.pcb.{module_id}_pcb.svg": make_pin_svg(title, pins, view="pcb"),
        f"svg.icon.{module_id}_icon.svg": make_pin_svg(title, pins, view="breadboard"),
        f"part.{module_id}.fzp": fzp,
    }
    with zipfile.ZipFile(out, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for name, data in svgs.items():
            zf.writestr(name, data)
    return out


def _normalize_fzp_images(fzp_bytes: bytes) -> bytes:
    """Ensure image= paths use view/filename form inside fzpz."""
    text = fzp_bytes.decode("utf-8", errors="ignore")

    def repl(m: re.Match[str]) -> str:
        img = m.group(1)
        name = Path(img).name
        if "/" in img:
            return m.group(0)
        if "_breadboard" in name:
            view = "breadboard"
        elif "_schematic" in name:
            view = "schematic"
        elif "_pcb" in name:
            view = "pcb"
        elif "_icon" in name:
            view = "icon"
        else:
            view = "breadboard"
        return f'image="{view}/{name}"'

    text = re.sub(r'image="([^"]+)"', repl, text)
    return text.encode("utf-8")


def package_from_tree(fzp_path: Path, svg_root: Path, out_name: str) -> Path | None:
    """Package an existing fzp + svg tree into fzpz."""
    if not fzp_path.is_file():
        return None
    fzp_bytes = _normalize_fzp_images(fzp_path.read_bytes())
    text = fzp_bytes.decode("utf-8", errors="ignore")
    images = re.findall(r'image="([^"]+)"', text)
    out = PARTS / out_name
    with zipfile.ZipFile(out, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        zf.writestr(f"part.{fzp_path.stem}.fzp", fzp_bytes)
        for img in sorted(set(images)):
            src = svg_root / img
            if not src.is_file():
                flat = svg_root / Path(img).name
                if flat.is_file():
                    src = flat
                else:
                    found = list(svg_root.rglob(Path(img).name))
                    src = found[0] if found else None
            if src and src.is_file():
                fname = Path(img).name
                view = img.split("/")[0] if "/" in img else "breadboard"
                zf.write(src, f"svg.{view}.{fname}")
    return out


def package_nucleo() -> Path | None:
    vendor = PARTS / "src" / "nucleo32"
    fzp = vendor / "STM32_Nucleo-32_board.fzp"
    if fzp.is_file():
        return package_from_tree(fzp, vendor, "STM32_Nucleo-32_board.fzpz")
    return package_from_tree(
        Path("/tmp/Fritzing-components/core/STM32_Nucleo-32_board.fzp"),
        Path("/tmp/Fritzing-components/svg"),
        "STM32_Nucleo-32_board.fzpz",
    )


def package_core(relative_fzp: str, out_name: str) -> Path | None:
    fzp = FZ_PARTS / "core" / relative_fzp
    return package_from_tree(fzp, FZ_PARTS / "svg", out_name)


def build_custom_parts() -> list[Path]:
    parts = []
    specs = [
        (
            "IBT2_BTS7960_wifeeder",
            "IBT-2 BTS7960 Dual H-Bridge",
            [
                "RPWM1",
                "LPWM1",
                "R_EN1",
                "L_EN1",
                "RPWM2",
                "LPWM2",
                "R_EN2",
                "L_EN2",
                "M1+",
                "M1-",
                "M2+",
                "M2-",
                "B+",
                "B-",
                "5VOUT",
                "GND",
            ],
            "motor driver",
            "IBT-2 dual BTS7960 module for WiFeeder v2. EN pins tie to 3.3V.",
        ),
        (
            "GX2_motor_wifeeder",
            "GX-2 Motor Connector",
            ["1_Mplus", "2_Mminus"],
            "connector",
            "Provisional GX 2-pin aviation connector for PN01007BRKT motor.",
        ),
        (
            "GX4_encoder_wifeeder",
            "GX-4 Encoder Connector",
            ["1_VCC", "2_GND", "3_A", "4_B"],
            "connector",
            "Provisional GX 4-pin aviation connector for GTS06 encoder. Verify continuity.",
        ),
        (
            "NRF24_AM1117_adapter_wifeeder",
            "NRF24 PA+LNA AM1117 Adapter",
            ["VCC_5V", "GND", "CE", "CSN", "SCK", "MOSI", "MISO", "IRQ"],
            "2.4ghz transceiver",
            "PA+LNA NRF24 module on AM1117 adapter. Adapter VCC must be 5V.",
        ),
        (
            "Mini360_buck_wifeeder",
            "Mini360 Buck Converter",
            ["IN+", "IN-", "OUT+", "OUT-"],
            "power",
            "Mini360 DC-DC buck. 12V in → 3.3V out for Nucleo/HX711.",
        ),
        (
            "RFID_125kHz_UART_wifeeder",
            "RFID 125kHz UART Reader",
            ["VCC", "GND", "TX"],
            "rfid",
            "EM4100-style 125 kHz reader. TX → Nucleo PA3 (USART2).",
        ),
        (
            "GTS06_encoder_wifeeder",
            "GTS06-OC-RA600A-2M Encoder",
            ["VCC", "GND", "A", "B"],
            "sensor",
            "600 P/R NPN open-collector rotary encoder. Needs 3.3V pull-ups on A/B.",
        ),
        (
            "PN01007BRKT_motor_wifeeder",
            "PN01007BRKT 50RPM 12V Gear Motor",
            ["M+", "M-"],
            "electromechanical",
            "12V 50 RPM DC gear motor with bracket.",
        ),
    ]
    for args in specs:
        parts.append(write_fzpz(*args))
    return parts


def build_downloaded_parts() -> list[Path]:
    built = []
    nucleo = package_nucleo()
    if nucleo:
        built.append(nucleo)
    for rel, name in [
        ("NRF24L01+_breakout.fzp", "NRF24L01+_breakout.fzpz"),
        ("hx711_weightsensor.fzp", "hx711_weightsensor.fzpz"),
        ("Raspberry_Pi_2_v1.1.fzp", "Raspberry_Pi_2_v1.1.fzpz"),
        ("dc_motor.fzp", "dc_motor.fzpz"),
        ("gear-motor_2.fzp", "gear-motor_2.fzpz"),
        ("resistor.fzp", "resistor.fzpz"),
        ("Battery block 9V.fzp", "Battery_block_9V.fzpz"),
    ]:
        p = package_core(rel, name)
        if p:
            built.append(p)
    return built


def note_xml(model_index: int, title: str, text: str, x: float, y: float) -> str:
    # NoteModuleID is built-in; keep path empty-ish for core
    body = escape(text).replace("\n", "&#10;")
    return dedent(
        f"""\
        <instance modelIndex="{model_index}" moduleIdRef="NoteModuleID" path=":/resources/parts/core/note.fzp">
          <title>{escape(title)}</title>
          <views>
            <breadboardView layer="breadboard">
              <geometry x="{x}" y="{y}" z="5"/>
              <text fontSize="9" color="#000000">{body}</text>
            </breadboardView>
            <schematicView layer="schematic">
              <geometry x="{x}" y="{y}" z="5"/>
              <text fontSize="9" color="#000000">{body}</text>
            </schematicView>
          </views>
        </instance>"""
    )


def part_instance(model_index: int, module_id: str, title: str, x: float, y: float, fzpz: str) -> str:
    return dedent(
        f"""\
        <instance modelIndex="{model_index}" moduleIdRef="{module_id}" path="parts/{fzpz}">
          <title>{escape(title)}</title>
          <views>
            <breadboardView layer="breadboard">
              <geometry x="{x}" y="{y}" z="2"/>
            </breadboardView>
            <schematicView layer="schematic">
              <geometry x="{x}" y="{y + 400}" z="2"/>
            </schematicView>
          </views>
        </instance>"""
    )


def build_fzz() -> Path:
    """Create main sketch: custom parts placed + authoritative netlist notes."""
    netlist = dedent(
        """\
        WiFeeder v2 MAIN WIRING (see CONNECTOR_MAP.md)

        POWER
        12V BAT → IBT-2 B+ / Mini360 IN+
        BAT- → common GND
        Mini360 OUT → Nucleo 3V3 rail + HX711 VCC (set to 3.3V)

        NRF STM (AM1117 adapter VCC=5V!)
        5V→VCC  GND→GND
        D3/PB0→CE  A3/PA4→CSN  A4/PA5→SCK
        A6/PA7→MOSI  D6/PB1→MISO
        NEVER use PA6 (shorted)

        NRF Pi (AM1117 VCC=5V)
        pin2/4→VCC  pin6→GND
        23→SCK  19→MOSI  21→MISO
        24→CSN  22/GPIO25→CE

        IBT-2 M1: PA8→RPWM1  PA9→LPWM1  EN→3V3
        IBT-2 M2: PB6→RPWM2  PB7→LPWM2  EN→3V3
        IBT M1+/- → GX2-1 → PN01007BRKT
        IBT M2+/- → GX2-2 → PN01007BRKT

        ENC1 GTS06 via GX4: VCC←IBT 5VOUT  GND common
        A→PA0  B→PA1  + 4.7k pull-up to 3V3 each
        ENC2 (Phase3b): A→PA11 B→PA12 same pull-ups

        RFID TX→PA3  VCC 5V  GND
        HX711 DOUT→PB4 SCK→PB5 VCC 3V3
        """
    )
    instances = [
        note_xml(1, "NETLIST", netlist, 20, 20),
        part_instance(10, "STM32_Nucleo-32_board", "NUCLEO-L432KC", 40, 220, "STM32_Nucleo-32_board.fzpz"),
        part_instance(11, "NRF24_AM1117_adapter_wifeeder", "NRF STM", 280, 220, "NRF24_AM1117_adapter_wifeeder.fzpz"),
        part_instance(12, "IBT2_BTS7960_wifeeder", "IBT-2", 40, 360, "IBT2_BTS7960_wifeeder.fzpz"),
        part_instance(13, "Mini360_buck_wifeeder", "Mini360", 280, 360, "Mini360_buck_wifeeder.fzpz"),
        part_instance(14, "GX2_motor_wifeeder", "GX2 Mot1", 40, 500, "GX2_motor_wifeeder.fzpz"),
        part_instance(15, "GX2_motor_wifeeder", "GX2 Mot2", 140, 500, "GX2_motor_wifeeder.fzpz"),
        part_instance(16, "PN01007BRKT_motor_wifeeder", "Motor1", 40, 580, "PN01007BRKT_motor_wifeeder.fzpz"),
        part_instance(17, "PN01007BRKT_motor_wifeeder", "Motor2", 140, 580, "PN01007BRKT_motor_wifeeder.fzpz"),
        part_instance(18, "GX4_encoder_wifeeder", "GX4 Enc1", 280, 500, "GX4_encoder_wifeeder.fzpz"),
        part_instance(19, "GX4_encoder_wifeeder", "GX4 Enc2", 400, 500, "GX4_encoder_wifeeder.fzpz"),
        part_instance(20, "GTS06_encoder_wifeeder", "Enc1 600P/R", 280, 580, "GTS06_encoder_wifeeder.fzpz"),
        part_instance(21, "GTS06_encoder_wifeeder", "Enc2 600P/R", 400, 580, "GTS06_encoder_wifeeder.fzpz"),
        part_instance(22, "RFID_125kHz_UART_wifeeder", "RFID", 520, 220, "RFID_125kHz_UART_wifeeder.fzpz"),
        part_instance(23, "hx711_weightsensor", "HX711", 520, 320, "hx711_weightsensor.fzpz"),
        part_instance(24, "Raspberry_Pi_2_v1.1", "Raspberry Pi 2", 700, 220, "Raspberry_Pi_2_v1.1.fzpz"),
        part_instance(25, "NRF24_AM1117_adapter_wifeeder", "NRF Pi", 700, 420, "NRF24_AM1117_adapter_wifeeder.fzpz"),
    ]
    fz = dedent(
        f"""\
        <?xml version="1.0" encoding="UTF-8"?>
        <module fritzingVersion="0.9.6" icon=".png">
         <title>WiFeeder v2 Full Wiring</title>
         <label>WiFeeder</label>
         <description>STM32 actuator + Raspberry Pi host wiring. Import parts from wiring/parts/ first. Full netlist in CONNECTOR_MAP.md and breadboard note.</description>
         <views>
          <view name="breadboardView" backgroundColor="#f7fafc" gridSize="0.1in" showGrid="1" alignToGrid="0"/>
          <view name="schematicView" backgroundColor="#ffffff" gridSize="0.1in" showGrid="1" alignToGrid="1"/>
          <view name="pcbView" backgroundColor="#333333" gridSize="0.05in" showGrid="1" alignToGrid="1"/>
         </views>
         <instances>
        {chr(10).join(instances)}
         </instances>
        </module>
        """
    ).lstrip()
    out = ROOT / "wifeeder-v2.fzz"
    with zipfile.ZipFile(out, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        zf.writestr("wifeeder-v2.fz", fz)
        # Bundle custom + packaged parts so the sketch is self-contained enough to import
        for fzpz in PARTS.glob("*.fzpz"):
            zf.write(fzpz, f"parts/{fzpz.name}")
    return out


def svg_box(x, y, w, h, title, fill="#edf2f7", stroke="#2d3748"):
    return (
        f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="8" fill="{fill}" stroke="{stroke}" stroke-width="2"/>'
        f'<text x="{x + 12}" y="{y + 22}" font-family="DejaVu Sans,sans-serif" font-size="14" font-weight="700" fill="#1a202c">{escape(title)}</text>'
    )


def svg_line(x1, y1, x2, y2, color="#3182ce", label=""):
    mid_x = (x1 + x2) / 2
    mid_y = (y1 + y2) / 2 - 6
    s = f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{color}" stroke-width="2"/>'
    if label:
        s += (
            f'<text x="{mid_x}" y="{mid_y}" text-anchor="middle" font-family="DejaVu Sans,sans-serif" '
            f'font-size="9" fill="{color}">{escape(label)}</text>'
        )
    return s


def svg_text_block(x, y, lines, size=11, fill="#2d3748"):
    parts = []
    for i, line in enumerate(lines):
        parts.append(
            f'<text x="{x}" y="{y + i * (size + 4)}" font-family="DejaVu Sans Mono,monospace" '
            f'font-size="{size}" fill="{fill}">{escape(line)}</text>'
        )
    return "\n".join(parts)


def build_breadboard_svg() -> Path:
    w, h = 1400, 900
    elems = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">',
        '<rect width="100%" height="100%" fill="#f7fafc"/>',
        '<text x="40" y="36" font-family="DejaVu Sans,sans-serif" font-size="22" font-weight="700" fill="#1a202c">WiFeeder v2 — Full wiring (breadboard view)</text>',
        '<text x="40" y="58" font-family="DejaVu Sans,sans-serif" font-size="12" fill="#4a5568">Authority: wiring/CONNECTOR_MAP.md · NRF adapter VCC = 5V · PA6 unused</text>',
        # islands
        svg_box(30, 80, 420, 280, "STM32 NUCLEO-L432KC", "#bee3f8"),
        svg_text_block(
            48,
            120,
            [
                "NRF CE  D3/PB0",
                "NRF CSN A3/PA4",
                "NRF SCK A4/PA5",
                "NRF MOSI A6/PA7",
                "NRF MISO D6/PB1",
                "RFID RX PA3",
                "ENC1 A/B  PA0/PA1",
                "ENC2 A/B  PA11/PA12",
                "M1 PWM/DIR PA8/PA9",
                "M2 PWM/DIR PB6/PB7",
                "HX711 PB4/PB5",
                "3V3 from Mini360",
            ],
        ),
        svg_box(480, 80, 280, 160, "NRF STM (AM1117)", "#feebc8"),
        svg_text_block(498, 120, ["VCC ← Nucleo 5V", "GND ← GND", "CE/CSN/SCK/MOSI/MISO", "as listed at left"], 12),
        svg_box(800, 80, 280, 200, "Raspberry Pi 2 + NRF", "#c6f6d5"),
        svg_text_block(
            818,
            120,
            [
                "NRF VCC ← pin 2/4 (5V)",
                "GND ← pin 6",
                "SCK←23 MOSI←19 MISO←21",
                "CSN←24 CE←22 (GPIO25)",
                "2.4 GHz ↔ STM NRF",
            ],
            12,
        ),
        svg_box(30, 390, 360, 200, "IBT-2 BTS7960", "#faf089"),
        svg_text_block(
            48,
            430,
            [
                "B+ ← 12V battery",
                "B-/GND common",
                "R_EN/L_EN ← 3V3",
                "RPWM1←PA8 LPWM1←PA9",
                "RPWM2←PB6 LPWM2←PB7",
                "5VOUT → encoder VCC",
                "M1+/- → GX2 Mot1",
                "M2+/- → GX2 Mot2",
            ],
            12,
        ),
        svg_box(420, 390, 260, 200, "GX + Motors", "#e9d8fd"),
        svg_text_block(
            438,
            430,
            [
                "GX2 Mot1: 1=M+ 2=M−",
                "→ PN01007BRKT #1",
                "GX2 Mot2: 1=M+ 2=M−",
                "→ PN01007BRKT #2",
                "12V 50 RPM gear motors",
            ],
            12,
        ),
        svg_box(710, 390, 340, 220, "Encoders GTS06 via GX4", "#fed7e2"),
        svg_text_block(
            728,
            430,
            [
                "GTS06-OC-RA600A-2M 600P/R NPN",
                "GX4: 1=VCC 2=GND 3=A 4=B",
                "VCC ← IBT 5VOUT",
                "Enc1 A→PA0 B→PA1",
                "Enc2 A→PA11 B→PA12",
                "4.7k pull-up A/B → 3V3",
                "VERIFY GX pin ↔ wire continuity",
            ],
            12,
        ),
        svg_box(30, 620, 300, 140, "Mini360 + Power", "#b2f5ea"),
        svg_text_block(48, 660, ["12V → IN+", "GND → IN−", "OUT+ = 3.3V to Nucleo/HX711", "Common ground star"], 12),
        svg_box(360, 620, 280, 140, "RFID + HX711", "#fbd38d"),
        svg_text_block(378, 660, ["RFID VCC 5V TX→PA3 GND", "HX711 VCC 3V3", "DOUT→PB4 SCK→PB5"], 12),
        svg_box(670, 640, 380, 120, "Critical warnings", "#feb2b2"),
        svg_text_block(
            688,
            680,
            [
                "• NRF AM1117 VCC = 5V (SPI OK ≠ RF OK on 3.3V)",
                "• Do not wire PA6 (shorted on this Nucleo)",
                "• NPN encoder outputs need 3.3V pull-ups",
            ],
            12,
        ),
        # decorative link lines
        svg_line(450, 220, 480, 160, "#dd6b20", "SPI"),
        svg_line(760, 160, 800, 160, "#38a169", "2.4GHz"),
        svg_line(200, 360, 200, 390, "#d69e2e", "PWM/DIR"),
        svg_line(390, 490, 420, 490, "#805ad5", "M+/-"),
        svg_line(450, 360, 780, 390, "#d53f8c", "A/B"),
        "</svg>",
    ]
    path = EXPORTS / "wifeeder-v2-breadboard.svg"
    path.write_text("\n".join(elems), encoding="utf-8")
    return path


def build_schematic_svg() -> Path:
    w, h = 1200, 1000
    elems = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">',
        '<rect width="100%" height="100%" fill="#ffffff"/>',
        '<text x="40" y="36" font-family="DejaVu Sans,sans-serif" font-size="22" font-weight="700">WiFeeder v2 — Schematic net map</text>',
        svg_text_block(
            40,
            70,
            [
                "BAT_12V ──┬── IBT2.B+",
                "          └── Mini360.IN+ ── OUT+ ── Nucleo.3V3 / HX711.VCC",
                "GND ──────── common (IBT B-, Mini360 IN-, Nucleo GND, Pi GND, NRF GND, Enc GND)",
                "",
                "Nucleo.5V ── NRF_STM.VCC(AM1117)     Pi.5V(pin2) ── NRF_PI.VCC(AM1117)",
                "Nucleo.D3 ── NRF_STM.CE              Pi.GPIO25(pin22) ── NRF_PI.CE",
                "Nucleo.A3 ── NRF_STM.CSN             Pi.GPIO8(pin24) ── NRF_PI.CSN",
                "Nucleo.A4 ── NRF_STM.SCK             Pi.GPIO11(pin23) ── NRF_PI.SCK",
                "Nucleo.A6 ── NRF_STM.MOSI            Pi.GPIO10(pin19) ── NRF_PI.MOSI",
                "Nucleo.D6 ── NRF_STM.MISO            Pi.GPIO9(pin21) ── NRF_PI.MISO",
                "",
                "Nucleo.PA8 ── IBT.RPWM1     Nucleo.PA9 ── IBT.LPWM1     3V3 ── IBT.R_EN1/L_EN1",
                "Nucleo.PB6 ── IBT.RPWM2     Nucleo.PB7 ── IBT.LPWM2     3V3 ── IBT.R_EN2/L_EN2",
                "IBT.M1+ ── GX2_1.1 ── Motor1.M+      IBT.M1- ── GX2_1.2 ── Motor1.M-",
                "IBT.M2+ ── GX2_2.1 ── Motor2.M+      IBT.M2- ── GX2_2.2 ── Motor2.M-",
                "",
                "IBT.5VOUT ── GX4_1.1 / GX4_2.1 ── Enc.VCC",
                "GX4_1.3 ── 4k7↑3V3 ── Nucleo.PA0 (Enc1 A)",
                "GX4_1.4 ── 4k7↑3V3 ── Nucleo.PA1 (Enc1 B)",
                "GX4_2.3 ── 4k7↑3V3 ── Nucleo.PA11 (Enc2 A)",
                "GX4_2.4 ── 4k7↑3V3 ── Nucleo.PA12 (Enc2 B)",
                "",
                "RFID.TX ── Nucleo.PA3     RFID.VCC ── 5V     HX711.DOUT──PB4  SCK──PB5",
                "",
                "Motor: PN01007BRKT 50RPM 12V    Encoder: GTS06-OC-RA600A-2M 600P/R NPN",
            ],
            13,
        ),
        "</svg>",
    ]
    path = EXPORTS / "wifeeder-v2-schematic.svg"
    path.write_text("\n".join(elems), encoding="utf-8")
    return path


def export_pngs(svgs: list[Path]) -> None:
    for svg in svgs:
        png = svg.with_suffix(".png")
        # inkscape CLI
        cmd = f'inkscape "{svg}" --export-type=png --export-filename="{png}" -w 1600 2>/dev/null'
        rc = os.system(cmd)
        if rc != 0 or not png.is_file():
            os.system(f'convert "{svg}" "{png}" 2>/dev/null')


def write_parts_readme(custom: list[Path], downloaded: list[Path]) -> None:
    lines = [
        "# Fritzing parts for WiFeeder v2",
        "",
        "Import each `.fzpz` into Fritzing: Parts panel → menu → **Import…**",
        "",
        "Fritzing app (this machine): `~/Applications/fritzing/Fritzing`",
        "",
        "## Custom parts (generated)",
        "",
        "| File | Purpose |",
        "|------|---------|",
    ]
    for p in custom:
        lines.append(f"| `{p.name}` | custom WiFeeder part |")
    lines += ["", "## Packaged from community / Fritzing core", ""]
    lines += ["| File | Source |", "|------|--------|"]
    src_notes = {
        "STM32_Nucleo-32_board.fzpz": "https://github.com/Blacksocks/Fritzing-components (Nucleo-32)",
        "NRF24L01+_breakout.fzpz": "Fritzing core parts (bundled with app)",
        "hx711_weightsensor.fzpz": "Fritzing core parts",
        "Raspberry_Pi_2_v1.1.fzpz": "Fritzing core parts",
        "dc_motor.fzp": "Fritzing core",
        "dc_motor.fzpz": "Fritzing core",
        "gear-motor_2.fzpz": "Fritzing core",
        "resistor.fzpz": "Fritzing core",
        "Battery_block_9V.fzpz": "Fritzing core (stand-in for 12V pack label)",
    }
    for p in downloaded:
        lines.append(f"| `{p.name}` | {src_notes.get(p.name, 'see build script')} |")
    lines += [
        "",
        "## Rebuild",
        "",
        "```bash",
        "python3 wiring/tools/build_wiring_pack.py",
        "```",
        "",
    ]
    (PARTS / "README.md").write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    ensure_dirs()
    custom = build_custom_parts()
    downloaded = build_downloaded_parts()
    write_parts_readme(custom, downloaded)
    fzz = build_fzz()
    bb = build_breadboard_svg()
    sch = build_schematic_svg()
    export_pngs([bb, sch])
    print("Custom parts:", len(custom))
    print("Downloaded parts:", len(downloaded))
    print("Sketch:", fzz)
    print("Exports:", list(EXPORTS.glob("*")))


if __name__ == "__main__":
    main()
