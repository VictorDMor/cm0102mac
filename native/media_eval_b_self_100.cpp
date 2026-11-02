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
constexpr std::uint32_t rated_query = 0x10187680;  // (table, player) -> a small code; 2 counts
constexpr int rated_table_slot = -0x7ba0;

class SelfStage {
public:
    explicit SelfStage(Reaction& reaction) : r(reaction), c(reaction.c) {}

    void run() {
        const bool flagged = nonzero(c, std::int32_t(contract().flags().get() & 0x2000000));
        if (compare(c, half_word(r.rival_kind), 0x13) == 0 && flagged) {
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
    records::Contract contract() const { return r.person_contract_record(); }
    records::Person me() const { return r.who(r.player); }
    records::Player record() const { return me().player_record(); }  // the player record behind +0x14
    std::int32_t sportsmanship() const { return me().sportsmanship().get(); }
    std::int32_t loyalty() const { return me().loyalty().get(); }
    std::int32_t morale_byte() const { return record().morale().get(); }
    std::uint32_t manager() const { return r.club_record().manager(); }

    // ---- rolls
    // A draw against the sample reduced by `by` (the next advance): the draw must be greater.
    bool beats(TraitOf trait, std::uint32_t draw_back, const Reciprocal& by, std::uint32_t divide_back) {
        const auto rolled = r.roll_trait(r.player, trait, draw_back);
        const auto rest = r.beaten_by(by, divide_back);
        return compare(c, rolled, rest) > 0;
    }

    // ---- the tally byte of a person: += the sample's remainder (the low byte is kept)
    // by 15 (0x10668be8): the old byte stays in r3, the row in r4, the quotient times 15 in r5, the sign in r6,
    // the table entry in r7.
    void add_rest_of_15(std::uint32_t person, std::uint32_t back) {
        r.rng.advance(back);
        const auto entry = c.u16(c.u32(r.rng.cursor));
        const auto reduced = reduced_by_signed(r.rng.sample(), divisors::by_15);
        const auto row = r.tally_row(person);
        const std::uint32_t old = row.tally();
        row.tally() = std::uint8_t(old + std::uint32_t(reduced.rest));
        const auto product = reduced.sample - std::uint32_t(reduced.rest);
        leave(c, {{3, old}, {4, row.at}, {5, product}, {6, reduced.sign}, {7, entry}});
    }
    // by 20 (0x10669280, 0x1066a364): r3 the old byte, r4 the row, r5 the sign, r6 the sample.
    void add_rest_of_20(std::uint32_t person, std::uint32_t back) {
        r.rng.advance(back);
        const auto reduced = reduced_by_signed(r.rng.sample(), divisors::by_20);
        const auto row = r.tally_row(person);
        const std::uint32_t old = row.tally();
        row.tally() = std::uint8_t(old + std::uint32_t(reduced.rest));
        leave(c, {{3, old}, {4, row.at}, {5, reduced.sign}, {6, reduced.sample}});
    }

    // ---- the club has players the query rates 2 (the contract says the story is about a rival kind)
    void crowded_club() {
        Kept<15> crowded{c, 0};
        Kept<16> slot{c, 0};
        while (compare(c, byte_word(slot), records::Club::squad_slots) < 0) {
            const auto member = r.club_record().squad(std::uint32_t(byte_word(slot)));
            if (compare_logical(c, member, 0) != 0) {
                // (quirk: the query is about the player on every occupied slot, not the member: the count is 0 or all)
                const auto code = r.program.call(rated_query, 0x10667e50, {c.u32(c.toc(rated_table_slot)), r.player});
                if (compare(c, byte_word(code), 2) == 0) crowded = crowded + 1;
            }
            slot = slot + 1;
        }
        if (compare(c, byte_word(crowded), 4) >= 0 && compare(c, sportsmanship(), 7) >= 0) return;
        const auto rolled = r.draw(loyalty(), 0x10667ec4);
        if (compare(c, rolled, 5) >= 0) return;
        r.program.call(clear_flag, 0x1066801c, {r.contract, 0x8000000, 0});
    }

    // ---- kind 6: the traits age (each a chance of one in the player's sportsmanship)
    void traits_age() {
        const auto temperament = me().temperament();
        if (compare(c, temperament.get(), 1) > 0) {
            const auto rolled = r.draw(sportsmanship(), 0x10668080);
            if (compare(c, rolled, 0) == 0) temperament = std::int8_t(std::uint32_t(temperament.get()) - 1);
        }
        rise(record().aggression(), 0x10668218);
        rise(record().dirtiness(), 0x106683b4);
    }
    // A byte of the player record below 20 rises by one on a draw of 0 (r4 keeps the record).
    void rise(const Field<std::int8_t>& byte, std::uint32_t back) {
        if (compare(c, byte.get(), 0x14) >= 0) return;
        const auto rolled = r.draw(sportsmanship(), back);
        if (compare(c, rolled, 0) != 0) return;
        c.set(4, record().at);
        byte = std::int8_t(std::uint32_t(byte.get()) + 1);
    }

    // ---- the rounds against the player's record, then the verdict
    void morale_rounds() {
        if (nonzero(c, std::int32_t(contract().flags().get() & 0x40000))) {
            const auto rolled = r.draw(me().professionalism().get(), 0x10668548);
            r.rng.advance(0x106686a4);
            const auto sample = r.rng.sample();
            const auto bound = power_remainder(sample, 3, sample >> 31);
            if (compare(c, rolled, bound) > 0) r.program.call(set_flag, 0x106686e8, {r.person_contract, 0x40000});
        }
        if (!first_branch()) second_branch();
    }

    // Morale rolled 0 and sportsmanship beats a remainder of ten: morale may fall by one (on a roll other than 0)
    // and the tally byte rises by a remainder of fifteen. False: the other branch.
    bool first_branch() {
        if (compare(c, r.draw(morale_byte(), 0x10668720), 0) != 0) return false;
        const auto rolled = r.draw(sportsmanship(), 0x10668898);
        const auto rest = r.beaten_by(divisors::by_10, 0x106689f4);
        if (compare(c, rolled, rest) >= 0) return false;
        if (compare(c, r.draw(morale_byte(), 0x10668a6c), 0) != 0) {
            const auto morale = record().morale();
            c.set(4, record().at);
            morale = std::int8_t(std::uint32_t(morale.get()) - 1);
        }
        add_rest_of_15(r.player, 0x10668be0);
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
        const auto cell = r.tally(r.player);
        if (compare_logical(c, cell, 0) != 0) {
            c.set(4, 0);
            cell = std::uint8_t(0);
        }
        r.rng.advance(0x10669034);
        const auto reduced = reduced_by_signed(r.rng.sample(), divisors::by_30);
        leave(c, {{3, std::uint32_t(reduced.rest)}, {4, reduced.sign}, {5, reduced.sample}});
        if (compare(c, loyalty(), reduced.rest) >= 0) return;
        const auto own_loyalty = me().loyalty();
        own_loyalty = std::int8_t(std::uint32_t(own_loyalty.get()) + 1);
        // held at 20 (quirk: the stored byte is reloaded when it is not above)
        const auto raised = compare(c, loyalty(), 0x14) <= 0 ? loyalty() : 0x14;
        own_loyalty = std::int8_t(raised);
    }

    // ---- the verdict against the lean: above, between and below the band
    void verdict_stage() {
        switch (r.band(0x70)) {
        case Band::above: high_verdict(); break;
        case Band::below: low_verdict(); break;
        case Band::between: middle_verdict(); break;
        }
    }

    // The visited person's tally byte below 50 rises by a remainder of twenty on a roll of 0 (n = his sportsmanship +
    // offset).
    void rise_tally(std::int32_t offset, std::uint32_t draw_back, std::uint32_t bump_back) {
        if (compare_logical(c, r.tally(r.person).get(), 0x32) >= 0) return;
        const auto n = r.who(r.person).sportsmanship().get() + offset;
        if (compare(c, r.draw(n, draw_back), 0) == 0) add_rest_of_20(r.person, bump_back);
    }
    // After the note, the original compares the rival kind with 0xa and, when equal, tests the contract's flag
    // 0x4000000 and sets it again (calls to 0x1037f7d0 returning to 0x106694e0 and 0x1066a5c4). It never gets that far:
    // r28 holds 0 or a kind whose low halfword is 0x13 (its only writes are at 0x10662110 and 0x10662200), so the
    // compare is never equal. Only the compare itself stays, for the condition register.
    void compare_rival_kind_with_ten() { compare(c, half_word(r.rival_kind), 0xa); }
    // The manager is told and the player's contract gets the flags 8 and 0x2000.
    void conclude(std::uint32_t first_back, std::uint32_t second_back, std::uint32_t third_back) {
        r.program.call(relations::warm_to, first_back, {r.player, manager(), 0});
        r.program.call(set_flag, second_back, {r.contract, 0x8});
        r.program.call(set_flag, third_back, {r.contract, 0x2000});
    }

    void high_verdict() {
        rise_tally(0xa, 0x10669114, 0x10669278);
        if (beats(&records::Person::loyalty, 0x10669304, divisors::by_40, 0x10669460)) {
            r.post_note(0x106694b8);
            compare_rival_kind_with_ten();
        }
        if (!beats(&records::Person::ambition, 0x10669514, divisors::by_60, 0x10669670)) return;
        if (!beats(&records::Person::loyalty, 0x106696e8, divisors::by_60, 0x10669844)) return;
        conclude(0x10669898, 0x106698a4, 0x106698b0);
    }

    // Below the band.
    void low_verdict() {
        if (compare_logical(c, r.tally(r.person).get(), 0x32) < 0) {
            if (compare(c, r.draw(r.who(r.person).sportsmanship().get(), 0x10669914), 0) == 0) {
                const auto n = 0x1e - r.who(r.person).sportsmanship().get();  // subfic
                const auto rolled = r.draw(n, 0x10669a94);
                const auto cell = r.tally(r.person);
                const std::uint32_t old = cell;
                cell = std::uint8_t(old + (std::uint32_t(rolled) & 0xff));
                leave(c, {{3, old}, {4, r.tally_row(r.person).at}});
            }
        }
        if (beats(&records::Person::loyalty, 0x10669c24, divisors::by_10, 0x10669d80)) r.post_note(0x10669dd8);
        if (!beats(&records::Person::ambition, 0x10669e0c, divisors::by_10, 0x10669f68)) return;
        if (!beats(&records::Person::loyalty, 0x10669fdc, divisors::by_10, 0x1066a138)) return;
        conclude(0x1066a188, 0x1066a194, 0x1066a1a0);
    }

    // Inside the band.
    void middle_verdict() {
        rise_tally(5, 0x1066a1f8, 0x1066a35c);
        if (beats(&records::Person::loyalty, 0x1066a3e8, divisors::by_20, 0x1066a544)) {
            r.post_note(0x1066a59c);
            compare_rival_kind_with_ten();
        }
        if (!beats(&records::Person::ambition, 0x1066a5f8, divisors::by_40, 0x1066a754)) return;
        if (!beats(&records::Person::loyalty, 0x1066a7c8, divisors::by_40, 0x1066a924)) return;
        conclude(0x1066a974, 0x1066a980, 0x1066a98c);
    }

    Reaction& r;
    Cpu& c;
};
}  // namespace

void Reaction::self_100() { SelfStage{*this}.run(); }
}
}
