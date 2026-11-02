// 0x10664790..0x106667e0 of 0x106620b0: flag 0x80 (the story's verdict reaches the dressing room) for the visited
// person being the player himself. His morale drops by 5; a story of kind 6 wears his dirtiness down; his
// professionalism and age decide whether his contract loses the 0x100000 bit; and, unless the reporter lists him as a
// target, his morale and sportsmanship may shake his consistency and morale. Then the verdict against the pool's band
// (lean +- the double at +0x70) picks one of three shapes: above, below or between. Every exit ends the person.
#include "media_eval_b_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;

namespace eval_b {
namespace {
// Callees without a typed function.
constexpr std::uint32_t check_a = 0x10003dc0, check_b = 0x103f43b0;  // (table, player, 0) -> byte
constexpr std::uint32_t check_c = 0x100cab10;                         // (player, the club's nation, 1) -> byte
constexpr std::uint32_t record_event = 0x10006900;                    // (table, player, 0, 0)
constexpr int table_a_slot = -0x7b24, table_b_slot = -0x7b40;
// The contract flag bits it clears or sets.
constexpr unsigned bit_100000 = 0x100000, bit_80000 = 0x80000, bit_80 = 0x80;

class SelfVerdict {
public:
    explicit SelfVerdict(Reaction& reaction) : r(reaction), c(reaction.c) {}

    void run() {
        r.morale(r.player, -5, 0x106647a0);
        if (compare(c, half_word(r.kind), 6) == 0) wear_down_dirtiness();
        keep_contract_bit();
        if (!r.listed_as_target()) shake_player();
        switch (r.band(0x70)) {
        case Band::above: above(); break;
        case Band::between: between(); break;
        default: below(); break;  // Band::below
        }
    }

private:
    Reaction& r;
    Cpu& c;

    // ---- views
    records::Person me() const { return r.who(r.player); }
    records::Player record() const { return me().player_record(); }  // his player record
    // The table's halfword the sample was built from.
    std::uint32_t entry() const { return c.u16(c.u32(r.rng.cursor)); }
    // Whether the verdict (not the caller) asked for this stage: the call came with neither flag 0x80 nor 0x100.
    bool asked_for_neither() const { return nonzero(c, byte_word(r.asked_for_neither)); }
    bool rival_kind_set() const { return nonzero(c, half_word(r.rival_kind)); }
    std::uint32_t kind_word() const { return std::uint32_t(half_word(r.kind)); }  // extsh r25
    std::int32_t roll(std::int32_t n, std::uint32_t back) { return r.draw(n, back); }
    bool rolled_nonzero(std::int32_t n, std::uint32_t back) { return nonzero(c, roll(n, back)); }

    // ---- the remainders, with the compiler's scratch registers each schedule leaves behind
    // sample mod 6 (r3 keeps the multiply-high word, as the shift is 0).
    std::int32_t beaten_by_six(std::uint32_t back) {
        r.rng.advance(back);
        const auto reduced = reduced_by_signed(r.rng.sample(), divisors::by_6);
        leave(c, {{3, std::uint32_t(reduced.high)}, {4, reduced.sample}});
        return reduced.rest;
    }
    // The player's byte against sample mod 30: r3 the remainder, r4 its sign, r5 the sample, r6 the table entry.
    bool byte_below_mod_30(std::uint32_t back, const Field<std::int8_t>& byte) {
        r.rng.advance(back);
        const auto reduced = reduced_by_signed(r.rng.sample(), divisors::by_30);
        leave(c, {{3, std::uint32_t(reduced.rest)}, {4, reduced.sign}, {5, reduced.sample}, {6, entry()}});
        return compare(c, byte.get(), reduced.rest) < 0;
    }
    // The player's versatility against sample mod `by` plus `extra`: r3 the byte, r4 the remainder, r5 the sample,
    // r6 the entry.
    bool versatility_beyond(std::uint32_t back, const Reciprocal& by, std::int32_t extra) {
        r.rng.advance(back);
        const auto reduced = reduced_by_signed(r.rng.sample(), by);
        const auto versatility = record().versatility().get();
        leave(c, {{3, std::uint32_t(versatility)},
                  {4, std::uint32_t(reduced.rest)},
                  {5, reduced.sample},
                  {6, entry()}});
        return compare(c, versatility, reduced.rest + extra) > 0;
    }
    // The player's tally byte grows by sample mod 15: r3 the tally row, r4 the remainder, r5 its sign, r6 the sample.
    void raise_tally(std::uint32_t back) {
        r.rng.advance(back);
        const auto reduced = reduced_by_signed(r.rng.sample(), divisors::by_15);
        const auto row = r.tally_row(r.player);
        leave(c, {{3, row.at}, {4, std::uint32_t(reduced.rest)}, {5, reduced.sign}, {6, reduced.sample}});
        row.tally() = std::uint8_t(row.tally().get() + reduced.rest);
    }

    // ---- the player's bytes
    // The record's byte is kept at 20 at most; r3 ends holding the record.
    void cap_at_twenty(const Field<std::int8_t>& byte) {
        const auto level = byte.get();
        c.set(3, record().at);
        byte = compare(c, level, 0x14) > 0 ? std::int8_t(0x14) : level;
    }

    // ---- the stages before the verdict
    // A story of kind 6 (0x106647ac): the dirtiness, when a draw of it comes up 0, drops by 1..3 while above 10, or by
    // 1 above 4.
    void wear_down_dirtiness() {
        c.set(3, record().at);
        const auto dirtiness = record().dirtiness();
        if (rolled_nonzero(dirtiness.get(), 0x106647e4)) return;
        const auto rec = record().at;
        c.set(3, rec);
        if (compare(c, dirtiness.get(), 0xa) > 0) {
            r.rng.advance(0x1066495c);
            const auto reduced = reduced_by_signed(r.rng.sample(), divisors::by_3);
            const auto drop = byte_word(std::uint32_t(reduced.rest + 1));
            leave(c, {{3, rec}, {4, std::uint32_t(drop)}, {5, std::uint32_t(reduced.high)}, {6, reduced.sample}});
            dirtiness = std::int8_t(std::uint32_t(dirtiness.get()) - std::uint32_t(drop));
        } else {
            c.set(3, record().at);
            if (compare(c, dirtiness.get(), 4) > 0) {
                c.set(4, record().at);
                c.set(3, std::uint8_t(dirtiness.get()));
                dirtiness = std::int8_t(std::uint32_t(dirtiness.get()) - 1u);
            }
        }
    }

    // 0x106649d0: a draw of his professionalism that comes up 0 puts the contract's 0x100000 bit at risk: at 23 or
    // younger it goes when a new roll beats sample mod 15, at 24 or older when a roll falls under sample mod 6.
    void keep_contract_bit() {
        const auto professionalism = [&] { return me().professionalism().get(); };
        if (rolled_nonzero(professionalism(), 0x10664a04)) return;
        const auto lose_bit = [&] { r.program.call(clear_flag, 0x10664f1c, {r.contract, bit_100000, 0}); };
        if (compare(c, me().age(), 0x18) < 0) {
            const auto rolled = roll(professionalism(), 0x10664b8c);
            if (compare(c, rolled, r.beaten_by(divisors::by_15, 0x10664ce8)) < 0) {
                lose_bit();
                return;
            }
        }
        if (compare(c, me().age(), 0x18) < 0) return;
        const auto rolled = roll(professionalism(), 0x10664d70);
        if (compare(c, rolled, beaten_by_six(0x10664ecc)) < 0) lose_bit();
    }

    // 0x10664f80 (the reporter does not list him): a morale draw and a sportsmanship draw, each beating sample mod 20,
    // move his consistency and morale up (with a 20 cap) when they fall under sample mod 30; his tally then restarts
    // from 0.
    void shake_player() {
        c.set(3, record().at);
        const auto morale = roll(record().morale().get(), 0x10664fb8);
        if (compare(c, morale, r.beaten_by(divisors::by_20, 0x10665114)) <= 0) return;
        const auto sportsmanship = roll(me().sportsmanship().get(), 0x10665188);
        if (compare(c, sportsmanship, r.beaten_by(divisors::by_20, 0x106652e4)) <= 0) return;
        if (byte_below_mod_30(0x10665344, record().consistency())) {
            r.rng.advance(0x106653b4);
            const auto reduced = reduced_by_signed(r.rng.sample(), divisors::by_5);
            const auto consistency = record().consistency();
            leave(c, {{3, record().at}, {4, std::uint32_t(reduced.rest)}, {5, reduced.sign}, {6, reduced.sample}});
            consistency = std::int8_t(std::uint32_t(consistency.get()) + std::uint32_t(reduced.rest));
            cap_at_twenty(consistency);
        }
        if (byte_below_mod_30(0x1066544c, record().morale())) {
            const auto morale_byte = record().morale();
            c.set(4, record().at);
            c.set(3, std::uint8_t(morale_byte.get()));
            morale_byte = std::int8_t(std::uint32_t(morale_byte.get()) + 1u);
            cap_at_twenty(morale_byte);
        }
        const auto row = r.tally_row(r.player);
        leave(c, {{3, row.at}, {4, 0}});
        row.tally() = std::uint8_t{0};
    }

    // ---- the verdict above the band (0x10665504)
    void above() {
        r.shift_contract_morale(r.player_contract(), std::uint32_t(std::uint8_t(me().temperament().get())) * 3u - 0x64);
        if (!rolled_nonzero(me().sportsmanship().get(), 0x10665598)) {
            r.turn_on_manager(0x106656ec, 0x106656fc, 1);
        } else {
            if (compare(c, std::int32_t(r.person_contract_record().flags().get()), 0) == 0 &&
                versatility_beyond(0x1066572c, divisors::by_30, 0xa) && !asked_for_neither())
                r.post_follow_up(kind_word(), 0x106657a0);
            if (rival_kind_set()) r.program.call(set_flag_b, 0x106657b4, {r.contract, bit_80});
            else r.program.call(clear_flag, 0x106657cc, {r.contract, bit_80000, 0});
        }
        if (!rolled_nonzero(me().temperament().get(), 0x10665800)) {
            // Two follow-ups are possible: the pair, after a kind 0x1e/0x20 story whose contract flag 0x40000 is set or
            // whose professionalism draw comes up 0, or the single one.
            bool pair = false;
            if (compare(c, half_word(r.kind), 0x1e) == 0 || compare(c, half_word(r.kind), 0x20) == 0)
                pair = nonzero(c, std::int32_t(r.person_contract_record().flags().get() & 0x40000)) ||
                       !rolled_nonzero(me().professionalism().get(), 0x1066599c);
            if (pair) {
                const bool from_verdict = asked_for_neither();
                r.post_reaction_item(from_verdict ? 3u : 2u, from_verdict ? 0x10665b00 : 0x10665b1c);
            } else if (!rival_kind_set() && !asked_for_neither()) {
                r.post_reaction_item(0, 0x10665b48);
            }
            raise_tally(0x10665b68);
        }
        look_into_it();
    }

    // 0x10665bc0: for a kind 1, 0x1e or 3 story that did not list the player, two checks and two draws (loyalty,
    // professionalism) all negative, then the club's nation asks about him, and a contract morale under -50 is
    // recorded.
    void look_into_it() {
        const auto story = half_word(r.kind);
        if (compare(c, story, 1) != 0 && compare(c, story, 0x1e) != 0 && compare(c, story, 3) != 0) return;
        if (asked_for_neither()) return;
        const auto table = [&](int slot) { return c.u32(c.toc(slot)); };
        if (nonzero(c, byte_word(r.program.call(check_a, 0x10665c00, {table(table_a_slot), r.player, 0})))) return;
        if (nonzero(c, byte_word(r.program.call(check_b, 0x10665c1c, {table(table_b_slot), r.player, 0})))) return;
        if (rolled_nonzero(me().loyalty().get(), 0x10665c58)) return;
        if (rolled_nonzero(me().professionalism().get(), 0x10665dd0)) return;
        const auto nation = r.club_record().nation();
        if (!nonzero(c, byte_word(r.program.call(check_c, 0x10665f24, {r.player, nation, 1})))) return;
        if (compare(c, r.person_contract_record().morale().get(), -0x32) >= 0) return;
        r.program.call(record_event, 0x10665f54, {table(table_a_slot), r.player, 0, 0});
    }

    // ---- the verdict below the band (0x10665f58)
    void below() {
        if (nonzero(c, byte_word(c.u8(r.frame.at(frame_flag))))) {
            // The flag survives only if the reporter's list did not shake the draw: sportsmanship + loyalty is drawn
            // against sample mod 120.
            const auto sportsmanship = me().sportsmanship().get(), loyalty = me().loyalty().get();
            c.set(3, std::uint32_t(sportsmanship));
            const auto rolled = roll(sportsmanship + loyalty, 0x10665fb4);
            if (compare(c, rolled, r.beaten_by(divisors::by_120, 0x10666110)) > 0) {
                r.post_escalation(0x10666168);
                return;
            }
        }
        if (compare(c, std::int32_t(r.person_contract_record().flags().get()), 0) != 0) return;
        if (!versatility_beyond(0x10666198, divisors::by_40, 0xf)) return;
        if (rival_kind_set() || asked_for_neither()) return;
        r.post_follow_up(kind_word(), 0x10666210);
    }

    // ---- the verdict inside the band (0x10666214)
    void between() {
        r.shift_contract_morale(r.player_contract(), std::uint32_t(std::uint8_t(me().temperament().get())) - 0x14u);
        const auto sportsmanship = me().sportsmanship().get(), loyalty = me().loyalty().get();
        c.set(3, std::uint32_t(sportsmanship));
        if (!nonzero(c, roll(sportsmanship + loyalty, 0x106662b0))) {
            r.fall_out_with_manager(r.player, 0, 0x10666404);
            if (rival_kind_set()) r.program.call(set_flag_b, 0x10666418, {r.contract, bit_80});
            else r.program.call(clear_flag, 0x10666430, {r.contract, bit_80000, 0});
        }
        if (rival_kind_set()) return;
        if (rolled_nonzero(me().temperament().get(), 0x1066646c)) return;
        if (rolled_nonzero(me().sportsmanship().get(), 0x106665e4)) return;
        const bool from_verdict = asked_for_neither();
        r.post_reaction_item(from_verdict ? 1u : 0u, from_verdict ? 0x10666764 : 0x10666748);
        raise_tally(0x10666784);
    }
};
}

void Reaction::self_80() { SelfVerdict{*this}.run(); }
}
}
