"""Pinned complete shortlist assessment contribution and club-interest operations."""
import hashlib
import json
import struct

CONTRACTS = ((271979664, 271986028, 1591, '780884b29e2018964df4070d8456302c8a92c65f87683136ca97f45e75a8292c'), (272011392, 272028216, 4206, 'b79bc9142b35853155f86498bb41a590d1000b592304f1e5d6c269695cc02786'))
FRAGMENTS = ((271979664, 66401, 732, 'db3cbe2c560aad45db0fa29922d9ccec61aab6dedee26e037b29a86cd875d759'), (271979664, 66402, 859, '6c90587b033bcd41c735c1f57564711b43f8f71adf694697de88810d20f32889'), (272011392, 66409, 992, 'e7527bb9d8e68435725f166099dd5c1fe2f7134943f05f2bf311bc856c492209'), (272011392, 66410, 1024, 'db8f4ee43e66a35f3408f59c06ac30c7125fe2761c7eddb3aadd2e60cca85d72'), (272011392, 66411, 1024, '7f6e6c25f0b249c8fd38ba702d2cf75a801ce924c4691353b57f60c8e8fd10ef'), (272011392, 66412, 1024, 'fee93271d92096b76b105633626a5883cfdcaafec9ebee8bd81e48704dc6e5b0'), (272011392, 66413, 142, '33624efc09113324944d784086aab387f24e0edd8cbb171302c44fb7b0e50faa'))
SHORTLIST_ASSESSMENT_ENTRIES = tuple(start for start, _, _, _ in CONTRACTS)
PREPARED_SHORTLIST_ASSESSMENT_ENTRIES = SHORTLIST_ASSESSMENT_ENTRIES


def fingerprint(body):
    canonical = [{key: r.get(key) for key in ('address', 'assembly', 'ops', 'flows', 'fallthrough')} for r in body]
    return hashlib.sha256(json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def require_shortlist_assessment_bodies(instructions, complete=False):
    for start, end, count, expected in CONTRACTS:
        body = [r for r in instructions if start <= r['address'] < end]
        if not body and not complete: continue
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac shortlist-assessment contract changed or missing at {start:x}')


def require_shortlist_assessment_edges(edges):
    for source, target in edges:
        for start, end, _, _ in CONTRACTS:
            if start < target < end and not start <= source < end:
                raise ValueError(f'External shortlist-assessment interior edge from {source:x} to {target:x}')


def require_shortlist_assessment_contract(page, instructions, selected=None):
    addresses = {r['address'] for r in instructions} | set(selected or ())
    for start, end, _, _ in CONTRACTS:
        if not start >> 12 <= page <= (end - 1) >> 12: continue
        low, high = max(start, page << 12), min(end, (page + 1) << 12)
        if start not in addresses and not any(low <= a < high for a in addresses): continue
        body = [r for r in instructions if low <= r['address'] < high]
        _, _, count, expected = next(f for f in FRAGMENTS if f[0] == start and f[1] == page)
        if len(body) != count or fingerprint(body) != expected:
            raise ValueError(f'Mac shortlist-assessment contract changed or missing at {start:x} page {page:x}')


def require_shortlist_assessment_index(db, selected=None):
    records = [json.loads(row[0]) for start, end, _, _ in CONTRACTS for row in db.execute(
        'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
    if selected is None: require_shortlist_assessment_bodies(records, complete=True)
    else:
        for page in sorted({f[1] for f in FRAGMENTS}):
            require_shortlist_assessment_contract(page, [r for _, r in sorted(selected.items())], selected)
    conditions = ' OR '.join('(e.value>=? AND e.value<?)' for _ in CONTRACTS)
    parameters = [value for start, end, _, _ in CONTRACTS for value in (start, end)]
    require_shortlist_assessment_edges(db.execute("""SELECT i.address,e.value
        FROM instructions AS i,
          json_each(json_insert(json_extract(i.body,'$.flows'),'$[#]',json_extract(i.body,'$.fallthrough'))) AS e
        WHERE """ + conditions, parameters))


TABLES = ((277572788, 276871384, 1120, '7d9414e3ef27b88323cdea9988c7ac1a0c1ad02285bc710960afbdae5adb2885'), (277572776, 276871340, 44, 'bda6289428ddab89c1a899df20e094b3927f69442c9e95350e27aa2f04b110f4'), (277572772, 277671540, 88, 'eade0270d0fb8aa7360fd8656f18702e5560e17bd132b80fb021416cb07b2b6b'), (277572768, 277671440, 100, 'd1357635ca1e49fa724f5e2d2c7b29864298c0aca02eb31353dc23ff76275287'), (277572744, 277672016, 32, 'e313606757a6ac5b75c5f0d9b1bd28b5bf72a1b8520b8fd1cff2d824c9530afd'), (277572740, 277671984, 32, '44c13490be899b4b6c97cc450ec3a16e902c3ef8945b8f8c66b5472ef2a1ba01'), (277572736, 277671952, 32, '842ab23028b7307d56d2209e54f87bde55b3a80284e56e23056f17ce40b15a5c'))

def require_shortlist_assessment_tables(read):
    for slot,base,size,expected in TABLES:
        if struct.unpack(">I",read(slot,4))[0]!=base or hashlib.sha256(read(base,size)).hexdigest()!=expected:
            raise ValueError("Changed original shortlist assessment table")

def load_shortlist_assessment_tables(directory):
    program=json.loads((directory/"program.json").read_text())
    segments=[(s["address"],(directory/s["file"]).read_bytes()) for s in program["segments"] if s.get("file")]
    def read(a,n):
        for base,data in segments:
            if base<=a and a+n<=base+len(data):return data[a-base:a-base+n]
        raise ValueError("Missing original shortlist-assessment table")
    require_shortlist_assessment_tables(read)
    require_shortlist_assessment_padding(read)
    return tuple(read(base,size) for _,base,size,_ in TABLES)


PADDING = ((271986028, '00000000'), (272028216, '00000000'), (272028220, '00000000'))

def require_shortlist_assessment_padding(read):
    for address,expected in PADDING:
        if read(address,4)!=bytes.fromhex(expected):
            raise ValueError("Changed nonindexed assessment alignment word")


# Retain these original instructions in both bodies. They cannot be reached from
# either complete root while declared callees preserve the PowerPC nonvolatile ABI.
STRUCTURALLY_UNREACHABLE = (
    *range(0x10369dfc, 0x10369e18, 4), *range(0x10369e24, 0x10369e30, 4),
    0x1036bdd8, 0x1036bdfc, 0x1036be60, 0x1036c858,
    0x1036cffc, 0x1036d13c, 0x1036d140,
)


def require_shortlist_assessment_structural_proofs(instructions, read):
    """Validate the pinned source/data premises of four bounded invariants."""
    import re
    require_shortlist_assessment_bodies(instructions, complete=True)
    require_shortlist_assessment_tables(read)
    assembly = {r['address']: r['assembly'] for r in instructions}
    premises = {
        0x10369d64: 'lbz r3,0x4c(r27)', 0x10369d6c: 'stw r0,0x10c(r1)',
        0x10369d70: 'extsb r0,r3', 0x10369d94: 'fdiv f1,f2,f1',
        0x10369d98: 'fsub f1,f4,f1',
        0x1036b92c: 'cmplwi r24,0x0', 0x1036b930: 'bne 0x1036b9c4',
        0x1036b9c0: 'b 0x1036c408',
        0x1036c784: 'fcmpo cr0,f31,f5', 0x1036c788: 'ble 0x1036c894',
        0x1036c7f0: 'ble 0x1036c7fc', 0x1036c7f4: 'lfd f0,0x150(r16)',
        0x1036c818: 'fdiv f0,f1,f0', 0x1036c81c: 'fsub f1,f2,f0',
        0x1036c830: 'fdiv f0,f31,f0', 0x1036c834: 'fsub f1,f1,f0',
        0x1036c850: 'fcmpo cr0,f0,f1', 0x1036c854: 'bge 0x1036c85c',
        0x1036cf60: 'fcmpo cr0,f31,f0', 0x1036cf64: 'bge 0x1036cf70',
        0x1036cf6c: 'b 0x1036d21c', 0x1036cfe4: 'fcmpo cr0,f0,f31',
        0x1036d134: 'fcmpo cr0,f31,f0', 0x1036d138: 'bge 0x1036d144',
    }
    if any(assembly.get(pc) != text for pc, text in premises.items()):
        raise ValueError('Changed assessment structural-proof premise')
    bank = read(0x1080b8d8, 1120)
    constants = {offset: struct.unpack('>d', bank[offset:offset+8])[0]
                 for offset in (0x30,0x48,0x68,0x80,0x150,0x178,0x200,0x308,0x3a8)}
    if constants != {0x30:100.0,0x48:0.1,0x68:0.5,0x80:1.0,0x150:10.0,
                     0x178:1.5,0x200:1.1,0x308:400.0,0x3a8:1.75}:
        raise ValueError('Changed assessment bound constants')
    # Signed byte / 400: factor lies in [0.6825,1.32], strictly inside both guards.
    factors = [1.0 - value / 400.0 for value in range(-128,128)]
    if not (min(factors)>0.5 and max(factors)<1.5):
        raise ValueError('Invalid signed-byte assessment interval')
    # r24 is unchanged after the club-resolution prelude; zero takes the bypass.
    if any(0x1036b92c <= pc < 0x1036c408 and re.match(r'(?:li|or|lwz|addi|add) r24,', text)
           for pc,text in assembly.items()):
        raise ValueError('Changed stable current-club register')
    # Every lower-reference factor is 1.1 - x/100 with x >= 10, rounded to float.
    # NaN takes the preceding unordered exit, and +infinity cannot exceed one.
    # The original f31 > 10 guard and the other arm's floor of 10 are pinned above.
    if struct.unpack('>f',struct.pack('>f',1.1-10.0/100.0))[0] != 1.0:
        raise ValueError('Invalid lower-reference ceiling')
    # The refinement path requires f31 >= 1.75; neither body nor ABI-preserving
    # dependencies writes f31 before the two redundant later comparisons.
    if any(0x1036cf70 <= pc <= 0x1036d140 and re.match(r'f\w+ f31,', text)
           for pc,text in assembly.items()):
        raise ValueError('Changed stable refinement score')
    return STRUCTURALLY_UNREACHABLE


def load_shortlist_assessment_structural_proofs(directory, instructions):
    program=json.loads((directory/'program.json').read_text())
    segments=[(s['address'],(directory/s['file']).read_bytes()) for s in program['segments'] if s.get('file')]
    def read(address,size):
        for base,data in segments:
            if base<=address and address+size<=base+len(data):return data[address-base:address-base+size]
        raise ValueError('Missing original structural-proof data')
    return require_shortlist_assessment_structural_proofs(instructions,read)
