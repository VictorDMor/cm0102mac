// MANAGER_MANAGER.CPP tail block (manager_manager_tail.hpp), native-first: the news that manager moves file with the
// news desk, and the queue of moves that fall due. Values live in native locals; the guest ABI appears only at the
// calls to code outside the block.
#include "manager_manager_tail.hpp"
#include "manager_manager_assess_detail.hpp"
#include "manager_manager_detail.hpp"

namespace cm::aot::manager_manager {
using guest::Cpu;
namespace {
namespace toc_cell {
constexpr std::int32_t date = -0x7b80, people = -0x7be0, news_desk = -0x7bf0, effects = -0x7aa4, search = -0x7a60;
}
namespace callee {
constexpr std::uint32_t clear_message = 0x102bdf90, set_field = 0x102cb4b0, file_message = 0x102cc140,
                        file_message_b = 0x102cc280, file_message_c = 0x102cc400, apply_effect = 0x100e0410,
                        next_node = 0x10738e00, remove_node = 0x10738e60, append_record = 0x10738d10,
                        init_list = 0x10738c00, release_list = 0x10738c20, announce_prepare = 0x10191fd0,
                        copy_bytes = 0x10717a10, search_moves = 0x1048b180;
}
// A person record (a manager): the club (+0x24) with its competition (+0x2c, whose id is at +0x68) and the three
// record links (+0x28, +0x2c, +0x30: each record has its id at +0x20). A club record: its region link (+0x2c), its
// league link (+0x30), its manager (+0x90) and its reputation (a halfword).
namespace person_at {
constexpr std::uint32_t id = 0, club = 0x24, link_a = 0x28, link_b = 0x2c, link_c = 0x30;
}
namespace club_at { constexpr std::uint32_t region = 0x2c, league = 0x30, manager = 0x90, reputation = 0x1ec; }
// A node of the queue of moves (list[5]): the person's index, the kind byte, the date it falls due (a halfword pair),
// four effect codes at +0x10, and the effect's parameters.
namespace move_at {
constexpr std::uint32_t person = 0, kind = 4, day = 8, year = 0xa, codes = 0x10, p_a = 0x14, p_b = 0x15, p_c = 0x16,
                        p_d = 0x18, p_e = 0x1a, p_f = 0x1c;
}
constexpr std::uint32_t list_queue = 0x14;

void set_field(Cpu& c, std::uint32_t buffer, std::uint32_t index, std::uint32_t value, std::uint32_t back) {
    c.set({{3, buffer}, {4, index}, {5, value}});
    c.call(callee::set_field, back);
}
std::uint32_t byte_value(std::int8_t value) { return std::uint32_t(std::int32_t(value)); }  // extsb
}

std::uint32_t standing_allows(Cpu& c, std::uint32_t record, std::int8_t kind) {
    return kind == 2 && c.u8(record + 0x240) == 0x14 ? 0 : 1;
}

void post_move_news(Cpu& c, std::uint32_t person, std::int8_t first, std::int8_t second, std::int16_t third,
                    std::int8_t fourth) {
    guest::Frame frame(c, 0x101c1180, 0x140, {.first_gpr = 27});
    if (!person) return;
    const auto club = c.u32(person + person_at::club);
    if (!club || !c.u32(club + club_at::region)) return;
    const auto buffer = frame.at(0x38);
    c.set({{3, buffer}, {4, 0x178a}, {5, 0}});
    c.call(callee::clear_message, 0x101c11d4);
    set_field(c, buffer, 0, c.u32(person + person_at::id), 0x101c11e4);
    set_field(c, buffer, 1, c.u32(c.u32(person + person_at::link_a) + 0x20), 0x101c11f8);
    set_field(c, buffer, 2, c.u32(c.u32(person + person_at::link_b) + 0x20), 0x101c120c);
    set_field(c, buffer, 3, c.u32(c.u32(person + person_at::link_c) + 0x20), 0x101c1220);
    set_field(c, buffer, 4, byte_value(first), 0x101c1230);
    set_field(c, buffer, 5, c.u32(c.u32(club + club_at::region) + 0x68), 0x101c1248);
    set_field(c, buffer, 6, byte_value(second), 0x101c1258);
    set_field(c, buffer, 7, std::uint32_t(std::int32_t(third)), 0x101c1268);
    set_field(c, buffer, 8, byte_value(fourth), 0x101c1278);
    c.set({{3, c.u32(c.toc(toc_cell::news_desk))}, {4, buffer}, {5, c.u32(person + person_at::club)}});
    c.call(callee::file_message, 0x101c128c);
}

void post_rival_news(Cpu& c, std::uint32_t club, std::uint32_t other_a, std::uint32_t other_b) {
    guest::Frame frame(c, 0x101c1a10, 0x150, {.first_gpr = 23});
    assess::Draws draws(c);
    if (!club || !c.u32(club + club_at::league) || !c.u32(club + club_at::manager) || !other_b) return;
    const auto buffer = frame.at(0x38);
    c.set({{3, buffer}, {4, 0x178d}, {5, 0}});
    c.call(callee::clear_message, 0x101c1a80);
    const auto manager = c.u32(club + club_at::manager);
    set_field(c, buffer, 0, c.u32(manager), 0x101c1a94);
    set_field(c, buffer, 1, c.u32(c.u32(manager + person_at::link_a) + 0x20), 0x101c1aa8);
    set_field(c, buffer, 2, c.u32(c.u32(manager + person_at::link_b) + 0x20), 0x101c1abc);
    set_field(c, buffer, 3, c.u32(c.u32(manager + person_at::link_c) + 0x20), 0x101c1ad0);
    set_field(c, buffer, 4, c.u32(club), 0x101c1ae0);
    set_field(c, buffer, 5, c.u32(other_a), 0x101c1af0);
    set_field(c, buffer, 6, c.u32(other_b), 0x101c1b00);
    set_field(c, buffer, 7, std::uint32_t(draws.modulo(5, 0x101c1b20)), 0x101c1b64);
    const auto desk = c.u32(c.toc(toc_cell::news_desk));
    if (c.i16(club + club_at::reputation) < 0x1676) {
        c.set({{3, desk}, {4, buffer}, {5, club}});
        c.call(callee::file_message, 0x101c1b80);
    } else if (draws.modulo(3, 0x101c1ba4) != 0) {
        c.set({{3, desk}, {4, buffer}, {5, c.u32(club + club_at::league)}});
        c.call(callee::file_message_b, 0x101c1bec);
    } else {
        c.set({{3, desk}, {4, buffer}, {5, c.u32(club + club_at::region)}});
        c.call(callee::file_message_c, 0x101c1c00);
    }
}

std::uint32_t announce_move(Cpu& c, std::uint32_t list, std::uint32_t club) {
    guest::Frame frame(c, 0x101c1830, 0x80, {.first_gpr = 26});
    constexpr std::uint32_t first_copy = 0x44, second_copy = 0x4c, iterator = 0x54, queued = 0x40, args_a = 0x38, args_b = 0x3c;
    const auto date = c.toc(toc_cell::date);
    c.set(3, frame.at(iterator));
    c.call(callee::init_list, 0x101c1858);
    auto release = [&](std::uint32_t back) {
        c.set({{3, frame.at(iterator)}, {4, 0xffffffff}});
        c.call(callee::release_list, back);
    };
    if (!club || !c.u32(club + club_at::manager) || !c.u32(club + club_at::league)) {
        release(0x101c1884);
        return 0;
    }
    c.set(3, club);
    c.call(callee::announce_prepare, 0x101c1894);
    c.set({{3, frame.at(first_copy)}, {4, date}, {5, 8}});
    c.call(callee::copy_bytes, 0x101c18a4);
    const auto year = c.u16(frame.at(first_copy + 2));
    c.set({{3, frame.at(second_copy)}, {4, date}, {5, 8}});
    c.call(callee::copy_bytes, 0x101c18b8);
    // The search's two stack arguments (the ninth and tenth words) and its register arguments.
    c.put32(frame.at(args_a), 0xfffffffe);
    c.put32(frame.at(args_b), 1);
    c.set({{3, c.u32(c.toc(toc_cell::search))}, {4, c.u16(date)}, {5, c.u16(date + 2)}, {6, c.u16(frame.at(second_copy))},
           {7, year}, {8, c.u32(club)}, {9, frame.at(iterator)}, {10, 0xffffffff}});
    c.call(callee::search_moves, 0x101c18fc);
    std::uint32_t found = 0;
    if (c.i16(frame.at(iterator)) != 0) {
        c.put32(frame.at(iterator + 0xc), c.u32(frame.at(iterator + 4)));  // the cursor starts at the head
        for (;;) {
            c.set(3, frame.at(iterator));
            c.call(callee::next_node, 0x101c1968);
            const auto node = c.r(3);
                    if (!node) break;
            // A node due today (its year is the date's halfword, its day the other) is skipped, and so is one without
            // the flag byte (+0x40) or whose +0x43 byte is not -1.
            const bool today = std::int32_t(c.u16(node + 0x2a)) == std::int32_t(c.i16(date)) && c.u16(node + 0x28) == c.u16(date + 2);
            if (today || c.i8(node + 0x40) == 0 || c.i8(node + 0x43) != -1) continue;
            found = node;
            break;
        }
    }
    if (!found || !c.u32(found + 0x14)) {
        release(found ? 0x101c19a4 : 0x101c1984);
        return 0;
    }
    auto other = c.u32(found + 0x1c);
    if (other == club) other = c.u32(found + 0x20);
    post_rival_news(c, club, other, c.u32(found + 0x14));
    c.put32(frame.at(queued), c.u32(club));
    c.set({{3, c.u32(list + 0x10)}, {4, frame.at(queued)}, {5, 4}});
    c.call(callee::append_record, 0x101c19e8);
    release(0x101c19f4);
    return 1;
}

namespace {
// The original's order test of the move's date against today's: the year halfwords compare unsigned, equal
// ones go through a bit trick on the two sign-extended day halfwords whose result is its sign bit.
bool before_today(Cpu& c, std::uint32_t node, std::uint32_t date) {
    const std::uint32_t today_year = c.u16(date + 2), node_year = c.u16(node + move_at::year);
    if (today_year < node_year) return true;
    if (today_year > node_year) return false;
    const auto a = std::uint32_t(std::int32_t(c.i16(date))), b = std::uint32_t(std::int32_t(c.i16(node + move_at::day)));
    const auto x = a ^ b;
    const auto result = std::uint32_t(std::int32_t(x) >> 1) - (x & b);
    return (result >> 31) != 0;
}
}

void process_due_moves(Cpu& c, std::uint32_t list) {
    guest::Frame frame(c, 0x101c0fc0, 0xa0, {.first_gpr = 22});
    const auto date = c.toc(toc_cell::date);
    constexpr std::uint32_t assert_high = 0x50, assert_low = 0x48, effect_stack = 0x38;
    const auto queue = c.u32(list + list_queue);
    c.put32(queue + 0xc, c.u32(queue + 4));  // the cursor starts at the head
    for (std::int32_t index = 0; index < c.i16(c.u32(list + list_queue)); ++index) {
        c.set(3, c.u32(list + list_queue));
        c.call(callee::next_node, 0x101c1000);
        const auto node = c.r(3);
        if (!node) {
            assess::report_assertion(c, frame, assert_high, assert_low, 0x57e4, {0x101c1020, 0x101c1024, 0x101c1040, 0x101c1050});
            return;
        }
        if (before_today(c, node, date)) continue;
        const auto person = c.u32(c.toc(toc_cell::people)) + c.u32(node + move_at::person) * 0x70;
        if (c.i8(node + move_at::kind) != 0) {
            for (std::uint32_t k = 0; k < 4; ++k) {
                const auto code = c.u8(node + move_at::codes + k);
                if (std::int8_t(code) < 0) continue;
                c.put32(frame.at(effect_stack), c.u8(node + move_at::p_f));
                c.set({{3, c.u32(c.toc(toc_cell::effects))}, {4, c.u32(person)}, {5, code}, {6, c.u8(node + move_at::p_a)},
                       {7, c.u8(node + move_at::p_b)}, {8, std::uint32_t(std::int32_t(c.i16(node + move_at::p_c)))},
                       {9, c.u8(node + move_at::p_d)}, {10, std::uint32_t(std::int32_t(c.i16(node + move_at::p_e)))}});
                c.call(callee::apply_effect, 0x101c1118);
            }
        }
        post_move_news(c, person, std::int8_t(c.u8(node + move_at::kind)), std::int8_t(c.u8(node + move_at::p_d)),
                       c.i16(node + move_at::p_e), std::int8_t(c.u8(node + move_at::p_f)));
        c.set({{3, c.u32(list + list_queue)}, {4, node}});
        c.call(callee::remove_node, 0x101c114c);
    }
}
}
