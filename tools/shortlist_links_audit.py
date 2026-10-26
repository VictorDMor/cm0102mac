"""Pinned complete shortlist link indices, transitions and contract marker operations."""
import hashlib
import json
import struct

CONTRACTS = ((271986032, 271986164, 33, '7df2562d5d8e2f70f620316c8415c30eb33bc57aed6c7becaa9c959d9bb85ba3'), (271986176, 271986412, 58, 'e281fd2f836ef0633c656455688a74909c6e60bdea58906d739fec0cf3661e0c'), (271986416, 271986984, 141, '2c9b601c5198ae2cb5a001b4473464ca8ad8e6d1da1a462340aff7ea21325574'), (271986992, 271987428, 109, '3866bce591d2e338aa070c309bf611b812b050e732de773a10bfd36f6e567807'), (271987440, 271988396, 239, 'b39e843d84793ca154d9a6933c0a4acb1f2608075e30bd1535e1c5d063b14ef7'), (271989632, 271989912, 69, '0c39491c7f68c196634ef250f7bee48295f6e4f5a97de2bfc818ac4a314da041'), (271996304, 271997992, 422, 'c7e59408b4a4d65e168613bf6b2a31c2aac0555f05da73a7cd6574272c23437d'), (271998000, 271998072, 18, 'bbc659313f597faf7278687fbca037c328e8404fc3ad86d4552f9de5ebb366e9'), (271998080, 271998140, 15, '67d2d2314551d17b769f828643a0b24743f4ed076c70149f7607d1536af2a805'), (271998144, 271998216, 18, '683db3c14e5ed95ab3acb9c15be760440c0873260e633cee286942f452b28f59'))
FRAGMENTS = ((271986032, 66402, 33, '7df2562d5d8e2f70f620316c8415c30eb33bc57aed6c7becaa9c959d9bb85ba3'), (271986176, 66402, 58, 'e281fd2f836ef0633c656455688a74909c6e60bdea58906d739fec0cf3661e0c'), (271986416, 66402, 68, 'e7c8b63204c5a64a1e2e58d11a61b7b3edf9031f8d76360b47660cb14e6ca6a6'), (271986416, 66403, 73, '525c0042b75f69090a512f510dfbcd191c83ffcfaa622050621f1873c7193132'), (271986992, 66403, 109, '3866bce591d2e338aa070c309bf611b812b050e732de773a10bfd36f6e567807'), (271987440, 66403, 239, 'b39e843d84793ca154d9a6933c0a4acb1f2608075e30bd1535e1c5d063b14ef7'), (271989632, 66403, 69, '0c39491c7f68c196634ef250f7bee48295f6e4f5a97de2bfc818ac4a314da041'), (271996304, 66405, 422, 'c7e59408b4a4d65e168613bf6b2a31c2aac0555f05da73a7cd6574272c23437d'), (271998000, 66405, 18, 'bbc659313f597faf7278687fbca037c328e8404fc3ad86d4552f9de5ebb366e9'), (271998080, 66405, 15, '67d2d2314551d17b769f828643a0b24743f4ed076c70149f7607d1536af2a805'), (271998144, 66405, 18, '683db3c14e5ed95ab3acb9c15be760440c0873260e633cee286942f452b28f59'))
SHORTLIST_LINKS_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_SHORTLIST_LINKS_ENTRIES = SHORTLIST_LINKS_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_shortlist_links_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac shortlist-links contract changed or missing at {start:x}')


def require_shortlist_links_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External shortlist-links interior edge from {source:x} to {target:x}')


def require_shortlist_links_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac shortlist-links contract changed or missing at {start:x} page {page:x}')


def require_shortlist_links_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_shortlist_links_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_shortlist_links_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_shortlist_links_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277572792, 276870732, 41, 'ce07ac15ec347dbdfdd04f996404bc24d13385e5b0400df475d23959b9d5d4b7'), (277567464, 277582348, 8, '27cc863287f340c6fc75f1da988cca5cef8e2abff48ca0bf2150e4047753576f'), (277567480, 277582380, 8, '3203ea67eaa0c1d4d49ef46930b25ed45bd72110025d9217a29f38b90359d681'))

def require_shortlist_links_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original shortlist links table")

def load_shortlist_links_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes()) for s in program["segments"] if s.get("file")]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original shortlist-links table")
    require_shortlist_links_tables(read)
    require_shortlist_links_padding(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)


PADDING = (271986288, 271986856, 271989800)

def require_shortlist_links_padding(read):
    for address in PADDING:
        if read(address,4)!=bytes.fromhex("60000000"):
            raise ValueError("Changed non-indexed shortlist padding")


def shortlist_links_hook(entry):
    return f'if (try_modern_shortlist_links(s, 0x{entry:x})) return true;'


def require_shortlist_links_hooks(page, instructions, code, taken_over=()):
    """Each entry has its hook, except those another family took over."""
    addresses = {r['address'] for r in instructions}
    for entry in SHORTLIST_LINKS_ENTRIES:
        if entry >> 12 != page or entry not in addresses or entry in taken_over: continue
        label, hook = f'i_{entry:x}:', shortlist_links_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced shortlist-links hook at {entry:x}')
