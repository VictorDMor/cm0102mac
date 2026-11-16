#!/usr/bin/env python3
"""Which of r3-r10 each routine reads before writing them, from the export's p-code.

  call_arguments.py START END [--export DIR]   one "target mask" line for every routine the
                                               code in START..END calls; bit 0 is r3

A checker compares those registers at each call. A callee's own calls count as reads of
what they read and it hasn't written. An indirect call or an import counts as reading all
eight: their arity isn't known here, so a difference there still needs a look.
"""
import argparse
import json
from pathlib import Path
import re
import sqlite3
import sys

ROOT = Path(__file__).resolve().parent.parent
ARGUMENTS = {(3 + n) * 4: n for n in range(8)}  # register offset in the export's p-code -> bit
VOLATILE = {n * 4 for n in (0, *range(3, 13))}
ALL = 0xff
INDIRECT = 0x10781320


class Program:
    def __init__(self, db):
        self.db = db
        self.memo = {}
        self.clobber_memo = {}
        self.cache = {}

    def record(self, address):
        if address not in self.cache:
            row = self.db.execute('SELECT body FROM instructions WHERE address=?', (address,)).fetchone()
            self.cache[address] = json.loads(row[0]) if row else None
        return self.cache[address]

    @staticmethod
    def accesses(record):
        """(read, written) register offsets of one instruction's p-code, in order of effect."""
        reads, writes = set(), set()
        for op in record.get('ops', []):
            output, inputs = op[1], op[2:]
            for item in inputs:
                if isinstance(item, list) and item and item[0] == 'register':
                    offset = int(item[1]) & ~3  # `extsb r0,r4` reads r4's low byte
                    if offset not in writes:
                        reads.add(offset)
            if isinstance(output, list) and output and output[0] == 'register' and output[2] == 4:
                writes.add(int(output[1]))
        return reads, writes

    def clobbers(self, entry):
        """The registers a call of `entry` may change: VOLATILE, unless it is a leaf (no calls, no
        indirect jumps), which changes only what it writes. The prologue's FPR-save millicode
        (bl 0x107737f0 ...) is such a leaf and leaves the arguments alone."""
        if entry in self.clobber_memo:
            return self.clobber_memo[entry]
        result, seen, pending = set(), set(), [entry]
        while pending:
            at = pending.pop()
            if at in seen:
                continue
            seen.add(at)
            record = self.record(at)
            if record is None or 'CALL' in record['flow'] or record['flow'] in ('COMPUTED_JUMP', 'COMPUTED_CALL'):
                result = set(VOLATILE)
                break
            result |= self.accesses(record)[1] & VOLATILE
            if record['flow'] != 'TERMINATOR':
                pending.extend(record.get('flows') or [])
            if record.get('fallthrough') is not None:
                pending.append(record['fallthrough'])
        self.clobber_memo[entry] = frozenset(result)
        return self.clobber_memo[entry]

    def mask(self, entry):
        if entry in self.memo:
            return self.memo[entry]
        if entry == INDIRECT or self.record(entry) is None:
            self.memo[entry] = ALL
            return ALL
        self.memo[entry] = 0  # a cycle adds nothing the first pass doesn't find
        written_at = {entry: frozenset()}
        pending = [entry]
        found = 0
        while pending:
            at = pending.pop()
            record = self.record(at)
            if record is None:
                continue
            written = written_at[at]
            reads, writes = self.accesses(record)
            for offset in reads - written:
                if offset in ARGUMENTS:
                    found |= 1 << ARGUMENTS[offset]
            after = set(written) | writes
            successors = []
            if record['flow'] == 'UNCONDITIONAL_CALL':
                for target in record['flows']:
                    nested = self.mask(target)
                    for offset, bit in ARGUMENTS.items():
                        if nested >> bit & 1 and offset not in after:
                            found |= 1 << bit
                for target in record['flows']:
                    after |= self.clobbers(target)
                if not record['flows']:
                    after |= VOLATILE
                if record.get('fallthrough') is not None:
                    successors.append(record['fallthrough'])
            else:
                successors.extend(record.get('flows') or [])
                if record.get('fallthrough') is not None:
                    successors.append(record['fallthrough'])
            state = frozenset(after)
            for next_at in successors:
                merged = written_at[next_at] & state if next_at in written_at else state
                if written_at.get(next_at) != merged:
                    written_at[next_at] = merged
                    pending.append(next_at)
        self.memo[entry] = found
        return found


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('start', type=lambda x: int(x, 16))
    parser.add_argument('end', type=lambda x: int(x, 16))
    parser.add_argument('--export', type=Path, default=ROOT / '.local/current/export')
    args = parser.parse_args()
    sys.setrecursionlimit(100000)
    db = sqlite3.connect(args.export / 'instructions.sqlite')
    targets = set()
    for address, assembly in db.execute(
            "SELECT address, json_extract(body,'$.assembly') FROM instructions WHERE address>=? AND address<?",
            (args.start, args.end)):
        match = re.fullmatch(r'bl 0x([0-9a-f]+)', assembly)
        if match:
            targets.add(int(match[1], 16))
    program = Program(db)
    for target in sorted(targets):
        print(f'{target:x} {program.mask(target):02x}')


if __name__ == '__main__':
    main()
