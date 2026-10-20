#!/usr/bin/env python3
"""Compare Date-semantic day replays with original and semantic RNG cores.

Reuse the immutable native-import/RNG transcript; do not capture Date streams.
Only translated instruction accounting differs. Reports and fixtures stay local.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess
import time
from check_season_reference import digest

ENTRIES = ('0x1077b770', '0x1077b790', '0x107474a0')


def matrix(output):
    values = {}
    for entry, mode, modern, original in re.findall(
            r'rng.entry=(0x[0-9a-f]+) mode=(original|modern) modern=(\d+) original=(\d+)', output):
        if entry in values:
            raise RuntimeError('Duplicate RNG coverage row')
        values[entry] = {'mode': mode, 'modern': int(modern), 'original': int(original)}
    if set(values) != set(ENTRIES):
        raise RuntimeError('Incomplete RNG core coverage report')
    return values


def check(executable, fixture, report_path, timeout):
    if report_path.exists():
        raise RuntimeError('Use a new private report path')
    paths = [executable, *(fixture / name for name in
                          ('capture.txt', 'initial.snapshot', 'expected.snapshot', 'imports.gz'))]
    identities = {str(p): digest(p) for p in paths}
    if 'status=complete\n' not in (fixture / 'capture.txt').read_text():
        raise RuntimeError('Fixture lacks a completed full-day capture')
    runs = []
    report = {'status': 'running', 'identities': identities, 'runs': runs,
              'instructionAccountingOnly': True, 'newStreamsRecorded': False}
    def save():
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(json.dumps(report, indent=2) + '\n')
    save()
    try:
        for mode, negative in [('baseline', False), ('baseline', False),
                               ('modern', False), ('modern', False), ('modern', True)]:
            options = ['--rng-' + mode] + (['--perturb-rng'] if negative else [])
            started = time.time()
            print('Starting', ' '.join(options), flush=True)
            result = subprocess.run([str(executable), str(fixture), *options],
                                    capture_output=True, text=True, timeout=timeout)
            run = {'options': options, 'exitCode': result.returncode,
                   'stdout': result.stdout, 'stderr': result.stderr, 'seconds': time.time() - started}
            runs.append(run); save()
            for p in paths:
                if digest(p) != identities[str(p)]:
                    raise RuntimeError('Executable or immutable fixture changed during replay: ' + str(p))
            if negative:
                if result.returncode != 1 or not result.stderr.startswith('First divergence:'):
                    raise RuntimeError('Initial RNG perturbation was not rejected')
            else:
                if result.returncode or not result.stdout.startswith('PASS: semantic calendar bounded simulation agreement\n'):
                    raise RuntimeError('Equivalent RNG replay failed: ' + result.stderr)
                run['coverage'] = matrix(result.stdout)
                for row in run['coverage'].values():
                    if row['mode'] != ('original' if mode == 'baseline' else 'modern') or row['modern' if mode == 'baseline' else 'original']:
                        raise RuntimeError('RNG mode selection mismatch')
                date_rows = re.findall(r'date.entry=0x[0-9a-f]+ mode=(\w+)', result.stdout)
                if len(date_rows) != 26 or any(value != 'modern' for value in date_rows):
                    raise RuntimeError('RNG replay failed to preserve the 26 semantic Date entries')
            save()
            print('Completed', ' '.join(options), 'seconds=', round(run['seconds'], 2), flush=True)
        if runs[0]['stdout'] != runs[1]['stdout'] or runs[2]['stdout'] != runs[3]['stdout']:
            raise RuntimeError('Repeated reports disagree within a mode')
        for entry in ENTRIES:
            if runs[0]['coverage'][entry]['original'] != runs[2]['coverage'][entry]['modern']:
                raise RuntimeError('Original/semantic own-call counts disagree')
        pattern = r'rng.ordered-boundaries=([0-9a-f]+)'
        if re.search(pattern, runs[0]['stdout']).group(1) != re.search(pattern, runs[2]['stdout']).group(1):
            raise RuntimeError('Original/semantic ordered boundary evidence disagrees')
        report.update(status='pass', equivalentRunsPerMode=2, initialRngNegativeRejected=True,
                      exactFinalStateAndTranscriptEof=True,
                      unexercisedEntries=[e for e in ENTRIES if not runs[2]['coverage'][e]['modern']])
        save()
    except BaseException as error:
        report.update(status='failed', error=str(error)); save(); raise
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('fixture', type=Path)
    parser.add_argument('--executable', type=Path, default=Path('.local/current/build/cm-season-reference'))
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--timeout', type=int, default=7200)
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error('Use a positive finite timeout')
    check(args.executable.resolve(strict=True), args.fixture.resolve(strict=True), args.report, args.timeout)
    print('PASS: RNG core bounded comparison and unchanged fixture/executable identities')


if __name__ == '__main__':
    main()
