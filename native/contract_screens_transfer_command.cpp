#include "contract_screens.hpp"
#include "guest_fpu.hpp"
#include "screen_manager_detail.hpp"

namespace cm::aot::contract_screens {
using namespace guest;

namespace {
using screen_manager::detail::Manager;
using screen_manager::detail::Program;

constexpr std::uint32_t read_history = 0x1073ef70, find_contract = 0x10082f90, update_news = 0x102cd0c0,
                        adjust_morale = 0x100d3570, place_on_transfer_list = 0x1037fa80;
constexpr std::uint32_t close_screen = std::uint32_t(-9);

struct RandomTable {
    Cpu& c;
    std::uint32_t seed, start, end, cursor;

    static RandomTable world(Cpu& c) {
        return {c, c.toc(-0x7b90), c.toc(-0x7b8c), c.toc(-0x7b88), c.toc(-0x7b84)};
    }

    void advance(std::uint32_t back) const {
        const auto next = c.u32(cursor) + 2, last = c.u32(end);
        c.set({{3, next}, {0, last}});
        const bool wraps = compare_logical(c, next, last) > 0;
        c.put32(cursor, next);
        if (wraps) {
            c.put32(cursor, start);
            const auto seed_word = Program{c}.call(0x1077b770, back) & 0xffff;
            c.set(0, seed_word);
            c.put32(seed, seed_word);
        }
    }

    std::int32_t narrow(std::int32_t bound) const {
        const auto position = c.u32(cursor);
        compare(c, bound);
        const auto sample = std::int32_t(c.u16(position) + c.u32(seed));
        const auto remainder = std::int32_t(std::int64_t(sample) % bound);
        c.set({{0, std::uint32_t(remainder)}, {3, std::uint32_t(sample)}});
        return bound > 0 ? remainder : -remainder;
    }

    std::int32_t draw(std::int32_t bound, std::uint32_t refill_back) const {
        if (compare(c, bound) == 0) return 0;
        advance(refill_back);
        return narrow(bound);
    }
};

std::int32_t high_word(std::int32_t left, std::int32_t right) {
    return std::int32_t((std::int64_t(left) * right) >> 32);
}

void shift_carry(Cpu& c, std::int32_t value, unsigned count) {
    c.s.wr<1>(reg::xer_ca, value < 0 && (std::uint32_t(value) & ((1u << count) - 1)) != 0);
}

std::uint32_t signed_double_word(std::int32_t value) { return std::uint32_t(value) ^ 0x80000000; }

std::int16_t transfer_action(Cpu& c, std::uint32_t screens, std::uint32_t layout, std::int16_t pressed,
                            unsigned scratch) {
    std::int16_t action = 0;
    if (compare(c, pressed, -1) != 0) {
        const auto index = layout + 0xc0000 + std::uint32_t(pressed) * 0x17c;
        c.set(scratch, index);
        action = std::int16_t(c.u32(index - 0x5794));
    }
    if (compare(c, action) == 0) {
        c.set(scratch, screens + 0x260000);
        action = c.i16(c.r(scratch) - 0x7e62);
    }
    c.set(0, std::uint32_t(action));
    return action;
}

bool demands_immediate_listing(Cpu& c, std::uint32_t item, std::uint8_t raw_morale) {
    const auto below = [&](std::int8_t value, int limit) {
        c.set(0, std::uint32_t(value));
        return compare(c, value, limit) < 0;
    };
    return below(std::int8_t(raw_morale), -50) && below(c.i8(item + 0x65), 6) && below(c.i8(item + 0x67), 10);
}

void decline_request(Cpu& c, const Program& program, const RandomTable& rng, std::uint32_t item,
                     std::uint32_t staff_table, std::uint32_t staff_offset, std::uint32_t contract) {
    const auto raw_morale = c.u8(contract + 0x4c);
    c.set(3, raw_morale);
    if (demands_immediate_listing(c, item, raw_morale)) {
        program.call(place_on_transfer_list, 0x10094030, {contract, 0x10000, 0});
    } else {
        const int interest = c.i8(item + 0x65), ambition = c.i8(item + 0x67), persistence = c.i8(item + 0x69);
        const int pressure = std::int8_t(raw_morale) + 50 + 20 * interest;
        const int scale = 5 * (ambition + persistence), tolerance = 375 - scale;
        keep(c, {{19, std::uint32_t(pressure)}, {20, std::uint32_t(tolerance)}});
        c.set({{0, std::uint32_t(scale)}, {3, std::uint32_t(ambition)}, {4, std::uint32_t(20 * interest)}});
        c.s.wr<1>(reg::xer_ca, 375u >= std::uint32_t(scale));

        // The four traits are signed bytes. Pressure is in [-2638,2717]
        // and tolerance in [-895,1655], so neither inlined draw is wide.
        const auto requested = rng.draw(pressure, 0x10094094);
        c.set(27, std::uint32_t(requested));
        const auto resisted = rng.draw(tolerance, 0x100941ec);
        c.set(18, std::uint32_t(resisted));
        if (compare(c, requested, resisted) < 0)
            program.call(place_on_transfer_list, 0x10094330, {contract, 0x10000, 0});
    }

    const auto raw = c.u8(contract + 0x4c);
    const auto base = c.u32(staff_table);
    const auto delta = std::int8_t(5 * int(c.u8(base + staff_offset + 0x65)) - 200);
    const int adjusted = std::int8_t(raw) + delta;
    c.set({{3, raw}, {4, std::uint32_t(delta)}, {0, std::uint32_t(adjusted)}});
    if (compare(c, adjusted, 100) > 0) c.set(0, 100);
    else if (compare(c, adjusted, -100) < 0) c.set(0, std::uint32_t(-100));
    else c.set(0, std::uint32_t(raw + delta));
    c.put8(contract + 0x4c, c.r(0));

    const auto people = c.u32(staff_table);
    c.set(0, people);
    program.call(adjust_morale, 0x10094398, {people + staff_offset, std::uint32_t(-12), 0});
}

void accept_request(Cpu& c, const Program& program, std::uint32_t item, std::uint32_t contract,
                    std::uint32_t offers, std::uint32_t staff_table, std::uint32_t staff_offset,
                    std::uint32_t clubs, std::uint32_t club_offset, std::uint32_t contracts) {
    const auto signed_status = std::int8_t(program.call(0x100d3230, 0x1009449c, {item, 0}));
    c.set(0, std::uint32_t(signed_status));
    const auto recruitment = [&] { return c.u32(c.u32(offers) + 0x988); };
    if (compare(c, signed_status) != 0)
        program.call(0x10365cc0, 0x100944b4, {recruitment(), item});

    const auto flags = c.u8(contract + 0x5c);
    c.set({{3, flags}, {0, flags & 2u}});
    if (compare_logical(c, flags & 2u) != 0) {
        c.set(0, flags & ~2u);
        c.put8(contract + 0x5c, c.r(0));
    }
    c.set(0, c.u8(contract + 0x5c) | 8u);
    c.put8(contract + 0x5c, c.r(0));
    auto people = c.u32(staff_table);
    c.set(0, people);
    program.call(adjust_morale, 0x100944e8, {people + staff_offset, 12, 0});
    people = c.u32(staff_table);
    c.set(0, people);
    program.call(0x10363ca0, 0x10094500, {recruitment(), people + staff_offset, 1});
    const auto club_base = c.u32(clubs);
    c.set(0, club_base);
    program.call(0x1008cd20, 0x10094518, {c.u32(contracts), c.u32(staff_table) + staff_offset, club_base + club_offset});
}
}

std::uint32_t sample_transfer_request(Cpu& c, std::uint32_t bound_word) {
    Frame frame(c, 0x10094560, 0x80, {.first_gpr = 20});
    const auto rng = RandomTable::world(c);
    const auto constants = c.toc(-0x56d0);
    keep(c, {{25, constants}, {26, rng.seed}, {27, rng.start}, {28, rng.end}, {29, rng.cursor}, {30, bound_word}});
    const auto bound = std::int32_t(bound_word);
    if (compare(c, bound) == 0) return 0;

    rng.advance(0x100945b4);
    c.set({{3, 0x10000}, {0, 65535}});
    if (compare(c, bound, 65535) <= 0) {
        c.set({{3, 0xffff0000}, {0, std::uint32_t(-65535)}});
        if (compare(c, bound, -65535) >= 0)
            return std::uint32_t(rng.narrow(bound));
    }

    constexpr std::uint32_t conversion_high = 0x43300000;
    const auto bias = c.f64(constants + 8), divisor = c.f64(constants);
    c.put32(frame.at(0x48), conversion_high);
    c.put32(frame.at(0x4c), signed_double_word(bound));
    Fpu fp(c);
    const auto value = fp.fsub(c.f64(frame.at(0x48)), bias);
    const auto word = c.u16(c.u32(rng.cursor));
    const auto sample = std::int32_t(word + c.u32(rng.seed));
    const auto share = fp.fdiv(value, divisor);
    c.set_f(1, share);
    c.set_f(2, bias);
    const auto residual = sample % 65535;
    const auto tenfold = std::int32_t(bound_word * 10u);
    const auto smaller = tenfold / 65535;
    c.put32(frame.at(0x40), conversion_high);
    c.put32(frame.at(0x44), signed_double_word(residual));
    const auto converted = fp.fsub(c.f64(frame.at(0x40)), bias);
    const auto part = fp.fmul(share, converted);
    const auto bits = fp.fctiwz(0, part);
    c.put64(frame.at(0x38), bits);
    c.set(31, std::uint32_t(bits));

    const auto high = high_word(std::int32_t(0x80008001u), tenfold);
    shift_carry(c, std::int32_t(std::uint32_t(high) + std::uint32_t(tenfold)), 15);
    c.set({{0, signed_double_word(residual)}, {3, c.u32(rng.cursor)}, {4, std::uint32_t(tenfold < 0)},
           {5, std::uint32_t(sample / 65535) * 65535u}, {6, std::uint32_t(sample < 0)}, {7, std::uint32_t(sample)},
           {8, word}, {9, conversion_high}, {22, std::uint32_t(smaller)}});

    // The reduced bound is signed 32-bit wrap(bound * 10) / 65535.
    // Its magnitude is at most 32768, including when the original wraps.
    // The deeper inlined wide forms and the recursive call are unreachable.
    const auto correction = rng.draw(smaller, 0x100946a0);
    c.set(24, std::uint32_t(correction));
    const auto dividend = correction + 5;
    const auto tenth_high = high_word(0x66666667, dividend);
    shift_carry(c, tenth_high, 2);
    c.set({{0, bound_word - 1}, {4, std::uint32_t(tenth_high < 0)}});
    const auto candidate = std::int32_t(std::uint32_t(bits) + std::uint32_t(dividend / 10));
    const auto maximum = std::int32_t(bound_word - 1);
    return std::uint32_t(compare(c, candidate, maximum) > 0 ? maximum : candidate);
}

std::uint32_t transfer_request_command(Cpu& c, std::uint32_t control) {
    Frame frame(c, 0x10093e60, 0x90, {.first_gpr = 18});
    const auto screens = c.toc(-0x7bb0), offers = c.toc(-0x7b40), contracts = c.toc(-0x7b7c),
               clubs = c.toc(-0x7be8), staff_table = c.toc(-0x7be0), news = c.toc(-0x7bf0);
    const auto rng = RandomTable::world(c);
    keep(c, {{19, control}, {21, offers}, {22, c.toc(-0x56d0)}, {23, rng.seed}, {24, rng.start}, {25, rng.end},
             {26, rng.cursor}, {27, contracts}, {28, clubs}, {29, screens}, {30, staff_table}, {31, news}});
    const auto layout = c.u32(Manager{c, screens}.group() + 0x24);
    c.set(3, layout);
    const auto pressed = std::int16_t(control);
    if (compare(c, transfer_action(c, screens, layout, pressed, 4), 0x22) == 0) return close_screen;
    const bool decline = compare(c, transfer_action(c, screens, layout, pressed, 4), 0x21) == 0;
    if (!decline && compare(c, transfer_action(c, screens, layout, pressed, 3), 0x20) != 0) {
        const Program program{c};
        const auto handled = program.call(0x102a54b0, 0x10094528, {control});
        if (compare(c, handled) != 0) return handled;
        const auto common = program.call(0x102a83a0, 0x1009453c, {control});
        const auto mask = std::int32_t(common | (0u - common));
        shift_carry(c, mask, 31);
        return common;
    }

    const Program program{c};
    const auto item = program.call(read_history, decline ? 0x10093f4c : 0x100943f0, {screens, 0});
    c.set(0, item);
    c.set(decline ? 20 : 18, item);
    const auto revision = program.call(read_history, decline ? 0x10093f60 : 0x10094404, {screens, 1});
    c.set(0, revision);
    c.set(decline ? 19 : 20, revision);
    const auto club_id = program.call(read_history, decline ? 0x10093f74 : 0x10094418, {screens, 2});
    c.set(decline ? 18 : 19, club_id);
    const auto club_offset = club_id * 0x248;
    const auto staff_offset = program.call(read_history, decline ? 0x10093f84 : 0x10094428, {screens, 3}) * 0x70;
    c.set(decline ? 21 : 22, staff_offset);
    const auto club_base = c.u32(clubs);
    if (!decline) c.set(23, club_offset);
    c.set(0, decline ? club_offset : club_base);
    const auto contract = program.call(find_contract, decline ? 0x10093fa4 : 0x10094448,
                                       {c.u32(contracts), c.u32(staff_table) + staff_offset, club_base + club_offset});
    c.set(decline ? 28 : 19, contract);
    if (compare(c, std::int32_t(contract)) == 0) return close_screen;

    program.call(update_news, decline ? 0x10093fd0 : 0x10094474, {c.u32(news), item, revision, 7, 1, 0});
    program.call(update_news, decline ? 0x10093fec : 0x10094490, {c.u32(news), item, revision, 8, decline ? 0u : 1u, 0});
    if (decline)
        decline_request(c, program, rng, item, staff_table, staff_offset, contract);
    else
        accept_request(c, program, item, contract, offers, staff_table, staff_offset, clubs, club_offset, contracts);
    return close_screen;
}
}
