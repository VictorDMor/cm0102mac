#pragma once
// MANAGER_MANAGER.CPP people text block: the decision data of the text chooser 0x101980a0. The chooser
// is a tree of three-way compares of one byte or halfword against constants; each node compares as
// the original's `cmpwi` does (cr0 holds the last one when the routine returns or calls) and goes on
// by less / equal / greater. The same shape is the second band's and the relation's dispatch, and each
// table cell's test of the byte the third helper leaves at sp+0x39.
#include "guest_values.hpp"
#include <cstdint>
#include <span>

namespace cm::aot::manager_manager::people_text {
// A node's way on: >= 0 is another node of the script, < 0 is ~exit (an exit of the script, or `end`).
using Out = std::int8_t;
constexpr Out node(int index) { return Out(index); }
constexpr Out leaf(int exit) { return Out(~exit); }
constexpr int none = 15;      // the exit "no text from this script"
constexpr Out end = leaf(none);
struct Node {
    std::int16_t k;  // the constant compared with
    Out less, equal, greater;
};
// Runs a script over `value` (a signed compare at each node); the exit it reaches.
inline int run(guest::Cpu& c, std::span<const Node> script, std::int32_t value) {
    std::size_t at = 0;
    for (;;) {
        const auto& n = script[at];
        const auto order = guest::compare(c, value, n.k);
        const Out next = order < 0 ? n.less : order == 0 ? n.equal : n.greater;
        if (next >= 0) at = std::size_t(next);
        else return ~next;
    }
}

// One sentence the chooser formats: the format's offset in the unit's text pool (TOC -0x5368), the
// return address after the original's call, and whether the byte at sp+0x38 is an extra argument.
struct Leaf {
    std::uint16_t format;
    std::uint32_t back;
    bool with_count = false;
};
// A jump-table case: the table's target address (the original leaves it in r3), the script over
// sp+0x39 (empty: the first leaf, no test) and the leaves its exits name.
struct Cell {
    std::uint32_t entry;
    std::span<const Node> script;
    Leaf leaves[4];
};
}
