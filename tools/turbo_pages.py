"""Match pages compiled with a local Turbo instruction counter.

On these pages MatchPageCounter keeps the instruction count in a local and
writes it back when the page returns. A semantic hook that charges
instructions must therefore run inside counter.semantic_boundary, which
publishes the count first and adopts the hook's charges afterwards;
otherwise the page's write-back erases them.
"""
import re

TURBO_PAGES = frozenset({0x1071a, 0x10773, 0x1073c})
_PLAIN_HOOK = re.compile(r"if \((try_\w+\(s(?:, [^()]*)?\))\) return true;")


def boundary_hook(entry, call):
    return f"if (counter.semantic_boundary(0x{entry:x}, [&] {{ return {call}; }})) return true;"


def page_hook(page, entry, call):
    """The hook text the generator emits for `call` at `entry` on `page`."""
    return boundary_hook(entry, call) if page in TURBO_PAGES else f"if ({call}) return true;"


def bound_turbo_hook(entry, line):
    """Wrap a plain `if (try_...(s, ...)) return true;` hook line; leave other lines alone."""
    match = _PLAIN_HOOK.fullmatch(line)
    return boundary_hook(entry, match.group(1)) if match else line
