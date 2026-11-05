#include "hall_of_fame_adapter.hpp"
#include "guest_abi.hpp"
#include "guest_saved_registers.hpp"
#include "routine_check.hpp"

#include <cstdlib>

namespace cm::aot {
namespace {
using guest::Cpu;
bool declining = std::getenv("CM_HALL_OF_FAME_ORIGINAL") != nullptr;
constexpr auto compare_rank_entry = 0x10142100u, initialize_record_entry = 0x101407a0u;

struct RankingRecord {
    Cpu& cpu;
    std::uint32_t address;
    std::uint32_t primary_rank() const { return cpu.u32(address + 0x68); }
    std::int32_t secondary_rank() const { return cpu.i32(address + 0x64); }
};

void block(Cpu& c, std::uint32_t pc, unsigned count) {
    c.s.step(pc);
    c.s.charge(count - 1);
}

void finish(Cpu& c, std::uint32_t pc, int result) {
    block(c, pc, 2);
    c.set(3, std::uint32_t(result));
    c.finish();
}

void initialize_record(Cpu& c) {
    GuestNativeScope scope(c.s, initialize_record_entry);
    block(c, initialize_record_entry, 7);
    const auto address = c.r(3), caller = c.sp(), frame = caller - 0x40;
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    c.set(31, address);
    c.put32(caller + 8, c.r(0));
    c.set(3, address + 0x70);
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);
    c.call(0x10715880, 0x101407bc);
    block(c, 0x101407bc, 6);
    c.set(3, address);
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
    c.set(31, c.u32(caller - 4));
    c.finish();
}

void compare_rank(Cpu& c) {
    const RankingRecord left{c, c.r(3)}, right{c, c.r(4)};
    block(c, compare_rank_entry, 2);
    c.compare_logical(0, left.address, 0);
    if (!left.address) return finish(c, 0x10142108, 1);
    block(c, 0x10142110, 2);
    c.compare_logical(0, right.address, 0);
    if (!right.address) return finish(c, 0x10142118, -1);

    block(c, 0x10142120, 4);
    const auto right_primary = right.primary_rank();
    c.set(0, right_primary);
    const auto left_primary = left.primary_rank();
    c.set(5, left_primary);
    c.compare_logical(0, right_primary, left_primary);
    if (right_primary > left_primary) return finish(c, 0x10142130, 1);
    block(c, 0x10142138, 2);
    c.compare_logical(0, left_primary, right_primary);
    if (left_primary > right_primary) return finish(c, 0x10142140, -1);

    block(c, 0x10142148, 4);
    const auto right_secondary = right.secondary_rank();
    c.set(4, std::uint32_t(right_secondary));
    const auto left_secondary = left.secondary_rank();
    c.set(5, std::uint32_t(left_secondary));
    c.compare(0, right_secondary, left_secondary);
    if (left_secondary > right_secondary) return finish(c, 0x10142158, 1);

    block(c, 0x10142160, 5);
    // The original subtracts sign bits with the carry from the unsigned
    // words. Keep the resulting carry separately from the ranking decision.
    const auto left_sign = unsigned(left_secondary < 0);
    const auto right_sign = unsigned(right_secondary < 0);
    const auto borrow = unsigned(std::uint32_t(left_secondary) < std::uint32_t(right_secondary));
    c.set(0, left_sign);
    c.s.wr<1>(guest::reg::xer_ca, right_sign >= left_sign + borrow);
    c.set(3, left_secondary == right_secondary ? 0 : UINT32_MAX);
    c.finish();
}
constexpr auto record_at_entry = 0x101407e0u, add_default_entry = 0x101409c0u;
constexpr auto add_for_nation_entry = 0x10140a90u, insert_ranked_entry = 0x10140b80u;
constexpr auto category_for_nation_entry = 0x10141e80u;
constexpr unsigned record_size = 0x78, category_size = 100 * record_size;

void record_compare(Cpu& c, std::int32_t value) {
    const auto high = c.cr(0) & 0xf0;
    c.compare(0, value, 0);
    c.set_cr(0, high | c.cr(0));
}

void push_frame(Cpu& c, GuestNativeScope& scope, std::uint32_t caller, std::uint32_t frame) {
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);
}

void restore_link(Cpu& c, std::uint32_t caller) {
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
}

void report_message(Cpu& c, std::uint32_t pc, std::uint32_t source, std::uint32_t message,
                    unsigned filename, unsigned extension, unsigned line) {
    const auto frame = c.sp();
    block(c, pc, 6);
    c.set({{3, source}, {4, 0}, {5, 0}, {6, frame + filename}, {7, frame + extension}});
    c.call(0x10739890, pc + 24);
    block(c, pc + 24, 1);
    c.link(pc + 28);
    c.set(3, c.toc(-0x5b20));
    c.s.charge(2);
    block(c, pc + 28, 7);
    c.set({{5, c.r(3)}, {3, message}, {4, source + 0x11},
           {6, frame + filename}, {7, frame + extension}, {8, line}});
    c.call(0x1077b720, pc + 56);
    block(c, pc + 56, 4);
    c.set({{4, message}, {3, source + 0x1e}, {5, 0}});
    c.call(0x10769b60, pc + 72);
}

void report_bad_record(Cpu& c, std::uint32_t pc, std::uint32_t source, std::uint32_t message,
                       unsigned filename, unsigned extension, unsigned line) {
    report_message(c, pc, source, message, filename, extension, line);
    block(c, pc + 72, 5);
    c.set(4, c.toc(-0x7bac));
    c.set({{0, 0}, {3, 0}});
    c.put8(c.r(4), 0);
}

void copy_word_pairs(Cpu& c, std::uint32_t to, std::uint32_t from, unsigned word_register,
                     unsigned offset, unsigned bytes) {
    // Both loads precede either store, including on partially overlapping rows.
    for (unsigned end = offset + bytes; offset < end; offset += 8) {
        const auto first = c.u32(from + offset);
        c.set(word_register, first);
        const auto second = c.u32(from + offset + 4);
        c.set(0, second);
        c.put32(to + offset, first);
        c.put32(to + offset + 4, second);
    }
}

void copy_rank_fields(Cpu& c, std::uint32_t to, std::uint32_t from) {
    c.set(0, c.u16(from + 0x62)); c.put16(to + 0x62, c.r(0));
    for (unsigned offset : {0x64u, 0x68u, 0x6cu}) {
        c.set(0, c.u32(from + offset));
        c.put32(to + offset, c.r(0));
    }
    c.set(0, c.u16(from + 0x72)); c.put16(to + 0x72, c.r(0));
    c.set(0, std::uint32_t(c.i16(from + 0x70))); c.put16(to + 0x70, c.r(0));
    c.set(0, c.u8(from + 0x74)); c.put8(to + 0x74, c.r(0));
}

void copy_ranked_row_tail(Cpu& c, std::uint32_t to, std::uint32_t from) {
    copy_word_pairs(c, to, from, 7, 8, 40);
    c.set(0, c.u16(from + 0x30));
    c.put16(to + 0x30, c.r(0));
    copy_word_pairs(c, to, from, 7, 0x32, 48);
    copy_rank_fields(c, to, from);
}

void record_at(Cpu& c) {
    GuestNativeScope scope(c.s, record_at_entry);
    block(c, record_at_entry, 9);
    const auto caller = c.sp(), frame = caller - 0x70;
    const auto table = c.r(3), out = c.r(6);
    const auto category = std::int8_t(c.r(4));
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    const auto source = c.toc(-0x54cc);
    c.set(31, source);
    c.put32(caller - 8, c.r(30));
    const auto message = c.toc(-0x7ba8);
    c.set(30, message);
    c.put32(caller + 8, c.r(0));
    c.set(0, std::uint32_t(category));
    record_compare(c, category);
    push_frame(c, scope, caller, frame);
    const auto row = std::int8_t(c.r(5));
    const bool valid = [&] {
        if (category < 0) return false;
        block(c, 0x10140804, 3);
        c.set(4, category);
        c.compare(0, category, 26);
        if (category > 26) return false;
        block(c, 0x10140810, 2);
        c.set(0, std::uint32_t(row));
        record_compare(c, row);
        if (row < 0) return false;
        block(c, 0x10140818, 3);
        c.compare(0, row, 99);
        return row <= 99;
    }();
    if (valid) {
        block(c, 0x10140880, 71);
        const auto address = table + unsigned(category) * category_size + unsigned(row) * record_size;
        c.set({{7, address}, {5, address}});
        const auto word = c.u32(address);
        c.set({{4, word}, {3, 1}});
        c.set(0, c.u32(address + 4));
        c.put32(out, word);
        c.put32(out + 4, c.r(0));
        copy_word_pairs(c, out, address, 4, 8, 40);
        c.set(0, c.u16(address + 0x30));
        c.put16(out + 0x30, c.r(0));
        copy_word_pairs(c, out, address, 4, 0x32, 48);
        copy_rank_fields(c, out, address);
    } else report_bad_record(c, 0x10140824, source, message, 0x40, 0x38, 0xe3);
    block(c, 0x1014099c, 6);
    restore_link(c, caller);
    c.set(31, c.u32(caller - 4));
    c.set(30, c.u32(caller - 8));
    c.finish();
}

void category_for_nation(Cpu& c) {
    constexpr std::array<int, 25> slots{
        -0x7508, -0x7650, -0x7510, -0x7b68, -0x7590, -0x750c, -0x7a10, -0x7594, -0x7a00,
        -0x7af4, -0x7598, -0x7a28, -0x7574, -0x7a14, -0x7a24, -0x7578, -0x7504, -0x75a8,
        -0x79fc, -0x758c, -0x7a1c, -0x7a18, -0x7514, -0x75b0, -0x7a20};
    const auto nation = c.r(3);
    for (unsigned i = 0; i < slots.size(); ++i) {
        const auto pc = category_for_nation_entry + i * 0x18;
        block(c, pc, 4);
        c.set(4, c.toc(slots[i]));
        const auto candidate = c.u32(c.r(4));
        c.set(0, candidate);
        c.compare(0, nation, candidate);
        if (nation == candidate) return finish(c, pc + 0x10, i + 1);
    }
    block(c, 0x101420d8, 5);
    c.set(4, c.toc(-0x757c));
    c.set(5, UINT32_MAX);
    const auto candidate = c.u32(c.r(4));
    c.set(0, candidate);
    c.compare(0, nation, candidate);
    if (nation == candidate) {
        block(c, 0x101420ec, 1);
        c.set(5, 26);
    }
    block(c, 0x101420f0, 2);
    c.set(3, c.r(5));
    c.finish();
}

void format_record_names(Cpu& c, std::uint32_t manager, std::uint32_t source) {
    const auto frame = c.sp();
    const auto display = frame + 0x270;
    block(c, 0x10140c4c, 5);
    c.set({{3, manager}, {4, display}, {5, 200}, {6, 1}});
    c.call(0x100caeb0, 0x10140c60);
    block(c, 0x10140c60, 4);
    c.set({{3, frame + 0x68}, {4, display}, {5, 50}});
    c.call(0x1077dc10, 0x10140c70);
    block(c, 0x10140c70, 5);
    c.set(0, 0);
    c.put8(frame + 0x99, 0);
    const auto club = c.u32(manager + 0x24);
    c.set(3, club);
    c.compare_logical(0, club, 0);
    if (club) {
        block(c, 0x10140c84, 6);
        c.set({{5, frame + 0x1a8}, {4, 0}, {6, 200}, {7, 1}, {8, 1}});
        c.call(0x100d0940, 0x10140c9c);
        block(c, 0x10140c9c, 1);
    } else {
        block(c, 0x10140ca0, 1);
        c.put8(frame + 0x1a8, c.r(0));
    }
    block(c, 0x10140ca4, 3);
    const auto nation = c.u32(manager + 0x20);
    c.set(3, nation);
    c.compare_logical(0, nation, 0);
    if (nation) {
        block(c, 0x10140cb0, 6);
        c.set({{5, frame + 0xe0}, {4, 0}, {6, 200}, {7, 1}, {8, 1}});
        c.call(0x100d0940, 0x10140cc8);
        block(c, 0x10140cc8, 1);
    } else {
        block(c, 0x10140ccc, 2);
        c.set(0, 0);
        c.put8(frame + 0xe0, 0);
    }
    const auto signed_first_byte = [&](unsigned offset, unsigned target) {
        const auto byte = c.u8(frame + offset);
        c.set(target, byte);
        const auto nonempty = std::int8_t(byte);
        c.set(0, std::uint32_t(nonempty));
        record_compare(c, nonempty);
        return nonempty != 0;
    };
    block(c, 0x10140cd4, 3);
    const auto has_club = signed_first_byte(0x1a8, 3);
    bool has_nation = false;
    if (has_club) {
        block(c, 0x10140ce0, 3);
        has_nation = signed_first_byte(0xe0, 0);
    }
    if (has_club && has_nation) {
        block(c, 0x10140cec, 5);
        c.set({{3, display}, {4, source + 0x24}, {5, frame + 0x1a8}, {6, frame + 0xe0}});
        c.call(0x1077b720, 0x10140d00);
        block(c, 0x10140d00, 1);
    } else {
        block(c, 0x10140d04, 2);
        c.set(0, std::uint32_t(std::int8_t(c.r(3))));
        record_compare(c, std::int32_t(c.r(0)));
        if (has_club) {
            block(c, 0x10140d0c, 3);
            c.set({{3, display}, {4, frame + 0x1a8}});
            c.call(0x1077dbf0, 0x10140d18);
            block(c, 0x10140d18, 1);
        } else {
            block(c, 0x10140d1c, 3);
            if (signed_first_byte(0xe0, 0)) {
                block(c, 0x10140d28, 3);
                c.set({{3, display}, {4, frame + 0xe0}});
                c.call(0x1077dbf0, 0x10140d34);
                block(c, 0x10140d34, 1);
            } else {
                block(c, 0x10140d38, 3);
                c.set({{3, display}, {4, source + 0x2a}});
                c.call(0x10727890, 0x10140d44);
            }
        }
    }
    block(c, 0x10140d44, 4);
    c.set({{3, frame + 0x9a}, {4, display}, {5, 50}});
    c.call(0x1077dc10, 0x10140d54);
}

bool already_ranked(Cpu& c, std::uint32_t category) {
    const auto frame = c.sp(), candidate = frame + 0x68;
    for (unsigned row = 0; row < 99; ++row) {
        block(c, 0x10140d9c, 4);
        const auto record = category + row * record_size;
        c.set({{27, record}, {4, candidate}, {3, record}});
        c.call(0x1077dc90, 0x10140dac);
        block(c, 0x10140dac, 2);
        const auto different_name = std::int32_t(c.r(3));
        c.compare(0, different_name, 0);
        if (!different_name) {
            block(c, 0x10140db4, 4);
            const auto game = c.u32(candidate + 0x6c);
            c.set(3, game);
            const auto old_game = c.u32(record + 0x6c);
            c.set(0, old_game);
            c.compare(0, game, old_game);
            if (game == old_game) {
                block(c, 0x10140dc4, 9);
                c.set(0, c.u32(candidate + 0x70));
                c.set(4, 0);
                c.set(5, c.u32(candidate + 0x74));
                c.put32(frame + 0x38, c.r(0));
                const auto year = c.u16(record + 0x72);
                c.set(0, year);
                const auto candidate_year = c.u16(frame + 0x3a);
                c.set(3, candidate_year);
                c.put32(frame + 0x3c, c.r(5));
                c.compare_logical(0, candidate_year, year);
                bool duplicate = false;
                if (candidate_year == year) {
                    block(c, 0x10140de8, 4);
                    const auto day = c.i16(frame + 0x38);
                    c.set(3, std::uint32_t(day));
                    const auto old_day = c.i16(record + 0x70);
                    c.set(0, std::uint32_t(old_day));
                    c.compare(0, day, old_day);
                    if (day == old_day) {
                        block(c, 0x10140df8, 1);
                        duplicate = true;
                        c.set(4, 1);
                    }
                }
                block(c, 0x10140dfc, 2);
                c.compare(0, duplicate, 0);
                if (duplicate) {
                    block(c, 0x10140e04, 2);
                    c.set(3, 0);
                    return true;
                }
            }
        }
        block(c, 0x10140e10, 4);
        c.set({{28, row + 1}, {31, (row + 1) * record_size}});
        c.compare(0, row + 1, 99);
    }
    return false;
}

void insert_ranked(Cpu& c) {
    GuestNativeScope scope(c.s, insert_ranked_entry);
    block(c, insert_ranked_entry, 14);
    const auto caller = c.sp(), frame = caller - 0x360;
    const auto table = c.r(3), manager = c.r(5), score = c.r(6);
    const auto category = std::int8_t(c.r(4));
    c.set(0, c.lr());
    guest::transfer_saved_registers<25>(c, caller);
    const auto today = c.toc(-0x7b80);
    c.set({{30, today}, {25, table}});
    const auto message = c.toc(-0x7ba8);
    c.set({{28, message}, {29, c.r(4)}});
    const auto source = c.toc(-0x54cc);
    c.set({{31, source}, {26, manager}, {27, score}});
    c.put32(caller + 8, c.r(0));
    push_frame(c, scope, caller, frame);
    c.set({{0, frame + 0xd8}, {3, frame + 0xd8}});
    c.call(0x10715880, 0x10140bb8);
    block(c, 0x10140bb8, 2);
    c.set(0, std::uint32_t(category));
    record_compare(c, category);
    const bool valid = [&] {
        if (category < 0) return false;
        block(c, 0x10140bc0, 3);
        c.compare(0, category, 26);
        if (category > 26) return false;
        block(c, 0x10140bcc, 2);
        c.compare_logical(0, manager, 0);
        return manager != 0;
    }();
    if (!valid) report_bad_record(c, 0x10140bd4, source, message, 0x48, 0x40, 0x137);
    else {
        block(c, 0x10140c30, 4);
        const auto offset = unsigned(category) * category_size;
        const auto bucket = table + offset;
        c.set(0, offset);
        c.compare_logical(0, score, 0);
        c.set(29, bucket);
        bool qualified = false;
        if (score) {
            block(c, 0x10140c40, 3);
            const auto last_score = c.u32(bucket + 0x2ed0);
            c.set(0, last_score);
            c.compare_logical(0, score, last_score);
            qualified = score > last_score;
        }
        if (!qualified) {
            block(c, 0x10140f3c, 1);
            c.set(3, 0);
        } else {
            format_record_names(c, manager, source);
            block(c, 0x10140d54, 4);
            c.set({{0, 0}, {3, 0}});
            c.put8(frame + 0xcb, 0);
            c.call(0x1077f900, 0x10140d64);
            block(c, 0x10140d64, 14);
            c.set({{4, c.toc(-0x7450)}, {28, 0}});
            const auto leap = c.u8(today + 4);
            c.set({{0, leap}, {31, 0}});
            const auto game = c.u32(c.r(4));
            c.set(6, game);
            const auto year = c.u16(today + 2);
            c.set(5, year);
            const auto day = c.i16(today);
            c.set(4, std::uint32_t(day));
            c.put32(frame + 0xcc, c.r(3));
            c.put32(frame + 0xd0, score);
            c.put32(frame + 0xd4, game);
            c.put16(frame + 0xda, year);
            c.put16(frame + 0xd8, day);
            c.put8(frame + 0xdc, leap);
            if (!already_ranked(c, bucket)) {
                block(c, 0x10140e20, 69);
                const auto candidate = frame + 0x68, last = bucket + 99 * record_size;
                c.set(7, c.u32(candidate));
                c.set(3, bucket);
                c.set(0, c.u32(candidate + 4));
                c.set({{4, 100}, {6, c.toc(-0x54d0)}, {5, record_size}});
                c.put32(last, c.r(7));
                c.put32(last + 4, c.r(0));
                copy_ranked_row_tail(c, last, candidate);
                c.call(0x10768300, 0x10140f34);
                block(c, 0x10140f34, 2);
                c.set(3, 1);
            }
        }
    }
    block(c, 0x10140f40, 5);
    restore_link(c, caller);
    guest::transfer_saved_registers<25, true>(c, caller);
    c.finish();
}

constexpr auto merge_record_entry = 0x10140f60u;

bool exact_duplicate(Cpu& c, std::uint32_t bucket, std::uint32_t candidate) {
    block(c, 0x10141000, 2);
    c.set({{28, 0}, {29, 0}});
    for (unsigned row = 0; row < 99; ++row) {
        block(c, 0x10141008, 5);
        c.set({{3, bucket + row * record_size}, {4, candidate}, {5, record_size}});
        c.call(0x10778390, 0x1014101c);
        block(c, 0x1014101c, 2);
        const auto difference = std::int32_t(c.r(3));
        c.compare(0, difference, 0);
        if (!difference) return true;
        block(c, 0x1014102c, 4);
        c.set({{28, row + 1}, {29, (row + 1) * record_size}});
        c.compare(0, row + 1, 99);
    }
    return false;
}

void replace_last_rank(Cpu& c, std::uint32_t bucket, std::uint32_t candidate) {
    const auto last = bucket + 99 * record_size;
    block(c, 0x10141060, 68);
    c.set(7, c.u32(candidate));
    c.set(4, 100);
    c.set(0, c.u32(candidate + 4));
    c.set({{5, record_size}, {6, c.toc(-0x54d0)}});
    c.put32(last, c.r(7));
    c.put32(last + 4, c.r(0));
    copy_ranked_row_tail(c, last, candidate);
    c.call(0x10768300, 0x10141170);
    block(c, 0x10141170, 2);
    c.set(3, 1);
}

void merge_record(Cpu& c) {
    GuestNativeScope scope(c.s, merge_record_entry);
    block(c, merge_record_entry, 11);
    const auto caller = c.sp(), table = c.r(3), candidate = c.r(5);
    const auto category = std::int8_t(c.r(4));
    c.set(0, c.lr());
    guest::transfer_saved_registers<26>(c, caller);
    c.set(27, c.r(4));
    const auto message = c.toc(-0x7ba8);
    c.set(29, message);
    const auto source = c.toc(-0x54cc);
    c.set({{30, source}, {26, table}, {31, candidate}});
    c.put32(caller + 8, c.r(0));
    c.set(0, std::uint32_t(category));
    record_compare(c, category);
    push_frame(c, scope, caller, caller - 0x80);

    const bool valid = [&] {
        if (category < 0) return false;
        block(c, 0x10140f8c, 3);
        c.compare(0, category, 26);
        if (category > 26) return false;
        block(c, 0x10140f98, 2);
        c.compare_logical(0, candidate, 0);
        return candidate != 0;
    }();
    if (!valid) {
        report_bad_record(c, 0x10140fa0, source, message, 0x40, 0x38, 0x18b);
    } else {
        block(c, 0x10140ffc, 1);
        const auto offset = unsigned(category) * category_size, bucket = table + offset;
        c.set(30, offset);
        if (exact_duplicate(c, bucket, candidate)) {
            block(c, 0x10141024, 2);
            c.set(3, 0);
        } else {
            block(c, 0x1014103c, 6);
            c.set(0, std::uint32_t(category));
            const auto score = c.u32(candidate + 0x68);
            c.set({{4, score}, {0, offset}});
            c.compare_logical(0, score, 0);
            c.set(3, bucket);
            bool qualifies = false;
            if (score) {
                block(c, 0x10141054, 3);
                const auto cutoff = c.u32(bucket + category_size - record_size + 0x68);
                c.set(0, cutoff);
                c.compare_logical(0, score, cutoff);
                qualifies = score > cutoff;
            }
            if (qualifies) replace_last_rank(c, bucket, candidate);
            else {
                block(c, 0x10141178, 1);
                c.set(3, 0);
            }
        }
    }
    block(c, 0x1014117c, 5);
    restore_link(c, caller);
    guest::transfer_saved_registers<26, true>(c, caller);
    c.finish();
}

void call_owned(Cpu& c, std::uint32_t target, std::uint32_t resume, void (*body)(Cpu&)) {
    if (routine_checker || dispatch_probe || c.s.scalar_capture) {
        c.call(target, resume);
        return;
    }
    const auto caller = c.sp();
    c.link(resume);
    c.s.pc = target;
    body(c);
    if (c.s.pc != resume || c.sp() != caller)
        throw std::logic_error("Hall of Fame owner returned outside its call boundary");
}

bool manager_eligible(Cpu& c, std::uint32_t manager, unsigned manager_register, unsigned count_register) {
    c.set(count_register, c.toc(-0x7be4));
    const auto id = c.u32(manager);
    c.set(manager_register, id);
    const auto count = c.u32(c.r(count_register));
    c.set(count_register, count);
    const auto cutoff = count - 16;
    c.set(0, cutoff);
    c.compare(0, id, cutoff);
    return std::int32_t(id) >= std::int32_t(cutoff);
}

void add_default(Cpu& c) {
    GuestNativeScope scope(c.s, add_default_entry);
    block(c, add_default_entry, 10);
    const auto caller = c.sp(), frame = caller - 0x70, manager = c.r(4);
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    c.compare_logical(0, manager, 0);
    const auto source = c.toc(-0x54cc);
    c.set(31, source);
    c.put32(caller - 8, c.r(30));
    c.set(6, c.r(5));
    const auto message = c.toc(-0x7ba8);
    c.set(30, message);
    c.put32(caller + 8, c.r(0));
    push_frame(c, scope, caller, frame);
    if (!manager) report_bad_record(c, 0x101409e8, source, message, 0x40, 0x38, 0xfb);
    else {
        block(c, 0x10140a44, 6);
        if (manager_eligible(c, manager, 7, 5)) {
            block(c, 0x10140a64, 3);
            c.set({{5, manager}, {4, 0}});
            call_owned(c, insert_ranked_entry, 0x10140a70, insert_ranked);
        } else {
            block(c, 0x10140a5c, 2);
            c.set(3, 0);
        }
    }
    block(c, 0x10140a70, 6);
    restore_link(c, caller);
    c.set(31, c.u32(caller - 4));
    c.set(30, c.u32(caller - 8));
    c.finish();
}

void add_for_nation(Cpu& c) {
    GuestNativeScope scope(c.s, add_for_nation_entry);
    block(c, add_for_nation_entry, 10);
    const auto caller = c.sp(), frame = caller - 0x80;
    const auto table = c.r(3), manager = c.r(4), nation = c.r(5), score = c.r(6);
    c.set(0, c.lr());
    guest::transfer_saved_registers<27>(c, caller);
    c.set(28, manager);
    record_compare(c, manager);
    const auto message = c.toc(-0x7ba8);
    c.set(30, message);
    const auto source = c.toc(-0x54cc);
    c.set({{31, source}, {27, table}, {29, score}});
    c.put32(caller + 8, c.r(0));
    push_frame(c, scope, caller, frame);
    const bool valid = [&] {
        if (!manager) return false;
        block(c, 0x10140ab8, 2);
        c.compare(0, nation, 0);
        if (std::int32_t(nation) < 0) return false;
        block(c, 0x10140ac0, 5);
        c.set(3, c.toc(-0x7b04));
        const auto nation_count = c.u32(c.r(3));
        c.set(3, nation_count);
        const auto last = nation_count - 1;
        c.set(0, last);
        c.compare(0, nation, last);
        return std::int32_t(nation) <= std::int32_t(last);
    }();
    if (!valid) report_bad_record(c, 0x10140ad4, source, message, 0x40, 0x38, 0x117);
    else {
        block(c, 0x10140b30, 6);
        if (manager_eligible(c, manager, 4, 3)) {
            block(c, 0x10140b50, 2);
            c.set(3, nation);
            call_owned(c, category_for_nation_entry, 0x10140b58, category_for_nation);
            block(c, 0x10140b58, 5);
            c.set({{4, c.r(3)}, {3, table}, {5, manager}, {6, score}});
            call_owned(c, insert_ranked_entry, 0x10140b6c, insert_ranked);
        } else {
            block(c, 0x10140b48, 2);
            c.set(3, 0);
        }
    }
    block(c, 0x10140b6c, 5);
    restore_link(c, caller);
    guest::transfer_saved_registers<27, true>(c, caller);
    c.finish();
}

constexpr auto encode_entry = 0x10142180u, decode_entry = 0x101426c0u;
constexpr unsigned key_period = 480;
constexpr std::uint32_t legacy_divisor = 0x88888889;

struct FileTransform {
    Cpu& c;
    std::uint32_t data, keys;
    unsigned size;

    std::uint32_t key_address(unsigned index) const { return keys + 4 * (index % key_period); }
    std::int32_t key(unsigned index) const { return c.i32(key_address(index)); }
    std::uint32_t position(std::int32_t word) const {
        return std::uint32_t(word % std::int32_t(size));
    }
    struct Swap {
        std::uint32_t left, right;
        std::uint8_t left_byte;
    };
    Swap swap(unsigned index, bool early_left_read = false) const {
        const auto left = position(key(index));
        // The fifth swap of the encoder's eight-swap block reads its
        // left byte before its right key. The two ranges can alias.
        auto left_byte = early_left_read ? c.u8(data + left) : std::uint8_t{};
        const auto right = position(key(index + 1));
        if (!early_left_read) left_byte = c.u8(data + left);
        const auto right_byte = c.u8(data + right);
        c.put8(data + left, right_byte);
        c.put8(data + right, left_byte);
        return {left, right, left_byte};
    }
};

void set_ctr(Cpu& c, unsigned count) { c.s.wr<4>(guest::reg::ctr, count); }

void encode_mask(FileTransform file, std::uint32_t checksum) {
    auto& c = file.c;
    unsigned index = 0;
    const auto mask = [&](bool data_first) {
        const auto address = file.data + index;
        const auto sum = c.u32(checksum);
        c.set(11, sum);
        const auto byte = c.u8(address);
        c.set(10, sum + byte);
        c.put32(checksum, sum + byte);

        std::uint32_t data = 0, key = 0;
        if (data_first) data = c.u8(address);
        c.set(0, (index % key_period) * 4);
        c.set(12, index + 1);
        key = std::uint32_t(file.key(index));
        if (!data_first) data = c.u8(address);
        c.set(10, data);
        const auto encoded = data ^ (key & 0xff);
        c.set(0, encoded);
        c.put8(address, encoded);
        ++index;
    };

    block(c, 0x10142234, 3);
    const auto groups = file.size / 4;
    c.set(0, groups);
    record_compare(c, groups);
    set_ctr(c, groups);
    for (unsigned remaining = groups; remaining > 0; --remaining) {
        block(c, 0x10142240, 62);
        mask(true);
        mask(false);
        mask(false);
        mask(false);
        c.set(28, file.data + index);
        set_ctr(c, remaining - 1);
    }
    auto tail = file.size;
    if (groups) {
        block(c, 0x10142338, 2);
        tail %= 4;
        c.set(7, tail);
        record_compare(c, tail);
        c.set(9, index / key_period);
    }
    if (!tail) return;
    block(c, 0x10142340, 1);
    set_ctr(c, tail);
    for (unsigned remaining = tail; remaining > 0; --remaining) {
        block(c, 0x10142344, 17);
        c.set(9, index / key_period);
        mask(true);
        c.set(28, file.data + index);
        set_ctr(c, remaining - 1);
    }
}

void encode_permutation(FileTransform file) {
    auto& c = file.c;
    block(c, 0x10142394, 3);
    c.compare(0, file.size, 8);
    c.set(9, file.size - 8);
    unsigned index = 0;
    if (file.size > 8) {
        block(c, 0x101423a0, 7);
        const auto groups = (file.size - 1) / 8;
        c.set({{7, groups}, {8, 0x88890000}, {0, legacy_divisor}});
        set_ctr(c, groups);
        c.compare(0, file.size - 8, 0);
        for (unsigned remaining = groups; remaining > 0; --remaining) {
            block(c, 0x101423bc, 151);
            for (unsigned lane = 0; lane < 8; ++lane) {
                const auto swapped = file.swap(index++, lane == 4);
                if (lane == 6) c.set({{11, swapped.left}, {12, swapped.left_byte}});
            }
            c.set(5, index);
            set_ctr(c, remaining - 1);
        }
    }
    block(c, 0x10142618, 6);
    c.set({{7, 0x88890000}, {0, file.size - index}, {10, legacy_divisor}});
    set_ctr(c, file.size - index);
    c.compare(0, index, file.size);
    for (; index < file.size; ++index) {
        block(c, 0x10142630, 25);
        const auto swapped = file.swap(index);
        c.set({{5, index + 1}, {7, swapped.right}, {8, swapped.left}, {9, swapped.left_byte}});
        set_ctr(c, file.size - index - 1);
    }
}

void decode_permutation(FileTransform file) {
    auto& c = file.c;
    block(c, 0x1014274c, 5);
    const auto last = file.size - 1;
    c.set(30, last);
    c.s.wr<1>(guest::reg::xer_ca, file.size != 0);
    record_compare(c, std::int32_t(last));
    c.set({{7, file.size}, {12, legacy_divisor}});
    if (!file.size) return;
    block(c, 0x10142760, 3);
    const auto groups = file.size / 2;
    c.set(0, groups);
    record_compare(c, groups);
    set_ctr(c, groups);
    unsigned index = last;
    for (unsigned remaining = groups; remaining > 0; --remaining) {
        block(c, 0x1014276c, 49);
        file.swap(index--);
        file.swap(index--);
        c.set(30, index);
        set_ctr(c, remaining - 1);
    }
    auto tail = file.size;
    if (groups) {
        block(c, 0x10142830, 2);
        tail %= 2;
        c.set(7, tail);
        record_compare(c, tail);
    }
    if (!tail) return;
    block(c, 0x10142838, 1);
    set_ctr(c, tail);
    block(c, 0x1014283c, 25);
    file.swap(index--);
    c.set(30, index);
    set_ctr(c, 0);
}

std::uint32_t decode_mask(FileTransform file) {
    auto& c = file.c;
    block(c, 0x101428a0, 6);
    c.compare(0, file.size, 0);
    c.set({{7, 0x88890000}, {8, legacy_divisor}, {12, 0}, {11, 0}});
    if (!file.size) return 0;

    unsigned index = 0;
    std::uint32_t sum = 0;
    const auto mask = [&] {
        const auto address = file.data + index;
        c.set(9, index / key_period);
        const auto byte = c.u8(address);
        c.set(10, byte);
        c.set(7, index % key_period);
        c.set(11, index + 1);
        const auto decoded = byte ^ (std::uint32_t(file.key(index)) & 0xff);
        c.set(0, decoded);
        c.put8(address, decoded);
        const auto stored = c.u8(address);
        c.set(0, stored);
        sum += stored;
        c.set(12, sum);
        ++index;
    };

    block(c, 0x101428b8, 3);
    const auto groups = file.size / 4;
    c.set(0, groups);
    record_compare(c, groups);
    set_ctr(c, groups);
    for (unsigned remaining = groups; remaining > 0; --remaining) {
        block(c, 0x101428c4, 54);
        for (unsigned lane = 0; lane < 4; ++lane) mask();
        c.set(3, file.data + index);
        set_ctr(c, remaining - 1);
    }
    auto tail = file.size;
    if (groups) {
        block(c, 0x1014299c, 2);
        tail %= 4;
        c.set(4, tail);
        record_compare(c, tail);
    }
    if (!tail) return sum;
    block(c, 0x101429a4, 1);
    set_ctr(c, tail);
    for (unsigned remaining = tail; remaining > 0; --remaining) {
        block(c, 0x101429a8, 15);
        mask();
        c.set(3, file.data + index);
        set_ctr(c, remaining - 1);
    }
    return sum;
}

template<bool Encode>
void transform_file(Cpu& c) {
    constexpr auto entry = Encode ? encode_entry : decode_entry;
    constexpr auto validate_size = Encode ? 0x101421b0u : 0x101426e8u;
    constexpr auto error = Encode ? 0x101421b8u : 0x101426f0u;
    GuestNativeScope scope(c.s, entry);
    block(c, entry, Encode ? 12 : 10);
    const auto caller = c.sp(), frame = caller - 0x70;
    const auto address = c.r(3), size = c.r(4), checksum = c.r(5);
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    c.compare_logical(0, address, 0);
    const auto keys = c.toc(-0x54d8);
    c.set(6, keys);
    c.put32(caller - 8, c.r(30));
    const auto message = c.toc(-0x7ba8);
    c.set(30, message);
    if constexpr (Encode) c.put32(caller - 12, c.r(29));
    else c.put32(caller + 8, c.r(0));
    const auto source = c.toc(-0x54cc);
    c.set(Encode ? 29 : 31, source);
    if constexpr (Encode) {
        c.put32(caller - 16, c.r(28));
        c.put32(caller + 8, c.r(0));
    }
    push_frame(c, scope, caller, frame);
    if (address) {
        block(c, validate_size, 2);
        c.compare(0, size, 0);
    }
    if (!address || std::int32_t(size) < 0) {
        report_bad_record(c, error, source, message, 0x40, 0x38, Encode ? 0x367 : 0x391);
    } else {
        const FileTransform file{c, address, keys, size};
        if constexpr (Encode) {
            block(c, 0x10142214, 8);
            c.set({{0, 0}, {8, 0x88890000}});
            c.put32(checksum, 0);
            c.set({{28, address}, {7, size}, {8, legacy_divisor}, {12, 0}});
            if (size) encode_mask(file, checksum);
            block(c, 0x10142388, 3);
            c.compare(0, size, 0);
            c.set(5, 0);
            if (size) encode_permutation(file);
            block(c, 0x10142694, 1);
            c.set(3, 1);
        } else {
            decode_permutation(file);
            const auto sum = decode_mask(file);
            block(c, 0x101429e4, 4);
            c.set(3, checksum == sum);
        }
    }
    block(c, Encode ? 0x10142698 : 0x101429f4, Encode ? 8 : 6);
    restore_link(c, caller);
    c.set(31, c.u32(caller - 4));
    c.set(30, c.u32(caller - 8));
    if constexpr (Encode) {
        c.set(29, c.u32(caller - 12));
        c.set(28, c.u32(caller - 16));
    }
    c.finish();
}

constexpr auto construct_table_entry = 0x10140730u;
constexpr auto read_file_entry = 0x10141190u, write_file_entry = 0x10141990u;
constexpr auto import_file_entry = 0x10141500u, validate_file_entry = 0x10143050u;
constexpr unsigned table_size = 27 * category_size;
constexpr auto file_magic = 0x003abfb2u;
constexpr auto close_stream = 0x107766f0u, clear_memory = 0x10778300u;

struct FileFrame {
    Cpu& cpu;
    std::uint32_t table, source, success_flag, message, handle;
    std::uint32_t frame() const { return cpu.sp(); }
};

struct FileError {
    std::uint32_t pc;
    unsigned filename, extension, line;
};

void file_error(const FileFrame& file, FileError error, bool clear_table = false) {
    auto& c = file.cpu;
    const auto pc = error.pc;
    report_message(c, pc, file.source, file.message, error.filename, error.extension, error.line);
    auto resume = pc + 88;
    if (clear_table) {
        block(c, pc + 72, 7);
        c.set({{0, 0}, {4, 0x50000}});
        c.put8(file.success_flag, 0);
        c.set({{3, file.table}, {5, table_size}, {4, 0}});
        c.call(clear_memory, pc + 100);
        block(c, pc + 100, 2);
        c.set(3, file.handle);
        resume = pc + 108;
    } else {
        block(c, pc + 72, 4);
        c.set({{0, 0}, {3, file.handle}});
        c.put8(file.success_flag, 0);
    }
    c.call(close_stream, resume);
    block(c, resume, 2);
    c.set(3, 0);
}

bool stream_item(const FileFrame& file, std::uint32_t resume, bool write) {
    auto& c = file.cpu;
    c.call(write ? 0x10776360 : 0x10775fe0, resume);
    block(c, resume, 2);
    c.compare_logical(0, c.r(3), 1);
    return c.r(3) == 1;
}

bool transfer_item(const FileFrame& file, std::uint32_t pc, std::uint32_t data, unsigned bytes,
                   bool write) {
    auto& c = file.cpu;
    const unsigned count = bytes == table_size ? 6 : 5;
    block(c, pc, count);
    c.set({{3, data}, {4, bytes}, {5, 1}, {6, file.handle}});
    return stream_item(file, pc + count * 4, write);
}

bool nonzero_byte_result(Cpu& c, std::uint32_t pc) {
    block(c, pc, 2);
    const auto status = std::int8_t(c.r(3));
    c.set(0, std::uint32_t(status));
    record_compare(c, status);
    return status != 0;
}

void close_success(const FileFrame& file, std::uint32_t pc) {
    auto& c = file.cpu;
    block(c, pc, 2);
    c.set(3, file.handle);
    c.call(close_stream, pc + 8);
    block(c, pc + 8, 1);
    c.set(3, 1);
}

bool valid_file_magic(Cpu& c, std::uint32_t pc, std::uint32_t header) {
    block(c, pc, 4);
    const auto magic = c.u32(header);
    c.set({{3, magic}, {0, magic - 0x003a0000}});
    c.compare_logical(0, c.r(0), 0xbfb2);
    return magic == file_magic;
}

bool valid_file_version(Cpu& c, std::uint32_t pc, std::uint32_t header) {
    block(c, pc, 3);
    const auto version = c.u8(header + 4);
    c.set(0, version);
    c.compare(0, version, 1);
    return version == 1;
}

bool decode_table(Cpu& c, std::uint32_t pc, std::uint32_t table, std::uint32_t checksum) {
    block(c, pc, 5);
    c.set(4, 0x50000);
    c.set(5, c.u32(checksum));
    c.set({{3, table}, {4, table_size}});
    call_owned(c, decode_entry, pc + 20, transform_file<false>);
    return nonzero_byte_result(c, pc + 20);
}

void load_table(const FileFrame& file) {
    auto& c = file.cpu;
    const auto header = file.frame() + 0x6c, checksum = file.frame() + 0x38;
    if (!file.handle) {
        block(c, 0x101411cc, 2);
        c.set(3, 0);
        return;
    }
    if (!transfer_item(file, 0x101411d4, header, 8, false))
        return file_error(file, {0x101411f0, 0x114, 0x64, 0x1bc});

    if (!valid_file_magic(c, 0x10141250, header))
        return file_error(file, {0x10141260, 0xf4, 0x5c, 0x1c4});
    if (!valid_file_version(c, 0x101412c0, header))
        return file_error(file, {0x101412cc, 0xd4, 0x54, 0x1cc});
    if (!transfer_item(file, 0x1014132c, file.table, table_size, false))
        return file_error(file, {0x1014134c, 0xb4, 0x4c, 0x1d4}, true);
    if (!transfer_item(file, 0x101413c0, checksum, 4, false))
        return file_error(file, {0x101413dc, 0x94, 0x44, 0x1dd}, true);

    if (!decode_table(c, 0x10141450, file.table, checksum))
        return file_error(file, {0x1014146c, 0x74, 0x3c, 0x1e6}, true);
    close_success(file, 0x101414e0);
}

void save_table(const FileFrame& file) {
    auto& c = file.cpu;
    const auto frame = file.frame(), checksum = frame + 0x38;
    if (!file.handle) {
        block(c, 0x101419cc, 3);
        c.set({{3, frame + 0x10c}, {4, file.source + 0x5a}});
        c.call(0x10727890, 0x101419d8);
        block(c, 0x101419d8, 3);
        c.set({{3, frame + 0x170}, {4, file.source + 0x6f}});
        c.call(0x10727890, 0x101419e4);
        block(c, 0x101419e4, 4);
        c.set({{3, frame + 0x10c}, {4, frame + 0x170}, {5, 0}});
        c.call(0x10769b60, 0x101419f4);
        block(c, 0x101419f4, 2);
        c.set(3, 0);
        return;
    }

    block(c, 0x101419fc, 10);
    c.set({{3, 0x003b0000}, {0, 1}, {4, file_magic}});
    c.put8(frame + 0x68, 1);
    c.set({{6, file.handle}, {3, frame + 0x64}});
    // The remaining three bytes of the header retain their stack contents.
    c.put32(frame + 0x64, file_magic);
    c.set({{4, 8}, {5, 1}});
    if (!stream_item(file, 0x10141a24, true))
        return file_error(file, {0x10141a2c, 0xec, 0x5c, 0x288});

    block(c, 0x10141a8c, 5);
    c.set({{3, file.table}, {4, table_size}, {5, checksum}});
    call_owned(c, encode_entry, 0x10141aa0, transform_file<true>);
    if (!nonzero_byte_result(c, 0x10141aa0))
        return file_error(file, {0x10141aa8, 0xcc, 0x54, 0x290});

    // A short write leaves the live table encoded in the original.
    if (!transfer_item(file, 0x10141b08, file.table, table_size, true))
        return file_error(file, {0x10141b28, 0xac, 0x4c, 0x298});
    if (!transfer_item(file, 0x10141b88, checksum, 4, true))
        return file_error(file, {0x10141ba4, 0x8c, 0x44, 0x2a0});

    if (!decode_table(c, 0x10141c04, file.table, checksum))
        return file_error(file, {0x10141c20, 0x6c, 0x3c, 0x2a8});
    close_success(file, 0x10141c80);
}

template<bool Write>
void table_file(Cpu& c) {
    constexpr auto entry = Write ? write_file_entry : read_file_entry;
    constexpr auto end = Write ? 0x10141c8cu : 0x101414ecu;
    GuestNativeScope scope(c.s, entry);
    block(c, entry, 11);
    const auto caller = c.sp(), table = c.r(3);
    c.set(0, c.lr());
    guest::transfer_saved_registers<27>(c, caller);
    const auto source = c.toc(-0x54cc);
    c.set(Write ? 29 : 31, source);
    c.set(27, table);
    const auto success = c.toc(-0x7bac);
    c.set(Write ? 31 : 29, success);
    const auto message = c.toc(-0x7ba8);
    c.set(Write ? 28 : 30, message);
    c.set({{3, source + 0x39}, {4, source + 0x43}});
    c.put32(caller + 8, c.r(0));
    push_frame(c, scope, caller, caller - (Write ? 0x380 : 0x150));
    c.call(0x10746280, entry + 44);
    block(c, entry + 44, 2);
    c.set(4, source + (Write ? 0x57 : 0x54));
    c.call(0x10739de0, entry + 52);
    block(c, entry + 52, 2);
    const auto handle = c.r(3);
    c.set(Write ? 30 : 28, handle);
    record_compare(c, std::int32_t(handle));

    const FileFrame file{c, table, source, success, message, handle};
    if constexpr (Write) save_table(file);
    else load_table(file);

    block(c, end, 5);
    restore_link(c, caller);
    guest::transfer_saved_registers<27, true>(c, caller);
    c.finish();
}

bool import_validation_only(Cpu& c, std::uint32_t pc, std::uint32_t mode) {
    block(c, pc, 2);
    const auto validation_only = std::int8_t(mode);
    c.set(0, std::uint32_t(validation_only));
    record_compare(c, validation_only);
    return validation_only != 0;
}

void import_error(const FileFrame& file, FileError error, std::uint32_t mode) {
    auto& c = file.cpu;
    if (!import_validation_only(c, error.pc - 8, mode)) {
        report_message(c, error.pc, file.source, file.message, error.filename, error.extension, error.line);
        block(c, error.pc + 72, 2);
        c.set(0, 0);
        c.put8(file.success_flag, 0);
    }
    auto finish = error.pc + 80;
    if (file.handle) {
        block(c, finish, 2);
        c.set(3, file.handle);
        finish += 8;
        c.call(close_stream, finish);
    }
    block(c, finish, 2);
    c.set(3, 0);
}

void merge_table(Cpu& c, std::uint32_t table, std::uint32_t imported) {
    block(c, 0x1014190c, 3);
    c.set({{31, 0}, {30, 0}});
    for (unsigned category = 0; ; ++category) {
        block(c, 0x10141964, 3);
        c.set(0, category);
        c.compare(0, category, 27);
        if (category == 27) break;
        block(c, 0x10141918, 3);
        c.set({{25, 0}, {29, 0}});
        for (unsigned row = 0; ; ++row) {
            block(c, 0x10141950, 3);
            c.set(0, row);
            c.compare(0, row, 100);
            if (row == 100) break;
            block(c, 0x10141928, 5);
            const auto candidate = imported + category * category_size + row * record_size;
            c.set(5, candidate);
            const auto stamp = c.i32(candidate + 0x64);
            c.set(0, std::uint32_t(stamp));
            c.compare(0, stamp, 0);
            if (stamp > 0) {
                block(c, 0x1014193c, 3);
                c.set({{3, table}, {4, category}});
                call_owned(c, merge_record_entry, 0x10141948, merge_record);
            }
            block(c, 0x10141948, 2);
            c.set({{29, (row + 1) * record_size}, {25, row + 1}});
        }
        block(c, 0x1014195c, 2);
        c.set({{30, (category + 1) * category_size}, {31, category + 1}});
    }
}

void read_import(const FileFrame& file, std::uint32_t imported, std::uint32_t mode) {
    auto& c = file.cpu;
    const auto header = file.frame() + 0x7c, checksum = file.frame() + 0x38;
    if (!transfer_item(file, 0x10141604, header, 8, false))
        return import_error(file, {0x10141628, 0x124, 0x64, 0x21c}, mode);
    if (!valid_file_magic(c, 0x10141688, header))
        return import_error(file, {0x101416a0, 0x104, 0x5c, 0x225}, mode);
    if (!valid_file_version(c, 0x10141700, header))
        return import_error(file, {0x10141714, 0xe4, 0x54, 0x22e}, mode);
    if (!transfer_item(file, 0x10141774, imported, table_size, false))
        return import_error(file, {0x1014179c, 0xc4, 0x4c, 0x237}, mode);
    if (!transfer_item(file, 0x101417fc, checksum, 4, false))
        return import_error(file, {0x10141820, 0xa4, 0x44, 0x240}, mode);
    if (!decode_table(c, 0x10141880, imported, checksum))
        return import_error(file, {0x101418a4, 0x84, 0x3c, 0x249}, mode);
    if (!import_validation_only(c, 0x10141904, mode)) merge_table(c, file.table, imported);
    close_success(file, 0x10141970);
}

void import_file(Cpu& c) {
    GuestNativeScope scope(c.s, import_file_entry);
    block(c, import_file_entry, 12);
    const auto caller = c.sp(), table = c.r(3), path = c.r(4), mode = c.r(5);
    c.set(0, c.lr());
    guest::transfer_saved_registers<25>(c, caller);
    c.compare_logical(0, path, 0);
    const auto imported = c.toc(-0x54d4);
    c.set(28, imported);
    const auto success = c.toc(-0x7bac);
    c.set({{29, success}, {26, table}});
    const auto message = c.toc(-0x7ba8);
    c.set({{30, message}, {25, mode}});
    const auto source = c.toc(-0x54cc);
    c.set(31, source);
    c.put32(caller + 8, c.r(0));
    push_frame(c, scope, caller, caller - 0x1b0);
    if (!path) {
        import_error({c, table, source, success, message, 0}, {0x10141538, 0x164, 0x74, 0x20b}, mode);
    } else {
        block(c, 0x10141590, 3);
        c.set({{3, path}, {4, source + 0x54}});
        c.call(0x10739de0, 0x1014159c);
        block(c, 0x1014159c, 2);
        const auto handle = c.r(3);
        c.set(27, handle);
        record_compare(c, std::int32_t(handle));
        const FileFrame file{c, table, source, success, message, handle};
        if (!handle) import_error(file, {0x101415ac, 0x144, 0x6c, 0x214}, mode);
        else read_import(file, imported, mode);
    }
    block(c, 0x1014197c, 5);
    restore_link(c, caller);
    guest::transfer_saved_registers<25, true>(c, caller);
    c.finish();
}

void validate_file(Cpu& c) {
    GuestNativeScope scope(c.s, validate_file_entry);
    block(c, validate_file_entry, 5);
    const auto caller = c.sp(), path = c.r(3);
    c.set(0, c.lr());
    const auto slot = c.toc(-0x7414);
    c.set(5, slot);
    c.put32(caller + 8, c.r(0));
    c.set(4, path);
    push_frame(c, scope, caller, caller - 0x40);
    block(c, 0x10143064, 3);
    const auto table = c.u32(slot);
    c.set(3, table);
    c.compare_logical(0, table, 0);
    bool valid = false;
    if (table) {
        block(c, 0x10143070, 2);
        c.set(5, 1);
        call_owned(c, import_file_entry, 0x10143078, import_file);
        valid = nonzero_byte_result(c, 0x10143078);
    }
    block(c, valid ? 0x10143080 : 0x10143088, valid ? 2 : 1);
    c.set(3, valid ? 1 : 2);
    block(c, 0x1014308c, 4);
    restore_link(c, caller);
    c.finish();
}

constexpr auto category_name_entry = 0x10142a10u;
constexpr auto sorted_categories_entry = 0x101430a0u, compare_categories_entry = 0x10143180u;
constexpr int category_label_mode = -0x3953;

struct CategoryPhrase {
    unsigned compact, full;
};

constexpr std::array<CategoryPhrase, 27> category_phrases{{
    {0x113, 0x11d}, {0x130, 0x15d}, {0x170, 0x19c}, {0x1ae, 0x1d8},
    {0x1e7, 0x210}, {0x221, 0x24b}, {0x25b, 0x285}, {0x293, 0x2bd},
    {0x2cc, 0x2f6}, {0x305, 0x32e}, {0x33c, 0x366}, {0x374, 0x39d},
    {0x3aa, 0x3d4}, {0x3e1, 0x417}, {0x424, 0x44c}, {0x45b, 0x483},
    {0x493, 0x4c6}, {0x4dc, 0x505}, {0x516, 0x53f}, {0x54d, 0x578},
    {0x58a, 0x5b3}, {0x5c2, 0x5ed}, {0x5fd, 0x625}, {0x634, 0x65d},
    {0x66c, 0x695}, {0x6a4, 0x6d4}, {0x6e9, 0x711},
}};

unsigned category_at_label(std::uint32_t label) {
    if (label == 0x10142acc) return 0;
    if (label == 0x10142af8) return 1;
    const auto offset = label - 0x10142b28;
    if (offset > 24 * 0x30 || offset % 0x30)
        throw std::runtime_error("Unrecognized Hall of Fame category label " + hex_address(label));
    return 2 + offset / 0x30;
}

void format_category(Cpu& c, unsigned category, std::uint32_t out, std::uint32_t source) {
    block(c, 0x10142ab0, 2);
    c.compare_logical(0, category, 26);
    block(c, 0x10142ab8, 5);
    const auto table = c.toc(-0x54dc);
    c.set(3, table);
    c.set(0, category * 4);
    const auto label = c.u32(table + category * 4);
    c.set(3, label);
    set_ctr(c, label);

    block(c, label, 2);
    const auto mode = std::int8_t(c.r(6));
    c.set(0, std::uint32_t(mode));
    record_compare(c, mode);
    const auto& phrase = category_phrases[category_at_label(label)];
    const bool has_marker = label != 0x10142acc;
    const auto full = label + (has_marker ? 28 : 24);
    const auto done = full + 12;
    if (mode) {
        const unsigned count = has_marker ? 4 : 3;
        block(c, label + 8, count);
        c.set({{3, out}, {4, source + phrase.compact}});
        if (has_marker) c.set(5, source + 0x15c);
        const auto resume = label + 8 + count * 4;
        c.call(0x10727890, resume);
        block(c, resume, 1);
    } else {
        block(c, full, 3);
        c.set({{3, out}, {4, source + phrase.full}});
        c.call(0x10727890, done);
    }
    block(c, done, 2);
    c.set(3, 1);
}

void category_name(Cpu& c) {
    GuestNativeScope scope(c.s, category_name_entry);
    block(c, category_name_entry, 11);
    const auto caller = c.sp(), out = c.r(4);
    const auto category = std::int8_t(c.r(3));
    const auto capacity = std::int32_t(c.r(5));
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    const auto source = c.toc(-0x54cc);
    c.set(31, source);
    c.put32(caller - 8, c.r(30));
    const auto message = c.toc(-0x7ba8);
    c.set(30, message);
    c.put32(caller - 12, c.r(29));
    const auto success = c.toc(-0x7bac);
    c.set(29, success);
    c.put32(caller + 8, c.r(0));
    c.set(0, std::uint32_t(category));
    record_compare(c, category);
    push_frame(c, scope, caller, caller - 0xa0);

    const bool valid = [&] {
        if (category < 0) return false;
        block(c, 0x10142a3c, 3);
        c.compare(0, category, 26);
        if (category > 26) return false;
        block(c, 0x10142a48, 2);
        c.compare_logical(0, out, 0);
        if (!out) return false;
        block(c, 0x10142a50, 2);
        c.compare(0, capacity, 50);
        return capacity >= 50;
    }();
    if (valid) format_category(c, category, out, source);
    else {
        report_message(c, 0x10142a58, source, message, 0x68, 0x40, 0x3d8);
        block(c, 0x10142aa0, 4);
        c.set({{0, 0}, {3, 0}});
        c.put8(success, 0);
    }
    block(c, 0x1014302c, 7);
    restore_link(c, caller);
    c.set(31, c.u32(caller - 4));
    c.set(30, c.u32(caller - 8));
    c.set(29, c.u32(caller - 12));
    c.finish();
}

void initialize_category_order(Cpu& c, std::uint32_t list) {
    // The original writes sequentially in groups of eight. Preserve the
    // register values at each store as well as the final byte sequence.
    for (unsigned first = 0; ; first += 8) {
        block(c, 0x1014311c, 3);
        c.set(0, first);
        c.compare(0, first, 19);
        if (first >= 19) break;
        block(c, 0x101430bc, 24);
        c.set({{8, first + 5}, {6, first + 1}});
        c.put8(list + first, first);
        c.set({{0, first + 1}, {4, first + 2}});
        c.put8(list + first + 1, first + 1);
        c.set({{0, first + 2}, {5, first + 3}, {9, first + 4}});
        c.put8(list + first + 2, first + 2);
        c.set({{0, first + 3}, {7, first + 5}, {6, first + 6}});
        c.put8(list + first + 3, first + 3);
        c.set({{0, first + 4}, {4, first + 7}, {5, first + 6}});
        c.put8(list + first + 4, first + 4);
        c.set({{0, first + 7}, {10, first + 8}});
        for (unsigned i = 5; i < 8; ++i) c.put8(list + first + i, first + i);
    }
    block(c, 0x10143128, 1);
    for (unsigned id = 24; ; ++id) {
        block(c, 0x10143138, 3);
        c.set(0, id);
        c.compare(0, id, 27);
        if (id == 27) break;
        block(c, 0x1014312c, 3);
        c.put8(list + id, id);
        c.set(10, id + 1);
    }
}

void sorted_categories(Cpu& c) {
    GuestNativeScope scope(c.s, sorted_categories_entry);
    block(c, sorted_categories_entry, 7);
    const auto caller = c.sp(), mode = c.r(3);
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    const auto list = c.toc(-0x54e0);
    c.set({{31, list}, {10, 0}});
    c.put32(caller + 8, c.r(0));
    push_frame(c, scope, caller, caller - 0x40);
    initialize_category_order(c, list);
    block(c, 0x10143144, 6);
    c.put8(c.r(2) + std::uint32_t(category_label_mode), mode);
    c.set(3, list);
    c.set(6, c.toc(-0x54e4));
    c.set({{4, 27}, {5, 1}});
    c.call(0x10768300, 0x1014315c);
    block(c, 0x1014315c, 6);
    c.set(3, list);
    restore_link(c, caller);
    c.set(31, c.u32(caller - 4));
    c.finish();
}

bool nonzero_category(Cpu& c, std::uint32_t id) {
    const auto value = std::int8_t(id);
    c.set(0, std::uint32_t(value));
    record_compare(c, value);
    return value != 0;
}

void compare_category_names(Cpu& c, std::uint32_t right) {
    const auto frame = c.sp();
    block(c, 0x101431e8, 4);
    c.set({{6, c.u8(c.r(2) + std::uint32_t(category_label_mode))}, {4, frame + 0x100}, {5, 200}});
    call_owned(c, category_name_entry, 0x101431f8, category_name);
    block(c, 0x101431f8, 5);
    c.set({{6, c.u8(c.r(2) + std::uint32_t(category_label_mode))},
           {3, right}, {4, frame + 0x38}, {5, 200}});
    call_owned(c, category_name_entry, 0x1014320c, category_name);
    block(c, 0x1014320c, 5);
    c.set({{3, frame + 0x100}, {4, frame + 0x38}, {5, 1}, {6, 0}});
    c.call(0x107465f0, 0x10143220);
}

void compare_category_rows(Cpu& c, std::uint32_t left_at, std::uint32_t right_at) {
    block(c, 0x101431b0, 4);
    const auto left = c.u8(left_at);
    c.set(3, left);
    const auto right = c.u8(right_at);
    c.set(31, right);
    if (!nonzero_category(c, left)) {
        block(c, 0x101431c0, 2);
        if (nonzero_category(c, right)) {
            block(c, 0x101431c8, 2);
            c.set(3, UINT32_MAX);
            return;
        }
    }
    block(c, 0x101431d0, 2);
    if (nonzero_category(c, left)) {
        block(c, 0x101431d8, 2);
        if (!nonzero_category(c, right)) {
            block(c, 0x101431e0, 2);
            c.set(3, 1);
            return;
        }
    }
    compare_category_names(c, right);
}

void compare_categories(Cpu& c) {
    GuestNativeScope scope(c.s, compare_categories_entry);
    block(c, compare_categories_entry, 6);
    const auto caller = c.sp(), left = c.r(3), right = c.r(4);
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    c.compare_logical(0, left, 0);
    c.put32(caller + 8, c.r(0));
    push_frame(c, scope, caller, caller - 0x1d0);
    if (!left) {
        block(c, 0x10143198, 2);
        c.set(3, 1);
    } else {
        block(c, 0x101431a0, 2);
        c.compare_logical(0, right, 0);
        if (!right) {
            block(c, 0x101431a8, 2);
            c.set(3, UINT32_MAX);
        } else compare_category_rows(c, left, right);
    }
    block(c, 0x10143220, 5);
    restore_link(c, caller);
    c.set(31, c.u32(caller - 4));
    c.finish();
}

void construct_table(Cpu& c) {
    GuestNativeScope scope(c.s, construct_table_entry);
    block(c, construct_table_entry, 10);
    const auto caller = c.sp(), table = c.r(3);
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    c.set(4, c.toc(-0x7404));
    c.set(31, table);
    c.put32(caller + 8, c.r(0));
    c.set({{5, 0}, {6, record_size}, {7, table_size / record_size}});
    push_frame(c, scope, caller, caller - 0x40);
    c.call(0x10773320, 0x10140758);
    block(c, 0x10140758, 5);
    c.set({{3, table}, {4, 0}, {5, table_size}});
    c.call(clear_memory, 0x1014076c);
    block(c, 0x1014076c, 2);
    c.set(3, table);
    call_owned(c, read_file_entry, 0x10140774, table_file<false>);
    if (!nonzero_byte_result(c, 0x10140774)) {
        block(c, 0x1014077c, 2);
        c.set(3, table);
        call_owned(c, write_file_entry, 0x10140784, table_file<true>);
    }
    block(c, 0x10140784, 6);
    c.set(3, table);
    restore_link(c, caller);
    c.set(31, c.u32(caller - 4));
    c.finish();
}

}

void hall_of_fame_run_original(State& state, std::uint32_t entry) {
    run_original_body(state, entry, declining);
}

void hall_of_fame_run_rewrite(State& state, std::uint32_t entry) {
    Cpu c(state);
    switch (entry) {
    case merge_record_entry: merge_record(c); break;
    case import_file_entry: import_file(c); break;
    case validate_file_entry: validate_file(c); break;
    case category_name_entry: category_name(c); break;
    case sorted_categories_entry: sorted_categories(c); break;
    case compare_categories_entry: compare_categories(c); break;
    case construct_table_entry: construct_table(c); break;
    case read_file_entry: table_file<false>(c); break;
    case write_file_entry: table_file<true>(c); break;
    case initialize_record_entry: initialize_record(c); break;
    case compare_rank_entry: compare_rank(c); break;
    case record_at_entry: record_at(c); break;
    case add_default_entry: add_default(c); break;
    case add_for_nation_entry: add_for_nation(c); break;
    case insert_ranked_entry: insert_ranked(c); break;
    case category_for_nation_entry: category_for_nation(c); break;
    case encode_entry: transform_file<true>(c); break;
    case decode_entry: transform_file<false>(c); break;
    default: throw std::logic_error("Unknown Hall of Fame entry " + hex_address(entry));
    }
}

bool try_modern_hall_of_fame(State& state, std::uint32_t entry) {
    if (declining || (routine_checker && !routine_checker->wants(entry))) return false;
    if (routine_checker)
        routine_checker->check(state, entry, hall_of_fame_run_original, hall_of_fame_run_rewrite,
                               RoutineChecker::result_r3);
    hall_of_fame_run_rewrite(state, entry);
    return true;
}
}
