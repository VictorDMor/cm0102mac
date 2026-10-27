"""SCRMAN source census, distinct production ownership and original-only oracle."""
from contextlib import closing
import inspect
import json
from pathlib import Path
import re
import sqlite3
import unittest

from screen_manager_audit import MODULE, ROUTINES, rewritten, require_screen_manager_index, volatile_reads
from compile_recomp import generate_page, OWNER_REPLACEMENTS, SEMANTIC_REPLACEMENTS, SCREEN_MANAGER_TAKEOVERS

ROOT=Path(__file__).resolve().parent.parent
INPUT=ROOT/'.local/current/export'

@unittest.skipUnless((INPUT/'instructions.sqlite').exists(),'Private export unavailable')
class ScreenManagerTests(unittest.TestCase):
    def test_complete_module_inventory_and_digests(self):
        self.assertEqual(len(ROUTINES),85)
        self.assertEqual((ROUTINES[0][0],ROUTINES[-1][1]),MODULE)
        self.assertTrue(all(a[1]==b[0] for a,b in zip(ROUTINES,ROUTINES[1:])))
        self.assertEqual([(a,b) for a,b,_ in rewritten()],list(ROUTINES))
        with closing(sqlite3.connect(INPUT/'instructions.sqlite')) as db:
            require_screen_manager_index(db)
            total=db.execute('select count(*) from instructions where address>=? and address<?',MODULE).fetchone()[0]
            self.assertEqual(total,8946)
    def test_no_unmodelled_scratch_inputs(self):
        with closing(sqlite3.connect(INPUT/'instructions.sqlite')) as db:
            self.assertEqual(volatile_reads(db),[])
    def test_single_modern_owner_and_24_superseded_owners(self):
        self.assertEqual(len(SEMANTIC_REPLACEMENTS),963)
        self.assertEqual(len(SCREEN_MANAGER_TAKEOVERS),24)
        entries={a for a,_ in ROUTINES}
        self.assertEqual([(a,m) for a,m in OWNER_REPLACEMENTS if a in entries],
                         [(a,'screen_manager_adapter') for a,_ in ROUTINES])
    def test_hooks_replace_prior_owners_and_reference_disables_all(self):
        with closing(sqlite3.connect(INPUT/'instructions.sqlite')) as db:
            records=[json.loads(row[0]) for row in db.execute(
                'select body from instructions where address>=? and address<? order by address',MODULE)]
        userops=json.loads((INPUT/'program.json').read_text())['userops']
        selected={r['address'] for r in records}
        disabled={name:False for name in inspect.signature(generate_page).parameters
                  if name.endswith('_hooks') or name=='guest_context_transfers'}
        for page in sorted({a>>12 for a,_ in ROUTINES}):
            part=[r for r in records if r['address']>>12==page]
            code=generate_page(page,part,selected,userops=userops)
            for a,_ in ROUTINES:
                if a>>12!=page:continue
                fragment=code.split(f'i_{a:x}:',1)[1].split(f'i_{a+4:x}:',1)[0]
                hooks=re.findall(r'try_modern_(\w+)\(s,',fragment)
                self.assertEqual(hooks,['screen_manager'])
            reference=generate_page(page,part,selected,userops=userops,**disabled)
            self.assertNotIn('try_modern_',reference)
    def test_no_instruction_builder_or_original_body_fallback_in_algorithms(self):
        for path in (ROOT/'native').glob('screen_manager*.cpp'):
            if path.name.endswith(('_adapter.cpp','_checks.cpp')):continue
            code=path.read_text()
            for token in ('SemanticGuestCall','dispatch_page(','run_original_body(','goto i_'):
                self.assertNotIn(token,code,str(path))

if __name__=='__main__':unittest.main()
