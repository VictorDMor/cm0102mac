"""Seed generated C++ with independent copy-on-write files when available.

This is a storage hint, never a source or semantic cache. The generator must
render and validate every unit again before publishing its completion manifest.
"""
from __future__ import annotations

import ctypes
import errno
import json
import os
from pathlib import Path
import re
import stat
import sys


def _checked_directory(path: Path, *, required: bool) -> Path:
    if '..' in path.parts:
        raise ValueError('Generated directory paths must not contain parent traversal')
    absolute = Path(os.path.abspath(path))
    current = Path(absolute.anchor)
    for part in absolute.parts[1:]:
        current /= part
        try:
            mode = current.lstat().st_mode
        except FileNotFoundError:
            if required:
                raise ValueError(f'Missing generated directory: {current}') from None
            continue
        if not stat.S_ISDIR(mode):
            raise ValueError(f'Generated directory component is not a regular directory: {current}')
    return absolute


def _regular_file(path: Path) -> os.stat_result:
    info = path.lstat()
    if not stat.S_ISREG(info.st_mode):
        raise ValueError(f'Generated source is not a regular file: {path}')
    return info


def _identity(info: os.stat_result) -> tuple[int, ...]:
    return (info.st_dev, info.st_ino, info.st_size, info.st_mtime_ns, info.st_mode)


def _copy_independent(source: Path, destination: Path) -> str:
    if sys.platform == 'darwin':
        clone = ctypes.CDLL(None, use_errno=True).clonefile
        clone.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
        clone.restype = ctypes.c_int
        if clone(os.fsencode(source), os.fsencode(destination), 0) == 0:
            return 'clone'
        error = ctypes.get_errno()
        if error not in {errno.ENOTSUP, errno.EXDEV, errno.EINVAL, errno.ENOSYS}:
            raise OSError(error, os.strerror(error), str(destination))
    # Exclusive creation protects an existing destination even on platforms
    # without clonefile. No hard links: later writes must leave references intact.
    with source.open('rb') as incoming, destination.open('xb') as outgoing:
        while block := incoming.read(1024 * 1024):
            outgoing.write(block)
    return 'copy'


def seed_generated(source: Path, output: Path) -> dict[str, int | str]:
    """Validate a completed source and an empty destination, then seed C++ only.

    An interrupted or failed seed has no compilation.json. Rerunning without
    this option regenerates that partial directory normally. The source tree is
    never edited, and the completion manifest is never copied.
    """
    source = _checked_directory(source, required=True)
    output = _checked_directory(output, required=False)
    if source == output or source in output.parents or output in source.parents:
        raise ValueError('Generated source and destination directories must be separate')
    if output.exists() and any(output.iterdir()):
        raise ValueError('Reusable generation requires an empty destination directory')
    manifest_path = source / 'compilation.json'
    manifest_info = _regular_file(manifest_path)
    manifest = json.loads(manifest_path.read_text())
    files = manifest.get('files')
    if (not isinstance(files, list) or not files or
            any(not isinstance(name, str) or not re.fullmatch(r'(?:page_[0-9a-f]+|program)\.cpp', name)
                for name in files) or len(set(files)) != len(files) or files.count('program.cpp') != 1 or
            type(manifest.get('pages')) is not int or manifest['pages'] != len(files) - 1 or
            type(manifest.get('instructions')) is not int or manifest['instructions'] < 1 or
            manifest.get('missingStaticTargets') != [] or manifest.get('pendingTargets') != []):
        raise ValueError('Reusable generation requires a complete, valid C++ source manifest')
    # Validate every listed file before creating anything. Other private files
    # in the source directory are deliberately excluded from the seed.
    sources = [(source / name, _regular_file(source / name)) for name in files]
    output.mkdir(parents=True, exist_ok=True)
    report: dict[str, int | str] = dict(source=str(source), clonedFiles=0, copiedFiles=0, sourceBytes=0)
    for path, before in sources:
        if _identity(_regular_file(path)) != _identity(before):
            raise ValueError(f'Generated source changed during reuse: {path}')
        destination = output / path.name
        method = _copy_independent(path, destination)
        after = _regular_file(destination)
        if (after.st_dev, after.st_ino) == (before.st_dev, before.st_ino) or after.st_size != before.st_size:
            raise ValueError(f'Generated seed is not an independent, complete file: {destination}')
        if _identity(_regular_file(path)) != _identity(before):
            raise ValueError(f'Generated source changed during reuse: {path}')
        key = 'clonedFiles' if method == 'clone' else 'copiedFiles'
        report[key] += 1
        report['sourceBytes'] += before.st_size
    if _identity(_regular_file(manifest_path)) != _identity(manifest_info):
        raise ValueError('Generated source manifest changed during reuse')
    return report
