#pragma once
// What the candidate scan's files share (shortlist_manager_candidates*.cpp):
// one invocation's state (Scan), the records it reads and the original's
// stack locals. The scan keeps most of its working values in non-volatile
// registers, as the original does: every routine it calls saves them into
// the stack image, so they are written where the original writes them.
#include "shortlist_manager_candidates.hpp"
#include "shortlist_manager_detail.hpp"
#include "shortlist_manager_random.hpp"
#include <bit>

namespace cm::aot::shortlist_manager::candidates {
using namespace detail;

constexpr std::uint32_t entry = 0x1035e9f0;

// A local the original keeps in non-volatile register R; unlike Kept it is
// not written at construction (the register holds the caller's value until
// the original first sets it).
template <unsigned R>
class Local {
    static_assert(R >= 13 && R <= 31, "Local: not a non-volatile register");

public:
    explicit Local(Cpu& c) : c(c) {}
    Local(const Local&) = delete;
    Local& operator=(std::uint32_t value) {
        c.set(R, value);
        return *this;
    }
    Local& operator=(const Local& other) { return *this = std::uint32_t(other); }
    operator std::uint32_t() const { return c.r(R); }
    std::int32_t value() const { return std::int32_t(c.r(R)); }

private:
    Cpu& c;
};
// The same for f29-f31 (the scan's prologue saves them; callees that use
// them save them again into the stack image).
template <unsigned N>
class FloatLocal {
public:
    explicit FloatLocal(Cpu& c) : c(c) {}
    FloatLocal(const FloatLocal&) = delete;
    FloatLocal& operator=(double value) {
        c.set_f(N, value);
        return *this;
    }
    operator double() const { return c.f(N); }

private:
    Cpu& c;
};

// TOC slots only the scan reads (names from use, inferred).
namespace slot {
using namespace detail::slot;
constexpr std::int32_t current_contract = -0x7b78;  // uint32: the last primary contract looked up (a scratch global)
constexpr std::int32_t person_meta = -0x7b9c;       // 100-byte records per person id; +0xc: two status nibbles
constexpr std::int32_t finances = -0x7ab8;          // the object 0x10103510 maps a club to its finance record by
constexpr std::int32_t humans = -0x7a54;            // the human managers' table
constexpr std::int32_t transfers = -0x7b40;         // the transfer manager (owns the shortlist manager)
constexpr std::int32_t injuries = -0x7ba0;          // 0x1018c340's object: a person's current injury
constexpr std::int32_t interest = -0x7b94;          // 0x10698bb0's object: where a pick's interest is registered
constexpr std::int32_t club_relations = -0x7bf0;    // 0x102cdbf0's object
constexpr std::int32_t negotiations = -0x7b1c;      // 0x1030fbf0's object
constexpr std::int32_t listings = -0x79f0;          // 0x101b1620's and 0x101ad080's object (may be null)
constexpr std::int32_t excluded_nation = -0x7588;   // a nation whose players are never scanned
// Nations whose clubs get the small sampling divisor (with the four
// regional ones of detail::slot), and those whose finance class matters.
constexpr std::int32_t small_divisor_nation = -0x7a28;
constexpr std::int32_t finance_nation_1 = -0x7a14, finance_nation_2 = -0x7a10, finance_nation_3 = -0x7af4,
                       finance_nation_4 = -0x7a00, finance_nation_5 = -0x7a18;
}

// The double pool's values the scan reads (slot::constants + offset; the
// bit patterns are the pool's).
namespace k {
using detail::k::signed_bias;
constexpr double of(std::uint64_t bits) { return std::bit_cast<double>(bits); }
constexpr double u16_max = of(0x40efffe000000000), hundred = of(0x4059000000000000), tenth = of(0x3fb999999999999a),
                 half = of(0x3fe0000000000000), one_point_three = of(0x3ff4cccccccccccd),
                 three_fifteen = of(0x4009333333333333), one = of(0x3ff0000000000000),
                 three_quarters = of(0x3fe8000000000000), one_quarter_more = of(0x3ff4000000000000),
                 reputation_1050 = of(0x4090680000000000), reputation_1125 = of(0x4091940000000000),
                 reputation_1175 = of(0x40925c0000000000), reputation_1250 = of(0x4093880000000000),
                 one_point_oh_five = of(0x3ff0cccccccccccd), bid_cap = of(0x414312d000000000),
                 thirteen_thousand = of(0x40c9640000000000), two_thirds = of(0x3fe570a3d70a3d71),
                 three = of(0x4008000000000000), hundredth = of(0x3f847ae147ae147b), ten = of(0x4024000000000000),
                 zero = of(0x0000000000000000), three_and_half = of(0x400c000000000000),
                 nine_tenths = of(0x3feccccccccccccd), one_point_two = of(0x3ff3333333333333),
                 one_and_half = of(0x3ff8000000000000), six_tenths = of(0x3fe3333333333333),
                 quarter_billion = of(0x41adcd6500000000);
// singles +0x08 (lfs): 0.9 as a single.
constexpr double nine_tenths_single = double(std::bit_cast<float>(0x3f666666u));
}

// The original's stack locals (offsets from the scan's stack pointer).
namespace local {
constexpr std::int32_t offer_cap = 0x40;      // int32 written by 0x1036fbf0 (0: none)
constexpr std::int32_t score = 0x44;          // int32: the candidate's score (0x10361490 or 0x101ad080)
constexpr std::int32_t ninth_argument = 0x38;  // 0x1036f170's ninth argument (its stack word)
constexpr std::int32_t joined_cutoff = 0x48, joined_cutoff_result = 0x60;  // today - 200 days
constexpr std::int32_t window_end = 0x50, window_end_result = 0x68;        // calendar end - 28 days
constexpr std::int32_t window_start = 0x58, window_start_result = 0x70;    // calendar start - 28 days
constexpr std::int32_t quote = 0x78;          // the quote object 0x103721f0 builds (100 bytes)
constexpr std::int32_t fee_floor = 0x10c;     // int32: the row's budget, at least 1
constexpr std::int32_t minimum = 0x110;       // int32: the lowest score the scan takes
constexpr std::int32_t chosen = 0x114;        // uint32: the best candidate's person (0: none)
constexpr std::int32_t divisor = 0x118;       // int16: the sampling divisor
constexpr std::int32_t in_window = 0x11a;     // int8: today is in the region's window (inferred)
constexpr std::int32_t human_talks = 0x11b;   // int8: 0x1030fbf0's answer for a human club
constexpr std::int32_t count = 0x11c;         // int32: the current category's entries
constexpr std::int32_t list = 0x120;          // uint32: the current category's list
constexpr std::int32_t category = 0x124;      // int8: the current category
constexpr std::int32_t index = 0x128;         // int32: the entry index in the list
constexpr std::int32_t category_offset = 0x12c;  // int32: category x 4
constexpr std::int32_t entry_offset = 0x130;  // int32: index x 12
constexpr std::int32_t limit = 0x134;         // int32: 0x1036f6f0's limit (-1: none)
constexpr std::int32_t lists_offset = 0x138;  // int32: 64 for the relaxed pass's second set of lists
constexpr std::int32_t last_category = 0x13c;  // int32
constexpr std::int32_t excluded = 0x1d4;      // the caller's parameter word: the excluded person (r6)
}

// The records the scan reads are shortlist_manager_records.hpp's.

// One invocation of the scan (0x1035e9f0): the original's registers r13-r31
// and f29-f31 by name, its frame and the calls it makes.
struct Scan {
    Scan(Cpu& c, Frame& frame, std::uint32_t row, std::uint32_t club)
        : c(c), frame(frame), program(c), fpu(c), manager(c), row_address(c), club_address(c), r13(c), r14(c),
          r18(c), player_row(c), r20(c), r21(c), person(c), bid_capped(c), best(c), price_high(c),
          contracts_slot(c), constants(c), salt_slot(c), table(c), end_slot(c), cursor_slot(c), f29(c), f30(c),
          f31(c), row{c, row}, club{c, club} {}

    Cpu& c;
    Frame& frame;
    Program program;
    Fpu fpu;
    // r15-r17: the arguments. r13 and r14 hold several values in turn.
    Local<15> manager;
    Local<16> row_address;
    Local<17> club_address;
    Local<13> r13;
    Local<14> r14;
    Local<18> r18;
    Local<19> player_row;   // the row asks for a player (kind 11); first 0x1014c5f0's answer
    // The candidate's price as an int64 pair (r21:r20); r20 is a scratch
    // register in the set-up.
    Local<20> r20;
    Local<21> r21;
    Local<22> person;
    Local<23> bid_capped;   // 0x1036fbf0 capped the offer (inferred)
    Local<24> best;         // the minimum score, then the best score so far
    Local<25> price_high;   // the high word of the asking price
    Local<26> contracts_slot;
    Local<27> constants;
    Local<28> salt_slot;
    Local<29> table;
    Local<30> end_slot;
    Local<31> cursor_slot;
    FloatLocal<29> f29;
    FloatLocal<30> f30;
    FloatLocal<31> f31;  // the club's reputation scale, at least 1
    Row row;
    Club club;

    std::uint32_t at(std::int32_t offset) const { return frame.at(offset); }
    Person candidate() const { return {c, person}; }
    // An int32 in an int-to-double temporary at `offset`, as a double.
    double integer(std::int32_t offset, std::int32_t value) {
        return fpu.fsub(stacked_integer(c, at(offset), signed_word(value)), k::signed_bias);
    }
    double integer_single(std::int32_t offset, std::int32_t value) {
        return fpu.fsubs(stacked_integer(c, at(offset), signed_word(value)), k::signed_bias);
    }
    // fctiwz into the stack temporary at `offset`; returns the integer (its low word).
    std::int32_t truncated(std::int32_t offset, double value) {
        c.put64(at(offset), fpu.fctiwz(0, value));
        return c.i32(at(offset) + 4);
    }
    // fcmpo cr0: the field (LT 8, GT 4, EQ 2, unordered 1).
    std::uint8_t fcompare(double a, double b) {
        fpu.fcmp(0, a, b);
        return c.cr(0);
    }
    // fcmpo then `cror eq,gt,eq`: a >= b.
    bool at_least(double a, double b) {
        auto field = fcompare(a, b);
        if (field & 4) field |= 2;
        c.set_cr(0, field);
        return field & 2;
    }

    // The game's random table (shortlist_manager_random.hpp), its variables'
    // addresses kept in r28-r31; `back` is the address after the bl to rand.
    RandomTable random() const { return {c, cursor_slot, end_slot, table, salt_slot}; }
    void advance(std::uint32_t back) const { random().advance(back); }
    std::uint32_t cursor() const { return random().position(); }
    // A draw: the cursor's uint16 plus the salt (after advance).
    std::int32_t draw() const { return std::int32_t(random().value()); }
    // The original's inlined random_below (0x10356e40) for n = r13, into
    // r14. Temporaries at `base` (n), +8 (the draw's remainder) and +0x10
    // (the scaled share). The two copies the original inlines leave
    // different scratch registers (WideDraw::leave_a, leave_b).
    enum class Copy { a, b };
    std::int32_t random_below(std::int32_t base, Copy copy, std::uint32_t rand_back, std::uint32_t division,
                              std::uint32_t call_back);
    // 0x10082f20 (the person's primary contract), published in
    // slot::current_contract; returns the contract's club (0 without one).
    // r4 is left holding the slot's address, as the original leaves it.
    std::uint32_t contract_club(std::uint32_t person_address, std::uint32_t back);
    std::uint32_t club_of(std::uint32_t contract) const {
        return c.u32(c.toc(slot::clubs)) + c.u32(contract + 4) * Club::size;
    }
    // 0x1014c2e0(club, a, 1, 0, 0): whether a club is in a human's hands.
    bool human(std::uint32_t club_address, std::uint32_t mode, std::uint32_t back) {
        return nonzero(c, std::int8_t(program.call(0x1014c2e0, back, {club_address, mode, 1, 0, 0})));
    }
    std::uint8_t meta(std::int32_t id) const { return c.u8(c.u32(c.toc(slot::person_meta)) + std::uint32_t(id) * 100 + 0xc); }

    std::int64_t price() const { return std::int64_t(std::uint64_t(r21) << 32 | r20); }
    void set_price(std::int32_t value) {
        r20 = std::uint32_t(value);
        r21 = std::uint32_t(value >> 31);
    }

    // The scan's parts, in the original's order (shortlist_manager_candidates.cpp).
    void run();
    void check_window(std::uint32_t region);
    void set_divisor();
    bool open();
    bool set_minimum();
    void set_categories();
    void walk();
    void take(std::uint32_t chosen);
    void conclude_empty();
    // One list entry: the filters (shortlist_manager_candidates_filter.cpp)...
    void consider(std::uint32_t entry_address);
    bool eligible(std::uint32_t entry_address);
    bool player_checks(std::uint32_t entry_address);
    bool staff_checks();
    bool common_checks();
    bool sampled();
    bool local_rules(std::int32_t sum);
    bool foreign_draw(bool youth);
    // ...and the valuation (shortlist_manager_candidates_value.cpp).
    void value();
    bool negotiate();
    void adjust_score();
    void injury_penalty(std::uint32_t back, std::int32_t threshold, std::uint32_t weight);
    bool evaluate();
    void approach();
    std::int32_t random_below_share();
    bool affordable();
    void surprise();
};
}
