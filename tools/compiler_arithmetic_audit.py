"""Pinned original contracts for the Mac compiler arithmetic helper block."""
import hashlib
import json

CONTRACTS = (
    (0x10773780, 0x107737d0, 20, 'decb1a8c06d57b4151543f7e1eba9803549371fddbee2a93482fb6defe06cb3b'),
    (0x10773870, 0x1077395c, 59, '97b2bbe0d7f7d13c545c90d312f0fdc825eed2b599eeac82b86518f507d243bb'),
    (0x10773960, 0x10773a90, 76, '2ba4b9094dc3889a72b1dfb0830e9b22864f3e5baf2c69c754a872a6a117eebb'),
    (0x10773a90, 0x10773b70, 56, 'cbada6c54a27fa77a267379589790277462a9a23aacb4673f95b53bc625147e9'),
    (0x10773b70, 0x10773c7c, 67, '0a87c187fe59c2a6b52340a4f09f31e4f72a603d1806730ad80e91de3eab9718'),
    (0x10773c80, 0x10773d28, 42, '732d8095f048bfb60440aed59c7e635dbe92dd367040526f948add5043f76096'),
    (0x10773d30, 0x10773ddc, 43, 'a20f7b0cfcb266db6c3e7b784df96bf24f2c748c1d99f1ec47f354580f280ac4'),
    (0x10773de0, 0x10773ea4, 49, '044df206c652517be64942d3d93a0105f687a5dbe903bac08b44136461f1ca3c'),
)
COMPILER_ARITHMETIC_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def require_compiler_arithmetic_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if complete and not body:
            raise ValueError(f'Missing complete Mac compiler arithmetic body at {start:x}')
        if not body:
            continue
        canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
        actual = hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
        if len(body) != count or actual != expected:
            raise ValueError(f'Mac compiler arithmetic contract changed at {start:x}')


def require_compiler_arithmetic_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External compiler arithmetic interior edge from {source:x} to {target:x}')


def require_compiler_arithmetic_contract(page, instructions, selected=None):
    if page != 0x10773:
        return
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if start in addresses:
            body = [r for r in instructions if start <= r['address'] < end]
            if not body:
                raise ValueError(f'Missing complete Mac compiler arithmetic body at {start:x}')
            require_compiler_arithmetic_bodies(body)


def require_compiler_arithmetic_index(db, selected=None):
    records = [json.loads(row[0]) for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
        (CONTRACTS[0][0], CONTRACTS[-1][1]))] if selected is None else [r for _, r in sorted(selected.items())]
    if selected is None:
        require_compiler_arithmetic_bodies(records, complete=True)
    else:
        require_compiler_arithmetic_contract(0x10773, records, selected)
    require_compiler_arithmetic_edges(db.execute("""SELECT i.address, edge.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body, '$.flows'),
                                '$[#]', json_extract(i.body, '$.fallthrough'))) AS edge
        WHERE edge.value>=? AND edge.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def compiler_arithmetic_hook(entry):
    return (f'if (counter.semantic_boundary(0x{entry:x}, [&] {{ return '
            f'try_modern_compiler_arithmetic(s, 0x{entry:x}); }})) return true;')


def require_compiler_arithmetic_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    entries = [entry for entry in COMPILER_ARITHMETIC_ENTRIES if entry >> 12 == page and entry in addresses]
    for entry in entries:
        label, hook = f'i_{entry:x}:', compiler_arithmetic_hook(entry)
        if label not in code or code.count(hook) != 1:
            raise ValueError(f'Missing or duplicate compiler arithmetic hook at {entry:x}')
        fragment = code.split(label, 1)[1].split(f'i_{entry + 4:x}:', 1)[0]
        step = f'counter.step(0x{entry:x});'
        if hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Misplaced compiler arithmetic hook at {entry:x}')
    if entries and 'try_fast_unsigned_division(s)' in code:
        raise ValueError('Legacy page-level unsigned division shortcut bypasses compiler arithmetic diagnostics')
