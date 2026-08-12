#!/usr/bin/env python3
"""Route wifeeder-actuator PCB with pad-accurate connectivity (KiCad 6 pcbnew)."""

from __future__ import annotations

import sys
from pathlib import Path

import pcbnew

ROOT = Path(__file__).resolve().parents[1]
PCB_PATH = ROOT / "wifeeder-actuator.kicad_pcb"
AUDIT_DIR = ROOT / "audit"

FCU = pcbnew.F_Cu
BCU = pcbnew.B_Cu


def mm(x: float, y: float) -> pcbnew.wxPoint:
    return pcbnew.wxPoint(int(pcbnew.FromMM(x)), int(pcbnew.FromMM(y)))


def pad_pos(board: pcbnew.BOARD, ref: str, num: str) -> tuple[float, float]:
    fp = board.FindFootprintByReference(ref)
    if fp is None:
        raise KeyError(f"footprint {ref!r} not found")
    for pad in fp.Pads():
        if pad.GetNumber() == num:
            pos = pad.GetPosition()
            return pcbnew.ToMM(pos.x), pcbnew.ToMM(pos.y)
    raise KeyError(f"pad {ref}/{num} not found")


def net_code(board: pcbnew.BOARD, name: str) -> int:
    net = board.FindNet(name)
    if net is None:
        raise KeyError(f"net {name!r} not found")
    return net.GetNetCode()


def delete_all_tracks(board: pcbnew.BOARD) -> int:
    n = len(list(board.GetTracks()))
    for track in list(board.GetTracks()):
        board.Delete(track)
    return n


def add_seg(
    board: pcbnew.BOARD,
    x1: float,
    y1: float,
    x2: float,
    y2: float,
    w: float,
    layer: int,
    net: str,
) -> None:
    if abs(x1 - x2) < 0.001 and abs(y1 - y2) < 0.001:
        return
    t = pcbnew.PCB_TRACK(board)
    t.SetStart(mm(x1, y1))
    t.SetEnd(mm(x2, y2))
    t.SetWidth(int(pcbnew.FromMM(w)))
    t.SetLayer(layer)
    t.SetNetCode(net_code(board, net))
    board.Add(t)


def add_via(board: pcbnew.BOARD, x: float, y: float, net: str, drill: float = 0.3) -> None:
    v = pcbnew.PCB_VIA(board)
    v.SetPosition(mm(x, y))
    v.SetWidth(int(pcbnew.FromMM(0.6)))
    v.SetDrill(int(pcbnew.FromMM(drill)))
    v.SetLayerPair(FCU, BCU)
    v.SetNetCode(net_code(board, net))
    board.Add(v)


def route_xy(
    board: pcbnew.BOARD,
    points: list[tuple[float, float]],
    w: float,
    layer: int,
    net: str,
) -> None:
    for (x1, y1), (x2, y2) in zip(points, points[1:]):
        if abs(x1 - x2) > 0.001:
            add_seg(board, x1, y1, x2, y1, w, layer, net)
        if abs(y1 - y2) > 0.001:
            add_seg(board, x2, y1, x2, y2, w, layer, net)


def p2p(
    board: pcbnew.BOARD,
    ref1: str,
    pad1: str,
    ref2: str,
    pad2: str,
    w: float,
    layer: int,
    net: str,
) -> None:
    route_xy(board, [pad_pos(board, ref1, pad1), pad_pos(board, ref2, pad2)], w, layer, net)


def p2p_via(
    board: pcbnew.BOARD,
    ref1: str,
    pad1: str,
    ref2: str,
    pad2: str,
    w: float,
    net: str,
    channel_x: float,
) -> None:
    """F.Cu stubs → B.Cu channel at channel_x → F.Cu stubs."""
    x1, y1 = pad_pos(board, ref1, pad1)
    x2, y2 = pad_pos(board, ref2, pad2)
    add_via(board, x1, y1, net)
    add_via(board, channel_x, y1, net)
    add_via(board, channel_x, y2, net)
    add_via(board, x2, y2, net)
    add_seg(board, x1, y1, channel_x, y1, w, FCU, net)
    route_xy(board, [(channel_x, y1), (channel_x, y2)], w, BCU, net)
    add_seg(board, channel_x, y2, x2, y2, w, FCU, net)


def bus_vertical(
    board: pcbnew.BOARD,
    x: float,
    y_lo: float,
    y_hi: float,
    w: float,
    layer: int,
    net: str,
) -> None:
    route_xy(board, [(x, y_lo), (x, y_hi)], w, layer, net)


def fill_zones(board: pcbnew.BOARD) -> None:
    zones = board.Zones()
    for z in zones:
        z.SetNeedRefill(True)
    pcbnew.ZONE_FILLER(board).Fill(zones)


def unconnected_count(board: pcbnew.BOARD) -> int:
    board.BuildConnectivity()
    return board.GetConnectivity().GetUnconnectedCount()


def export_pdf(board: pcbnew.BOARD, name: str, layer: int) -> None:
    AUDIT_DIR.mkdir(parents=True, exist_ok=True)
    plot = pcbnew.PLOT_CONTROLLER(board)
    opts = plot.GetPlotOptions()
    opts.SetOutputDirectory(str(AUDIT_DIR))
    opts.SetPlotFrameRef(False)
    plot.SetLayer(layer)
    plot.OpenPlotfile(name, pcbnew.PLOT_FORMAT_PDF, name)
    plot.PlotLayer()
    plot.ClosePlot()


def route_gnd(board: pcbnew.BOARD) -> None:
    """B.Cu ground spine + stitch via at every GND pad."""
    # Main B.Cu GND frame
    bus_vertical(board, 6.0, 3.0, 97.0, 0.8, BCU, "GND")
    route_xy(board, [(6.0, 97.0), (147.0, 97.0)], 0.8, BCU, "GND")
    route_xy(board, [(147.0, 97.0), (147.0, 3.0)], 0.8, BCU, "GND")
    route_xy(board, [(147.0, 3.0), (6.0, 3.0)], 0.8, BCU, "GND")

    for fp in board.GetFootprints():
        for pad in fp.Pads():
            if pad.GetNetname() != "GND":
                continue
            pos = pad.GetPosition()
            x, y = pcbnew.ToMM(pos.x), pcbnew.ToMM(pos.y)
            add_via(board, x, y, "GND")
            # Shortest path to frame on B.Cu
            if x < 75:
                route_xy(board, [(x, y), (6.0, y)], 0.5, BCU, "GND")
            else:
                route_xy(board, [(x, y), (147.0, y)], 0.5, BCU, "GND")
            if y < 50:
                route_xy(board, [(x if x < 75 else 147.0, y), (x if x < 75 else 147.0, 3.0)], 0.5, BCU, "GND")
            else:
                route_xy(board, [(x if x < 75 else 147.0, y), (x if x < 75 else 147.0, 97.0)], 0.5, BCU, "GND")


def route_12v(board: pcbnew.BOARD) -> None:
    # 12V_IN: TB1 → F1 → vertical bus x=19.3 → Q1 source pins
    p2p(board, "TB1", "1", "F1", "1", 1.5, FCU, "12V_IN")
    x_in = 19.3
    y_f1 = pad_pos(board, "F1", "1")[1]
    route_xy(board, [pad_pos(board, "F1", "1"), (x_in, y_f1), (x_in, 74.1)], 1.2, FCU, "12V_IN")
    for q_pad in ("1", "2", "3"):
        xq, yq = pad_pos(board, "Q1", q_pad)
        add_seg(board, xq, yq, x_in, yq, 0.8, FCU, "12V_IN")

    # 12V_SAFE: F1 → TB2/TP1 backbone → vertical bus x=24.7
    x_safe = 24.7
    p2p(board, "F1", "2", "TB2", "1", 1.5, FCU, "12V_SAFE")
    route_xy(
        board,
        [pad_pos(board, "F1", "2"), pad_pos(board, "TP1", "1"), (x_safe, 88.0)],
        1.5,
        FCU,
        "12V_SAFE",
    )
    bus_vertical(board, x_safe, 14.0, 88.0, 1.0, FCU, "12V_SAFE")
    for q_pad in ("5", "6", "7", "8"):
        xq, yq = pad_pos(board, "Q1", q_pad)
        add_seg(board, xq, yq, x_safe, yq, 0.8, FCU, "12V_SAFE")
    p2p(board, "D1", "1", "Q1", "8", 0.8, FCU, "12V_SAFE")
    p2p(board, "J_MINI", "1", "Q1", "8", 0.6, FCU, "12V_SAFE")
    for cap, buck in [("C1", "U1"), ("C4", "U2")]:
        xc, yc = pad_pos(board, cap, "1")
        add_seg(board, xc, yc, x_safe, yc, 0.8, FCU, "12V_SAFE")
        for bp in ("4", "5"):
            xb, yb = pad_pos(board, buck, bp)
            add_seg(board, xb, yb, x_safe, yb, 0.8, FCU, "12V_SAFE")


def route_5v(board: pcbnew.BOARD) -> None:
    # Star at L1/C2 junction
    for u_pad in ("1", "3", "6"):
        p2p(board, "U1", u_pad, "L1", "1", 0.8, FCU, "5V")
    p2p(board, "L1", "1", "L1", "2", 0.8, FCU, "5V")
    p2p(board, "L1", "2", "C2", "1", 0.8, FCU, "5V")
    p2p(board, "C2", "1", "C3", "1", 0.8, FCU, "5V")
    p2p(board, "C3", "1", "TP2", "1", 0.6, FCU, "5V")
    p2p(board, "TP2", "1", "D2", "2", 0.5, FCU, "5V")
    p2p(board, "D2", "2", "R2", "2", 0.5, FCU, "5V")
    # Loads
    p2p(board, "C3", "1", "TB4", "1", 0.8, FCU, "5V")
    p2p(board, "C3", "1", "J_IBT", "7", 0.8, FCU, "5V")
    p2p(board, "C3", "1", "J_RFID", "1", 0.5, FCU, "5V")
    # 5V to NRF on top edge
    x_nrf, y_nrf = pad_pos(board, "J_NRF", "2")
    x5, y5 = pad_pos(board, "C3", "1")
    route_xy(board, [(x5, y5), (x5, y_nrf), (x_nrf, y_nrf)], 0.8, FCU, "5V")


def route_3v3(board: pcbnew.BOARD) -> None:
    for u_pad in ("1", "3", "6"):
        p2p(board, "U2", u_pad, "L2", "1", 0.6, FCU, "3V3")
    p2p(board, "L2", "1", "L2", "2", 0.6, FCU, "3V3")
    p2p(board, "L2", "2", "C5", "1", 0.6, FCU, "3V3")
    p2p(board, "C5", "1", "C6", "1", 0.6, FCU, "3V3")
    p2p(board, "C6", "1", "TP3", "1", 0.6, FCU, "3V3")
    p2p(board, "TP3", "1", "D3", "2", 0.5, FCU, "3V3")
    p2p(board, "D3", "2", "R3", "2", 0.5, FCU, "3V3")
    p2p(board, "C6", "1", "J_NUC_L", "2", 0.6, FCU, "3V3")
    p2p(board, "C6", "1", "U3", "10", 0.6, FCU, "3V3")
    p2p(board, "C6", "1", "C7", "1", 0.5, FCU, "3V3")
    p2p(board, "C7", "1", "C8", "1", 0.5, FCU, "3V3")
    p2p(board, "C6", "1", "R4", "1", 0.5, FCU, "3V3")
    p2p(board, "C6", "1", "R5", "1", 0.5, FCU, "3V3")
    p2p(board, "C6", "1", "R6", "1", 0.5, FCU, "3V3")
    p2p(board, "C6", "1", "R7", "1", 0.5, FCU, "3V3")
    p2p(board, "C6", "1", "J_IBT", "3", 0.5, FCU, "3V3")
    p2p(board, "J_IBT", "3", "J_IBT", "4", 0.5, FCU, "3V3")
    p2p(board, "C6", "1", "J_HX711", "1", 0.5, FCU, "3V3")
    p2p(board, "C6", "1", "SJ_MINI", "1", 0.5, FCU, "3V3")
    p2p(board, "C6", "1", "J_MINI", "3", 0.5, FCU, "3V3")


def route_signals(board: pcbnew.BOARD) -> None:
    # I2C — B.Cu pair at x=72
    for net, nuc_p, pca_p, r_ref, r_p in [
        ("I2C_SCL", "8", "8", "R6", "2"),
        ("I2C_SDA", "7", "9", "R7", "2"),
    ]:
        x_n, y_n = pad_pos(board, "J_NUC_R", nuc_p)
        x_p, y_p = pad_pos(board, "U3", pca_p)
        x_r, y_r = pad_pos(board, r_ref, r_p)
        ch = 72.0
        add_via(board, x_n, y_n, net)
        add_via(board, ch, y_n, net)
        add_via(board, ch, y_p, net)
        add_via(board, ch, y_r, net)
        add_via(board, x_p, y_p, net)
        add_via(board, x_r, y_r, net)
        add_seg(board, x_n, y_n, ch, y_n, 0.25, FCU, net)
        route_xy(board, [(ch, y_n), (ch, y_p), (ch, y_r)], 0.25, BCU, net)
        add_seg(board, ch, y_p, x_p, y_p, 0.25, FCU, net)
        add_seg(board, ch, y_r, x_r, y_r, 0.25, FCU, net)

    # PWM — direct F.Cu
    p2p(board, "U3", "13", "J_IBT", "1", 0.35, FCU, "PWM0")
    p2p(board, "U3", "14", "J_IBT", "2", 0.35, FCU, "PWM1")

    # NRF SPI — single B.Cu bus x=118
    ch = 118.0
    spi = [
        ("NRF_CE", "J_NUC_R", "6", "J_NRF", "3"),
        ("NRF_CSN", "J_NUC_L", "7", "J_NRF", "4"),
        ("NRF_SCK", "J_NUC_L", "8", "J_NRF", "5"),
        ("NRF_MOSI", "J_NUC_L", "10", "J_NRF", "6"),
        ("NRF_MISO", "J_NUC_R", "9", "J_NRF", "7"),
    ]
    ys = []
    for net, r1, p1, r2, p2 in spi:
        p2p_via(board, r1, p1, r2, p2, 0.25, net, ch)
        ys.extend([pad_pos(board, r1, p1)[1], pad_pos(board, r2, p2)[1]])

    # Encoder
    p2p(board, "J_NUC_L", "4", "R4", "2", 0.25, FCU, "ENC_A")
    p2p(board, "R4", "2", "TB4", "3", 0.25, FCU, "ENC_A")
    p2p(board, "J_NUC_L", "5", "R5", "2", 0.25, FCU, "ENC_B")
    p2p(board, "R5", "2", "TB4", "4", 0.25, FCU, "ENC_B")

    # NUC_NRST tie (both Nucleo headers)
    p2p_via(board, "J_NUC_L", "13", "J_NUC_R", "3", 0.25, "NUC_NRST", 59.0)

    # DNP optional — route to clear ratsnest
    p2p_via(board, "J_NUC_L", "6", "J_RFID", "3", 0.25, "RFID_RX", 56.0)
    p2p_via(board, "J_NUC_R", "15", "J_HX711", "3", 0.25, "HX711_DOUT", 74.0)
    p2p_via(board, "J_NUC_R", "14", "J_HX711", "4", 0.25, "HX711_SCK", 75.0)


def main() -> int:
    board = pcbnew.LoadBoard(str(PCB_PATH))
    removed = delete_all_tracks(board)
    print(f"Removed {removed} tracks/vias")

    route_gnd(board)
    route_12v(board)
    route_5v(board)
    route_3v3(board)
    route_signals(board)

    fill_zones(board)
    board.Save(str(PCB_PATH))

    uc = unconnected_count(board)
    ntracks = len(list(board.GetTracks()))
    print(f"Saved {PCB_PATH}")
    print(f"Tracks+vias: {ntracks}")
    print(f"KiCad unconnected: {uc}")

    export_pdf(board, "wifeeder-actuator-F_Cu", FCU)
    export_pdf(board, "wifeeder-actuator-B_Cu", BCU)

    return 1 if uc > 0 else 0


if __name__ == "__main__":
    sys.exit(main())
