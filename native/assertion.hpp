#pragma once
// The assertion helpers (tools/assertion_audit.py): the two routines every assertion of the game ends in, rewritten as typed C++.
//
//  * 0x10739890 splits the source file's name an assertion site passes (a Mac path, ':' separated, e.g. "Disk:Folder:FILE.CPP") into four
//    stack buffers and copies them out: volume, directory, name and extension (each output pointer may be null). All 6,486 call sites of it
//    are the first call of an assertion report (guest_abi.hpp, report_assertion) or sit next to one; none of its callers is a pointer.
//  * 0x107755c0 is the C library's assertion reporter (the font engine's units, POL_CUP and QSORT4X call it): it prints
//    "Assertion (%s) failed in "%s" on line %d\n" to the error stream and aborts.
//
// Each function is one routine of the original; the adapter (assertion_adapter.cpp, assertion_routines.inc) reads the guest ABI's
// arguments, calls it and leaves the registers the original leaves. They are native inside and keep the guest ABI at their edge: the
// original's frames are built exactly (the game's printf reads their dead bytes), every callee is called with `c.call` at the original's
// return address and with the registers the original holds there, and r3-r10 are left as the original leaves them.
#include "guest_abi.hpp"

namespace cm::aot::assertion {
using guest::Cpu;

// 0x10739890: splits the file name `path` (null: nothing is copied out) at its last ':' into the directory part (its volume, the text up to
// the first ':', and the directory after it, a trailing ':' cut) and the file name (its extension, from the last '.', and the name before
// it), and copies each part to its output buffer when that is not null.
void split_file_name(Cpu&, std::uint32_t path, std::uint32_t volume, std::uint32_t directory, std::uint32_t name, std::uint32_t extension);
// 0x107755c0: prints "Assertion (condition) failed in "file" on line N\n" to the error stream (fprintf 0x1077b610) and aborts (0x10773fb0).
void report_failed_assertion(Cpu&, std::uint32_t condition, std::uint32_t file, std::uint32_t line);
}
