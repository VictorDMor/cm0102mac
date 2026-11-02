#pragma once
// 0x106620b0: the press's reaction to a story about a player and his club. The routine is one frame
// (0xf0) with 19 saved registers; the context below carries what it keeps in them. Its parts, by
// address, live in their own files:
//   media_eval_b_start.cpp    0x106620b0..0x10662d30  prologue, the third column, the verdict, the 50 persons, the gate
//   media_eval_b_stages.cpp   0x10662d30..0x10664794  the morale nudges by flag bit, then the 0x40 stage
//   media_eval_b_self_80.cpp  0x10664794..0x106667e0  flag 0x80, the person is the player
//   media_eval_b_other_80.cpp 0x106667e0..0x10667df4  flag 0x80, another person
//   media_eval_b_self_100.cpp 0x10667e04..0x1066a990  flag 0x100, the player
//   media_eval_b_other_100.cpp 0x1066a990..0x1066b5bc flag 0x100, another person
#include "guest_fpu.hpp"
#include "media_eval_b.hpp"
#include "media_leaves.hpp"
#include "media_records.hpp"
#include "media_values.hpp"
#include "media_values_detail.hpp"

namespace cm::aot::media::eval_b {
using namespace guest;
using namespace detail;

constexpr std::uint32_t entry = 0x106620b0;
// The callees (addresses of the original, for those without a typed function). The questions about likes and
// dislikes and the fall-out are in detail::relations.
constexpr std::uint32_t move_morale = 0x100d3570;  // (person, delta, 1)
constexpr std::uint32_t contract_of = 0x10082f90;  // (table, person, club) -> the person's contract with the club
// (contract, bits[, 0])
constexpr std::uint32_t set_flag = 0x1037f7d0, clear_flag = 0x1037fa80, set_flag_b = 0x1037f8a0;
constexpr std::uint32_t absolute_value = 0x10775540;  // abs: srawi, xor, subf
// The frame byte at sp+0x70: set at the start, cleared when the reporter's list shook the draw.
constexpr std::uint32_t frame_flag = 0x70;

// The outcome of comparing the verdict with a threshold (fcmpo cr0): `ble` is taken unless greater,
// `bge` unless less (unordered counts as neither).
struct Comparison {
    bool less, greater;
};
// Where the verdict stands against the band around the lean: above its top, below its bottom, or between.
enum class Band { above, between, below };

// x mod 2^bits for a signed x, by the compiler's rotate sequence (the sign of x rides along).
inline std::int32_t power_remainder(std::uint32_t x, unsigned bits, std::uint32_t sign) {
    const auto low = (x & ((1u << bits) - 1)) << (32 - bits);
    const auto t = low - sign;
    return std::int32_t(((t << bits) | (t >> (32 - bits))) + sign);
}

// A personality byte of a person, for the draws that roll over one.
using TraitOf = Field<std::int8_t> (records::Person::*)() const;

struct Reaction {
    Reaction(Cpu& cpu, std::uint32_t reporter_, std::uint32_t club_, std::uint32_t kind_, std::uint32_t flags_,
             double factor_, std::uint32_t side_, std::uint32_t amount_, std::uint32_t player_);
    void run();

    // ---- start (media_eval_b_start.cpp)
    void move_third_column(std::uint32_t amount);
    void assistant_pull();
    bool find_contract();            // r30; false: the routine ends
    void weigh_story();              // f29, f30 and the flag 0x400/0x100/0x80 stage of ratings
    bool listed_as_target();         // the scan of the reporter's list (0x18-byte rows) for the player
    void rate_by_flags();
    void every_person();
    void one_person();
    bool gate();                     // the person's contract and a draw; false: next person

    // ---- the morale nudges and the 0x40 stage (media_eval_b_stages.cpp)
    void stages();                   // 0x10662d30 .. 0x10664778, falls out at 0x10664778

    // ---- the verdict stages (the other files); each ends the person
    void self_80();                  // 0x10664794
    void other_80();                 // 0x106667e0
    void self_100();                 // 0x10667e04
    void other_100();                // 0x1066a990

    // ---- views
    records::Person who(std::uint32_t at) const { return {c, at}; }
    records::Club club_record() const { return {c, club}; }
    records::Contract player_contract() const { return {c, contract}; }          // the player's with the club
    records::Contract person_contract_record() const { return {c, person_contract}; }  // the visited person's
    records::TallyRow tally_row(std::uint32_t person_) const { return records::tally_row_of(c, c.u32(person_)); }
    Field<std::uint8_t> tally(std::uint32_t person_) const { return tally_row(person_).tally(); }
    double dbl(std::uint32_t offset) const { return c.f64(pool + offset); }     // the pool of doubles (r18)
    double single(std::uint32_t offset) const { return double(c.f32(singles + offset)); }  // the pool of singles (r17)
    double bias() const { return dbl(8); }
    double verdict() const { return c.f(30); }
    double lean() const { return c.f(29); }
    bool has(std::uint32_t bit) const { return nonzero(c, std::int32_t(flags & bit)); }
    // `lfd f0,off(r18); fadd f0,f0,f29; fcmpo cr0,f30,f0`.
    Comparison compare_verdict(std::uint32_t offset) {
        const double threshold = fpu.fadd(dbl(offset), lean());
        c.set_f(0, threshold);
        fpu.fcmp(0, verdict(), threshold);
        return {verdict() < threshold, threshold < verdict()};
    }
    // The verdict against the band lean +- the pool's double at `offset`.
    Band band(std::uint32_t offset) {
        if (compare_verdict(offset).greater) return Band::above;
        const double bottom = fpu.fsub(lean(), dbl(offset));  // lfd f0,off(r18); fsub f0,f29,f0
        c.set_f(0, bottom);
        fpu.fcmp(0, verdict(), bottom);
        return verdict() < bottom ? Band::below : Band::between;
    }

    // ---- draws
    // `r16 = n; r15 = random(n)` in line (the wide form of the original never runs for a byte-sized range;
    // zero draws nothing and leaves 0). `back` is the address after the refill call.
    std::int32_t draw(std::int32_t n, std::uint32_t back) {
        range = std::uint32_t(n);
        return draw_narrow(c, rng, n, back, 15);
    }
    // random(trait of `holder`).
    std::int32_t roll_trait(std::uint32_t holder, TraitOf trait, std::uint32_t back) {
        return draw((who(holder).*trait)().get(), back);
    }
    // random(trait of `holder` + bias). The trait is in r3 until the draw replaces it (a zero range keeps it).
    std::int32_t roll_trait_plus(std::uint32_t holder, TraitOf trait, std::int32_t bias, std::uint32_t back) {
        const std::int32_t value = (who(holder).*trait)().get();
        c.set(3, std::uint32_t(value));
        return draw(value + bias, back);
    }
    // random(trait of `holder` / 2), the halving rounding toward zero ((x + sign) >> 1). The trait is in r3 as above.
    std::int32_t roll_half_trait(std::uint32_t holder, TraitOf trait, std::uint32_t back) {
        const std::int32_t value = (who(holder).*trait)().get();
        c.set(3, std::uint32_t(value));
        return draw((value + std::int32_t(std::uint32_t(value) >> 31)) >> 1, back);
    }
    // The next sample reduced by a constant, as the compiler forms it; r3 ends holding the quotient's sign and
    // r4 the sample.
    std::int32_t beaten_by(const Reciprocal& by, std::uint32_t back) {
        rng.advance(back);
        const auto reduced = reduced_by_signed(rng.sample(), by);
        leave(c, {{3, reduced.sign}, {4, reduced.sample}});
        return reduced.rest;
    }

    // ---- the routines of other blocks (r3 is the reporter wherever the original sets it)
    void morale(std::uint32_t who_, std::int32_t delta, std::uint32_t back) {
        program.call(move_morale, back, {who_, std::uint32_t(delta), 1});
    }
    bool asks(std::uint32_t query, std::uint32_t back, std::uint32_t about, std::uint32_t other_) {
        return nonzero(c, byte_word(program.call(query, back, {about, other_})));
    }
    void change_a(std::uint32_t amount, std::uint32_t back) {
        enter(c, back, {reporter, club, player, kind, amount, person});
        change_rating_a(c, club, player, kind, amount, person);
    }
    void change_b(std::uint32_t amount, std::uint32_t back) {
        enter(c, back, {reporter, club, player, kind, amount, person, 0});
        change_rating_b(c, club, player, kind, amount, person, 0);
    }
    // A person falls out with the club's manager (`replace`: over a drawn slot when the list is full).
    void fall_out_with_manager(std::uint32_t who_, std::uint32_t replace, std::uint32_t back) {
        program.call(relations::fall_out, back, {who_, club_record().manager(), replace});
    }
    // The player falls out with the club's manager and loses the contract's bit 8.
    void turn_on_manager(std::uint32_t call_back, std::uint32_t clear_back, std::uint32_t replace = 0) {
        fall_out_with_manager(player, replace, call_back);
        program.call(clear_flag, clear_back, {contract, 0x8, 0});
    }
    // The contract's morale moves by the signed byte of `raw`, kept within -100..100. r3 holds `raw` and r4 that byte;
    // after a top-out r3 still holds `raw`, otherwise the old morale.
    void shift_contract_morale(const records::Contract& deal, std::uint32_t raw) {
        const auto step = byte_word(raw);
        c.set(3, raw);
        c.set(4, std::uint32_t(step));
        const auto morale = deal.morale();
        const auto old = morale.get();
        if (compare(c, old + step, 0x64) > 0) {
            morale = std::int8_t(0x64);
            return;
        }
        c.set(3, std::uint32_t(old));
        if (compare(c, old + step, -0x64) < 0) morale = std::int8_t(-0x64);
        else morale = std::int8_t(std::uint32_t(old) + std::uint32_t(step));
    }
    // 0x1067d830 (reporter, player, club, kind, 2): a follow-up item about the player.
    void post_follow_up(std::uint32_t kind_code, std::uint32_t back) {
        enter(c, back, {reporter, player, club, kind_code, 2});
        post_item_bdc(c, player, club, kind_code, 2);
    }
    // 0x1067d040 (reporter, player, club, 0..3, kind).
    void post_reaction_item(std::uint32_t grade, std::uint32_t back) {
        enter(c, back, {reporter, player, club, grade, kind});
        post_item_bcf(c, player, club, grade, kind);
    }
    // 0x10649900 (reporter, player, club, kind).
    void post_escalation(std::uint32_t back) {
        enter(c, back, {reporter, player, club, kind});
        post_item_bdb(c, player, club, kind);
    }
    // 0x106548e0 (reporter, player, club, kind, 0).
    void post_note(std::uint32_t back) {
        enter(c, back, {reporter, player, club, kind, 0});
        post_item_bda(c, reporter, player, club, kind, 0);
    }

    Cpu& c;
    Frame frame;
    Fpu fpu;
    Stream rng;
    Program program;
    std::uint32_t pool, singles;
    // The registers the routine keeps: r23 the reporter, r24 the club, r25 the story's kind, r26 the flags (the
    // verdict may add 0x80), r27 the player (0: none), r28 the kind when it is 0x13 (else 0), r29 the side, then the
    // person being visited, r13 whether the call came with neither flag 0x80 nor 0x100 (a 0x80 stage then comes from
    // the verdict).
    Kept<23> reporter;
    Kept<24> club;
    Kept<25> kind;
    Kept<26> flags;
    Kept<27> player;
    Kept<28> rival_kind;
    Kept<29> person;
    Kept<13> asked_for_neither;
    // r30 the player's contract with the club, r31 the visited person's: the caller's until they are found.
    Held<30> contract;
    Held<31> person_contract;
    // r15 and r16: the result and the range of the latest draw (stages keep other values there: the gain, a count).
    Held<15> drawn;
    Held<16> range;
    double factor;  // f28
    std::uint32_t amount;
};
}
