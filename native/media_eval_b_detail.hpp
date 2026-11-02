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
#include "media_records.hpp"
#include "media_values.hpp"
#include "media_values_detail.hpp"

namespace cm::aot::media::eval_b {
using namespace guest;
using namespace detail;

constexpr std::uint32_t entry = 0x106620b0;
// The callees (addresses of the original, for those without a typed function).
constexpr std::uint32_t query_best = 0x100d9480, query_worst = 0x100d94d0, query_rival_good = 0x100d9520, query_rival_bad = 0x100d9580;
constexpr std::uint32_t move_morale = 0x100d3570;     // (person, delta, 1)
constexpr std::uint32_t contract_of = 0x10082f90;     // (table, person, club) -> the person's contract with the club
constexpr std::uint32_t set_flag = 0x1037f7d0, clear_flag = 0x1037fa80, set_flag_b = 0x1037f8a0;  // (contract, bits[, 0])
constexpr std::uint32_t round_half = 0x10775540;

// A division the compiler turned into a multiply-high: the magic, the shift, the divisor and whether the
// quotient adds the dividend back (magic above 2^31 as a signed word).
struct Divisor {
    std::int32_t magic;
    unsigned shift;
    std::int32_t divisor;
    bool adds;
};
namespace divisors {
constexpr Divisor by_3{0x55555556, 0, 3, false}, by_5{0x66666667, 1, 5, false}, by_6{0x2aaaaaab, 0, 6, false},
    by_10{0x66666667, 2, 10, false}, by_15{-0x77777777, 3, 15, true}, by_20{0x66666667, 3, 20, false},
    by_25{0x51eb851f, 3, 25, false}, by_30{-0x77777777, 4, 30, true}, by_40{0x66666667, 4, 40, false},
    by_60{-0x77777777, 5, 60, true}, by_100{0x51eb851f, 5, 100, false}, by_120{-0x77777777, 6, 120, true};
}
// The outcome of comparing the verdict with a threshold (fcmpo cr0): `ble` is taken unless greater,
// `bge` unless less (unordered counts as neither).
struct Verdict {
    bool less, greater;
};

struct Reaction {
    Reaction(Cpu& cpu, std::uint32_t reporter_, std::uint32_t club_, std::uint32_t kind_, std::uint32_t flags_, double factor_,
             std::uint32_t side_, std::uint32_t amount_, std::uint32_t player_);
    void run();

    // ---- start (media_eval_b_start.cpp)
    void move_third_column(std::uint32_t amount);
    void rival_pull();
    bool find_standing();            // r30; false: the routine ends
    void weigh_story();              // f29, f30 and the flag 0x400/0x100/0x80 stage of ratings
    bool listed_as_target();         // the scan of the reporter's list (0x18-byte rows) for the player
    void rate_by_flags();
    void every_person();
    void one_person();
    bool gate();                     // the person's standing and a draw; false: next person

    // ---- the morale nudges and the 0x40 stage (media_eval_b_stages.cpp)
    void stages();                   // 0x10662d30 .. 0x10664778, falls out at 0x10664778

    // ---- the verdict stages (the other files); each ends the person
    void self_80();                  // 0x10664794
    void other_80();                 // 0x106667e0
    void self_100();                 // 0x10667e04
    void other_100();                // 0x1066a990

    // ---- views
    records::Person who(std::uint32_t at) const { return {c, at}; }
    double dbl(std::uint32_t offset) const { return c.f64(pool + offset); }     // the pool of doubles (r18)
    double single(std::uint32_t offset) const { return double(c.f32(singles + offset)); }  // the pool of singles (r17)
    double bias() const { return dbl(8); }
    double verdict() const { return c.f(30); }
    double lean() const { return c.f(29); }
    // The per-person tally byte (+0xa of the 0x64-byte rows behind the TOC slot -0x7b9c, r14).
    Field<std::uint8_t> tally(std::uint32_t person_) const { return {c, c.u32(tallies) + person_ * 0x64 + 0xa}; }
    // `lfd f0,off(r18); fadd f0,f0,f29; fcmpo cr0,f30,f0`.
    Verdict compare_verdict(std::uint32_t offset) {
        const double threshold = fpu.fadd(dbl(offset), lean());
        c.set_f(0, threshold);
        fpu.fcmp(0, verdict(), threshold);
        return {verdict() < threshold, threshold < verdict()};
    }

    // ---- draws
    // `r16 = n; r15 = random(n)` in line (the wide form of the original never runs for a byte-sized range;
    // zero draws nothing and leaves 0). `back` is the address after the refill call.
    std::int32_t draw(std::int32_t n, std::uint32_t back) {
        c.set(16, std::uint32_t(n));
        return draw_narrow(c, rng, n, back, 15);
    }
    // The next sample reduced by a constant, as the compiler forms it; r3 ends holding the quotient's sign and
    // r4 the sample.
    std::int32_t beaten_by(const Divisor& by, std::uint32_t back) {
        rng.advance(back);
        const auto sample = rng.sample();
        const auto high = std::uint32_t(multiply_high(by.magic, std::int32_t(sample))) + (by.adds ? sample : 0u);
        const auto shifted = std::uint32_t(std::int32_t(high) >> by.shift), sign = shifted >> 31;
        leave(c, {{3, sign}, {4, sample}});
        return std::int32_t(sample - (shifted + sign) * std::uint32_t(by.divisor));
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

    Cpu& c;
    Frame frame;
    Fpu fpu;
    Stream rng;
    Program program;
    std::uint32_t pool, singles, tallies;
    // r23, r24, r25, r26 (mutable: the verdict stage may add 0x80), r27 (0: none)
    std::uint32_t reporter, club, kind, flags, player;
    // r28 (the kind when it is 0x13, else 0), r29 (the side, then the person being visited), r30 (the player's
    // contract with the club), r31 (the visited person's contract), r13 (a flag: the story found the player listed)
    std::uint32_t rival_kind, person, standing, person_standing;
    double factor;  // f28
    std::uint32_t amount;
};
}
