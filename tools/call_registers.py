#!/usr/bin/env python3
"""Show what a routine holds in registers at each of its calls.

Usage: call_registers.py ENTRY... [--export DIR]

A routine rewritten in C++ must call its callees with the registers the
original holds there: the callee's prologue saves the non-volatile ones into
the stack image, and a few callees (varargs, spills) store argument registers
too (see native/guest_abi.hpp). For each `bl` this prints the return address
and, for the argument registers r3-r10 and every non-volatile register the
routine changes (r13-r31, f14-f31), the instructions whose value can reach
the call ("entry" when the caller's value can).

The routine's body is its control flow from ENTRY without following calls;
other routines' entries (direct call targets) end it.
"""
import argparse
import json
import re
import sqlite3
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GPR = re.compile(r"\br(\d+)\b")
FPR = re.compile(r"\bf(\d+)\b")


def load(export, entry):
    db = sqlite3.connect(export / "instructions.sqlite")
    starts = {int(row[0]) for row in db.execute(
        "SELECT json_extract(body,'$.flows[0]') FROM instructions WHERE json_extract(body,'$.flow')='UNCONDITIONAL_CALL'")
        if row[0] is not None}
    body, stack = {}, [entry]
    while stack:
        address = stack.pop()
        if address in body or (address != entry and address in starts):
            continue
        row = db.execute("SELECT body FROM instructions WHERE address=?", (address,)).fetchone()
        if not row:
            continue
        item = json.loads(row[0])
        body[address] = item
        flow, flows = item["flow"], item.get("flows") or []
        if flow in ("FALL_THROUGH", "UNCONDITIONAL_CALL", "CONDITIONAL_TERMINATOR"):
            stack.append(address + 4)
        if flow in ("CONDITIONAL_JUMP", "COMPUTED_JUMP"):
            stack.extend(flows)
            if flow == "CONDITIONAL_JUMP":
                stack.append(address + 4)
        if flow == "UNCONDITIONAL_JUMP":
            stack.extend(f for f in flows if f == entry or f not in starts)
    return body


def defined(assembly):
    """Registers an instruction writes: ('r', n) and ('f', n)."""
    mnemonic, _, operands = assembly.partition(" ")
    ops = operands.split(",")
    base = mnemonic.rstrip(".")
    if base == "lmw":
        first = int(GPR.search(ops[0]).group(1))
        return {("r", n) for n in range(first, 32)}
    result = set()
    if base.startswith(("st", "cmp", "fcmp", "b", "mt", "tw", "dcb", "icbi", "sync", "isync", "eieio")):
        # Stores with update write their base register.
        if base.startswith("st") and base.endswith(("u", "ux")) and "(" in operands:
            match = GPR.search(operands[operands.index("("):])
            if match: result.add(("r", int(match.group(1))))
        elif base.startswith("st") and base.endswith("ux"):
            result.add(("r", int(GPR.search(ops[1]).group(1))))
        return result
    if not ops or not ops[0]:
        return result
    if base.startswith("l") and base.endswith(("u", "ux")):
        tail = operands[operands.index("("):] if "(" in operands else ops[1]
        match = GPR.search(tail)
        if match: result.add(("r", int(match.group(1))))
    first = ops[0].strip()
    if first.startswith("r") and GPR.fullmatch(first):
        result.add(("r", int(first[1:])))
    elif first.startswith("f") and FPR.fullmatch(first):
        result.add(("f", int(first[1:])))
    return result


def analyse(body, entry):
    """Reaching definitions per instruction (before it executes)."""
    tracked = [("r", n) for n in range(32)] + [("f", n) for n in range(32)]
    order = sorted(body)
    successors = {}
    for address in order:
        item = body[address]
        flow, flows = item["flow"], item.get("flows") or []
        nexts = []
        if flow in ("FALL_THROUGH", "UNCONDITIONAL_CALL", "CONDITIONAL_TERMINATOR", "CONDITIONAL_JUMP"):
            nexts.append(address + 4)
        if flow in ("CONDITIONAL_JUMP", "UNCONDITIONAL_JUMP", "COMPUTED_JUMP"):
            nexts.extend(flows)
        successors[address] = [n for n in nexts if n in body]
    state_in = {address: None for address in order}
    state_in[entry] = {reg: frozenset({"entry"}) for reg in tracked}
    work = [entry]
    while work:
        address = work.pop()
        current = dict(state_in[address])
        item = body[address]
        writes = defined(item["assembly"])
        if item["flow"] == "UNCONDITIONAL_CALL":
            # The callee may change volatile registers.
            writes |= {("r", 0)} | {("r", n) for n in range(3, 13)} | {("f", n) for n in range(0, 14)}
        for reg in writes:
            current[reg] = frozenset({address})
        for successor in successors[address]:
            old = state_in[successor]
            merged = current if old is None else {reg: old[reg] | current[reg] for reg in tracked}
            if merged != old:
                state_in[successor] = merged
                work.append(successor)
    return state_in


def describe(body, definitions):
    parts = []
    for d in sorted(definitions, key=lambda x: (x != "entry", x)):
        if d == "entry":
            parts.append("entry")
        elif body[d]["flow"] == "UNCONDITIONAL_CALL":
            parts.append(f"{d:08x} (call result)")
        else:
            parts.append(f"{d:08x} {body[d]['assembly']}")
    return "; ".join(parts)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("entries", nargs="+")
    parser.add_argument("--export", type=Path, default=ROOT / ".local/current/export")
    args = parser.parse_args()
    for text in args.entries:
        entry = int(text, 16)
        body = load(args.export, entry)
        state = analyse(body, entry)
        changed = sorted({reg for item in body.values() for reg in defined(item["assembly"])
                          if (reg[0] == "r" and reg[1] >= 13) or (reg[0] == "f" and reg[1] >= 14)})
        print(f"== {entry:08x}: {len(body)} instructions; changes "
              + (" ".join(f"{k}{n}" for k, n in changed) or "no non-volatile register"))
        for address in sorted(body):
            item = body[address]
            if item["flow"] != "UNCONDITIONAL_CALL" or state[address] is None:
                continue
            target = (item.get("flows") or [None])[0]
            print(f"{address:08x} bl {target:08x}  return {address + 4:08x}")
            for reg in [("r", n) for n in range(3, 11)] + changed:
                defs = state[address][reg]
                if reg[0] == "r" and reg[1] < 13 and defs == frozenset({"entry"}):
                    continue
                print(f"    {reg[0]}{reg[1]:<3} {describe(body, defs)}")


if __name__ == "__main__":
    main()
