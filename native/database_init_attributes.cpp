// Player Initialisation, the player record's attributes: the 4-byte slot array copied from the record
// (database_generation) is completed, tied to the best matching other player's, bounded to 1-20 and written back.
#include "database_generation.hpp"
#include "database_init_detail.hpp"
#include "database_records.hpp"

namespace cm::aot::database::init {
namespace {
// The slot array's offsets on the stack: the 16-bit value of slot i and its flag byte.
constexpr std::uint32_t slot_at(std::uint32_t base, std::uint32_t i) { return base + 4 * i; }

// What the first eight slots' draw (100bd5e0) sets: x = sample % 182 picks a case; each case marks slot 2 or
// 4 with 20 and draws up to three slots as 15-20 (sample % 6 + 15). Offsets are the slots' stack offsets.
struct Pattern {
    std::int32_t below;                 // the case applies while x < below
    std::uint32_t twenty[2];            // slots set to 20 (0: none)
    std::uint32_t draws[3];             // slots drawn, in order (0: none)
};
constexpr Pattern patterns[] = {
    {1, {0x1b4, 0}, {}},
    {7, {0x1b8, 0}, {0x1b4}},
    {0xb, {0x1b8, 0}, {0x1b4, 0x1bc}},
    {0xf, {0x1b8, 0}, {0x1b4, 0x1cc}},
    {0x10, {0x1b8, 0}, {0x1b4, 0x1bc, 0x1c0}},
    {0x11, {0x1b8, 0}, {0x1b4, 0x1cc, 0x1c0}},
    {0x25, {0x1b8, 0}, {}},
    {0x31, {0x1b8, 0}, {0x1bc}},
    {0x3d, {0x1b8, 0}, {0x1cc}},
    {0x49, {0x1b8, 0}, {0x1cc, 0x1c0}},
    {0x55, {0x1b8, 0}, {0x1bc, 0x1c0}},
    {0x56, {0x1b8, 0}, {0x1c8}},
    {0x65, {0x1c0, 0}, {0x1bc}},
    {0x67, {0x1c0, 0}, {0x1bc, 0x1c4}},
    {0x6a, {0x1c0, 0}, {0x1cc, 0x1c4}},
    {0x7e, {0x1c0, 0}, {}},
    {0x8c, {0x1c0, 0}, {0x1c4}},
    {0x8e, {0x1c0, 0}, {0x1c8, 0x1c4}},
    {0x8f, {0x1c0, 0}, {0x1c8}},
    {0x95, {0x1c0, 0x1c4}, {}},
    {0x9b, {0x1c4, 0}, {0x1c8}},
    {0xa1, {0x1c8, 0}, {0x1c4}},
    {0x7fffffff, {0x1c8, 0}, {}},
};
}

// 100bd400 to 100c02c0.
void Gen::player_attributes(std::uint32_t p, std::uint32_t player) {
    auto& luck = g[13];
    auto& other = g[24];
    auto& index = g[16];
    const auto slot16 = [&](std::uint32_t offset) { return s16(c.u16(at(offset))); };
    const auto set16 = [&](std::uint32_t offset, std::int32_t v) { c.put16(at(offset), u32(v)); };
    const auto value_at = [&](std::uint32_t base) { return s16(c.u16(at(slot_at(base, index)))); };
    const auto flag_at = [&]() { return s8(c.u8(at(slot_at(0x1b2, index)))); };
    // A slot whose flag asks for a change: flag 2 beside a strong first slot (15 or more), flag 1 beside a weak one.
    const auto heavy_slot = [&](std::int32_t flag) {
        return (slot16(0x1b0) >= 0xf && flag == 2) || (slot16(0x1b0) < 0xf && flag == 1);
    };

    // Every slot of the first group at its best value (twice in the original: 100bd418 and 100bd58c).
    const auto all_best = [&] {
        c.set(11, 0x14);
        c.set(10, 1);
        for (const auto offset : {0x1b4u, 0x1b8u, 0x1bcu, 0x1c0u, 0x1c4u, 0x1c8u, 0x1d8u, 0x1dcu, 0x1d4u}) set16(offset, 1);
        set16(0x1b0, 0x14);
    };
    // The first slot: a mid-range first value (16 to 19) is lifted to a full set.
    if (slot16(0x1b0) > 0xf && slot16(0x1b0) < 0x14) all_best();

    // 100bd468: the first eight slots, a 20 among them, else the best of those between 6 and 19.
    std::int32_t best = -1;
    bool has_twenty = false;
    for (index = 0; std::int32_t(index) <= 7; index = index + 1) {
        const auto v = slot16(slot_at(0x1b0, index));
        if (v == 20) {
            has_twenty = true;
            break;
        }
        if (v > 5 && v < 20 && (s8(u32(best)) == -1 || v > slot16(slot_at(0x1b0, std::uint32_t(s8(u32(best))))))) {
            best = s8(index);
        }
    }
    if (!has_twenty) {
        if (s8(u32(best)) > -1) {
            set16(slot_at(0x1b0, std::uint32_t(s8(u32(best)))), 0x14);
        } else if (rem_s(next(), 20) == 0) {
            all_best();
        } else {
            const auto x = s16(u32(rem_s(next(), 182)));
            for (const auto& pattern : patterns) {
                if (x >= pattern.below) continue;
                for (const auto offset : pattern.twenty) if (offset) set16(offset, 0x14);
                for (const auto offset : pattern.draws) {
                    if (!offset) continue;
                    set16(offset, s8(u32(rem_s(next(), 6) + 15)));
                }
                break;
            }
        }
    }

    // 100be33c: tie the weaker slots together.
    if (slot16(0x1b0) != 0x14) {
        if (slot16(0x1b4) >= 0xf) {
            set16(0x1dc, 0x14);
        } else if (slot16(0x1cc) >= 0xf && slot16(0x1d4) < 0xf && slot16(0x1d8) < 0xf) {
            if (rem_s(next(), 10) < 3) set16(0x1d8, 0x14);
            else set16(0x1d4, 0x14);
        }
        // 100be99c: until one of 0x1d4, 0x1d8, 0x1dc reaches 20, each is raised on draws.
        const auto settled = [&] { return slot16(0x1d4) >= 0x14 || slot16(0x1d8) >= 0x14 || slot16(0x1dc) >= 0x14; };
        while (!settled()) {
            const auto step = [&](std::uint32_t offset, std::int32_t gate, bool gate_is_below) {
                // A draw below 11 gates the raise; otherwise a draw below the slot's value decides it.
                const auto r = rem_s(next(), 11);
                bool raise = gate_is_below ? r < gate : r > gate;
                if (!raise) {
                    g[15] = u32(slot16(offset));
                    luck = u32(g[15] == 0 ? 0 : below(s16(g[15])));
                    raise = std::int32_t(luck) > 5;
                }
                if (raise) set16(offset, 0x14);
            };
            step(0x1dc, 4, true);
            step(0x1d8, 8, false);
            step(0x1d4, 4, true);
        }
    }
    // 100be9c4
    if (slot16(0x1c4) >= 0xf && slot16(0x1c0) < 0xf && slot16(0x1dc) < 0xf) set16(0x1c0, slot16(0x1c4));
    const auto half = [&](std::int32_t v) { return (v + (v < 0 ? 1 : 0)) >> 1; };
    set16(0x1c0, s8(u32(slot16(0x1c0) > half(slot16(0x1c4)) ? slot16(0x1c0) : half(slot16(0x1c4)))));
    set16(0x1b8, s8(u32(slot16(0x1b8) > half(slot16(0x1b4)) ? slot16(0x1b8) : half(slot16(0x1b4)))));
    if (slot16(0x1c8) >= 0xf && slot16(0x1dc) < 0xf) {
        luck = u32(s8(u32(rem_s(next(), 11) + 10)));
        const auto fresh = s8(u32(rem_s(next(), 7) + 14));
        set16(0x1c4, s8(u32(fresh > slot16(0x1c4) ? fresh : slot16(0x1c4))));
        set16(0x1c0, s8(u32(s8(luck) > slot16(0x1c0) ? s8(luck) : slot16(0x1c0))));
    }

    // 100beb88: slots 12 and 13 (one of the pair is raised to 20, the other drawn).
    const auto in_range = [&](std::uint32_t offset) { return slot16(offset) >= 1 && slot16(offset) <= 0x14; };
    const auto draw_slot = [&](std::uint32_t offset) {
        luck = u32(rem_s(next(), 20));
        g[15] = luck == 0 ? 0 : u32(below(std::int32_t(luck)));
        set16(offset, s8(u32(std::int32_t(g[15]) + 1)));
    };
    if (slot16(0x1e0) != 0x14 && slot16(0x1e4) != 0x14) {
        if (in_range(0x1e0) && in_range(0x1e4)) {
            if (slot16(0x1e4) > slot16(0x1e0)) set16(0x1e4, 0x14);
            else set16(0x1e0, 0x14);
        } else if (in_range(0x1e0)) {
            set16(0x1e4, 0x14);
        } else if (in_range(0x1e4)) {
            set16(0x1e0, 0x14);
        } else if (slot16(0x1d8) > slot16(0x1d4)) {
            set16(0x1e0, 0x14);
            draw_slot(0x1e4);
        } else {
            set16(0x1e4, 0x14);
            draw_slot(0x1e0);
        }
    }
    if (!in_range(0x1e4)) draw_slot(0x1e4);
    if (!in_range(0x1e0)) draw_slot(0x1e0);
    if (slot16(0x1e0) != 0x14 && slot16(0x1e4) != 0x14) {
        if (rem_s(next(), 4) != 0) set16(0x1e4, 0x14);
        else set16(0x1e0, 0x14);
    }

    // 100bf4a0: are all slots from the 13th on within 1-20?
    bool all_valid = true;
    for (index = 0xc; slot16(slot_at(0x1b0, index)) != slot_end; index = index + 1) {
        if (!(slot16(slot_at(0x1b0, index)) >= 1 && slot16(slot_at(0x1b0, index)) <= 0x14)) {
            all_valid = false;
            break;
        }
    }
    if (all_valid) {
        enter(0x100c02c0, {player, p, at(slot_array)});
        copy_all_attributes_from_slots(c, player, p, at(slot_array));
        final_player_byte(player);
        return;
    }

    // 100bf508: the closest other player: the smallest distance over 250 draws of a row of the person table.
    other = 0;
    for (index = 0; s16(index) < 0xfa; index = index + 1) {
        g[15] = c.u32(count_word);
        luck = u32(below_wide(std::int32_t(g[15]), {0x2ac, 0x2b4, 0x2bc}, 0x100bf628));
        const auto record = c.u32(c.toc(-0x7be0)) + luck * 0x70;
        luck = c.u32(record + person::player);
        if (luck != 0 && s16(c.u16(luck + 4)) >= 1 && s16(c.u16(luck + 4)) <= 0xc8) {
            enter(0x100bf6e0, {player, p, luck});
            c.set(6, record);
            const auto distance = attribute_distance_all(c, player, p, luck, record);
            if (distance < std::int32_t(g[29]) || other == 0) {
                other = luck;
                g[29] = u32(distance);
            }
        }
    }
    // The 1-20 bound of a drawn slot value (100bfa44): below 1 a draw of 1-8, above 20 twenty.
    const auto store_bounded = [&] {
        if (s8(luck) < 1) set16(slot_at(0x1b0, index), s8(u32(rem_s(next(), 8) + 1)));
        else if (s8(luck) > 0x14) set16(slot_at(0x1b0, index), 0x14);
        else set16(slot_at(0x1b0, index), s8(luck));
    };
    if (other != 0) {
        enter(0x100bf730, {other, p, at(slot_array_b), 0, 0, 0});
        copy_all_attributes_to_slots(c, other, p, at(slot_array_b), 0, 0, 0);
        g[25] = u32(s16(c.u16(player + 4)));
        if (!(s16(g[25]) >= 1 && s16(g[25]) <= 200)) g[25] = ext16(rem_s(next(), 100) + 1);
        g[24] = u32(s16(c.u16(other + 4)));
        if (!(s16(g[24]) >= 1 && s16(g[24]) <= 200)) g[24] = g[25];
        for (index = u32(s8(u32(c.u8(at(0x39)))));; index = index + 1) {
            if (value_at(0x1b0) == slot_end) break;
            const auto v = value_at(0x1b0);
            if (v >= 1 && v <= 0x14) {
                // In range: a heavy slot (flag 2 with a strong first slot, flag 1 with a weak one) loses ten.
                if (heavy_slot(flag_at())) {
                    const auto lowered = v - 10 < 1 ? 1 : v - 10;
                    set16(slot_at(0x1b0, index), s8(u32(lowered)));
                }
                continue;
            }
            const auto other_value = value_at(slot_array_b);
            if (other_value > 0 && other_value < 0x15) {
                const auto delta = s8(u32(div_s(s16(g[25]) - s16(g[24]), 10)));
                if (std::int32_t(index) < s8(c.u8(at(0x38)))) {
                    luck = u32(s8(u32(other_value + delta)));
                } else {
                    luck = u32(s8(u32(other_value)));
                    if (std::int32_t(luck) <= 5) luck = luck + u32(s8(u32(rem_s(next(), 5))));
                    else if (s8(luck) >= 0xf) luck = luck - u32(s8(u32(rem_s(next(), 4))));
                }
                luck = luck + u32(s8(u32(rem_s(next(), 6) - 3)));
            } else {
                const auto sample = next();
                luck = u32(s8(u32((div_s(s16(c.u16(player + 4)), 10) + rem_s(sample, 11)) - 5)));
            }
            store_bounded();
            raise_special_slots();
        }
        enter(0x100c000c, {player, p, at(slot_array)});
        copy_all_attributes_from_slots(c, player, p, at(slot_array));
    } else {
        for (index = u32(s8(u32(c.u8(at(0x39)))));; index = index + 1) {
            if (value_at(0x1b0) == slot_end) break;
            const auto v = value_at(0x1b0);
            if (v >= 1 && v <= 0x14) continue;
            const auto sample = next();
            luck = u32(s8(u32((rem_s(sample, 7) + div_s(s16(c.u16(player + 4)), 10)) - 3)));
            store_bounded();
            if (heavy_slot(flag_at())) set16(slot_at(0x1b0, index), rem_s(next(), 5) + 1);
        }
        enter(0x100c02ac, {player, p, at(slot_array)});
        copy_all_attributes_from_slots(c, player, p, at(slot_array));
    }
    final_player_byte(player);
}

// 100bfb94 and 100bfdf4: after a slot is drawn, slots 0x34 and 0x35 are raised when below 12, slots 0x24, 0x25
// and 0x28 are lowered by a draw while the age is under 22, and a slot whose flag asks for it is redrawn.
void Gen::raise_special_slots() {
    auto& luck = g[13];
    auto& index = g[16];
    const auto slot16 = [&](std::uint32_t offset) { return s16(c.u16(at(offset))); };
    const auto here = [&] { return slot_at(0x1b0, index); };
    const auto value = [&] { return slot16(here()); };
    if (index == 0x34 || index == 0x35) {
        if (value() < 0xc) {
            g[15] = u32(0xd - value());
            luck = u32(below(std::int32_t(g[15])));
            c.put16(at(here()), u32(value() + s8(luck)));
        }
    } else if (index - 0x24 <= 1 || index == 0x28) {
        if (s8(g[23]) < 0x16 && value() > 0xf) c.put16(at(here()), u32(value() - s8(u32(rem_s(next(), 5)))));
    }
    const auto flag = s8(c.u8(at(slot_at(0x1b2, index))));
    if ((slot16(0x1b0) >= 0xf && flag == 2) || (slot16(0x1b0) < 0xf && flag == 1)) c.put16(at(here()), u32(rem_s(next(), 10) + 1));
}

// 100c02c0: a player rated 20 in byte +0xf whose byte +0x2e is under 8 gets 5-10 there.
void Gen::final_player_byte(std::uint32_t player) {
    if (c.u8(player + 0xf) == 0x14 && s8(c.u8(player + 0x2e)) < 8) c.put8(player + 0x2e, u32(rem_s(next(), 6) + 5));
}
}
