// MATCH_ENG.CPP, events: the six routines that adjust a side's players by their role flags.
#include "match_eng_events.hpp"
#include <array>

namespace cm::aot::match_eng {
using namespace guest;
using namespace events_detail;

namespace {
constexpr std::uint32_t usable_at = 0x101d3b50, role_flags_at = 0x10289560;
// How one role-flag bit scales the amount: by a double of the constants pool, or not at all
// (plain), or the player is left alone (skip).
constexpr std::int32_t skip = -1, plain = -2;
struct RoleStep {
    std::uint32_t mask;
    std::int32_t factor;
};
using RoleSteps = std::array<RoleStep, 7>;  // bits 1, 2, 4, 8, 0x10, 0x20, 0x40: the first set one counts

constexpr RoleSteps steps_a{{{1, skip}, {2, 0x168}, {4, 0x170}, {8, 0x178}, {0x10, 0x118}, {0x20, 0x180}, {0x40, 0x188}}};
constexpr RoleSteps steps_b{{{1, 0x190}, {2, 0x198}, {4, plain}, {8, 0x1a0}, {0x10, 0x1a8}, {0x20, 0x170}, {0x40, 0x1b0}}};
constexpr RoleSteps steps_c{{{1, skip}, {2, 0x118}, {4, 0x118}, {8, 0x118}, {0x10, 0x118}, {0x20, 0x118}, {0x40, 0x1b8}}};
constexpr RoleSteps steps_d{{{1, skip}, {2, 0x1c0}, {4, 0x1c0}, {8, 0x1c8}, {0x10, 0x1d0}, {0x20, 0x1d8}, {0x40, 0x1b8}}};
constexpr RoleSteps steps_e{{{1, 0x190}, {2, 0x90}, {4, 0x90}, {8, 0x90}, {0x10, 0x90}, {0x20, 0x1e0}, {0x40, 0x168}}};
constexpr RoleSteps steps_f{{{1, 0x98}, {2, 0xe0}, {4, 0x168}, {8, 0x170}, {0x10, 0x1b0}, {0x20, 0x1e8}, {0x40, 0x1e8}}};

// The shared body. The first scaling step of a routine keeps the converted amount at sp+0x40 and the
// truncated product at sp+0x38; every later one swaps the two slots (the stack image shows both).
void adjust_players(Cpu& c, std::uint32_t entry, std::uint32_t match, std::uint32_t side, std::uint32_t amount,
                    const RoleSteps& steps) {
    Frame frame(c, entry, 0x60, {.first_gpr = 26});
    c.put32(frame.at(0x78), match);
    c.put8(frame.at(0x7f), side);
    Fpu fp(c);
    Kept<28> amount_register(c, amount);
    Kept<31> pool(c, c.toc(-0x52c4));
    const auto side_offset = u(s8(c.u8(frame.at(0x7f))) * std::int32_t(side_size));
    c.set(4, c.u32(frame.at(0x78)));
    c.set(3, side_offset);
    Kept<30> player(c, c.r(4) + side_offset + players_at);
    Kept<27> index(c, 0);
    const auto first_scaling_mask = steps[0].factor == skip ? steps[1].mask : steps[0].mask;
    while (compare(c, s8(index), 0x14) < 0) {
        c.set(3, player);
        match_eng::call(c, usable_at, entry + 0x48);
        if (compare(c, s8(c.r(3))) >= 0) {
            c.set(3, player);
            match_eng::call(c, role_flags_at, entry + 0x5c);
            Kept<29> flags(c, c.r(3));
            const auto bits = std::int32_t(std::int16_t(std::uint32_t(flags)));
            for (const auto& step : steps) {
                // The first set bit ends the chain; the routine's first scaling step is laid out differently.
                if (compare(c, bits & std::int32_t(step.mask)) == 0) continue;
                if (step.factor == skip) break;
                const bool first_scaling = step.mask == first_scaling_mask;
                const auto before = extended(c.i16(player + 0x36));
                c.set(3, before);
                const auto slot = first_scaling ? 0x40u : 0x38u, result = first_scaling ? 0x38u : 0x40u;
                const double magic = c.f64(pool + 0x60);
                c.set_f(1, magic);
                const auto scaled_amount = std::uint32_t(std::int32_t(std::int16_t(std::uint32_t(amount_register))));
                c.put32(frame.at(slot + 4), scaled_amount ^ 0x80000000);
                c.put32(frame.at(slot), 0x43300000);
                double value = fp.fsub(c.f64(frame.at(slot)), magic);
                if (step.factor != plain) value = fp.fmul(c.f64(pool + std::uint32_t(step.factor)), value);
                c.put64(frame.at(result), fp.fctiwz(0, value));
                c.put16(player + 0x36, before + c.u32(frame.at(result + 4)));
                break;
            }
        }
        index = index + 1;
        c.set(26, player);
        player = player + player_size;
    }
}
}

void adjust_players_a(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t amount) {
    adjust_players(c, 0x101de0a0, match, side, amount, steps_a);
}
void adjust_players_b(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t amount) {
    adjust_players(c, 0x101de340, match, side, amount, steps_b);
}
void adjust_players_c(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t amount) {
    adjust_players(c, 0x101de610, match, side, amount, steps_c);
}
void adjust_players_d(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t amount) {
    adjust_players(c, 0x101de8b0, match, side, amount, steps_d);
}
void adjust_players_e(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t amount) {
    adjust_players(c, 0x101deb50, match, side, amount, steps_e);
}
void adjust_players_f(Cpu& c, std::uint32_t match, std::uint32_t side, std::uint32_t amount) {
    adjust_players(c, 0x101dee30, match, side, amount, steps_f);
}
}
