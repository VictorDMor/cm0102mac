"""Pinned complete team line boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x101d56f0, 0x101d56f8, 2, 'c1853f2059a8fba472a208a14240e65b466ffd5d0e9ee4a83b23068ad9841d0d'),
    (0x101d7d30, 0x101d7d88, 22, '49880cc81619b1106bd1a0ea0862d49008f834ac74ef104b47c067ef6395e7c3'),
    (0x101e4e10, 0x101e4ea0, 36, '65fb0cbd8962ef7e78caef9c729589e4f949d4d31b5ad02aef18f098a139d1ec'),
    (0x101e4ea0, 0x101e4ecc, 11, '169bfd63e3bd47fbfe1405bc594eaf3b3e9e832dccd695054bd1901f1c70c0c6'),
    (0x101e4ed0, 0x101e5064, 101, 'edde177a60bb2481510c0f61ecdc111a338a1126f04ca47db9ef91e35731bfc1'),
)
FRAGMENTS = (
    (0x101d56f0, 0x101d5, 2, 'c1853f2059a8fba472a208a14240e65b466ffd5d0e9ee4a83b23068ad9841d0d'),
    (0x101d7d30, 0x101d7, 22, '49880cc81619b1106bd1a0ea0862d49008f834ac74ef104b47c067ef6395e7c3'),
    (0x101e4e10, 0x101e4, 36, '65fb0cbd8962ef7e78caef9c729589e4f949d4d31b5ad02aef18f098a139d1ec'),
    (0x101e4ea0, 0x101e4, 11, '169bfd63e3bd47fbfe1405bc594eaf3b3e9e832dccd695054bd1901f1c70c0c6'),
    (0x101e4ed0, 0x101e4, 76, 'e1017c5c46fe250ca5b58671ac9a6a588807196bdd7edc12e3153acc4fbb854a'),
    (0x101e4ed0, 0x101e5, 25, '488460ce726468c29914fa9c13f63f967c9e7e60bad79e10fdda925b3f6231e9'),
)
TEAM_LINE_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_team_line_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac team-line contract changed or missing at {start:x}')


def require_team_line_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External team-line interior edge from {source:x} to {target:x}')


def require_team_line_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac team-line contract changed or missing at {start:x} page {page:x}')


def require_team_line_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_team_line_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_team_line_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_team_line_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def team_line_hook(entry):
    return f'if (try_modern_team_line(s, 0x{entry:x})) return true;'


def require_team_line_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in TEAM_LINE_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', team_line_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced team-line hook at {entry:x}')
