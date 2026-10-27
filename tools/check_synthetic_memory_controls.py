#!/usr/bin/env python3
"""Require both intentional final-memory failures from a complete replay."""
import re
import subprocess
import sys


def main():
    if len(sys.argv) != 4:
        print("Usage: check_synthetic_memory_controls.py REPLAY CAPTURE CASES", file=sys.stderr)
        return 1
    command = [sys.argv[1], sys.argv[2], '1', '--check-routines', '1', '1000000',
               '--only', '0x1007f720', '--synthetic', sys.argv[3],
               '--strict-volatile', '--compare-f1']
    try:
        result = subprocess.run(command, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, timeout=150, check=False)
    except (OSError, subprocess.TimeoutExpired) as error:
        print(f"Synthetic memory control did not finish: {error}", file=sys.stderr)
        return 1
    output = result.stdout
    required = [
        r'original memory expectation [0-9a-f]{8}: 00000001 != 00000002',
        r'rewrite memory expectation [0-9a-f]{8}: 00000001 != 00000002',
        r'original memory expectation is unmapped at 00000000',
        r'rewrite memory expectation is unmapped at 00000000',
        r'synthetic calls: 2 listed, 2 made, 2 differ',
    ]
    if (result.returncode != 1 or output.count('synthetic DIFFERS:') != 2 or
            'both stopped' in output or any(not re.search(pattern, output) for pattern in required)):
        print(output, file=sys.stderr)
        print(f"Expected both deliberate memory failures; replay exit={result.returncode}", file=sys.stderr)
        return 1
    print("Both deliberate final-memory failures detected in completed original and C++ calls.")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
