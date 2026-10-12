#!/usr/bin/env python3
"""Focused real-engine checks for the standalone debugger's process protocol."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import queue
import subprocess
import tempfile
import threading
import time


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


class Worker:
    def __init__(self, executable: Path, program: Path, game: Path, root: Path):
        self.root = root
        root.mkdir()
        self.stderr = tempfile.TemporaryFile()
        self.process = subprocess.Popen(
            [str(executable), "--engine-worker", str(program), str(game), str(root)],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=self.stderr,
            text=True, bufsize=1,
        )
        self.messages: queue.Queue = queue.Queue()

        def receive():
            try:
                for line in self.process.stdout:
                    self.messages.put(json.loads(line))
            except Exception as error:
                self.messages.put(error)
            finally:
                self.messages.put(EOFError("Engine worker closed its output"))

        self.reader = threading.Thread(target=receive, daemon=True)
        self.reader.start()
        self.last_request = 0
        require(self.next()["type"] == "ready", "Worker readiness")

    def next(self):
        value = self.messages.get(timeout=120)
        if isinstance(value, Exception):
            raise value
        return value

    def send(self, command: dict):
        self.last_request += 1
        self.process.stdin.write(json.dumps({**command, "requestID": self.last_request}) + "\n")
        self.process.stdin.flush()

    def state(self, **expected):
        deadline = time.monotonic() + 120
        while time.monotonic() < deadline:
            value = self.next()
            if value["type"] == "error":
                raise AssertionError(value["message"])
            if value["type"] == "state" and all(value.get(k) == v for k, v in expected.items()):
                return value
        raise TimeoutError("Expected engine worker state was not reported")

    def rejected(self, command: dict, preserved: dict):
        self.send(command)
        require(self.next()["type"] == "error", "Invalid command was accepted")
        state = self.state()
        for key in ("generation", "homeID", "awayID", "seed", "stepCount", "engineUnits", "homeScore", "awayScore", "players", "sharedModernCalls"):
            require(state.get(key) == preserved.get(key), f"Rejected command changed {key}")

    def close(self, *, eof=False):
        if self.process.poll() is None:
            if eof:
                self.process.stdin.close()
            else:
                self.send({"command": "quit"})
            self.process.wait(timeout=10)
        require(self.process.returncode == 0, "Worker did not exit normally")
        require(not list(self.root.iterdir()), "Worker left experiment backing after normal exit")
        self.stderr.seek(0)
        errors = self.stderr.read().decode(errors="replace")
        require(not errors, f"Worker stderr: {errors[:1000]}")
        self.stderr.close()


def lineup(state):
    for side in (0, 1):
        active = [p for p in state["players"] if p["team"] == side and p["active"]]
        require(len(active) == 11, "Actual starting eleven")
        require(len({p["staffID"] for p in active}) == 11, "Unique actual starters")
        require(len({p["position"] for p in active}) == 11, "Unique original positions")
        require(all(p["name"] and p["staffID"] >= 0 for p in active), "Resolved actual player names")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("program", type=Path)
    parser.add_argument("game", type=Path)
    parser.add_argument("--private-root", required=True, type=Path)
    args = parser.parse_args()
    args.private_root.mkdir(parents=True, exist_ok=True)
    workers = []
    with tempfile.TemporaryDirectory(prefix="worker-check-", dir=args.private_root) as directory:
        root = Path(directory)
        try:
            first = Worker(args.executable.resolve(), args.program.resolve(), args.game.resolve(), root / "first")
            workers.append(first)
            first.rejected({"command": "step"}, {"status": "empty"})
            first.send({"command": "create", "homeID": 677, "awayID": 5677, "seed": 12345})
            initial = first.state(status="paused", engineUnits=0, stepCount=0)
            lineup(initial)
            require(initial["requestID"] == first.last_request, "Creation response correlation")
            require(initial["sharedModernCalls"] == 0, "Fresh instance already advanced")
            require(any(p["name"] == "David Seaman" for p in initial["players"]), "Actual Arsenal name")
            require(any(p["name"] == "Michaël Silvestre" for p in initial["players"]), "Original non-ASCII player name")
            for tick in range(1, 4):
                first.send({"command": "step"})
                stepped = first.state(status="paused", stepCount=tick)
                require(stepped["engineUnits"] == tick and stepped["sharedModernCalls"] == tick and stepped["lastInstructions"] > 0,
                        "Step bypassed the real shared C++23 engine")
                require(stepped["requestID"] == first.last_request, "Step response correlation")
            for command in (
                {"command": "create", "homeID": 677, "awayID": 5677, "seed": 0},
                {"command": "create", "homeID": 677, "awayID": 5677, "seed": True},
                {"command": "create", "homeID": 677, "awayID": 5677, "seed": 1.25},
                {"command": "create", "homeID": 677, "awayID": 5677, "seed": -1},
                {"command": "create", "homeID": 677, "awayID": 5677, "seed": 4294967296},
                {"command": "unknown"}, {"command": "step", "extra": 1},
                {"command": "create", "homeID": 1000000, "awayID": 5677, "seed": 12345},
            ):
                first.rejected(command, stepped)
            first.send({"command": "run"})
            first.state(status="running")
            running = first.state(status="running")
            require(running["stepCount"] > 3, "Scheduled worker did not advance")
            first.send({"command": "pause"})
            paused = first.state(status="paused")
            time.sleep(0.15)
            first.send({"command": "inspect"})
            stable = first.state(status="paused")
            require(stable["engineUnits"] == paused["engineUnits"] and stable["stepCount"] == paused["stepCount"], "Pause advanced the engine")

            second = Worker(args.executable.resolve(), args.program.resolve(), args.game.resolve(), root / "second")
            workers.append(second)
            require(first.process.pid != second.process.pid, "Engine states share a process")
            second.send({"command": "create", "homeID": 5346, "awayID": 1953, "seed": 67890})
            other = second.state(status="paused", engineUnits=0)
            lineup(other)
            require({p["staffID"] for p in other["players"]}.isdisjoint({p["staffID"] for p in initial["players"]}), "Different clubs share player selection")
            # The original injury/history path first queries its manager on
            # this seed's 86th call. Cover that concrete bootstrap regression.
            for tick in range(1, 101):
                second.send({"command": "step"})
                second.state(status="paused", stepCount=tick, sharedModernCalls=tick)
            first.send({"command": "inspect"})
            retained = first.state(status="paused")
            require(retained["players"] == stable["players"] and retained["stepCount"] == stable["stepCount"], "Another worker changed the first experiment")
            first.send({"command": "reset"})
            reset = first.state(status="paused", engineUnits=0, stepCount=0)
            require(reset["players"] == initial["players"] and reset["seed"] == initial["seed"] and
                    reset["homeScore"] == initial["homeScore"] and reset["awayScore"] == initial["awayScore"], "Seeded reset changed initial setup")
            require(reset["requestID"] == first.last_request, "Reset response correlation")
            require(len(list(first.root.iterdir())) == 1, "Reset retained obsolete experiment backing")
            second.close(eof=True)
            first.close()
            print("PASS: two actual pairings/processes, named starting elevens, productive C++23 steps, run/pause, rejected commands, seeded reset, independent state, immediate old-backing cleanup, normal quit/EOF cleanup")
        finally:
            for worker in workers:
                if worker.process.poll() is None:
                    worker.process.terminate()
                    worker.process.wait(timeout=10)


if __name__ == "__main__":
    main()
