"""Pinned Mac guest arenas/pools, audited against the guarded production oracle."""
import hashlib
import json

CONTRACTS = (
    (0x107740f0, 0x10774184, '899991252ae8d0a90551f0d0e27aff502b549b8bc4734cf233ca038dd0418874'),
    (0x10774190, 0x10774268, 'e91c37ea32029bb0a5850ad914d8f1b92978176ffd0354232ee6ab753b74bb94'),
    (0x10774270, 0x10774354, 'a9badebe7c44a6c29d520e42ed9646e8853cefea5f1c061abefc797c46349278'),
    (0x10774360, 0x107743d4, '46db492d7b8bf0117155650d71021ceaa443cc7a0d7bc047a4b254c062969afa'),
    (0x107743e0, 0x10774430, 'defee65552080892727defd8cffa736f0841303341b081347028c8ca5c09315c'),
    (0x10774430, 0x107744d0, '1b3759e1aad753dacd69eb8885ee1fe5e54b128f41865de89a2bdd167090aa7a'),
    (0x107744d0, 0x10774568, '002e8f3b47aced8869ba50259b0213cafc05c9a9a29d9c046a6f6ab10e09f933'),
    (0x10774570, 0x1077462c, '346e27dbb2ba8cc8f11e08e50205fcec3ad3f26c0f9b7821ab440509d618cec5'),
    (0x10774630, 0x10774674, 'ad89c9bf7c181551a63b09da1151ed8545f3ba7be7172c500149ca8438cd0995'),
    (0x10774680, 0x107746cc, '064156d3d9f352309ea1335ba3b8a7083de97ea14cc9a21317d85609f101b39b'),
    (0x107746d0, 0x10774754, '5a4266e828c346e4cf92a6f4a48982cc12eae20181fee480e1b0d35391ca526f'),
    (0x10774760, 0x1077483c, 'f888e846ac6fded2020fef449ad6121319c015714e9ea42b384b4d5b86608fd2'),
    (0x10774840, 0x1077491c, '233fa935bc02c70159aab33616bcb29fb98bc4c8e2234abaf7a331c72c5bed23'),
    (0x10774920, 0x107749ac, '651472f54721c0bfe36e403a82b80c3f5db907e2d0db84e90ae039385ae5a87c'),
    (0x107749b0, 0x10774ad4, '860f062b04be068f9350cc9a7a6360d5d98ece352b00594f7f9c897ee1655fd2'),
    (0x10774ae0, 0x10774b08, '6f079c08c2fa85903a15178d319cc28b97303b6cd66cce7b213720fe871eb8cd'),
    (0x10774b10, 0x10774b58, '19336e3f73c506ecd4a97f7814da2c88c081d95e671f38d7d2294b6d949fde69'),
    (0x10774b60, 0x10774d20, '179cd6ecfc7129e22dfb7642b997dcee4c75e71cba1de3640ca676dad800fca0'),
    (0x10774d20, 0x10774e74, 'daf82fb8c1795572a2102f5374813720c4b6171917059acebcec855369fec39a'),
    (0x10774e80, 0x10774ed4, 'abeb332fa94f94fd97cda4eac9f5fc82fd5764ff1611d06cfb7dd76904c34a5b'),
    (0x10774ee0, 0x10774f38, '224588c051e02fca7eabd023d99ef087e9d3cd6fe85bb9d767dd40176ab260db'),
    (0x10774f40, 0x107750e0, 'de1e23660c8ab1f486b7b861ad2631aa25b9341609065cce13fab2c20bdd3ce7'),
    (0x107750e0, 0x1077512c, '95446dda867a8c76c3dd7075c60f4bc164662749615e3e9d0a2e0993e8393e2b'),
    (0x10775130, 0x10775164, '8c93c7ba18df1a72fbb8a46d81d3e5373180e9f65b65f34ab4777a08f298263f'),
    (0x10775170, 0x107751a4, '56a985f44c579aafb7ee77d6a06cd133a3770dfdd43a1c6edbd4e5c4ec60e71e'),
    (0x107751b0, 0x107751f4, 'fd300973f33c95758ade4fc7b0d9da5b0b8836635c55a7a3fb6987486d206be9'),
    (0x10775200, 0x10775240, 'd1840afa4b2dc77cb1c531b1f4a85af844c37bd7f07c66d65f032dd856455056'),
)
ALLOCATOR_ENTRIES = tuple(start for start, _, _ in CONTRACTS)
INSTRUCTION_COUNTS = (37, 53, 57, 29, 20, 40, 38, 47, 17, 19, 33, 55, 55, 35, 73, 10, 18, 111, 85, 21, 22, 104, 19, 13, 13, 17, 16)
REALLOCATION_PAGE_PINS = {
    0x10774: '4c2c60a384d74408d7a49906fb09f30964ec7191ab11857a4c60eabff422bb7e',
    0x10775: '20e909d26bf62b6c1bec4877f0024a4e49b6b6756c4b5800c576bd49c0172e54',
}


def _require(records, expected, start):
    body = [{key: item.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')}
            for item in records]
    actual = hashlib.sha256(json.dumps(body, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if actual != expected:
        raise ValueError(f'Mac allocator contract changed at {start:x}')


def require_allocator_contract(page, instructions):
    for start, end, expected in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12:
            continue
        records = [r for r in instructions if start <= r['address'] < end]
        if records:
            _require(records, REALLOCATION_PAGE_PINS[page] if start == 0x10774f40 else expected, start)


def require_allocator_bodies(instructions, complete=False):
    for start, end, expected in CONTRACTS:
        records = [r for r in instructions if start <= r['address'] < end]
        if complete and not records:
            raise ValueError(f'Missing complete Mac allocator body at {start:x}')
        if records:
            _require(records, expected, start)


def require_allocator_edges(edges):
    for source, target in edges:
        for start, end, _ in CONTRACTS:
            if start <= target < end and not start <= source < end and target != start:
                raise ValueError(f'External allocator interior edge from {source:x} to {target:x}')


def require_allocator_index(db, selected=None):
    instructions = ([json.loads(row[0]) for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
        (CONTRACTS[0][0], CONTRACTS[-1][1]))] if selected is None
        else [item for _, item in sorted(selected.items())])
    require_allocator_bodies(instructions, complete=selected is None)
    require_allocator_edges(db.execute("""SELECT i.address, edge.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body, '$.flows'),
                                '$[#]', json_extract(i.body, '$.fallthrough'))) AS edge
        WHERE edge.value>=? AND edge.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def require_allocator_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in ALLOCATOR_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label = f'i_{entry:x}:'
        hook = f'if (try_modern_allocator(s, 0x{entry:x})) return true;'
        step = f's.step(0x{entry:x});'
        if code.count(hook) != 1 or label not in code:
            raise ValueError(f'Missing or duplicate allocator hook at {entry:x}')
        fragment = code.split(label, 1)[1].split(f'i_{entry + 4:x}:', 1)[0]
        if hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Misplaced allocator hook at {entry:x}')
