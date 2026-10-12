"""Pinned complete actor spatial membership and tactical role boundaries."""
import hashlib
import json

CONTRACTS = (
    (0x101d3b50, 0x101d3b58, 2, '670a10fdc888d2fc648ba28b27e5f68c435ffd7413b52c169eaf56a395f6855a'),
    (0x101d5680, 0x101d5688, 2, '2dcaae74ca2b622a8066e4a9f779e28d50ce28322774f5c96c068b9546c86f88'),
    (0x101d5690, 0x101d5698, 2, '8c41e358173a6fae606b40b9555fa4a3a399852a26f8a7b77c37c82923850952'),
    (0x101d56c0, 0x101d56c8, 2, '75df1dd60ea57318cebac9c747549cb79d108d56aa2e3e41f59641475797f952'),
    (0x101d5730, 0x101d5738, 2, 'b70d808b3a47cfbff86b73fb5c3a66fd347bbe58bac8bedbc03f8adc956658d8'),
    (0x101d5740, 0x101d574c, 3, '02d4a1fe9a5425bdb44ccf6471b7269952b06865ec5829b82186f0fd99afb2c6'),
    (0x101d7db0, 0x101d7db8, 2, '08d1cbb3c6ca17e04d6ecd2d5f40c0a5fc726ca5102eabb8408c514f5f170aba'),
    (0x101e3d50, 0x101e3ddc, 35, '1a920cd68083655d60dc3fe51b0eafd576b705d3104835f3ab50f10579e126cf'),
    (0x101e3de0, 0x101e3e6c, 35, '9b0ba046f6d1bf8ff6fb82855671bea9c862af7c586b3a0fd41e62c4a0205c3c'),
    (0x101e3e70, 0x101e3efc, 35, 'cb19afaef4bfd88c81fd7ceebcca53685b26eb0cfedb57e15d322e363cd84db6'),
    (0x101e3f00, 0x101e3f08, 2, '4fbbdaa34c2e2e698fdd20fa544ebec7c753827194d047a434cd46608b1d85d7'),
    (0x101f1880, 0x101f192c, 43, 'a3b1ea80d1ecdca5e5593bd0df28496d1059e96dd9df2c266702e4c7181c4b49'),
    (0x101f1930, 0x101f1938, 2, '1d64ee4abdbb46335eedde091ba5ae2ff63a14a6b5fa6a2827e635609a359708'),
    (0x101f1940, 0x101f1948, 2, 'd83d0aee46addc9222cff8bdef520ea529bf9d7a8f47ce89b9d1291ccfc6b01d'),
    (0x101f1950, 0x101f1dc0, 284, 'bb9bab8e46365b638e43d720891ecd729d9c01a46e0a4d736a33cfde598d36f2'),
    (0x101f1dc0, 0x101f1dd4, 5, '8df9e897e8ee3535f15cfce411dd3a7c5690795a4b65bd143c960bd6fa8a82b7'),
    (0x101f1de0, 0x101f1e0c, 11, '77ad9c261d8d131f1bd97633cb324bb50f5be355c8ebdca3cb60cd87feb3d1c7'),
    (0x101f1e10, 0x101f21c0, 236, '33102b6956e2cc1a38d6b4ac991830a72c810bfc5427ace7d8f2ac87e3e1e859'),
    (0x101f21c0, 0x101f21c8, 2, '58a65554f67fbb98e1b92cce737f399bc8cbc813178f28bac4d5e0ae61dc1942'),
    (0x10289550, 0x10289558, 2, '830067b240b6c98bc9f5289316f7b19069ec8bb4fac02a14db01c618fbd6bec8'),
    (0x10289560, 0x1028959c, 15, '7a0d1c3dc61bb7d5a144ab583941358f10aa4f1d65fc164b02c8cb42c100663e'),
    (0x102895a0, 0x102895dc, 15, '9d3a8ce9ed3e69f88d9d5261e752ca0fc170f9a063e45c74589272d28ff349bb'),
    (0x102895e0, 0x1028961c, 15, 'd44a59e901861084194a8bbe6e3ee4b2c45a60d441a6a4f13b5a8242844c719d'),
)
FRAGMENTS = (
    (0x101d3b50, 0x101d3, 2, '670a10fdc888d2fc648ba28b27e5f68c435ffd7413b52c169eaf56a395f6855a'),
    (0x101d5680, 0x101d5, 2, '2dcaae74ca2b622a8066e4a9f779e28d50ce28322774f5c96c068b9546c86f88'),
    (0x101d5690, 0x101d5, 2, '8c41e358173a6fae606b40b9555fa4a3a399852a26f8a7b77c37c82923850952'),
    (0x101d56c0, 0x101d5, 2, '75df1dd60ea57318cebac9c747549cb79d108d56aa2e3e41f59641475797f952'),
    (0x101d5730, 0x101d5, 2, 'b70d808b3a47cfbff86b73fb5c3a66fd347bbe58bac8bedbc03f8adc956658d8'),
    (0x101d5740, 0x101d5, 3, '02d4a1fe9a5425bdb44ccf6471b7269952b06865ec5829b82186f0fd99afb2c6'),
    (0x101d7db0, 0x101d7, 2, '08d1cbb3c6ca17e04d6ecd2d5f40c0a5fc726ca5102eabb8408c514f5f170aba'),
    (0x101e3d50, 0x101e3, 35, '1a920cd68083655d60dc3fe51b0eafd576b705d3104835f3ab50f10579e126cf'),
    (0x101e3de0, 0x101e3, 35, '9b0ba046f6d1bf8ff6fb82855671bea9c862af7c586b3a0fd41e62c4a0205c3c'),
    (0x101e3e70, 0x101e3, 35, 'cb19afaef4bfd88c81fd7ceebcca53685b26eb0cfedb57e15d322e363cd84db6'),
    (0x101e3f00, 0x101e3, 2, '4fbbdaa34c2e2e698fdd20fa544ebec7c753827194d047a434cd46608b1d85d7'),
    (0x101f1880, 0x101f1, 43, 'a3b1ea80d1ecdca5e5593bd0df28496d1059e96dd9df2c266702e4c7181c4b49'),
    (0x101f1930, 0x101f1, 2, '1d64ee4abdbb46335eedde091ba5ae2ff63a14a6b5fa6a2827e635609a359708'),
    (0x101f1940, 0x101f1, 2, 'd83d0aee46addc9222cff8bdef520ea529bf9d7a8f47ce89b9d1291ccfc6b01d'),
    (0x101f1950, 0x101f1, 284, 'bb9bab8e46365b638e43d720891ecd729d9c01a46e0a4d736a33cfde598d36f2'),
    (0x101f1dc0, 0x101f1, 5, '8df9e897e8ee3535f15cfce411dd3a7c5690795a4b65bd143c960bd6fa8a82b7'),
    (0x101f1de0, 0x101f1, 11, '77ad9c261d8d131f1bd97633cb324bb50f5be355c8ebdca3cb60cd87feb3d1c7'),
    (0x101f1e10, 0x101f1, 124, '6d02ee50ee48a5c6889d6589d7f50376f6f3278c9ad5c1a0cd2c7a1d24d637d2'),
    (0x101f1e10, 0x101f2, 112, '0751c7e838e8152b21aa090db66bb620feb7c381947825da6456731e77576345'),
    (0x101f21c0, 0x101f2, 2, '58a65554f67fbb98e1b92cce737f399bc8cbc813178f28bac4d5e0ae61dc1942'),
    (0x10289550, 0x10289, 2, '830067b240b6c98bc9f5289316f7b19069ec8bb4fac02a14db01c618fbd6bec8'),
    (0x10289560, 0x10289, 15, '7a0d1c3dc61bb7d5a144ab583941358f10aa4f1d65fc164b02c8cb42c100663e'),
    (0x102895a0, 0x10289, 15, '9d3a8ce9ed3e69f88d9d5261e752ca0fc170f9a063e45c74589272d28ff349bb'),
    (0x102895e0, 0x10289, 15, 'd44a59e901861084194a8bbe6e3ee4b2c45a60d441a6a4f13b5a8242844c719d'),
)
SPATIAL_ROLE_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_spatial_role_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac spatial-role contract changed or missing at {start:x}')


def require_spatial_role_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External spatial-role interior edge from {source:x} to {target:x}')


def require_spatial_role_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac spatial-role contract changed or missing at {start:x} page {page:x}')


def require_spatial_role_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_spatial_role_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_spatial_role_contract(page, [r for _, r in sorted(selected.items())], selected)
    require_spatial_role_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE e.value>=? AND e.value<?""", (CONTRACTS[0][0], CONTRACTS[-1][1])))


def spatial_role_hook(entry):
    return f'if (try_modern_spatial_role(s, 0x{entry:x})) return true;'


def require_spatial_role_hooks(page, instructions, code):
    addresses = {r['address'] for r in instructions}
    for entry in SPATIAL_ROLE_ENTRIES:
        if entry >> 12 != page or entry not in addresses: continue
        label, hook = f'i_{entry:x}:', spatial_role_hook(entry)
        fragment = code.split(label, 1)[-1].split(f'i_{entry + 4:x}:', 1)[0]
        steps = [token for token in (f's.step(0x{entry:x});', f'counter.step(0x{entry:x});') if token in fragment]
        if code.count(hook) != 1 or hook not in fragment or len(steps) != 1 or fragment.index(hook) > fragment.index(steps[0]):
            raise ValueError(f'Missing or misplaced spatial-role hook at {entry:x}')
