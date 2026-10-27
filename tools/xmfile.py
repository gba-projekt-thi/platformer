#!/usr/bin/env python3
"""
xmfile.py

Minimal FastTracker 2 XM module reader/writer (pure standard library), used by
tools/Make_boss_music.py to derive a boss track from a world's own module.

Patterns are decoded into rows of Note objects; instruments are kept as the
raw bytes of the file (header + samples), so a read/write round trip is
lossless for what the game's modules use. Only the sample count and the sample
data size are parsed out of an instrument.
"""

import struct
from dataclasses import dataclass, field

ID_TEXT = b"Extended Module: "
HEADER_SIZE = 276
# An instrument without samples: 29-byte header, no sample data.
EMPTY_INSTRUMENT = struct.pack("<I22sBH", 29, b"", 0, 0)


@dataclass
class Note:
    note: int = 0  # 0 = none, 1..96 = C-0..B-7, 97 = key off
    inst: int = 0  # 0 = none, 1..128
    vol: int = 0  # volume column byte
    fx: int = 0
    param: int = 0

    def empty(self):
        return not (self.note or self.inst or self.vol or self.fx or self.param)


@dataclass
class Pattern:
    rows: list  # rows[row][channel] -> Note


@dataclass
class Module:
    name: bytes
    tracker: bytes
    version: int
    restart: int
    channels: int
    flags: int
    tempo: int  # ticks per row ("speed")
    bpm: int
    order: list
    patterns: list = field(default_factory=list)
    instruments: list = field(default_factory=list)  # raw bytes each


def _unpack_pattern(data, rows, channels):
    out, pos = [], 0
    for _ in range(rows):
        row = []
        for _ in range(channels):
            n = Note()
            first = data[pos]
            pos += 1
            if first & 0x80:
                for bit, attr in enumerate(("note", "inst", "vol", "fx", "param")):
                    if first & (1 << bit):
                        setattr(n, attr, data[pos])
                        pos += 1
            else:
                n.note = first
                n.inst, n.vol, n.fx, n.param = data[pos : pos + 4]
                pos += 4
            row.append(n)
        out.append(row)
    return out


def _pack_pattern(rows):
    out = bytearray()
    for row in rows:
        for n in row:
            flags = 0x80
            body = bytearray()
            for bit, v in enumerate((n.note, n.inst, n.vol, n.fx, n.param)):
                if v:
                    flags |= 1 << bit
                    body.append(v)
            out.append(flags)
            out += body
    return bytes(out)


def _sample_sizes(raw):
    """Sample data lengths (bytes) of a raw instrument blob."""
    (hdr,) = struct.unpack_from("<I", raw, 0)
    (nsamples,) = struct.unpack_from("<H", raw, 27)
    if nsamples == 0:
        return []
    (sample_hdr,) = struct.unpack_from("<I", raw, 29)
    return [
        struct.unpack_from("<I", raw, hdr + sample_hdr * i)[0]
        for i in range(nsamples)
    ]


def instrument_sample_bytes(raw):
    return sum(_sample_sizes(raw))


def _instrument_end(data, pos):
    (hdr,) = struct.unpack_from("<I", data, pos)
    (nsamples,) = struct.unpack_from("<H", data, pos + 27)
    if nsamples == 0:
        return pos + hdr
    (sample_hdr,) = struct.unpack_from("<I", data, pos + 29)
    end = pos + hdr + sample_hdr * nsamples
    for i in range(nsamples):
        (n,) = struct.unpack_from("<I", data, pos + hdr + sample_hdr * i)
        end += n
    return end


def parse(data):
    if data[:17] != ID_TEXT:
        raise ValueError("not an XM file")
    name = data[17:37]
    tracker = data[38:58]
    version, hsize = struct.unpack_from("<HI", data, 58)
    songlen, restart, channels, npat, ninst, flags, tempo, bpm = struct.unpack_from(
        "<HHHHHHHH", data, 64
    )
    order = list(data[80 : 80 + songlen])
    pos = 60 + hsize
    patterns = []
    for _ in range(npat):
        phdr, _packing, rows, size = struct.unpack_from("<IBHH", data, pos)
        body = data[pos + phdr : pos + phdr + size]
        if size == 0:
            rows_data = [[Note() for _ in range(channels)] for _ in range(rows)]
        else:
            rows_data = _unpack_pattern(body, rows, channels)
        patterns.append(Pattern(rows_data))
        pos += phdr + size
    instruments = []
    for _ in range(ninst):
        end = _instrument_end(data, pos)
        instruments.append(bytes(data[pos:end]))
        pos = end
    return Module(name, tracker, version, restart, channels, flags, tempo, bpm,
                  order, patterns, instruments)


def write(mod):
    out = bytearray()
    out += ID_TEXT + mod.name[:20].ljust(20, b"\0") + b"\x1a"
    out += mod.tracker[:20].ljust(20, b"\0")
    out += struct.pack("<HI", mod.version, HEADER_SIZE)
    out += struct.pack("<HHHHHHHH", len(mod.order), mod.restart, mod.channels,
                       len(mod.patterns), len(mod.instruments), mod.flags,
                       mod.tempo, mod.bpm)
    out += bytes(mod.order).ljust(256, b"\0")
    for p in mod.patterns:
        packed = _pack_pattern(p.rows)
        out += struct.pack("<IBHH", 9, 0, len(p.rows), len(packed)) + packed
    for raw in mod.instruments:
        out += raw
    return bytes(out)


def used_instruments(mod):
    """Instrument numbers (1-based) used by patterns that are in the order list."""
    used = set()
    for pi in set(mod.order):
        for row in mod.patterns[pi].rows:
            for n in row:
                if n.inst:
                    used.add(n.inst)
    return used


def sample_total(mod):
    return sum(instrument_sample_bytes(r) for r in mod.instruments)
