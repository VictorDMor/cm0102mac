#include "news_detail.hpp"

namespace cm::aot::news {
using namespace detail;

std::uint32_t destroy_queue(Cpu& c, std::uint32_t address, std::uint32_t deleting) {
    Frame frame(c, 0x102cbb70, 0x40, {.first_gpr = 30});
    Kept<30> queue(c, address); Kept<31> flag(c, deleting);
    Program program{c};
    if (compare(c, queue) != 0) {
        Queue records{c, queue};
        if (compare_logical(c, records.items()) != 0) {
            program.call(0x10775170, 0x102cbba4, {records.items()});
            c.set(3, 0); records.items() = 0; records.item_count() = 0;
        }
        if (compare_logical(c, records.managers()) != 0) {
            program.call(0x10775170, 0x102cbbc8, {records.managers()});
            c.set(3, 0); records.managers() = 0; records.manager_count() = 0;
        }
        if (compare(c, std::int16_t(flag)) > 0) program.call(0x107735a0, 0x102cbbe8, {queue});
    }
    return queue;
}

std::uint32_t find_item(Cpu& c, std::uint32_t address, std::uint32_t manager,
                        std::uint32_t id, std::uint32_t destination) {
    Frame frame(c, 0x102ccdd0, 0xc0, {.first_gpr = 29});
    keep(c, {{29, c.toc(slot::success)}, {30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    if (compare(c, id) == 0) {
        assertion(c, frame, 0x102cce10, 0x90, 0x48, 0x2252); return 0;
    }
    if (compare_logical(c, destination) == 0) {
        assertion(c, frame, 0x102cce70, 0x70, 0x40, 0x225a); return 0;
    }
    Queue queue{c, address};
    if (compare_logical(c, manager) != 0) {
        c.set(7, c.toc(slot::first_manager));
        c.set(4, c.u32(manager) - c.u32(c.r(7)));
        const auto index = c.r(4) + 16;
        if (compare(c, index) < 0) {
            assertion(c, frame, 0x102ccee8, 0x50, 0x38, 0x2268); return 0;
        }
        c.set(4, queue.managers()); c.set(8, c.r(4) + index * Inbox::size);
        Inbox inbox{c, c.r(8)};
        if (compare(c, inbox.first(), -1) == 0) return 0;
        c.set(7, 0);
        for (unsigned offset = 0; compare(c, offset, Inbox::capacity) < 0; ++offset) {
            c.set(7, offset);
            c.set(4, queue.item(std::uint32_t(inbox.first()) + offset));
            if (compare(c, c.u32(c.r(4) + 0xdc), id) != 0) continue;
            copy_item(c, destination, c.r(4), 3, 5, 7); return 1;
        }
        c.set(7, Inbox::capacity);
    } else {
        c.set(7, 0);
        unsigned index = 0;
        for (; compare(c, index, queue.item_count()) < 0; ++index) {
            c.set(7, index); c.set(4, queue.item(index));
            if (compare(c, c.u32(c.r(4) + 0xdc), id) != 0) continue;
            copy_item(c, destination, c.r(4), 3, 5, 7); return 1;
        }
        c.set(7, index);
    }
    return 0;
}

std::uint32_t update_word(Cpu& c, std::uint32_t address, std::uint32_t manager,
                          std::uint32_t id, std::uint32_t index, std::uint32_t value,
                          std::uint32_t current_item) {
    Frame frame(c, 0x102cd0c0, 0x170, {.first_gpr = 24});
    Kept<24> queue_address(c, address); Kept<25> recipient(c, manager);
    Kept<26> news_id(c, id); Kept<27> position(c, index); Kept<28> replacement(c, value);
    keep(c, {{29, c.toc(slot::success)}, {30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    if (compare(c, recipient) == 0) {
        assertion(c, frame, 0x102cd108, 0x130, 0x68, 0x22b1); return 0;
    }
    if (compare(c, news_id) == 0) {
        assertion(c, frame, 0x102cd168, 0x110, 0x60, 0x22b9); return 0;
    }
    if (compare(c, position) < 0 || compare(c, position, Item::payload_words) >= 0) {
        assertion(c, frame, 0x102cd1d0, 0xf0, 0x58, 0x22c1); return 0;
    }
    if (compare_logical(c, current_item) != 0) {
        if (compare(c, news_id, c.u32(current_item + 0xdc)) != 0) {
            assertion(c, frame, 0x102cd2b8, 0xd0, 0x50, 0x22d0); return 0;
        }
        // This is an inline copy of the original's payload setter, with no
        // nested guest frame or call. The full-width check above implies the
        // original's repeated signed-byte slot validation always succeeds.
        compare(c, std::int8_t(position)); compare(c, std::int8_t(position), 50);
        c.set(3, current_item + std::uint32_t(position) * 4);
        c.put32(c.r(3) + 8, replacement);
    }
    c.set(3, c.u32(recipient) - c.u32(c.toc(slot::first_manager)));
    const auto inbox_index = c.r(3) + 16;
    if (compare(c, inbox_index) < 0) {
        assertion(c, frame, 0x102cd328, 0xb0, 0x48, 0x22dc); return 0;
    }
    Queue queue{c, queue_address}; c.set(3, queue.managers());
    c.set(5, c.r(3) + inbox_index * Inbox::size); Inbox inbox{c, c.r(5)};
    if (compare(c, inbox.first(), -1) == 0) return 0;
    c.set(4, 0);
    for (unsigned offset = 0; compare(c, offset, Inbox::capacity) < 0; ++offset) {
        c.set(4, offset); c.set(3, queue.items());
        c.set(6, c.r(3) + (std::uint32_t(inbox.first()) + offset) * Item::size);
        if (compare(c, c.u32(c.r(6) + 0xdc), news_id) != 0) continue;
        c.set(4, c.r(6) + std::uint32_t(position) * 4); c.put32(c.r(4) + 8, replacement); return 1;
    }
    c.set(4, Inbox::capacity);
    assertion(c, frame, 0x102cd3e8, 0x90, 0x40, 0x22f7); return 0;
}

std::uint32_t unread_count(Cpu& c, std::uint32_t address, std::uint32_t manager,
                           std::uint32_t filter) {
    Frame frame(c, 0x102cd440, 0xa0, {.first_gpr = 26});
    Kept<30> queue_address(c, address); Kept<31> requested_category(c, filter);
    keep(c, {{27, c.toc(slot::success)}, {28, c.toc(slot::message)}, {29, c.toc(slot::pool)}});
    if (compare_logical(c, manager) == 0) {
        assertion(c, frame, 0x102cd480, 0x68, 0x40, 0x230e); return 0;
    }
    c.set(3, c.u32(manager) - c.u32(c.toc(slot::first_manager)));
    const auto index = c.r(3) + 16;
    if (compare(c, index) < 0) {
        assertion(c, frame, 0x102cd4f0, 0x48, 0x38, 0x2319); return 0;
    }
    Queue queue{c, queue_address}; c.set(3, queue.managers());
    Kept<29> inbox_address(c, c.r(3) + index * Inbox::size); Inbox inbox{c, inbox_address};
    if (compare(c, inbox.unread()) < 0) inbox.unread() = 0;
    if (compare(c, std::int8_t(requested_category)) == 0) return inbox.unread();
    if (compare(c, inbox.first(), -1) == 0) return 0;
    Kept<27> count(c, 0); Kept<28> offset(c, 0);
    while (compare(c, offset, Inbox::capacity) < 0) {
        Kept<26> item_address(c, queue.item(std::uint32_t(inbox.first()) + std::uint32_t(offset)));
        if (compare(c, c.i32(item_address + 0xdc)) != 0) {
            enter(c, 0x102cd5ac, {item_address}); c.set(3, category(c, item_address));
            c.set(3, std::uint32_t(std::int8_t(c.r(3))));
            if (compare(c, std::int8_t(requested_category), c.r(3)) == 0 &&
                compare(c, c.i8(item_address + 0xe0)) == 0) count = count + 1;
        }
        offset = offset + 1;
    }
    return count;
}

std::uint32_t club_enabled(Cpu& c, std::uint32_t, std::uint32_t club) {
    Frame frame(c, 0x102cdbf0, 0x40, {.first_gpr = 31});
    Kept<31> team(c, club);
    if (compare(c, team) == 0) return 0;
    Program program{c};
    const auto controlled = program.call(0x1014c2e0, 0x102cdc28, {team, 1, 1, 0, 0});
    if (compare(c, std::int8_t(controlled)) == 0) return 0;
    const auto nation = program.call(0x100c96e0, 0x102cdc40, {team});
    c.set(5, c.toc(-0x7a54));
    const auto enabled = program.call(0x10147e70, 0x102cdc50, {c.u32(c.r(5)), nation});
    compare(c, std::int8_t(enabled)); c.set(4, 1);
    // Both arms of the original final branch return one, while retaining
    // the call and its CR effects.
    return 1;
}
}
