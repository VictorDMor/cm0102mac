// The original's sort (0x10768300 and its three heapsorts), as C++.
//
// What a caller can observe besides the sorted rows, and so what stays:
// - the order of the comparator calls and of the swaps: comparators are game
//   routines with frames of their own, and ties end where the original's do;
// - the prologue image (saved r21-r31, LR, back chain): the stack audit lists
//   fifteen readers of what 0x107684c0 leaves there, in the match engine, the
//   weather, the league and the finances (tools/stack_contract.py module
//   qsort4x.c), and the other two sorts keep the same image;
// - r21-r31 at each comparator call, which a comparator's prologue saves below
//   that image, and r3-r8, cr0 and the carry as a swap leaves them, which the
//   caller gets back at the return.
// r0 is kept only at the assertion call: the comparator glue and the epilogue
// overwrite it everywhere else.
#include "qsort4x.hpp"

namespace cm::aot::qsort4x {
namespace {
using guest::Cpu;

constexpr std::uint32_t comparator_glue = 0x10781320, failed_assertion = 0x107755c0;

// What differs between the three heapsorts.
struct Kind {
    std::uint32_t entry;
    std::uint32_t unit;                          // bytes a swap moves per step
    std::uint32_t after_sibling, after_parent;   // return addresses of the two comparator calls
    std::int32_t assertion = 0;                  // TOC slot of "(member_size & N) == 0", then the file name
    std::uint32_t after_assertion = 0;
};
constexpr Kind bytes{0x10768340, 1, 0x1076842c, 0x10768450};
constexpr Kind words{0x107684c0, 4, 0x107685cc, 0x107685f0, -0x3c04, 0x10768504};
constexpr Kind pairs{0x10768660, 8, 0x10768774, 0x10768798, -0x3c00, 0x107686a4};

class Heapsort {
public:
    Heapsort(Cpu& cpu, const Kind& kind, std::uint32_t rows, std::uint32_t count, std::uint32_t width,
             std::uint32_t compare)
        : c(cpu), kind(kind), rows(rows), count(count), width(width), compare(compare), sibling_at(cpu.r(21)) {}

    void run() {
        guest::Frame frame(c, kind.entry, 0x70, {.first_gpr = 21});
        if (kind.assertion) {
            sibling_at = count;  // r21 holds the count until the first sibling
            c.set(4, c.toc(kind.assertion));
        }
        c.compare_logical(0, count, 2);
        if (count < 2) return;
        if (kind.assertion) check_width();

        left = (count >> 1) + 1;
        right = count;
        left_at = rows + width * (left - 1);
        right_at = rows + width * (count - 1);
        for (;;) {
            c.compare_logical(0, left, 1);
            if (left > 1) {
                // Building the heap: the next row down becomes a root.
                left_at -= width;
                --left;
            } else {
                // The heap is built: its top is the largest row left, and goes to the end.
                swap(right_at, left_at);
                --right;
                c.compare_logical(0, right, 1);
                if (right == 1) return;
                right_at -= width;
            }
            // What a comparator's prologue may save of the heap's bounds.
            c.set({{24, right_at}, {25, left_at}, {27, right}, {28, left}, {29, rows}, {30, width}, {31, compare}});
            sift_down();
        }
    }

private:
    // Moves the row at `left` down until neither child sorts after it.
    void sift_down() {
        node = left;
        node_at = rows + width * (left - 1);
        for (;;) {
            c.compare_logical(0, node << 1, right);
            if ((node << 1) > right) return;
            parent_at = node_at;
            node *= 2;
            node_at = rows + width * (node - 1);
            c.compare_logical(0, node, right);
            if (node < right) {
                sibling_at = node_at + width;
                if (precedes(node_at, sibling_at, kind.after_sibling)) {
                    node_at = sibling_at;
                    ++node;
                }
            }
            if (!precedes(parent_at, node_at, kind.after_parent)) return;
            swap(node_at, parent_at);
        }
    }

    // compare(first, second) < 0, through the comparator's transition vector.
    bool precedes(std::uint32_t first, std::uint32_t second, std::uint32_t back) {
        c.set({{21, sibling_at}, {22, node_at}, {23, parent_at}, {26, node}});
        c.set({{3, first}, {4, second}, {12, compare}});
        c.call(comparator_glue, back);
        c.set(2, c.u32(c.sp() + 0x14));
        const auto order = std::int32_t(c.r(3));
        c.compare(0, order, 0);
        return order < 0;
    }

    // Exchanges two rows a unit at a time. The byte sort moves `width` bytes;
    // the other two move whole units and leave a remainder of the width alone.
    // The loop counts down from steps + 1, as the original's does.
    void swap(std::uint32_t first, std::uint32_t second) {
        auto remaining = (kind.unit == 1 ? width : kind.unit == 4 ? width >> 2 : width >> 3) + 1;
        if (kind.unit == 8) {
            while (--remaining != 0) {
                const auto second_high = c.u32(second), second_low = c.u32(second + 4);
                const auto first_high = c.u32(first), first_low = c.u32(first + 4);
                c.put32(second + 4, first_low);
                c.put32(second, first_high);
                c.put32(first + 4, second_low);
                c.put32(first, second_high);
                first += 8;
                second += 8;
                c.set({{3, first_low}, {7, second_high}, {8, second_low}});
            }
            c.set({{4, first - 8}, {5, second - 8}, {6, 0}});
        } else if (kind.unit == 4) {
            while (--remaining != 0) {
                const auto moved = c.u32(second);
                c.put32(second, c.u32(first));
                c.put32(first, moved);
                first += 4;
                second += 4;
                c.set(6, moved);
            }
            c.set({{3, first - 4}, {4, second - 4}, {5, 0}});
        } else {
            while (--remaining != 0) {
                const auto moved = c.i8(second);
                c.put8(second, c.u8(first));
                c.put8(first, std::uint8_t(moved));
                ++first;
                ++second;
                c.set(6, std::uint32_t(std::int32_t(moved)));
            }
            c.set({{3, first - 1}, {4, second - 1}, {5, 0}});
        }
        // `subic.` reaching zero: cr0 equal, carry set.
        c.compare(0, 0, 0);
        c.s.wr<1>(guest::reg::xer_ca, 1);
    }

    // The word and pair sorts assert that the width is a multiple of their
    // unit. The entry never sends them another width, and nothing else calls
    // them; the report is here because the original has it.
    void check_width() {
        const auto remainder = width & (kind.unit - 1);
        c.compare(0, std::int32_t(remainder), 0);
        if (remainder == 0) return;
        const auto text = c.r(4);
        c.set({{21, count}, {29, rows}, {30, width}, {31, compare}});
        c.set({{0, remainder}, {3, text}, {4, text + 0x17}, {5, 0x78}});
        c.call(failed_assertion, kind.after_assertion);
    }

    Cpu& c;
    const Kind& kind;
    const std::uint32_t rows, count, width, compare;
    // The heap is rows 1..right (1-based); rows left..right already satisfy
    // it. Each `_at` is the address of that row.
    std::uint32_t left = 0, right = 0, left_at = 0, right_at = 0;
    std::uint32_t node = 0, node_at = 0, parent_at = 0;
    std::uint32_t sibling_at;  // r21: the caller's value (or the count) until a node has a sibling
};
}

void sort_by_bytes(Cpu& c, std::uint32_t rows, std::uint32_t count, std::uint32_t width, std::uint32_t compare) {
    Heapsort(c, bytes, rows, count, width, compare).run();
}
void sort_by_words(Cpu& c, std::uint32_t rows, std::uint32_t count, std::uint32_t width, std::uint32_t compare) {
    Heapsort(c, words, rows, count, width, compare).run();
}
void sort_by_pairs(Cpu& c, std::uint32_t rows, std::uint32_t count, std::uint32_t width, std::uint32_t compare) {
    Heapsort(c, pairs, rows, count, width, compare).run();
}

void sort(Cpu& c, std::uint32_t, std::uint32_t, std::uint32_t width, std::uint32_t) {
    // The arguments stay in r3-r6 for the sort chosen here.
    guest::Frame frame(c, 0x10768300, 0x40, {});
    // The pair sort is called through its page, where the fused 8-byte sort
    // (tools/native_sort8.py) answers first for the comparators it knows.
    if ((width & 7) == 0) c.call(pairs.entry, 0x10768318);
    else if ((width & 3) == 0) call(c, words.entry, 0x10768328);
    else call(c, bytes.entry, 0x10768330);
}
}
