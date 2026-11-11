#!/usr/bin/env python3
"""What a routine reads of its caller's registers, and what its callers read of the ones it leaves.

  register_contract.py ENTRY...            for each routine: the registers it (or a routine it calls)
                                           reads before writing them: r3-r10 as a mask (as
                                           call_arguments.py prints) and every other register by name
                                           (r13-r31, f14-f31: the caller's values; r0, r11, r12, f0-f13:
                                           leftovers), split into what its own body reads and what only
                                           the routines it calls read
  register_contract.py --after ENTRY...    for each `bl ENTRY` in the export: the volatile registers
                                           (r0, r3-r12, f1-f13) the caller reads after the call
                                           before writing them: r3 or f1 is the result, anything else
                                           a leftover the routine must keep leaving

A rewrite that keeps its values in native locals leaves the non-volatile registers as its caller
had them and the volatile ones as its last callee left them. That is the original's behaviour only
where nobody reads them: this lists the readers. A store of a non-volatile register to memory
before the routine writes it is a save (a prologue, a context save), not a read. An indirect
call or an import is taken to read r3-r10 and nothing else. Reads are found along every path, so a
register listed here may be read on some paths only, or on none that can happen: the walk does not
know which paths are feasible and does not follow jump tables. Each listed register is a place to
look at the listing, not a finding.
"""
import argparse
import json
from pathlib import Path
import re
import sqlite3
import sys

ROOT = Path(__file__).resolve().parent.parent
GPR = {n * 4: f'r{n}' for n in range(32)}
FPR = {0x4000 + n * 16: f'f{n}' for n in range(32)}
ARGUMENTS = {(3 + n) * 4: n for n in range(8)}
VOLATILE = {n * 4 for n in (0, *range(3, 13))} | {0x4000 + n * 16 for n in range(14)}
TRACKED = (set(GPR) - {4, 8}) | set(FPR)  # not r1 (the stack pointer) or r2 (the table of contents)
INDIRECT = 0x10781320


class Program:
    def __init__(self, db):
        self.db = db
        self.memo = {}
        self.cache = {}

    def record(self, address):
        if address not in self.cache:
            row = self.db.execute('SELECT body FROM instructions WHERE address=?', (address,)).fetchone()
            self.cache[address] = json.loads(row[0]) if row else None
        return self.cache[address]

    @staticmethod
    def accesses(record):
        """(read, written) tracked register offsets of one instruction, in order of effect."""
        reads, writes = set(), set()
        for op in record.get('ops', []):
            output, inputs = op[1], op[2:]
            for index, item in enumerate(inputs):
                if isinstance(item, list) and item and item[0] == 'register':
                    offset = int(item[1])
                    offset = offset & ~3 if offset < 0x1000 else offset & ~15
                    # The value a STORE writes is a save when the register is the caller's.
                    saved = op[0] == 'STORE' and index == 2 and offset not in VOLATILE
                    if offset in TRACKED and offset not in writes and not saved:
                        reads.add(offset)
            if isinstance(output, list) and output and output[0] == 'register':
                offset = int(output[1])
                if offset in TRACKED and output[2] in (4, 8):
                    writes.add(offset)
        return reads, writes

    def walk(self, start, written, own=None):
        """Tracked registers read before being written on some path from `start` to a return;
        `own` collects the ones this code reads itself, not through a routine it calls."""
        written_at = {start: frozenset(written)}
        pending = [start]
        found = set()
        while pending:
            at = pending.pop()
            record = self.record(at)
            if record is None:
                continue
            written = written_at[at]
            reads, writes = self.accesses(record)
            found |= reads - written
            if own is not None:
                own |= reads - written
            after = set(written) | writes
            successors = []
            if record['flow'] == 'UNCONDITIONAL_CALL':
                for target in record['flows']:
                    found |= self.live_in(target) - after
                after |= VOLATILE
                if record.get('fallthrough') is not None:
                    successors.append(record['fallthrough'])
            elif record['flow'] not in ('TERMINATOR', 'CALL_TERMINATOR'):
                successors.extend(record.get('flows') or [])
                if record.get('fallthrough') is not None:
                    successors.append(record['fallthrough'])
            state = frozenset(after)
            for next_at in successors:
                merged = written_at[next_at] & state if next_at in written_at else state
                if written_at.get(next_at) != merged:
                    written_at[next_at] = merged
                    pending.append(next_at)
        return found

    def live_in(self, entry):
        if entry in self.memo:
            return self.memo[entry]
        if entry == INDIRECT or self.record(entry) is None:
            self.memo[entry] = frozenset(ARGUMENTS)
            return self.memo[entry]
        self.memo[entry] = frozenset()  # a cycle adds nothing the first pass doesn't find
        self.memo[entry] = frozenset(self.walk(entry, ()))
        return self.memo[entry]


def names(offsets):
    return ' '.join(sorted(({**GPR, **FPR})[o] for o in offsets))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('entries', nargs='+', type=lambda x: int(x, 16))
    parser.add_argument('--after', action='store_true')
    parser.add_argument('--export', type=Path, default=ROOT / '.local/current/export')
    args = parser.parse_args()
    sys.setrecursionlimit(100000)
    db = sqlite3.connect(args.export / 'instructions.sqlite')
    program = Program(db)
    for entry in args.entries:
        if not args.after:
            live = program.live_in(entry)
            own = set()
            program.walk(entry, (), own)
            mask = sum(1 << bit for offset, bit in ARGUMENTS.items() if offset in live)
            rest = live - set(ARGUMENTS)
            print(f'{entry:x} {mask:02x} itself: {names(rest & own) or "-"}; through its calls: {names(rest - own) or "-"}')
            continue
        sites = [address for (address,) in db.execute(
            "SELECT address FROM instructions WHERE json_extract(body,'$.assembly')=?", (f'bl 0x{entry:x}',))]
        for site in sites:
            record = program.record(site)
            after = record.get('fallthrough')
            volatile = {n * 4 for n in (0, *range(3, 13))} | {0x4000 + n * 16 for n in range(1, 14)}
            # Everything else counts as written: only the leftovers of the call are asked for.
            read = program.walk(after, TRACKED - volatile) & volatile if after is not None else set()
            print(f'{entry:x} called at {site:x}: {names(read) or "-"}')
        if not sites:
            print(f'{entry:x}: no direct call in the export')


if __name__ == '__main__':
    main()
