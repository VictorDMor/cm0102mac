"""Check original module bounds, production ownership and observable registers."""
from contextlib import closing
import json
from pathlib import Path
import sqlite3
import unittest
from club_history_audit import MODULE, ROUTINES, rewritten, require_club_history_index, volatile_reads
ROOT=Path(__file__).resolve().parent.parent
INPUT=ROOT/'.local/current/export/instructions.sqlite'
@unittest.skipUnless(INPUT.exists(),'Private export unavailable')
class ClubHistoryTests(unittest.TestCase):
    def test_complete_original_inventory(self):
        self.assertEqual(len(ROUTINES),11)
        self.assertEqual((ROUTINES[0][0],ROUTINES[-1][1]),MODULE)
        self.assertTrue(all(a[1]==b[0] for a,b in zip(ROUTINES,ROUTINES[1:])))
        self.assertEqual([(a,b) for a,b,_ in rewritten()],list(ROUTINES))
        with closing(sqlite3.connect(INPUT)) as db: require_club_history_index(db)
    def test_no_unmodelled_scratch_register_dependencies(self):
        with closing(sqlite3.connect(INPUT)) as db: self.assertEqual(volatile_reads(db),[])
    def test_virtual_dispatch_supplies_original_r12(self):
        with closing(sqlite3.connect(INPUT)) as db:
            for site,offset in ((0x1000d73c,'0x14'),(0x1000e570,'0x18')):
                body=json.loads(db.execute('select body from instructions where address=?',(site-4,)).fetchone()[0])
                self.assertEqual(body['assembly'],f'lwz r12,{offset}(r12)')
if __name__=='__main__': unittest.main()
