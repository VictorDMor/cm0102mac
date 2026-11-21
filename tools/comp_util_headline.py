#!/usr/bin/env python3
"""Draft the data of COMP_UTIL.CPP's result headline (0x10464d00) from the instruction database.

The routine builds the text of a fixture's result line for a news item. After the common set-up it picks one of
about 170 localized sentences with a decision tree whose leaves are all of a few shapes: one localized-format call
(a pool string and arguments that are genders, names, scores and texts) and sometimes a second call (the score text
or a copy of the competition's name). This tool does not read that tree by hand: it walks the original's code from
0x10465114 (where the set-up has joined) with a small symbolic executor, forks at every conditional branch and
collects, for every path to the common tail (0x10469618):

  * the conditions of the path, which it maps to the named facts of comp_util_headline.cpp (it fails on one it has
    no name for),
  * the call(s) the path makes: the pool string's offset, the address after the `bl` (the callee saves it as its
    link register), the registers the path sets for the call and the words it stores on the stack for the formatter's
    varargs, as named operands (it fails on one it has no name for), grouped into the shapes the .inc lists,
  * and nothing else: a path that calls anything but the localized format (0x10727890), the string copy
    (0x1077dbf0) and, for the sentence about a round, 0x10469bb0 and 0x1046f1f0 fails.

It then checks over every combination of facts a game state can make that exactly one leaf matches (so the table is
a function of the facts, with no first-match order), and writes native/comp_util_headline_tables.inc.

  tools/comp_util_headline.py report   leaves, shapes, facts and counts (nothing written)
  tools/comp_util_headline.py emit     (re)write the .inc
  tools/comp_util_headline.py check    exit 1 when the .inc differs from what the database gives

Only offsets into the unit's string pool, addresses and operand names are written, no game text."""
import argparse
import collections
import itertools
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from comp_util_tables import Export  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "native/comp_util_headline_tables.inc"

ENTRY, END = 0x10464D00, 0x10469710
START, TAIL = 0x10465114, 0x10469618
FORMAT, COPY, ROUND_TEXT, ROUND_GENDER = 0x10727890, 0x1077DBF0, 0x10469BB0, 0x1046F1F0

# The registers at START, by what they hold (r13..r31 and the sp): the operands are named from these.
HELD = {13: "toc6b94", 14: "records_cell", 15: "row", 16: "k3", 17: "res", 18: "found", 19: "theirs", 20: "mine",
        21: "cgender", 22: "cls", 23: "b4", 24: "other", 25: "me", 26: "smine", 27: "sother", 28: "code30",
        29: "buf", 30: "idcell", 31: "pool", 1: "sp", 2: "toc"}

# The facts a leaf's conditions are made of, in the order of the bits (comp_util_headline.cpp declares the same
# names). A condition maps to (fact, polarity).
FACTS = ["result_positive", "result_minus_two",
         "decider_aggregate", "decider_penalties", "decider_shootout", "decider_away_goals", "decider_golden_goal",
         "decider_seeding",
         "grade_excellent", "grade_fine", "grade_disappointing", "grade_shock", "grade_very_disappointing",
         "club_through", "has_first_leg", "no_deciding_score", "final_round", "either_below_zero",
         "mine_less", "mine_greater"]
# Two tests the original makes in a row, each of which the leaves see as one fact: the fixture's first round code is
# 150 with no second code (the final), and either of the sides' tie-break bytes is negative (a short-circuit `or`).
RAW = ("first_code_150", "no_second_code", "mine_below_zero", "theirs_below_zero")
DECIDERS = {3: "decider_aggregate", 4: "decider_penalties", 5: "decider_shootout", 6: "decider_away_goals",
            7: "decider_golden_goal", 8: "decider_seeding"}
GRADES = {2: "grade_excellent", 3: "grade_fine", 5: "grade_disappointing", 6: "grade_shock", 7: "grade_very_disappointing"}

OPERANDS = {
    "sb(u8[me+0x23b])": "my_gender", "sb(u8[other+0x23b])": "other_gender", "sb(u8[u32[row+0x18]+0x4b])": "first_leg_gender",
    "me+0x221": "my_name", "other+0x221": "other_name", "smine": "my_score", "sother": "other_score",
    "sp+0xd4": "first_leg_text",
    # what the sentence is about: the competition's name, a round's text or the record's own name
    "sb(cgender)": "subject_gender:competition", "sp+0x200": "subject_text:competition",
    "sb(u8[u32[row+0x14]+0x6a])": "subject_gender:record", "u32[row+0x14]+0x1a": "subject_text:record",
    "sp+0x70": "subject_text:round",
}
SUBJECTS = ("competition", "round", "record")

# The shapes by their operands (r5..r10, then the words from sp+0x38). Named after what they say.
SHAPES = {
    "score_and_text": (("my_gender", "my_gender", "subject_gender", "subject_gender", "other_gender", "other_gender"),
                       ("my_name", "subject_text", "my_score", "other_score", "other_name")),
    "swapped_score_and_text": (("my_gender", "my_gender", "subject_gender", "subject_gender", "other_gender", "other_gender"),
                               ("my_name", "subject_text", "other_score", "my_score", "other_name")),
    "text_only": (("my_gender", "my_gender", "subject_gender", "subject_gender", "other_gender", "other_gender"),
                  ("my_name", "subject_text", "other_name")),
    "first_leg_score": (("my_gender", "my_gender", "subject_gender", "subject_gender", "other_gender", "other_gender"),
                        ("first_leg_gender", "first_leg_gender", "my_name", "subject_text", "my_score", "other_score",
                         "other_name", "first_leg_text")),
    "first_leg_swapped": (("my_gender", "my_gender", "subject_gender", "subject_gender", "other_gender", "other_gender"),
                          ("first_leg_gender", "first_leg_gender", "my_name", "subject_text", "other_score", "my_score",
                           "other_name", "first_leg_text")),
    "first_leg_only": (("my_gender", "my_gender", "other_gender", "other_gender", "first_leg_gender", "first_leg_gender"),
                       ("my_name", "my_score", "other_score", "other_name", "first_leg_text")),
    "name_and_score": (("my_gender", "my_gender", "other_gender", "other_gender", "my_name", "my_score"),
                       ("other_score", "other_name")),
}


def s32(x):
    x &= 0xFFFFFFFF
    return x - (1 << 32) if x >> 31 else x


def add(e, imm):
    if imm == 0:
        return e
    if re.match(r"^-?\d+$", e):
        return str(int(e) + imm)
    m = re.match(r"^([a-z_0-9]+)\+(0x[0-9a-f]+)$", e)
    if m:
        return f"{m[1]}+{hex(int(m[2], 0) + imm)}"
    return f"{e}+{hex(imm)}" if imm >= 0 else f"{e}-{hex(-imm)}"


class Unrecognised(Exception):
    pass


def walk(asm, start):
    """Every path from `start` to TAIL: dicts with `conds` [(signed, lhs, op, rhs)] and `calls` [at, target, args,
    set registers, ordered stack stores]."""
    out = []
    todo = [(start, dict(HELD), {}, None, [], [], set(), [])]
    while todo:
        pc, regs, stk, cmp, conds, calls, dirty, stores = todo.pop()
        while pc != TAIL:
            text = asm[pc]
            op, _, rest = text.partition(" ")
            a = [x for x in re.split(r"[,()]", rest) if x]
            reg = lambda x: int(x[1:])  # noqa: E731

            def put(n, value):
                regs[n] = value
                dirty.add(n)
            following = pc + 4
            if op == "or":
                put(reg(a[0]), regs[reg(a[1])] if a[1] == a[2] else None)
            elif op == "li":
                put(reg(a[0]), str(int(a[1], 0)))
            elif op == "addi":
                put(reg(a[0]), add(regs[reg(a[1])], int(a[2], 0)) if reg(a[1]) else str(int(a[2], 0)))
            elif op in ("extsb", "extsb.", "extsh", "extsh."):
                f = "sb" if "b" in op else "sh"
                e = regs[reg(a[1])]
                put(reg(a[0]), e if e.startswith(f + "(") else f"{f}({e})")
                if op.endswith("."):
                    cmp = ("s", regs[reg(a[0])], "0")
            elif op in ("lbz", "lhz", "lha", "lwz"):
                base, off = reg(a[2]), int(a[1], 0)
                if base == 1:
                    put(reg(a[0]), stk.get(off, f"stack[{off:#x}]"))
                else:
                    kind = {"lbz": "u8", "lhz": "u16", "lha": "s16", "lwz": "u32"}[op]
                    put(reg(a[0]), f"{kind}[{add(regs[base], off)}]")
            elif op == "stw":
                off = int(a[1], 0)
                stk[off] = regs[reg(a[0])]
                stores = stores + [(off, regs[reg(a[0])])]
            elif op in ("cmpwi", "cmpw", "cmplwi"):
                cmp = ("s", regs[reg(a[0])], regs[reg(a[1])]) if op == "cmpw" else (
                    "u" if op == "cmplwi" else "s", regs[reg(a[0])], str(int(a[1], 0)))
            elif op in ("beq", "bne", "blt", "bge", "ble", "bgt"):
                inverse = {"beq": "!=", "bne": "==", "blt": ">=", "bge": "<", "ble": ">", "bgt": "<="}[op]
                taken = {"beq": "==", "bne": "!=", "blt": "<", "bge": ">=", "ble": "<=", "bgt": ">"}[op]
                _, x, y = cmp
                todo.append((int(a[0], 0), dict(regs), dict(stk), cmp, conds + [(x, taken, y)], list(calls), set(dirty),
                             list(stores)))
                conds = conds + [(x, inverse, y)]
            elif op == "b":
                following = int(a[0], 0)
            elif op == "bl":
                target = int(a[0], 0)
                calls = calls + [dict(at=pc, target=target, args={r: regs.get(r) for r in range(3, 11)},
                                      dirty=sorted(dirty), stores=stores)]
                dirty, stores = set(), []
                regs[3] = f"ret@{pc + 4:x}"
                for r in range(4, 13):
                    regs[r] = None
            else:
                raise Unrecognised(f"{pc:x}: {text}")
            pc = following
        out.append(dict(conds=conds, calls=calls))
    return out


def fact_of(cond):
    """(fact, polarity) of one condition (lhs, op, rhs)."""
    lhs, op, rhs = cond
    eq = {"==": True, "!=": False}
    if lhs == "sh(res)" and rhs == "0" and op in (">", "<="):
        return "result_positive", op == ">"
    if lhs == "sh(res)" and rhs == "-2" and op in eq:
        return "result_minus_two", eq[op]
    if lhs == "sb(cls)" and int(rhs) in DECIDERS and op in eq:
        return DECIDERS[int(rhs)], eq[op]
    if lhs == "sb(k3)" and int(rhs) in GRADES and op in eq:
        return GRADES[int(rhs)], eq[op]
    if lhs == "sb(b4)" and rhs == "0" and op in eq:
        return "club_through", not eq[op]
    if lhs == "u32[row+0x18]" and rhs == "0" and op in eq:
        return "has_first_leg", not eq[op]
    if lhs == "sb(u8[row+0x4b])" and rhs == "-1" and op in eq:
        return "no_deciding_score", eq[op]
    if lhs == "code30" and rhs == "150" and op in eq:
        return "first_code_150", eq[op]
    if lhs == "s16[row+0x32]" and rhs == "0" and op in eq:
        return "no_second_code", eq[op]
    if lhs == "sb(mine)" and rhs == "0" and op in ("<", ">="):
        return "mine_below_zero", op == "<"
    if lhs == "sb(theirs)" and rhs == "0" and op in ("<", ">="):
        return "theirs_below_zero", op == "<"
    if lhs == "sb(mine)" and rhs == "sb(theirs)" and op in ("<", ">="):
        return "mine_less", op == "<"
    if lhs == "sb(mine)" and rhs == "sb(theirs)" and op in (">", "<="):
        return "mine_greater", op == ">"
    raise Unrecognised(f"condition {lhs} {op} {rhs}")


def valuations():
    """Every combination of facts a game state can make, as bitmasks."""
    index = {name: 1 << n for n, name in enumerate(FACTS)}
    results = [0, index["result_positive"], index["result_minus_two"]]
    deciders = [0] + [index[n] for n in DECIDERS.values()]
    grades = [0] + [index[n] for n in GRADES.values()]
    singles = [index[n] for n in ("club_through", "has_first_leg", "no_deciding_score", "final_round", "either_below_zero")]
    order = [0, index["mine_less"], index["mine_greater"]]
    negatives = []
    for r, d, g, o in itertools.product(results, deciders, grades, order):
        for s in range(1 << len(singles)):
            base = r | d | g | o | sum(b for i, b in enumerate(singles) if s >> i & 1)
            yield base


def operand(expr, round_gender_at):
    if expr == f"sb(ret@{round_gender_at:x})":
        return "subject_gender:round"
    if expr in OPERANDS:
        return OPERANDS[expr]
    raise Unrecognised(f"operand {expr}")


class Headline:
    def __init__(self, export):
        self.asm = export.assembly(ENTRY, END)
        paths = walk(self.asm, START)
        self.paths = paths
        self.round_gender_at = next(c["at"] + 4 for p in paths for c in p["calls"] if c["target"] == ROUND_GENDER)
        index = {name: 1 << n for n, name in enumerate(FACTS)}
        self.index = index
        groups = collections.OrderedDict()
        for p in paths:
            mask = {"when": 0, "unless": 0}
            raw = {}
            for cond in p["conds"]:
                fact, polarity = fact_of(cond)
                if fact in RAW:
                    raw[fact] = polarity
                else:
                    mask["when" if polarity else "unless"] |= index[fact]
            for name, polarity in self.derived(raw):
                mask["when" if polarity else "unless"] |= index[name]
            if mask["when"] & mask["unless"]:
                continue  # a path no state takes
            leaf = self.leaf(p)
            groups.setdefault(leaf["site"], (leaf, []))[1].append((mask["when"], mask["unless"]))
        self.leaves = []
        space = list(valuations())
        for site, (leaf, masks) in sorted(groups.items()):
            matched = {v for v in space if any(v & w == w and v & u == 0 for w, u in masks)}
            if not matched:
                continue
            # one row when the paths of a leaf form one cube of the facts, else one row per path
            when = (1 << len(FACTS)) - 1
            unless = when
            for w, u in masks:
                when, unless = when & w, unless & u
            cube = {v for v in space if v & when == when and v & unless == 0}
            rows = [(when, unless)] if cube == matched else sorted(set(masks))
            for w, u in rows:
                self.leaves.append(dict(leaf, when=w, unless=u))
        self.check_function(space)

    @staticmethod
    def derived(raw):
        """The facts the raw tests of a path make: the final (code 150 and no second code), either side negative."""
        out = []
        if "first_code_150" in raw:
            out.append(("final_round", raw["first_code_150"] and raw.get("no_second_code", False)))
        if "mine_below_zero" in raw:
            out.append(("either_below_zero", raw["mine_below_zero"] or raw.get("theirs_below_zero", False)))
        return out

    def leaf(self, path):
        calls = path["calls"]
        prefix = [c for c in calls if c["target"] in (ROUND_TEXT, ROUND_GENDER)]
        main = [c for c in calls if c["target"] not in (ROUND_TEXT, ROUND_GENDER)]
        if [c["target"] for c in prefix] not in ([], [ROUND_TEXT, ROUND_GENDER]) or any(
                c["target"] not in (FORMAT, COPY) for c in main) or not 1 <= len(main) <= 2 or main[0]["target"] != FORMAT:
            raise Unrecognised(f"calls of a path: {[hex(c['target']) for c in calls]}")
        first = main[0]
        key = re.fullmatch(r"pool\+(0x[0-9a-f]+)", first["args"][4] or "")
        if not key or first["args"][3] != "buf":
            raise Unrecognised(f"format call {first['at']:x}: {first['args']}")
        regs = tuple(operand(first["args"][r], self.round_gender_at) for r in range(5, 11))
        stack = tuple(operand(e, self.round_gender_at) for off, e in first["stores"])
        if [off for off, _ in first["stores"]] != [0x38 + 4 * i for i in range(len(stack))]:
            raise Unrecognised(f"stack words of {first['at']:x}: {first['stores']}")
        # the gender and the text of what the sentence is about come from the competition, the round or the record
        # (the same one except in a few sentences)
        genders = {x.split(":")[1] for x in regs + stack if x.startswith("subject_gender")}
        texts = {x.split(":")[1] for x in regs + stack if x.startswith("subject_text")}
        if len(genders) > 1 or len(texts) > 1:
            raise Unrecognised(f"two subjects at {first['at']:x}: {genders} {texts}")
        gender_of = genders.pop() if genders else "none"
        text_of = texts.pop() if texts else "none"
        subject = (gender_of, text_of)
        plain = lambda ops: tuple(x.split(":")[0] for x in ops)  # noqa: E731
        shape = next((name for name, (r, s) in SHAPES.items() if (r, s) == (plain(regs), plain(stack))), None)
        if shape is None:
            raise Unrecognised(f"shape of {first['at']:x}: {plain(regs)} {plain(stack)}")
        if "round" in subject and not prefix:
            raise Unrecognised(f"a round subject without a round text at {first['at']:x}")
        # the dirty registers of the call: every one of r3, r5..r10 (r4 is the string)
        if sorted(r for r in first["dirty"] if 3 <= r <= 10) != [3, 4, 5, 6, 7, 8, 9, 10]:
            raise Unrecognised(f"registers set for {first['at']:x}: {first['dirty']}")
        trailer, trailer_site = "none", 0
        if len(main) == 2:
            t = main[1]
            if t["target"] == FORMAT and t["args"][3] == "sp+0x60" and t["args"][4] == "pool+0x21" \
                    and (t["args"][5], t["args"][6]) == ("sother", "smine") and not t["stores"] and t["dirty"] == [3, 4, 5, 6]:
                trailer = "score_text"
            elif t["target"] == COPY and t["args"][3] == "sp+0x200" and t["args"][4] == "u32[row+0x14]+0x1a" \
                    and not t["stores"] and t["dirty"] == [3, 4]:
                trailer = "competition_name"
            else:
                raise Unrecognised(f"second call {t['at']:x}: {t['args']} {t['dirty']} {t['stores']}")
            trailer_site = t["at"] + 4
        return dict(site=first["at"] + 4, key=int(key[1], 0), shape=shape, gender_of=gender_of, text_of=text_of, trailer=trailer,
                    trailer_site=trailer_site, rounds=bool(prefix))

    def check_function(self, space):
        """Every state matches exactly one leaf."""
        for v in space:
            n = sum(1 for leaf in self.leaves if v & leaf["when"] == leaf["when"] and v & leaf["unless"] == 0)
            if n != 1:
                names = [f for i, f in enumerate(FACTS) if v >> i & 1]
                raise SystemExit(f"{n} leaves match the facts {names}")
        # a result above 0 asks for a round text exactly in the leaves that name a round
        for leaf in self.leaves:
            if leaf["rounds"] != bool(leaf["when"] & self.index["result_positive"]):
                raise SystemExit(f"leaf {leaf['site']:x}: round text and result sign disagree")

    def report(self):
        sites = {leaf["site"] for leaf in self.leaves} | {l["trailer_site"] for l in self.leaves if l["trailer_site"]}
        shapes = collections.Counter(l["shape"] for l in self.leaves)
        return [f"0x10464d00 from 0x{START:x}: {len(self.paths)} paths, {len(self.leaves)} leaf rows over "
                f"{len({l['site'] for l in self.leaves})} sentences; {len(sites)} call sites (formats and copies) after the set-up",
                "  shapes: " + ", ".join(f"{n} {s}" for s, n in sorted(shapes.items())),
                "  trailers: " + ", ".join(f"{n} {t}" for t, n in sorted(collections.Counter(
                    l["trailer"] for l in self.leaves).items()))]


def emit(headline):
    out = []
    w = out.append
    w("// GENERATED by tools/comp_util_headline.py from the original's code (instructions.sqlite): do not edit.")
    w("// `tools/comp_util_headline.py emit` rewrites it; `check` fails when it is stale. Data of comp_util_headline.cpp:")
    w("// offsets into the unit's string pool, call-site addresses and operand names only, no game text.")
    w("")
    w("// The operands of each shape: r5..r10, then the words stored from sp+0x38 for the formatter's varargs.")
    w("inline constexpr ShapeOperands shapes[] = {")
    for name, (regs, stack) in SHAPES.items():
        r = ", ".join(f"Operand::{x}" for x in regs)
        s = ", ".join(f"Operand::{x}" for x in stack)
        w(f"    /* {name} */ {{{{{r}}},")
        w(f"        {{{s}}}, {len(stack)}}},")
    w("};")
    w("")
    w("// One row per sentence the original can choose (a sentence reached on several unrelated paths has a row per")
    w("// path). `when` facts must hold and `unless` facts must not; exactly one row matches any state.")
    w("// site: the address after the sentence's `bl`; trailer_site: the address after the second call, if any.")
    w("inline constexpr Leaf leaves[] = {")
    names = [f"Shape::{n}" for n in SHAPES]
    for leaf in sorted(headline.leaves, key=lambda l: (l["site"], l["when"], l["unless"])):
        when = " | ".join(f"Fact::{f}" for i, f in enumerate(FACTS) if leaf["when"] >> i & 1) or "0"
        unless = " | ".join(f"Fact::{f}" for i, f in enumerate(FACTS) if leaf["unless"] >> i & 1) or "0"
        shape = list(SHAPES).index(leaf["shape"])
        w(f"    {{{when},")
        w(f"     {unless},")
        w(f"     0x{leaf['key']:x}, 0x{leaf['site']:x}, {names[shape]}, Source::{leaf['gender_of']}, Source::{leaf['text_of']}, "
          f"Trailer::{leaf['trailer']}, 0x{leaf['trailer_site']:x}}},")
    w("};")
    return "\n".join(out) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("command", choices=("report", "emit", "check"))
    parser.add_argument("--export", type=Path, default=ROOT / ".local/current/export")
    args = parser.parse_args()
    headline = Headline(Export(args.export))
    print("\n".join(headline.report()))
    if args.command == "report":
        return
    text = emit(headline)
    if args.command == "check":
        if not OUTPUT.exists() or OUTPUT.read_text() != text:
            raise SystemExit(f"{OUTPUT.relative_to(ROOT)} is stale: run tools/comp_util_headline.py emit")
        print("the headline table is current")
    else:
        OUTPUT.write_text(text)
        print(f"wrote {OUTPUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
