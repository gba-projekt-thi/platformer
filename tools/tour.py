#!/usr/bin/env python3
"""
tour.py

A scripted walk through the real game - menus, story pages, levels with
their recordings, the five boss fights, the ending - photographed in mGBA.
For a look at everything that only shows on screen, without playing it.

    make tour                                   build + run + report
    python3 tools/tour.py generate              platformer.tour/tour_data.h
    python3 tools/tour.py run ROM OUT_DIR       photograph a built tour ROM
    python3 tools/tour.py compare OLD NEW       shots that changed between runs

The tour ROM (PLATFORMER_TOUR, game/src/tour.cpp) replays the menu buttons
below with Butano's keypad commands (one per frame); in a level with a
recording in game/replays/ that recording moves the duck. Slot 1 gets a new
save for every part of the tour (SLOT_1), slot 2 one with everything
unlocked. `run` starts mgba-qt without a window (Qt's VNC platform); the ROM
logs "TOUR SHOT <n>" at the frame of every shot, and the next screen read
over VNC is kept. Then it writes OUT_DIR/report.html, OUT_DIR/contact.png
(every shot on one sheet) and OUT_DIR/shots.json. `compare` lists the shots
whose picture changed between two runs - a visual regression check.
"""

import json
import os
import queue
import re
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "platformer.tour"
FPS = 59.7275  # the GBA's frame rate

# bn::keypad::key_type bits
KEYS = {"A": 1, "B": 2, "SELECT": 4, "START": 8, "RIGHT": 16, "LEFT": 32,
        "UP": 64, "DOWN": 128, "R": 256, "L": 512}


class Tour:
    """Builds the frame-by-frame keypad and the list of shots."""

    def __init__(self):
        self.frames = []   # held keys per frame
        self.shots = []    # (frame, name, what to look at)

    def wait(self, frames):
        self.frames += [0] * frames
        return self

    def press(self, key, times=1):
        for _ in range(times):
            self.frames += [KEYS[key]] * 3 + [0] * 9
        return self

    def shot(self, name, check):
        self.shots.append((len(self.frames), name, check))
        return self.wait(30)


def keypad_commands(frames):
    """Butano's format (bn_keypad_manager.cpp): two characters per frame,
    the low 5 key bits and the rest, each as '0' + value."""
    return "".join(chr(48 + (k & 31)) + chr(48 + (k >> 5)) for k in frames)


def _levels():
    sys.path.insert(0, str(ROOT / "tools"))
    import importlib.util
    spec = importlib.util.spec_from_file_location("cwi", ROOT / "tools/Check_world_index.py")
    cwi = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(cwi)
    return cwi.parse_levels((ROOT / "game/src/main.cpp").read_text())


def story_page_counts():
    """Pages of each story chapter in the order of story.h (intro, worlds 2-5,
    ending), read from the data so the tour follows edits to the text."""
    text = (ROOT / "game/include/story.h").read_text()
    chunks = text.split("Picture::")[1:]  # one per chapter (the enum has no ::)
    # A chapter ends "<last page's line count>}}, <page count>};" or ",".
    return [int(re.search(r"\}\},\s*(\d+)\}[;,]", c).group(1)) for c in chunks]


def replay_steps(name):
    path = ROOT / "game/replays" / f"{name}.json"
    return sum(c for _, c in json.loads(path.read_text())["runs"])


def build_tour(levels):
    """The tour, and slot 1's save for every time the start screen opens
    ((level, furthest) - tour.cpp writes it then)."""
    t = Tour()
    slot_1 = []
    index = {name: i for i, name in enumerate(levels)}
    fade = 120  # frames until a new scene has faded in
    pages = story_page_counts()  # intro, worlds 2-5, ending

    def story(chapter, name, what):
        """Photograph every page of a story chapter; A turns each page (the
        last one is left for the caller to confirm)."""
        for page in range(pages[chapter]):
            if page:
                t.press("A").wait(10)
            t.shot(name if page == 0 else f"{name}_p{page + 1}",
                   f"{what}, Seite {page + 1}/{pages[chapter]}")

    def start_screen(level, furthest):
        """The start screen opens (again); slot 1 holds this save."""
        slot_1.append((level, furthest))
        t.wait(fade)

    def to_title(level, furthest):
        t.press("START").wait(20).press("DOWN", 4).press("A")
        start_screen(level, furthest)

    # Start screen, controls, a new game's intro, the pause menu.
    start_screen(0, 0)
    t.shot("start", "Startbildschirm: drei Slots und Controls")
    t.press("DOWN", 3).press("A").wait(fade)
    t.shot("controls", "Steuerung: 6 Zeilen, darunter 'Press B to go back' ganz sichtbar")
    t.press("B")
    start_screen(0, 0)
    # Slot 2's statistics (the seeded save), at the top and scrolled down.
    t.press("DOWN").press("SELECT").wait(fade)
    t.shot("statistics", "Statistik Slot 2: Summen und die ersten Level, Titel und Hinweis sichtbar")
    t.press("DOWN", 12)
    t.shot("statistics_scrolled", "Statistik gescrollt: Tode pro Level in der Mitte der Liste")
    t.press("B")
    start_screen(0, 0)
    t.press("A").wait(fade)
    story(0, "story_intro", "Story: Intro bei einem neuen Spiel")
    t.press("A").wait(fade).shot("level0", "Level 1 beginnt")
    t.press("START").wait(20).shot("pause_level0", "Pausemenü über Level 1: Level abgedunkelt, Text hell")
    t.press("DOWN", 2).press("A").wait(20)
    t.shot("pause_options", "Pausemenü, Optionen: Level abgedunkelt")
    t.press("B").wait(20).press("DOWN").press("A").wait(20)
    t.shot("pause_controls", "Pausemenü, Steuerung: 6 Zeilen ohne Überschrift")
    t.press("B").wait(20).press("DOWN").press("A")

    # Each world's boss; the door leads to the next world's story page
    # (World 5: the finale, the ending, the reunion, the summary).
    bosses = [n for n in levels if n.endswith("_BOSS")]
    for world, boss in enumerate(bosses, start=1):
        start_screen(index[boss], index[boss])
        steps = replay_steps(boss)
        t.press("A").wait(fade)
        done = 0  # frames of the recording played so far
        for k, part in enumerate((0.25, 0.4, 0.55, 0.7, 0.85)):
            target = int(steps * part)
            t.wait(max(0, target - done))
            t.shot(f"boss{world}_{k + 1}", f"Boss Welt {world}, Kampf ({int(part * 100)} %)")
            done = target + 30
            if k == 0:
                # The pause menu on this world's background: the level is
                # dimmed, the white text stands out. The recording waits.
                t.press("START").wait(20).shot(
                    f"pause_world{world}", f"Pausemenü über Welt {world}: Level abgedunkelt, Text hell")
                t.press("START")
                done += 12 + 12
        # The door finish holds the world still for a moment (clear_logic.h):
        # the door blinks and "Clear!" stands on the screen.
        t.wait(max(0, steps - done) + 20)
        t.shot(f"boss{world}_clear", f"Boss Welt {world}: Tür erreicht, 'Clear!' und die blinkende Tür")
        t.wait(100)
        t.shot(f"boss{world}_won", f"Boss Welt {world}: besiegt, Trittstein und Tür, Banner 'New Best!'")
        t.press("A").wait(fade)
        if world < len(bosses):
            story(world, f"story_w{world + 1}", f"Story-Kapitel Welt {world + 1}")
            t.press("A").wait(fade)
            t.shot(f"world{world + 1}_first",
                   f"Erstes Level von Welt {world + 1}, zum ersten Mal: der Einführungshinweis, falls es einen gibt")
            t.press("START").wait(20).press("DOWN", 4).press("A")
        else:
            finale = levels[-1]
            t.wait(replay_steps(finale) + 120)
            t.shot("finale_door", "Finale: Susanne erreicht, Banner")
            t.press("A").wait(fade)
            story(len(pages) - 1, "story_ending", "Abschlusstext vor der Kuss-Szene")
            t.press("A").wait(fade).shot("kiss", "Kuss-Szene")
            t.wait(330).shot("summary", "Zusammenfassung")
            t.press("A")

    # Slot 2 (all unlocked): World 4's level select and Bramble Hop.
    start_screen(*slot_1[-1])
    pogo = "LEVEL_WORLD4_POGO"
    t.press("DOWN").press("B").wait(fade).press("DOWN", 3).press("A").wait(fade)
    t.shot("select_w4", "Levelauswahl Welt 4: 5 Level, Bramble Hop zuerst; Ziel Ohne Tod erreicht, Zeit nicht")
    t.press("DOWN", 2).shot("select_w4_both", "Levelauswahl Welt 4, drittes Level: beide Ziele erreicht")
    t.press("UP", 2)
    t.press("A").wait(fade).shot("pogo_start", "Bramble Hop über die Levelauswahl (schon geschafft: ohne Hinweis)")
    steps = replay_steps(pogo)
    t.wait(steps // 3).shot("pogo_1", "Bramble Hop: Abprall vom ersten Busch")
    t.wait(steps // 3).shot("pogo_2", "Bramble Hop: zweite Grube, zwei Büsche")
    t.wait(steps // 3 + 120).press("A").wait(fade)
    to_title(*slot_1[-1])

    # Slot 2: Hedge Your Bets, World 4's delayed trap chain before the boss.
    trick = "LEVEL_WORLD4_TRICK"
    t.press("DOWN").press("B").wait(fade).press("DOWN", 3).press("A").wait(fade)
    t.press("DOWN", 3).press("A").wait(fade).shot("trick_start", "Hedge Your Bets: Start vor der ersten Grube")
    steps = replay_steps(trick)
    done = 0  # frames of the recording played so far
    for name, step, check in (
            ("trick_1", 60, "Hedge Your Bets: Absprung zur ersten Grube"),
            ("trick_2", 90, "Hedge Your Bets: Abprall vom Busch, Landung auf der Kettenplatte"),
            ("trick_3", 125, "Hedge Your Bets: der Ziegel fällt auf den fernen Korb, die Ente wartet"),
            ("trick_4", 150, "Hedge Your Bets: der Busch sprießt aus dem fernen Korb"),
            ("trick_5", 185, "Hedge Your Bets: wieder weg, die Ente geht zur zweiten Grube")):
        t.wait(max(0, step - done))
        t.shot(name, check)
        done = step + 30
    t.wait(max(0, steps - done) + 120).press("A").wait(fade)
    to_title(*slot_1[-1])

    # Slot 2: World 5's level select (7 rows) and Barrel Tower.
    climb = "LEVEL_WORLD5_CLIMB"
    t.press("DOWN").press("B").wait(fade).press("DOWN", 4).press("A").wait(fade)
    t.shot("select_w5", "Levelauswahl Welt 5: alle 7 Zeilen sichtbar; erstes Level nur mit Zeitziel")
    t.press("DOWN", 4).press("A").wait(fade).shot("climb_start", "Barrel Tower: Start, Tür auf dem Boden")
    steps = replay_steps(climb)
    t.wait(steps // 4).shot("climb_door", "Barrel Tower: die Tür ist auf den Turm geflohen")
    t.wait(steps // 4).shot("climb_shaft", "Barrel Tower: Wandsprung im Schacht")
    t.wait(steps // 4).shot("climb_top", "Barrel Tower: oben an der Tür")
    t.wait(steps - 3 * (steps // 4) + 120).press("A").wait(fade)
    to_title(*slot_1[-1])

    # Slot 2 has finished the game: Mirror Mode (SELECT in the level select),
    # an ordinary level and World 1's boss, flipped left-right.
    scroll, boss = "LEVEL_WORLD1_SCROLL", "LEVEL_WORLD1_BOSS"
    t.press("DOWN").press("B").wait(fade).press("A").wait(fade)
    t.shot("select_mirror_off", "Levelauswahl Welt 1: oben 'Mirror off (Select)', Zeilen darunter unverdeckt")
    t.press("SELECT").wait(20)
    t.shot("select_mirror_on", "Levelauswahl Welt 1: oben 'Mirror ON (Select)'")
    t.press("DOWN", 2).press("A").wait(fade)
    t.shot("mirror_scroll_start", "Gespiegeltes Level: Ente links-rechts vertauscht, Bilder gedreht, Tür links")
    steps = replay_steps(scroll)
    t.wait(steps // 2).shot("mirror_scroll_mid", "Gespiegeltes Level, Mitte: Ente läuft nach links, Kamera folgt")
    t.wait(steps // 2 + 120).press("A").wait(fade)
    to_title(*slot_1[-1])
    t.press("DOWN").press("B").wait(fade).press("A").wait(fade).press("DOWN", 4).press("A").wait(fade)
    steps = replay_steps(boss)
    done = 0
    for k, part in enumerate((0.15, 0.45, 0.75)):
        target = int(steps * part)
        t.wait(max(0, target - done))
        t.shot(f"mirror_boss_{k + 1}", f"Gespiegelter Boss Welt 1 ({int(part * 100)} %): Gegner, Plattformen und Tür seitenverkehrt")
        done = target + 30
    t.wait(max(0, steps - done) + 120)
    return t, slot_1


def generate():
    t, slot_1 = build_tour(_levels())
    OUT.mkdir(exist_ok=True)
    lines = ["#pragma once", "", "// Generated by tools/tour.py - do not edit.", "",
             "namespace tour_data {", "",
             "struct Save {", "    int level;", "    int furthest;", "};", "",
             "inline constexpr Save SLOT_1[] = {"
             + ", ".join(f"{{{a}, {b}}}" for a, b in slot_1) + "};",
             f"inline constexpr int SLOT_1_COUNT = {len(slot_1)};", "",
             "// The frame of every shot (tour.cpp logs \"TOUR SHOT <n>\" then).",
             "inline constexpr int SHOT_FRAMES[] = {" + ", ".join(str(f) for f, _, _ in t.shots) + "};",
             f"inline constexpr int SHOT_COUNT = {len(t.shots)};", "",
             f'inline constexpr char KEYPAD[] = "{keypad_commands(t.frames)}";', "",
             "}  // namespace tour_data", ""]
    (OUT / "tour_data.h").write_text("\n".join(lines))
    (OUT / "tour.json").write_text(json.dumps(
        {"frames": len(t.frames), "shots": [{"frame": f, "name": n, "check": c} for f, n, c in t.shots]},
        ensure_ascii=False, indent=1))
    print(f"tour: {len(t.frames)} frames ({len(t.frames) / FPS:.0f} s), {len(t.shots)} shots")
    return 0


# ---- photographing over VNC ---------------------------------------------------
def _recv(sock, n):
    data = bytearray()
    while len(data) < n:
        chunk = sock.recv(n - len(data))
        if not chunk:
            raise ConnectionError("VNC connection closed")
        data += chunk
    return bytes(data)


def write_png(path, width, height, fb):
    """fb: 32 bpp little endian (blue, green, red, pad)."""
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        row = fb[y * width * 4:(y + 1) * width * 4]
        raw += bytes(b for x in range(width) for b in (row[4 * x + 2], row[4 * x + 1], row[4 * x]))

    def chunk(kind, data):
        return (struct.pack(">I", len(data)) + kind + data
                + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF))

    Path(path).write_bytes(b"\x89PNG\r\n\x1a\n"
                           + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
                           + chunk(b"IDAT", zlib.compress(bytes(raw), 6)) + chunk(b"IEND", b""))


def read_png(path):
    """(width, height, RGB bytes) of a PNG written by write_png()."""
    data = Path(path).read_bytes()
    pos, idat = 8, b""
    while pos < len(data):
        n, = struct.unpack(">I", data[pos:pos + 4])
        kind, body = data[pos + 4:pos + 8], data[pos + 8:pos + 8 + n]
        if kind == b"IHDR":
            w, h = struct.unpack(">II", body[:8])
        elif kind == b"IDAT":
            idat += body
        pos += 12 + n
    raw = zlib.decompress(idat)
    stride = w * 3 + 1
    return w, h, b"".join(raw[y * stride + 1:(y + 1) * stride] for y in range(h))


def changed(a, b, tolerance=24):
    """Share of pixels (RGB bytes) that differ by more than `tolerance` in a
    colour channel."""
    pixels = len(a) // 3
    diff = 0
    for i in range(0, len(a), 3):
        if (abs(a[i] - b[i]) > tolerance or abs(a[i + 1] - b[i + 1]) > tolerance
                or abs(a[i + 2] - b[i + 2]) > tolerance):
            diff += 1
    return diff / pixels


MENU_BAR = 26  # mgba-qt's menu above the 480x320 picture


def contact_sheet(out_dir, records, columns=4):
    """All shots at half size on one picture (contact.png)."""
    cw, ch = 240, 160
    rows = (len(records) + columns - 1) // columns
    W, H = columns * cw, rows * ch
    fb = bytearray(W * H * 4)
    for i, r in enumerate(records):
        w, _, rgb = read_png(Path(out_dir) / r["image"])
        ox, oy = (i % columns) * cw, (i // columns) * ch
        for y in range(ch):
            row = (MENU_BAR + 2 * y) * w * 3
            for x in range(cw):
                p = row + 6 * x
                o = ((oy + y) * W + ox + x) * 4
                fb[o], fb[o + 1], fb[o + 2] = rgb[p + 2], rgb[p + 1], rgb[p]
    write_png(Path(out_dir) / "contact.png", W, H, fb)


def compare(old_dir, new_dir, threshold=0.02):
    """Shots whose picture changed between two runs (more than `threshold`
    of the pixels). Animations and capture timing move a few pixels, so
    small changes are normal."""
    old = {r["name"]: r for r in json.loads((Path(old_dir) / "shots.json").read_text())}
    report = []
    for r in json.loads((Path(new_dir) / "shots.json").read_text()):
        if r["name"] not in old:
            report.append((r["name"], None))
            continue
        _, _, a = read_png(Path(old_dir) / old[r["name"]]["image"])
        _, _, b = read_png(Path(new_dir) / r["image"])
        report.append((r["name"], changed(a, b) if len(a) == len(b) else 1.0))
    for name, share in report:
        if share is None:
            print(f"NEW      {name}")
        elif share > threshold:
            print(f"CHANGED  {name}: {share:.1%} of the pixels")
    flagged = sum(1 for _, s in report if s is None or s > threshold)
    print(f"\n{flagged} of {len(report)} shots changed (threshold {threshold:.0%}).")
    return 0


def capture(port, shot_count, signals, timeout):
    """Keeps the screen at every "TOUR SHOT <n>" the ROM logs (`signals`, a
    queue of shot numbers fed from mgba-qt's log): the next frame VNC sends."""
    s = None
    for _ in range(100):
        try:
            s = socket.create_connection(("127.0.0.1", port), timeout=10)
            break
        except OSError:
            time.sleep(0.2)
    if s is None:
        raise RuntimeError("mgba-qt did not open its VNC port")
    _recv(s, 12)
    s.sendall(b"RFB 003.003\n")
    _recv(s, 4)
    s.sendall(b"\x01")
    w, h = struct.unpack(">HH", _recv(s, 4))
    _recv(s, 16)
    _recv(s, struct.unpack(">I", _recv(s, 4))[0])
    s.sendall(struct.pack(">B3xBBBBHHHBBB3x", 0, 32, 24, 0, 1, 255, 255, 255, 16, 8, 0))
    s.sendall(struct.pack(">BxHi", 2, 1, 0))
    fb = bytearray(w * h * 4)
    kept = {}
    waiting = []
    deadline = time.time() + timeout
    while len(kept) < shot_count and time.time() < deadline:
        while not signals.empty():
            waiting.append(signals.get())
        s.sendall(struct.pack(">BBHHHH", 3, 0, 0, 0, w, h))
        if _recv(s, 1)[0] != 0:
            continue
        _recv(s, 1)
        for _ in range(struct.unpack(">H", _recv(s, 2))[0]):
            x, y, rw, rh, _enc = struct.unpack(">HHHHi", _recv(s, 12))
            data = _recv(s, rw * rh * 4)
            for row in range(rh):
                o = ((y + row) * w + x) * 4
                fb[o:o + rw * 4] = data[row * rw * 4:(row + 1) * rw * 4]
        for n in waiting:
            kept[n] = bytes(fb)
        waiting = []
    return w, h, kept


def run(rom, out_dir):
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    tour = json.loads((OUT / "tour.json").read_text())
    shots = [(s["frame"], s["name"], s["check"]) for s in tour["shots"]]
    work = Path(tempfile.mkdtemp(prefix="tour-"))
    (work / "xdg").mkdir(mode=0o700)
    game = work / "platformer.gba"   # its own .sav: the player's saves stay untouched
    game.write_bytes(Path(rom).read_bytes())
    # mgba-qt prints the game's log (the shot signals) with this config
    (work / ".config/mgba").mkdir(parents=True)
    (work / ".config/mgba/config.ini").write_text("[ports.qt]\nlogToStdout=1\nlogLevel=127\n")
    port = 5900 + os.getpid() % 90
    env = dict(os.environ, HOME=str(work), QT_QPA_PLATFORM=f"vnc:size=480x360:port={port}",
               XDG_RUNTIME_DIR=str(work / "xdg"))
    proc = subprocess.Popen(["stdbuf", "-oL", "mgba-qt", "-2", str(game)], cwd=work, env=env,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    signals = queue.Queue()

    def read_log():
        for line in proc.stdout:
            if "TOUR SHOT " in line:
                signals.put(int(line.rsplit("TOUR SHOT ", 1)[1].split()[0]))

    threading.Thread(target=read_log, daemon=True).start()
    try:
        w, h, kept = capture(port, len(shots), signals, tour["frames"] / FPS * 2 + 60)
    finally:
        proc.terminate()
    records = []
    for n, (frame, name, check) in enumerate(shots):
        if n not in kept:
            continue
        fb = kept[n]
        write_png(out_dir / f"{name}.png", w, h, fb)
        records.append({"name": name, "check": check, "frame": frame, "image": f"{name}.png"})
    (out_dir / "shots.json").write_text(json.dumps(records, ensure_ascii=False, indent=1))
    contact_sheet(out_dir, records)
    items = "\n".join(
        f'<figure><img src="{r["image"]}" alt="{r["check"]}"><figcaption>'
        f'<b>{r["name"]}</b> · Frame {r["frame"]}<br>{r["check"]}</figcaption></figure>'
        for r in records)
    (out_dir / "report.html").write_text(
        "<!doctype html><meta charset=utf-8><title>Tour</title><style>"
        "body{font:15px system-ui;margin:24px;background:#eee}"
        "main{display:grid;grid-template-columns:repeat(auto-fill,minmax(360px,1fr));gap:20px}"
        "img{width:100%;image-rendering:pixelated;background:#000}figure{margin:0}"
        "</style><h1>Tour</h1><p><a href=contact.png>Alle Bilder auf einem Blatt</a></p>"
        "<main>" + items + "</main>")
    print(f"{len(records)} of {len(shots)} shots in {out_dir}")
    return 0 if len(records) == len(shots) else 1


def main(argv):
    if argv[:1] == ["generate"]:
        return generate()
    if argv[:1] == ["run"] and len(argv) == 3:
        return run(argv[1], argv[2])
    if argv[:1] == ["compare"] and len(argv) == 3:
        return compare(argv[1], argv[2])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
