#!/usr/bin/env python3
"""Drive an isolated validation copy of the game through its automation channel.

The app reads one request at a time from <run>/Automation/inbox at an original
interactive event boundary and reports in <run>/Automation/status.json.
Requests name original commands, control-table indexes and keys, never screen
positions. Create the run with:

    tools/game_driver.py prepare NAME [--save PATH] [--season-reference-days N]

then launch it, send requests, and time Continue:

    tools/game_driver.py launch NAME
    tools/game_driver.py send NAME "command load_game"
    tools/game_driver.py send NAME "snapshot start"
    tools/game_driver.py advance NAME --days 7
    tools/game_driver.py stop NAME
"""
import argparse
import datetime
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parent.parent


def run_directory(name):
    path = ROOT / ".local/performance" / name
    if not (path / "manifest.json").is_file():
        sys.exit(f"No validation run {name}; create it with: tools/game_driver.py prepare {name}")
    return path


def channel(run):
    manifest = json.loads((run / "manifest.json").read_text())
    if "automation" not in manifest:
        sys.exit("This run was prepared without --automation")
    return Path(manifest["automation"]), manifest


def status(directory):
    try:
        return json.loads((directory / "status.json").read_text())
    except (FileNotFoundError, json.JSONDecodeError):
        return None


def alive(pid):
    try:
        os.kill(pid, 0)
        return True
    except (ProcessLookupError, PermissionError):
        return False


def wait(directory, condition, timeout, what):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        current = status(directory)
        if current and condition(current):
            return current
        if current and not alive(current["pid"]):
            sys.exit(f"The game exited while waiting for {what}")
        time.sleep(0.02)
    sys.exit(f"Timed out after {timeout:.0f} s waiting for {what}")


def settled(directory, settle, timeout, what):
    """Wait until the game waits for input and its status stays unchanged."""
    deadline = time.monotonic() + timeout
    stable_since, previous = None, None
    while time.monotonic() < deadline:
        current = status(directory)
        if current and not alive(current["pid"]):
            sys.exit(f"The game exited while waiting for {what}")
        if current != previous:
            previous, stable_since = current, time.monotonic()
        elif current and current["ready"] and time.monotonic() - stable_since >= settle:
            return current
        time.sleep(0.02)
    sys.exit(f"Timed out after {timeout:.0f} s waiting for {what}")


def next_sequence(directory):
    current = status(directory) or {"done": 0}
    pending = [int(p.stem) for p in (directory / "inbox").glob("*.req") if p.stem.isdigit()]
    return max([current["done"], *pending]) + 1


def send(directory, request, timeout=600, settle=0.5):
    """Send one request; return (result, status after it settled, seconds)."""
    sequence = next_sequence(directory)
    temporary = directory / "inbox" / f"{sequence}.tmp"
    temporary.write_text(request + "\n")
    started = time.time()
    temporary.rename(temporary.with_suffix(".req"))
    done = wait(directory, lambda s: s["done"] >= sequence, timeout, f"request {sequence}")
    if not done["result"].startswith("ok"):
        return done["result"], done, 0.0
    verb = request.split()[0]
    if verb in ("snapshot", "memory"):
        return done["result"], done, time.time() - started
    final = settled(directory, settle, timeout, f"the game after request {sequence}")
    return done["result"], final, final["time"] / 1000 - started


def ordinal(date):
    """Days since an epoch; the game counts days of the year from 0."""
    return datetime.date(date["year"], 1, 1).toordinal() + date["day"]


def describe(current):
    date = current.get("date")
    when = f"day {date['day']} of {date['year']} phase {date['phase']}" if date else "no date"
    commands = ", ".join(current.get("commands", {})) or "none"
    return (f"ready={current['ready']} {when}; handler {current.get('handler', '-')}; "
            f"commands: {commands}; {len(current.get('controls', []))} controls; status: {current['status']}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="action", required=True)
    prepare = commands.add_parser("prepare", help="Create an isolated validation app with automation")
    prepare.add_argument("name")
    prepare.add_argument("--save", type=Path, action="append", default=[])
    prepare.add_argument("--season-reference-days", type=int)
    for action in ("launch", "status", "stop"):
        commands.add_parser(action).add_argument("name")
    request = commands.add_parser("send", help="Send requests in order and wait for each")
    request.add_argument("name")
    request.add_argument("requests", nargs="+")
    request.add_argument("--timeout", type=float, default=600)
    request.add_argument("--settle", type=float, default=0.5)
    advance = commands.add_parser("advance", help="Continue until the game date moves N days; time each step")
    advance.add_argument("name")
    advance.add_argument("--days", type=int, default=7)
    advance.add_argument("--timeout", type=float, default=600)
    advance.add_argument("--settle", type=float, default=0.5)
    args = parser.parse_args()

    if args.action == "prepare":
        command = [sys.executable, str(ROOT / "tools/prepare_validation.py"), args.name, "--automation"]
        for save in args.save:
            command += ["--save", str(save)]
        if args.season_reference_days:
            command += ["--season-reference-days", str(args.season_reference_days)]
        subprocess.run(command, check=True, stdout=subprocess.DEVNULL)
        print(run_directory(args.name))
        return
    run = run_directory(args.name)
    directory, manifest = channel(run)
    if args.action == "launch":
        current = status(directory)
        if current and alive(current["pid"]):
            sys.exit(f"Already running as pid {current['pid']}")
        (directory / "status.json").unlink(missing_ok=True)
        for leftover in (directory / "inbox").glob("*"):
            leftover.unlink()
        app = Path(manifest["app"])
        executable = app / "Contents/MacOS" / json.loads(subprocess.check_output(
            ["plutil", "-convert", "json", "-o", "-", str(app / "Contents/Info.plist")]))["CFBundleExecutable"]
        subprocess.Popen([str(executable)], stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL, start_new_session=True)
        print(describe(settled(directory, 1.0, 300, "the start screen")))
    elif args.action == "status":
        current = status(directory)
        print(json.dumps(current, indent=2) if current else "No status yet")
    elif args.action == "stop":
        current = status(directory)
        if current and alive(current["pid"]):
            os.kill(current["pid"], signal.SIGTERM)
            print(f"Stopped pid {current['pid']}")
        else:
            print("Not running")
    elif args.action == "send":
        for text in args.requests:
            result, current, seconds = send(directory, text, args.timeout, args.settle)
            print(f"{text}: {result} ({seconds:.2f} s)\n  {describe(current)}")
            if not result.startswith("ok"):
                sys.exit(1)
    elif args.action == "advance":
        start = settled(directory, args.settle, args.timeout, "an idle screen")
        if not start.get("date"):
            sys.exit("The game has no career date yet")
        total, steps, current = 0.0, 0, start
        while True:
            if ordinal(current["date"]) - ordinal(start["date"]) >= args.days:
                break
            if "continue_game" not in current.get("commands", {}):
                sys.exit(f"Continue is unavailable after {steps} steps: {describe(current)}")
            result, current, seconds = send(directory, "command continue_game", args.timeout, args.settle)
            if not result.startswith("ok"):
                sys.exit(f"Continue refused: {result}")
            steps += 1
            busy = current["last_busy_seconds"]
            total += busy
            print(f"step {steps}: day {current['date']['day']} of {current['date']['year']}; "
                  f"processing {busy:.2f} s (request to settled {seconds:.2f} s)")
        print(f"{steps} Continue steps, {args.days} days: processing {total:.2f} s")


if __name__ == "__main__":
    main()
