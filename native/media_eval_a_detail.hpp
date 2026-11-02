#pragma once
// 0x106584d0: the manager's verdict on a story about a player. The routine is one chain of draws: each
// story keeps asking whether a trait of the manager (or of the staff record behind him) beats a draw of the
// table, and answers 1, 2 or 3. The compiler expands the table's draws in line, so the helpers here carry
// the registers and stack slots every site leaves.
#include "guest_fpu.hpp"
#include "media_eval_a.hpp"
#include "media_values.hpp"
#include "media_values_detail.hpp"

namespace cm::aot::media::eval_a {
using namespace guest;
using namespace detail;

constexpr std::uint32_t entry = 0x106584d0;
// Routines of other modules the verdict asks.
namespace ask {
constexpr std::uint32_t contract_of = 0x10082f90, staff_trait = 0x100d2f50, query_c = 0x100d9520, query_d = 0x100d9580,
    team_mate = 0x100d3150, transfer_story = 0x1069ecf0, has_flag = 0x1037f110, moves_in = 0x1036d240, calm_down = 0x100d3040,
    praise = 0x100d9b80;
}

// The manager's personality bytes, signed (the person's +0x63...: as records.hpp names them) and the attributes of the
// staff record behind him (+0x1c) that the verdict weighs; 0x39 is the one 0x100d2f50 adjusts. (The attributes are
// named by offset: nothing else in the game names them yet.)
namespace personality {
constexpr unsigned ambition = 0x63, loyalty = 0x65, professionalism = 0x66, sportsmanship = 0x67, trait_68 = 0x68, trait_69 = 0x69;
}
namespace attribute {
constexpr unsigned attribute_34 = 0x34, attribute_39 = 0x39, attribute_3b = 0x3b, attribute_3d = 0x3d, attribute_3f = 0x3f;
}

// The compiler's remainder of the table's sample by a constant: the multiply-high magic, its shift, the
// divisor, whether the high word is added to the sample first, and which scratch registers the schedule
// leaves behind: `quotient` (r3 the quotient's sign, r4 the sample), `high` (r3 the multiply-high word, r4 the
// sample: a quotient that needs no shift), `loaded` (the operand was loaded after: r3 the remainder, r4 the
// sign, r5 the sample).
enum class Scratch { quotient, high, loaded };
struct Division {
    std::int32_t magic;
    unsigned shift;
    std::int32_t divisor;
    bool add;
    Scratch scratch;
    bool negated = false;  // the draw is compared as the negated remainder
};
namespace by {
constexpr Division n3{0x55555556, 0, 3, false, Scratch::high}, n6{0x2aaaaaab, 0, 6, false, Scratch::high},
    n5{0x66666667, 1, 5, false, Scratch::quotient}, n10{0x66666667, 2, 10, false, Scratch::quotient},
    n20{0x66666667, 3, 20, false, Scratch::quotient}, n40{0x66666667, 4, 40, false, Scratch::quotient},
    n50{0x51eb851f, 4, 50, false, Scratch::quotient}, n36{0x38e38e39, 3, 36, false, Scratch::quotient},
    n150{0x1b4e81b5, 4, 150, false, Scratch::quotient}, n23{std::int32_t(0xb21642c9), 4, 23, true, Scratch::quotient},
    n15{std::int32_t(0x88888889), 3, 15, true, Scratch::quotient}, n30{std::int32_t(0x88888889), 4, 30, true, Scratch::quotient},
    loaded10{0x66666667, 2, 10, false, Scratch::loaded}, loaded20{0x66666667, 3, 20, false, Scratch::loaded},
    loaded100{0x51eb851f, 5, 100, false, Scratch::loaded},
    // the remainder by -5, compared negated
    minus5{std::int32_t(0x99999999), 1, -5, false, Scratch::quotient, true};
}

class Verdict {
public:
    Verdict(Cpu& cpu, std::uint32_t reporter, std::uint32_t manager, std::uint32_t player, std::uint32_t kind);
    void run();

private:
    // ---- the preamble (media_eval_a_start.cpp)
    bool admit();
    void value_of_story();
    // ---- the stories (media_eval_a_*.cpp), by kind; each returns true once it has answered
    bool story_83();
    bool story_82();
    bool story_1f();
    bool mood();
    bool mood_good();
    bool mood_bad();
    bool story_3c();
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
    std::uint32_t staff() const { return c.u32(manager + 0x1c); }
    std::int32_t kind_half() const { return half_word(kind); }
    std::int32_t amount() const { return std::int32_t(c.r(30)); }
    std::int32_t manager_byte(unsigned offset) const { return byte_word(c.u8(manager + offset)); }
    std::int32_t staff_byte(unsigned offset) const { return byte_word(c.u8(staff() + offset)); }
    std::int32_t player_byte(unsigned offset) const { return byte_word(c.u8(c.u32(player + 0x14) + offset)); }
    std::uint32_t contract_state() const { return c.u8(contract + 0x60) & 0xf; }
    bool asks(std::uint32_t query, std::uint32_t back, std::uint32_t about, std::uint32_t other) {
        return nonzero(c, byte_word(program.call(query, back, {about, other})));
    }
    // The staff record's attribute at `offset`, as 0x100d2f50 adjusts it.
    std::int32_t staff_trait(unsigned offset, std::uint32_t back) {
        const auto record = staff();
        return byte_word(program.call(ask::staff_trait, back, {record, c.u8(record + offset)}));
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
    std::int32_t roll(unsigned held, unsigned into, std::int32_t n, std::uint32_t refill_back, std::uint32_t call_offset = 0xd8) {
        if (held) c.set(held, std::uint32_t(n));
        return draw_below(c, frame, fpu, rng, c.r(28), site(refill_back, into, call_offset), n);
    }
    // r19 = n; r18 = a draw in [0, n): the draw most of the checks chain.
    std::int32_t draw(std::int32_t n, std::uint32_t refill_back) { return roll(19, 18, n, refill_back); }
    // The draw of the manager's own byte `offset` (r19 holds the byte).
    std::int32_t draw_manager(unsigned offset, std::uint32_t refill_back) { return draw(manager_byte(offset), refill_back); }
    std::int32_t draw_staff(unsigned offset, std::uint32_t refill_back) { return draw(staff_byte(offset), refill_back); }
    // The draw of the staff attribute 0x100d2f50 gives (the call's return address, then the draw's).
    std::int32_t draw_trait(unsigned offset, std::uint32_t query_back, std::uint32_t refill_back) {
        return draw(staff_trait(offset, query_back), refill_back);
    }
    // The story's amount (r30) as the range of a draw held in r30.
    std::int32_t draw_amount(std::uint32_t refill_back) { return roll(30, 18, amount(), refill_back); }
    // A draw whose range is an expression of the amount that the original keeps in no register: the narrow form
    // leaves r3 the range and r4 the sample; r5 the table entry (`entry` set) or the cursor's address; the wide one
    // leaves r3 the range when the original recomputes it after the call (`range_after`).
    std::int32_t roll_range(unsigned into, std::int32_t n, std::uint32_t refill_back, bool range_after, bool entry);
    // Two draws the compiler overlapped: the amount moves up by the first remainder by 10 and down by the second.
    void swing_amount(std::uint32_t first_back, std::uint32_t second_back);
    // r19 = the staff attribute 0x3d + the adjusted attribute 0x39 + the manager's loyalty; the draw of it (r18).
    std::int32_t draw_sum(std::uint32_t query_back, std::uint32_t refill_back);
    std::int32_t draw_sum_plain(std::uint32_t refill_back);
    bool kind_in(std::initializer_list<std::int32_t> kinds);
    bool kind_between(std::uint32_t first, std::uint32_t last);
    DrawSite site(std::uint32_t refill_back, unsigned into, std::uint32_t call_offset) const {
        return {refill_back, refill_back + call_offset, 0x38, 0x40, 0x48, into};
    }
    // The remainder of the next sample by a constant, compared the way the schedule leaves the registers.
    std::int32_t remainder(const Division& by, std::uint32_t refill_back);
    // cmpw operand,(remainder)
    std::int32_t compare_remainder(std::int32_t operand, const Division& by, std::uint32_t refill_back) {
        return compare(c, operand, remainder(by, refill_back));
    }

    Cpu& c;
    Frame frame;
    Fpu fpu;
    Stream rng;
    Program program;
    std::uint32_t reporter, manager, player, kind;
    std::uint32_t club = 0, contract = 0;
};
}
