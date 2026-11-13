#pragma once
// TRANSFER_OFFER.CPP, building an offer (0x104126d0): the records it reads, its constants and the state
// its stages share. One entry with one frame; the stages are the member functions of OfferBuild
// (transfer_offer_unit_build.cpp: the price, transfer_offer_unit_build_terms.cpp: the terms).
#include "transfer_offer_unit_build.hpp"
#include "transfer_offer_unit_detail.hpp"
#include "guest_fpu.hpp"
#include <bit>

namespace cm::aot::transfer_offer_unit::build_detail {
using guest::Cpu;
using guest::Frame;

inline constexpr std::uint32_t entry = 0x104126d0;

// Offer bytes the detail header does not name.
namespace offer_more {
inline constexpr std::uint32_t byte_24 = 0x24,     // cleared
                               link_a = 0x2c,      // -1 in a new offer
                               link_b = 0x30,      // -1 in a new offer
                               byte_34 = 0x34,     // cleared (status_of's kind 3)
                               chance = 0x35,      // 1 when a draw marks the offer (a reputed player, same league)
                               byte_36 = 0x36,     // cleared
                               fixed = 0x37,       // the fee is the seller's own price (0x103fac70) at the end
                               handling = 0x38,    // high nibble: how the deal is handled (0, 1, 3, 4, 5)
                               clause = 0x3c;      // high nibble: 1 when the fee stops at the release clause
}
// A person (0x70 bytes) and the records behind him.
namespace person {
inline constexpr std::uint32_t id = 0x0, value = 0x8, nation = 0xc, playing = 0x14, birth_club = 0x20,
                               age = 0x5c /* the low byte of the halfword */, role_flag = 0x5e, role = 0x62;
}
namespace playing {  // the player record (person+0x14)
inline constexpr std::uint32_t reputation = 0x8, world_reputation = 0xa;  // int16
}
namespace nation {
inline constexpr std::uint32_t reputation = 0xac, number = 0x68;  // int16, word
}
namespace club {
inline constexpr std::uint32_t id = 0x0, nation = 0x2c, league = 0x30;
}
// A player's contract with his club (0x10082f20): the club's index, his release clause, its date.
namespace contract {
inline constexpr std::uint32_t club = 0x4, release_fee = 0x24, date = 0x30, kind = 0x38;  // kind: bits 2-7, signed
}
// The shortlist row the bid comes from (SHORTLIST_MANAGER's Row): kind 11 for a player.
namespace wanted {
inline constexpr std::uint32_t kind = 0x0, level = 0x4, weight = 0xc, flags = 0x28;  // category: bits 3-6
inline constexpr std::uint8_t player = 11;
}
namespace finances {
inline constexpr std::uint32_t barred = 0x165;
}
// Words of the table of contents this routine reads besides the detail header's.
namespace cell {
inline constexpr std::int32_t contracts = -0x7b7c,        // the contract manager
                              last_contract = -0x7b78,    // where the contract lookups leave their answer
                              transfers = -0x7b40,        // the transfer manager
                              today = -0x7b80,            // the date
                              finances = -0x7ab8,         // the finance manager
                              nation_a = -0x7a10,         // two nations whose clubs start from 0.75
                              nation_b = -0x7a1c;
}
namespace call {
inline constexpr std::uint32_t contract_of = 0x10082f20, contract_pending = 0x10083110, sign_free = 0x10080d30,
                               value_to = 0x10085860, human_asking = 0x10089a40, keeps_player = 0x100d3040,
                               valuation = 0x10084be0, club_human = 0x1014c2e0, deal_construct = 0x103721f0,
                               has_release_clause = 0x10380520, clause_payable = 0x10380f20,
                               refuses = 0x1036fbf0, ceiling = 0x10361490, unwanted = 0x103faff0,
                               earlier_offer = 0x103f5d20, pending_offer = 0x103f4240, seller_price = 0x103fac70,
                               approach = 0x103f8820, finances_of = 0x10103510, finance_state = 0x100f55b0,
                               balance = 0x100f5770, foreign_rule = 0x100ce250, role_rule = 0x100c7a20,
                               transfer_a = 0x103eaea0, transfer_b = 0x103eb020, transfer_row = 0x103eb440;
// Routines of the unit in other blocks' files.
inline constexpr std::uint32_t draw_below = 0x10415e50, total = 0x10416710, reshape_fee = 0x104172f0;
}
// The unit's doubles (TOC -0x4b8c), by offset.
namespace k {
inline constexpr std::uint32_t one = 0x00, noise = 0x08, base_factor = 0x10, three_quarters = 0x18,
                               factor_2 = 0x20, factor_1 = 0x28, factor_0 = 0x30, factor_m1 = 0x38, factor_m2 = 0x40,
                               tier_24 = 0x48, divisor_24 = 0x50, tier_18 = 0x58, divisor_18 = 0x60, tier_12 = 0x68,
                               divisor_12 = 0x30, divisor_6 = 0x70, much_more = 0x78, more = 0x80,
                               reputation_ratio = 0x88, gap_share = 0x90, hundred = 0x98, sample_range = 0xa0,
                               level_share = 0xa8, ratio_floor = 0xb0, clause_margin = 0xb8, magic = 0xc0;
// The same constants by their use where one serves several.
inline constexpr std::uint32_t two_fifths = factor_1, four_fifths = factor_0, month_scale = factor_m1;
}
// The unit's floats (TOC -0x4b90), by offset.
namespace kf {
inline constexpr std::uint32_t ratio_floor = 0x0, ratio_ceiling = 0x4, ten = 0x8, half = 0xc;
}

// The return addresses of one inline draw below a bound: the step's redraw and the wide bound's call
// of 0x10415e50.
struct BelowSite { std::uint32_t draw, call; };
// One split of the fee: the months (draw % modulus + base, in tens), the instalments' share
// (draw % 11 + share_add, percent-like), and two sell-on draws (5 * (draw % 6), or 5 * (draw % 5 + 1)
// with `five_steps`); each draw's redraw return address.
struct Split {
    std::uint32_t months_back;
    std::int32_t months_modulus, months_base;
    std::uint32_t share_back;
    std::int32_t share_add;
    std::uint32_t sell_on_a, sell_on_b;
    bool five_steps;
};
// Which of the two sell-on draws an offer takes: the lower, or the higher when the valuation gap is
// large (then optionally capping the instalments by the gap).
enum class SellOn { lower, higher_when_large, higher_and_capped_when_large };

class OfferBuild {
public:
    OfferBuild(Cpu& cpu, Frame& f, std::uint32_t offer_, std::uint32_t player_, std::uint32_t club_,
               std::uint32_t wanted_, std::int32_t budget_, std::uint32_t deal_)
        : c(cpu), frame(f), fpu(cpu), offer(offer_), player(player_), club(club_), row(wanted_), deal(deal_),
          budget(budget_), pool(cpu.toc(toc::doubles)), floats(cpu.toc(toc::floats)) {}

    // transfer_offer_unit_build.cpp
    void mark_chance(std::uint32_t first_draw, std::uint32_t contract_a, std::uint32_t contract_b,
                     std::uint32_t second_draw, std::int32_t one_in);
    void blank_terms(std::uint8_t fixed);
    void repeat_human_offer();
    std::uint32_t computer_offer();
    std::uint32_t finish();

private:
    Cpu& c;
    Frame& frame;
    guest::Fpu fpu;
    const std::uint32_t offer, player, club, row, deal;
    std::int32_t budget;
    const std::uint32_t pool, floats;

    std::uint32_t player_contract = 0;  // the player's contract (no last-contract store), 0: none
    std::uint32_t seller = 0;           // the player's club, 0: none
    std::int32_t asking = 0;            // what the seller asks
    std::int32_t ceiling = 0;           // the most the buyer will go to
    std::int32_t gap = 0;               // the seller's two valuations apart (0 for a human seller)
    std::int32_t human_floor = -1;      // a human seller's floor, -1: none
    bool seller_limited = false;        // the ceiling came from the seller's barred finances
    bool terms_set = false;             // a stage chose instalments, an extra or a sell-on share
    double more_than_fee = 0;           // 1.2 times the fee, as the budget check left it

    // ---- the price (transfer_offer_unit_build.cpp) ----
    std::uint32_t refuse_or_sign_free();
    void ask_price();
    void judge_ceiling();
    void scale_by_finances();
    double finance_factor(double factor, std::uint32_t of, std::uint32_t back_finances, std::uint32_t back_state,
                          bool seller_side);
    void divide_prices(std::uint8_t months, std::uint32_t divisor);
    enum class Verdict { settle, negotiate, small_gap };
    Verdict against_budget();
    std::uint32_t settle();
    std::uint32_t check_funds();

    // ---- the terms (transfer_offer_unit_build_terms.cpp) ----
    bool negotiate();
    void appearance_extra();
    void random_terms();
    std::uint32_t revise(std::int32_t prior, std::int32_t current);
    std::uint32_t counter_offer(std::int32_t current);
    void stretch_to_budget();
    void split_fee(const Split& split, SellOn rule);
    void cap_instalments();
    std::int32_t percent_of_fee(std::int32_t percent);
    std::int32_t instalments_for(std::int32_t share);
    std::int8_t sell_on_share(std::uint32_t back, bool five_steps);
    bool gap_is_large();
    void months_from(std::uint32_t back, std::int32_t modulus, std::int32_t base);

    // ---- shared pieces ----
    std::uint32_t transfers() const { return c.u32(c.toc(cell::transfers)); }
    std::uint8_t category() const { return std::uint8_t((c.u8(row + wanted::flags) >> 3) & 0xf); }
    bool wants_player() const { return row != 0 && c.u8(row + wanted::kind) == wanted::player; }
    // A row of category 5, or of category 4 for a player under 28.
    bool keen() const { return category() == 5 || (category() == 4 && age() < 28); }
    std::int32_t age() const { return std::int8_t(c.u16(player + person::age)); }
    std::int32_t fee() const { return c.i32(offer + field::fee); }
    std::int32_t months() const { return c.i8(offer + field::instalment_months); }
    std::int32_t nation_reputation() const { return c.i16(c.u32(player + person::nation) + nation::reputation); }
    double constant(std::uint32_t at) const { return c.f64(pool + at); }
    double single_constant(std::uint32_t at) const { return double(c.f32(floats + at)); }
    // An integer as the original converts it: the double of high word 0x43300000 and low word `x` xor
    // 0x80000000, less the constant at +0xc0 (fsub, or fsubs where the original has it).
    double to_double(std::int32_t x) { return fpu.fsub(magic_word(x), constant(k::magic)); }
    double to_single(std::int32_t x) { return fpu.fsubs(magic_word(x), constant(k::magic)); }
    static double magic_word(std::int32_t x) {
        return std::bit_cast<double>((std::uint64_t(0x43300000) << 32) | (std::uint32_t(x) ^ 0x80000000u));
    }
    std::int32_t truncate(double value) { return std::int32_t(std::uint32_t(fpu.fctiwz(0, value))); }
    // One step of the sample table, as a signed word.
    std::int32_t sample(std::uint32_t back) { return std::int32_t(draw(c, back)); }
    // `in_r13`: the original leaves the draw (and a wide bound's partial sum) in r13; one site keeps it in a
    // stack slot instead. The bound is in r14 at every site.
    std::int32_t draw_below(std::int32_t bound, const BelowSite& site, bool in_r13 = true);
    // The contract lookup (0x10082f20) whose answer the original leaves in the last-contract cell.
    std::uint32_t contract_of(std::uint32_t person_record, std::uint32_t back);
    // The club of a contract (by its index), 0 for none.
    std::uint32_t club_of(std::uint32_t found) const {
        return found ? c.u32(c.toc(toc::clubs)) + c.u32(found + contract::club) * club_size : 0;
    }
    static std::int32_t contract_kind(std::uint8_t byte) { return std::int8_t(byte) >> 2; }
    // The player's club when his contract is not of kind 5, read back from the last-contract cell.
    std::uint32_t contracted_club(std::uint32_t back);
    void set_handling(std::uint32_t nibble) {
        c.put8(offer + offer_more::handling, (c.u8(offer + offer_more::handling) & 0x0f) | (nibble << 4));
    }
    std::int32_t handling() const { return std::int8_t(c.u8(offer + offer_more::handling)) >> 4; }
    void free_deal(std::uint32_t back);
    // The original's non-volatile registers. The routines this one calls save them in their frames, and code
    // that runs after it reads those frames as stale stack: replaying season-day and same-save-continue with
    // the C++ diverges in the end state unless at least r18-r28 hold the original's values at the calls
    // (bisected 2026-11-13). So r13-r31 and f31 hold what the original holds at every call (the unit's
    // checker compares them per call), each set where the original sets it: the entry for the constant
    // ones, r13 and r14 at the original's reuses of them (an earlier offer, a refusal, an estimate, a draw's
    // bound and result, a sell-on share, a contract test, the approach's answer).
    void keep(unsigned n, std::uint32_t value) { c.set(n, value); }
    void keep_signed(unsigned n, std::int32_t value) { c.set(n, std::uint32_t(value)); }
    void keep_f31(double value) { c.set_f(31, value); }
    void set_asking(std::int32_t value) { asking = value; keep(22, std::uint32_t(value)); }
    void set_ceiling(std::int32_t value) { ceiling = value; keep(23, std::uint32_t(value)); }
    void set_budget(std::int32_t value) { budget = value; keep(19, std::uint32_t(value)); }
    // Whether the player is a reputed one for his nation: his reputation over 1.3 above the nation's,
    // or the birth-club rule (0x100ce250, only when he has one).
    bool reputed_for_nation(std::int32_t reputation_of_nation, std::uint32_t back);
};

// divw as the generated code computes it (a zero divisor or 0x80000000 / -1 stops it as there).
inline std::int32_t divide(std::int32_t a, std::int32_t b) {
    return std::int32_t(std::uint32_t(divide_signed<4>(std::uint32_t(a), std::uint32_t(b))));
}
inline std::int32_t wrap_add(std::int32_t a, std::int32_t b) { return std::int32_t(std::uint32_t(a) + std::uint32_t(b)); }
inline std::int32_t wrap_sub(std::int32_t a, std::int32_t b) { return std::int32_t(std::uint32_t(a) - std::uint32_t(b)); }
inline std::int32_t wrap_mul(std::int32_t a, std::int32_t b) { return std::int32_t(std::uint32_t(a) * std::uint32_t(b)); }
}
