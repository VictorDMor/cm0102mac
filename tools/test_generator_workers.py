"""Parallel generation must keep every audit and match one-process generation.

The private export is required evidence, rather than a reason to skip tests.
"""
from concurrent.futures.process import BrokenProcessPool
from contextlib import closing
import os
from pathlib import Path
import re
import sqlite3
import tempfile
import unittest

import compile_recomp as generator

EXPORT = Path(__file__).resolve().parent.parent / ".local/current/export"
# A page with owner hooks and direct owner calls, a page with fused sites and the program's first.
PAGES = (0x100ee, 0x100f0, 0x10000)


def rejecting_audit(db, selected=None):
    raise ValueError("audit rejected the export")


def dying_audit(db, selected=None):
    os._exit(1)


def passing_audit(db, selected=None):
    db.execute("SELECT 1").fetchone()


class GeneratorWorkerTests(unittest.TestCase):
    def setUp(self):
        if not (EXPORT / "instructions.sqlite").is_file():
            raise RuntimeError("Private export unavailable; generation cannot be certified")

    def test_a_rejecting_audit_stops_generation(self):
        with generator.generator_pool(2, EXPORT, None) as pool:
            with self.assertRaisesRegex(ValueError, "audit rejected the export"):
                generator.run_index_audits(pool, generator.run_selectable_index_audit,
                                           (passing_audit, rejecting_audit, passing_audit))

    def test_a_dead_worker_stops_generation_rather_than_hanging(self):
        with self.assertRaises(BrokenProcessPool):
            with generator.generator_pool(2, EXPORT, None) as pool:
                generator.run_index_audits(pool, generator.run_selectable_index_audit, (passing_audit, dying_audit))

    def test_every_index_audit_the_generator_imports_runs(self):
        source = Path(generator.__file__).read_text()
        imported = {name for name in vars(generator) if re.fullmatch(r"require_\w+_index", name)}
        listed = {audit.__name__ for audit in (*generator.SELECTABLE_INDEX_AUDITS, *generator.INDEX_AUDITS)}
        self.assertEqual(imported - listed, set())
        self.assertIn(generator.require_list_search, generator.INDEX_AUDITS)
        self.assertEqual(source.count("run_index_audits(pool, run_selectable_index_audit, SELECTABLE_INDEX_AUDITS)"), 1)
        self.assertEqual(source.count("run_index_audits(pool, run_index_audit, INDEX_AUDITS)"), 1)

    def test_workers_match_one_process_generation(self):
        with closing(sqlite3.connect(f"file:{EXPORT / 'instructions.sqlite'}?mode=ro", uri=True)) as db:
            divisions = generator.quotient_free_divisions(db)
            leaves = generator.inline_leaves(db, divisions)
            pages = {page: [generator.json.loads(row[0]) for row in db.execute(
                "SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address",
                (page << 12, (page + 1) << 12))] for page in PAGES}
        metadata = generator.json.loads((EXPORT / "program.json").read_text())
        hooks = {f"{family}_hooks": False
                 for family in generator.GENERATED_FASTER_FAMILIES + generator.SHORTLIST_MANAGER_FAMILIES}
        options = dict(local_temporaries=True, userops=metadata["userops"], nontrapping_divisions=divisions, **hooks)
        inlining, direct_calls = generator.Inlining(leaves), generator.DirectOwnerCalls()
        serial = {page: generator.generate_page(page, records, {r["address"] for r in records}, block_accounting=True,
                                                inlining=inlining, direct_owner_calls=direct_calls, **options)
                  for page, records in pages.items()}
        with tempfile.TemporaryDirectory() as output:
            with generator.generator_pool(2, EXPORT, None, leaves, (Path(output), options)) as pool:
                results = list(pool.map(generator.generate_indexed_page, PAGES))
            for page, result in zip(PAGES, results):
                self.assertEqual((Path(output) / result["name"]).read_text(), serial[page])
                self.assertEqual(result["count"], len(pages[page]))
        merged = generator.Inlining(leaves), generator.DirectOwnerCalls()
        for result in results:
            merged[0].inlined.update(result["inlined"])
            merged[0].special.update(result["special"])
            merged[1].called.update(result["called"])
            merged[1].entry_hooks.update(result["entry_hooks"])
        self.assertEqual(merged[0].inlined, inlining.inlined)
        self.assertEqual(merged[0].special, inlining.special)
        self.assertEqual(merged[1].called, direct_calls.called)
        self.assertEqual(merged[1].entry_hooks, direct_calls.entry_hooks)
        self.assertTrue(merged[1].entry_hooks or merged[1].called, "these pages should exercise the merge")
        # What the parent checks after the merge still fails on a lost hook.
        callee = "owner_family"
        merged[1].called[0x100] = callee
        with self.assertRaisesRegex(ValueError, "standard owner_family hook"):
            merged[1].require_standard()


if __name__ == "__main__":
    unittest.main()
