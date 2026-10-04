"""Store identical private Date gzip bytes in an optional lossless xz archive.

Only the host storage changes. Every native replay receives the complete original
gzip through a fresh FIFO and performs its existing framing, ABI and EOF checks.
"""
from contextlib import contextmanager
import errno
import hashlib
import lzma
import os
from pathlib import Path
import subprocess
import threading


class DateOracle:
    def __init__(self, path, archived=False):
        self.path = Path(path)
        self.archived = archived
        self.archive = self.path.with_suffix(self.path.suffix + ".xz")
        self.sequence = 0

    @property
    def stored_path(self):
        return self.archive if self.archived else self.path

    @contextmanager
    def raw_bytes(self):
        opener = lzma.open if self.archived else open
        try:
            with opener(self.stored_path, "rb") as source:
                yield source
        except (EOFError, lzma.LZMAError) as error:
            raise ValueError("Invalid lossless Date archive") from error

    def digest(self):
        digest = hashlib.sha256()
        with self.raw_bytes() as source:
            for block in iter(lambda: source.read(1024 * 1024), b""):
                digest.update(block)
        return digest.hexdigest()

    def run(self, command, *, timeout=7200):
        if not self.archived:
            return subprocess.run(command, capture_output=True, text=True, timeout=timeout)
        flag = "--date-record" if "--date-record" in command else "--date-check"
        position = command.index(flag) + 1
        if Path(command[position]) != self.path:
            raise ValueError("Date oracle path differs from the command")
        recording = flag == "--date-record"
        if recording and self.archive.exists():
            raise FileExistsError(self.archive)
        self.sequence += 1
        pipe = self.path.parent / f".date-{self.sequence:04d}.pipe"
        os.mkfifo(pipe, 0o600)
        stop = threading.Event()
        errors = []

        def record():
            descriptor = os.open(pipe, os.O_RDONLY | os.O_NONBLOCK)
            received = False
            try:
                with lzma.open(self.archive, "wb", preset=6) as destination:
                    while True:
                        try:
                            block = os.read(descriptor, 1024 * 1024)
                        except BlockingIOError:
                            if stop.is_set():
                                break
                            stop.wait(0.01)
                            continue
                        if block:
                            received = True
                            destination.write(block)
                        elif received or stop.is_set():
                            break
                        else:
                            stop.wait(0.01)
                if not received:
                    raise RuntimeError("Date reference capture produced no gzip bytes")
            finally:
                os.close(descriptor)

        def replay():
            descriptor = None
            while not stop.is_set():
                try:
                    descriptor = os.open(pipe, os.O_WRONLY | os.O_NONBLOCK)
                    break
                except OSError as error:
                    if error.errno != errno.ENXIO:
                        raise
                    stop.wait(0.01)
            if descriptor is None:
                return
            os.set_blocking(descriptor, True)
            try:
                with os.fdopen(descriptor, "wb", buffering=0) as destination, self.raw_bytes() as source:
                    for block in iter(lambda: source.read(1024 * 1024), b""):
                        view = memoryview(block)
                        while view:
                            written = destination.write(view)
                            if not written:
                                raise RuntimeError("Date FIFO made no write progress")
                            view = view[written:]
            except BrokenPipeError:
                # A negative replay closes at its first own divergence. The
                # caller still requires the exact native rejection diagnostic.
                pass

        def worker():
            try:
                (record if recording else replay)()
            except BaseException as error:
                errors.append(error)

        thread = threading.Thread(target=worker, daemon=True)
        thread.start()
        arguments = list(command)
        arguments[position] = str(pipe)
        try:
            result = subprocess.run(arguments, capture_output=True, text=True, timeout=timeout)
        finally:
            stop.set()
            thread.join(60)
            pipe.unlink()
        if thread.is_alive():
            raise RuntimeError("Date storage worker did not finish")
        if errors:
            raise errors[0]
        return result
