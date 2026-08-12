#!/usr/bin/env python3
"""Install wiring/parts/*.fzpz into ~/Documents/Fritzing and rewrite wifeeder-v2.fzz."""
from __future__ import annotations

import re
import shutil
import tempfile
import zipfile
from pathlib import Path
from textwrap import dedent
from xml.sax.saxutils import escape

ROOT = Path(__file__).resolve().parents[1]
PARTS_SRC = ROOT / "parts"
FZ_USER = Path.home() / "Documents" / "Fritzing"
USER_PARTS = FZ_USER / "parts" / "user"
SVG_USER = FZ_USER / "parts" / "svg" / "user"
BIN_PATH = FZ_USER / "bins" / "my_parts.fzb"


def normalize_fzp_images(text: str) -> str:
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

    return re.sub(r'image="([^"]+)"', repl, text)


def install_fzpz(fzpz: Path) -> str:
    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        with zipfile.ZipFile(fzpz) as zf:
            zf.extractall(tmp_path)
        fzp = next(tmp_path.glob("part.*.fzp"))
        stem = fzp.name[len("part.") : -len(".fzp")]
        USER_PARTS.mkdir(parents=True, exist_ok=True)
        for view in ("breadboard", "schematic", "pcb", "icon"):
            (SVG_USER / view).mkdir(parents=True, exist_ok=True)
        dest_fzp = USER_PARTS / f"{stem}.fzp"
        dest_fzp.write_text(normalize_fzp_images(fzp.read_text(encoding="utf-8", errors="ignore")))
        for svg in tmp_path.glob("svg.*"):
            # svg.breadboard.NAME.svg
            parts = svg.name.split(".", 2)
            if len(parts) < 3:
                continue
            _, view, fname = parts
            if view in ("breadboard", "schematic", "pcb", "icon"):
                shutil.copy2(svg, SVG_USER / view / fname)
        print("installed", stem)
        return stem


def update_my_parts(stems: list[str]) -> None:
    instances = []
    for stem in stems:
        fzp = USER_PARTS / f"{stem}.fzp"
        text = fzp.read_text(encoding="utf-8", errors="ignore")
        mid_m = re.search(r'moduleId="([^"]+)"', text)
        mid = mid_m.group(1) if mid_m else stem
        title_m = re.search(r"<title>([^<]+)</title>", text)
        title = title_m.group(1) if title_m else stem
        instances.append(
            f'    <instance moduleIdRef="{mid}" path="user/{fzp.name}">\n'
            f"        <title>{title}</title>\n"
            f"    </instance>"
        )
    BIN_PATH.parent.mkdir(parents=True, exist_ok=True)
    BIN_PATH.write_text(
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<module fritzingVersion="0.9.6" icon="Mine.png">\n'
        "    <title>My Parts</title>\n"
        "    <instances>\n"
        + "\n".join(instances)
        + "\n    </instances>\n</module>\n"
    )
    print("my_parts.fzb updated:", len(instances), "parts")


def module_id(stem: str) -> str:
    text = (USER_PARTS / f"{stem}.fzp").read_text(encoding="utf-8", errors="ignore")
    m = re.search(r'moduleId="([^"]+)"', text)
    return m.group(1) if m else stem


def note_xml(model_index: int, title: str, text: str, x: float, y: float) -> str:
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


def part_instance(model_index: int, module_id: str, title: str, x: float, y: float, fzp_name: str) -> str:
    path = USER_PARTS / fzp_name
    return dedent(
        f"""\
        <instance modelIndex="{model_index}" moduleIdRef="{module_id}" path="{path}">
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


def rewrite_fzz() -> None:
    netlist = (
        "WiFeeder v2 MAIN WIRING (see CONNECTOR_MAP.md)\n\n"
        "POWER: 12V BAT -> IBT B+ / Mini360 IN+; common GND\n"
        "Mini360 OUT -> Nucleo 3V3 + HX711\n\n"
        "NRF STM AM1117 VCC=5V: D3 CE, A3 CSN, A4 SCK, A6 MOSI, D6 MISO\n"
        "NEVER use PA6\n\n"
        "NRF Pi AM1117 VCC=5V: pin2/4 VCC, 6 GND, 23 SCK, 19 MOSI, 21 MISO, 24 CSN, 22 CE\n\n"
        "IBT M1 PA8/PA9, M2 PB6/PB7, EN->3V3\n"
        "Motors GX2, Encoders GTS06 GX4 + 4k7 to 3V3\n"
        "RFID TX->PA3, HX711 PB4/PB5"
    )
    instances = [
        note_xml(1, "NETLIST", netlist, 20, 20),
        part_instance(10, module_id("STM32_Nucleo-32_board"), "NUCLEO-L432KC", 40, 220, "STM32_Nucleo-32_board.fzp"),
        part_instance(11, module_id("NRF24_AM1117_adapter_wifeeder"), "NRF STM", 280, 220, "NRF24_AM1117_adapter_wifeeder.fzp"),
        part_instance(12, module_id("IBT2_BTS7960_wifeeder"), "IBT-2", 40, 360, "IBT2_BTS7960_wifeeder.fzp"),
        part_instance(13, module_id("Mini360_buck_wifeeder"), "Mini360", 280, 360, "Mini360_buck_wifeeder.fzp"),
        part_instance(14, module_id("GX2_motor_wifeeder"), "GX2 Mot1", 40, 500, "GX2_motor_wifeeder.fzp"),
        part_instance(15, module_id("GX2_motor_wifeeder"), "GX2 Mot2", 140, 500, "GX2_motor_wifeeder.fzp"),
        part_instance(16, module_id("PN01007BRKT_motor_wifeeder"), "Motor1", 40, 580, "PN01007BRKT_motor_wifeeder.fzp"),
        part_instance(17, module_id("PN01007BRKT_motor_wifeeder"), "Motor2", 140, 580, "PN01007BRKT_motor_wifeeder.fzp"),
        part_instance(18, module_id("GX4_encoder_wifeeder"), "GX4 Enc1", 280, 500, "GX4_encoder_wifeeder.fzp"),
        part_instance(19, module_id("GX4_encoder_wifeeder"), "GX4 Enc2", 400, 500, "GX4_encoder_wifeeder.fzp"),
        part_instance(20, module_id("GTS06_encoder_wifeeder"), "Enc1 600P/R", 280, 580, "GTS06_encoder_wifeeder.fzp"),
        part_instance(21, module_id("GTS06_encoder_wifeeder"), "Enc2 600P/R", 400, 580, "GTS06_encoder_wifeeder.fzp"),
        part_instance(22, module_id("RFID_125kHz_UART_wifeeder"), "RFID", 520, 220, "RFID_125kHz_UART_wifeeder.fzp"),
        part_instance(23, module_id("hx711_weightsensor"), "HX711", 520, 320, "hx711_weightsensor.fzp"),
        part_instance(24, module_id("Raspberry_Pi_2_v1.1"), "Raspberry Pi 2", 700, 220, "Raspberry_Pi_2_v1.1.fzp"),
        part_instance(25, module_id("NRF24_AM1117_adapter_wifeeder"), "NRF Pi", 700, 420, "NRF24_AM1117_adapter_wifeeder.fzp"),
    ]
    fz = dedent(
        f"""\
        <?xml version="1.0" encoding="UTF-8"?>
        <module fritzingVersion="0.9.6" icon=".png">
         <title>WiFeeder v2 Full Wiring</title>
         <label>WiFeeder</label>
         <description>STM32 + Pi wiring. Parts in Documents/Fritzing/parts/user.</description>
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
    for line in fz.splitlines():
        if 'path="' in line and "Documents/Fritzing" in line:
            p = Path(line.split('path="')[1].split('"')[0])
            if not p.is_file():
                raise FileNotFoundError(p)
    print("rewrote", out)


def main() -> None:
    stems = [install_fzpz(p) for p in sorted(PARTS_SRC.glob("*.fzpz"))]
    update_my_parts(stems)
    rewrite_fzz()
    print("Done. Quit Fritzing completely, then reopen:", ROOT / "wifeeder-v2.fzz")


if __name__ == "__main__":
    main()
