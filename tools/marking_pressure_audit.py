"""Pinned complete actor marking lists, eligibility and local pressure boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x101d5660, 0x101d5668, 2, '3dec4f35cb322161c85da6e5a116e2791e02f6cc985107fa478229401dafe8bb'),
    (0x101d56a0, 0x101d56a8, 2, '63c47e701f7c5bcfb03b1ee45109ff1df3125efcc48c2206c7c94472b41f5430'),
    (0x101d5700, 0x101d5730, 12, 'e6392744885fcffaaf6dcc082c103ff7664dc397aa4561eb400dd411a1fb9350'),
    (0x101d57d0, 0x101d5804, 13, '06984e474010516d87c5c03b3cc589f2d980b95ef1d2ebbbc9a0daaecdeedeee'),
    (0x101d7da0, 0x101d7da8, 2, '1d3e0e57e07375da9171edb7148601dcdb6288d35424b0bab26d51b347516270'),
    (0x101d7dc0, 0x101d8100, 208, '744c7be322db8a4a2a7a714b4db562134edf240cdd9366b198c5dc946d9739f0'),
    (0x101d8100, 0x101d8108, 2, 'a7888848c033e5aa6e525cc00476608d47503166bdded583177dce27f119db22'),
    (0x101d8110, 0x101d8118, 2, 'c8dba9aed111078efceff8f11572f6e8bdaea4993f7a0fec080f982c448bea11'),
    (0x101d8120, 0x101d8254, 77, 'fb98370c03eae2f45ba00c0246e6ca541397a3f3abea2cc8d26c066b1069c58b'),
    (0x102373c0, 0x1023741c, 23, '8e77d51c8b1ebf165d81fbe1161e31ece3c715d50dce31cee2ff6b69f2128560'),
    (0x10283540, 0x10283588, 18, 'd6fde2a2d035b1c1eef38f4d4b849aa45e618d71ec66f9e520b8c41c80be74c3'),
    (0x10283590, 0x10283614, 33, 'b8cc8d4a34e13d7d21ec968eb19cffdcc625f8c193156e0ce18e56a4946f15a4'),
    (0x10283690, 0x10283698, 2, '3a583b9dbc54ed422ee2663401b4b2977b5c1da1aced932f948369c0e6259571'),
)
FRAGMENTS = (
    (0x101d5660, 0x101d5, 2, '3dec4f35cb322161c85da6e5a116e2791e02f6cc985107fa478229401dafe8bb'),
    (0x101d56a0, 0x101d5, 2, '63c47e701f7c5bcfb03b1ee45109ff1df3125efcc48c2206c7c94472b41f5430'),
    (0x101d5700, 0x101d5, 12, 'e6392744885fcffaaf6dcc082c103ff7664dc397aa4561eb400dd411a1fb9350'),
    (0x101d57d0, 0x101d5, 13, '06984e474010516d87c5c03b3cc589f2d980b95ef1d2ebbbc9a0daaecdeedeee'),
    (0x101d7da0, 0x101d7, 2, '1d3e0e57e07375da9171edb7148601dcdb6288d35424b0bab26d51b347516270'),
    (0x101d7dc0, 0x101d7, 144, '6c3128abfc51272a60c11e144172ef244f740b78289f379f873cdc735ccdbea1'),
    (0x101d7dc0, 0x101d8, 64, '388f2875ac4711555c909a6fb0e325f0356ae6667d090ea6d9333ab2a59fbd92'),
    (0x101d8100, 0x101d8, 2, 'a7888848c033e5aa6e525cc00476608d47503166bdded583177dce27f119db22'),
    (0x101d8110, 0x101d8, 2, 'c8dba9aed111078efceff8f11572f6e8bdaea4993f7a0fec080f982c448bea11'),
    (0x101d8120, 0x101d8, 77, 'fb98370c03eae2f45ba00c0246e6ca541397a3f3abea2cc8d26c066b1069c58b'),
    (0x102373c0, 0x10237, 23, '8e77d51c8b1ebf165d81fbe1161e31ece3c715d50dce31cee2ff6b69f2128560'),
    (0x10283540, 0x10283, 18, 'd6fde2a2d035b1c1eef38f4d4b849aa45e618d71ec66f9e520b8c41c80be74c3'),
    (0x10283590, 0x10283, 33, 'b8cc8d4a34e13d7d21ec968eb19cffdcc625f8c193156e0ce18e56a4946f15a4'),
    (0x10283690, 0x10283, 2, '3a583b9dbc54ed422ee2663401b4b2977b5c1da1aced932f948369c0e6259571'),
)
MARKING_PRESSURE_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_marking_pressure_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac marking-pressure contract changed or missing at {start:x}')


def require_marking_pressure_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External marking-pressure interior edge from {source:x} to {target:x}')


def require_marking_pressure_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac marking-pressure contract changed or missing at {start:x} page {page:x}')


def require_marking_pressure_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_marking_pressure_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_marking_pressure_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_marking_pressure_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def marking_pressure_hook(entry):
    return f'if (try_modern_marking_pressure(s, 0x{entry:x})) return true;'


def require_marking_pressure_hooks(page, instructions, code, taken_over=()):
    addresses = {r['address'] for r in instructions}
    for entry in MARKING_PRESSURE_ENTRIES:
        if entry >> 12 != page or entry not in addresses or entry in taken_over: continue
        label, hook = f'i_{entry:x}:', marking_pressure_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced marking-pressure hook at {entry:x}')
