"""Pinned complete ball-contact boundaries; component evidence is not productive acceptance."""
import hashlib
import json
import struct

CONTRACTS = (
    (0x101f5050, 0x101f50b8, 26, '48f8dc033812534561db5f85fc1445efec6fb762dd28ee27f21cb1ea532379d6'),
    (0x101f50c0, 0x101f5110, 20, 'ad66c601f0001f02ce9cec90c5f8b2598d41f18d717a8900d1017559151455b2'),
    (0x101f5110, 0x101f5180, 28, 'e33c3207298656fa995e0f9cf77a8d0573d9dc544e2e88fbf9dc2c69dd0257b7'),
    (0x101f5180, 0x101f51e0, 24, '6ef24ee6579d01008c5cbb4dadfd5f0df5f3da506b1176a1b6b3f86827a897d0'),
    (0x101f5350, 0x101f6940, 1404, 'ba168e05dfa6e7094912a71e14a43de7c25cb736a95e4f5b19be0fd8664786ce'),
    (0x10270320, 0x10270b94, 541, '96058bda47f0ec291e0008b339916becce57073cde75d2d2a44947ab04cf19fe'),
)

FACADE_ENTRIES = tuple(row[0] for row in CONTRACTS[:4])
ENTRIES = tuple(row[0] for row in CONTRACTS)

TABLES = (
    (0x101f5ab4, 0x108b67b0, 0x108ccc50, 48, '0d907543483c964d8af0ac7952a7978d38a0b040ab624b95c87fc54e4e539a22'),
    (0x101f6428, 0x108b67ac, 0x108ccc28, 10, '0245272b346c9f9090928f904f748a868a2e47ed5423f833f11518b9ec77e0ef'),
    (0x101f66b0, 0x108b67a8, 0x108ccc04, 9, 'd0ee8ff4d638016e7d4e28cd415013d2cc8695d644838f2389616d66e4bbf1b6'),
)

def fingerprint(records):
    fields = ('address', 'assembly', 'ops', 'flows', 'fallthrough')
    return hashlib.sha256(json.dumps([{key: row.get(key) for key in fields} for row in records], sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_ball_contact_bodies(records):
    if any(not any(start <= row['address'] < end for start, end, *_ in CONTRACTS) for row in records):
        raise ValueError('Unrelated original contact body')
    for start, end, count, digest in CONTRACTS:
        part = [row for row in records if start <= row['address'] < end]
        if len(part) != count or fingerprint(part) != digest:
            raise ValueError(f'Changed or incomplete original contact body at {start:x}')


def require_ball_contact_edges(edges):
    for source, target in edges:
        for start, end, *_ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External contact interior entrance {source:x} -> {target:x}')


def load_ball_contact(db):
    records = [json.loads(row[0]) for start, end, *_ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    require_ball_contact_bodies(records)
    require_ball_contact_edges(db.execute("""SELECT i.address,e.value FROM instructions AS i,
        json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))
    return records


def require_ball_contact_tables(read_word):
    tables = {}
    for site, slot, table, count, digest in TABLES:
        if read_word(slot) != table:
            raise ValueError(f'Changed original contact table pointer at {slot:x}')
        words = [read_word(table + n * 4) for n in range(count)]
        if hashlib.sha256(b''.join(struct.pack('>I', word) for word in words)).hexdigest() != digest:
            raise ValueError(f'Changed original contact table at {table:x}')
        if any(not CONTRACTS[4][0] <= word < CONTRACTS[4][1] or word % 4 for word in words):
            raise ValueError(f'Exterior or unaligned contact table destination at {site:x}')
        tables[site] = words
    return tables


def load_ball_contact_tables(input_directory):
    program = json.loads((input_directory / 'program.json').read_text())
    segments = [(segment['address'], (input_directory / segment['file']).read_bytes())
                for segment in program['segments'] if segment.get('file') and not segment.get('execute')]
    def read_word(address):
        for base, data in segments:
            if base <= address and address + 4 <= base + len(data):
                return struct.unpack_from('>I', data, address - base)[0]
        raise ValueError(f'Missing original contact table word at {address:x}')
    return require_ball_contact_tables(read_word)


SHARED_DISCIPLINE_ENTRIES = (0x101f5110, 0x101f5180)
BALL_CONTACT_ENTRIES = tuple(entry for entry in ENTRIES if entry not in SHARED_DISCIPLINE_ENTRIES)
FRAGMENTS = (
    (0x101f5050, 0x101f5, 26, '48f8dc033812534561db5f85fc1445efec6fb762dd28ee27f21cb1ea532379d6'),
    (0x101f50c0, 0x101f5, 20, 'ad66c601f0001f02ce9cec90c5f8b2598d41f18d717a8900d1017559151455b2'),
    (0x101f5110, 0x101f5, 28, 'e33c3207298656fa995e0f9cf77a8d0573d9dc544e2e88fbf9dc2c69dd0257b7'),
    (0x101f5180, 0x101f5, 24, '6ef24ee6579d01008c5cbb4dadfd5f0df5f3da506b1176a1b6b3f86827a897d0'),
    (0x101f5350, 0x101f5, 812, '31821166a4956dced06f0903e32621070dc937a44df82e8f8ca437a000f7421e'),
    (0x101f5350, 0x101f6, 592, '786fbaa9f244fc6ed02f42421248070378006ec51df26f7d549ae093cb79acd9'),
    (0x10270320, 0x10270, 541, '96058bda47f0ec291e0008b339916becce57073cde75d2d2a44947ab04cf19fe'),
)

def require_ball_contact_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac ball-contact contract changed or missing at {start:x} page {page:x}')


def require_ball_contact_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_ball_contact_bodies(records)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_ball_contact_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_ball_contact_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def ball_contact_hook(entry):
    if entry not in BALL_CONTACT_ENTRIES:
        raise ValueError('Only complete contact roots are callable')
    return f'if (try_modern_ball_contact(s, 0x{entry:x})) return true;'


def require_ball_contact_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in BALL_CONTACT_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', ball_contact_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced ball-contact hook at {entry:x}')
