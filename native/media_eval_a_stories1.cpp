// 0x106584d0: the verdict on the contract stories (0x83, 0x82), the manager's moods that come before every
// other kind, and the kinds 0x1f, 0x3c, 0x3d and the transfer stories.
#include "media_eval_a_detail.hpp"

namespace cm::aot::media::eval_a {

// A contract in its last states (1: 2:) or a favourite on a 3: his manager rules out; otherwise the draws of
// the manager's mind decide between 1 and 3.
bool Verdict::story_83() {
    const auto state = contract_state();
    if (compare(c, state, 1) == 0 || compare(c, state, 2) == 0) return answer(1);
    if (compare(c, state, 3) == 0 && compare(c, byte_word(c.u8(contract + 0x46)), 1) > 0) return answer(1);
    if (compare(c, half_word(c.u16(c.u32(player + 0x14) + 0xa)), 0x1482) < 0 && compare(c, staff_trait(attribute::attribute_34, 0x10658648), 0xf) > 0)
        return answer(2);
    if (compare(c, state, 7) == 0 && compare(c, staff_trait(attribute::attribute_34, 0x1065867c), 0xa) > 0) return answer(2);
    const auto n = staff_trait(attribute::attribute_39, 0x106586a0);
    const auto drawn = roll(18, 17, n, 0x106586d0);
    return compare(c, drawn) != 0 ? answer(1) : answer(3);
}

bool Verdict::story_82() {
    if (asks(ask::query_d, 0x1065883c, player, manager)) {
        if (compare(c, manager_byte(personality::sportsmanship), 0xf) < 0) {
            if (compare(c, draw_manager(personality::sportsmanship, 0x10658888)) == 0) {
                const auto n = draw_trait(attribute::attribute_39, 0x106589dc, 0x10658a0c);
                if (compare_remainder(n, by::n20, 0x10658b68) < 0) return answer(2);
            }
        }
        if (compare(c, manager_byte(personality::sportsmanship), 0xe) > 0) {
            if (compare(c, draw_manager(personality::sportsmanship, 0x10658bf4), 0xa) > 0) {
                const auto n = draw_trait(attribute::attribute_39, 0x10658d48, 0x10658d78);
                if (compare_remainder(n, by::n20, 0x10658ed4) > 0) return answer(1);
            }
        }
        return answer(3);
    }
    if (asks(ask::query_c, 0x10658f30, player, manager)) {
        const auto d = draw_manager(personality::sportsmanship, 0x10658f6c);
        if (compare_remainder(d, by::n10, 0x106590c8) < 0) {
            const auto n = draw_trait(attribute::attribute_39, 0x10659118, 0x10659148);
            if (compare_remainder(n, by::n15, 0x106592a4) > 0) return answer(1);
        }
        return answer(3);
    }
    if (compare(c, manager_byte(personality::trait_69), 6) < 0) {
        if (compare_remainder(manager_byte(personality::trait_69), by::loaded20, 0x10659328) < 0) return answer(1);
    }
    const auto n = draw_trait(attribute::attribute_39, 0x10659388, 0x106593b8);
    if (compare_remainder(n, by::n20, 0x10659514) > 0) return answer(1);
    if (compare(c, manager_byte(personality::trait_68), 0xf) > 0 && compare_remainder(manager_byte(personality::trait_68), by::loaded10, 0x1065958c) > 0) return answer(2);
    return answer(3);
}

// Kind 0x1f: a team mate of the club's, else the contract's flags against a draw.
bool Verdict::story_1f() {
    if (asks(ask::team_mate, 0x10659d54, player, club)) return answer(2);
    c.set(17, c.u8(contract + 0x44));  // (kept unsigned in r17, compared as a signed byte)
    if (compare(c, byte_word(c.r(17)), remainder(by::loaded100, 0x10659d88)) <= 0) return answer(3);
    if (nonzero(c, byte_word(program.call(ask::has_flag, 0x10659dd4, {contract})))) return answer(3);
    if (nonzero(c, byte_word(c.u8(contract + 0x54)))) return answer(3);
    if (!nonzero(c, remainder(by::n3, 0x10659e08))) return answer(3);
    return answer(1);
}

// What comes before every other kind: a manager who likes the player is easy on a small loss, one who does not
// is hard on a gain.
bool Verdict::mood() {
    if (asks(ask::query_d, 0x106595fc, player, manager) && compare(c, amount(), 5) < 0) {
        if (compare(c, draw_manager(personality::sportsmanship, 0x10659640)) == 0) {
            const auto n = draw_trait(attribute::attribute_39, 0x10659794, 0x106597c4);
            if (compare_remainder(n, by::n20, 0x10659920) < 0) return answer(2);
        }
        return answer(3);
    }
    if (asks(ask::query_c, 0x1065997c, player, manager) && compare(c, amount(), -5) > 0) {
        if (compare_remainder(draw_manager(personality::sportsmanship, 0x106599c0), by::n10, 0x10659b1c) < 0) {
            const auto n = draw_trait(attribute::attribute_39, 0x10659b6c, 0x10659b9c);
            if (compare_remainder(n, by::n20, 0x10659cf8) < 0) return answer(1);
        }
        return answer(3);
    }
    return false;
}

bool Verdict::story_3c() {
    if (compare(c, kind_half(), 0x3c) != 0 && compare(c, kind_half(), 0x3d) != 0) return false;
    const auto n = draw_trait(attribute::attribute_3b, 0x10659e78, 0x10659ea8);
    if (compare_remainder(n, by::n23, 0x1065a004) > 0) {
        if (compare_remainder(draw_staff(attribute::attribute_3d, 0x1065a080), by::n10, 0x1065a1dc) < 0) return answer(2);
    }
    if (compare(c, draw_manager(personality::professionalism, 0x1065a258)) != 0) {
        if (compare_remainder(draw_manager(personality::loyalty, 0x1065a3d0), by::n40, 0x1065a52c) <= 0) {
            if (compare_remainder(draw_staff(attribute::attribute_3d, 0x1065a5a4), by::n50, 0x1065a700) <= 0) return answer(3);
        }
    }
    return answer(1);
}

// The kinds a transfer story covers (0x1069ecf0 says which).
bool Verdict::transfer_story() {
    if (!nonzero(c, byte_word(program.call(ask::transfer_story, 0x1065a75c, {reporter, std::uint32_t(kind_half())})))) return false;
    const auto sportsmanship = byte_word(c.u8(player + personality::sportsmanship));
    if (compare_remainder(draw(sportsmanship, 0x1065a798), by::n20, 0x1065a8f4) > 0) {
        const auto n = draw_trait(attribute::attribute_39, 0x1065a944, 0x1065a974);
        const auto limit = roll_range(19, std::int32_t(20u - std::uint32_t(amount())), 0x1065aae4, true, true);
        if (compare(c, n, limit) > 0) return answer(1);
        const auto m = staff_trait(attribute::attribute_39, 0x1065ac60);
        const auto drawn = roll(18, 17, m, 0x1065ac90);
        if (compare(c, drawn) == 0 && compare(c, amount(), 0xa) < 0) return answer(2);
        return answer(3);
    }
    if (compare_remainder(draw_trait(attribute::attribute_39, 0x1065adfc, 0x1065ae2c), by::n10, 0x1065af88) > 0) {
        // r18: whether the player has any morale; then (if so) a draw against the player's row sets whether it
        // is the game's turn: (morale != 0) and the second draw came up zero (the compiler's xor/and/shift).
        const auto morale = player_byte(0x44);
        c.set(3, std::uint32_t(morale));
        std::uint32_t flag = ((0u - std::uint32_t(morale)) | std::uint32_t(morale)) >> 31;
        c.set(18, flag);
        if (compare(c, std::int32_t(flag)) != 0) {
            const auto row = c.u32(c.r(21)) + c.u32(player) * 0x64;
            c.set(3, row);
            c.set(19, c.u8(row + 0xa));
            const auto first = roll(19, 18, std::int32_t(c.u8(row + 0xa)), 0x1065b028);
            const auto table = c.u32(c.r(21));
            leave(c, {{3, c.u32(player) * 0x64}, {4, table}});
            const auto extra = c.u8(table + c.u32(player) * 0x64 + 0xc) & 1;
            const auto second = roll(20, 19, 0x28 - 0x14 * std::int32_t(extra), 0x1065b1b4);
            const auto t = std::uint32_t(first) ^ std::uint32_t(second);
            const auto half = std::int32_t(t) >> 1;
            c.set(3, std::uint32_t(half));
            flag = (std::uint32_t(half) - (t & std::uint32_t(first))) >> 31;
            c.set(18, flag);
        }
        if (compare(c, std::int32_t(c.r(18))) == 0) c.set(19, 0);
        else roll(18, 19, std::int32_t(c.r(18)), 0x1065b334);
        if (compare(c, std::int32_t(c.r(19))) == 0 && compare(c, amount(), 0xa) < 0) return answer(2);
    }
    if (compare(c, draw(player_byte(0x45), 0x1065b4c0)) == 0) return answer(1);
    const auto loyalty = draw_manager(personality::loyalty, 0x1065b638);
    const auto limit = roll_range(17, std::int32_t(20u - std::uint32_t(amount())), 0x1065b7a8, true, true);
    return compare(c, loyalty, limit) > 0 ? answer(1) : answer(3);
}
}
