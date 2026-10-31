"""A sweep must not report a stale capture, a timeout or a differing C++ run as a pass."""
import contextlib
import io
import os
from pathlib import Path
import stat
import tempfile
import unittest
from unittest import mock

import replay_sweep

# Stands in for cm-world-replay: each capture's files say what each mode does.
# The original charges 5 instructions and the C++ 4, unless a behaviour says otherwise.
FAKE = """#!/bin/sh
capture=$1
if [ -n "$CM_FAKE_ORIGINAL" ]; then behaviour=$(cat "$capture/original"); instructions=5
else
  behaviour=$(cat "$capture/rewrite"); instructions=4
  case " $* " in *" --timing "*) [ -f "$capture/production" ] && behaviour=$(cat "$capture/production");; esac
fi
case "$behaviour" in
  pass) echo 'census 0x100 3'; echo 'census 0x200 0'; echo "{\\"run\\":0,\\"instructions\\":$instructions,\\"nativeCalls\\":7}";;
  same) echo '{"run":0,"instructions":5,"nativeCalls":7}';;
  calls) echo '{"run":0,"instructions":4,"nativeCalls":8}';;
  silent) ;;
  diverge) echo 'First divergence: native import order' >&2; exit 1;;
  hang) sleep 30;;
esac
"""


class ReplaySweepTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        self.binary = self.root / "replay"
        self.binary.write_text(FAKE)
        self.binary.chmod(self.binary.stat().st_mode | stat.S_IXUSR)
        (self.root / "adapter.cpp").write_text('std::getenv("CM_FAKE_ORIGINAL")')
        patcher = mock.patch.object(replay_sweep, "NATIVE", self.root)
        patcher.start()
        self.addCleanup(patcher.stop)

    def capture(self, name, original, rewrite, production=None):
        path = self.root / name
        path.mkdir()
        (path / "capture.txt").write_text("")
        (path / "original").write_text(original)
        (path / "rewrite").write_text(rewrite)
        if production: (path / "production").write_text(production)
        return path

    def sweep(self, *arguments):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            status = replay_sweep.main([*arguments, "--binary", str(self.binary), "--timeout", "2"])
        return status, output.getvalue()

    def pair(self, original, rewrite, production=None):
        return self.sweep("pair", "CM_FAKE_ORIGINAL", "--captures", str(self.capture("c", original, rewrite, production)))

    def test_all_runs_passing_with_different_instructions_match(self):
        status, output = self.pair("pass", "pass")
        self.assertEqual(status, 0)
        self.assertIn("MATCH=1 STALE=0 SAME=0 DIFFERS=0 TIMEOUT=0", output)

    def test_a_failing_rewrite_differs_and_fails_the_sweep(self):
        status, output = self.pair("pass", "diverge")
        self.assertEqual(status, 1)
        self.assertIn("DIFFERS", output)
        self.assertIn("native import order", output)

    def test_a_production_run_failing_alone_differs(self):
        status, output = self.pair("pass", "pass", "diverge")
        self.assertEqual(status, 1)
        self.assertIn("production: native import order", output)

    def test_different_native_calls_are_a_note_because_they_are_diagnostics(self):
        status, output = self.pair("pass", "calls")
        self.assertEqual(status, 0)
        self.assertIn("native calls 7 and 8", output)

    def test_an_equal_instruction_count_is_not_a_match(self):
        status, output = self.pair("same", "same")
        self.assertEqual(status, 1)
        self.assertIn("MATCH=0 STALE=0 SAME=1", output)

    def test_a_capture_failing_on_the_original_is_stale_and_gives_no_match(self):
        status, output = self.pair("diverge", "pass")
        self.assertEqual(status, 1)
        self.assertIn("MATCH=0 STALE=1", output)

    def test_a_stale_capture_beside_a_match_does_not_fail_the_sweep(self):
        status, output = self.sweep("pair", "CM_FAKE_ORIGINAL", "--captures", str(self.capture("good", "pass", "pass")),
                                    str(self.capture("old", "diverge", "pass")))
        self.assertEqual(status, 0)
        self.assertIn("MATCH=1 STALE=1", output)

    def test_a_timeout_on_any_side_is_never_a_pass(self):
        for name, sides in (("a", ("hang", "pass")), ("b", ("pass", "hang")), ("c", ("pass", "pass", "hang"))):
            status, output = self.sweep("pair", "CM_FAKE_ORIGINAL", "--captures", str(self.capture(name, *sides)))
            self.assertEqual(status, 1)
            self.assertIn("TIMEOUT=1", output)
            self.assertIn("MATCH=0", output)

    def test_a_missing_final_line_is_not_a_match(self):
        status, output = self.pair("silent", "silent")
        self.assertEqual(status, 1)
        self.assertIn("no final line", output)

    def test_a_path_that_is_not_a_capture_is_refused_rather_than_stale(self):
        with self.assertRaises(SystemExit), contextlib.redirect_stderr(io.StringIO()) as error:
            self.sweep("pair", "CM_FAKE_ORIGINAL", "--captures", str(self.root / "missing"))
        self.assertIn("is not a capture", error.getvalue())

    def test_a_printed_name_can_be_passed_back_from_the_project_root(self):
        self.assertEqual(replay_sweep.label(replay_sweep.ROOT / ".local/benchmarks/ui/x"), ".local/benchmarks/ui/x")

    def test_a_variable_nothing_reads_is_refused(self):
        with self.assertRaisesRegex(SystemExit, "CM_TYPO_ORIGINAL is not read"):
            self.sweep("pair", "CM_TYPO_ORIGINAL", "--captures", str(self.capture("c", "pass", "pass")))

    def test_an_exported_variable_does_not_reach_the_cpp_side(self):
        with mock.patch.dict(os.environ, {"CM_FAKE_ORIGINAL": "1"}):
            status, output = self.pair("pass", "pass")
        self.assertEqual(status, 0)
        self.assertIn("5 and 4 instructions", output)

    def test_census_lists_only_entries_reached_and_says_when_it_cannot_tell(self):
        entries = self.root / "entries"
        entries.write_text("0x100\n0x200\n")
        reached = self.capture("reached", "pass", "pass")
        broken = self.capture("broken", "pass", "diverge")
        status, output = self.sweep("census", str(entries), "--captures", str(reached), str(broken))
        self.assertEqual(status, 0)
        self.assertIn("complete; 0x100=3", output)
        self.assertNotIn("0x200=", output)
        self.assertIn("failed: native import order; entries unknown", output)
        self.assertIn("2 captures: 1 complete, 1 reach an entry", output)

    def test_census_fails_on_a_timeout_or_when_nothing_completes(self):
        entries = self.root / "entries"
        entries.write_text("0x100\n")
        self.assertEqual(self.sweep("census", str(entries), "--captures", str(self.capture("h", "pass", "hang")))[0], 1)
        self.assertEqual(self.sweep("census", str(entries), "--captures", str(self.capture("d", "pass", "diverge")))[0], 1)

    def test_captures_finds_nested_ones_and_needs_a_capture_file(self):
        root = self.root / "project"
        for path in (".local/benchmarks/ui/kept", ".local/benchmarks/world/group/0", ".local/performance/x/captures/y"):
            (root / path).mkdir(parents=True)
            (root / path / "capture.txt").write_text("")
        (root / ".local/benchmarks/ui/skipped").mkdir()
        self.assertEqual([path.name for path in replay_sweep.captures(root)], ["kept", "0", "y"])


if __name__ == "__main__":
    unittest.main()
