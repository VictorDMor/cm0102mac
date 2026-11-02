// 0x10664790..0x106667e0 of 0x106620b0: flag 0x80 (the story's verdict reaches the dressing room) for the visited person
// being the player himself. His morale drops by 5; a story of kind 6 wears his +0x25 byte down; his professionalism and
// age decide whether his contract loses the 0x100000 bit; and, unless the reporter lists him as a target, his morale and
// sportsmanship may shake his +0x21 and +0x44 bytes. Then the verdict against the pool's band (lean +- the double at +0x70)
// picks one of three shapes: above, below or between. Every exit ends the person.
#include "media_eval_b_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;

namespace eval_b {
namespace {
// Callees without a typed function.
constexpr std::uint32_t toward_manager = 0x100d9b80;                  // (player, the club's manager, 1 or 0)
constexpr std::uint32_t follow_up_a = 0x1067d830;                     // (reporter, player, club, kind, 2)
constexpr std::uint32_t follow_up_b = 0x1067d040;                     // (reporter, player, club, 0..3, kind)
constexpr std::uint32_t escalate = 0x10649900;                        // (reporter, player, club, kind)
constexpr std::uint32_t check_a = 0x10003dc0, check_b = 0x103f43b0;  // (table, player, 0) -> byte
constexpr std::uint32_t check_c = 0x100cab10;                         // (player, the club's +0x2c, 1) -> byte
constexpr std::uint32_t record_event = 0x10006900;                    // (table, player, 0, 0)
constexpr int table_a_slot = -0x7b24, table_b_slot = -0x7b40;

// The player record's signed bytes this stage moves (meanings not known), the contract's note byte and the
// contract flag bits it clears.
constexpr unsigned level_21 = 0x21, level_25 = 0x25, morale_44 = 0x44, level_45 = 0x45;
constexpr unsigned note_4c = 0x4c;
constexpr unsigned bit_100000 = 0x100000, bit_80000 = 0x80000, bit_80 = 0x80, bit_8 = 0x8;
constexpr unsigned frame_flag = 0x70;  // the S+0x70 byte: set at the start, cleared when the reporter's list shook the draw

// A sample divided by a constant as the compiler forms it: the multiply-high word, the quotient's sign bit, the remainder.
struct Quotient {
    std::uint32_t sample, high, sign;
    std::int32_t rest;
};
Quotient divide(std::uint32_t sample, const Divisor& by) {
    const auto high = std::uint32_t(multiply_high(by.magic, std::int32_t(sample)));
    const auto sum = by.adds ? high + sample : high;
    const auto shifted = std::uint32_t(std::int32_t(sum) >> by.shift), sign = shifted >> 31;
    return {sample, high, sign, std::int32_t(sample - (shifted + sign) * std::uint32_t(by.divisor))};
}

class SelfVerdict {
public:
    explicit SelfVerdict(Reaction& reaction) : r(reaction), c(reaction.c) {}

    void run() {
        r.morale(r.player, -5, 0x106647a0);
        if (compare(c, half_word(r.kind), 6) == 0) wear_down_level_25();
        keep_contract_bit();
        if (!r.listed_as_target()) shake_player();
        const auto band = r.compare_verdict(0x70);
        if (band.greater) {
            above();
            return;
        }
        const double low = r.fpu.fsub(r.lean(), r.dbl(0x70));  // lfd f0,0x70(r18); fsub f0,f29,f0
        c.set_f(0, low);
        r.fpu.fcmp(0, r.verdict(), low);
        if (r.verdict() < low) below();
        else between();
    }

private:
    Reaction& r;
    Cpu& c;

    // ---- views
    std::uint32_t record() const { return c.u32(r.player + 0x14); }  // his player record
    records::Person me() const { return r.who(r.player); }
    std::int32_t signed_at(std::uint32_t address) const { return byte_word(c.u8(address)); }
    std::int32_t signed_in_record(unsigned offset) const { return signed_at(record() + offset); }
    std::uint32_t entry() const { return c.u16(c.u32(r.rng.cursor)); }  // the table's halfword the sample was built from
    bool r13_set() const { return nonzero(c, byte_word(c.r(13))); }
    bool rival_kind_set() const { return nonzero(c, half_word(r.rival_kind)); }
    std::uint32_t kind_word() const { return std::uint32_t(half_word(r.kind)); }  // extsh r25
    std::int32_t roll(std::int32_t n, std::uint32_t back) { return r.draw(n, back); }
    bool rolled_nonzero(std::int32_t n, std::uint32_t back) { return nonzero(c, roll(n, back)); }

    // ---- the remainders, with the compiler's scratch registers each schedule leaves behind
    // sample mod 6 (r3 keeps the multiply-high word, as the shift is 0).
    std::int32_t beaten_by_six(std::uint32_t back) {
        r.rng.advance(back);
        const auto q = divide(r.rng.sample(), divisors::by_6);
        leave(c, {{3, q.high}, {4, q.sample}});
        return q.rest;
    }
    // The player's byte at `offset` against sample mod 30: r3 the remainder, r4 its sign, r5 the sample, r6 the table entry.
    bool byte_below_mod_30(std::uint32_t back, unsigned offset) {
        r.rng.advance(back);
        const auto q = divide(r.rng.sample(), divisors::by_30);
        leave(c, {{3, std::uint32_t(q.rest)}, {4, q.sign}, {5, q.sample}, {6, entry()}});
        return compare(c, signed_in_record(offset), q.rest) < 0;
    }
    // The player's +0x45 byte against sample mod `by` plus `extra`: r3 the byte, r4 the remainder, r5 the sample, r6 the entry.
    bool level_45_beyond(std::uint32_t back, const Divisor& by, std::int32_t extra) {
        r.rng.advance(back);
        const auto q = divide(r.rng.sample(), by);
        const auto level = signed_in_record(level_45);
        leave(c, {{3, std::uint32_t(level)}, {4, std::uint32_t(q.rest)}, {5, q.sample}, {6, entry()}});
        return compare(c, level, q.rest + extra) > 0;
    }
    // The player's tally byte grows by sample mod 15: r3 the tally row, r4 the remainder, r5 its sign, r6 the sample.
    void raise_tally(std::uint32_t back) {
        r.rng.advance(back);
        const auto q = divide(r.rng.sample(), divisors::by_15);
        const auto id = c.u32(r.player);
        leave(c, {{3, c.u32(r.tallies) + id * 0x64}, {4, std::uint32_t(q.rest)}, {5, q.sign}, {6, q.sample}});
        const auto cell = r.tally(id);
        cell = std::uint8_t(cell.get() + q.rest);
    }

    // ---- the player's bytes
    // The record's byte at `offset` is kept at 20 at most (a signed byte); r3 ends holding the record.
    void cap_at_twenty(unsigned offset) {
        const auto rec = record();
        const auto level = signed_at(rec + offset);
        c.set(3, rec);
        c.put8(rec + offset, compare(c, level, 0x14) > 0 ? 0x14u : std::uint32_t(level));
    }
    // The contract's note moves by the signed byte of `raw` (r3 holds `raw`, r4 that byte), kept within -100..100. After a
    // top-out r3 still holds `raw`; otherwise it holds the old note.
    void shift_note(std::uint32_t raw) {
        const auto step = byte_word(raw);
        c.set(3, raw);
        c.set(4, std::uint32_t(step));
        const auto at = r.standing + note_4c;
        const auto old = signed_at(at);
        if (compare(c, old + step, 0x64) > 0) {
            c.put8(at, 0x64);
            return;
        }
        c.set(3, std::uint32_t(old));
        if (compare(c, old + step, -0x64) < 0) c.put8(at, std::uint32_t(-0x64));
        else c.put8(at, c.u8(at) + std::uint32_t(step));
    }

    // ---- the stages before the verdict
    // A story of kind 6 (0x106647ac): the +0x25 byte, when a draw of it comes up 0, drops by 1..3 while above 10, or by 1 above 4.
    void wear_down_level_25() {
        c.set(3, record());
        if (rolled_nonzero(signed_in_record(level_25), 0x106647e4)) return;
        const auto rec = record();
        c.set(3, rec);
        if (compare(c, signed_at(rec + level_25), 0xa) > 0) {
            r.rng.advance(0x1066495c);
            const auto q = divide(r.rng.sample(), divisors::by_3);
            const auto drop = byte_word(std::uint32_t(q.rest + 1));
            leave(c, {{3, rec}, {4, std::uint32_t(drop)}, {5, q.high}, {6, q.sample}});
            c.put8(rec + level_25, c.u8(rec + level_25) - std::uint32_t(drop));
        } else {
            c.set(3, record());
            if (compare(c, signed_at(rec + level_25), 4) > 0) {
                c.set(4, record());
                c.set(3, c.u8(rec + level_25));
                c.put8(rec + level_25, c.u8(rec + level_25) - 1u);
            }
        }
    }

    // 0x106649d0: a draw of his professionalism that comes up 0 puts the contract's 0x100000 bit at risk: at 23 or younger it goes
    // when a new roll beats sample mod 15, at 24 or older when a roll falls under sample mod 6.
    void keep_contract_bit() {
        const auto professionalism = [&] { return me().professionalism().get(); };
        if (rolled_nonzero(professionalism(), 0x10664a04)) return;
        const auto lose_bit = [&] { r.program.call(clear_flag, 0x10664f1c, {r.standing, bit_100000, 0}); };
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

    // 0x10664f80 (the reporter does not list him): a morale draw and a sportsmanship draw, each beating sample mod 20, move his +0x21
    // and +0x44 bytes up (with a 20 cap) when they fall under sample mod 30; his tally then restarts from 0.
    void shake_player() {
        c.set(3, record());
        const auto morale = roll(signed_in_record(morale_44), 0x10664fb8);
        if (compare(c, morale, r.beaten_by(divisors::by_20, 0x10665114)) <= 0) return;
        const auto sportsmanship = roll(me().sportsmanship().get(), 0x10665188);
        if (compare(c, sportsmanship, r.beaten_by(divisors::by_20, 0x106652e4)) <= 0) return;
        if (byte_below_mod_30(0x10665344, level_21)) {
            r.rng.advance(0x106653b4);
            const auto q = divide(r.rng.sample(), divisors::by_5);
            const auto rec = record();
            leave(c, {{3, rec}, {4, std::uint32_t(q.rest)}, {5, q.sign}, {6, q.sample}});
            c.put8(rec + level_21, c.u8(rec + level_21) + std::uint32_t(q.rest));
            cap_at_twenty(level_21);
        }
        if (byte_below_mod_30(0x1066544c, morale_44)) {
            const auto rec = record();
            c.set(4, rec);
            c.set(3, c.u8(rec + morale_44));
            c.put8(rec + morale_44, c.u8(rec + morale_44) + 1u);
            cap_at_twenty(morale_44);
        }
        const auto id = c.u32(r.player);
        leave(c, {{3, c.u32(r.tallies) + id * 0x64}, {4, 0}});
        r.tally(id) = std::uint8_t{0};
    }

    // ---- the verdict above the band (0x10665504)
    void above() {
        shift_note(c.u8(r.player + 0x69) * 3u - 0x64);
        if (!rolled_nonzero(me().sportsmanship().get(), 0x10665598)) {
            r.program.call(toward_manager, 0x106656ec, {r.player, c.u32(r.club + 0x90), 1});
            r.program.call(clear_flag, 0x106656fc, {r.standing, bit_8, 0});
        } else {
            if (compare(c, std::int32_t(c.u32(r.person_standing + 0x50)), 0) == 0 && level_45_beyond(0x1066572c, divisors::by_30, 0xa) && !r13_set())
                r.program.call(follow_up_a, 0x106657a0, {r.reporter, r.player, r.club, kind_word(), 2});
            if (rival_kind_set()) r.program.call(set_flag_b, 0x106657b4, {r.standing, bit_80});
            else r.program.call(clear_flag, 0x106657cc, {r.standing, bit_80000, 0});
        }
        if (!rolled_nonzero(me().trait_69().get(), 0x10665800)) {
            // Two follow-ups are possible: the pair, after a kind 0x1e/0x20 story whose contract flag 0x40000 is set or whose
            // professionalism draw comes up 0, or the single one.
            bool pair = false;
            if (compare(c, half_word(r.kind), 0x1e) == 0 || compare(c, half_word(r.kind), 0x20) == 0)
                pair = nonzero(c, std::int32_t(c.u32(r.person_standing + 0x50) & 0x40000)) || !rolled_nonzero(me().professionalism().get(), 0x1066599c);
            if (pair) {
                const bool found = r13_set();
                r.program.call(follow_up_b, found ? 0x10665b00 : 0x10665b1c, {r.reporter, r.player, r.club, found ? 3u : 2u, r.kind});
            }
            else if (!rival_kind_set() && !r13_set()) r.program.call(follow_up_b, 0x10665b48, {r.reporter, r.player, r.club, 0, r.kind});
            raise_tally(0x10665b68);
        }
        look_into_it();
    }

    // 0x10665bc0: for a kind 1, 0x1e or 3 story that did not list the player, two checks and two draws (loyalty, professionalism)
    // all negative, then the club's +0x2c asks about him, and a contract note under -50 is recorded.
    void look_into_it() {
        if (compare(c, half_word(r.kind), 1) != 0 && compare(c, half_word(r.kind), 0x1e) != 0 && compare(c, half_word(r.kind), 3) != 0) return;
        if (r13_set()) return;
        const auto table = [&](int slot) { return c.u32(c.toc(slot)); };
        if (nonzero(c, byte_word(r.program.call(check_a, 0x10665c00, {table(table_a_slot), r.player, 0})))) return;
        if (nonzero(c, byte_word(r.program.call(check_b, 0x10665c1c, {table(table_b_slot), r.player, 0})))) return;
        if (rolled_nonzero(me().loyalty().get(), 0x10665c58)) return;
        if (rolled_nonzero(me().professionalism().get(), 0x10665dd0)) return;
        if (!nonzero(c, byte_word(r.program.call(check_c, 0x10665f24, {r.player, c.u32(r.club + 0x2c), 1})))) return;
        if (compare(c, signed_at(r.person_standing + note_4c), -0x32) >= 0) return;
        r.program.call(record_event, 0x10665f54, {table(table_a_slot), r.player, 0, 0});
    }

    // ---- the verdict below the band (0x10665f58)
    void below() {
        if (nonzero(c, signed_at(r.frame.at(frame_flag)))) {
            // The flag survives only if the reporter's list did not shake the draw: sportsmanship + loyalty is drawn against sample mod 120.
            const auto sportsmanship = me().sportsmanship().get(), loyalty = me().loyalty().get();
            c.set(3, std::uint32_t(sportsmanship));
            const auto rolled = roll(sportsmanship + loyalty, 0x10665fb4);
            if (compare(c, rolled, r.beaten_by(divisors::by_120, 0x10666110)) > 0) {
                r.program.call(escalate, 0x10666168, {r.reporter, r.player, r.club, r.kind});
                return;
            }
        }
        if (compare(c, std::int32_t(c.u32(r.person_standing + 0x50)), 0) != 0) return;
        if (!level_45_beyond(0x10666198, divisors::by_40, 0xf)) return;
        if (rival_kind_set() || r13_set()) return;
        r.program.call(follow_up_a, 0x10666210, {r.reporter, r.player, r.club, kind_word(), 2});
    }

    // ---- the verdict inside the band (0x10666214)
    void between() {
        shift_note(c.u8(r.player + 0x69) - 0x14u);
        const auto sportsmanship = me().sportsmanship().get(), loyalty = me().loyalty().get();
        c.set(3, std::uint32_t(sportsmanship));
        if (!nonzero(c, roll(sportsmanship + loyalty, 0x106662b0))) {
            r.program.call(toward_manager, 0x10666404, {r.player, c.u32(r.club + 0x90), 0});
            if (rival_kind_set()) r.program.call(set_flag_b, 0x10666418, {r.standing, bit_80});
            else r.program.call(clear_flag, 0x10666430, {r.standing, bit_80000, 0});
        }
        if (rival_kind_set()) return;
        if (rolled_nonzero(me().trait_69().get(), 0x1066646c)) return;
        if (rolled_nonzero(me().sportsmanship().get(), 0x106665e4)) return;
        const bool found = r13_set();
        r.program.call(follow_up_b, found ? 0x10666764 : 0x10666748, {r.reporter, r.player, r.club, found ? 1u : 0u, r.kind});
        raise_tally(0x10666784);
    }
};
}

void Reaction::self_80() { SelfVerdict{*this}.run(); }
}
}
