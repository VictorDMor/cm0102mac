#include "shortlist_manager_detail.hpp"
#include "shortlist_manager_random.hpp"
#include <array>
#include <utility>

// The daily interest distribution (0x10363ca0) and the two registrations of
// a club's interest in a person (0x10363f60, from the distribution;
// 0x10364d00, for a requested move). Field and routine names come from use;
// "(inferred)" marks meanings not confirmed elsewhere.
namespace cm::aot::shortlist_manager {
using namespace detail;
namespace {
std::int32_t signed_value(std::uint32_t value) { return std::int32_t(value); }

// The game's slots these routines read besides detail::slot.
namespace slot_recruitment {
constexpr std::int32_t staff_extras = -0x7b9c;    // 100-byte records per person id (+0xc: two status fields)
constexpr std::int32_t last_contract = -0x7b78;   // word: the contract the last lookup found (shared scratch)
constexpr std::int32_t club_order = -0x765c;      // int32 club ids, slot::club_count of them
constexpr std::int32_t home_nation_id = -0x7588;  // int32 compared with a nation's +0x68 (meaning not established)
constexpr std::int32_t transfers = -0x7b40;       // the transfer object (0x103f43b0, 0x103f83c0, 0x103ecef0)
constexpr std::int32_t work_rules = -0x7a54;      // 0x10147e70's and 0x10149df0's object (inferred: work permits)
constexpr std::int32_t permits = -0x7b1c;         // 0x1030fbf0's object (inferred: work permits)
constexpr std::int32_t finances = -0x7ab8;        // club finances (0x10103510)
constexpr std::int32_t fitness = -0x7ba0;         // 0x1018c340's object (a record with an int16 at +0x34)
}

// The pool's doubles these routines load (slot::constants + offset).
namespace constant {
constexpr double score_floor = 0.1;          // +0x048
constexpr double budget_share = 0.2;         // +0x058
constexpr double value_margin = 1.3;         // +0x070
constexpr double registration_score = 3.15;  // +0x078
constexpr double one = 1.0;                  // +0x080
constexpr double funds_share = 0.8;          // +0x0f8
constexpr double reputation_unit = 1250.0;   // +0x118
constexpr double weight_base = 0.67;         // +0x138
constexpr double weight_ceiling = 3.0;       // +0x140
constexpr double weight_floor = 0.01;        // +0x148
constexpr double risk_numerator = 3.5;       // +0x160
constexpr double risk_floor = 0.9;           // +0x168
constexpr double risk_margin = 1.2;          // +0x170
constexpr double requested_score = 2.75;     // +0x2c0
constexpr double budget_curve = 0.01536;     // +0x2c8
constexpr double risk_floor_single = double(0.9f);  // singles +0x08 (lfs)
}

std::uint32_t club_of(Cpu& c, std::int32_t id) { return c.u32(c.toc(slot::clubs)) + std::uint32_t(id) * Club::size; }

// Three small routines of the rest of the program that the registrations
// call on almost every call (1.76M registrations a day, three-quarters of
// which end after them). Each is reproduced here as its generated code runs
// it: the registers and cr0 it leaves, and the stack image of its frames
// (saved registers, LR, back chains), so the hot path makes no dispatch for
// them. The routine checker compares the result with the original calls.

// 0x10082f20 (a leaf, no frame): the contract of `person` from the contracts
// object at `contracts` (its +0 array, 100-byte records), through the
// person's extra record's first word; 0 for no person, a non-player id or a
// negative index. Leaves r4-r6 and cr0 as it does.
std::uint32_t contract_of(Cpu& c, std::uint32_t contracts, std::uint32_t person) {
    c.set(4, person);
    if (compare_logical(c, person, 0) == 0) return result(c, 0);
    const std::uint32_t count = c.toc(slot::person_count);
    const std::int32_t id = c.i32(person);
    leave(c, {{5, count}, {6, std::uint32_t(id)}});
    if (compare(c, id, c.i32(count)) >= 0) return result(c, 0);
    const std::uint32_t extras = c.u32(c.toc(slot_recruitment::staff_extras));
    c.set(4, extras);
    const std::int32_t index = c.i32(extras + std::uint32_t(id) * 100);
    if (compare(c, index, 0) < 0) return result(c, 0);
    return result(c, c.u32(contracts) + std::uint32_t(index) * 100);
}
// 0x103721f0: constructs the contract draft at `draft` (two dates set to
// day 0x28 of 1972 by the leaf 0x10715880, the rest cleared or -1). Its
// frame: r31 saved below sp, LR at sp+8, a 0x40-byte frame.
void construct_draft(Cpu& c, std::uint32_t draft, std::uint32_t back) {
    const std::uint32_t sp = c.sp();
    c.put32(sp - 4, c.r(31));
    c.put32(sp + 8, back);
    c.put32(sp - 0x40, sp);
    for (const std::uint32_t date : {draft + 0x28, draft + 0x30}) {
        c.put16(date, 0x28);
        c.put16(date + 2, 0x7b4);
        c.put8(date + 4, 1);
    }
    c.put32(draft + 4, std::uint32_t(-1));
    c.put8(draft + 0x54, 0);
    c.put32(draft + 0x58, std::uint32_t(-1));
    c.put16(draft + 0x42, 0);
    c.put8(draft + 0x5c, 0);
    c.put32(draft + 0x48, 0);
    c.put32(draft + 0x50, 0);
    c.put32(draft + 8, 0);
    leave(c, {{3, draft}, {4, std::uint32_t(-1)}});
}
// 0x1014c2e0(club, r4, r5, 0, r7): whether a human manager controls the
// club. Its plain path is reproduced: a club without a manager (+0x90) or
// with one below the last 16 staff ids (the human managers), and not among
// the last nation-count clubs (0x100ce250's verdict, its branch-free signed
// compare); it returns 0 whatever r4, r5 and r7 are. Any other club takes
// the real call.
std::uint32_t human_controlled(Cpu& c, Program& program, std::uint32_t club, std::uint32_t r4, std::uint32_t r5,
                               std::uint32_t r7, std::uint32_t back) {
    bool plain = club != 0;
    std::int32_t id = 0;
    if (plain) {
        const std::uint32_t manager = c.u32(club + 0x90);
        plain = manager == 0 || c.i32(manager) < c.i32(c.toc(slot::person_count)) - 16;
    }
    if (plain) {
        id = c.i32(club);
        const std::int32_t limit = c.i32(c.toc(slot::club_count)) - c.i32(c.toc(-0x7b04));
        const std::uint32_t carry = std::uint32_t(id) >= std::uint32_t(limit);
        plain = std::int8_t(std::uint32_t(id >> 31) + (std::uint32_t(limit) >> 31) + carry) == 0;
    }
    if (!plain) return program.call(0x1014c2e0, back, {club, r4, r5, 0, r7});
    // 0x1014c2e0's frame (r24-r31 saved, 0x80 bytes), then 0x100ce250's
    // (r31 and r30 saved, 0x70 bytes) below it.
    const std::uint32_t sp = c.sp(), inner = sp - 0x80;
    for (unsigned n = 24; n < 32; ++n) c.put32(sp - 0x20 + (n - 24) * 4, c.r(n));
    c.put32(sp + 8, back);
    c.put32(inner, sp);
    c.put32(inner - 4, c.toc(-0x7a54));
    c.put32(inner - 8, r7);
    c.put32(inner + 8, 0x1014c44c);
    c.put32(inner - 0x70, inner);
    leave(c, {{4, std::uint32_t(id >> 31)}, {5, c.toc(-0x7b04)}, {6, std::uint32_t(id)}, {7, r7}});
    compare(c, 0);
    return result(c, 0);
}

// The roles a person's profile gives (0x10117490) and the roles they imply,
// added as the original does: 2 -> 4, 8 -> 16, 32 -> 16, 128 -> 256,
// 2048 -> 1024. Each addition sign-extends the halfword; the value stays as
// returned when nothing is added.
std::uint32_t with_implied_roles(Cpu& c, std::uint32_t roles) {
    static constexpr std::array<std::pair<std::uint32_t, std::uint32_t>, 5> implied{
        {{2, 4}, {8, 0x10}, {0x20, 0x10}, {0x80, 0x100}, {0x800, 0x400}}};
    for (const auto& [test, add] : implied)
        if (nonzero(c, signed_value(roles & test))) roles = extended(std::int16_t(roles | add));
    return roles;
}

// Whether the date at `first` comes before the date at `second` (year, then
// day of the year: the original's branch-free signed compare). Leaves r4 as
// the original.
bool earlier(Cpu& c, std::uint32_t first, std::uint32_t second) {
    const std::uint32_t year = c.u16(first + 2), other_year = c.u16(second + 2);
    c.set(4, year);
    if (compare_logical(c, year, other_year) != 0) return nonzero(c, year < other_year);
    const std::int32_t day = c.i16(first), other_day = c.i16(second);
    c.set(4, std::uint32_t(other_day));
    const std::uint32_t mixed = std::uint32_t(other_day ^ day);
    const std::uint32_t difference = std::uint32_t(signed_value(mixed) >> 1) - (mixed & std::uint32_t(other_day));
    return nonzero(c, std::int8_t(difference >> 31));
}

// One draw from the game's random table: advance the cursor (back to the
// table's start with a new salt from rand() when it passes the end) and take
// the value plus the salt modulo `range`, divided as the compiler does
// (mulhw by 0x10624dd3, `shift`, plus the sign bit). `sum` and `sign` are
// what the original leaves in r7 and r6.
struct Draw {
    std::int32_t remainder;
    std::uint32_t sum, sign;
};
Draw draw(Cpu& c, std::uint32_t refill_back, std::int32_t shift, std::int32_t range) {
    const RandomTable random{c};
    random.advance(refill_back, {.previous_in_r4 = true});
    const std::uint32_t sum = random.value();
    const std::int32_t high = multiply_high(0x10624dd3, signed_value(sum)) >> shift;
    const std::uint32_t sign = std::uint32_t(high) >> 31;
    return {signed_value(sum - (std::uint32_t(high) + sign) * std::uint32_t(range)), sum, sign};
}

// The int-to-double idiom (detail::stacked_integer) and the bias's fsub.
double as_double(Cpu& c, Fpu& fpu, std::uint32_t at, std::int32_t value) {
    return fpu.fsub(stacked_integer(c, at, signed_word(value)), k::signed_bias);
}
// The same with fsubs (rounded to single).
double as_single(Cpu& c, Fpu& fpu, std::uint32_t at, std::int32_t value) {
    return fpu.fsubs(stacked_integer(c, at, signed_word(value)), k::signed_bias);
}

// An assessment's weight: `ratio` held within [0.01, 3.0], then above 1.0
// eased to 0.67 + w / 3.
double weight(Fpu& fpu, double ratio) {
    double w = constant::weight_floor;
    fpu.fcmp(0, w, ratio);
    if (!(w > ratio)) {
        w = constant::weight_ceiling;
        fpu.fcmp(0, w, ratio);
        if (!(w < ratio)) w = ratio;
    }
    fpu.fcmp(0, w, constant::one);
    if (w > constant::one) w = fpu.fadd(constant::weight_base, fpu.fdiv(w, constant::weight_ceiling));
    return w;
}
// The int at `local` times `w`, truncated (fctiwz through the stack: the
// int's temporary at `temporary`, the result's 8 bytes at `result_at`).
std::int32_t scale_local(Cpu& c, Fpu& fpu, std::uint32_t local, std::uint32_t temporary, std::uint32_t result_at, double w) {
    const double scaled = fpu.fmul(as_double(c, fpu, temporary, c.i32(local)), w);
    c.set_f(1, k::signed_bias);  // the original loads the bias into f1 (lfd f1)
    const std::uint64_t bits = fpu.fctiwz(0, scaled);
    c.put64(result_at, bits);
    c.put32(local, std::uint32_t(bits));
    return std::int32_t(std::uint32_t(bits));
}
// The int at `local` loses the square of the excess over 15 of the int16 at
// +0x34 of the person's record from 0x1018c340 (inferred: an injury or
// fatigue level).
void fitness_penalty(Cpu& c, Program& program, std::uint32_t person, std::uint32_t local, std::uint32_t back) {
    const std::uint32_t record = program.call(0x1018c340, back, {c.u32(c.toc(slot_recruitment::fitness)), person});
    if (!nonzero_logical(c, record)) return;
    const std::int32_t level = c.i16(record + 0x34);
    if (compare(c, level, 15) <= 0) return;
    const std::uint32_t excess = std::uint32_t(level - 15);
    c.put32(local, c.u32(local) - excess * excess);
}

// The club `contract` belongs to (its +4 club id), or 0 without one.
std::uint32_t contract_club(Cpu& c, std::uint32_t contract) {
    return nonzero_logical(c, contract) ? club_of(c, c.i32(contract + 4)) : 0;
}
// The contract of `person` (0x10082f20), published in the shared scratch slot.
std::uint32_t published_contract(Cpu& c, std::uint32_t person) {
    const std::uint32_t contract = contract_of(c, c.u32(c.toc(slot::contracts)), person);
    c.put32(c.toc(slot_recruitment::last_contract), contract);
    return contract;
}
// What a move of `person` from `from` to `to` is worth (inferred): the form
// for a human-controlled club (0x10089a40) or a computer's (0x10085860).
// `backs`: after the control query, after either call.
std::uint32_t move_value(Cpu& c, Program& program, std::uint32_t from, std::uint32_t to, std::uint32_t person,
                         const std::array<std::uint32_t, 3>& backs) {
    const std::uint32_t contracts = c.u32(c.toc(slot::contracts));
    if (nonzero(c, std::int8_t(human_controlled(c, program, from, 0, 1, 0, backs[0]))))
        return program.call(0x10089a40, backs[1], {contracts, from, to, person});
    return program.call(0x10085860, backs[2], {contracts, from, to, person, 0, 1, 1});
}
// The person's own club caps the assessment when its finances' flag +0x165
// is 1: a human-controlled club by 0x10084be0 (asked again when it is the
// smaller), a computer's by the person's value (inferred). `backs`: after
// each call in the original's order.
std::uint32_t own_club_cap(Cpu& c, Program& program, std::uint32_t staff, std::uint32_t own_club, std::uint32_t assessed,
                           const std::array<std::uint32_t, 5>& backs) {
    if (!nonzero_logical(c, own_club)) return assessed;
    const std::uint32_t finances = c.toc(slot_recruitment::finances);
    if (!nonzero_logical(c, program.call(0x10103510, backs[0], {c.u32(finances), own_club}))) return assessed;
    const std::uint32_t record = program.call(0x10103510, backs[1], {c.u32(finances), own_club});
    if (compare(c, c.u8(record + 0x165), 1) != 0) return assessed;
    const std::uint32_t contracts = c.toc(slot::contracts);
    if (nonzero(c, std::int8_t(human_controlled(c, program, own_club, 0, 1, 0, backs[2])))) {
        const std::uint32_t cap = program.call(0x10084be0, backs[3], {c.u32(contracts), staff, own_club, 0, std::uint32_t(-1), 0});
        if (compare(c, signed_value(assessed), signed_value(cap)) < 0) return assessed;
        return program.call(0x10084be0, backs[4], {c.u32(contracts), staff, own_club, 0, std::uint32_t(-1), 0});
    }
    const std::int32_t value = Person{c, staff}.value();
    return compare(c, signed_value(assessed), value) < 0 ? assessed : std::uint32_t(value);
}
// 0x10369080 (the interest group's score of the person for the club), its f1.
double interest_score(Cpu& c, Program& program, std::uint32_t manager, std::uint32_t staff, std::uint32_t club,
                      std::uint32_t flag, std::uint32_t category, std::uint32_t back) {
    program.call(0x10369080, back, {manager, staff, club, flag, 0, category, 0});
    return c.f(1);
}

// What both registrations check before the rows: the club's entry recruits
// (state 1, counter below 100); an existing link between the person and the
// club (bsearch over the manager's pairs, key at `key`) ends it with 1.
enum class Before { go, refuse, linked };
Before entry_and_link(Cpu& c, Program& program, const Manager& manager, const ClubEntry& entry, std::uint32_t staff,
                      std::uint32_t club, std::uint32_t key, std::uint32_t back) {
    if (compare(c, entry.status(), 1) != 0) return Before::refuse;
    if (compare_logical(c, entry.changes(), 100) >= 0) return Before::refuse;
    bool linked = false;
    if (nonzero_logical(c, manager.links_by_staff())) {
        c.put32(key, std::uint32_t(Person{c, staff}.id().get()));
        c.put32(key + 4, nonzero_logical(c, club) ? std::uint32_t(Club{c, club}.id().get()) : std::uint32_t(-1));
        linked = nonzero_logical(c, program.bsearch(key, manager.links_by_staff(), manager.count_by_staff(), 8,
                                                    c.toc(slot::compare_10366410_tv), back));
    }
    return nonzero(c, linked) ? Before::linked : Before::go;
}
// A row that can take the person, by its own fields: open (kind 11, not
// closed, not taken) and wanting roles the person has (the roles in
// `roles`); `needs_roles` also rejects a row wanting none (the
// distribution's form).
bool row_open(Cpu& c, const Row& row, std::int32_t roles, bool needs_roles) {
    if (nonzero(c, signed_value(row.loan()))) return false;
    if (compare(c, row.kind(), 11) != 0) return false;
    if (compare(c, row.approach(), -1) != 0) return false;
    const std::int32_t wanted = row.positions();
    if (needs_roles && !nonzero(c, wanted)) return false;
    return compare(c, wanted, wanted & roles) == 0;
}
// A row with its person and other id equal (and set) is closed to new
// people; otherwise it opens once its date has passed (the date copied to
// the stack at `local`, as the original).
bool row_due(Cpu& c, const Row& row, std::uint32_t local) {
    const std::int32_t person = row.target();
    if (compare(c, person, row.previous()) == 0 && compare(c, person, -1) != 0) return false;
    c.put32(local, c.u32(row.date().address));
    c.put32(local + 4, c.u32(row.date().address + 4));
    return earlier(c, local, c.toc(slot::today));
}
// Hot prospects (category 5, inferred) must be 24 or younger, and a
// foreign one (neither the person's nation nor their club's is the club's)
// needs the profile's reputation at `threshold` and 0x100cdef0's verdict at
// least a draw of `range` plus `floor`. `waived`, when given, skips the
// foreign check (the distribution's work-permit flag). The draw is kept in
// non-volatile register R as the original keeps it.
struct ProspectRule {
    std::int32_t threshold, shift, range, floor;
    std::uint32_t refill_back, back;
};
template <unsigned R>
bool prospect_allowed(Cpu& c, Program& program, const Row& row, const Person& staff, const Club& club,
                      const std::uint32_t* waived, const ProspectRule& rule) {
    const std::uint32_t category = row.category();
    if (compare_logical(c, category, 5) == 0 && compare(c, staff.age(), 24) > 0) return false;
    if (compare_logical(c, category, 5) != 0) return true;
    if (waived && nonzero(c, std::int8_t(*waived))) return true;
    const std::uint32_t nation = club.nation();
    if (compare_logical(c, nation, staff.nation()) == 0) return true;
    const std::uint32_t own = staff.club();
    c.set(4, own);
    if (nonzero_logical(c, own) && compare_logical(c, Club{c, own}.nation(), nation) == 0) return true;
    if (compare(c, c.i16(staff.player() + 0xa), rule.threshold) < 0) return false;
    const Draw drawn = draw(c, rule.refill_back, rule.shift, rule.range);
    Kept<R> kept{c, std::uint32_t(drawn.remainder)};
    const std::int32_t verdict =
        std::int16_t(program.call(0x100cdef0, rule.back, {staff.address, club.manager(), 0, drawn.sign, drawn.sum}));
    return compare(c, verdict, drawn.remainder + rule.floor) >= 0;
}
// A club the transfer rules restrict (0x103ecef0) only fills its first
// three categories.
bool category_allowed(Cpu& c, Program& program, const Row& row, std::uint32_t staff, std::uint32_t club, std::uint32_t back) {
    if (!nonzero(c, std::int8_t(program.call(0x103ecef0, back, {c.u32(c.toc(slot_recruitment::transfers)), staff, club, 1}))))
        return true;
    const std::uint32_t category = row.category();
    return compare_logical(c, category, 1) == 0 || compare_logical(c, category, 2) == 0 ||
           compare_logical(c, category, 3) == 0;
}
// The two registrations' weight of an assessment gap: 1 - gap / divisor.
double gap_ratio(Fpu& fpu, double gap, double divisor) { return fpu.fsub(constant::one, fpu.fdiv(gap, divisor)); }
// The requested registration's divisor: the club's reputation squared over
// ten, divided as the compiler does (mulhw by 0x66666667, >> 2, plus the
// sign bit). Returns the quotient; `sign` is what the original leaves in r3.
std::int32_t reputation_divisor(const Club& club) {
    const std::int32_t reputation = club.reputation();
    const std::int32_t square = std::int32_t(std::uint32_t(reputation) * std::uint32_t(reputation));
    const std::int32_t high = multiply_high(0x66666667, square) >> 2;
    return high + std::int32_t(std::uint32_t(high) >> 31);
}
}

std::uint32_t register_club_interest(Cpu& c, std::uint32_t manager_address, std::uint32_t staff_address,
                                     std::uint32_t club_address, std::uint32_t roles) {
    Frame frame(c, 0x10363f60, 0x150, {.first_gpr = 13, .gpr_offset = -0x5c, .first_fpr = 31});
    keep(c, {{25, manager_address}, {15, c.toc(slot_recruitment::permits)}, {26, staff_address},
             {14, c.toc(slot_recruitment::work_rules)}, {27, club_address}, {20, c.toc(slot::contracts)},
             {21, c.toc(slot::constants)}, {22, c.toc(slot::random_cursor)}});
    c.put16(frame.at(0x176), roles);
    Program program{c};
    const Manager manager{c, manager_address};
    const Person staff{c, staff_address};
    const Club club{c, club_address};
    // A contract draft on the stack (0x103721f0), used by the final risk check.
    const std::uint32_t draft = frame.at(0x5c);
    construct_draft(c, draft, 0x10363fa0);
    // Whether the person may work in the club's nation (inferred); it waives
    // the foreign-prospect check.
    Kept<13> permit{c, 0};
    if (nonzero(c, std::int8_t(human_controlled(c, program, club_address, 0, 1, 0, 0x10363fbc)))) return 0;
    // A club under a human manager: the nation's work rules (inferred).
    if (nonzero(c, std::int8_t(human_controlled(c, program, club_address, 1, 1, 0, 0x10363fe4)))) {
        const std::uint32_t rules = c.toc(slot_recruitment::work_rules), permits = c.toc(slot_recruitment::permits);
        const std::uint32_t nation = program.call(0x100c96e0, 0x10363ff4, {club_address});
        if (nonzero(c, std::int8_t(program.call(0x10147e70, 0x10364000, {c.u32(rules), nation})))) {
            const std::uint32_t nation_again = program.call(0x100c96e0, 0x10364010, {club_address});
            if (nonzero(c, std::int8_t(program.call(0x1030fbf0, 0x10364020, {c.u32(permits), staff_address, nation_again}))))
                permit = 1;
            const std::uint32_t nation_rule = program.call(0x100c96e0, 0x10364034, {club_address});
            const std::int32_t rule = std::int8_t(program.call(0x10149df0, 0x10364044, {c.u32(rules), 4, nation_rule}));
            if (compare(c, rule, 2) == 0) {
                const std::uint32_t nation_permit = program.call(0x100c96e0, 0x10364070, {club_address});
                if (!nonzero(c, std::int8_t(program.call(0x1030fbf0, 0x10364080, {c.u32(permits), staff_address, nation_permit}))))
                    return 0;
            } else if (rule < 2 && compare(c, rule, 1) >= 0) {
                return 0;
            }
        }
    }
    // r15: the club's entry, then each of its rows.
    const ClubEntry entry{c, manager.clubs() + std::uint32_t(club.id()) * 12};
    Kept<15> at{c, entry.address};
    c.set(5, manager.clubs());
    Kept<16> contract{c, contract_of(c, c.u32(c.toc(slot::contracts)), staff_address)};
    Kept<30> own_club{c, compare(c, signed_value(contract)) != 0 ? club_of(c, c.i32(contract + 4)) : 0};
    switch (entry_and_link(c, program, manager, entry, staff_address, club_address, frame.at(0x54), 0x10364140)) {
    case Before::refuse: return 0;
    case Before::linked: return 1;
    case Before::go: break;
    }
    const std::uint32_t found_club = contract_club(c, published_contract(c, staff_address));
    if (!nonzero(c, std::int8_t(program.call(0x1036f590, 0x103641ac, {manager_address, staff_address, found_club, club_address}))))
        return 0;
    if (!nonzero(c, std::int8_t(program.call(0x103f83c0, 0x103641d8, {c.u32(c.toc(slot_recruitment::transfers)),
                                                                      staff_address, club_address, 0, 0}))))
        return 0;
    const std::uint32_t finances = program.call(0x10103510, 0x103641f8, {c.u32(c.toc(slot_recruitment::finances)), club_address});
    if (!nonzero_logical(c, finances)) return 0;
    // The club's funds (0x100f5770's low word), none when negative (inferred).
    program.call(0x100f5770, 0x10364220, {finances, 0, 0, 0, 1, 0});
    Kept<14> funds{c, c.r(4)};
    if (compare(c, signed_value(funds)) < 0) funds = 0;
    // The club's scale: its reputation over 1,250, at least 1 (f31).
    Fpu fpu{c};
    const double reputation_d = as_double(c, fpu, frame.at(0xe0), club.reputation());
    c.set_f(1, reputation_d);  // the original computes it into f1
    const double reputation = fpu.fdiv(reputation_d, constant::reputation_unit);
    fpu.fcmp(0, reputation, constant::one);
    c.set_f(31, reputation > constant::one ? reputation : constant::one);

    // Locals the original keeps in registers across the rows (their values
    // reach the callees' frames).
    Kept<17> index{c, 0};
    Kept<18> previous{c, c.r(18)};  // the row's person, also the foreign draw
    Kept<19> offer{c, c.r(19)};
    Kept<23> policy{c, c.r(23)};    // 0x1036fbf0's verdict on the row (meaning not established)
    Kept<24> assessed{c, c.r(24)};
    Kept<29> divisor{c, c.r(29)};
    Kept<31> rating{c, c.r(31)};
    const std::uint32_t wanted_roles = c.u16(frame.at(0x176));
    at = entry.rows();
    for (; compare(c, signed_value(index), std::int32_t(Row::per_club)) < 0; index = index + 1, at = at + Row::size) {
        const Row row{c, at};
        // r28: the row's budget before this registration changed it, -1 none.
        Kept<28> budget_before{c, std::uint32_t(-1)};
        const auto restore_budget = [&] {
            if (compare(c, signed_value(budget_before), -1) != 0) row.budget() = signed_value(budget_before);
        };
        if (!row_open(c, row, std::int16_t(wanted_roles), true)) continue;
        policy = program.call(0x1036fbf0, 0x103642cc, {manager_address, at, contract, club_address, frame.at(0x40)});
        // A person worth over 1.3 times the row's budget is only for a club
        // below 7,000 reputation when their own club is a computer's, they
        // are worth less than the budget plus 1,500,000 and 0x100d3040 agrees.
        if (nonzero_logical(c, contract) && !nonzero(c, std::int8_t(policy)) && compare(c, club.reputation(), 7000) < 0) {
            c.set(4, signed_word(row.budget()));
            const double limit = fpu.fmul(constant::value_margin, as_double(c, fpu, frame.at(0xe0), row.budget()));
            c.set_f(1, limit);  // the original computes it into f1
            const double value = as_double(c, fpu, frame.at(0xd8), staff.value());
            fpu.fcmp(0, limit, value);
            if (limit < value) {
                if (!nonzero_logical(c, own_club)) continue;
                if (!nonzero(c, std::int8_t(human_controlled(c, program, own_club, 0, 1, 0, 0x10364354)))) continue;
                if (compare(c, signed_value(std::uint32_t(staff.value()) - 1500000), row.budget()) >= 0) continue;
                if (!nonzero(c, std::int8_t(program.call(0x100d3040, 0x10364380, {staff_address, 0})))) continue;
            }
        }
        if (!row_due(c, row, frame.at(0x4c))) continue;
        const std::uint32_t waived = permit;
        if (!prospect_allowed<18>(c, program, row, staff, club, &waived, {1000, 7, 2000, 1000, 0x10364494, 0x103644e4}))
            continue;
        if (!category_allowed(c, program, row, staff_address, club_address, 0x1036450c)) continue;
        // A budget at most a fifth of the funds becomes 0.01536 * rating^2
        // (single precision), at most 0.8 of the funds; r28 keeps the old one.
        c.set(4, 0x43300000);
        const std::int32_t budget = row.budget();
        c.set(5, std::uint32_t(budget));
        const double share = fpu.fmul(constant::budget_share, as_double(c, fpu, frame.at(0xe0), signed_value(funds)));
        const double current = as_double(c, fpu, frame.at(0xd8), budget);
        c.set_f(1, current);  // the original computes it into f1
        fpu.fcmp(0, current, share);
        if (current <= share && !nonzero(c, std::int8_t(policy))) {
            budget_before = std::uint32_t(budget);
            const std::int32_t wanted = row.level();
            const double curve = fpu.fmul(constant::budget_curve, as_single(c, fpu, frame.at(0xd8), wanted));
            const double rated = as_single(c, fpu, frame.at(0xe0), wanted);
            const double squared = fpu.fmuls(fpu.frsp(curve), rated);
            const double available = as_double(c, fpu, frame.at(0xc0), signed_value(funds));
            const std::uint64_t bits = fpu.fctiwz(2, squared);
            const double ceiling = fpu.fmul(constant::funds_share, available);
            c.set_f(1, ceiling);  // the original computes it into f1
            c.put64(frame.at(0xd0), bits);
            row.budget() = std::int32_t(std::uint32_t(bits));
            const double fresh = as_double(c, fpu, frame.at(0xc8), row.budget());
            fpu.fcmp(0, fresh, ceiling);
            if (fresh > ceiling) {
                const std::uint64_t capped = fpu.fctiwz(0, ceiling);
                c.put64(frame.at(0xc0), capped);
                row.budget() = std::int32_t(std::uint32_t(capped));
            }
        }
        const std::int32_t budget_now = row.budget();
        divisor = compare(c, budget_now, 1) > 0 ? std::uint32_t(budget_now) : 1;
        // The person the row holds, if any: their assessment (sp+0x48) by
        // the gap between their move's value and their assessment.
        const std::int32_t held = row.target();
        bool holds = false;
        if (compare(c, held, -1) != 0) {
            previous = c.u32(c.toc(slot::persons)) + std::uint32_t(held) * 0x70;
            holds = nonzero_logical(c, c.u32(previous + 0x14));
        }
        if (holds) {
            rating = std::uint32_t(row.level().get());
            assessed = program.call(0x10361490, 0x1036467c, {manager_address, club_address, previous, at, frame.at(0x48), rating, 1});
            offer = contract_club(c, published_contract(c, previous));
            std::uint32_t value = 0;
            if (nonzero_logical(c, offer)) value = move_value(c, program, offer, club_address, previous, {0x103646e0, 0x103646fc, 0x10364720});
            c.set(4, 0x43300000);
            const double scaled_divisor = fpu.fmul(c.f(31), as_double(c, fpu, frame.at(0xc8), signed_value(divisor)));
            const double gap = as_double(c, fpu, frame.at(0xc0), signed_value(value - assessed));
            const std::int32_t scaled =
                scale_local(c, fpu, frame.at(0x48), frame.at(0xd0), frame.at(0xd8), weight(fpu, gap_ratio(fpu, gap, scaled_divisor)));
            if (compare(c, scaled, row.level()) >= 0) continue;  // quirk: the budget stays changed
            c.put32(frame.at(0x48), compare(c, scaled, signed_value(rating)) > 0 ? std::uint32_t(scaled) : std::uint32_t(rating));
            fitness_penalty(c, program, previous, frame.at(0x48), 0x10364808);
        } else {
            row.target() = -1;
            previous = 0;
            rating = std::uint32_t(row.level().get());
            c.put32(frame.at(0x48), rating);
        }
        // The candidate's assessment (sp+0x44), capped by their own club,
        // by the gap to what their own club's move is worth.
        assessed = program.call(0x10361490, 0x10364868, {manager_address, club_address, staff_address, at, frame.at(0x44), rating, 1});
        assessed = own_club_cap(c, program, staff_address, own_club, assessed,
                                {0x10364884, 0x1036489c, 0x103648c0, 0x103648e4, 0x10364910});
        offer = nonzero_logical(c, own_club) && !nonzero(c, std::int8_t(policy))
                    ? move_value(c, program, own_club, club_address, staff_address, {0x1036495c, 0x10364978, 0x103649a0})
                    : 0;
        const std::int32_t policy_cap = c.i32(frame.at(0x40));
        if (nonzero(c, policy_cap) && nonzero(c, std::int8_t(policy)) && compare(c, policy_cap, signed_value(offer)) < 0)
            offer = std::uint32_t(policy_cap);
        if (nonzero_logical(c, own_club) && !nonzero(c, std::int8_t(policy))) {
            // 0x1036f170 may refuse the move outright (2).
            c.put32(frame.at(0x38), 0);
            const std::uint32_t verdict = program.call(
                0x1036f170, 0x10364a08,
                {manager_address, own_club, staff_address, std::uint32_t(signed_value(assessed) >> 31), assessed,
                 club_address, std::uint32_t(signed_value(offer) >> 31), offer});
            if (compare(c, std::int8_t(verdict), 2) == 0) {
                restore_budget();
                continue;
            }
        }
        const double scaled_divisor = fpu.fmul(c.f(31), as_double(c, fpu, frame.at(0xc8), signed_value(divisor)));
        const double gap = as_double(c, fpu, frame.at(0xc0), signed_value(offer - assessed));
        const double w = weight(fpu, gap_ratio(fpu, gap, scaled_divisor));
        c.set(4, staff_address);
        scale_local(c, fpu, frame.at(0x44), frame.at(0xd0), frame.at(0xd8), w);
        fitness_penalty(c, program, staff_address, frame.at(0x44), 0x10364aec);
        if (compare(c, c.i32(frame.at(0x44)), c.i32(frame.at(0x48))) < 0) {
            restore_budget();
            continue;
        }
        // The club's interest score must reach 3.15.
        const double score = interest_score(c, program, manager_address, staff_address, club_address, 1,
                                            std::uint32_t(std::int8_t(row.category())), 0x10364b5c);
        fpu.fcmp(0, score, constant::registration_score);
        if (!(score >= constant::registration_score)) {
            restore_budget();
            continue;
        }
        if (!nonzero(c, signed_value(row.loan()))) {
            // The risk: the draft's figure (0x1037b6f0) times 3.5 over the
            // score (at least 0.9), single precision, must stay within 1.2
            // times what the finances allow for the category (0x100f2680).
            const std::uint32_t category = std::uint32_t(std::int8_t(row.category()));
            program.call(0x1037b6f0, 0x10364b9c, {draft, staff_address, club_address, 0, category, 11, 0});
            c.set_f(31, c.f(1));
            const double first = interest_score(c, program, manager_address, staff_address, club_address, 0, category, 0x10364bc8);
            double chosen = constant::score_floor;
            fpu.fcmp(0, chosen, first);
            // Quirk: a score at the floor or above is asked for again.
            if (!(chosen > first))
                chosen = interest_score(c, program, manager_address, staff_address, club_address, 0,
                                        std::uint32_t(std::int8_t(row.category())), 0x10364c00);
            double risk = fpu.frsp(fpu.fdiv(constant::risk_numerator, fpu.frsp(chosen)));
            fpu.fcmp(0, risk, constant::risk_floor);
            if (risk < constant::risk_floor) risk = constant::risk_floor_single;
            c.set_f(31, fpu.fmuls(risk, c.f(31)));
            const std::uint32_t club_finances =
                program.call(0x10103510, 0x10364c3c, {c.u32(c.toc(slot_recruitment::finances)), club_address});
            const std::int32_t allowed = signed_value(program.call(
                0x100f2680, 0x10364c50, {club_finances, staff_address, std::uint32_t(std::int8_t(row.category()))}));
            const double allowed_d = as_single(c, fpu, frame.at(0xc0), allowed);
            c.set_f(1, allowed_d);  // the original computes it into f1
            const double limit = fpu.fmul(constant::risk_margin, allowed_d);
            fpu.fcmp(0, c.f(31), limit);
            if (c.f(31) > limit) return 0;
        }
        // Registered: the row takes the person, the person it held loses the
        // link (0x10365590), and the new link is added (0x103632f0).
        row.target() = staff.id().get();
        if (nonzero_logical(c, previous)) program.call(0x10365590, 0x10364cac, {manager_address, previous, club_address, 3, 0});
        program.call(0x103632f0, 0x10364cbc, {manager_address, staff_address, club_address});
        return 1;
    }
    return 0;
}

std::uint32_t consider_requested_club_interest(Cpu& c, std::uint32_t manager_address, std::uint32_t staff_address,
                                               std::uint32_t club_address, std::uint32_t valuation, std::uint32_t apply) {
    Frame frame(c, 0x10364d00, 0xc0, {.first_gpr = 13});
    keep(c, {{28, c.toc(slot::contracts)}, {15, manager_address}, {26, c.toc(slot_recruitment::fitness)},
             {16, staff_address}, {27, c.toc(slot::today)}, {17, club_address}, {14, c.toc(slot_recruitment::finances)},
             {18, valuation}, {29, c.toc(slot::constants)}, {13, c.toc(slot::random_salt)},
             {30, c.toc(slot::random_cursor)}});
    c.put8(frame.at(0xeb), apply);
    Program program{c};
    const Manager manager{c, manager_address};
    const Person staff{c, staff_address};
    const Club club{c, club_address};
    Kept<25> own_club{c, contract_club(c, published_contract(c, staff_address))};
    const ClubEntry entry{c, manager.clubs() + std::uint32_t(club.id()) * 12};
    Kept<20> held_rating{c, entry.address};  // the entry, then the foreign draw, then the row's rating
    const std::uint32_t given_roles = program.call(0x10117490, 0x10364d8c, {staff.player(), manager.clubs()});
    Kept<19> roles{c, with_implied_roles(c, given_roles)};
    switch (entry_and_link(c, program, manager, entry, staff_address, club_address, frame.at(0x48), 0x10364e50)) {
    case Before::refuse: return 0;
    case Before::linked: return 1;
    case Before::go: break;
    }
    const std::uint32_t found_club = contract_club(c, published_contract(c, staff_address));
    if (!nonzero(c, std::int8_t(program.call(0x1036f590, 0x10364ebc, {manager_address, staff_address, found_club, club_address}))))
        return 0;
    if (!nonzero(c, std::int8_t(program.call(0x103f83c0, 0x10364ee8, {c.u32(c.toc(slot_recruitment::transfers)),
                                                                      staff_address, club_address, 0, 1}))))
        return 0;
    Fpu fpu{c};
    Kept<21> previous{c, c.r(21)};  // the row's person
    Kept<24> previous_club{c, c.r(24)};
    Kept<31> previous_assessed{c, c.r(31)};
    Kept<22> at{c, entry.rows()};
    for (Kept<23> index{c, 0}; compare(c, signed_value(index), std::int32_t(Row::per_club)) < 0; index = index + 1, at = at + Row::size) {
        const Row row{c, at};
        if (!row_open(c, row, std::int16_t(roles), false)) continue;
        if (!row_due(c, row, frame.at(0x40))) continue;
        if (!prospect_allowed<20>(c, program, row, staff, club, nullptr, {5000, 6, 1000, 500, 0x10365034, 0x1036507c}))
            continue;
        if (!category_allowed(c, program, row, staff_address, club_address, 0x103650a4)) continue;
        // The person the row holds, if any: their assessment (sp+0x3c) by the
        // gap between their move's value and their assessment, over the
        // club's reputation squared / 10.
        const std::int32_t held = row.target();
        if (compare(c, held, -1) != 0) {
            held_rating = std::uint32_t(row.level().get());
            previous = c.u32(c.toc(slot::persons)) + std::uint32_t(held) * 0x70;
            previous_assessed = program.call(0x10361490, 0x1036510c, {manager_address, club_address, previous, at,
                                                                     frame.at(0x3c), held_rating, 1});
            previous_club = contract_club(c, published_contract(c, previous));
            std::uint32_t value = 0;
            if (nonzero_logical(c, previous_club))
                value = move_value(c, program, previous_club, club_address, previous, {0x10365170, 0x1036518c, 0x103651b0});
            const std::uint32_t gap = value - previous_assessed;
            leave(c, {{4, signed_word(signed_value(gap))}, {5, 0x43300000}, {6, gap}});
            const double gap_d = as_double(c, fpu, frame.at(0x68), signed_value(gap));
            const double divisor = as_double(c, fpu, frame.at(0x60), reputation_divisor(club));
            const std::int32_t scaled =
                scale_local(c, fpu, frame.at(0x3c), frame.at(0x58), frame.at(0x50), weight(fpu, gap_ratio(fpu, gap_d, divisor)));
            if (compare(c, scaled, row.level()) >= 0) continue;
            c.put32(frame.at(0x3c), compare(c, scaled, signed_value(held_rating)) > 0 ? std::uint32_t(scaled)
                                                                                      : std::uint32_t(held_rating));
            fitness_penalty(c, program, previous, frame.at(0x3c), 0x103652b0);
        } else {
            held_rating = std::uint32_t(row.level().get());
            previous = 0;
            c.put32(frame.at(0x3c), held_rating);
        }
        // The candidate's assessment (sp+0x38), capped by their own club, by
        // the gap to the caller's valuation.
        std::uint32_t assessed = program.call(0x10361490, 0x10365308, {manager_address, club_address, staff_address, at,
                                                                      frame.at(0x38), held_rating, 0});
        held_rating = assessed;
        assessed = own_club_cap(c, program, staff_address, own_club, assessed,
                                {0x10365320, 0x10365334, 0x10365358, 0x1036537c, 0x103653a8});
        held_rating = assessed;
        const std::uint32_t gap = valuation - assessed;
        leave(c, {{4, 0x43300000}, {5, signed_word(signed_value(gap))}});
        const double gap_d = as_double(c, fpu, frame.at(0x50), signed_value(gap));
        const double divisor = as_double(c, fpu, frame.at(0x58), reputation_divisor(club));
        const double w = weight(fpu, gap_ratio(fpu, gap_d, divisor));
        scale_local(c, fpu, frame.at(0x38), frame.at(0x60), frame.at(0x68), w);
        fitness_penalty(c, program, staff_address, frame.at(0x38), 0x103654a4);
        if (compare(c, c.i32(frame.at(0x38)), c.i32(frame.at(0x3c))) < 0) continue;
        const double score = interest_score(c, program, manager_address, staff_address, club_address, 1,
                                            std::uint32_t(std::int8_t(row.category())), 0x10365504);
        fpu.fcmp(0, score, constant::requested_score);
        if (!(score >= constant::requested_score)) continue;
        if (!nonzero(c, std::int8_t(c.u8(frame.at(0xeb))))) return 1;
        row.target() = staff.id().get();
        if (nonzero_logical(c, previous)) program.call(0x10365590, 0x10365548, {manager_address, previous, club_address, 3, 0});
        program.call(0x103632f0, 0x10365558, {manager_address, staff_address, club_address});
        return 1;
    }
    return 0;
}

void distribute_staff_interest(Cpu& c, std::uint32_t manager_address, std::uint32_t staff_address, std::uint32_t force) {
    Frame frame(c, 0x10363ca0, 0x70, {.first_gpr = 22});
    keep(c, {{26, staff_address}, {27, c.toc(slot_recruitment::staff_extras)}, {28, c.toc(slot::contracts)},
             {25, manager_address}, {29, c.toc(slot_recruitment::last_contract)}, {30, c.toc(slot::clubs)},
             {31, c.toc(slot::club_count)}});
    Program program{c};
    const Person staff{c, staff_address};
    const std::uint32_t profile = staff.player();
    c.set(4, profile);
    if (!nonzero_logical(c, profile)) return;
    if (!nonzero(c, std::int8_t(force)) && compare(c, c.i16(profile + 0xa), 5500) < 0) return;
    // The person's extra record (100 bytes per id): byte +0xc, bits 2-3 a
    // status (1: not distributed; inferred), bits 4-7 the clubs already
    // interested (inferred).
    const std::uint32_t extras = c.u32(c.toc(slot_recruitment::staff_extras));
    c.set(4, extras);
    const std::uint32_t status_at = extras + std::uint32_t(staff.id()) * 100 + 0xc;
    if (compare_logical(c, (c.u8(status_at) >> 2) & 3, 1) == 0) return;
    if (compare(c, std::uint8_t(staff.job()), 11) != 0) return;
    if (nonzero_logical(c, staff.national_team()) && compare(c, staff.national_job(), 5) == 0) return;
    const std::uint32_t nation = staff.nation();
    c.set(4, nation);
    if (nonzero_logical(c, nation)) {
        const std::int32_t nation_id = c.i32(nation + 0x68);
        c.set(4, std::uint32_t(nation_id));
        if (compare(c, nation_id, c.i32(c.toc(slot_recruitment::home_nation_id))) == 0) return;
    }
    if (nonzero(c, std::int8_t(program.call(0x103f43b0, 0x10363d68, {c.u32(c.toc(slot_recruitment::transfers)), staff_address, 0}))))
        return;
    // A person with a club who joined it within the last 200 days (inferred)
    // stays unless their contract (status nibble not 7) has its +0x50 set.
    if (nonzero_logical(c, staff.club())) {
        program.call(0x10717b20, 0x10363d8c, {frame.at(0x40), c.toc(slot::today), 200});
        c.put32(frame.at(0x38), c.u32(frame.at(0x40)));
        c.put32(frame.at(0x3c), c.u32(frame.at(0x44)));
        if (earlier(c, frame.at(0x38), staff.joined().address)) {
            const std::uint32_t contract = contract_of(c, c.u32(c.toc(slot::contracts)), staff_address);
            if (nonzero_logical(c, contract) && compare(c, c.u8(contract + 0x60) & 0xf, 7) != 0 &&
                compare(c, c.i32(contract + 0x50), 0) == 0)
                return;
        }
    }
    const std::uint32_t contract = contract_of(c, c.u32(c.toc(slot::contracts)), staff_address);
    c.put32(c.toc(slot_recruitment::last_contract), contract);
    Kept<28> own_club{c, contract_club(c, contract)};
    const std::uint32_t id_offset = std::uint32_t(staff.id()) * 100;
    Kept<29> count{c, std::uint32_t(std::int8_t((c.u8(extras + id_offset + 0xc) >> 4) & 0xf))};
    const std::uint32_t given_roles = program.call(0x10117490, 0x10363e6c, {profile, id_offset, extras});
    Kept<27> roles{c, with_implied_roles(c, given_roles)};
    // The clubs in the game's order, until five are interested.
    const std::uint32_t clubs = c.toc(slot::clubs);
    Kept<23> index{c, 0};
    Kept<24> next{c, c.u32(c.toc(slot_recruitment::club_order))};
    while (compare(c, signed_value(index), c.i32(c.toc(slot::club_count))) < 0) {
        // The club's id in r3 (lwz r3): what the routine returns with when
        // five clubs are interested.
        c.set(3, c.u32(next));
        c.set(4, c.u32(clubs));
        Kept<22> club{c, club_of(c, c.i32(next))};
        if (compare(c, std::int8_t(count), 5) >= 0) return;
        if (!nonzero(c, std::int8_t(program.call(0x100c7d00, 0x10363ef8, {club}))) &&
            compare_logical(c, own_club, club) != 0 && compare_logical(c, staff.club(), club) != 0) {
            enter(c, 0x10363f28, {manager_address, staff_address, club, roles});
            if (nonzero(c, std::int8_t(result(c, register_club_interest(c, manager_address, staff_address, club, roles)))))
                count = count + 1;
        }
        index = index + 1;
        next = next + 4;
    }
}
}
