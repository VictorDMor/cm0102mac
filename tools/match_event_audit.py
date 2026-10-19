"""Pinned complete original match event pipeline boundaries."""
import hashlib
import json

CONTRACTS = ((270414208, 270414240, 8, 'e2f109bee0273364ad93eb70c47448f36813fd3567a23460bdcae0c67d57af00'), (270526992, 270527116, 31, '47378e9775d9247ebdf267ef2e60752ef3dc03b86af9298aede69df4f20edfaa'), (270528976, 270530028, 263, 'eaf490fc6c38bf79c60e7c791113f760b4d92ac6e6b8f61e8907dd3c6a9a4b9d'), (270530032, 270531432, 350, 'fd502cfaa33d79e8fde3b22ee49e211940103cca5eb90f3cea65331e8a57dc31'), (270531440, 270536824, 1346, '0e5bfa93692d37001030f67a3ac18d4cc33e396b71bd75b0d366c333d58c1387'), (270536832, 270536896, 16, '4dcba183ef0cc6f5cf85ef633c553e027ce56e8beb55eb9c8f55382a10226504'), (270536896, 270537088, 48, '4ec29b1d0fd7dcf3b831c2ed0e402977ee53ed3ac00b5f9ed2b16a34709abcc3'), (270537088, 270540336, 810, 'f991a63afa654acbd9b309368f39c309e2a11ff6c761120f25350132aaaf1d81'), (270540336, 270540516, 45, 'a269a00dd353b6f6e1f0f59cb73e4a54a49c6ad5a08ec7a076b31de9db2eb362'))
FRAGMENTS = ((270414208, 66019, 8, 'e2f109bee0273364ad93eb70c47448f36813fd3567a23460bdcae0c67d57af00'), (270526992, 66046, 31, '47378e9775d9247ebdf267ef2e60752ef3dc03b86af9298aede69df4f20edfaa'), (270528976, 66047, 263, 'eaf490fc6c38bf79c60e7c791113f760b4d92ac6e6b8f61e8907dd3c6a9a4b9d'), (270530032, 66047, 350, 'fd502cfaa33d79e8fde3b22ee49e211940103cca5eb90f3cea65331e8a57dc31'), (270531440, 66047, 292, '2e25133706f4b5f5c4f01b9d95faf7a6b082ddf10b8610f61e022f21c8c30fa9'), (270531440, 66048, 1024, '9c384b949fdff72b19529f1c53a98324bfabb9f47663acb58afb03cbd57d038a'), (270531440, 66049, 30, '4da1cefdbd7fd3ebe53314f9a7113e968bb4747f216a8cad0a02ceb443abe8cb'), (270536832, 66049, 16, '4dcba183ef0cc6f5cf85ef633c553e027ce56e8beb55eb9c8f55382a10226504'), (270536896, 66049, 48, '4ec29b1d0fd7dcf3b831c2ed0e402977ee53ed3ac00b5f9ed2b16a34709abcc3'), (270537088, 66049, 810, 'f991a63afa654acbd9b309368f39c309e2a11ff6c761120f25350132aaaf1d81'), (270540336, 66049, 45, 'a269a00dd353b6f6e1f0f59cb73e4a54a49c6ad5a08ec7a076b31de9db2eb362'))
MATCH_EVENT_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_match_event_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac match-event contract changed or missing at {start:x}')


def require_match_event_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External match-event interior edge from {source:x} to {target:x}')


def require_match_event_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac match-event contract changed or missing at {start:x} page {page:x}')


def require_match_event_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_match_event_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_match_event_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_match_event_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (min(v[0] for v in CONTRACTS), max(v[1] for v in CONTRACTS))))


def match_event_hook(entry):
    return f'if (try_modern_match_event(s, 0x{entry:x})) return true;'


def require_match_event_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in MATCH_EVENT_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', match_event_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced match-event hook at {entry:x}')
