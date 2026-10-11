#!/usr/bin/env python3
"""Emit a private, bounded static reference for exception-context comparisons.

The original capture calls search inside its own AOT page, so the full page can
cross a run_until stop without returning to the dispatcher. Selecting only the
pinned context and frame bodies makes calls to other entries explicit transfer
boundaries. No selected instruction is modified or interpreted at runtime.
"""
import argparse
import json
import sqlite3
from pathlib import Path
from compile_recomp import generate_page
from exception_runtime_audit import CONTRACTS, require_exception_runtime_bodies


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--with-search', action='store_true', help='Include complete search/catch bodies instead of the capture-prefix stop')
    parser.add_argument('--with-transfers', action='store_true', help='Record original context values and signal completed host-scope transfers')
    parser.add_argument('--with-capture', action='store_true', help='Include scalar capture and the scalar/vector selector with complete search')
    parser.add_argument('--scalar-prefix', action='store_true', help='Select only scalar capture, stopping at its search call')
    parser.add_argument('--complete', action='store_true', help='Select all twenty-one exception bodies including the unexpected landing pad')
    args = parser.parse_args()
    if args.complete:
        if args.scalar_prefix:
            parser.error('--complete cannot select the scalar-prefix boundary')
        args.with_search = True
    if args.with_transfers and not args.with_search:
        parser.error('--with-transfers requires --with-search')
    if args.with_capture and not args.with_search:
        parser.error('--with-capture requires --with-search')
    if args.scalar_prefix and (args.with_search or args.with_transfers or args.with_capture):
        parser.error('--scalar-prefix cannot select full search or transfer modes')
    entries = {0x10771640, 0x107717f0, 0x10771b30, 0x10772430, 0x10772650, 0x10772b10, 0x10772f10}
    if args.with_search:
        entries.update((0x10772780, 0x10772810))
    if args.with_capture:
        entries.update((0x10772ad0, 0x10772ca0))
    if args.scalar_prefix:
        entries = {0x10772ca0}
    if args.complete:
        entries = {a for a, _, _ in CONTRACTS}
    bounds = [(a, b) for a, b, _ in CONTRACTS if a in entries]
    prefix = ('full_reference' if args.complete else 'scalar_reference' if args.scalar_prefix else 'capture_reference' if args.with_capture else
              'transfer_reference' if args.with_transfers else 'search_reference' if args.with_search else 'context_reference')
    records = []
    with sqlite3.connect((args.input / 'instructions.sqlite').resolve().as_uri() + '?mode=ro', uri=True) as db:
        for a, b in bounds:
            body = [json.loads(row[0]) for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (a, b))]
            if not body:
                raise ValueError(f'Missing recovered context body at {a:x}; use the complete exception export')
            require_exception_runtime_bodies(body)
            records.extend(body)
    metadata = json.loads((args.input / 'program.json').read_text())
    pages = sorted({r['address'] >> 12 for r in records})
    sources = []
    for page in pages:
        selected = [r for r in records if r['address'] >> 12 == page]
        code = generate_page(page, selected, {r['address'] for r in selected}, userops=metadata['userops'],
                             guest_context_transfers=args.with_transfers, exception_runtime_hooks=False)
        # Bounded references execute every selected original instruction.
        if 'try_modern_exception_runtime' in code:
            raise ValueError('Context reference must execute the original bodies, not semantic hooks')
        declaration = f'bool page_{page:x}(State& s) {{'
        if code.count(declaration) != 1:
            raise ValueError(f'Missing original reference page {page:x}')
        # Rename the templated implementation too. Otherwise its weak symbols
        # can replace the full production page's instantiations at link time.
        code = code.replace(f'page_{page:x}_impl', f'{prefix}_{page:x}_impl')
        sources.append(code.replace(declaration, f'bool {prefix}_{page:x}(State& s) {{', 1))
    code = '\n'.join(sources)
    if not args.with_search and 's.pc = 0x10772810; return true;' not in code:
        raise ValueError('Missing explicit original capture boundary')
    if args.with_search and 'i_10772810:' not in code:
        raise ValueError('Missing complete original search body')
    code += f'\nnamespace cm::aot {{\nbool exception_{prefix}(State& s) {{\n'
    # A selected body can share a page with a dependency or callback that the
    # harness must execute through the full dispatcher. Claim only the actual
    # selected instructions, not every address in their containing pages.
    code += 'switch (s.pc) {\n'
    for page in pages:
        code += ''.join(f"case 0x{r['address']:x}:\n" for r in records if r['address'] >> 12 == page)
        code += f'return {prefix}_{page:x}(s);\n'
    code += 'default: return false;\n}\n}\n}\n'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if not args.output.exists() or args.output.read_text() != code:
        args.output.write_text(code)
    else:
        # Mark unchanged output current after a dependency changed, so each
        # consumer does not repeatedly rerun the successful custom command.
        args.output.touch()
    print(f'Emitted {len(records)} pinned original context/frame instructions with explicit callee boundaries')


if __name__ == '__main__':
    main()
