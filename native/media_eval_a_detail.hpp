#pragma once
// 0x106584d0: the manager's verdict on a story about a player. The routine is one chain of draws: each
// story keeps asking whether a trait of the manager (or of the staff record behind him) beats a draw of the
// table, and answers 1, 2 or 3. The compiler expands the table's draws in line, so the helpers here carry
// the registers and stack slots every site leaves.
//
// The kinds 0x82 and 0x83 are not stories but the codes under which the press items ask: 0x83 the item about a
// person's standing at his club (0xbe8), 0x82 the items about two people or a person and his club (0xbe6, 0xbe5).
#include "guest_fpu.hpp"
#include "media_eval_a.hpp"
#include "media_leaves.hpp"
#include "media_records.hpp"
#include "media_values.hpp"
#include "media_values_detail.hpp"

namespace cm::aot::media::eval_a {
using namespace guest;
using namespace detail;

constexpr std::uint32_t entry = 0x106584d0;
// Routines of other modules the verdict asks (the questions about likes and dislikes are in detail::relations).
namespace callee {
constexpr std::uint32_t contract_of = 0x10082f90, effective_skill = 0x100d2f50, team_mate = 0x100d3150,
                        has_flag = 0x1037f110, moves_in = 0x1036d240, calm_down = 0x100d3040;
}
constexpr int world_slot = -0x7b40;  // the object whose +0x988 is the list of moves (0x1036d240)

// What a draw is made over. The personality bytes of a person (the manager, or the person the story is about),
// the attributes of the manager's staff record and the bytes of the player's record.
enum class Trait { ambition, loyalty, professionalism, sportsmanship, composure, temperament };
enum class Skill { attitude, man_management, tactics, standing, discipline };
enum class Attribute { dirtiness, versatility };

// A draw in [0, n) with n one of those. `refill_back` is the address after the call that refills the table; an
// effective skill goes through 0x100d2f50 first (`query_back` is the address after that call).
enum class From { manager, staff, effective, amount, subject, subject_attribute };
struct Draw {
    From from;
    unsigned which;
    std::uint32_t query_back, refill_back;
};
namespace over {
constexpr Draw manager(Trait trait, std::uint32_t back) { return {From::manager, unsigned(trait), 0, back}; }
constexpr Draw staff(Skill skill, std::uint32_t back) { return {From::staff, unsigned(skill), 0, back}; }
constexpr Draw effective(Skill skill, std::uint32_t query_back, std::uint32_t back) {
    return {From::effective, unsigned(skill), query_back, back};
}
constexpr Draw amount(std::uint32_t back) { return {From::amount, 0, 0, back}; }
constexpr Draw subject(Trait trait, std::uint32_t back) { return {From::subject, unsigned(trait), 0, back}; }
constexpr Draw subject_attribute(Attribute attribute, std::uint32_t back) {
    return {From::subject_attribute, unsigned(attribute), 0, back};
}
}

// The compiler's remainder of the table's sample by a constant: the divisor's multiply-high sequence and which
// scratch registers the schedule leaves behind: `quotient` (r3 the quotient's sign, r4 the sample), `high` (r3 the
// multiply-high word, r4 the sample: a quotient that needs no shift), `loaded` (the operand was loaded after: r3 the
// remainder, r4 the sign, r5 the sample). `negated`: the draw is compared as the negated remainder.
enum class Scratch { quotient, high, loaded };
struct Remainder {
    Reciprocal reciprocal;
    Scratch scratch;
    bool negated = false;
};
namespace by {
constexpr Remainder n3{divisors::by_3, Scratch::high}, n6{divisors::by_6, Scratch::high},
    n5{divisors::by_5, Scratch::quotient}, n10{divisors::by_10, Scratch::quotient},
    n15{divisors::by_15, Scratch::quotient}, n20{divisors::by_20, Scratch::quotient},
    n23{divisors::by_23, Scratch::quotient},
    n30{divisors::by_30, Scratch::quotient}, n36{divisors::by_36, Scratch::quotient},
    n40{divisors::by_40, Scratch::quotient}, n50{divisors::by_50, Scratch::quotient},
    n150{divisors::by_150, Scratch::quotient}, loaded10{divisors::by_10, Scratch::loaded},
    loaded20{divisors::by_20, Scratch::loaded}, loaded100{divisors::by_100, Scratch::loaded},
    minus5{divisors::by_minus_5, Scratch::quotient, true};  // the remainder by -5, compared negated
}

// One link of a chain of draws that must all come out as `test` for a story to answer: the draw against the zero
// (zero, nonzero) or against the next sample's remainder by a constant, which is read after the draw.
enum class Test { zero, nonzero, below, above, at_most };
struct Step {
    Draw draw;
    Test test;
    Remainder against = {{}, Scratch::quotient};
    std::uint32_t reduce_back = 0;  // the refill call of the remainder's sample
};

class Verdict {
public:
    Verdict(Cpu& cpu, std::uint32_t reporter, std::uint32_t manager, std::uint32_t player, std::uint32_t kind);
    void run();

private:
    // ---- the preamble (media_eval_a_start.cpp)
    bool admit();
    void value_of_story();
    // ---- the stories (media_eval_a_*.cpp), by kind; the bool ones return true once they have answered
    void standing_item();    // code 0x83
    void pair_item();        // code 0x82
    void happiness_story();  // kind 0x1f
    bool mood();
    bool fixture_story();    // kinds 0x3c, 0x3d
    bool transfer_story();
    bool story_social();
    bool story_8_25();
    bool story_1();
    bool story_1e_20();
    bool story_rated();
    bool story_3_4();
    bool story_5();
    bool story_6();

    // ---- views
    records::Person boss() const { return {c, manager}; }
    records::Person subject() const { return {c, player}; }  // the person the story is about (0: none)
    records::StaffRecord boss_staff() const { return boss().staff_record(); }
    records::Contract deal() const { return {c, contract}; }
    records::Club employer() const { return {c, club}; }
    records::TallyRow tally_row() const { return records::tally_row_of(c, c.u32(player)); }
    std::int32_t kind_half() const { return half_word(kind); }
    std::int32_t amount() const { return std::int32_t(std::uint32_t(story_amount)); }
    std::int32_t trait(const records::Person& who, Trait trait) const;
    std::int32_t skill(Skill skill) const;
    std::int32_t attribute(Attribute attribute) const;
    std::uint32_t contract_state() const { return deal().squad_status().get() & 0xf; }
    bool asks(std::uint32_t query, std::uint32_t back, std::uint32_t about, std::uint32_t other) {
        return nonzero(c, byte_word(program.call(query, back, {about, other})));
    }
    // The staff record's skill as 0x100d2f50 scales it by the record's ability.
    std::int32_t effective(Skill skill_, std::uint32_t back) {
        const auto record = boss().non_player().get();
        return byte_word(program.call(callee::effective_skill, back, {record, std::uint8_t(skill(skill_))}));
    }
    // 32-bit wrapping arithmetic on the amount, as the original's add and subfic do.
    static std::int32_t sum(std::int32_t a, std::int32_t b) {
        return std::int32_t(std::uint32_t(a) + std::uint32_t(b));
    }
    static std::int32_t difference(std::int32_t a, std::int32_t b) {
        return std::int32_t(std::uint32_t(a) - std::uint32_t(b));
    }

    // ---- the answers
    // `li r3,v; b 0x10661a3c`
    bool answer(std::uint32_t v) {
        c.set(3, v);
        return true;
    }
    // `li r17,v; b 0x10661a38` (r3 follows r17)
    bool verdict_is(std::uint32_t v) {
        c.set(17, v);
        c.set(3, v);
        return true;
    }

    // ---- the table's draws, in line
    // A draw in [0, n): n waits in register `held` (0: in no kept register), the result goes to `into`.
    // (The wide form's call to 0x10647c00 follows the refill by 0xd8 bytes; by 0xe8 where the range is an expression.)
    std::int32_t roll(unsigned held, unsigned into, std::int32_t n, std::uint32_t refill_back,
                      std::uint32_t call_offset = 0xd8) {
        if (held) c.set(held, std::uint32_t(n));
        return draw_below(c, frame, fpu, rng, c.r(28), site(refill_back, into, call_offset), n);
    }
    // r19 = n; r18 = a draw in [0, n): the draw most of the checks chain.
    std::int32_t draw(std::int32_t n, std::uint32_t refill_back) { return roll(19, 18, n, refill_back); }
    // The story's amount (r30) as the range of a draw held in r30.
    std::int32_t draw_amount(std::uint32_t refill_back) { return roll(30, 18, amount(), refill_back); }
    std::int32_t draw_of(const Draw& what);
    // A draw whose range is an expression of the amount that the original keeps in no register: the narrow form
    // leaves r3 the range and r4 the sample; r5 the table entry (`entry` set) or the cursor's address; the wide one
    // leaves r3 the range when the original recomputes it after the call (`range_after`).
    std::int32_t roll_range(unsigned into, std::int32_t n, std::uint32_t refill_back, bool range_after, bool entry);
    // Two draws the compiler overlapped: the amount moves up by the first remainder by 10 and down by the second.
    void swing_amount(std::uint32_t first_back, std::uint32_t second_back);
    // r19 = the staff skill `standing` + the effective skill `man_management` + the manager's loyalty; the draw of it
    // (r18).
    std::int32_t draw_sum(std::uint32_t query_back, std::uint32_t refill_back);
    std::int32_t draw_sum_plain(std::uint32_t refill_back);
    bool kind_in(std::initializer_list<std::int32_t> kinds);
    bool kind_between(std::uint32_t first, std::uint32_t last);
    DrawSite site(std::uint32_t refill_back, unsigned into, std::uint32_t call_offset) const {
        return {refill_back, refill_back + call_offset, 0x38, 0x40, 0x48, into};
    }
    // The remainder of the next sample by a constant, compared the way the schedule leaves the registers.
    std::int32_t remainder(const Remainder& divisor, std::uint32_t refill_back);
    // cmpw operand,(remainder)
    std::int32_t compare_remainder(std::int32_t operand, const Remainder& divisor, std::uint32_t refill_back) {
        return compare(c, operand, remainder(divisor, refill_back));
    }
    // A draw against `step.test`, and a chain of them: the first that fails ends the chain (no later draw is made).
    bool passes(const Step& step);
    bool all(std::initializer_list<Step> steps);

    Cpu& c;
    Frame frame;
    Fpu fpu;
    Stream rng;
    Program program;
    std::uint32_t reporter, manager, player, kind;
    std::uint32_t club = 0, contract = 0;
    Held<30> story_amount;  // r30: the story's value, scaled
};
}
