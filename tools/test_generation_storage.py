"""Storage isolation, failed generation and genuine CLI regeneration checks."""
import errno
import hashlib
import json
from pathlib import Path
import sqlite3
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import Mock, patch

import generation_storage
from generation_storage import seed_generated
from formatted_output_audit import SHARED_EDGE


class StorageTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        # macOS's temporary directory is exposed through the /var symlink.
        # Use its canonical path so these tests still reject user-supplied aliases.
        self.root = Path(self.temporary.name).resolve()
        self.source = self.root / 'previous'
        self.output = self.root / 'next'
        self.source.mkdir()
        self.manifest = dict(files=['page_20000.cpp', 'program.cpp'], pages=1,
                             instructions=1, missingStaticTargets=[], pendingTargets=[])
        (self.source / 'page_20000.cpp').write_text('previous page\n' * 8192)
        (self.source / 'program.cpp').write_text('previous loader\n')
        self.publish()

    def publish(self):
        (self.source / 'compilation.json').write_text(json.dumps(self.manifest))

    def inventory(self):
        return {p.name: (hashlib.sha256(p.read_bytes()).hexdigest(), p.stat().st_ino, p.stat().st_mtime_ns)
                for p in self.source.iterdir() if p.is_file()}

    def test_seed_has_independent_inodes_and_writes_preserve_source(self):
        before = self.inventory()
        (self.source / 'private-reference.txt').write_text('must not be copied')
        report = seed_generated(self.source, self.output)
        self.assertEqual(set(p.name for p in self.output.iterdir()), set(self.manifest['files']))
        self.assertEqual(report['clonedFiles'] + report['copiedFiles'], 2)
        self.assertEqual(report['sourceBytes'], sum((self.source / p).stat().st_size for p in self.manifest['files']))
        for name in self.manifest['files']:
            self.assertNotEqual((self.source / name).stat().st_ino, (self.output / name).stat().st_ino)
            self.assertEqual((self.source / name).read_bytes(), (self.output / name).read_bytes())
            (self.output / name).write_text('new generation')
        for name, info in before.items():
            self.assertEqual(self.inventory()[name], info)
        self.assertFalse((self.output / 'compilation.json').exists())

    def test_regular_copy_fallback_keeps_the_same_isolation(self):
        before = self.inventory()
        with patch.object(generation_storage.sys, 'platform', 'linux'):
            report = seed_generated(self.source, self.output)
        self.assertEqual(report['clonedFiles'], 0)
        self.assertEqual(report['copiedFiles'], 2)
        (self.output / 'page_20000.cpp').write_text('replacement')
        self.assertEqual(self.inventory(), before)

    def test_unsupported_filesystem_uses_independent_regular_copies(self):
        library = Mock()
        library.clonefile.return_value = -1
        with patch.object(generation_storage.sys, 'platform', 'darwin'), \
                patch.object(generation_storage.ctypes, 'CDLL', return_value=library), \
                patch.object(generation_storage.ctypes, 'get_errno', return_value=errno.ENOTSUP):
            report = seed_generated(self.source, self.output)
        self.assertEqual(report['copiedFiles'], 2)
        self.assertEqual(report['clonedFiles'], 0)
        for name in self.manifest['files']:
            self.assertEqual((self.source / name).read_bytes(), (self.output / name).read_bytes())
            self.assertNotEqual((self.source / name).stat().st_ino, (self.output / name).stat().st_ino)

    def test_clone_errors_are_not_hidden_by_fallback_copy(self):
        before = self.inventory()
        library = Mock()
        library.clonefile.return_value = -1
        for error in (errno.EACCES, errno.ENOSPC, errno.EEXIST):
            with self.subTest(error=error), \
                    patch.object(generation_storage.sys, 'platform', 'darwin'), \
                    patch.object(generation_storage.ctypes, 'CDLL', return_value=library), \
                    patch.object(generation_storage.ctypes, 'get_errno', return_value=error):
                with self.assertRaises(OSError) as raised: seed_generated(self.source, self.output)
                self.assertEqual(raised.exception.errno, error)
                self.assertEqual(list(self.output.iterdir()), [])
                self.assertEqual(self.inventory(), before)

    def test_existing_output_is_never_overwritten(self):
        self.output.mkdir()
        (self.output / 'important.txt').write_text('keep')
        with self.assertRaisesRegex(ValueError, 'empty destination'):
            seed_generated(self.source, self.output)
        self.assertEqual((self.output / 'important.txt').read_text(), 'keep')
        self.assertEqual(len(list(self.output.iterdir())), 1)

    def test_invalid_manifest_fails_before_creating_destination(self):
        for change in (
                {'files': ['page_20000.cpp', '../program.cpp']},
                {'files': ['page_20000.cpp', '/program.cpp']},
                {'files': ['page_20000.cpp', 'private.txt']},
                {'files': ['page_20000.cpp', 'page_20000.cpp', 'program.cpp'], 'pages': 2},
                {'files': ['page_20000.cpp']}, {'pages': True}, {'pages': 2},
                {'instructions': 0}, {'missingStaticTargets': [1]}, {'pendingTargets': [1]},
                {'missingStaticTargets': None}, {'files': None}):
            with self.subTest(change=change):
                candidate = self.manifest | change
                (self.source / 'compilation.json').write_text(json.dumps(candidate))
                with self.assertRaises(ValueError): seed_generated(self.source, self.output)
                self.assertFalse(self.output.exists())

    def test_missing_source_fails_before_any_seed(self):
        (self.source / 'program.cpp').unlink()
        with self.assertRaises(FileNotFoundError): seed_generated(self.source, self.output)
        self.assertFalse(self.output.exists())

    def test_manifest_and_cpp_symlinks_are_rejected(self):
        for name in ('compilation.json', 'program.cpp'):
            with self.subTest(name=name):
                real = self.root / ('real-' + name)
                (self.source / name).rename(real)
                (self.source / name).symlink_to(real)
                with self.assertRaises(ValueError): seed_generated(self.source, self.output)
                self.assertFalse(self.output.exists())
                (self.source / name).unlink()
                real.rename(self.source / name)

    def test_directory_aliases_and_parent_traversal_are_rejected(self):
        alias = self.root / 'alias'
        alias.symlink_to(self.source, target_is_directory=True)
        for source, output in ((alias, self.output), (self.source, alias / 'next'),
                               (self.source, self.source), (self.source, self.source / 'next'),
                               (self.source, self.root), (self.source, self.root / 'unused' / '..' / 'next')):
            with self.subTest(source=source, output=output), self.assertRaises(ValueError):
                seed_generated(source, output)
        self.assertFalse(self.output.exists())

    def test_failed_seed_has_no_completion_and_keeps_references(self):
        before = self.inventory()
        copy = generation_storage._copy_independent
        def out_of_space(source, destination):
            if source.name == 'program.cpp': raise OSError(errno.ENOSPC, 'controlled disk-full failure')
            return copy(source, destination)
        with patch.object(generation_storage, '_copy_independent', out_of_space):
            with self.assertRaises(OSError): seed_generated(self.source, self.output)
        self.assertTrue((self.output / 'page_20000.cpp').exists())
        self.assertFalse((self.output / 'compilation.json').exists())
        self.assertEqual(self.inventory(), before)

    def test_changed_source_manifest_invalidates_seed(self):
        copy = generation_storage._copy_independent
        def change_manifest(source, destination):
            result = copy(source, destination)
            with (self.source / 'compilation.json').open('a') as stream: stream.write('\n')
            return result
        with patch.object(generation_storage, '_copy_independent', change_manifest):
            with self.assertRaisesRegex(ValueError, 'manifest changed'):
                seed_generated(self.source, self.output)
        self.assertFalse((self.output / 'compilation.json').exists())

    def export(self, broken=False):
        export = self.root / 'input'
        export.mkdir()
        digest = '91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b'
        metadata = dict(language='PowerPC:BE:32:default', executableSHA256=digest,
                        symbols=[], segments=[], userops=[])
        (export / 'program.json').write_text(json.dumps(metadata))
        record = dict(address=0x20000000, size=4, assembly='blr', flows=[],
                      ops=[['INVALID' if broken else 'RETURN', None, ['register', '4128', 4]]])
        with sqlite3.connect(export / 'instructions.sqlite') as db:
            db.execute('CREATE TABLE metadata(key TEXT, value TEXT)')
            db.execute('INSERT INTO metadata VALUES (?,?)', ('executableSHA256', digest))
            db.execute('CREATE TABLE instructions(address INTEGER PRIMARY KEY, body TEXT)')
            db.execute('INSERT INTO instructions VALUES (?,?)', (record['address'], json.dumps(record)))
            # The global index audit requires this unrelated edge annotation;
            # it is outside our selected synthetic root and is never generated.
            annotation = dict(address=SHARED_EDGE[0], size=4, assembly='synthetic annotation',
                              ops=[], flows=[SHARED_EDGE[1]])
            db.execute('INSERT INTO instructions VALUES (?,?)', (annotation['address'], json.dumps(annotation)))
        return export

    def generate(self, export, output, reuse=None):
        command = [sys.executable, str(Path(__file__).with_name('compile_recomp.py')), str(export), str(output),
                   '--root', '0x20000000']
        if reuse is not None: command += ['--reuse-generated', str(reuse)]
        return subprocess.run(command, capture_output=True, text=True)

    def test_cli_regenerates_poisoned_seed_identically_to_fresh_output(self):
        before = self.inventory()
        export = self.export()
        fresh = self.root / 'fresh'
        clean = self.generate(export, fresh)
        seeded = self.generate(export, self.output, self.source)
        self.assertEqual(clean.returncode, 0, clean.stderr)
        self.assertEqual(seeded.returncode, 0, seeded.stderr)
        self.assertIn('Seeded generated C++:', seeded.stdout)
        self.assertEqual(set(p.name for p in fresh.iterdir()), set(p.name for p in self.output.iterdir()))
        for path in fresh.iterdir(): self.assertEqual(path.read_bytes(), (self.output / path.name).read_bytes())
        self.assertEqual(self.inventory(), before)
        page = (self.output / 'page_20000.cpp').read_text()
        self.assertIn('s.step(0x20000000);', page)
        self.assertNotIn('previous page', page)

    def test_cli_failure_never_copies_completion_manifest(self):
        before = self.inventory()
        result = self.generate(self.export(broken=True), self.output, self.source)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Unsupported operation: INVALID', result.stderr)
        self.assertIn('Seeded generated C++:', result.stdout)
        self.assertFalse((self.output / 'compilation.json').exists())
        self.assertEqual(self.inventory(), before)


if __name__ == '__main__': unittest.main()
