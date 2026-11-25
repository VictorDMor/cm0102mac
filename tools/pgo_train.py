#!/usr/bin/env python3
"""Run the game's own workload in isolated driven copies: PGO training, page selection and timing.

Usage: pgo_train.py [SCENARIO ...] [--profile-dir DIR] [--sample-dir DIR] [--app APP] [--repeat N] [--keep]

Scenarios (default: all), each a fresh copy of the current build (tools/game_driver.py prepare) with one
benchmark save from .local/benchmarks/saves and nothing else in its saves folder:
  holiday  the multi-league career of 25 August 2001 on holiday until day 244, matches on 8 workers
           (Parallel Matches), the installed game's pointer mode: the daily world, background matches,
           the shortlist scans with their judging on workers, the screens that go by on holiday
  season   the multi-league career of 14 July 2001 continued to day 212: the human's matches (best eleven,
           then the Z shortcut), the pre-match confirmations, the days between
  week     the same as holiday, only to day 240 (a shorter timing input)
  heldout  another career (10 January 2002) played to day 17: never trained on, for timing a profile on
           work it did not see

--profile-dir DIR: the build is instrumented (tools/dev.sh pgo instrument); each scenario writes
  DIR/<scenario>-<pid>.profraw in LLVM's continuous mode (%c), so the counters are on disk while the game
  runs and the driver's SIGTERM loses nothing.
--sample-dir DIR: macOS `sample` follows the game process for the whole scenario and writes
  DIR/<scenario>.sample.txt (tools/pgo_pages.py reads it to choose the pages to instrument).
Prints one line per scenario with its wall time from the load to the target day (the timing input for
alternating pairs). A scenario that does not reach its day in time fails the run.
"""
import argparse
import json
import os
import signal
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DRIVER = ROOT / "tools/game_driver.py"
SAVES = ROOT / ".local/benchmarks/saves"

SCENARIOS = {
    "holiday": {"save": "Multi-Arsenal-Aug2001-holiday.sav", "target": 244, "mode": "holiday", "limit": 900},
    "week": {"save": "Multi-Arsenal-Aug2001-holiday.sav", "target": 240, "mode": "holiday", "limit": 600},
    "season": {"save": "Multi-Arsenal-14Jul2001.sav", "target": 212, "mode": "play", "limit": 1500},
    # Not trained on: another career, mid-season (10 January 2002), for checking that a profile is not fitted to
    # its training input.
    "heldout": {"save": "Mac-Teste-Validado-Pos-Jogo.sav", "year": 2002, "target": 17, "mode": "play", "limit": 1200},
}
MATCH_SCREEN = "0x1028dc80"                       # the match in progress: Z plays it out
CONFIRMATIONS = ("0x1029cd10", "0x100f0ba0")      # "Proceed to the match ?" and the message box: the last control


def driver(*arguments, timeout=1200, check=True):
    result = subprocess.run([sys.executable, str(DRIVER), *arguments], capture_output=True, text=True, timeout=timeout + 60)
    if check and result.returncode:
        raise SystemExit(f"game_driver {' '.join(arguments)} failed:\n{result.stdout[-600:]}{result.stderr[-600:]}")
    return result.stdout


def status(run):
    path = ROOT / ".local/performance" / run / "Automation/status.json"
    for _ in range(100):
        try:
            return json.loads(path.read_text())
        except (OSError, ValueError):
            time.sleep(0.1)
    raise SystemExit(f"{run}: no status")


def prepare(run, save, app=None):
    driver("stop", run, check=False)
    subprocess.run(["rm", "-rf", str(ROOT / ".local/performance" / run)], check=True)
    driver("prepare", run, "--save", str(SAVES / save), *(["--app", str(app)] if app else []))
    # prepare copies the player's own saves too, and the newest is the load dialog's first row.
    for path in (ROOT / ".local/performance" / run / "Runtime/Files/games").rglob("*.sav"):
        if path.name != save:
            path.unlink()
    # The installed game's pointer mode, not the validation copy's event coordinates (which send every event
    # poll through the main thread).
    app = next((ROOT / ".local/performance" / run).glob("CM Validation *.app"))
    plist = app / "Contents/Info.plist"
    subprocess.run(["/usr/libexec/PlistBuddy", "-c", "Delete :CMValidationEventPointer", str(plist)], capture_output=True)
    subprocess.run(["/usr/libexec/PlistBuddy", "-c", "Add :CMValidationEventPointer bool false", str(plist)], check=True)
    subprocess.run(["codesign", "--force", "-s", "-", str(app)], check=True, capture_output=True)


def run_scenario(name, args):
    scenario = SCENARIOS[name]
    run = f"pgo-{name}{args.tag}"
    prepare(run, scenario["save"], args.app)
    environment = dict(os.environ, CM_MATCH_DAY_ORDER="parallel", CM_MATCH_WORKERS="8")
    if args.profile_dir:
        environment["LLVM_PROFILE_FILE"] = str(args.profile_dir.resolve() / f"{name}-%c%p.profraw")
    subprocess.run([sys.executable, str(DRIVER), "launch", run], env=environment, check=True, capture_output=True, timeout=400)
    sampler = None
    try:
        driver("send", run, "command load_game", "--timeout", "600")
        if args.sample_dir:
            out = args.sample_dir / f"{name}.sample.txt"
            sampler = subprocess.Popen(["/usr/bin/sample", str(status(run)["pid"]), str(scenario["limit"] + 60), "1", "-file", str(out)],
                                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        started = time.time()
        if scenario["mode"] == "holiday":
            holiday(run, scenario, started)
        else:
            play(run, scenario, started)
        elapsed = time.time() - started
    finally:
        if sampler:
            sampler.send_signal(signal.SIGINT)   # sample writes its report when interrupted
            try:
                sampler.wait(timeout=120)
            except subprocess.TimeoutExpired:
                sampler.kill()
        driver("stop", run, check=False)
        time.sleep(1)
    print(f"{name}: day {scenario['target']} after {elapsed:.1f} s", flush=True)
    if not args.keep:
        subprocess.run(["rm", "-rf", str(ROOT / ".local/performance" / run)], check=True)


def day(run):
    current = status(run)
    return current.get("date", {}).get("day", 0), current


def holiday(run, scenario, started):
    # The first row is the career; its manager is on holiday, so loading continues by itself.
    driver("send", run, "control 11", "control 19", "--timeout", str(scenario["limit"]))
    while day(run)[0] < scenario["target"]:
        if time.time() - started > scenario["limit"]:
            raise SystemExit(f"{run}: day {day(run)[0]} after {scenario['limit']} s")
        time.sleep(0.5)


def act(run, request):
    """Send one request; a refusal (the game is between two states) is waited out by the caller's loop."""
    out = driver("send", run, request, check=False)
    if "refused" in out.lower() or "failed" in out.lower():
        time.sleep(0.5)


def play(run, scenario, started):
    driver("send", run, "control 11", "control 19", "--timeout", "600")
    # The load goes on after the dialog's answer, from the start screen.
    while "continue_game" not in status(run).get("commands", {}):
        if time.time() - started > 300:
            raise SystemExit(f"{run}: the career did not load")
        time.sleep(0.5)
    last, same = None, 0
    while True:
        current = status(run)
        date = current.get("date", {})
        if (date.get("year", 0), date.get("day", 0)) >= (scenario.get("year", 2001), scenario["target"]):
            return
        if time.time() - started > scenario["limit"]:
            raise SystemExit(f"{run}: day {date.get('day')} after {scenario['limit']} s")
        key = (date.get("day"), date.get("phase"), current.get("handler"), len(current.get("controls", [])))
        same = same + 1 if key == last else 0
        last = key
        if same > 240:
            raise SystemExit(f"{run}: stuck at {key}, commands {list(current.get('commands', {}))}")
        commands = current.get("commands", {})
        if "continue_game" in commands:
            if "best_xi" in commands and date.get("phase") == 1 and same == 0:
                act(run, "command best_xi")
            act(run, "command continue_game")
        elif current.get("handler") == MATCH_SCREEN:
            act(run, "key z")
        elif current.get("handler") in CONFIRMATIONS:
            act(run, f"control {current['controls'][-1][0]}")
        elif same >= 10 and "back_or_cancel" in commands:
            # A screen this loop does not know that stays (a report or notice the game opened): leave it.
            act(run, "command back_or_cancel")
        else:
            # A screen between two steps (the game is busy, or one this loop does not know): wait, and fail
            # through the stuck test above if it stays.
            time.sleep(0.5)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("scenarios", nargs="*", metavar="SCENARIO")
    parser.add_argument("--profile-dir", type=Path)
    parser.add_argument("--sample-dir", type=Path)
    parser.add_argument("--keep", action="store_true", help="keep the run directories")
    parser.add_argument("--app", type=Path, help="the app to run (default: the current build's)")
    parser.add_argument("--tag", default="", help="suffix of the run directories (two runs at once)")
    parser.add_argument("--repeat", type=int, default=1, help="run the scenarios this many times")
    args = parser.parse_args()
    unknown = [name for name in args.scenarios if name not in SCENARIOS]
    if unknown:
        parser.error(f"unknown scenario {unknown[0]} (known: {', '.join(SCENARIOS)})")
    for directory in (args.profile_dir, args.sample_dir):
        if directory:
            directory.mkdir(parents=True, exist_ok=True)
    for _ in range(args.repeat):
        for name in args.scenarios or ["holiday", "season"]:
            run_scenario(name, args)


if __name__ == "__main__":
    main()
