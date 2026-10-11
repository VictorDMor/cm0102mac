"""Pinned Mac file-controller contracts; production integration is separately verified."""
import hashlib
import json

CONTRACTS = (
    (0x10776690, 0x107766bc, 'ba3edc2c585acb2624ea9bb934f606b7de4e00e04973a377129f3eb2301c138b'),
    (0x107766c0, 0x107766ec, '1d8d5eadcd06343b8bfe33f30a1829cadfd7757d4380d5cb41b5415f3629389c'),
    (0x107768f0, 0x10776938, '8ab3acdbe6077f1ec0e767e2c0253a160a461cd988cc3cc5ffaeffafd82d2358'),
    (0x10776940, 0x10776a38, '384c4802d7a72ae6764340dc33b146a1e70faa71352801acb33d053488eb5bf2'),
    (0x10776a40, 0x10776bb8, '673a20e1b3d88ebbade4c16825a5a6bc8003115a2d050285e656b61ae9e9315b'),
    (0x10776bc0, 0x10776c24, 'e4dbdd0cbf9998dacd9866ba0cf0489b1a08fcd5b5bdb44a49b2647df7fcc12a'),
    (0x10776c30, 0x10776cc4, 'bd369995e774ec3ae7bedbad1f72644f112786cfd4588a0012dadb306b63b9f8'),
    (0x10776cd0, 0x10776d0c, 'ff2be9ab4d03a55c3ea769ff6beae7159605fc41ab31b91fa3201e34bc8d2305'),
    (0x10776d10, 0x10776d18, '4a180ecb9179e0e91c5fdbe0643f0d2426222d3788f1c922e1d3d351ed7280de'),
    (0x10776d20, 0x10776d54, 'ac0a1212235062fc978082e839dc961f85c41524c9c7e202b98f78c7679ecfea'),
    (0x10776d60, 0x10776d94, '15d94fdf1475d985a34ccfaad1fa9813fac45a32cbec5722e8e93ad61f3876a8'),
    (0x10776da0, 0x1077713c, 'c6a26e05478eba597a7951ff0d3bff68320d18108e9c0aaf8381e6897c54229a'),
    (0x10777140, 0x107772e0, '07e4f377b5c56b33a14104be72d09444884bccf289d30ac94df9a965c6ebba01'),
    (0x107772e0, 0x10777430, '909ffabcdf3b5113664cfdcd7f2661dd6332d397f8753bc86462fdb0820a01bf'),
    (0x10777430, 0x107777cc, 'b68a5ff2ef041a78a4ff219b3f9f8d4c15b318180f5732a63ecc29729e13fefe'),
    (0x107777d0, 0x10777984, 'd9da77797dc71773672e085e614474e71ab8bcf531f2b5aa6912c36bb36b2621'),
    (0x10777990, 0x10777a58, 'fb27c9132bf77039473df8bedb3eb43b1ab473bc7f6d7e5957c41e838ac79f2d'),
    (0x10777a60, 0x10777c3c, '5b06a86e18d6d0ad87a9186ef9cf193cfde6644a642de83cb5c8e5683e162f06'),
    (0x10777c40, 0x10777cb4, '5667705450fd3665ef30970d5e0b1e0eb6c9b2cc92bc95ff9c6d471cf650b33a'),
    (0x10777cc0, 0x10777ce0, 'ef9e7984ba831876ee3e41f9fc4ecd9d80fa393c14bdfa7b178fa0974972ab48'),
    (0x10777ce0, 0x10777d20, 'f18b11ce6cae2c7996cd77cd7bc848cc95eac0226b93e618462919241df36069'),
    (0x10777d20, 0x10777f04, 'aa5a8e34e3bd093f231f628f03e439c573e3aa0ddb9fa89f1cd07a4ab53c7c36'),
    (0x10777f10, 0x10777f30, '61058fc2ba2223eba9ea3c2228c3e8f1b15049c2002a9601925f5346a34e954c'),
    (0x10777f30, 0x10777f58, '787c5bfc5728109c234a8466f50ac4510bb3ce52870ea83715ee42b7607a2f22'),
    (0x107784a0, 0x107784b0, '44e1db050b03f18af258e9ec331d994f4308404d1c2d651b9825e3c43695ccb7'),
    (0x107784b0, 0x107784c0, '064fc896aa81c3a44688f4312ee65511f902e566c6954c7265699680ecfddd44'),
)
FILE_CONTROLLER_ENTRIES = tuple(start for start, _, _ in CONTRACTS)
INSTRUCTION_COUNTS = (11, 11, 18, 62, 94, 25, 37, 13, 2, 13, 13, 231, 103, 83, 227, 109, 50, 119, 29, 8, 16, 121, 8, 10, 4, 4)
PAGE_FRAGMENTS = {
    (0x10776da0, 0x10776): (152, '7b66cc6cbbc7c50515c69ba451ece2b5b179e86e9bc8b9fae8846d9c5b26d0e6'),
    (0x10776da0, 0x10777): (79, '065a2b4dccbb6bbdf95303bbc2e31b4c9406d8c319cb87618719afa61b0dc789'),
}


def _require(records, expected, start):
    body = [{key: item.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')}
            for item in records]
    actual = hashlib.sha256(json.dumps(body, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if actual != expected:
        raise ValueError(f'Mac file_controller contract changed at {start:x}')


def require_file_controller_contract(page, instructions):
    for start, end, expected in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12:
            continue
        records = [r for r in instructions if start <= r['address'] < end]
        if records:
            fragment = PAGE_FRAGMENTS.get((start, page))
            if fragment:
                count, digest = fragment
                if len(records) != count:
                    raise ValueError(f'Mac file_controller fragment size changed at {start:x}/{page:x}')
                _require(records, digest, start)
            else:
                _require(records, expected, start)


def require_file_controller_bodies(instructions, complete=False):
    for start, end, expected in CONTRACTS:
        records = [r for r in instructions if start <= r['address'] < end]
        if complete and not records:
            raise ValueError(f'Missing complete Mac file_controller body at {start:x}')
        if records:
            _require(records, expected, start)


def require_file_controller_edges(edges):
    for source, target in edges:
        for start, end, _ in CONTRACTS:
            if start <= target < end and not start <= source < end and target != start:
                raise ValueError(f'External file_controller interior edge from {source:x} to {target:x}')


def require_file_controller_index(db, selected=None):
    instructions = ([json.loads(row[0]) for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
        (CONTRACTS[0][0], CONTRACTS[-1][1]))] if selected is None
        else [item for _, item in sorted(selected.items())])
    require_file_controller_bodies(instructions, complete=selected is None)
    require_file_controller_edges(db.execute("""SELECT i.address, edge.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body, '$.flows'),
                                '$[#]', json_extract(i.body, '$.fallthrough'))) AS edge
        WHERE edge.value>=? AND edge.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def require_file_controller_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in FILE_CONTROLLER_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        label = f'i_{entry:x}:'
        hook = f'if (try_modern_file_controller(s, 0x{entry:x})) return true;'
        step = f's.step(0x{entry:x});'
        if code.count(hook) != 1 or label not in code:
            raise ValueError(f'Missing or duplicate file_controller hook at {entry:x}')
        fragment = code.split(label, 1)[1].split(f'i_{entry + 4:x}:', 1)[0]
        if hook not in fragment or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f'Misplaced file_controller hook at {entry:x}')
