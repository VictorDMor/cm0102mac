#pragma once
// 0x10672800: the press evaluates a story about a club. Each kind of story moves the club's two
// rating columns through the rating routines of the values block (0x1066b600, 0x1066bc50) by amounts
// that scale with the story's strength, the club's flags and a pull towards the story's other club,
// and ends with an item posted by 0x106620b0. The original expands the same few sequences at every
// site (the compiler recomputes every int-to-double conversion in a fresh stack pair), so the
// helpers here carry the stack slots of each site.
#include "guest_fpu.hpp"
#include "media_eval_c.hpp"
#include "media_values.hpp"
#include "media_records.hpp"
#include "media_values_detail.hpp"
#include <span>

namespace cm::aot::media::eval_c {
using namespace guest;
using namespace detail;

constexpr std::uint32_t evaluate_entry = 0x10672800;
constexpr std::uint32_t rate_a = 0x1066b600, rate_b = 0x1066bc50, post_item = 0x106620b0, club_query = 0x10459b50;
constexpr std::uint32_t standing_of = 0x100c7d00, base_factor = 0x10457d00;

// rlwinm's mask for bits mb..me (PowerPC numbering, wrapping when mb > me).
constexpr std::uint32_t mask(unsigned mb, unsigned me) {
    const std::uint32_t low = ~0u >> mb, high = ~0u << (31 - me);
    return mb <= me ? low & high : low | high;
}

// An edit of the club's flag halfword: the bits kept and the bits set.
struct Edit {
    std::uint32_t keep, set;
};
// One scaling of the story's scale by a pool double, when a flag bit of the club is set.
struct Pull {
    std::uint32_t bits, pool;
};
// The addresses after the original's calls on one side of a story (what a callee saves as its link
// register): the table's refill, the two club queries, the two rating routines (and the boosted second
// one), the item, and a second item. Zero where the side makes no such call.
struct Backs {
    std::uint32_t refill, query_first, query_second, rate_a, rate_b, rate_boost, post, post_other;
};
// One further draw of the chained side 1 (kinds 0x40, 0x41): the draw, the flag the club must have, the
// edits it makes to the club's flags, and the boosted second rating. `check` asks more of the club's group:
// 1 a group record whose +0x1c is not -1 and whose +0xc1 byte is set, 2 one whose +0x1c is -1.
struct Step {
    std::uint32_t refill, flag;
    Edit first, second;
    int check;
    std::uint32_t boost_site, rate_boost;
};
// The stories whose losses are the strength less 1000 (kinds 0x3f, 0x60, 0x40, 0x41).
struct Penalty {
    std::span<const Pull> pulls;
    Backs down, up, third;
    bool upward_third;           // side 3 draws against (single + factor) rather than (factor - single)
    int sign_a, sign_b, sign_post;  // the sign of the strength in side 1's amounts
    std::span<const Step> chain;    // side 1 of 0x40/0x41; empty: one draw and the queries after it
    std::uint32_t boost_site;       // the single draw's boosted fmadd
};
struct Family {
    std::span<const Pull> pulls;
    std::span<const Edit> edits;        // side 1: the club's flags after the reporter's query
    std::uint32_t boost_site, boost_pool;  // the fmadd of the boosted second rating and its pool constant; side 1
    bool queries;                       // side 1 asks 0x10459b50 about the story's club first
    bool twenty;                        // side 1 draws out of 20, not 100
    bool column_after;                  // side 1: the story also moves the club's third rating column
    Backs down, up, third;
};

class Press;
namespace families {
extern const Family kind_63, kind_62, kind_5f, kind_5c;
extern const Penalty penalty_3f, penalty_60, penalty_40;
}

class Press {
public:
    Press(Cpu& cpu, std::uint32_t reporter_, std::uint32_t club_, std::uint32_t kind_, std::uint32_t side_, std::uint32_t other_);
    void run();

private:
    // ---- the preamble (media_eval_c_start.cpp)
    bool admit();
    void prepare();
    bool kind_is(std::initializer_list<std::int32_t> kinds);

    // ---- the stories, by family of kinds
    void story_3c();                          // 0x3c, 0x3d
    void up_3c();
    void down_3c();
    void third_3c();
    std::uint32_t loss_roll();
    void story_family(const Family& family);  // 0x63 0x5b 0x61 / 0x62 0x5d / 0x5f / 0x5c
    void family_down(const Family& family);
    void family_up(const Family& family);
    void family_third(const Family& family);

    void story_penalty(const Penalty& penalty);  // 0x3f, 0x60, 0x40, 0x41
    void penalty_losses(const Backs& at);
    void penalty_up_single(const Penalty& penalty);
    void penalty_up_chain(const Penalty& penalty);
    void penalty_third(const Penalty& penalty);
    double ratio(int sign, std::uint32_t first, std::uint32_t second);
    bool rolled_above(std::uint32_t back);
    bool queries_pass(const Backs& at);
    bool group_check(int check);
    void story_singles();                        // 0x43
    void singles_up();
    void singles_down();
    // The single-precision amounts of 0x43: rate * factor + offset (fmadds / fmsubs at `site`), the negated rate times
    // the factor, and that limited by the pool's single 8 (the expression is evaluated twice, as the original does).
    double single_fused(std::uint32_t site, std::uint32_t rate, std::uint32_t offset);
    double single_negative();
    double single_floored_negative();
    void story_42();
    void story_52();
    void story_scored(std::uint32_t down_refill, std::uint32_t down_back, std::uint32_t up_refill, std::uint32_t up_back);  // 0x44, 0x5a
    bool other_asks(std::uint32_t entry, std::uint32_t back, std::uint32_t about) {
        return nonzero(c, byte_word(Program{c}.call(entry, back, {other, about})));
    }

    // ---- views
    double dbl(std::uint32_t offset) const { return c.f64(pool + offset); }
    double bias() const { return dbl(8); }
    double single_at(std::uint32_t offset) const { return double(c.f32(singles + offset)); }
    std::int32_t strength() const { return half_word(c.r(30)); }
    double scale() const { return c.f(29); }
    double pull() const { return c.f(30); }
    double factor() const { return c.f(31); }
    void scale_by(std::uint32_t offset) { c.set_f(29, fpu.fmul(scale(), dbl(offset))); }

    // ---- the club's rating row
    // `lha 0xe` of the club's row. The club is never null here (the guards end the story without one), so
    // the original's `cmplwi` leaves cr0 greater-than and its null branch (0) never runs. r3 is left
    // holding the row.
    records::ClubRatingRow club_row() {
        compare_logical(c, club);
        const auto row = records::Club{c, club}.rating_row();
        c.set(3, row.at);
        return row;
    }
    std::int32_t club_flags() { return club_row().flags(); }
    // A record-form test of the flags against `bits` (cr0 compares the masked value with 0).
    bool flag(std::uint32_t bits) { return nonzero(c, std::int32_t(std::uint32_t(club_flags()) & bits)); }
    void edit_flags(const Edit& edit) {
        const auto flags = club_row().flags();
        flags = std::int16_t((std::uint32_t(flags.get()) & edit.keep) | edit.set);
    }
    // The first flag of `pulls` the club has scales the story by its pool constant.
    void apply_pulls(std::span<const Pull> pulls);
    // The third column (+0xa) of the club's row moves by pool(0x40) times the story's strength, within 1..10000.
    void move_third_column();

    // ---- floating point through the original's stack pairs
    // The int-to-double conversion of `value` at frame offset `at`, less the bias.
    double integer(std::uint32_t at, std::int32_t value) {
        return fpu.fsub(stacked_integer(c, frame.at(std::int32_t(at)), signed_word(value)), bias());
    }
    double strength_at(std::uint32_t at) { return integer(at, strength()); }
    // fctiwz into the pair at `at`; the low word is the amount.
    std::uint32_t truncated(std::uint32_t at, double value) {
        c.put64(frame.at(std::int32_t(at)), fpu.fctiwz(0, value));
        return c.u32(frame.at(std::int32_t(at + 4)));
    }
    // A value the original limits to `limit` (the pool's constant): the expression is evaluated and
    // compared with the limit and, unless the limit is smaller, evaluated again.
    template <class First, class Second>
    double limited_by(double limit, First&& first, Second&& second) {
        const double value = first();
        fpu.fcmp(0, limit, value);
        return limit < value ? limit : second();
    }
    // The same through stack pairs: the expression takes the pair of the evaluation. The second pair is
    // preceded by the r7 the original computes there (`leftover`: the strength, in the form the pair uses).
    template <class Expression>
    double limited_pair(double limit, std::uint32_t first, std::uint32_t second, std::uint32_t leftover, Expression&& expression) {
        return limited_by(limit, [&] { return expression(first); }, [&] {
            c.set(7, leftover);
            return expression(second);
        });
    }
    template <class Expression>
    std::uint32_t limited(double limit, std::uint32_t first, std::uint32_t second, std::uint32_t out, Expression&& expression,
                          std::uint32_t leftover) {
        return truncated(out, limited_pair(limit, first, second, leftover, expression));
    }
    template <class Expression>
    std::uint32_t limited(double limit, std::uint32_t first, std::uint32_t second, std::uint32_t out, Expression&& expression) {
        return limited(limit, first, second, out, expression, signed_word(strength()));
    }

    // ---- the routines of other blocks
    void change_a(std::uint32_t amount, std::uint32_t back) {
        enter(c, back, {reporter, club, other, kind, amount, side});
        change_rating_a(c, club, other, kind, amount, side);
    }
    void change_b(std::uint32_t amount, std::uint32_t boosted, std::uint32_t back) {
        enter(c, back, {reporter, club, other, kind, amount, side, boosted});
        change_rating_b(c, club, other, kind, amount, side, boosted);
    }
    // The story's item: `code` is the reaction, r9 an amount (or 0), r10 0.
    void post(std::uint32_t code, std::uint32_t amount, std::uint32_t back, std::uint32_t last = 0) {
        c.set_f(1, factor());
        c.set({{3, reporter}, {4, club}, {5, kind}, {6, code}, {8, side}, {9, amount}, {10, last}});
        call(c, post_item, back);
    }

    // ---- the draw in line
    // The table's sample reduced by 100 as the compiler does (and negated for the downward form):
    // r3 the quotient's sign, r4 the sample, r5 the table entry.
    std::int32_t hundred(std::uint32_t back, bool downward) {
        return downward ? roll(back, -0x51eb851f, 5, -100, true) : roll(back, 0x51eb851f, 5, 100, false);
    }
    std::int32_t roll(std::uint32_t back, std::int32_t magic, unsigned shift, std::int32_t modulus, bool negated) {
        rng.advance(back);
        const auto drawn = reduced(magic, shift, modulus);
        leave(c, {{3, drawn.sign}, {4, std::uint32_t(drawn.sample)}, {5, drawn.entry}});
        return negated ? std::int32_t(0u - std::uint32_t(drawn.rest)) : drawn.rest;
    }

    // The table's sample reduced by a constant the way the compiler does it: the multiply-high quotient
    // (shifted), its sign bit and the remainder, and the table entry the sample is made of.
    struct Residue {
        std::int32_t sample, high, rest;
        std::uint32_t sign, entry;
    };
    Residue reduced(std::int32_t magic, unsigned shift, std::int32_t modulus) {
        const auto entry = c.u16(c.u32(rng.cursor));
        const auto sample = std::int32_t(entry + c.u32(rng.seed));
        const auto high = multiply_high(magic, sample);
        const auto shifted = std::uint32_t(high >> shift), sign = shifted >> 31;
        return {sample, high, std::int32_t(std::uint32_t(sample) - (shifted + sign) * std::uint32_t(modulus)), sign, entry};
    }

    template <class Up, class Down, class Third>
    void by_side(Up&& up, Down&& down, Third&& third) {
        const auto s = byte_word(side);
        if (compare(c, s, 2) == 0) down();
        else if (compare(c, s, 2) > 0) {
            if (compare(c, s, 4) < 0) third();
        } else if (compare(c, s, 1) >= 0) up();
    }

    Cpu& c;
    Frame frame;
    Fpu fpu;
    Stream rng;
    Program program;
    std::uint32_t pool, singles, ratings;
    std::uint32_t reporter, club, kind, side, other;
};
}
