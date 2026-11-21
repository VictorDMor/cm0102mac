#!/usr/bin/env python3
"""Draft the data of COMP_UTIL.CPP's two table routines from the instruction database.

0x1046f1f0 (a round code and a kind byte, under the game-mode byte, to a small number) and 0x10469bb0 (a round
code to its localized name) are jump tables compiled as code: 74 and 27 `bctr` dispatches whose arms are all of
a few shapes (`li r3,n; b exit`, one localized-format call each). This tool walks them without reading them by
hand:

  * it follows the routine's own dispatch code with a small emulator (the dozen instructions the dispatch uses;
    anything else stops it) for every code from -32768 to 32767, under every mode and kind class it finds, so the
    range checks, the gaps and the out-of-range paths come from the code and not from a transcription;
  * it classifies the arm each case reaches by shape and fails, listing them, on any arm of another shape;
  * it checks that every jump-table entry is reached by some class (nothing in a table is left unread);
  * it writes native/comp_util_rounds_tables.inc, the constexpr data comp_util_rounds.cpp reads.

  tools/comp_util_tables.py report   shapes, classes and counts (nothing written)
  tools/comp_util_tables.py emit     (re)write the .inc
  tools/comp_util_tables.py check    exit 1 when the .inc differs from what the database gives

The data is read from instructions.sqlite and segment-1.bin of the audited export (see check_jump_tables.py).
Names of the original's strings are not written: only offsets into the unit's string pool."""
import argparse
import collections
import json
from pathlib import Path
import re
import sqlite3
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from check_jump_tables import AUDITED_EXECUTABLE, TOC, table_cases  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "native/comp_util_rounds_tables.inc"

NUMBER_ROUTINE = (0x1046F1F0, 0x10476774)   # a code, a kind byte, the mode byte -> a number
TEXT_ROUTINE = (0x10469BB0, 0x1046F1F0)     # a code, a kind byte -> the round's name in a buffer
SPRINTF, FORMAT, STRCPY = 0x1077B720, 0x10727890, 0x1077DBF0
POOL_SLOT, EMPTY_TEXT, ERROR_TEXT = -0x47E4, 0x5380, 0x5565   # the unit's string pool; the empty string; the error text
NUMBER_EXIT, NUMBER_ZERO = 0x10476760, 0x1047675C              # `lwz r0,0x58(r1)` (the epilogue) and `li r3,0` before it
TEXT_EXIT, TEXT_ERROR_EXIT = 0x1046F1D8, 0x1046F1D4            # the epilogue, and `or r3,r31,r31` before it
MODES = (2, 3, 4, 5, 9)   # the mode bytes 0x1046f1f0 distinguishes (every other value takes the same path)
TEXT_KINDS = (0, 1, 3)    # the kind bytes 0x10469bb0 tables (every other value reports an assertion)
CODES = range(-32768, 32768)


def s32(x):
    x &= 0xFFFFFFFF
    return x - (1 << 32) if x >> 31 else x


def sext(x, bits):
    x &= (1 << bits) - 1
    return x - (1 << bits) if x >> (bits - 1) else x


class Export:
    def __init__(self, export):
        meta = json.loads((export / "program.json").read_text())
        if meta["executableSHA256"] != AUDITED_EXECUTABLE:
            raise SystemExit("comp_util_tables.py supports only the audited CM 01/02 Mac executable")
        self.segments = [(s["address"], (export / s["file"]).read_bytes()) for s in meta["segments"]
                         if s.get("initialized") and "file" in s and (export / s["file"]).exists()]
        self.db = sqlite3.connect((export / "instructions.sqlite").resolve().as_uri() + "?mode=ro", uri=True)

    def word(self, address):
        for start, data in self.segments:
            if start <= address and address + 4 <= start + len(data):
                return struct.unpack_from(">I", data, address - start)[0]
        return None

    def byte(self, address):
        for start, data in self.segments:
            if start <= address < start + len(data):
                return data[address - start]
        return None

    def assembly(self, lo, hi):
        return dict(self.db.execute("SELECT address, json_extract(body, '$.assembly') FROM instructions "
                                    "WHERE address >= ? AND address < ? ORDER BY address", (lo, hi)))


class Unrecognised(Exception):
    pass


class Stop:
    """Where an emulation ended; `calls` the skipped calls it passed (return address, target), `written` the
    registers it wrote, `after_call` those written since the last skipped call."""
    def __init__(self, kind, pc, target, regs, written, calls, after_call):
        self.kind, self.pc, self.target, self.regs, self.written = kind, pc, target, regs, written
        self.calls, self.after_call = calls, after_call


_decoded = {}


def operands(text):
    if text not in _decoded:
        op, _, rest = text.partition(" ")
        _decoded[text] = (op, [x for x in re.split(r"[,()]", rest) if x])
    return _decoded[text]


def reg(text):
    return int(text[1:])


class Emulator:
    """The instructions a table routine's dispatch uses. Stops at a `bctr`, at a call it does not skip (the
    error report) and at the epilogue's `lmw`; any other instruction raises Unrecognised."""
    CELL_OBJECT = 0x70000000   # what the game-mode cell holds in the emulation: only the byte at +8 is read

    def __init__(self, export, assembly):
        self.export, self.asm = export, assembly
        self.cell = export.word(TOC - 0x7AC8)  # r27: the address of the cell the mode byte hangs from

    def run(self, entry, regs, mode=0, skip=()):
        r = {i: 0xE0000000 + i for i in range(32)}
        r[1], r[2] = 0x7F000000, TOC
        r.update(regs)
        written, after_call, calls, flags, ctr, pc = set(), set(), [], None, None, entry

        def put(n, value):
            r[n] = value & 0xFFFFFFFF
            written.add(n)
            after_call.add(n)
        while True:
            text = self.asm[pc]
            op, a = operands(text)
            following = pc + 4
            if op in ("mfspr", "stmw", "stb") or (op == "mtspr" and a[0] == "LR"):
                pass
            elif op in ("stw", "stwu"):
                if op == "stwu":
                    put(1, r[reg(a[2])] + int(a[1], 0))
            elif op == "lwz":
                base, offset = reg(a[2]), int(a[1], 0)
                if base == 2:
                    put(reg(a[0]), self.export.word((r[2] + offset) & 0xFFFFFFFF))
                elif base == 1:
                    put(reg(a[0]), 0xBBBB0000)  # the epilogue reloads the saved link register
                elif base == 27 and offset == 0:
                    put(reg(a[0]), self.CELL_OBJECT)
                else:
                    raise Unrecognised(text)
            elif op == "lbz" and reg(a[2]) == 3 and r[3] == self.CELL_OBJECT and int(a[1], 0) == 8:
                put(reg(a[0]), mode)
            elif op == "lwzx":
                put(reg(a[0]), self.export.word((r[reg(a[1])] + r[reg(a[2])]) & 0xFFFFFFFF))
            elif op == "or":
                put(reg(a[0]), r[reg(a[1])] | r[reg(a[2])])
            elif op in ("extsb", "extsb."):
                put(reg(a[0]), sext(r[reg(a[1])], 8))
                if op == "extsb.":
                    flags = (s32(r[reg(a[0])]), 0)
            elif op == "extsh":
                put(reg(a[0]), sext(r[reg(a[1])], 16))
            elif op == "li":
                put(reg(a[0]), int(a[1], 0))
            elif op in ("addi", "subi", "addis"):
                sign, shift = (-1, 0) if op == "subi" else (1, 16 if op == "addis" else 0)
                put(reg(a[0]), r[reg(a[1])] + sign * (int(a[2], 0) << shift))
            elif op == "rlwinm" and a[2:] == ["0x2", "0x0", "0x1d"]:
                put(reg(a[0]), (r[reg(a[1])] << 2) & 0xFFFFFFFC)   # slwi 2: the index of a word table
            elif op in ("cmpwi", "cmplwi"):
                x, y = r[reg(a[-2])], int(a[-1], 0)
                flags = (s32(x), s32(y)) if op == "cmpwi" else (x, y & 0xFFFFFFFF)
            elif op == "mtspr" and a[0] == "CTR":
                ctr = r[reg(a[1])]
            elif op in ("bne", "beq", "bgt", "bge", "blt", "ble"):
                x, y = flags
                taken = {"bne": x != y, "beq": x == y, "bgt": x > y, "bge": x >= y, "blt": x < y, "ble": x <= y}[op]
                if taken:
                    following = int(a[0], 0)
            elif op == "b":
                following = int(a[0], 0)
            elif op == "lmw":
                return Stop("exit", pc, None, r, written, calls, after_call)
            elif op == "bctr":
                return Stop("bctr", pc, ctr, r, written, calls, after_call)
            elif op == "bl":
                if int(a[0], 0) in skip:
                    calls.append((pc + 4, int(a[0], 0)))
                    for n in range(3, 11):
                        put(n, 0xC0000000 + n)   # what the skipped callee leaves: a marker, never compared
                    after_call.clear()
                else:
                    return Stop("bl", pc, int(a[0], 0), r, written, calls, after_call)
            else:
                raise Unrecognised(text)
            pc = following


def runs(sequence):
    """(first, last, value) of each run of equal values in a list of (code, value)."""
    out = []
    for code, value in sequence:
        if out and out[-1][2] == value and out[-1][1] == code - 1:
            out[-1][1] = code
        else:
            out.append([code, code, value])
    return out


# ---------------------------------------------------------------------------------------------------------------
# 0x1046f1f0: round code -> number
# ---------------------------------------------------------------------------------------------------------------
class Numbers:
    def __init__(self, export):
        self.export = export
        self.asm = export.assembly(*NUMBER_ROUTINE)
        self.emu = Emulator(export, self.asm)
        self.shapes = collections.Counter()

    def arm(self, address):
        """('number', n) or ('error', site): the shapes an arm of this routine has. The error arm is the
        report's three instructions (`bl` sprintf into the shared buffer); control then goes on at the next
        instruction, which is `li r3,0` (a branch to it) or the next range test (a fall-through)."""
        a = self.asm
        first, second = a.get(address, ""), a.get(address + 4, "")
        if first.startswith("li r3,") and second == f"b 0x{NUMBER_EXIT:x}":
            value = int(first.split(",")[1], 0)
            if not 0 <= value <= 9:
                raise Unrecognised(f"{address:x}: number {value}")
            return ("number", value)
        if [a.get(address + 4 * k) for k in range(3)] == ["or r3,r31,r31", f"addi r4,r30,0x{ERROR_TEXT:x}", f"bl 0x{SPRINTF:x}"] \
                and a.get(address + 12) in (f"b 0x{NUMBER_ZERO:x}", "lwz r3,0x0(r27)"):
            return ("error", address + 12)
        raise Unrecognised(f"{address:x}: {first} | {second}")

    def classify_arms(self):
        """Every case of every jump table, by shape; fails listing the arms of no recognised shape."""
        bad = []
        for pc, text in self.asm.items():
            if text == "bctr":
                for case, target in enumerate(table_cases(self.asm, self.export.word, pc)):
                    try:
                        shape = self.arm(target)
                        self.shapes[shape[0] + (" falling through" if shape[0] == "error" and
                                                self.asm[target + 12] != f"b 0x{NUMBER_ZERO:x}" else "")] += 1
                    except Unrecognised as why:
                        bad.append(f"bctr {pc:x} case {case}: {why}")
        if bad:
            raise SystemExit("arms of no recognised shape:\n  " + "\n  ".join(bad))

    def outcome(self, code, kind, mode):
        """What the routine does for one input: (tabled, shape, value, site). shape 'number' (value), 'error'
        (a report of the error text from `site`, then 0) or 'none' (0, nothing else)."""
        regs = {3: code & 0xFFFFFFFF, 4: kind}
        stop = self.emu.run(NUMBER_ROUTINE[0], regs, mode, skip=(SPRINTF,))
        tabled, written, shape, value, site = stop.kind == "bctr", set(stop.written), "none", None, None
        if tabled:
            arm = self.arm(stop.target)
            if arm[0] == "number":
                value = arm[1]
                shape = "number"
            else:
                again = self.emu.run(stop.target, stop.regs, mode, skip=(SPRINTF,))
                if again.kind != "exit":
                    raise Unrecognised(f"after the report at {stop.target:x}: {again.kind}")
                stop, written = again, written | again.written
        if shape != "number":
            if stop.kind != "exit" or stop.regs[3] != 0 or len(stop.calls) > 1:
                raise Unrecognised(f"code {code} kind {kind} mode {mode}: {stop.kind} r3={stop.regs[3]:x}")
            if stop.calls:
                shape, site = "error", stop.calls[0][0]
                if stop.calls[0][1] != SPRINTF or not stop.after_call <= {0, 1, 3}:
                    raise Unrecognised(f"after the report: {sorted(stop.after_call)}")
        # r5 (the code, sign-extended) is written from the range checks on, in the handled modes only
        if (5 in written) != (mode in MODES):
            raise Unrecognised(f"r5 written: {5 in written} for mode {mode}")
        return tabled, shape, value, site

    def classes(self):
        """{(mode or 'other', kind is 3): a (mode, kind) of the class}, found by probing every byte pair."""
        probe = (0, 1, 0x2C, 0x2D, 0x31, 0x32, 0x63, 0x64, 0x9F, 0xA0, 0x3E7, 0x3E8, 0x3FB, 0x3FC, 0x40B, 0x40C,
                 0x417, 0x418, 0x41E, 0x41F, 0x483, 0x484, 0x485, -1, -32768, 32767)
        groups = collections.defaultdict(list)
        for mode in range(256):
            for kind in range(256):
                groups[tuple(self.outcome(code, kind, mode) for code in probe)].append((mode, kind))
        found = {}
        for members in groups.values():
            modes, kinds = {m for m, _ in members}, {k for _, k in members}
            found[(min(modes) if len(modes) < 5 else "other", kinds == {3})] = members[0]
        expected = {(m, k) for m in MODES for k in (False, True)} | {("other", False)}
        if set(found) != expected:
            raise SystemExit(f"unexpected mode/kind classes: {sorted(map(str, found))}")
        return found

    def variants(self):
        """[{mode, kind3, spans: [(first, last, numbers or None, error_site)]}] for the handled modes."""
        self.classify_arms()
        result = []
        for (mode, kind3), (m, k) in sorted(self.classes().items(), key=str):
            if mode == "other":
                continue
            per_code = [(code,) + self.outcome(code, k, m) for code in CODES]
            spans, start = [], 0
            while start < len(per_code):
                tabled = per_code[start][1]
                end = start
                while end + 1 < len(per_code) and per_code[end + 1][1] == tabled and (
                        tabled or per_code[end + 1][2:] == per_code[start][2:]):
                    end += 1
                part = per_code[start:end + 1]
                if tabled:
                    numbers = "".join(str(v) if shape == "number" else "-" for _, _, shape, v, _ in part)
                    sites = {site for _, _, shape, _, site in part if shape == "error"}
                    if len(sites) > 1:
                        raise SystemExit(f"a table with several error arms: mode {mode} kind3 {kind3} from {part[0][0]}")
                    spans.append((part[0][0], part[-1][0], numbers, sites.pop() if sites else 0))
                else:
                    spans.append((part[0][0], part[-1][0], None, part[0][4] or 0))
                start = end + 1
            result.append({"mode": mode, "kind3": kind3, "spans": spans})
        return result

    def report(self):
        variants = self.variants()
        entries = sum(len(table_cases(self.asm, self.export.word, pc)) for pc, t in self.asm.items() if t == "bctr")
        reached = sum(last - first + 1 for v in variants for first, last, numbers, _ in v["spans"] if numbers)
        return [f"0x1046f1f0: {entries} jump-table cases, by arm shape: " +
                ", ".join(f"{n} {shape}" for shape, n in sorted(self.shapes.items())),
                f"  {len(variants)} (mode, kind) classes with tables; {reached} table entries reached by emulating "
                f"every code ({entries} in the tables)"], variants


# ---------------------------------------------------------------------------------------------------------------
# 0x10469bb0: round code -> name
# ---------------------------------------------------------------------------------------------------------------
class Names:
    def __init__(self, export):
        self.export = export
        self.asm = export.assembly(*TEXT_ROUTINE)
        self.emu = Emulator(export, self.asm)
        self.shapes = collections.Counter()
        self.arms = {}

    def arm(self, address):
        """An arm: a localized-format call (or sprintf for the empty text) on a string of the pool, or the
        error text. (kind, site, key offset, second argument is the empty text)."""
        if address in self.arms:
            return self.arms[address]
        a = self.asm
        pos, instructions = address, []
        while True:
            instructions.append(a[pos])
            if a[pos].startswith("b "):
                break
            pos += 4
        buffer, key, second, callee, bl_at, high = "r31", None, False, None, None, 0
        for k, text in enumerate(instructions):
            op, x = operands(text)
            if text == "or r3,r31,r31":
                continue
            if text == "or r3,r29,r29":
                buffer = "r29"
            elif op == "addi" and x[0] == "r4" and x[1] == "r30":
                key = int(x[2], 0)
            elif op == "addis" and x[0] == "r4" and x[1] == "r30":
                high = int(x[2], 0) << 16
            elif op == "subi" and x[0] == "r4" and x[1] == "r4":
                key = high - int(x[2], 0)
            elif op == "addi" and x[0] == "r5" and x[1] == "r30" and int(x[2], 0) == EMPTY_TEXT:
                second = True
            elif op == "bl":
                callee, bl_at = int(x[0], 0), address + 4 * k
            elif op == "b":
                exit_ = int(x[0], 0)
            else:
                raise Unrecognised(f"{address:x}: {text}")
        if buffer == "r29" and key == ERROR_TEXT and callee == SPRINTF and exit_ == TEXT_ERROR_EXIT and not second:
            result = ("error", bl_at + 4, 0, False)
        elif buffer == "r31" and exit_ == TEXT_EXIT and key is not None and 0 < key < 0x10000 and (
                callee == FORMAT or (callee == SPRINTF and key == EMPTY_TEXT and not second)):
            result = ("sprintf" if callee == SPRINTF else "format", bl_at + 4, key, second)
        else:
            raise Unrecognised(f"{address:x}: {instructions}")
        self.arms[address] = result
        return result

    def classify_arms(self):
        bad = []
        for pc, text in self.asm.items():
            if text == "bctr":
                for case, target in enumerate(table_cases(self.asm, self.export.word, pc)):
                    try:
                        shape = self.arm(target)
                        self.shapes[shape[0] + (" +empty" if shape[3] else "")] += 1
                    except Unrecognised as why:
                        bad.append(f"bctr {pc:x} case {case}: {why}")
        if bad:
            raise SystemExit("arms of no recognised shape:\n  " + "\n  ".join(bad))
        if self.export.byte(self.export.word(TOC + POOL_SLOT) + EMPTY_TEXT) != 0:
            raise SystemExit("the pool's +0x5380 is not an empty string")

    def run(self, code, kind, size=100):
        return self.emu.run(TEXT_ROUTINE[0], {3: 0x1000, 4: size & 0xFFFFFFFF, 5: code & 0xFFFFFFFF, 6: kind & 0xFFFFFFFF},
                            skip=(STRCPY,))

    def outcome(self, code, kind):
        """('arm', address) for a table case, ('gap', site) for a range check's error block, ('assert', pc)."""
        stop = self.run(code, kind)
        if stop.kind == "bctr":
            return "arm", stop.target, stop
        if stop.kind == "bl" and stop.target == SPRINTF:
            shape = self.arm(stop.pc - 8)   # the block's own `or r3,r29,r29; addi r4,r30,0x5565`
            return "gap", shape[1], stop
        if stop.kind == "bl" and stop.target == 0x10739890:
            return "assert", stop.pc, stop
        raise Unrecognised(f"{stop.kind} at {stop.pc:x}")

    def classes(self):
        probe = (0, 1, 0x2C, 0x2D, 0x31, 0x32, 0x63, 0x64, 0x9F, 0xA0, 0x3E7, 0x3E8, 0x3FB, 0x3FC, 0x40B, 0x40C,
                 0x417, 0x418, 0x41E, 0x41F, 0x483, 0x484, 0x485, -1, -32768, 32767)
        groups = collections.defaultdict(list)
        for kind in range(256):
            signature = tuple(self.outcome(code, kind)[:2] for code in probe)
            groups[signature].append(kind)
        tabled = sorted(members[0] for members in groups.values() if len(members) == 1)
        # kinds 0, 1 and 3 alone; all the others (253 values) share the assertion
        if tabled != list(TEXT_KINDS) or sorted(len(m) for m in groups.values()) != [1, 1, 1, 253]:
            raise SystemExit(f"unexpected kind classes: {[m[:4] for m in groups.values()]}")
        return {k: self.arms_by_code(k) for k in TEXT_KINDS}

    def arms_by_code(self, kind):
        per_code = []
        for code in CODES:
            what, where, stop = self.outcome(code, kind)
            assert what != "assert", (kind, code)
            assert 5 in stop.written and stop.regs[5] == sext(code, 16) & 0xFFFFFFFF
            per_code.append((code, what, where))
        return per_code

    def variants(self):
        """{kind: [(first, last, arm addresses|None, error_site)]} and the arms, in address order."""
        by_kind = {}
        self.classify_arms()
        for kind, per_code in self.classes().items():
            spans, start = [], 0
            while start < len(per_code):
                tabled = per_code[start][1] == "arm"
                end = start
                while end + 1 < len(per_code) and (per_code[end + 1][1] == "arm") == tabled and (
                        tabled or per_code[end + 1][2] == per_code[start][2]):
                    end += 1
                part = per_code[start:end + 1]
                spans.append((part[0][0], part[-1][0], [w for _, _, w in part] if tabled else None,
                              0 if tabled else part[0][2]))
                start = end + 1
            by_kind[kind] = spans
        return by_kind

    def report(self):
        by_kind = self.variants()
        used = {a for spans in by_kind.values() for _, _, arms, _ in spans if arms for a in arms}
        entries = sum(len(table_cases(self.asm, self.export.word, pc)) for pc, t in self.asm.items() if t == "bctr")
        reached = sum(last - first + 1 for spans in by_kind.values() for first, last, arms, _ in spans if arms)
        out = [f"0x10469bb0: {entries} jump-table cases into {len(self.arms)} arms, by shape: " +
               ", ".join(f"{n} {shape}" for shape, n in sorted(self.shapes.items())),
               f"  kinds {TEXT_KINDS}: {reached} table entries reached by emulating every code; {len(used)} arms used"]
        return out, by_kind


# ---------------------------------------------------------------------------------------------------------------
# the data file
# ---------------------------------------------------------------------------------------------------------------
def chunks(text, size):
    return [text[i:i + size] for i in range(0, len(text), size)]


def emit(numbers_variants, text, names):
    out = []
    w = out.append
    w("// GENERATED by tools/comp_util_tables.py from the original's jump tables (instructions.sqlite): do not edit.")
    w("// `tools/comp_util_tables.py emit` rewrites it; `check` fails when it is stale. Data of comp_util_rounds.cpp:")
    w("// numbers and offsets into the unit's string pool only, no game text.")
    w("")
    w("// ---- 0x1046f1f0: round code -> number. Spans cover every code from -32768 to 32767 under one (mode, kind)")
    w("// class: a table (a digit per code, the arm's `li r3,n`; '-' the table's error arm) or a gap (no table).")
    w("// `error_site` is the address after the original's `bl` that reports the error text for the span's gap or")
    w("// the table's error arm; 0 where the original returns 0 without a report.")
    names_of = {}
    for v in numbers_variants:
        name = f"numbers_mode{v['mode']}_{'kind3' if v['kind3'] else 'plain'}"
        names_of[(v["mode"], v["kind3"])] = name
        w(f"inline constexpr NumberSpan {name}[] = {{")
        for first, last, table, site in v["spans"]:
            if table is None:
                w(f"    {{{first}, {last}, {{}}, 0x{site:x}}},")
            else:
                w(f"    {{{first}, {last},")
                for i, piece in enumerate(chunks(table, 50)):
                    parts = " ".join(f'"{p}"' for p in chunks(piece, 10))
                    w(f"        {parts}{',' if i == len(chunks(table, 50)) - 1 else ''}  // codes {first + 50 * i}...")
                w(f"        0x{site:x}}},")
        w("};")
    w("inline constexpr NumberVariant number_variants[] = {")
    for (mode, kind3), name in sorted(names_of.items()):
        w(f"    {{{mode}, {'true' if kind3 else 'false'}, {name}}},")
    w("};")
    w("")
    w("// ---- 0x10469bb0: round code -> the round's name. `text_arms` are the original's arms in address order: the")
    w("// address after the arm's call, the string's offset in the pool (0 for the error text) and the call it makes.")
    w("// A kind's spans give, for a stretch of codes, the arm of each code (indexes into text_arms) or, for a gap,")
    w("// the address after the call that reports the error text.")
    used = sorted({a for spans in names.values() for _, _, arms, _ in spans if arms for a in arms})
    index = {a: i for i, a in enumerate(used)}
    w(f"inline constexpr TextArm text_arms[{len(used)}] = {{")
    row = []
    for a in used:
        kind, site, key, second = text.arms[a]
        call = "Call::format_with_empty" if second else {"format": "Call::format", "sprintf": "Call::copy", "error": "Call::error"}[kind]
        row.append(f"{{0x{site:x}, 0x{key:x}, {call}}}")
    for i in range(0, len(row), 3):
        w("    " + ", ".join(row[i:i + 3]) + ",")
    w("};")
    for kind, spans in sorted(names.items()):
        for n, (first, last, arms, site) in enumerate(spans):
            if arms:
                w(f"inline constexpr std::uint16_t kind{kind}_codes_{first}[] = {{")
                values = [str(index[a]) for a in arms]
                for i in range(0, len(values), 16):
                    w("    " + ", ".join(values[i:i + 16]) + ",")
                w("};")
        w(f"inline constexpr TextSpan text_kind{kind}[] = {{")
        for first, last, arms, site in spans:
            w(f"    {{{first}, {last}, {'kind%d_codes_%d' % (kind, first) if arms else '{}'}, 0x{site:x}}},")
        w("};")
    w("inline constexpr TextVariant text_variants[] = {")
    for kind in sorted(names):
        w(f"    {{{kind}, text_kind{kind}}},")
    w("};")
    return "\n".join(out) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("command", choices=("report", "emit", "check"))
    parser.add_argument("--export", type=Path, default=ROOT / ".local/current/export")
    args = parser.parse_args()
    export = Export(args.export)
    numbers, names = Numbers(export), Names(export)
    lines, numbers_variants = numbers.report()
    more, names_variants = names.report()
    print("\n".join(lines + more))
    if args.command == "report":
        return
    text = emit(numbers_variants, names, names_variants)
    if args.command == "check":
        if not OUTPUT.exists() or OUTPUT.read_text() != text:
            raise SystemExit(f"{OUTPUT.relative_to(ROOT)} is stale: run tools/comp_util_tables.py emit")
        print("the tables are current")
    else:
        OUTPUT.write_text(text)
        print(f"wrote {OUTPUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
