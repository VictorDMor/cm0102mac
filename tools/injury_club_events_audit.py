"""Pinned complete club injury event queue boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x1018bb50, 0x1018bdd0, 160, '16f5387eb2bedef7b9b9ffc160128a7ded683056c45596473785272b962fdbe8'),
    (0x1018bdd0, 0x1018c180, 236, 'fa7f04faa7d225fcd0223cfcbc05da7e9a8888a6445697a57a43b2f2557d4883'),
    (0x101900b0, 0x10190180, 52, '48148b7679b910d794233629b46cf1ce16f0a2799f3b44b1948e17b1c5dea096'),
)
FRAGMENTS = (
    (0x1018bb50, 0x1018b, 160, '16f5387eb2bedef7b9b9ffc160128a7ded683056c45596473785272b962fdbe8'),
    (0x1018bdd0, 0x1018b, 140, '7bf0f2ff34290aa622e974096c60d0c6f104f790832a68d209fdc1fce63f7d5e'),
    (0x1018bdd0, 0x1018c, 96, 'd5b5d0a65e3fc65e5191c8ba66da30d1e911ff28b05a4c89c2528f34989004c0'),
    (0x101900b0, 0x10190, 52, '48148b7679b910d794233629b46cf1ce16f0a2799f3b44b1948e17b1c5dea096'),
)
INJURY_CLUB_EVENTS_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_injury_club_events_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-club-events contract changed or missing at {start:x}')


def require_injury_club_events_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External injury-club-events interior edge from {source:x} to {target:x}')


def require_injury_club_events_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac injury-club-events contract changed or missing at {start:x} page {page:x}')


def require_injury_club_events_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_injury_club_events_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_injury_club_events_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_injury_club_events_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def injury_club_events_hook(entry):
    return f'if (try_modern_injury_club_events(s, 0x{entry:x})) return true;'


def require_injury_club_events_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in INJURY_CLUB_EVENTS_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', injury_club_events_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced injury-club-events hook at {entry:x}')


CALLBACK_DESCRIPTORS = ((0x108b6630,0x108b8b44,0x101900b0,0x108bba90),)

def require_injury_club_events_tables(word):
    for slot,descriptor,entry,toc in CALLBACK_DESCRIPTORS:
        if word(slot)!=descriptor or word(descriptor)!=entry or word(descriptor+4)!=toc:
            raise ValueError('Changed club injury event sort descriptor')

def load_injury_club_events_tables(input_directory):
    import struct
    program=json.loads((input_directory/'program.json').read_text())
    segments=[(s['address'],(input_directory/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    def word(address):
        for base,data in segments:
            if base<=address and address+4<=base+len(data):return struct.unpack_from('>I',data,address-base)[0]
        raise ValueError('Missing club injury event descriptor word')
    require_injury_club_events_tables(word)
