"""Whole twelve-entry preference lifecycle and productive integration contracts."""
import inspect
from compile_recomp import generate_page
from contextlib import closing
from copy import deepcopy
import hashlib, json, sqlite3, unittest
from pathlib import Path
from compile_recomp import SEMANTIC_REPLACEMENTS, require_unique_semantic_replacements
from preferences_audit import (CONTRACTS, FRAGMENTS, PREFERENCES_ENTRIES, TABLES, PADDING,
    load_preferences_tables, require_preferences_tables, require_preferences_padding,
    require_preferences_bodies, require_preferences_contract, require_preferences_edges, require_preferences_index,require_preferences_hooks,preferences_hook)
from make_preferences_reference import render_reference

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/compiler-arithmetic-reconstruction/input'


class PreparedScopeTests(unittest.TestCase):
    def test_exact_884_prefix_and_thirteen_productive_owners(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),958)
        prefix=[{'entry':hex(e),'module':m}for e,m in SEMANTIC_REPLACEMENTS[:884]]
        self.assertEqual(hashlib.sha256(json.dumps(prefix,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'7a8921cdc98b39bccdab14d402aa2c570b036df8041f4d7b9035762c328ba493')
        from money_configuration_audit import MONEY_CONFIGURATION_ENTRIES
        from preferences_audit import PREFERENCES_ENTRIES
        expected=tuple((e,m+'_adapter')for m,es in [('money_configuration',MONEY_CONFIGURATION_ENTRIES),('preferences',PREFERENCES_ENTRIES)]for e in es)
        self.assertEqual(SEMANTIC_REPLACEMENTS[884:897],expected);require_unique_semantic_replacements(SEMANTIC_REPLACEMENTS)
        for entry,_ in expected:
            with self.assertRaises(ValueError):require_unique_semantic_replacements((*SEMANTIC_REPLACEMENTS,(entry,'duplicate')))

    def test_whole_operations_and_interior_edge_rejection(self):
        self.assertEqual(len(CONTRACTS), 12)
        self.assertEqual(tuple(x[2] for x in CONTRACTS), (245,45,37,67,310,57,43,61,43,521,2,13))
        self.assertEqual(sum(x[2] for x in FRAGMENTS), 1444)
        self.assertEqual(tuple(x[2] for x in TABLES), (262,4,1,4,4,4,4,1,1))
        for start, end, *_ in CONTRACTS:
            require_preferences_edges([(0, start), (start, end-4), (end-4, end)])
            for target in {start+4, end-4} - {start}:
                with self.assertRaises(ValueError):
                    require_preferences_edges([(0, target)])

    def test_named_semantic_stages_without_runtime_replay(self):
        names = ('preferences_lifecycle.cpp', 'preferences_options.cpp', 'preferences_setters.cpp', 'preferences_apply.cpp')
        source = ''.join((ROOT / 'native' / name).read_text() for name in names)
        for token in ('dispatch_page', 'run_until', 'switch (s.pc)', 'goto ', '_reference(s)',
                      'pcode', 'while(s.pc', 'memory.clear', 'memset', 'GAME_CONFIG.CPP'):
            self.assertNotIn(token, source)
        for token in ('initialize_preferences', 'copy_preferences', 'destroy_preferences', 'set_defaults',
                      'choose_option', 'select_audio_device_group', 'reject_option', 'apply_preferences',
                      'set_interval', 'currency_for_language'):
            self.assertIn(token, source)
        self.assertIn('preferences_adapter.cpp', (ROOT / 'native/CMakeLists.txt').read_text())


@unittest.skipUnless((INPUT / 'instructions.sqlite').is_file(), 'Private original source unavailable')
class OriginalContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with closing(sqlite3.connect((INPUT/'instructions.sqlite').resolve().as_uri()+'?mode=ro', uri=True)) as db:
            require_preferences_index(db)
            cls.records = [json.loads(row[0]) for start, end, *_ in CONTRACTS for row in db.execute(
                'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address', (start, end))]
        program = json.loads((INPUT/'program.json').read_text())
        cls.userops = program['userops']
        cls.segments = [(s['address'], (INPUT/s['file']).read_bytes() if s.get('file') else bytes(s['size'])) for s in program['segments']]

    @classmethod
    def read(cls, a, n):
        for base, data in cls.segments:
            if base <= a and a+n <= base+len(data):
                return data[a-base:a-base+n]
        raise ValueError('Missing original preference data')

    def test_initialization_pointers_and_every_bank_byte_are_pinned(self):
        require_preferences_tables(self.read)
        self.assertEqual(len(load_preferences_tables(INPUT)), 9)
        for slot, base, size, _ in TABLES:
            for address in [*range(slot,slot+4), *range(base,base+size)]:
                def changed(a, n):
                    b = bytearray(self.read(a,n))
                    if a <= address < a+n:
                        b[address-a] ^= 1
                    return bytes(b)
                with self.subTest(address=hex(address)), self.assertRaises(ValueError):
                    require_preferences_tables(changed)

    def test_alignment_is_pinned_and_not_claimed_as_instructions(self):
        require_preferences_padding(self.read)
        self.assertFalse({a for a,_ in PADDING} & {r['address'] for r in self.records})
        for address, _ in PADDING:
            def changed(a,n):
                b = bytearray(self.read(a,n))
                if a <= address < a+n:
                    b[address-a] ^= 1
                return bytes(b)
            with self.assertRaises(ValueError):
                require_preferences_padding(changed)

    def test_productive_hooks_precede_original_accounting(self):
        disabled={name:False for name in inspect.signature(generate_page).parameters if name.endswith('_hooks') or name=='guest_context_transfers'}
        disabled['preferences_hooks']=True
        for page in sorted({f[1] for f in FRAGMENTS}):
            rows=[r for r in self.records if r['address']>>12==page]
            source=generate_page(page,rows,{r['address'] for r in self.records},userops=self.userops,**disabled)
            require_preferences_hooks(page,rows,source)
            for entry in PREFERENCES_ENTRIES:
                if entry>>12!=page:continue
                hook=preferences_hook(entry);step=f's.step(0x{entry:x});'
                for changed in (source.replace(hook,'',1),source.replace(hook,'',1).replace(step,step+'\n'+hook,1),source.replace(hook,hook+'\n'+hook,1)):
                    with self.assertRaises(ValueError):require_preferences_hooks(page,rows,changed)

    def test_complete_reference_with_all_hooks_disabled(self):
        source = render_reference(self.records,self.userops)
        self.assertNotIn('try_modern_',source)
        self.assertNotIn('_impl<true,',source)
        for entry in PREFERENCES_ENTRIES:
            self.assertIn(f'case 0x{entry:x}:',source)
        for row in self.records:
            self.assertIn(f's.step(0x{row["address"]:x});',source)

    def test_changed_instruction_fields_and_partial_pages_fail(self):
        for start,page,*_ in FRAGMENTS:
            i = next(i for i,r in enumerate(self.records) if r['address']>=start and r['address']>>12==page)
            for key,value in (('address',-1),('assembly','changed'),('ops',[]),('flows',[-1]),('fallthrough',-1)):
                changed = deepcopy(self.records)
                changed[i][key] = value
                with self.subTest(start=hex(start),page=hex(page),key=key), self.assertRaises(ValueError):
                    require_preferences_bodies(changed,complete=True)
            rows = [r for r in self.records if r['address']>>12==page]
            require_preferences_contract(page,rows,{r['address'] for r in self.records})
            with self.assertRaises(ValueError):
                require_preferences_contract(page,[r for r in rows if r['address']!=self.records[i]['address']],{start})

    def test_missing_duplicate_reordered_or_absent_bodies_fail(self):
        for start,end,*_ in CONTRACTS:
            i = next(i for i,r in enumerate(self.records) if r['address']==start)
            for changed in (self.records[:i]+self.records[i+1:],self.records+[self.records[i]],list(reversed(self.records)),
                            [r for r in self.records if not start<=r['address']<end]):
                with self.subTest(start=hex(start)), self.assertRaises(ValueError):
                    render_reference(changed,self.userops)


if __name__ == '__main__':
    unittest.main()
