// 0x106620b0, flag 0x100 with the visited person being the player (0x10667e04 .. 0x1066a990): a club that has
// too many persons the query rates 2 ends the person early or clears a flag; otherwise the player's morale moves,
// his traits age (kind 6), a run of chance rolls against his personality updates his record and his tally byte,
// and the verdict (against the lean) decides which of three closing rounds may flag his contract.
#include "media_eval_b_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;

namespace eval_b {
namespace {
constexpr std::uint32_t press_note = 0x106548e0;   // (reporter, player, club, kind, 0)
constexpr std::uint32_t manager_move = 0x100d9960; // (player, manager, 0)
constexpr std::uint32_t rated_query = 0x10187680;  // (table, player) -> a small code; 2 counts

// x mod 2^bits for a signed x, by the compiler's rotate sequence.
std::int32_t power_remainder(std::uint32_t x, unsigned bits, std::uint32_t sign) {
    const auto low = (x & ((1u << bits) - 1)) << (32 - bits);
    const auto t = low - sign;
    return std::int32_t(((t << bits) | (t >> (32 - bits))) + sign);
}

// A sample divided by a constant as the compiler forms it, with the words the sequence leaves behind.
struct Division {
    std::uint32_t sample, shifted, sign, product;
    std::int32_t rest;
};
Division divide(std::uint32_t sample, const Divisor& by) {
    const auto high = std::uint32_t(multiply_high(by.magic, std::int32_t(sample))) + (by.adds ? sample : 0u);
    const auto shifted = std::uint32_t(std::int32_t(high) >> by.shift), sign = shifted >> 31;
    const auto product = (shifted + sign) * std::uint32_t(by.divisor);
    return {sample, shifted, sign, product, std::int32_t(sample - product)};
}

class SelfStage {
public:
    explicit SelfStage(Reaction& reaction) : r(reaction), c(reaction.c) {}

    void run() {
        if (compare(c, half_word(r.rival_kind), 0x13) == 0 && nonzero(c, std::int32_t(contract().flags().get() & 0x2000000))) {
            crowded_club();
            return;
        }
        r.morale(r.player, 10, 0x10668030);
        if (compare(c, half_word(r.kind), 6) == 0) traits_age();
        morale_rounds();
        verdict_stage();
    }

private:
    // ---- views
    records::Contract contract() const { return {c, r.person_standing}; }
    std::uint32_t record() const { return r.who(r.player).player().get(); }  // the player record behind +0x14
    std::int32_t trait_of(std::uint32_t who, std::uint32_t offset) const { return byte_word(c.u8(who + offset)); }
    std::int32_t sportsmanship() const { return trait_of(r.player, 0x67); }
    std::int32_t loyalty() const { return trait_of(r.player, 0x65); }
    std::int32_t morale_byte() const { return trait_of(record(), 0x44); }
    std::uint32_t manager() const { return records::Club{c, r.club}.manager().get(); }
    std::uint32_t tally_row(std::uint32_t id) const { return r.tally(id).address() - 0xa; }

    // ---- rolls
    // A draw against the sample reduced by `by` (the next advance): the draw must be greater.
    bool beats(std::uint32_t trait_offset, std::uint32_t draw_back, const Divisor& by, std::uint32_t divide_back) {
        const auto drawn = r.draw(trait_of(r.player, trait_offset), draw_back);
        const auto rest = r.beaten_by(by, divide_back);
        return compare(c, drawn, rest) > 0;
    }

    // ---- the tally byte of a person: += the sample's remainder (the low byte is kept)
    // by 15 (0x10668be8): the old byte stays in r3, the row in r4, the quotient times 15 in r5, the sign in r6, the table entry in r7.
    void add_rest_of_15(std::uint32_t id, std::uint32_t back) {
        r.rng.advance(back);
        const auto entry = c.u16(c.u32(r.rng.cursor));
        const auto d = divide(r.rng.sample(), divisors::by_15);
        const auto cell = r.tally(id);
        const std::uint32_t old = cell;
        cell = std::uint8_t(old + std::uint32_t(d.rest));
        leave(c, {{3, old}, {4, tally_row(id)}, {5, d.product}, {6, d.sign}, {7, entry}});
    }
    // by 20 (0x10669280, 0x1066a364): r3 the old byte, r4 the row, r5 the sign, r6 the sample.
    void add_rest_of_20(std::uint32_t id, std::uint32_t back) {
        r.rng.advance(back);
        const auto d = divide(r.rng.sample(), divisors::by_20);
        const auto cell = r.tally(id);
        const std::uint32_t old = cell;
        cell = std::uint8_t(old + std::uint32_t(d.rest));
        leave(c, {{3, old}, {4, tally_row(id)}, {5, d.sign}, {6, d.sample}});
    }

    // ---- the club has players the query rates 2 (the contract says the story is about a rival kind)
    void crowded_club() {
        c.set(15, 0);
        c.set(16, 0);
        while (compare(c, byte_word(c.r(16)), 0x32) < 0) {
            const auto member = c.u32(r.club + (std::uint32_t(byte_word(c.r(16))) << 2) + 0x98);
            if (compare_logical(c, member, 0) != 0) {
                const auto code = r.program.call(rated_query, 0x10667e50, {c.u32(c.toc(-0x7ba0)), r.player});
                if (compare(c, byte_word(code), 2) == 0) c.set(15, c.r(15) + 1);
            }
            c.set(16, c.r(16) + 1);
        }
        if (compare(c, byte_word(c.r(15)), 4) >= 0 && compare(c, sportsmanship(), 7) >= 0) return;
        const auto drawn = r.draw(loyalty(), 0x10667ec4);
        if (compare(c, drawn, 5) >= 0) return;
        r.program.call(clear_flag, 0x1066801c, {r.standing, 0x8000000, 0});
    }

    // ---- kind 6: the traits age (each a chance of one in the player's sportsmanship)
    void traits_age() {
        if (compare(c, trait_of(r.player, 0x69), 1) > 0) {
            const auto drawn = r.draw(sportsmanship(), 0x10668080);
            if (compare(c, drawn, 0) == 0) c.put8(r.player + 0x69, c.u8(r.player + 0x69) - 1);
        }
        rise(0x1c, 0x10668218);
        rise(0x25, 0x106683b4);
    }
    // A byte of the player record below 20 rises by one on a draw of 0 (r4 keeps the record).
    void rise(std::uint32_t offset, std::uint32_t back) {
        if (compare(c, trait_of(record(), offset), 0x14) >= 0) return;
        const auto drawn = r.draw(sportsmanship(), back);
        if (compare(c, drawn, 0) != 0) return;
        c.set(4, record());
        c.put8(record() + offset, c.u8(record() + offset) + 1);
    }

    // ---- the rounds against the player's record, then the verdict
    void morale_rounds() {
        if (nonzero(c, std::int32_t(contract().flags().get() & 0x40000))) {
            const auto drawn = r.draw(trait_of(r.player, 0x66), 0x10668548);
            r.rng.advance(0x106686a4);
            const auto sample = r.rng.sample();
            const auto bound = power_remainder(sample, 3, sample >> 31);
            if (compare(c, drawn, bound) > 0) r.program.call(set_flag, 0x106686e8, {r.person_standing, 0x40000});
        }
        if (!first_branch()) second_branch();
    }

    // Morale rolled 0 and sportsmanship beats a remainder of ten: morale may fall by one (on a roll other than 0)
    // and the tally byte rises by a remainder of fifteen. False: the other branch.
    bool first_branch() {
        if (compare(c, r.draw(morale_byte(), 0x10668720), 0) != 0) return false;
        const auto drawn = r.draw(sportsmanship(), 0x10668898);
        const auto rest = r.beaten_by(divisors::by_10, 0x106689f4);
        if (compare(c, drawn, rest) >= 0) return false;
        if (compare(c, r.draw(morale_byte(), 0x10668a6c), 0) != 0) {
            c.set(4, record());
            c.put8(record() + 0x44, c.u8(record() + 0x44) - 1);
        }
        add_rest_of_15(c.u32(r.player), 0x10668be0);
        return true;
    }

    // Two draws against remainders of 25 and 40 clear the tally byte, and loyalty may rise (held at 20).
    void second_branch() {
        const auto first = r.draw(sportsmanship(), 0x10668c70);
        const auto first_bound = r.beaten_by(divisors::by_25, 0x10668dcc);
        if (compare(c, first, first_bound) <= 0) return;
        const auto second = r.draw(morale_byte(), 0x10668e44);
        const auto second_bound = r.beaten_by(divisors::by_40, 0x10668fa0);
        if (compare(c, second, second_bound) <= 0) return;
        const auto cell = r.tally(c.u32(r.player));
        if (compare_logical(c, cell, 0) != 0) {
            c.set(4, 0);
            cell = std::uint8_t(0);
        }
        r.rng.advance(0x10669034);
        const auto d = divide(r.rng.sample(), divisors::by_30);
        leave(c, {{3, std::uint32_t(d.rest)}, {4, d.sign}, {5, d.sample}});
        if (compare(c, loyalty(), d.rest) >= 0) return;
        c.put8(r.player + 0x65, c.u8(r.player + 0x65) + 1);
        // held at 20 (quirk: the stored byte is reloaded when it is not above)
        const auto raised = compare(c, loyalty(), 0x14) <= 0 ? loyalty() : 0x14;
        c.put8(r.player + 0x65, std::uint32_t(raised));
    }

    // ---- the verdict against the lean: above, between and below
    void verdict_stage() {
        if (r.compare_verdict(0x70).greater) {
            high_verdict();
            return;
        }
        const double threshold = r.fpu.fsub(r.lean(), r.dbl(0x70));
        c.set_f(0, threshold);
        r.fpu.fcmp(0, r.verdict(), threshold);
        if (r.verdict() < threshold) middle_verdict();
        else low_verdict();
    }

    // The visited person's tally byte below 50 rises by a remainder of twenty on a roll of 0 (n = his sportsmanship + offset).
    void rise_tally(std::int32_t offset, std::uint32_t draw_back, std::uint32_t bump_back) {
        if (compare_logical(c, r.tally(c.u32(r.person)).get(), 0x32) >= 0) return;
        const auto n = trait_of(r.person, 0x67) + offset;
        if (compare(c, r.draw(n, draw_back), 0) == 0) add_rest_of_20(c.u32(r.person), bump_back);
    }
    void press_note_call(std::uint32_t back) { r.program.call(press_note, back, {r.reporter, r.player, r.club, r.kind, 0}); }
    // A rival kind 0xa with the contract's flag 0x4000000 set: set it again.
    void flag_rival_kind(std::uint32_t back) {
        if (compare(c, half_word(r.rival_kind), 0xa) == 0 && nonzero(c, std::int32_t(contract().flags().get() & 0x4000000)))
            r.program.call(set_flag, back, {r.person_standing, 0x4000000});
    }
    // The manager is told and the player's contract gets the flags 8 and 0x2000.
    void conclude(std::uint32_t first_back, std::uint32_t second_back, std::uint32_t third_back) {
        r.program.call(manager_move, first_back, {r.player, manager(), 0});
        r.program.call(set_flag, second_back, {r.standing, 0x8});
        r.program.call(set_flag, third_back, {r.standing, 0x2000});
    }

    void high_verdict() {
        rise_tally(0xa, 0x10669114, 0x10669278);
        if (beats(0x65, 0x10669304, divisors::by_40, 0x10669460)) {
            press_note_call(0x106694b8);
            flag_rival_kind(0x106694e0);
        }
        if (!beats(0x63, 0x10669514, divisors::by_60, 0x10669670)) return;
        if (!beats(0x65, 0x106696e8, divisors::by_60, 0x10669844)) return;
        conclude(0x10669898, 0x106698a4, 0x106698b0);
    }

    void middle_verdict() {
        if (compare_logical(c, r.tally(c.u32(r.person)).get(), 0x32) < 0) {
            if (compare(c, r.draw(trait_of(r.person, 0x67), 0x10669914), 0) == 0) {
                const auto n = 0x1e - trait_of(r.person, 0x67);  // subfic
                const auto drawn = r.draw(n, 0x10669a94);
                const auto cell = r.tally(c.u32(r.person));
                const std::uint32_t old = cell;
                cell = std::uint8_t(old + (std::uint32_t(drawn) & 0xff));
                leave(c, {{3, old}, {4, tally_row(c.u32(r.person))}});
            }
        }
        if (beats(0x65, 0x10669c24, divisors::by_10, 0x10669d80)) press_note_call(0x10669dd8);
        if (!beats(0x63, 0x10669e0c, divisors::by_10, 0x10669f68)) return;
        if (!beats(0x65, 0x10669fdc, divisors::by_10, 0x1066a138)) return;
        conclude(0x1066a188, 0x1066a194, 0x1066a1a0);
    }

    void low_verdict() {
        rise_tally(5, 0x1066a1f8, 0x1066a35c);
        if (beats(0x65, 0x1066a3e8, divisors::by_20, 0x1066a544)) {
            press_note_call(0x1066a59c);
            flag_rival_kind(0x1066a5c4);
        }
        if (!beats(0x63, 0x1066a5f8, divisors::by_40, 0x1066a754)) return;
        if (!beats(0x65, 0x1066a7c8, divisors::by_40, 0x1066a924)) return;
        conclude(0x1066a974, 0x1066a980, 0x1066a98c);
    }

    Reaction& r;
    Cpu& c;
};
}  // namespace

void Reaction::self_100() { SelfStage{*this}.run(); }
}
}
