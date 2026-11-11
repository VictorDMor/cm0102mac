// MANAGER_MANAGER.CPP standing block (manager_manager_standing.hpp), native-first: values live in
// native locals; the guest ABI appears only at the calls to code outside the block.
#include "manager_manager_standing.hpp"
#include "guest_values.hpp"
#include "guest_fpu.hpp"
#include "manager_manager_detail.hpp"
#include <bit>
#include <stdexcept>

namespace cm::aot::manager_manager {
using guest::Cpu;
namespace {
namespace toc_slot { constexpr std::int32_t employer = -0x7ab8, news_desk = -0x7bf0; }
namespace callee {
constexpr std::uint32_t employer_record = 0x10103510, clear_message = 0x102bdf90,
                        relation = 0x100f55b0, file_message = 0x102cc140;
}
constexpr std::uint32_t frame_size = 0x140, buffer_offset = 0x38, message_kind = 0x1786, row_size = 0x4c,
                        row_rating = 6;
constexpr std::uint32_t counter_limit = 1000000;
}

void post_club_message(Cpu& c, std::uint32_t table, std::uint32_t entry_index, std::int8_t first, std::int8_t second) {
    if (!entry_index) return;
    guest::Frame frame(c, 0x101bd320, frame_size, {});  // the stack pointer for the callees; the epilogue puts LR back
    const auto buffer = frame.at(buffer_offset);
    c.set({{3, c.u32(c.toc(toc_slot::employer))}, {4, entry_index}});  // it reads r3 and r4 (masks.txt)
    c.call(callee::employer_record, 0x101bd350);
    const auto employer = c.r(3);
    if (!employer) return;
    c.set({{3, buffer}, {4, message_kind}, {5, 0}});
    c.call(callee::clear_message, 0x101bd368);
    const auto index = c.u32(entry_index);
    set_field(c, buffer, 0, index, 0x101bd378);
    set_field(c, buffer, 1, std::uint32_t(std::int32_t(first)), 0x101bd388);
    set_field(c, buffer, 2, std::uint32_t(std::int32_t(second)), 0x101bd398);
    const auto row = c.u32(table) + index * row_size;
    set_field(c, buffer, 3, std::uint32_t(std::int32_t(c.i16(row + row_rating))), 0x101bd3b8);
    c.set(3, employer);
    c.call(callee::relation, 0x101bd3c0);
    const auto level = std::int8_t(c.r(3));
    bool flag = level < 0 && level >= -2;
    // The employer's 64-bit counter (words +0 high, +4 low), signed, under the limit (the
    // subfc/subfe chain at 0x101bd3fc..0x101bd40c).
    const std::int64_t high = c.i32(employer), low = c.u32(employer + 4);
    if ((high * 0x100000000LL + low) < counter_limit) flag = true;
    set_field(c, buffer, 4, flag ? 1 : 0, 0x101bd424);
    set_field(c, buffer, 5, 0, 0x101bd434);
    c.set({{3, c.u32(c.toc(toc_slot::news_desk))}, {4, buffer}, {5, entry_index}});
    c.call(callee::file_message, 0x101bd448);
}

namespace {
// What the request reads of the world: TOC slots and record offsets (the original's, named by use).
namespace toc_cell {
constexpr std::int32_t clubs = -0x7be8, leagues = -0x7b5c, windows = -0x7b44, date = -0x7b80, constants = -0x536c,
                       employer = -0x7ab8;
}
namespace row {  // the club's row in the table *list (0x4c bytes)
constexpr std::uint32_t flags = 2, mood = 3, count = 4, spare = 5, rating = 6, amount = 0x28, tag = 0x2c, high = 0x30,
                        low = 0x38;
}
namespace flag_bit { constexpr std::uint32_t asked = 1, granted = 2, locked = 4; }  // row+2
namespace club {  // the club record (0x248 bytes)
constexpr std::uint32_t index = 0, worth = 8, league = 0x30, board = 0x3c, owner = 0x80, manager = 0x90,
                        reputation = 0x1ec, board_flag = 0x23e, standing = 0x240, region = 0x2c;  // region: in the 0x248 row of the club table
}
namespace record {  // manager (club+0x90) and owner (club+0x80) records
constexpr std::uint32_t profile = 0x1c, plan_a = 0x4c, plan_b = 0x50, temper = 0x63;
}
namespace profile { constexpr std::uint32_t value = 0x2a, trait = 0x2f, level = 0x3d; }  // int16, int8, int8
namespace window { constexpr std::uint32_t low_a = 0x2c, low_b = 0x2e, high_a = 0x3c, high_b = 0x3e, tag = 0x75; }
// Pool constants (TOC -0x536c): scale factors, the int-to-double magic, the ratio of kind 1.
constexpr std::uint32_t pool_scale_a = 0x00, pool_scale_b = 0x10, pool_scale_c = 0x28, pool_magic = 0x38,
                        pool_ratio = 0x2c0;

class Request {
public:
    Request(Cpu& c, std::uint32_t list, std::uint32_t club_record, std::uint32_t kind)
        : c(c), fpu(c), t(draw_table::cells(c)), list(list), club(club_record), kind(std::int8_t(kind)) {}

    void run() {
        pool = c.toc(toc_cell::constants);
        if (!club) return;
        if (!c.u32(club + club::league)) return;
        const auto manager = c.u32(club + club::manager);
        if (!manager) return;
        if (!c.u32(manager + record::profile)) return;
        c.set(3, club);
        c.call(0x100c7d00, 0x101bba64);
        if (std::int8_t(c.r(3)) != 0) return;
        c.put8(row_at() + row::count, c.u8(row_at() + row::count) + 1u);
        // The count the club is due: 3, or the owner's level over 5, plus 2.
        std::int8_t due = 3;
        if (const auto profile_at = owner_profile(); profile_at)
            due = std::int8_t(std::int32_t(c.i8(profile_at + profile::level)) / 5 + 2);
        const std::int8_t count = c.i8(row_at() + row::count);
        if (due == count && kind != 5) return post(0xc);
        if (count > due && kind != 5) {
            post(0xa);
            c.put16(row_at() + row::rating, std::uint32_t(std::int32_t(c.i16(row_at() + row::rating)) - 0xfa));
            return;
        }
        if (c.i16(row_at() + row::rating) < 0xdac && kind != 5) return post(5);
        if (kind == signed_nibble(c.u8(row_at() + row::mood)) && (c.u8(row_at() + row::flags) & flag_bit::locked))
            return post(3);
        c.put8(row_at() + row::mood, ((std::uint32_t(kind) << 3) & 0x78) | (c.u8(row_at() + row::mood) & 0x87));
        in_window = date_in_window();
        const auto league_list = c.u32(c.u32(c.u32(c.toc(toc_cell::leagues)) + 4 * c.u32(c.u32(club + club::league))) + 8);
        c.set({{3, league_list}, {4, club}});
        c.call(0x10459b50, 0x101bbcc8);
        league = c.r(3);
        if (!league) return;
        c.set({{3, c.u32(c.toc(toc_cell::employer))}, {4, club}});
        c.call(0x10103510, 0x101bbce0);
        employer = c.r(3);
        if (!employer) return;
        switch (kind) {
            case 1: return kind_one();
            case 2: return kind_two();
            case 3: return kind_three();
            case 4: return kind_four();
            case 5: return kind_five();
            case 6: return kind_six();
            default: return;
        }
    }

private:
    Cpu& c;
    guest::Fpu fpu;
    draw_table::Table t;
    std::uint32_t list, club, pool = 0, league = 0, employer = 0;
    std::int8_t kind;
    bool in_window = false;  // the original's r22

    // ---- small views ----
    // The club's row, looked up afresh each time as the original does.
    std::uint32_t row_at() const { return c.u32(list) + c.u32(club + club::index) * 0x4c; }
    // Bits 3..6 of the mood byte as a signed nibble (rlwinm 25,0,4; srawi 28).
    static std::int8_t signed_nibble(std::uint32_t byte) {
        return std::int8_t(std::int32_t((byte >> 3) & 0xf) - ((byte & 0x40) ? 16 : 0));
    }
    // Files the message (0x101bd320) with the request's kind and the code (0: done, others: the reason).
    void post(std::uint32_t code) { post_club_message(c, list, club, kind, std::int8_t(code)); }
    void set_flag(std::uint32_t bit) { c.put8(row_at() + row::flags, c.u8(row_at() + row::flags) | bit); }
    // The employer's 64-bit counter (high word at +0, low at +4), signed: the original's subfc/subfe chains.
    std::int64_t funds() const { return std::int64_t(c.i32(employer)) * 0x100000000LL + c.u32(employer + 4); }
    // The profile record of the owner (club+0x80), or 0.
    std::uint32_t owner_profile() const {
        const auto owner = c.u32(club + club::owner);
        return owner ? c.u32(owner + record::profile) : 0;
    }
    // The manager's value against 5000 (or `floor`) and the reputation less 0x5dc: either refuses.
    bool value_refused(std::int32_t floor = 5000) const {
        const auto profile_at = c.u32(c.u32(club + club::manager) + record::profile);
        const std::int32_t value = c.i16(profile_at + profile::value);
        return value < floor || value < std::int32_t(c.i16(club + club::reputation)) - 0x5dc;
    }
    // The class distance between the club row's tag word and the league's byte at +0x75 (a byte).
    std::int8_t class_gap() const {
        return std::int8_t(std::uint8_t(c.u32(row_at() + row::tag)) - c.u8(league + window::tag));
    }

    // ---- floating point: the original's int-to-double (magic word less the pool constant) and scaling ----
    double to_double(std::int32_t x) {
        const auto bits = (std::uint64_t(0x43300000) << 32) | (std::uint32_t(x) ^ 0x80000000u);
        return fpu.fsub(std::bit_cast<double>(bits), c.f64(pool + pool_magic));
    }
    // trunc(pool[scale_at] * x) through fmul and fctiwz.
    std::int32_t scaled(std::uint32_t scale_at, std::int32_t x) {
        auto f = to_double(x);
        f = fpu.fmul(c.f64(pool + scale_at), f);
        return std::int32_t(fpu.fctiwz(0, f));
    }
    // The amount scaled by the class gap: above 4 by c, up to 0 by a, otherwise unchanged; outside the window
    // always by b.
    std::int32_t by_window(std::int32_t x) {
        if (!in_window) return scaled(pool_scale_b, x);
        const auto gap = class_gap();
        if (gap > 4) return scaled(pool_scale_c, x);
        if (gap <= 0) return scaled(pool_scale_a, x);
        return x;
    }

    // ---- the draw table: one step, then the number it gives ----
    std::int32_t draw(std::uint32_t back) {
        const auto cursor = c.u32(t.cursor_cell) + 2;
        c.put32(t.cursor_cell, cursor);
        if (cursor > c.u32(t.last_cell)) {
            c.put32(t.cursor_cell, t.first);
            c.call(draw_table::redraw, back);
            c.put32(t.base_cell, c.r(3) & 0xffff);
        }
        return std::int32_t(c.u16(c.u32(t.cursor_cell)) + c.u32(t.base_cell));
    }
    // A draw below the byte n (n != 0): the table's number modulo n, with n's sign. The original has a
    // floating-point branch for |n| >= 0x10000 (it calls 0x10195e10); every n reaching here is an extsb of a
    // byte (listing 0x101bbf34, 0x101bc67c, 0x101bca5c, 0x101bce70), so that branch cannot run.
    std::int32_t draw_below(std::int8_t n, std::uint32_t back) {
        const auto rest = remainder(draw(back), n);
        return n > 0 ? rest : -rest;
    }
    // The owner's first draw of kinds 1 and 3 (a byte at +0x63 bounds it; 0 means no chance): the draw's
    // value, 0 when the byte is 0.
    std::int32_t owner_draw(std::uint32_t owner, std::uint32_t back) {
        const auto n = c.i8(owner + record::temper);
        return n == 0 ? 0 : draw_below(n, back);
    }

    // ---- the window test (0x101bbc08..0x101bbca4) ----
    // The class of the club's region (0x10191f80) picks an entry of the window table; the date (year int16,
    // day uint16 at the TOC date cell) must lie between its two bounds, each compared as (year, day).
    bool date_in_window() {
        std::int8_t cls = 0;
        if (const auto word = c.u32(c.u32(c.toc(toc_cell::clubs)) + c.u32(club + club::index) * 0x248 + club::region); word) {
            c.set(3, word);
            c.call(0x10191f80, 0x101bbc18);
            cls = std::int8_t(c.r(3));
        }
        if (cls < 0 || cls >= 0x22) return false;
        const auto entry = c.toc(toc_cell::windows) + std::uint32_t(std::int32_t(cls)) * 0x4c;
        const auto date = c.toc(toc_cell::date);
        const std::int32_t year = c.i16(date);
        const auto day = [&] { return c.u16(date + 2); };
        const bool after = (std::int32_t(c.i16(entry + window::low_a)) < year && day() == c.u16(entry + window::low_b)) ||
                           c.u16(entry + window::low_b) < day();
        if (!after) return false;
        return (year < std::int32_t(c.i16(entry + window::high_a)) && day() == c.u16(entry + window::high_b)) ||
               day() < c.u16(entry + window::high_b);
    }

    // ---- kind 1 (0x101bbd20): needs the board and its budget; the owner's chance decides ----
    void kind_one() {
        if (value_refused()) return post(0xb);
        if (c.u8(row_at() + row::flags) & flag_bit::granted) return post(6);
        const auto board = c.u32(club + club::board);
        if (!board || c.i8(club + club::board_flag) == 0) return post(7);
        const std::int32_t budget = c.i32(board + 4);
        if (budget > 75000) return post(4);
        if (funds() < (budget > 25000 ? 7500000 : budget > 15000 ? 5000000 : 2500000)) return post(2);
        // The worth against the budget (one rounding each, as the original's fsub/fmul/fsub/fcmpo).
        auto budget_f = to_double(budget);
        budget_f = fpu.fmul(c.f64(pool + pool_ratio), budget_f);
        const auto worth_f = to_double(c.i32(club + club::worth));
        fpu.fcmp(0, worth_f, budget_f);
        if (!(worth_f > budget_f)) return post(4);
        if (const auto owner = c.u32(club + club::owner); owner) {
            if (owner_draw(owner, 0x101bbf64) == 0) return post(4);
        } else if (draw(0x101bc14c) % 5 == 0) {
            return post(4);
        }
        post(0);
        set_flag(flag_bit::granted);
    }

    // ---- kind 2 (0x101bc1d8): a request by the club's standing and funds ----
    void kind_two() {
        if (value_refused(0x109a)) return post(0xb);
        if (c.u8(row_at() + row::flags) & flag_bit::asked) return post(6);
        const std::int8_t standing = c.i8(club + club::standing);
        const auto limit = std::int32_t(std::uint32_t(std::int32_t(standing)) * 0x1e848u + 250000u);
        if (funds() < limit) return post(2);
        const std::int32_t rating = c.i16(club + club::reputation);
        const auto granted = [&] {
            post(0);
            set_flag(flag_bit::asked);
        };
        if (rating > 0x1c52 && standing < 0x14) return granted();
        if (rating > 0x128e && standing < 0xf) return granted();
        if (rating > 0xcb2 && standing < 0xa) return granted();
        if (funds() > 1250000 && standing < 6) return granted();
        if (!c.u32(club + club::owner)) {
            const auto rest = draw(0x101bc4f0) % 20;
            return 0x14 - standing < rest ? post(4) : granted();
        }
        const auto rest = draw(0x101bc424) % 20;
        const std::int32_t temper = c.i8(c.u32(club + club::owner) + record::temper);
        const std::int32_t standing_now = c.i8(club + club::standing);
        return (temper + 10) - standing_now < rest ? post(4) : granted();
    }

    // ---- kind 3 (0x101bc58c): the amount the club may spend; the largest decision ----
    void kind_three() {
        if (value_refused()) return post(0xb);
        const auto row_first = row_at();
        if (c.u32(row_first + row::amount) != 0) return post(6);
        if (funds() < 2500000) return post(2);
        fpu.fcmp(0, c.f64(row_first + row::low), c.f64(row_first + row::high));
        if (c.f64(row_first + row::low) > c.f64(row_first + row::high)) return post(8);
        if (const auto owner = c.u32(club + club::owner); owner) {
            if (owner_draw(owner, 0x101bc6a8) == 0) return post(4);
        } else if (draw(0x101bc810) % 5 == 0) {
            return post(4);
        }
        c.set({{3, employer}, {4, 0}, {5, 0}, {6, 0}, {7, 1}, {8, 0}});
        c.call(0x100f5770, 0x101bc880);
        // The original adds r4 as that routine leaves it (a second result: its callers read it too).
        const auto second_result = std::int32_t(c.r(4));
        const std::int32_t rating = c.i16(club + club::reputation);
        const auto row_now = row_at();
        const std::int32_t current = std::int32_t(fpu.fctiwz(0, c.f64(row_now + row::low))) + second_result;
        std::int32_t cost = rating > 0x1d4c   ? (rating - 0x1d4c) * 0x1388 + 12500000
                            : rating > 0x1482 ? (rating - 0x1482) * 1000 + 3000000
                                              : (rating - 5000) * 100 + 50000;
        cost = by_window(cost);
        if (current > cost) return post(4);
        const std::int32_t amount = rating > 0x1d4c   ? (rating - 0x1d4c) * 0x1388 + 5000000
                                    : rating > 0x109a ? (rating - 0x109a) * 1000 + 2000000
                                                      : rating * 100 + 50000;
        if (funds() < amount) {
            bool refused;
            const auto owner = c.u32(club + club::owner);
            const auto owner_profile_at = owner ? c.u32(owner + record::profile) : 0;
            if (!owner_profile_at) {
                refused = draw(0x101bcbf4) % 10 != 0;
            } else {
                const auto n = c.i8(owner_profile_at + profile::trait);
                refused = (n == 0 ? 0 : draw_below(n, 0x101bca8c)) != 0;
            }
            if (refused) return post(2);
        }
        c.put32(row_at() + row::amount, std::uint32_t(amount));
        post(0);
    }

    // ---- kind 4 (0x101bcc74): the spare count, scored from the owner and the employer's relation ----
    void kind_four() {
        if (value_refused()) return post(0xb);
        if (c.i8(row_at() + row::spare) != 0) return post(6);
        if (c.i32(employer + 0x14) > 60000) return post(4);
        std::int32_t score = 0;
        if (const auto profile_at = owner_profile(); profile_at)
            score = std::int8_t(10 - std::int32_t(c.u8(profile_at + profile::trait)));
        c.set(3, employer);
        c.call(0x100f55b0, 0x101bcd44);
        const std::int8_t relation = std::int8_t(c.r(3));
        score += relation == 0 ? 5 : relation == 1 ? 10 : relation == 2 ? 15 : relation == -1 ? -5 : -15;
        score = by_window_byte(score);
        const auto n = std::int8_t(score);
        const std::int32_t chance = n == 0 ? 0 : draw_below(n, 0x101bcea0);
        const auto percent = draw(0x101bcfe8) % 100;
        if (chance <= percent) return post(4);
        std::uint32_t spare;
        if (const auto profile_at = owner_profile(); profile_at) {
            const auto owner = c.u32(club + club::owner);
            const std::int32_t temper = c.i8(owner + record::temper);
            spare = std::uint32_t(temper > 5 ? temper - 5 : 5);
        } else {
            spare = std::uint32_t(draw(0x101bd0a8) % 10 + 5);
        }
        c.put8(row_at() + row::spare, spare);
        c.set({{3, employer}, {4, c.u8(row_at() + row::spare)}});
        c.call(0x100f4f20, 0x101bd114);
        post(0);
    }
    // Kind 4 scales the byte of its score.
    std::int32_t by_window_byte(std::int32_t score) {
        const auto byte = std::int32_t(std::int8_t(score));
        if (!in_window) return scaled(pool_scale_b, byte);
        const auto gap = class_gap();
        if (gap > 4) return scaled(pool_scale_c, byte);
        if (gap <= 0) return scaled(pool_scale_a, byte);
        return score;
    }

    // ---- kind 5 (0x101bd144): a rise of the row's rating when the date is close ----
    void kind_five() {
        if (c.u8(row_at() + row::mood) & 0x80) return post(6);
        if (c.i16(row_at() + row::rating) > 0x10b3) return post(4);
        const auto manager = c.u32(club + club::manager);
        c.set({{3, c.toc(toc_cell::date)}, {4, c.u32(manager + record::plan_a)}, {5, c.u32(manager + record::plan_b)}});
        c.call(0x10718130, 0x101bd1bc);
        std::int32_t score = std::int32_t(c.r(3));
        if (const auto profile_at = owner_profile(); profile_at)
            score = std::int32_t(std::uint32_t(score) - std::uint32_t(std::int32_t(c.i8(profile_at + profile::level))) * 10u);
        else
            score -= 100;
        if (score >= 0xb4) return post(9);
        c.put8(row_at() + row::mood, (c.u8(row_at() + row::mood) & 0x7f) | 0x80);
        c.put16(row_at() + row::rating, std::uint32_t(std::int32_t(c.i16(row_at() + row::rating)) + 0x4e2));
        if (c.i16(row_at() + row::rating) < 0x10b3) c.put16(row_at() + row::rating, 0x10b3);
        post(0);
    }

    // ---- kind 6 (0x101bd284) ----
    void kind_six() {
        if (value_refused()) return post(0xb);
        if (funds() < 500000) post(2);
    }
};
}

void club_request(Cpu& c, std::uint32_t list, std::uint32_t club, std::uint32_t kind) {
    guest::Frame frame(c, 0x101bb9f0, 0x90, {.first_gpr = 19});
    Request(c, list, club, kind).run();
}

namespace {
namespace text_cell { constexpr std::int32_t buffer = -0x7bb8, clubs = -0x7be8, pool = -0x5368, constants = -0x536c; }
constexpr std::uint32_t club_name = 0x1ee, club_standing = 0x242, headline_tag = 0x7d5, text_pool = 0x20000;
constexpr std::uint32_t message_field_getter = 0x102cb570, formatter = 0x10727890, plain_formatter = 0x1077b720,
                        heading_formatter = 0x1077dbf0, text_to_object = 0x1074b830, name_to_object = 0x1074bab0;
constexpr std::uint32_t standing_limit_slot = 0x2c8;

// The board's reply texts (listing 0x101bd460..0x101bde64). Every text is a string of the pool at
// text_pool - k; a formatting site is the address of its block's first instruction (the call's return
// address is 0x30 after it for a text with the club's name, 0x20 for a plain one, 0x10 for a headline). The original reaches the blocks through five jump tables (TOC -0x540c..-0x541c, the
// kind and then the reason; -0x5420 for the headlines): static data of the executable, written here as switches.
class ReplyText {
public:
    ReplyText(Cpu& c, std::uint32_t object, std::uint32_t record, std::uint32_t out)
        : c(c), fpu(c), object(object), record(record), out(out) {}

    void with_reason() {
        const auto field = [&](unsigned n, std::uint32_t back) {
            c.set({{3, record}, {4, n}});
            c.call(message_field_getter, back);
            return c.r(3);
        };
        index = field(0, 0x101bd49c);
        const auto kind_byte = std::int8_t(field(1, 0x101bd4ac));
        const auto reason_byte = std::int8_t(field(2, 0x101bd4bc));
        const std::int32_t rating = std::int16_t(field(3, 0x101bd4cc));
        const auto short_of_money = std::int8_t(field(4, 0x101bd4dc));
        with_registers = true;
        saved_r24 = std::uint32_t(rating);
        saved_r25 = std::uint32_t(std::int32_t(reason_byte));
        saved_r26 = std::uint32_t(std::int32_t(kind_byte));
        const auto reason_code = std::uint8_t(reason_byte);  // the original's jump bounds are unsigned compares
        if (reason_byte == 0xc) {
            say(0x101bd4e8, 0x1da0);
        } else if (reason_byte == 0xa) {
            say(0x101bd524, 0x1d0b);
        } else if (std::uint8_t(kind_byte) > 6) {
            plain(0x101bdd18, 0x818);
        } else {
            switch (kind_byte) {
                case 0: plain(0x101bdd18, 0x818); break;
                case 1: stadium(reason_code); break;
                case 2: training(reason_code); break;
                case 3: transfer_funds(reason_code, rating, short_of_money); break;
                case 4: wages(reason_code); break;
                case 5: time_to_rebuild(reason_byte); break;
                default: plain(0x101bdcf4, 0x8b3); break;  // 6: a placeholder text
            }
        }
        // Leftover-stack reader: 0x1074bab0 appends a 0xd0-byte row built in its own frame (0x1074bc88), whose bytes after
        // the name string are the register save area (stmw r22-r31 at base-0x28) left by the last callee that wrote it, the
        // formatter, or 0x1074b830. They are the original's r22-r31 (the module object, the record, the three fields,
        // the pool, the output, the clubs cell, the text buffer, the club index), so those are set before the formatter
        // calls (original_registers) and here.
        original_registers();
        c.set({{3, out}, {4, buffer()}});
        c.call(text_to_object, 0x101bdd44);
        file_name(0x101bdd60);
    }

    void headline() {
        c.set({{3, record}, {4, 0}});
        c.call(message_field_getter, 0x101bdd70);
        index = c.r(3);
        c.set({{3, record}, {4, 1}});
        c.call(message_field_getter, 0x101bdd80);
        const auto kind = std::int8_t(c.r(3));
        // Table -0x5420: kinds 1..6 their own headline, 0 and everything above 6 the last (0x1077dbf0).
        const auto kind_index = std::uint8_t(kind);
        constexpr std::uint32_t headings[7][2] = {{0, 0}, {0x101bdda0, 0x776}, {0x101bddb4, 0x74a}, {0x101bddc8, 0x71b},
                                                  {0x101bdddc, 0x6ee}, {0x101bddf0, 0x6bd}, {0x101bde04, 0x689}};
        if (kind_index > 6 || kind_index == 0) {
            c.set({{3, buffer()}, {4, pool_text(0x655)}});
            original_registers();
            c.call(heading_formatter, 0x101bde28);
        } else {
            c.set({{3, buffer()}, {4, pool_text(headings[kind_index][1])}});
            original_registers();
            c.call(formatter, headings[kind_index][0] + 0x10);
        }
        c.set({{3, out}, {4, buffer()}});
        c.call(text_to_object, 0x101bde34);
        file_name(0x101bde50);
    }

private:
    Cpu& c;
    guest::Fpu fpu;
    std::uint32_t object, record, out, index = 0;
    bool with_registers = false;
    std::uint32_t saved_r24 = 0, saved_r25 = 0, saved_r26 = 0;
    // The registers the formatter's stmw (at base-0x28) leaves as the original's; see the comment at the call.
    void original_registers() {
        c.set({{22, object}, {23, record}, {27, c.toc(text_cell::pool)}, {28, out}, {29, c.toc(text_cell::clubs)},
               {30, buffer()}, {31, index}});
        if (with_registers) c.set({{24, saved_r24}, {25, saved_r25}, {26, saved_r26}});
    }

    std::uint32_t buffer() const { return c.toc(text_cell::buffer); }
    std::uint32_t pool_text(std::uint32_t k) const { return c.toc(text_cell::pool) + text_pool - k; }
    std::uint32_t clubs() const { return c.u32(c.toc(text_cell::clubs)); }
    std::uint32_t club_row() const { return clubs() + index * 0x248; }
    // The club's name is at club+0x1ee (a string in the record); the formatter takes its address.
    void say(std::uint32_t site, std::uint32_t k) {
        const std::int32_t standing = std::int8_t(c.u8(club_row() + club_standing));
        c.set({{3, buffer()}, {4, pool_text(k)}, {5, std::uint32_t(standing)}, {6, std::uint32_t(standing)},
               {7, club_row() + club_name}});
        original_registers();
        c.call(formatter, site + 0x30);
    }
    // The texts that name the club only (1077b720; a placeholder).
    void plain(std::uint32_t site, std::uint32_t k) {
        c.set({{3, buffer()}, {4, pool_text(k)}, {5, club_row() + club_name}});
        original_registers();
        c.call(plain_formatter, site + 0x20);
    }
    // The club's name goes to the output object with the headline tag (0x1074bab0): r6 = the club's row.
    void file_name(std::uint32_t back) {
        const auto row = club_row();
        c.set({{3, out}, {4, row + club_name}, {5, headline_tag}, {6, row}});
        c.call(name_to_object, back);
    }

    // ---- kind 1: the stadium (jump table -0x5410; reasons above 7 take the text of reason 4, the table's default) ----
    void stadium(std::uint8_t code) {
        struct Block { std::uint32_t site, k; };
        constexpr Block blocks[8] = {{0x101bd5c4, 0x1bc8}, {0x101bd5f8, 0x1afa}, {0x101bd62c, 0x1a4a}, {0x101bd660, 0x19be},
                                     {0x101bd6c8, 0x184f}, {0x101bd6c8, 0x184f}, {0x101bd694, 0x18ed}, {0x101bd590, 0x1c56}};
        const auto block = blocks[code > 7 ? 4 : code];
        say(block.site, block.k);
    }
    // ---- kind 2: the training facilities (-0x5414) ----
    void training(std::uint8_t code) {
        struct Block { std::uint32_t site, k; };
        constexpr Block blocks[8] = {{0x101bd718, 0x17c4}, {0x101bd74c, 0x174a}, {0x101bd780, 0x168a}, {0x101bd7b4, 0x15eb},
                                     {0x101bd81c, 0x1477}, {0x101bd81c, 0x1477}, {0x101bd7e8, 0x150a}, {0x101bd81c, 0x1477}};
        const auto block = blocks[code > 7 ? 4 : code];
        say(block.site, block.k);
    }
    // ---- kind 3: transfer funds (-0x5418; reasons above 8 behave as 4) ----
    void transfer_funds(std::uint8_t code, std::int32_t rating, std::int8_t short_of_money) {
        switch (code > 8 ? 4 : code) {
            case 0: return say(0x101bd86c, 0x13f1);
            case 1: return say(0x101bd8a0, 0x1375);
            case 2: {
                // The club cannot afford it (rating above 0x1c52 with the shortage flag), or the row's
                // double against the pool's limit decides between two texts.
                if (rating > 0x1c52 && short_of_money != 0) return say(0x101bd8e4, 0x12ab);
                const auto row = c.u32(object) + index * 0x4c;
                const auto limit = c.f64(c.toc(text_cell::constants) + standing_limit_slot);
                const auto value = c.f64(row + 0x38);
                fpu.fcmp(0, value, limit);
                return value > limit ? say(0x101bd938, 0x11ab) : say(0x101bd96c, 0x10f8);
            }
            case 3: return say(0x101bd9a0, 0x1375);
            case 6: return say(0x101bd9d4, 0x107d);
            case 8: return say(0x101bda08, 0xffd);
            default: return short_of_money != 0 ? say(0x101bda44, 0x10f8) : say(0x101bda78, 0xf67);  // 4, 5, 7
        }
    }
    // ---- kind 4: the wage budget (-0x541c) ----
    void wages(std::uint8_t code) {
        struct Block { std::uint32_t site, k; };
        constexpr Block blocks[8] = {{0x101bdac8, 0xef0}, {0x101bdafc, 0xe50}, {0x101bdb30, 0xd7e}, {0x101bdb64, 0xe50},
                                     {0x101bdbcc, 0xc27}, {0x101bdbcc, 0xc27}, {0x101bdb98, 0xcb7}, {0x101bdbcc, 0xc27}};
        const auto block = blocks[code > 7 ? 4 : code];
        say(block.site, block.k);
    }
    // ---- kind 5: time to rebuild (a chain of signed compares) ----
    void time_to_rebuild(std::int8_t reason_byte) {
        if (reason_byte == 4) return say(0x101bdc8c, 0xa31);
        if (reason_byte == 0) return say(0x101bdc24, 0xb82);
        if (reason_byte == 6) return say(0x101bdc58, 0xaeb);
        say(0x101bdcc0, 0x977);
    }
};
}

void board_reply_text(Cpu& c, std::uint32_t object, std::uint32_t record, std::int8_t flag, std::uint32_t out) {
    guest::Frame frame(c, 0x101bd460, 0x60, {.first_gpr = 22});
    ReplyText text(c, object, record, out);
    if (flag != 0) text.with_reason();
    else text.headline();
}
}
