// DATABASE.CPP's search block, part 4: the pass over every club that moves the people
// of its linked club (database_search.hpp).
#include "database_search.hpp"
#include "database_records.hpp"
#include "database_search_detail.hpp"
#include "guest_values.hpp"

namespace cm::aot::database {
namespace {
using guest::compare;
using guest::compare_logical;
constexpr std::uint32_t club_stride = 0x248;
}

void relink_club_squads(Cpu& c) {
    guest::Frame frame(c, 0x100d3f30, 0x70, {.first_gpr = 22});
    const auto table_slot = c.toc(search::slot::club_table);
    c.set(3, table_slot);
    const auto order_slot = c.toc(-0x7980), kind_word = c.toc(-0x7b80), count_word = c.toc(search::slot::club_count),
               limit_word = c.toc(-0x7be4);
    guest::keep(c, {{28, order_slot}, {29, kind_word}, {30, count_word}, {31, limit_word}, {24, 0}});
    guest::keep(c, {{27, c.u32(table_slot)}});
    const auto found = frame.at(0x38);
    // Walks `count` slots of the list at `offset` of `owner`: a slot with a person whose id is below
    // the global is passed to `hit` (the original keeps the person in r22 for the short lists).
    const auto walk = [&](std::uint32_t owner, std::uint32_t offset, unsigned count, bool in_r22, auto&& hit) {
        for (std::uint32_t i = 0;; ++i) {
            if (compare(c, std::int32_t(i), std::int32_t(count)) >= 0) break;
            c.set(23, i);
            c.set(0, i << 2);
            c.set(3, owner + (i << 2));
            const auto person = c.u32(owner + (i << 2) + offset);
            c.set(in_r22 ? 22 : 0, person);
            if (compare_logical(c, person) == 0) continue;
            if (!in_r22) {
                c.set(3, i << 2);
                c.set(0, c.u32(limit_word));
                c.set(3, owner + (i << 2));
                c.set(3, person);
            }
            const auto id = c.u32(person);
            c.set(3, id);
            if (in_r22) c.set(0, c.u32(limit_word));
            if (compare(c, std::int32_t(id), std::int32_t(c.r(0))) >= 0) continue;
            hit(person);
        }
        c.set(23, count);
    };
    const auto remove = [&](std::uint32_t club, std::uint32_t person, std::uint32_t back) {
        guest::enter(c, back, {club, person, 0, 1, 0});
        remove_from_record(c, club, person, 0, 1, 0);
    };
    for (std::uint32_t index = 0;; ++index) {
        const auto count = c.u32(count_word);
        c.set(0, count);
        if (compare(c, std::int32_t(index), std::int32_t(count)) >= 0) break;
        const auto record = c.r(27);
        guest::enter(c, 0x100d3f70, {record, found, 1});
        const auto target = club_of_code(c, record, found, 1);
        c.set(26, target);
        if (compare(c, std::int32_t(target)) == 0) goto next;
        {
            const auto flag = std::uint32_t(std::int32_t(c.i8(found)));
            c.set(0, flag);
            if (compare(c, std::int32_t(flag)) == 0) goto next;
        }
        {
            std::uint32_t listed = 0;
            c.set(25, 0);
            walk(record, 0x98, 0x32, false, [&](std::uint32_t) { c.set(25, ++listed); });
            c.set({{6, order_slot}, {3, target + 0x98}, {4, 0x32}, {5, 4}});
            c.call(search::qsort, 0x100d3fe4);
            walk(target, 0x98, 0x32, false, [&](std::uint32_t person) {
                c.set(25, ++listed);
                if (compare(c, std::int32_t(listed), 0x32) > 0) {
                    remove(target, person, 0x100d4048);
                }
            });
            // The shorter lists: the club's own people count first (no more than the list holds),
            // then the linked club's are removed and, while room is left, added to this club.
            struct Short { std::uint32_t offset, size, kind, remove_back, add_back; };
            for (const Short list : {Short{0x160, 5, 8, 0x100d40e4, 0x100d410c}, Short{0x174, 7, 9, 0x100d41a8, 0x100d41d0},
                                     Short{0x190, 3, 10, 0x100d426c, 0x100d4294}}) {
                listed = 0;
                c.set(25, 0);
                walk(record, list.offset, list.size, false, [&](std::uint32_t) { c.set(25, ++listed); });
                walk(target, list.offset, list.size, true, [&](std::uint32_t person) {
                    remove(target, person, list.remove_back);
                    if (compare(c, std::int32_t(listed), std::int32_t(list.size)) < 0) {
                        c.set(25, ++listed);
                        guest::enter(c, list.add_back, {record, person, list.kind, kind_word, 1, 0});
                        add_to_record(c, record, person, list.kind, kind_word, 1, 0);
                    }
                });
            }
            // The two single slots.
            const auto mine = c.u32(record + 0x90);
            c.set(0, mine);
            const auto theirs = c.u32(target + 0x90);
            if (compare_logical(c, mine) != 0 && (c.set(0, theirs), compare_logical(c, theirs) != 0)) {
                c.set(4, c.u32(target + 0x90));
                remove(target, c.r(4), 0x100d42d0);
            } else {
                c.set(0, theirs);
                if (compare_logical(c, theirs) != 0) {
                    const auto person = c.u32(target + 0x90);
                    c.set(22, person);
                    remove(target, c.u32(target + 0x90), 0x100d42fc);
                    const auto kind = std::uint32_t(c.u8(person + 0x62));
                    guest::enter(c, 0x100d4318, {record, person, kind, 0, 1, 0});
                    add_to_record(c, record, person, kind, 0, 1, 0);
                }
            }
            const auto mine94 = c.u32(record + 0x94);
            c.set(0, mine94);
            if (compare_logical(c, mine94) != 0) {
                const auto theirs94 = c.u32(target + 0x94);
                c.set(0, theirs94);
                if (compare_logical(c, theirs94) != 0) {
                    c.set(4, c.u32(target + 0x94));
                    remove(target, c.r(4), 0x100d4348);
                }
            }
        }
    next:
        c.set(24, index + 1);
        c.set(27, c.r(27) + club_stride);
    }
}
}
