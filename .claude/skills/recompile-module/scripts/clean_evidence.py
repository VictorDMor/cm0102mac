#!/usr/bin/env python3
"""Delete a delivered module's bulky, reproducible rewrite evidence.

Usage: clean_evidence.py MODULE [MODULE...] [--delete]
       clean_evidence.py --all [--delete]

MODULE is a directory name under .local/rewrites (siprint, award_shortlist).
Without --delete it only lists what would go. It removes coverage profiles and
directories, logs, baselines and owned diffs, plus the module's coverage output
and checker binary in the build. The ledger, notes and small sources stay.

A module with no commit, or with uncommitted native/<module>* or tools/<module>*
files, is still being worked on and is refused (skipped with --all).
"""
import argparse
import fnmatch
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(subprocess.run(["git", "rev-parse", "--show-toplevel"], cwd=Path(__file__).parent,
                           capture_output=True, text=True, check=True).stdout.strip())
REWRITES = ROOT / ".local/rewrites"
BUILD = ROOT / ".local/current/build"
FILES = ("*.profraw*", "*.profdata", "*.log", "*.tmp", "*.owned.diff", "*.patch", "baseline*")
DIRECTORIES = ("coverage*", "baseline*")


def git(*args):
    return subprocess.run(["git", *args], cwd=ROOT, capture_output=True, text=True, check=True).stdout.strip()


def refusal(module):
    paths = [f"native/{module}*", f"tools/{module}*", f"tools/test_{module}*"]
    if not git("log", "-1", "--format=%h", "--", *paths):
        return "no commit touches its sources"
    if git("status", "--porcelain", "--", *paths):
        return "it has uncommitted work"
    return None


def size(path):
    if path.is_symlink() or path.is_file():
        return path.lstat().st_size
    return sum(p.lstat().st_size for p in path.rglob("*") if p.is_file() and not p.is_symlink())


def matches(name, patterns):
    return any(fnmatch.fnmatch(name, pattern) for pattern in patterns)


def targets(module):
    found = []
    def walk(directory):
        for entry in sorted(directory.iterdir()):
            if entry.is_dir() and not entry.is_symlink():
                if matches(entry.name, DIRECTORIES):
                    found.append(entry)
                else:
                    walk(entry)
            elif matches(entry.name, FILES):
                found.append(entry)
    if (REWRITES / module).is_dir():
        walk(REWRITES / module)
    dashed = module.replace("_", "-")
    for name in {module, dashed}:
        output = BUILD / f"{name}-coverage"
        if output.is_dir():  # CMake creates this directory at configure time; it stays.
            found.extend(sorted(output.iterdir()))
    checker = BUILD / f"cm-{dashed}-checks"
    if checker.is_file():
        found.append(checker)
    return found


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("modules", nargs="*")
    parser.add_argument("--all", action="store_true", help="every module under .local/rewrites")
    parser.add_argument("--delete", action="store_true", help="delete instead of listing")
    args = parser.parse_args()
    if bool(args.modules) == args.all:
        parser.error("name modules or pass --all")
    modules = sorted(p.name for p in REWRITES.iterdir() if p.is_dir()) if args.all else args.modules

    failed = False
    total = 0
    for module in modules:
        if not re.fullmatch(r"[a-z0-9_]+", module):
            parser.error(f"not a module directory name: {module}")
        reason = refusal(module)
        if reason:
            print(f"{module}: skipped, {reason}")
            failed = failed or not args.all
            continue
        found = targets(module)
        bytes_ = sum(size(path) for path in found)
        total += bytes_
        print(f"{module}: {len(found)} items, {bytes_ / 1e6:.1f} MB")
        for path in found:
            if args.delete:
                if path.is_dir() and not path.is_symlink():
                    shutil.rmtree(path)
                else:
                    path.unlink()
            elif size(path) >= 50_000_000:
                print(f"  {size(path) / 1e6:.0f} MB {path.relative_to(ROOT)}")
    print(f"{'Deleted' if args.delete else 'Would delete'} {total / 1e9:.2f} GB")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
