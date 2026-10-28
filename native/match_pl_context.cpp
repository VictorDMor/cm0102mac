#include "match_pl_detail.hpp"

namespace cm::aot::match_pl {
using namespace guest;
namespace {
std::uint64_t read_strategy(Cpu& c, std::uint32_t player, std::uint32_t back) {
    const auto team = std::uint32_t(std::int32_t(c.i8(player + 0x27)));
    const auto match = c.u32(player + 0x1a4);
    leave(c, {{3, match + 0x7c6c + team * 0x18e0}, {4, c.u8(player + 0x19)}, {5, match}});
    c.call(0x10113e10, back);
    return (std::uint64_t(c.r(3)) << 32) | c.r(4);
}
std::uint32_t test_strategy(Cpu& c, std::uint32_t player, std::uint32_t back, std::uint32_t mask) {
    const auto flags = std::uint32_t(read_strategy(c, player, back)) & mask;
    // The original subic/subfe pair leaves carry set exactly for a set bit.
    c.s.wr<1>(reg::xer_ca, flags != 0);
    leave(c, {{4, 0}, {5, 0}, {6, flags}, {7, 0}});
    return result(c, flags != 0);
}
std::uint32_t role_from(Cpu& c, std::uint32_t player, std::uint32_t entry, std::uint32_t target) {
    Frame frame(c, entry, 0x40, {});
    c.set(4, player);
    const auto match = Player{c, player}.match();
    if (compare_logical(c, match) == 0) return result(c, 0);
    c.set(3, Player{c, c.r(4)}.match());
    c.call(target, entry + 0x24);
    return c.r(3);
}
std::uint32_t condition_mask(Cpu& c, std::uint32_t player, std::uint32_t back, std::uint32_t mask) {
    const auto masked = std::uint32_t(read_strategy(c, player, back)) & mask;
    leave(c, {{3, masked}, {4, 0}, {5, 0}, {6, masked}, {7, 0}});
    compare(c, std::int32_t(masked));
    return masked;
}
}

std::int32_t larger_value(Cpu& c, std::int32_t first, std::int32_t second) {
    const auto larger = compare(c, first, second) > 0 ? first : second;
    c.set(4, std::uint32_t(larger));
    return std::int32_t(result(c, std::uint32_t(larger)));
}
std::int32_t heading(Cpu& c, std::uint32_t player) { return std::int32_t(result(c, std::uint32_t(c.i16(player + 0x19e)))); }
std::int32_t cached_role(Cpu& c, std::uint32_t player) { return std::int32_t(result(c, std::uint32_t(c.i16(player + 0x1c2)))); }
std::uint32_t side(Cpu& c, std::uint32_t player) { return result(c, c.u8(player + 0x27)); }
void add_pressure(Cpu& c, std::uint32_t player, std::uint32_t amount) { c.put8(player + 0x2c, c.u8(player + 0x2c) + amount); }
void set_pressure(Cpu& c, std::uint32_t player, std::uint32_t amount) { c.put8(player + 0x2c, amount); }
std::uint32_t has_possession(Cpu& c, std::uint32_t player) {
    const auto match = Player{c, player}.match();
    c.set(4, match + 0x10000);
    return result(c, c.u32(match + 0xe000) == player);
}

std::uint32_t contains_marker(Cpu& c, std::uint32_t player, std::uint32_t marker) {
    unsigned index = 0;
    for (;;) {
        const auto count = c.i8(player + 0x104);
        c.set(6, index); c.set(5, index);
        if (compare(c, std::int8_t(index), count) >= 0) return result(c, 0);
        const auto row = player + std::uint32_t(std::int32_t(std::int8_t(index))) * 4;
        c.set(5, row);
        if (compare_logical(c, c.u32(row + 0x1b4), marker) == 0) return result(c, 1);
        ++index;
    }
}
void remove_marker(Cpu& c, std::uint32_t player, std::uint32_t marker) {
    unsigned index = 0;
    c.set(6, UINT32_MAX);
    for (;;) {
        const auto count = c.i8(player + 0x104);
        c.set(7, index); c.set(5, index);
        if (compare(c, std::int8_t(index), count) >= 0) break;
        const auto row = player + std::uint32_t(std::int32_t(std::int8_t(index))) * 4;
        c.set(5, row);
        if (compare_logical(c, c.u32(row + 0x1b4), marker) == 0) { c.set(6, index); break; }
        ++index;
    }
    if (compare(c, std::int8_t(c.r(6)), -1) == 0) return;
    const auto count = std::uint32_t(std::int32_t(c.i8(player + 0x104)));
    const auto destination = player + std::uint32_t(std::int32_t(std::int8_t(c.r(6)))) * 4;
    const auto last = player + count * 4;
    c.set(4, destination); c.set(5, last);
    c.put32(destination + 0x1b4, c.u32(last + 0x1b0));
    c.set(4, c.u8(player + 0x104));
    c.put8(player + 0x104, c.r(4) - 1);
}

std::uint32_t marking_trait(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x102373c0, 0x40, {.first_gpr = 31});
    Kept<31> retained(c, player);
    const auto team = std::uint32_t(std::int32_t(c.i8(player + 0x27)));
    const auto match = Player{c, player}.match();
    leave(c, {{3, match + 0x7c6c + team * 0x18e0}, {4, c.u8(player + 0x19)}, {5, match}});
    c.call(0x101149f0, 0x102373f4);
    return result(c, c.u8(retained + (compare(c, c.r(3) & 2) != 0 ? 0x10d : 0x10b)));
}
std::int32_t team_line(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x10285750, 0x40, {});
    c.set(4, player);
    if (compare_logical(c, Player{c, player}.match()) == 0) return std::int32_t(result(c, UINT32_MAX));
    c.set(3, Player{c, c.r(4)}.match()); c.call(0x101e4e10, 0x10285774);
    return std::int32_t(result(c, std::uint32_t(std::int32_t(std::int8_t(c.r(3))))));
}
std::int32_t match_context(Cpu& c, std::uint32_t player) {
    const auto match = Player{c, player}.match();
    if (compare_logical(c, match) == 0) return std::int32_t(result(c, 0));
    return std::int32_t(result(c, std::uint32_t(std::int32_t(c.i8(match + 0x791f)))));
}
std::uint32_t current_role(Cpu& c, std::uint32_t player) { return role_from(c, player, 0x10289560, 0x101e3d50); }
std::uint32_t previous_role(Cpu& c, std::uint32_t player) { return role_from(c, player, 0x102895a0, 0x101e3e70); }
std::uint32_t secondary_role(Cpu& c, std::uint32_t player) { return role_from(c, player, 0x102895e0, 0x101e3de0); }

std::uint32_t marked_opponent(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x10285ef0, 0x50, {.first_gpr = 29});
    keep(c, {{30, player}});
    const auto tactical_call = [&](std::uint32_t target, std::uint32_t back) {
        const auto side = std::uint32_t(std::int32_t(c.i8(player + 0x27)));
        const auto match = c.u32(player + 0x1a4);
        leave(c, {{3, match + side * 0x18e0 + 0x7c6c}, {4, c.u8(player + 0x19)}, {5, match}});
        c.call(target, back);
    };
    tactical_call(0x101149f0, 0x10285f2c);
    if (compare(c, c.r(3) & 4) == 0) return result(c, 0);
    tactical_call(0x10114b10, 0x10285f54);
    const auto staff = c.r(3);
    const auto other_side = c.i8(player + 0x27) == 0 ? 1u : 0u;
    auto candidate = c.u32(player + 0x1a4) + 0x311c + other_side * 0x2350;
    c.set(4, c.u32(player + 0x1a4) + 0x311c); c.set(5, 0);
    for (unsigned index = 0; ; ++index, candidate += 0x1c4) {
        keep(c, {{31, candidate}});
        if (compare(c, std::int8_t(index), 20) >= 0) return result(c, 0);
        if (compare(c, c.i8(candidate + 0x19)) >= 0 && compare_logical(c, c.u32(candidate + 0x6c), staff) == 0) {
            tactical_call(0x101149f0, 0x10285fc4);
            if (compare(c, c.r(3) & 0x10) != 0) {
                auto match = c.u32(player + 0x1a4);
                const auto opposite = c.i8(player + 0x27) == 0 ? 1u : 0u;
                const auto team = match + opposite * 0x18e0 + 0x7c6c;
                c.set(3, team); c.set(4, match);
                bool constrained = compare(c, c.u32(team + 0x1880) & 0x10000000) != 0;
                if (!constrained) {
                    c.set(3, match + opposite);
                    constrained = compare(c, c.i8(match + opposite + 0xaf), 1) > 0;
                }
                if (constrained && compare(c, c.i8(player + 0x109)) != 0) return result(c, 0);
                enter(c, 0x10286044, {candidate}); current_role(c, candidate);
                keep(c, {{29, std::uint32_t(std::int32_t(std::int16_t(c.r(3))))}});
                enter(c, 0x10286054, {candidate}); secondary_role(c, candidate);
                if (compare(c, std::int16_t(c.r(3)), std::int32_t(c.r(29))) != 0) {
                    enter(c, 0x10286068, {candidate}); secondary_role(c, candidate);
                    if (compare(c, c.r(3) & 0x40) == 0 && compare(c, c.i8(player + 0x109)) != 0) return result(c, 0);
                }
            }
            return result(c, candidate);
        }
        c.set(5, index + 1);
    }
}

std::uint64_t strategy_flags(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x10286470, 0x40, {}); return read_strategy(c, player, 0x1028649c);
}
std::uint64_t alternate_strategy_flags(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x102864b0, 0x40, {}); return read_strategy(c, player, 0x102864dc);
}
std::uint32_t strategy_bit21(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x102862b0, 0x40, {}); return test_strategy(c, player, 0x102862dc, 1u << 21);
}
std::uint32_t strategy_bit25(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x10286320, 0x40, {}); return test_strategy(c, player, 0x1028634c, 1u << 25);
}
std::uint32_t strategy_bit29(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x10286390, 0x40, {}); return test_strategy(c, player, 0x102863bc, 1u << 29);
}
std::uint32_t strategy_bit27(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x10286400, 0x40, {}); return test_strategy(c, player, 0x1028642c, 1u << 27);
}
std::uint32_t team_or_slot_status(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x102864f0, 0x40, {});
    const auto team = std::uint32_t(std::int32_t(c.i8(player + 0x27)));
    const auto context = Player{c, player}.match() + 0x10000 + team * 0x18e0;
    c.set(4, context);
    if (compare(c, c.u32(context - 0x7e0c) & 0x1000) != 0) {
        return result(c, c.u8(Player{c, player}.match() + std::uint32_t(std::int32_t(c.i8(player + 0x27))) + 6));
    }
    return test_strategy(c, player, 0x10286558, 1u << 23);
}
std::uint32_t forward_option(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x102860c0, 0x40, {.first_gpr = 31});
    Kept<31> retained(c, player);
    if (condition_mask(c, retained, 0x102860f4, 1u << 17)) return result(c, 1);
    enter(c, 0x10286130, {retained}); current_role(c, retained);
    if (compare(c, std::int16_t(c.r(3)), 1) == 0) return result(c, 0);
    return result(c, condition_mask(c, retained, 0x1028615c, 1u << 30) ? 2 : 0);
}
std::uint32_t wide_option(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x102861b0, 0x40, {.first_gpr = 31});
    Kept<31> retained(c, player);
    enter(c, 0x102861c8, {retained}); current_role(c, retained);
    if (compare(c, std::int16_t(c.r(3)), 1) != 0 && condition_mask(c, retained, 0x102861f4, 1u << 31)) return result(c, 3);
    enter(c, 0x10286230, {retained}); current_role(c, retained);
    if (compare(c, std::int16_t(c.r(3)), 1) == 0) return result(c, 0);
    return result(c, condition_mask(c, retained, 0x1028625c, 1u << 19) ? 4 : 0);
}
}
