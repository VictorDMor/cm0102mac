// MANAGER_MANAGER.CPP lists block (manager_manager_lists.hpp).
#include "manager_manager_lists.hpp"
#include "guest_values.hpp"
#include "list_search.hpp"
#include "manager_manager_detail.hpp"

namespace cm::aot::manager_manager {
using guest::Cpu;
namespace {
namespace container { constexpr std::uint32_t list = 0x8; }
namespace element { constexpr std::uint32_t key = 0x8; }
}

// 0x101b1620. Frame 0x80 (stmw r26-r31 at sp-0x18, LR at sp+8): the saved
// registers are the stack image a later reader takes (0x105c4d74, per the
// stack audit). The original keeps container, key, the index, the saved cursor,
// the assert buffer and the pool in r26-r31; they reach memory only in the
// error report's callees, so they are set there. list_next (0x10738e00, a
// frameless leaf) is followed inline: it leaves r5 the node, r4 its data, r0
// the next node, and records the TOC at its bl.
std::uint32_t list_contains(Cpu& c, std::uint32_t container_record, std::uint32_t key) {
    // The common case (list_search.cpp, checked by list_search_checks): every read
    // traced first, then the frame, the cursor restore and the registers written,
    // without the cursor's intermediate writes. It steps aside, with no effect, when
    // reads are observed, a write observer watches the cursor, a write would alias a
    // later read, or the list holds a null element; the search below covers those.
    if (try_native_list_search(c.s)) return c.r(3);
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, 0x101b1620, 0x80, {.first_gpr = 26});
    lmw_leaves(c, frame.caller());  // where the epilogue's lmw r26,-0x18(r1) leaves it on every exit
    // The prologue's registers, set before the first load that can fault (a bad list).
    const auto pool = c.toc(slots::pool);
    guest::keep(c, {{26, container_record}, {30, c.toc(slots::message)}, {31, pool}, {27, key}, {28, 0}});
    const auto list_at = [&] { return c.u32(container_record + container::list); };  // lwz r3,8(r26)
    const auto first = list_at();
    c.set(3, first);
    const auto saved = c.u32(first + list::cursor);
    c.set(29, saved);
    const auto head = c.u32(first + list::head);
    c.set(0, head);
    c.put32(first + list::cursor, head);
    std::int32_t index = 0;
    for (;; ++index) {
        c.set(28, std::uint32_t(index));
        const auto current = list_at();
        c.set(3, current);
        const std::int32_t count = c.i16(current + list::count);
        c.set(0, std::uint32_t(count));
        guest::compare(c, index, count);  // cmpw r28,r0
        if (index >= count) {
            // Falls through: the cursor back, false.
            c.put32(current + list::cursor, saved);
            return guest::result(c, 0);
        }
        // list_next(current), inline (bl at 0x101b1658; the epilogue restores LR).
        c.s.wr<4>(guest::reg::r2_save, c.r(2));
        c.set_lr(0x101b165c);
        const auto node = c.u32(current + list::cursor);
        c.set(5, node);
        guest::compare_logical(c, node);  // cmplwi r5,0
        std::uint32_t data = 0;
        if (node) {
            data = c.u32(node + list_node::data);
            c.set(4, data);
            const auto next = c.u32(node + list_node::next);
            c.set(0, next);
            c.put32(current + list::cursor, next);
        }
        c.set(3, data);
        guest::compare_logical(c, data);  // cmplwi r3,0
        if (!data) {
            // A null element (or the list's end before its count): the original
            // puts the cursor back and asserts (line 0x3713), then answers false.
            c.set(4, list_at());
            c.put32(c.r(4) + list::cursor, saved);
            guest::report_assertion(c, {.at = 0x101b1680, .file = pool + 0x37, .format = pool + 0x4b, .title = pool + 0x58,
                                        .name = frame.at(0x40), .extension = frame.at(0x38), .line = 0x3713});
            c.set({{4, c.toc(slots::success)}, {0, 0}});
            lmw_leaves(c, frame.caller());  // the report's callees moved it
            return guest::result(c, 0);
        }
        const auto found = std::int32_t(c.u32(data + element::key));
        c.set(3, std::uint32_t(found));
        const auto wanted = std::int32_t(c.u32(key));
        c.set(0, std::uint32_t(wanted));
        guest::compare(c, found, wanted);  // cmpw r3,r0
        if (found == wanted) {
            const auto current_now = list_at();
            c.set(4, current_now);
            c.put32(current_now + list::cursor, saved);
            return guest::result(c, 1);
        }
    }
}
}
