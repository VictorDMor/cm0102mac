// MANAGER_MANAGER.CPP tail block (manager_manager_tail.hpp), native-first: 0x101c0230, the club's weighing of a manager's
// application. Values live in native locals; the decision record (0x20 bytes, copied into list[5] at the end) lives in
// the frame at the original's offsets, because the copy takes its padding bytes from the stack too.
#include "manager_manager_tail.hpp"
#include "guest_fpu.hpp"
#include "manager_manager_assess_detail.hpp"
#include "manager_manager_detail.hpp"
#include <bit>

namespace cm::aot::manager_manager {
using guest::Cpu;
namespace {
namespace toc_cell {
constexpr std::int32_t date = -0x7b80, slots = -0x7ab4, gate = -0x7428, tiers = -0x7984, reaction = -0x7b94,
                       constants = -0x536c, league_a = -0x7598, league_b = -0x7650, league_c = -0x7a00;
}
namespace callee {
constexpr std::uint32_t init_date = 0x10715880, date_of = 0x10717890, append_record = 0x10738d10, gate_level = 0x102d5880,
                        is_young = 0x100dbb60, is_veteran = 0x100dbb20, react = 0x106a15f0;
}
constexpr std::uint32_t frame_size = 0xf0;
// The decision record in the frame (0x20 bytes): the person's index, the outcome byte at +4, the date at +8 (5 bytes
// of the date the call 0x10717890 returns), the five argument bytes at +0x10, then the stage byte, the number
// halfword, two result bytes and the kind byte.
namespace record {
constexpr std::uint32_t base = 0x60, id = 0, outcome = 4, date = 8, bytes = 0x10, stage = 0x15, number = 0x16, grade = 0x18,
                        wait = 0x1a, kind = 0x1c;
}
// Pool constants (TOC -0x536c): scale factors and the int-to-double magic.
constexpr std::uint32_t pool_scale_a = 0x20, pool_scale_b = 0x28, pool_magic = 0x38;
namespace person_at { constexpr std::uint32_t profile = 0x14, club = 0x24, trait = 0x67; }
namespace club_at { constexpr std::uint32_t id = 0, region = 0x2c, manager = 0x90, reputation = 0x1ec; }
namespace profile_at { constexpr std::uint32_t talent = 0x1c, form = 0x25, rating = 0xc, value = 0x2a; }  // +0xc, +0x2a halfwords
constexpr std::uint32_t row_stride = 0x4c, row_level = 0x40;

std::int16_t half(std::uint32_t value) { return std::int16_t(std::uint16_t(value)); }
// The five-bit signed count at the low bits of a row's byte (rlwinm/srawi by 27).
std::int32_t level_of(std::uint32_t byte) { return std::int32_t(std::uint32_t(byte & 0x1f) << 27) >> 27; }

class Weigh {
public:
    Weigh(Cpu& cpu, guest::Frame& frame, std::uint32_t list, std::uint32_t person, std::int8_t flag,
                 const Application& a)
        : c(cpu), frame(frame), draws(cpu), list(list), person(person), flag(flag), a(a), date(cpu.toc(toc_cell::date)),
          pool(cpu.toc(toc_cell::constants)) {}

    std::uint32_t run() {
        c.set(3, at(record::date));
        c.call(callee::init_date, 0x101c0294);
        put8(record::outcome, 1);
        if (!person || !c.u32(person + person_at::profile)) return 0;
        std::uint32_t slot_record = 0;
        if (a.slot > -1) slot_record = c.u32(c.toc(toc_cell::slots)) + std::uint32_t(a.slot) * 0x2c;
        if (const auto gate = c.u32(c.toc(toc_cell::gate)); gate) {
            c.set({{3, gate}, {4, slot_record}});
            c.call(callee::gate_level, 0x101c02e8);
            if (std::int8_t(c.r(3)) > 6 && flag != 0) return refuse(0x58, 0x101c0320, 0x101c0370);
        }
        if (flag != 0) put8(record::outcome, 0);
        club = c.u32(person + person_at::club);
        if (!club) return refuse(0x50, 0x101c03b8, 0x101c0408);
        manager = c.u32(club + club_at::manager);
        if (!manager || !c.u32(manager + 0x1c)) return refuse(0x48, 0x101c044c, 0x101c049c);
        manager_profile = c.u32(manager + 0x1c);
        // The count the club wants: its reputation's step (2, 3, 5, 7) and the manager's (0, +1, +3).
        std::int32_t want = 2;
        if (reputation() > 0x1a5e) want = 7;
        else if (reputation() > 0x109a) want = 5;
        else if (reputation() > 0x7d0) want = 3;
        const std::int32_t manager_value = c.i16(manager_profile + profile_at::value);
        if (manager_value > 0x1a5e) want += 3;
        else if (manager_value > 0x109a) want += 1;
        wanted = std::int8_t(want);

        bool event = false;
        if (level() > 3) {
            const auto tier = [&] { return std::int32_t(c.i8(c.u32(c.toc(toc_cell::tiers)) + c.u32(club + club_at::id) * 8 + 4)); };
            if (tier() < -3 && draws.modulo(5, 0x101c0560) == 0) {
                react(1, a.stage, std::uint32_t(std::int32_t(a.number)), 0x101c05c0);
                event = true;
            } else if (level() > 5 && draws.modulo(5, 0x101c0610) == 0) {
                react(3, 0xffffffff, 0xffffffff, 0x101c0670);
                event = true;
            } else if (tier() > -1 && draws.modulo(5, 0x101c06bc) == 0) {
                react(2, 0xffffffff, 0xffffffff, 0x101c071c);
                event = true;
            }
        }
        if (level() >= std::int32_t(wanted) && flag != 0 && draws.modulo(10000, 0x101c0774) > reputation())
            return refuse(0x40, 0x101c07dc, 0x101c082c);
        if (std::int8_t(a.kind) == 3) { put8(record::outcome, 0); put16(record::wait, 0xffff); put8(record::grade, 0xff); }
        if (!event && draws.modulo(1000, 0x101c087c) == 0) react(0, 0xffffffff, 0xffffffff, 0x101c08dc);
        // The row's count goes up by one in its five bits (the original's test against 0x20 can never fail).
        {
            const auto at_row = c.u32(list) + c.u32(club + club_at::id) * row_stride + row_level;
            const auto byte = c.u8(at_row);
            c.put8(at_row, (byte & 0xe0u) | (std::uint32_t(level_of(byte) + 1) & 0x1f));
        }
        const std::int32_t sum = std::int32_t(c.i16(club + club_at::reputation)) + c.i16(manager_profile + profile_at::value);
        const auto person_profile = c.u32(person + person_at::profile);
        const std::int32_t talent = c.i8(person_profile + profile_at::talent), form = c.i8(person_profile + profile_at::form);
        std::int32_t second = std::int32_t(std::uint32_t(talent + form) - std::uint32_t(std::int32_t(c.i8(person + person_at::trait))));
        const std::int32_t first = scaled(pool_scale_a, sum);
        if (c.i16(person_profile + profile_at::rating) > 0x1482) second = scaled(pool_scale_b, second);
        const std::int32_t cost = std::int32_t(std::uint32_t(first) - std::uint32_t(second) * 0x7d);
        if (cost > 0x186a) {
            if (draws.modulo(3, 0x101c09e0) == 0) put8(record::outcome, 0);
        } else if (cost > 0xcb2) {
            if (draws.modulo(0x14, 0x101c0a4c) != 0) put8(record::outcome, 0);
        } else if (draws.modulo(0x32, 0x101c0ab4) != 0) {
            put8(record::outcome, 0);
        }
        const auto league = c.u32(c.u32(club + club_at::region) + 0x68);
        if (league == c.u32(c.toc(toc_cell::league_a)) || league == c.u32(c.toc(toc_cell::league_b))) put8(record::outcome, 0);
        if (std::int8_t(a.kind) == 3 || std::int8_t(c.u8(at(record::outcome))) != 0) {
            put16(record::wait, 0xffff);
            put8(record::grade, 0xff);
        } else {
            decide();
        }
        if (std::int8_t(a.kind) == 4 || std::int8_t(a.kind) == 0 || event) {
            put8(record::outcome, 0);
            put16(record::wait, 0xffff);
            put8(record::grade, 0xff);
        }
        return finish(0x38, 0x101c0f58, 0x101c0fa8);
    }

private:
    Cpu& c;
    guest::Frame& frame;
    assess::Draws draws;
    std::uint32_t list, person;
    std::int8_t flag;
    const Application& a;
    std::uint32_t date, pool, club = 0, manager = 0, manager_profile = 0;
    std::int8_t wanted = 0;

    std::uint32_t at(std::uint32_t field) const { return frame.at(std::int32_t(record::base + field)); }
    void put8(std::uint32_t field, std::uint32_t v) { c.put8(at(field), v); }
    void put16(std::uint32_t field, std::uint32_t v) { c.put16(at(field), v); }
    std::int32_t reputation() const { return c.i16(club + club_at::reputation); }
    std::int32_t level() const {
        return level_of(c.u8(c.u32(list) + c.u32(club + club_at::id) * row_stride + row_level));
    }
    // The club's reaction 0x106a15f0 (the first fixed argument is the game's reaction table cell).
    void react(std::uint32_t code, std::uint32_t seventh, std::uint32_t eighth, std::uint32_t back) {
        c.set({{3, c.u32(c.toc(toc_cell::reaction))}, {4, club}, {5, person}, {6, code}, {7, seventh}, {8, eighth}, {9, 0xffffffff}});
        c.call(callee::react, back);
    }
    // The scaled value through the pool: scale * (magic word pair - magic), truncated by fctiwz.
    std::int32_t scaled(std::uint32_t scale_at, std::int32_t value) {
        guest::Fpu fpu(c);
        const auto bits = (std::uint64_t(0x43300000) << 32) | (std::uint32_t(value) ^ 0x80000000u);
        auto result = fpu.fsub(std::bit_cast<double>(bits), c.f64(pool + pool_magic));
        result = fpu.fmul(c.f64(pool + scale_at), result);
        return std::int32_t(fpu.fctiwz(0, result));
    }
    // Copies the date, the five bytes and the rest into the frame's record and appends it to list[5].
    void append(std::uint32_t temp, std::uint32_t date_back, std::uint32_t append_back) {
        c.set({{3, frame.at(std::int32_t(temp))}, {4, date}, {5, 2}});
        c.call(callee::date_of, date_back);
        c.put16(at(record::date + 2), c.u16(frame.at(std::int32_t(temp + 2))));
        c.put16(at(record::date), c.u16(frame.at(std::int32_t(temp))));
        c.put8(at(record::date + 4), c.u8(frame.at(std::int32_t(temp + 4))));
        c.put32(at(record::id), c.u32(person));
        c.put8(at(record::bytes + 4), a.byte_e);
        c.put16(at(record::number), std::uint32_t(std::int32_t(a.number)));
        c.put8(at(record::stage), a.stage);
        c.put8(at(record::bytes), a.byte_a);
        c.put8(at(record::bytes + 1), a.byte_b);
        c.put8(at(record::bytes + 2), a.byte_c);
        c.put8(at(record::bytes + 3), a.byte_d);
        c.put8(at(record::kind), a.kind);
        c.set({{3, c.u32(list + 0x14)}, {4, at(0)}, {5, 0x20}});
        c.call(callee::append_record, append_back);
    }
    // The early exits: the record is filed as refused with no wait.
    std::uint32_t refuse(std::uint32_t temp, std::uint32_t date_back, std::uint32_t append_back) {
        put8(record::outcome, 0);
        put16(record::wait, 0xffff);
        put8(record::grade, 0xff);
        append(temp, date_back, append_back);
        return 0;
    }
    std::uint32_t finish(std::uint32_t temp, std::uint32_t date_back, std::uint32_t append_back) {
        append(temp, date_back, append_back);
        return c.u8(at(record::outcome));
    }
    // What the club thinks of the application when nothing refused it: the outcome 2 (accepted with a grade) comes
    // from the person's strength and the club's, plus a draw, one of two ways by the person's number and kind of club.
    void decide() {
        const auto person_profile = c.u32(person + person_at::profile);
        const std::int32_t talent = c.i8(person_profile + profile_at::talent), form = c.i8(person_profile + profile_at::form);
        const std::int32_t trait = c.i8(person + person_at::trait);
        // Each step truncated to 16 bits (extsh); the cut for a flag of 0 is not.
        std::uint32_t strength = std::uint32_t(std::int32_t(half(std::uint32_t(talent * talent * 10))));
        strength = std::uint32_t(std::int32_t(half(strength + std::uint32_t(form * form * 10))));
        strength = std::uint32_t(std::int32_t(half(strength - std::uint32_t(trait * trait * 10))));
        if (flag == 0) strength -= 6000u;
        const std::int32_t stage = std::int8_t(a.stage);
        c.set(3, club);
        c.call(callee::is_young, 0x101c0b9c);
        const bool young = std::int8_t(c.r(3)) != 0;
        auto beats = [&](std::uint32_t modulus, std::uint32_t back, std::uint32_t base) {
            const std::int32_t drawn = draws.modulo(std::int32_t(modulus), back);
            return reputation() > std::int32_t(half(strength + std::uint32_t(drawn) + base));
        };
        if (young && a.number > 0x3c) {
            if (!beats(10000, 0x101c0bcc, 0xcb2)) return;
            const auto pick = draws.modulo(5, 0x101c0c38);
            put8(record::outcome, 2);
            put16(record::wait, pick == 1 ? 0x1e : pick == 0 ? 0x28 : pick == 2 ? 0x14 : 10);
            return;
        }
        c.set(3, club);
        c.call(callee::is_veteran, 0x101c0cd8);
        if (std::int8_t(c.r(3)) == 0) return;
        if (stage > 3 && beats(12500, 0x101c0d0c, 0xcb2)) {
            put8(record::outcome, 2);
            const auto league = c.u32(c.u32(club + club_at::region) + 0x68);
            if (league == c.u32(c.toc(toc_cell::league_c))) put8(record::grade, 1);
            else if (league == c.u32(c.toc(toc_cell::league_a))) put8(record::grade, std::uint32_t(draws.draw(0x101c0db4) & 1) + 1);
            else if (stage > 5) put8(record::grade, std::uint32_t(draws.draw(0x101c0e10) & 1) + 1);
            else put8(record::grade, 1);
            return;
        }
        if (stage > 2 && level() < std::int32_t(wanted) && beats(15000, 0x101c0ea8, 0x109a)) {
            put8(record::outcome, 2);
            put8(record::grade, 1);
        }
    }
};
}

Application read_application(const Cpu& c) {
    // r6..r10 are the five bytes; the caller keeps the rest at +0x38 of its frame (byte, halfword, byte, word).
    const auto sp = c.sp();
    return {c.r(6), c.r(7), c.r(8), c.r(9), c.r(10), c.u8(sp + 0x3b), c.i16(sp + 0x3e), c.u8(sp + 0x43), c.i32(sp + 0x44)};
}

std::uint32_t weigh_application(Cpu& c, std::uint32_t list, std::uint32_t person, std::int8_t flag, const Application& a) {
    guest::Frame frame(c, 0x101c0230, frame_size, {.first_gpr = 13});
    const auto outcome = Weigh(c, frame, list, person, flag, a).run();
    c.set(3, outcome);
    return outcome;
}
}
