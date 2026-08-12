#!/usr/bin/env python3
"""
WiFeeder v2 Fritzing sketches.

Builds:
  - wiring/wifeeder-v2.fzz          full MVP + breadboard power/signal hubs (bench test)
  - wiring/blocks/01-power.fzz      USB Nucleo → X/Z; Mini360 IN only (OUT off X)
  - wiring/blocks/02-pca9685.fzz    Nucleo I2C ↔ PCA9685 (+ OE)
  - wiring/blocks/03-motor.fzz      PCA PWM → IBT-2 → GX-2 → motor
  - wiring/blocks/04-encoder.fzz    4-wire encoder pigtail + pull-ups → A0/A1
  - wiring/blocks/05-nrf-stm.fzz    Nucleo SPI ↔ NRF24
  - wiring/blocks/06-nrf-pi.fzz     Raspberry Pi ↔ NRF24

Full sketch breadboard rails (Fritzing Generic Bajillion Hole):
  X = 3V3 from Nucleo USB (NRF VCC + logic), W = GND (top), Z = 5V from Nucleo 5V
  (IBT/encoder only), Y = GND (bottom).
  Mini360 OUT is NOT wired to X — an unset pot is ~10 V and kills STM32 + IBT 74HC244.
  NRF VCC is 3.3 V — never Nucleo 5V (~4.8 V destroys PA/LNA; SPI still looks OK).
  Signal columns: see BB_* constants in build_full().
"""
from __future__ import annotations

import re
import shutil
import tempfile
import zipfile
from pathlib import Path
from textwrap import dedent
from xml.sax.saxutils import escape

ROOT = Path(__file__).resolve().parents[1]
BLOCKS = ROOT / "blocks"
PARTS_DIR = ROOT / "parts"
FZ_USER = Path.home() / "Documents" / "Fritzing"
USER_PARTS = FZ_USER / "parts" / "user"
SVG_USER = FZ_USER / "parts" / "svg" / "user"
CORE = Path("/usr/share/fritzing/parts/core")
CORE_SVG = Path("/usr/share/fritzing/parts/svg/core")

IBT = {
    "RPWM": "connector0",
    "LPWM": "connector1",
    "R_EN": "connector2",
    "L_EN": "connector3",
    "VCC": "connector6",
    "GND": "connector7",
    "Mplus": "connector8",
    "Mminus": "connector9",
    "Bplus": "connector10",
    "Bminus": "connector11",
}
NUC = {
    "GND": "connector1001",
    "5V": "connector1003",
    "A6": "connector1005",
    "A4": "connector1007",
    "A3": "connector1008",
    "A1": "connector1010",
    "A0": "connector1011",
    "3V3": "connector1013",
    "GND_D": "connector1103",
    "D3": "connector1105",
    "D4": "connector1106",
    "D5": "connector1107",
    "D6": "connector1108",
}
NRF = {
    "GND": "connector8",
    "VCC": "connector9",
    "CE": "connector10",
    "CS": "connector11",
    "SCK": "connector12",
    "MOSI": "connector13",
    "MISO": "connector14",
}
PI = {
    "3V3": "connector0",  # header pin 1 — NRF VCC (never Pi 5V on module VCC)
    "5V": "connector38",
    "GND": "connector30",
    "MOSI": "connector9",
    "MISO": "connector10",
    "SCLK": "connector11",
    "CE0": "connector28",
    "CE_RF": "connector29",
}
# Adafruit PCA9685 — use RIGHT logic header only (never C2 pads connector7/8)
PCA = {
    "VCC": "connector44",  # right header VCC (not left 38)
    "SDA": "connector45",
    "SCL": "connector46",
    "OE": "connector47",
    "GND": "connector48",  # right header GND (NOT connector8 = C2 capacitor)
    "PWM0": "connector34",
    "PWM1": "connector31",
}
PCA_FORBIDDEN = frozenset({"connector7", "connector8"})  # C2 electrolytic pads
MOT = {"p1": "connector0", "p2": "connector1"}
RES = {"a": "connector0", "b": "connector1"}
BAT = {"neg": "connector0", "pos": "connector1"}
BUCK = {"INp": "connector0", "INn": "connector1", "OUTp": "connector2", "OUTn": "connector3"}
GX2 = {"1": "connector0", "2": "connector1"}
# One factory-cabled encoder + 4 flying leads (not a separate GX-4 + GTS06 body).
ENC = {"VCC": "connector0", "GND": "connector1", "A": "connector2", "B": "connector3"}

# Full breadboard hole helpers (connector ids are A1, X5, … — not connectorN)
def BB(row: str, col: int) -> str:
    return f"{row}{col}"


# Bench hubs used by build_full (document in CONNECTOR_MAP.md)
BB_3V3 = "X"  # top + rail
BB_GND_T = "W"  # top − rail
BB_5V = "Z"  # bottom + rail
BB_GND_B = "Y"  # bottom − rail (jumpered to W)
BB_COL = {
    "SCL": 30,
    "SDA": 31,
    "PWM0": 33,
    "PWM1": 34,
    "ENC_A": 40,
    "ENC_B": 41,
    "CE": 45,
    "CSN": 46,
    "SCK": 47,
    "MOSI": 48,
    "MISO": 49,
}

RED, BLK, ORN, YEL, GRN, BLU, PUR, GRY, BRN, ORG, WHT = (
    "#cc0000",
    "#404040",
    "#ff6600",
    "#cccc00",
    "#00aa00",
    "#0066cc",
    "#9900cc",
    "#888888",
    "#996633",
    "#ff9900",
    "#e8e8e8",
)


def install_fzpz(fzpz: Path, rename_stem: str | None = None) -> str:
    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        with zipfile.ZipFile(fzpz) as zf:
            zf.extractall(tmp_path)
        fzp = next(tmp_path.glob("part.*.fzp"))
        stem = rename_stem or fzp.name[len("part.") : -len(".fzp")]
        USER_PARTS.mkdir(parents=True, exist_ok=True)
        for view in ("breadboard", "schematic", "pcb", "icon"):
            (SVG_USER / view).mkdir(parents=True, exist_ok=True)
        (USER_PARTS / f"{stem}.fzp").write_text(fzp.read_text(encoding="utf-8", errors="ignore"))
        for svg in tmp_path.glob("svg.*"):
            parts = svg.name.split(".", 2)
            if len(parts) < 3:
                continue
            _, view, fname = parts
            if view in ("breadboard", "schematic", "pcb", "icon"):
                shutil.copy2(svg, SVG_USER / view / fname)
        print("installed", stem)
        return stem


def install_core(fzp_name: str) -> str:
    fzp = CORE / fzp_name
    text = fzp.read_text(encoding="utf-8", errors="ignore")
    images = re.findall(r'image="([^"]+)"', text)
    stem = fzp.stem
    USER_PARTS.mkdir(parents=True, exist_ok=True)
    for view in ("breadboard", "schematic", "pcb", "icon"):
        (SVG_USER / view).mkdir(parents=True, exist_ok=True)
    (USER_PARTS / f"{stem}.fzp").write_text(text)
    for img in set(images):
        src = CORE_SVG / img
        if not src.is_file():
            found = list(CORE_SVG.rglob(Path(img).name))
            src = found[0] if found else None
        if src and src.is_file():
            shutil.copy2(src, SVG_USER / img.split("/")[0] / Path(img).name)
    print("installed core", stem)
    return stem


def module_id(stem: str) -> str:
    t = (USER_PARTS / f"{stem}.fzp").read_text(encoding="utf-8", errors="ignore")
    return re.search(r'moduleId="([^"]+)"', t).group(1)


def breadboard_layer(stem: str) -> str:
    t = (USER_PARTS / f"{stem}.fzp").read_text(encoding="utf-8", errors="ignore")
    m = re.search(r"<breadboardView>[\s\S]*?<layer layerId=\"([^\"]+)\"", t)
    return m.group(1) if m else "breadboard"


def path_of(stem: str) -> str:
    return str(USER_PARTS / f"{stem}.fzp")


class Sketch:
    def __init__(self, title: str, description: str) -> None:
        self.title = title
        self.description = description
        self.instances: list[str] = []
        self.next_id = 100
        self.meta: dict[str, dict] = {}

    def alloc(self) -> int:
        i = self.next_id
        self.next_id += 1
        return i

    def add_part(self, key: str, stem: str, title: str, x: float, y: float) -> int:
        mid = self.alloc()
        mod = module_id(stem)
        layer = breadboard_layer(stem)
        self.meta[key] = {"id": mid, "layer": layer, "x": x, "y": y, "stem": stem}
        self.instances.append(
            dedent(
                f"""\
            <instance modelIndex="{mid}" moduleIdRef="{mod}" path="{path_of(stem)}">
              <title>{escape(title)}</title>
              <views>
                <breadboardView layer="{layer}">
                  <geometry x="{x:.2f}" y="{y:.2f}" z="1.5"/>
                </breadboardView>
                <schematicView layer="schematic">
                  <geometry x="{x:.2f}" y="{y + 900:.2f}" z="1.5"/>
                </schematicView>
              </views>
            </instance>"""
            )
        )
        return mid

    def add_note(self, title: str, text: str, x: float, y: float) -> None:
        mid = self.alloc()
        body = escape(text).replace("\n", "&#10;")
        self.instances.append(
            dedent(
                f"""\
            <instance modelIndex="{mid}" moduleIdRef="NoteModuleID" path=":/resources/parts/core/note.fzp">
              <title>{escape(title)}</title>
              <views>
                <breadboardView layer="breadboard">
                  <geometry x="{x}" y="{y}" z="5"/>
                  <text fontSize="10" color="#000000">{body}</text>
                </breadboardView>
                <schematicView layer="schematic">
                  <geometry x="{x}" y="{y}" z="5"/>
                  <text fontSize="10" color="#000000">{body}</text>
                </schematicView>
              </views>
            </instance>"""
            )
        )

    def wire(
        self,
        title: str,
        a: str,
        ac: str,
        b: str,
        bc: str,
        color: str,
        *,
        allow_same: bool = False,
    ) -> None:
        if a == b and not allow_same:
            raise ValueError(f"refusing same-part wire {title!r} on {a}")
        mid = self.alloc()
        A, B = self.meta[a], self.meta[b]
        x, y = A["x"] + 40, A["y"] + 40
        dx, dy = (B["x"] - A["x"]), (B["y"] - A["y"])
        if abs(dx) < 8 and abs(dy) < 8:
            dx, dy = 20.0, 20.0
        cpx, cpy = dx * 0.55, dy * 0.15
        # Breadboard schematic view reuses breadboardbreadboard (not "schematic")
        a_sch = A["layer"] if A["layer"] == "breadboardbreadboard" else "schematic"
        b_sch = B["layer"] if B["layer"] == "breadboardbreadboard" else "schematic"
        self.instances.append(
            dedent(
                f"""\
            <instance modelIndex="{mid}" moduleIdRef="WireModuleID" path=":/resources/parts/core/wire.fzp">
              <title>{escape(title)}</title>
              <views>
                <breadboardView layer="breadboardWire">
                  <geometry wireFlags="64" x="{x:.2f}" x1="0" x2="{dx:.2f}" y="{y:.2f}" y1="0" y2="{dy:.2f}" z="3.5"/>
                  <wireExtras banded="0" color="{color}" mils="16" opacity="1">
                    <bezier>
                      <cp0 x="0" y="0"/>
                      <cp1 x="{cpx:.2f}" y="{cpy:.2f}"/>
                    </bezier>
                  </wireExtras>
                  <connectors>
                    <connector connectorId="connector0" layer="breadboardWire">
                      <geometry x="0" y="0"/>
                      <connects>
                        <connect connectorId="{ac}" layer="{A["layer"]}" modelIndex="{A["id"]}"/>
                      </connects>
                    </connector>
                    <connector connectorId="connector1" layer="breadboardWire">
                      <geometry x="0" y="0"/>
                      <connects>
                        <connect connectorId="{bc}" layer="{B["layer"]}" modelIndex="{B["id"]}"/>
                      </connects>
                    </connector>
                  </connectors>
                </breadboardView>
                <schematicView layer="schematicTrace">
                  <geometry wireFlags="64" x="{x:.2f}" x1="0" x2="{dx:.2f}" y="{y + 900:.2f}" y1="0" y2="{dy:.2f}" z="3.5"/>
                  <wireExtras banded="0" color="{color}" mils="11" opacity="1"/>
                  <connectors>
                    <connector connectorId="connector0" layer="schematicTrace">
                      <geometry x="0" y="0"/>
                      <connects>
                        <connect connectorId="{ac}" layer="{a_sch}" modelIndex="{A["id"]}"/>
                      </connects>
                    </connector>
                    <connector connectorId="connector1" layer="schematicTrace">
                      <geometry x="0" y="0"/>
                      <connects>
                        <connect connectorId="{bc}" layer="{b_sch}" modelIndex="{B["id"]}"/>
                      </connects>
                    </connector>
                  </connectors>
                </schematicView>
              </views>
            </instance>"""
            )
        )

    def assert_nucleo_pins_unique(self) -> None:
        """Fail build if any Nucleo connector is used more than once (one jumper per pin)."""
        if "nucleo" not in self.meta:
            return
        nucleo_id = int(self.meta["nucleo"]["id"])
        usage: dict[str, list[str]] = {}
        for xml in self.instances:
            if "WireModuleID" not in xml:
                continue
            # Only breadboardView — schematicTrace would double-count the same jumper
            bb = re.search(
                r"<breadboardView[\s\S]*?</breadboardView>",
                xml,
            )
            if not bb:
                continue
            title_m = re.search(r"<title>([^<]*)</title>", xml)
            title = title_m.group(1) if title_m else "?"
            for m in re.finditer(
                r'<connect connectorId="(connector\d+)" layer="[^"]*" modelIndex="(\d+)"/>',
                bb.group(0),
            ):
                cid, mid = m.group(1), int(m.group(2))
                if mid != nucleo_id:
                    continue
                usage.setdefault(cid, []).append(title)
        print("Nucleo pin usage:")
        dupes = []
        for cid, titles in sorted(usage.items()):
            print(f"  {cid}: {len(titles)} × {titles}")
            if len(titles) > 1:
                dupes.append((cid, titles))
        if dupes:
            msg = "; ".join(f"{c} used by {t}" for c, t in dupes)
            raise SystemExit(f"Nucleo one-jumper rule violated: {msg}")

    def assert_pca_pins_safe(self) -> None:
        """Fail if any wire targets PCA C2 capacitor pads (connector7/8)."""
        pca_id = self.meta.get("pca", {}).get("id")
        if pca_id is None:
            return
        for xml in self.instances:
            if "WireModuleID" not in xml:
                continue
            bb = re.search(r"<breadboardView[\s\S]*?</breadboardView>", xml)
            if not bb:
                continue
            for m in re.finditer(
                r'<connect connectorId="(connector\d+)" layer="[^"]*" modelIndex="(\d+)"/>',
                bb.group(0),
            ):
                cid, mid = m.group(1), int(m.group(2))
                if mid == pca_id and cid in PCA_FORBIDDEN:
                    title_m = re.search(r"<title>([^<]*)</title>", xml)
                    title = title_m.group(1) if title_m else "?"
                    raise SystemExit(
                        f"PCA capacitor pad wired: {title} → {cid} "
                        f"(use header GND connector48 / VCC connector44)"
                    )

    def save(self, out: Path) -> None:
        self.assert_nucleo_pins_unique()
        self.assert_pca_pins_safe()
        out.parent.mkdir(parents=True, exist_ok=True)
        fz = dedent(
            f"""\
            <?xml version="1.0" encoding="UTF-8"?>
            <module fritzingVersion="0.9.6" icon=".png">
             <title>{escape(self.title)}</title>
             <label>WiFeeder</label>
             <description>{escape(self.description)}</description>
             <views>
              <view name="breadboardView" backgroundColor="#f7fafc" gridSize="0.1in" showGrid="1" alignToGrid="1"/>
              <view name="schematicView" backgroundColor="#ffffff" gridSize="0.1in" showGrid="1" alignToGrid="1"/>
              <view name="pcbView" backgroundColor="#333333" gridSize="0.05in" showGrid="1" alignToGrid="1"/>
             </views>
             <instances>
            {chr(10).join(self.instances)}
             </instances>
            </module>
            """
        ).lstrip()
        with zipfile.ZipFile(out, "w", compression=zipfile.ZIP_DEFLATED) as zf:
            zf.writestr(out.stem + ".fz", fz)
        print("wrote", out, "n=", len(self.instances))


def prepare() -> dict[str, str]:
    stems: dict[str, str] = {}
    stems["pca"] = install_fzpz(PARTS_DIR / "Adafruit_PCA9685.fzpz", "Adafruit_PCA9685")
    stems["ibt"] = install_fzpz(PARTS_DIR / "IBT2_BTS7960_real.fzpz", "IBT2_BTS7960_real")
    stems["nucleo"] = install_fzpz(PARTS_DIR / "STM32_Nucleo-32_board.fzpz", "STM32_Nucleo-32_board")
    stems["gx2"] = install_fzpz(PARTS_DIR / "GX2_motor_wifeeder.fzpz")
    stems["enc"] = install_fzpz(PARTS_DIR / "GTS06_encoder_wifeeder.fzpz")
    for key, fzp in [
        ("nrf", "NRF24L01+_breakout.fzp"),
        ("pi", "Raspberry_Pi_2_v1.1.fzp"),
        ("motor", "gear-motor_2.fzp"),
        ("res", "resistor.fzp"),
        ("bat", "Battery block 9V.fzp"),
        ("buck", "RioRand_LM2596_94ce9810d0922bc731e2f315085d46ab_1.fzp"),
        ("bb", "breadboard.fzp"),
    ]:
        stems[key] = install_core(fzp)
    inst = []
    for fzp in sorted(USER_PARTS.glob("*.fzp")):
        t = fzp.read_text(encoding="utf-8", errors="ignore")
        mid = re.search(r'moduleId="([^"]+)"', t)
        title = re.search(r"<title>([^<]+)</title>", t)
        if mid:
            inst.append(
                f'    <instance moduleIdRef="{mid.group(1)}" path="user/{fzp.name}">\n'
                f'        <title>{title.group(1) if title else fzp.stem}</title>\n'
                f"    </instance>"
            )
    (FZ_USER / "bins").mkdir(parents=True, exist_ok=True)
    (FZ_USER / "bins" / "my_parts.fzb").write_text(
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<module fritzingVersion="0.9.6" icon="Mine.png">\n'
        "    <title>My Parts</title>\n"
        "    <instances>\n" + "\n".join(inst) + "\n    </instances>\n</module>\n"
    )
    return stems


# ---------------------------------------------------------------------------
# Shared breadboard helpers (same map as CONNECTOR_MAP / main .fzz)
# ---------------------------------------------------------------------------


def add_breadboard(s: Sketch, stems: dict[str, str], x: float = 0, y: float = 200) -> None:
    s.add_part("bb", stems["bb"], "Breadboard (same map as main)", x, y)


def wire_bb_rails(s: Sketch) -> None:
    """Tie top/bottom GND rails — same as main sketch."""
    s.wire(
        "BB GND W↔Y",
        "bb",
        BB(BB_GND_T, 5),
        "bb",
        BB(BB_GND_B, 5),
        BLK,
        allow_same=True,
    )


def wire_nucleo_logic_rails(s: Sketch, *, with_5v: bool = False) -> None:
    """USB Nucleo *supplies* the breadboard. Never inject Mini360 into 3V3."""
    s.wire("Nucleo 3V3 → BB X", "nucleo", NUC["3V3"], "bb", BB(BB_3V3, 3), RED)
    s.wire("Nucleo GND → BB W", "nucleo", NUC["GND"], "bb", BB(BB_GND_T, 3), BLK)
    if with_5v:
        s.wire("Nucleo 5V → BB Z", "nucleo", NUC["5V"], "bb", BB(BB_5V, 1), RED)


def wire_mini360_in_only(s: Sketch) -> None:
    """Battery on Mini360 IN so the pot can be set. OUT+ stays off rail X."""
    s.wire("BAT+ → Buck IN+", "bat", BAT["pos"], "buck", BUCK["INp"], RED)
    s.wire("BAT- → Buck IN-", "bat", BAT["neg"], "buck", BUCK["INn"], BLK)
    s.wire("Buck GND → BB W", "buck", BUCK["OUTn"], "bb", BB(BB_GND_T, 1), BLK)


def bb_map_note(s: Sketch, extra: str, x: float = 480, y: float = -120) -> None:
    s.add_note(
        "BB map",
        "SAME as main wifeeder-v2.fzz\n"
        "X=3V3 from Nucleo USB (NRF VCC)\n"
        "Z=5V from Nucleo 5V (IBT/Enc)\n"
        "Mini360 OUT not on X until 3.30 V\n"
        "30 SCL 31 SDA  33 PWM0 34 PWM1\n"
        "40 EncA 41 EncB\n"
        "45 CE 46 CSN 47 SCK 48 MOSI 49 MISO\n" + extra,
        x,
        y,
    )


def wire_encoder_pigtail(s: Sketch) -> None:
    """Four flying leads from the factory encoder cable — no second GTS06 body."""
    s.wire("Enc red VCC → Z", "enc", ENC["VCC"], "bb", BB(BB_5V, 7), RED)
    s.wire("Enc black GND → Y", "enc", ENC["GND"], "bb", BB(BB_GND_B, 7), BLK)
    c = BB_COL["ENC_A"]
    s.wire("Enc green A → BB", "enc", ENC["A"], "bb", BB("A", c), GRN)
    s.wire("BB → A0", "bb", BB("C", c), "nucleo", NUC["A0"], GRN)
    s.wire("pullup A", "rA", RES["a"], "bb", BB("D", c), ORN)
    s.wire("pullup A 3V3", "rA", RES["b"], "bb", BB(BB_3V3, 13), RED)
    c = BB_COL["ENC_B"]
    s.wire("Enc white B → BB", "enc", ENC["B"], "bb", BB("A", c), WHT)
    s.wire("BB → A1", "bb", BB("C", c), "nucleo", NUC["A1"], WHT)
    s.wire("pullup B", "rB", RES["a"], "bb", BB("D", c), ORN)
    s.wire("pullup B 3V3", "rB", RES["b"], "bb", BB(BB_3V3, 14), RED)


# ---------------------------------------------------------------------------
# Block sketches (each has its own breadboard, same rails/columns)
# ---------------------------------------------------------------------------


def build_power(stems: dict[str, str]) -> None:
    s = Sketch(
        "WiFeeder — 01 Power",
        "USB Nucleo supplies X=3V3 and Z=5V. Mini360 IN from battery; OUT not on X.",
    )
    s.add_note(
        "Block",
        "01 POWER + breadboard\n"
        "USB Nucleo → X (3V3) and Z (5V)\n"
        "Mini360 IN = battery (set pot)\n"
        "OUT+ NOT on X until 3.30 V\n"
        "W jumper Y\n"
        "BAT → IBT B± in 03 (direct)",
        -80,
        -160,
    )
    add_breadboard(s, stems)
    s.add_part("bat", stems["bat"], "12V battery", -200, -40)
    s.add_part("buck", stems["buck"], "Mini360 SET 3.3V — OUT off X", 40, -40)
    s.add_part("nucleo", stems["nucleo"], "NUCLEO-L432KC", -40, 40)

    wire_mini360_in_only(s)
    wire_bb_rails(s)
    wire_nucleo_logic_rails(s, with_5v=True)
    bb_map_note(s, "This block: rails X/W/Y only")
    s.save(BLOCKS / "01-power.fzz")


def build_pca9685(stems: dict[str, str]) -> None:
    s = Sketch(
        "WiFeeder — 02 PCA9685 PWM",
        "Breadboard: X/W power + cols 30/31 I2C. Same map as main. Nucleo D4/D5 once.",
    )
    s.add_note(
        "Block",
        "02 PCA9685 + breadboard\n"
        "X → PCA VCC   W → GND/OE\n"
        "col 30 SCL (D5)\n"
        "col 31 SDA (D4)\n"
        "right header only (not C2)",
        -80,
        -160,
    )
    add_breadboard(s, stems)
    s.add_part("nucleo", stems["nucleo"], "NUCLEO-L432KC", -40, 40)
    s.add_part("pca", stems["pca"], "PCA9685", -200, 380)

    wire_bb_rails(s)
    wire_nucleo_logic_rails(s)
    s.wire("BB 3V3 → PCA VCC", "bb", BB(BB_3V3, 7), "pca", PCA["VCC"], RED)
    s.wire("BB GND → PCA", "bb", BB(BB_GND_T, 7), "pca", PCA["GND"], BLK)
    s.wire("PCA OE → BB GND", "pca", PCA["OE"], "bb", BB(BB_GND_T, 8), BLK)
    c = BB_COL["SCL"]
    s.wire("D5 SCL → BB", "nucleo", NUC["D5"], "bb", BB("A", c), PUR)
    s.wire("BB → PCA SCL", "bb", BB("E", c), "pca", PCA["SCL"], PUR)
    c = BB_COL["SDA"]
    s.wire("D4 SDA → BB", "nucleo", NUC["D4"], "bb", BB("A", c), GRN)
    s.wire("BB → PCA SDA", "bb", BB("E", c), "pca", PCA["SDA"], GRN)
    bb_map_note(s, "This block: X/W + cols 30/31")
    s.save(BLOCKS / "02-pca9685.fzz")


def build_motor(stems: dict[str, str]) -> None:
    s = Sketch(
        "WiFeeder — 03 Motor drive",
        "Breadboard: X EN, Z IBT VCC, cols 33/34 PWM. BAT→IBT B± direct. Same map as main.",
    )
    s.add_note(
        "Block",
        "03 MOTOR + breadboard\n"
        "USB Nucleo → X EN + Z IBT VCC\n"
        "Mini360 OUT off X\n"
        "col 33 PWM0→RPWM\n"
        "col 34 PWM1→LPWM\n"
        "BAT → IBT B± direct",
        -80,
        -160,
    )
    add_breadboard(s, stems)
    s.add_part("bat", stems["bat"], "12V battery", -200, -40)
    s.add_part("buck", stems["buck"], "Mini360 SET 3.3V — OUT off X", 40, -40)
    s.add_part("nucleo", stems["nucleo"], "NUCLEO-L432KC", -40, 40)
    s.add_part("pca", stems["pca"], "PCA9685", -200, 380)
    s.add_part("ibt", stems["ibt"], "IBT-2 (1 ch)", 200, 380)
    s.add_part("gx2", stems["gx2"], "GX2 motor", 200, 560)
    s.add_part("mot", stems["motor"], "PN01007BRKT", 380, 580)

    wire_mini360_in_only(s)
    s.wire("BAT+ → IBT B+", "bat", BAT["pos"], "ibt", IBT["Bplus"], RED)
    s.wire("BAT- → IBT B-", "bat", BAT["neg"], "ibt", IBT["Bminus"], BLK)
    wire_bb_rails(s)
    wire_nucleo_logic_rails(s, with_5v=True)
    s.wire("BB 3V3 → PCA VCC", "bb", BB(BB_3V3, 7), "pca", PCA["VCC"], RED)
    s.wire("BB GND → PCA", "bb", BB(BB_GND_T, 7), "pca", PCA["GND"], BLK)
    s.wire("BB 5V → IBT VCC", "bb", BB(BB_5V, 3), "ibt", IBT["VCC"], RED)
    s.wire("BB 3V3 → IBT R_EN", "bb", BB(BB_3V3, 10), "ibt", IBT["R_EN"], ORG)
    s.wire("BB 3V3 → IBT L_EN", "bb", BB(BB_3V3, 11), "ibt", IBT["L_EN"], ORG)
    s.wire("BB GND → IBT", "bb", BB(BB_GND_T, 10), "ibt", IBT["GND"], BLK)
    c = BB_COL["PWM0"]
    s.wire("PWM0 → BB", "pca", PCA["PWM0"], "bb", BB("A", c), GRY)
    s.wire("BB → RPWM", "bb", BB("E", c), "ibt", IBT["RPWM"], GRY)
    c = BB_COL["PWM1"]
    s.wire("PWM1 → BB", "pca", PCA["PWM1"], "bb", BB("A", c), "#555555")
    s.wire("BB → LPWM", "bb", BB("E", c), "ibt", IBT["LPWM"], "#555555")
    s.wire("IBT M+ → GX2", "ibt", IBT["Mplus"], "gx2", GX2["1"], RED)
    s.wire("IBT M- → GX2", "ibt", IBT["Mminus"], "gx2", GX2["2"], BLK)
    s.wire("GX2 → Mot+", "gx2", GX2["1"], "mot", MOT["p1"], RED)
    s.wire("GX2 → Mot-", "gx2", GX2["2"], "mot", MOT["p2"], BLK)
    bb_map_note(s, "This block: X/Z + cols 33/34")
    s.save(BLOCKS / "03-motor.fzz")


def build_encoder(stems: dict[str, str]) -> None:
    s = Sketch(
        "WiFeeder — 04 Encoder",
        "Breadboard: Z=Enc VCC, cols 40/41 A/B + pull-ups to X. Same map as main.",
    )
    s.add_note(
        "Block",
        "04 ENCODER + breadboard\n"
        "ONE 4-wire pigtail (2 red + 2 black)\n"
        "ID nets by molded pin + meter\n"
        "then VCC→Z  GND→Y\n"
        "A→col40+A0  B→col41+A1\n"
        "4k7 A/B → X; one jumper A0/A1",
        -80,
        -160,
    )
    add_breadboard(s, stems)
    s.add_part("nucleo", stems["nucleo"], "NUCLEO-L432KC", -40, 40)
    s.add_part("rA", stems["res"], "4k7 EncA", 420, 420)
    s.add_part("rB", stems["res"], "4k7 EncB", 500, 420)
    s.add_part("enc", stems["enc"], "Encoder 4-wire", 560, 260)

    wire_bb_rails(s)
    wire_nucleo_logic_rails(s, with_5v=True)
    wire_encoder_pigtail(s)
    bb_map_note(s, "This block: X/Z + cols 40/41")
    s.save(BLOCKS / "04-encoder.fzz")


def build_nrf_stm(stems: dict[str, str]) -> None:
    s = Sketch(
        "WiFeeder — 05 NRF (STM)",
        "Breadboard: X → NRF VCC (3.3 V), cols 45–49 SPI. NEVER 5V on NRF. Same map as main.",
    )
    s.add_note(
        "Block",
        "05 NRF STM + breadboard\n"
        "X → NRF VCC (3.3 V)\n"
        "NEVER Z/5V on NRF\n"
        "GND_D → NRF GND once\n"
        "45 CE 46 CSN 47 SCK\n"
        "48 MOSI 49 MISO",
        -80,
        -160,
    )
    add_breadboard(s, stems)
    s.add_part("nucleo", stems["nucleo"], "NUCLEO-L432KC", -40, 40)
    s.add_part("nrf_stm", stems["nrf"], "NRF STM (3V3)", 200, 0)

    wire_bb_rails(s)
    wire_nucleo_logic_rails(s)
    s.wire("BB 3V3 → NRF VCC", "bb", BB(BB_3V3, 16), "nrf_stm", NRF["VCC"], ORG)
    s.wire("GND_D → NRF", "nucleo", NUC["GND_D"], "nrf_stm", NRF["GND"], BLK)
    c = BB_COL["CE"]
    s.wire("D3 CE → BB", "nucleo", NUC["D3"], "bb", BB("A", c), ORN)
    s.wire("BB → NRF CE", "bb", BB("E", c), "nrf_stm", NRF["CE"], ORN)
    c = BB_COL["CSN"]
    s.wire("A3 CSN → BB", "nucleo", NUC["A3"], "bb", BB("A", c), YEL)
    s.wire("BB → NRF CSN", "bb", BB("E", c), "nrf_stm", NRF["CS"], YEL)
    c = BB_COL["SCK"]
    s.wire("A4 SCK → BB", "nucleo", NUC["A4"], "bb", BB("A", c), PUR)
    s.wire("BB → NRF SCK", "bb", BB("E", c), "nrf_stm", NRF["SCK"], PUR)
    c = BB_COL["MOSI"]
    s.wire("A6 MOSI → BB", "nucleo", NUC["A6"], "bb", BB("A", c), GRN)
    s.wire("BB → NRF MOSI", "bb", BB("E", c), "nrf_stm", NRF["MOSI"], GRN)
    c = BB_COL["MISO"]
    s.wire("D6 MISO → BB", "nucleo", NUC["D6"], "bb", BB("A", c), BLU)
    s.wire("BB → NRF MISO", "bb", BB("E", c), "nrf_stm", NRF["MISO"], BLU)
    bb_map_note(s, "This block: X + cols 45–49")
    s.save(BLOCKS / "05-nrf-stm.fzz")


def build_nrf_pi(stems: dict[str, str]) -> None:
    s = Sketch(
        "WiFeeder — 06 NRF (Pi)",
        "Own breadboard, same column numbers: Pi 3V3→X, SPI on 45–49. Never Pi 5V on NRF.",
    )
    s.add_note(
        "Block",
        "06 NRF Pi + breadboard\n"
        "Pi pin1 3V3 → BB X\n"
        "Pi pin6 GND → BB W\n"
        "X → NRF VCC (never 5V)\n"
        "same cols 45–49 as STM",
        -80,
        -160,
    )
    add_breadboard(s, stems, 0, 200)
    s.add_part("pi", stems["pi"], "Raspberry Pi 2", -40, -20)
    s.add_part("nrf_pi", stems["nrf"], "NRF Pi (3V3)", 200, 380)

    s.wire("Pi 3V3 → BB X", "pi", PI["3V3"], "bb", BB(BB_3V3, 1), ORG)
    s.wire("Pi GND → BB W", "pi", PI["GND"], "bb", BB(BB_GND_T, 1), BLK)
    wire_bb_rails(s)
    s.wire("BB 3V3 → NRF VCC", "bb", BB(BB_3V3, 16), "nrf_pi", NRF["VCC"], ORG)
    s.wire("BB GND → NRF", "bb", BB(BB_GND_T, 3), "nrf_pi", NRF["GND"], BLK)
    c = BB_COL["CE"]
    s.wire("Pi GPIO25 → BB", "pi", PI["CE_RF"], "bb", BB("A", c), ORN)
    s.wire("BB → NRF CE", "bb", BB("E", c), "nrf_pi", NRF["CE"], ORN)
    c = BB_COL["CSN"]
    s.wire("Pi CE0 → BB", "pi", PI["CE0"], "bb", BB("A", c), YEL)
    s.wire("BB → NRF CSN", "bb", BB("E", c), "nrf_pi", NRF["CS"], YEL)
    c = BB_COL["SCK"]
    s.wire("Pi SCLK → BB", "pi", PI["SCLK"], "bb", BB("A", c), PUR)
    s.wire("BB → NRF SCK", "bb", BB("E", c), "nrf_pi", NRF["SCK"], PUR)
    c = BB_COL["MOSI"]
    s.wire("Pi MOSI → BB", "pi", PI["MOSI"], "bb", BB("A", c), GRN)
    s.wire("BB → NRF MOSI", "bb", BB("E", c), "nrf_pi", NRF["MOSI"], GRN)
    c = BB_COL["MISO"]
    s.wire("Pi MISO → BB", "pi", PI["MISO"], "bb", BB("A", c), BLU)
    s.wire("BB → NRF MISO", "bb", BB("E", c), "nrf_pi", NRF["MISO"], BLU)
    bb_map_note(s, "Pi island: X from pin1, cols 45–49")
    s.save(BLOCKS / "06-nrf-pi.fzz")


def build_full(stems: dict[str, str]) -> None:
    """Full MVP with breadboard hubs for bench organization; one jumper per Nucleo pin."""
    s = Sketch(
        "WiFeeder v2 MVP — breadboard bench",
        "USB Nucleo supplies X=3V3 and Z=5V. Mini360 OUT not on X. "
        "NRF never on Nucleo 5V. One jumper per Nucleo pin.",
    )
    s.add_note(
        "MVP",
        "BENCH: USB Nucleo feeds X/Z\n"
        "X=3V3 = NRF/PCA/EN\n"
        "Z=5V = IBT VCC + Enc\n"
        "Mini360 OUT OFF X until 3.30V\n"
        "NEVER 5V on NRF VCC\n"
        "30/31 I2C  33/34 PWM  40/41 Enc",
        -80,
        -160,
    )

    # Breadboard under the assembly; modules around it
    s.add_part("bb", stems["bb"], "Breadboard (bench hub)", 0, 200)
    s.add_part("bat", stems["bat"], "12V battery", -200, -40)
    s.add_part("buck", stems["buck"], "Mini360 SET 3.3V — OUT off X", 40, -40)
    s.add_part("nucleo", stems["nucleo"], "NUCLEO-L432KC", -40, 40)
    s.add_part("nrf_stm", stems["nrf"], "NRF STM (3V3)", 200, 0)
    s.add_part("pca", stems["pca"], "PCA9685", -200, 380)
    s.add_part("ibt", stems["ibt"], "IBT-2 (1 ch)", 200, 380)
    s.add_part("rA", stems["res"], "4k7 EncA", 420, 420)
    s.add_part("rB", stems["res"], "4k7 EncB", 500, 420)
    s.add_part("gx2", stems["gx2"], "GX2 motor", 200, 560)
    s.add_part("mot", stems["motor"], "PN01007BRKT", 380, 580)
    s.add_part("enc", stems["enc"], "Encoder 4-wire", 560, 260)
    s.add_part("pi", stems["pi"], "Raspberry Pi 2", 980, -20)
    s.add_part("nrf_pi", stems["nrf"], "NRF Pi (3V3)", 1060, 300)

    # --- Battery: IBT B± + Mini360 IN only (OUT+ not on X) ---
    wire_mini360_in_only(s)
    s.wire("BAT+ → IBT B+", "bat", BAT["pos"], "ibt", IBT["Bplus"], RED)
    s.wire("BAT- → IBT B-", "bat", BAT["neg"], "ibt", IBT["Bminus"], BLK)

    wire_bb_rails(s)
    wire_nucleo_logic_rails(s, with_5v=True)
    s.wire("BB 3V3 → PCA VCC", "bb", BB(BB_3V3, 7), "pca", PCA["VCC"], RED)
    s.wire("BB GND → PCA", "bb", BB(BB_GND_T, 7), "pca", PCA["GND"], BLK)
    s.wire("PCA OE → BB GND", "pca", PCA["OE"], "bb", BB(BB_GND_T, 8), BLK)
    s.wire("BB 3V3 → IBT R_EN", "bb", BB(BB_3V3, 10), "ibt", IBT["R_EN"], ORG)
    s.wire("BB 3V3 → IBT L_EN", "bb", BB(BB_3V3, 11), "ibt", IBT["L_EN"], ORG)
    s.wire("BB GND → IBT", "bb", BB(BB_GND_T, 10), "ibt", IBT["GND"], BLK)

    # --- 5V rail already from Nucleo → Z; IBT / encoder only (never NRF) ---
    s.wire("BB 5V → IBT VCC", "bb", BB(BB_5V, 3), "ibt", IBT["VCC"], RED)
    # --- NRF power: 3.3 V from BB X (Nucleo USB). GND_D once. ---
    s.wire("BB 3V3 → NRF VCC", "bb", BB(BB_3V3, 16), "nrf_stm", NRF["VCC"], ORG)
    s.wire("GND_D → NRF", "nucleo", NUC["GND_D"], "nrf_stm", NRF["GND"], BLK)

    # --- I2C via columns 30/31 ---
    c = BB_COL["SCL"]
    s.wire("D5 SCL → BB", "nucleo", NUC["D5"], "bb", BB("A", c), PUR)
    s.wire("BB → PCA SCL", "bb", BB("E", c), "pca", PCA["SCL"], PUR)
    c = BB_COL["SDA"]
    s.wire("D4 SDA → BB", "nucleo", NUC["D4"], "bb", BB("A", c), GRN)
    s.wire("BB → PCA SDA", "bb", BB("E", c), "pca", PCA["SDA"], GRN)

    # --- PWM via columns 33/34 ---
    c = BB_COL["PWM0"]
    s.wire("PWM0 → BB", "pca", PCA["PWM0"], "bb", BB("A", c), GRY)
    s.wire("BB → RPWM", "bb", BB("E", c), "ibt", IBT["RPWM"], GRY)
    c = BB_COL["PWM1"]
    s.wire("PWM1 → BB", "pca", PCA["PWM1"], "bb", BB("A", c), "#555555")
    s.wire("BB → LPWM", "bb", BB("E", c), "ibt", IBT["LPWM"], "#555555")

    # --- Motor harness (direct) ---
    s.wire("IBT M+ → GX2", "ibt", IBT["Mplus"], "gx2", GX2["1"], RED)
    s.wire("IBT M- → GX2", "ibt", IBT["Mminus"], "gx2", GX2["2"], BLK)
    s.wire("GX2 → Mot+", "gx2", GX2["1"], "mot", MOT["p1"], RED)
    s.wire("GX2 → Mot-", "gx2", GX2["2"], "mot", MOT["p2"], BLK)

    # --- Encoder 4-wire pigtail via columns 40/41 (pull-ups + MCU on same strip) ---
    wire_encoder_pigtail(s)

    # --- NRF STM SPI via columns 45–49 ---
    c = BB_COL["CE"]
    s.wire("D3 CE → BB", "nucleo", NUC["D3"], "bb", BB("A", c), ORN)
    s.wire("BB → NRF CE", "bb", BB("E", c), "nrf_stm", NRF["CE"], ORN)
    c = BB_COL["CSN"]
    s.wire("A3 CSN → BB", "nucleo", NUC["A3"], "bb", BB("A", c), YEL)
    s.wire("BB → NRF CSN", "bb", BB("E", c), "nrf_stm", NRF["CS"], YEL)
    c = BB_COL["SCK"]
    s.wire("A4 SCK → BB", "nucleo", NUC["A4"], "bb", BB("A", c), PUR)
    s.wire("BB → NRF SCK", "bb", BB("E", c), "nrf_stm", NRF["SCK"], PUR)
    c = BB_COL["MOSI"]
    s.wire("A6 MOSI → BB", "nucleo", NUC["A6"], "bb", BB("A", c), GRN)
    s.wire("BB → NRF MOSI", "bb", BB("E", c), "nrf_stm", NRF["MOSI"], GRN)
    c = BB_COL["MISO"]
    s.wire("D6 MISO → BB", "nucleo", NUC["D6"], "bb", BB("A", c), BLU)
    s.wire("BB → NRF MISO", "bb", BB("E", c), "nrf_stm", NRF["MISO"], BLU)

    # --- Pi NRF (separate island; no shared BB) ---
    s.wire("Pi 3V3 → NRF", "pi", PI["3V3"], "nrf_pi", NRF["VCC"], ORG)
    s.wire("Pi GND → NRF", "pi", PI["GND"], "nrf_pi", NRF["GND"], BLK)
    s.wire("Pi SCLK", "pi", PI["SCLK"], "nrf_pi", NRF["SCK"], PUR)
    s.wire("Pi MOSI", "pi", PI["MOSI"], "nrf_pi", NRF["MOSI"], GRN)
    s.wire("Pi MISO", "pi", PI["MISO"], "nrf_pi", NRF["MISO"], BLU)
    s.wire("Pi CE0", "pi", PI["CE0"], "nrf_pi", NRF["CS"], YEL)
    s.wire("Pi GPIO25 CE", "pi", PI["CE_RF"], "nrf_pi", NRF["CE"], ORN)
    s.add_note("RF", "STM NRF ↔ Pi NRF over air\n(no copper)", 780, 80)
    s.add_note(
        "BB map",
        "X=3V3 from Nucleo USB  Z=5V from Nucleo\n"
        "Mini360 OUT off X until 3.30 V\n"
        "NEVER 5V on NRF module VCC\n"
        "30 SCL 31 SDA  33 PWM0 34 PWM1\n"
        "40 EncA 41 EncB  45–49 SPI",
        520,
        -120,
    )
    s.save(ROOT / "wifeeder-v2.fzz")


def main() -> None:
    if not (PARTS_DIR / "Adafruit_PCA9685.fzpz").is_file():
        raise SystemExit("Missing parts/Adafruit_PCA9685.fzpz")
    if not (PARTS_DIR / "IBT2_BTS7960_real.fzpz").is_file():
        raise SystemExit("Missing IBT2_BTS7960_real.fzpz")
    stems = prepare()
    BLOCKS.mkdir(parents=True, exist_ok=True)
    build_power(stems)
    build_pca9685(stems)
    build_motor(stems)
    build_encoder(stems)
    build_nrf_stm(stems)
    build_nrf_pi(stems)
    build_full(stems)


if __name__ == "__main__":
    main()
