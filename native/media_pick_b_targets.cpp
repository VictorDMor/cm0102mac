// 0x10699d10: how interested the club is in a story about `person` (a score from the two clubs'
// ties, the rival clubs, the person's own standing), then, for each member of the club's staff
// list, a weight of that member's own, and two draws that decide whether the member's contract
// gets flag 1 or the 0x800 mark.
//
// Facts the original's shape rests on (no code for the impossible branches):
// - the club score only grows from 10 (-5 at worst) and is clamped to 5 below, at most 10 + 5 +
//   127 * 100, so its draws are always the remainder form (positive, within 16 bits) and never zero;
// - the member weight is clamped to a signed byte >= 5 before its draws, so those are positive too.
#include "media_pick_a_detail.hpp"
#include "media_pick_b.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::pick;
namespace {
namespace tie {
constexpr std::uint32_t near_a = 0x100d9480, near_b = 0x100d94d0, rival = 0x10464160, nudge = 0x100d97a0,
    unmovable = 0x100d3150, standing = 0x10082f90, settled = 0x10380710, squad = 0x100d06f0, sponsor = 0x1014c2e0,
    sponsor_flag = 0x100c7a20, wanted = 0x100d3300, ties_a = 0x100d9520, ties_b = 0x100d9580, record = 0x100d9b80,
    mark = 0x1037fa80, flag = 0x1037f8a0;
}
constexpr std::uint32_t list_size = 0x39;  // the loop stops before the list's last word (quirk)

// The next table sample, in the forms the compiler expanded in line. Each leaves the scratch
// registers its form leaves.
struct Roll {
    Cpu& c;
    Stream rng;
    // remainder by a count n > 0: r3 the sample.
    std::int32_t by_count(std::uint32_t refill_back, std::int32_t n, unsigned result) const {
        return draw_narrow(c, rng, n, refill_back, result);
    }
    // remainder by a byte n > 0: r3 n, r4 the sample, r5 the table cursor.
    std::int32_t by_byte(std::uint32_t refill_back, std::int32_t n, unsigned result) const {
        const auto value = draw_narrow(c, rng, n, refill_back, result);
        leave(c, {{3, std::uint32_t(n)}, {4, rng.sample()}, {5, c.u32(rng.cursor)}});
        return value;
    }
    // remainder by ten: r3 the quotient's sign, r4 the sample.
    std::int32_t by_ten(std::uint32_t refill_back) const {
        rng.advance(refill_back);
        const auto drawn = reduced_by(rng.sample(), divisors::by_10);
        leave(c, {{3, drawn.sign}, {4, drawn.sample}});
        return drawn.rest;
    }
};

bool answer(Cpu& c, std::uint32_t target, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
    Program{c}.call(target, back, arguments);
    return compare(c, byte_word(c.r(3))) != 0;  // extsb.
}

struct Targets {
    Cpu& c;
    const Frame& frame;
    std::uint32_t kind, person, club, rivals, rival_count;
    Kept<15> other;
    Kept<16> clubs_or_roll;  // the clubs table's slot, then the member's second draw
    Kept<22> global_or_contract;
    Kept<29> interest;
    Kept<30> index;
    Kept<31> global_or_weight;
    Roll roll;
    std::uint32_t list() const { return frame.at(0x38); }

    Targets(Cpu& cpu, const Frame& f, std::uint32_t k, std::uint32_t p, std::uint32_t cl, std::uint32_t r, std::uint32_t n)
        : c(cpu), frame(f), kind(k), person(p), club(cl), rivals(r), rival_count(n), other(cpu, cpu.r(15)),
          clubs_or_roll(cpu, cpu.toc(-0x7be8)), global_or_contract(cpu, cpu.toc(-0x7a04)), interest(cpu, 10), index(cpu, cpu.r(30)),
          global_or_weight(cpu, cpu.toc(-0x7a08)), roll{cpu, Stream::world(cpu)} {}

    void run() {
        if (!nonzero(c, std::int32_t(club))) return;
        if (!nonzero_logical(c, person)) return;
        tie_interest();
        rival_interest();
        draw_interest();
        gather_staff();
        for (index = 0; compare(c, std::int32_t(index), std::int32_t(list_size)) < 0; index = std::uint32_t(index) + 1) weigh_member();
    }

    // The person's ties with the club: -5, or +5.
    void tie_interest() {
        if (answer(c, tie::near_a, 0x10699d7c, {person, club})) interest = std::uint32_t(interest) - 5;
        else if (answer(c, tie::near_b, 0x10699d98, {person, club})) interest = std::uint32_t(interest) + 5;
    }

    // Each rival club in the list: both of the two special clubs +100, rivals +10, the person's
    // club listed among its three links +15, a shared parent +5.
    void rival_interest() {
        for (index = 0;; index = std::uint32_t(index) + 1) {
            if (!nonzero_logical(c, rivals)) break;
            if (compare(c, std::int32_t(index), byte_word(rival_count)) >= 0) break;
            const auto special_a = c.u32(global_or_contract), special_b = c.u32(global_or_weight);
            const auto club_id = c.u32(club);
            const auto table = c.u32(clubs_or_roll);
            const auto offset = c.u32(rivals + index * 4) * 0x248;
            leave(c, {{3, club_id}, {4, offset}, {5, table}});
            other = table + offset;
            const auto other_id = c.u32(other);
            const auto special = [&](std::uint32_t id) { return std::int32_t(id) == std::int32_t(special_a) || std::int32_t(id) == std::int32_t(special_b); };
            if (special(club_id)) {
                leave(c, {{3, other_id}});
                if (special(other_id)) { interest = std::uint32_t(interest) + 0x64; continue; }
            }
            if (answer(c, tie::rival, 0x10699e14, {club, other})) { interest = std::uint32_t(interest) + 10; continue; }
            const Club rival_club{c, other};
            if (club == rival_club.affiliate(0) || club == rival_club.affiliate(1) || club == rival_club.affiliate(2)) {
                interest = std::uint32_t(interest) + 0xf;
                continue;
            }
            const auto parent = Club{c, club}.division().get();
            if (parent == 0) continue;
            leave(c, {{3, c.u32(parent)}});
            if (std::int32_t(c.u32(parent)) == std::int32_t(c.u32(c.r(17)))) continue;
            leave(c, {{3, parent}});
            if (parent == rival_club.division()) interest = std::uint32_t(interest) + 5;
        }
    }

    // Two draws against the interest; the second, a digit, decides whether the tie gets a nudge.
    void draw_interest() {
        // The interest is >= 5 (see the file's comment): never zero, never beyond the table's range.
        const auto first = roll.by_count(0x10699ecc, std::int32_t(interest), 15);
        const auto digit = roll.by_ten(0x1069a028);
        if (first > digit) Program{c}.call(tie::nudge, 0x1069a078, {club, person, 1});
        if (answer(c, tie::unmovable, 0x1069a084, {person, 0})) interest = std::uint32_t(interest) - 0x19;
        else {
            other = Program{c}.call(tie::standing, 0x1069a0a4, {c.u32(c.r(14)), person, club});
            if (compare(c, std::int32_t(other)) == 0) interest = std::uint32_t(interest) - 0xf;
            else {
                if (answer(c, tie::settled, 0x1069a0bc, {other})) interest = std::uint32_t(interest) - 0xa;
                // A tenth of the signed morale byte at +0x44, rounded toward zero, as a signed byte.
                const auto morale = std::int32_t(Contract{c, other}.happiness().get());
                const auto shifted = std::uint32_t(multiply_high(0x66666667, morale) >> 2), sign = shifted >> 31;
                interest = std::uint32_t(interest) + std::uint32_t(byte_word(shifted + sign));
            }
        }
        if (compare(c, std::int32_t(interest), 5) < 0) interest = 5;
    }

    // The club's staff list on the stack: 50 from 0x100d06f0, the club's words at +0x80, its
    // manager (0 when the sponsor test passes), +0x94 and five from +0x160.
    void gather_staff() {
        const auto put = [&](std::uint32_t at, std::uint32_t value) { c.put32(list() + at * 4, value); };
        Program{c}.call(tie::squad, 0x1069a10c, {club, list(), 0});
        index = 0x32;
        put(0x32, c.u32(club + 0x80));
        leave(c, {{3, list()}, {4, c.u32(club + 0x80)}});
        other = 0;
        index = 0x33;
        bool flagged = answer(c, tie::sponsor, 0x1069a140, {club, 0, 1, 0, 0});
        const Club team{c, club};
        if (!flagged && nonzero_logical(c, team.manager()))
            flagged = answer(c, tie::sponsor_flag, 0x1069a160, {std::uint8_t(team.manager_record().job().get())});
        if (flagged) other = 1;
        compare(c, std::int32_t(other));
        const auto manager = flagged ? 0 : team.manager().get();
        leave(c, {{4, manager}, {3, list()}});
        put(0x33, manager);
        index = 0x34;
        put(0x34, team.deputy());
        leave(c, {{3, list()}, {4, team.deputy()}, {5, 0}});
        index = 0x35;
        for (std::uint32_t k = 0; k < 5; ++k) {
            index = 0x35 + k + 1;
            put(0x35 + k, c.u32(club + 0x160 + k * 4));
            leave(c, {{3, list()}, {4, c.u32(club + 0x160 + k * 4)}, {5, k + 1}});
        }
        index = 0;
    }

    // One member of the list: its weight, then the draws.
    void weigh_member() {
        const auto p = c.u32(list() + std::uint32_t(index) * 4);
        global_or_weight = 10;
        leave(c, {{3, list()}});
        if (!nonzero_logical(c, p)) return;
        if (p == person) return;
        const Person member{c, p};
        leave(c, {{3, p}});
        if (c.u32(member.at + 0x18) == 0) return;  // the +0x18 link
        if (!weight_of(member)) return;
        draw_outcome(member);
    }

    bool weight_of(const Person& m) {
        const auto add = [&](std::int32_t delta) { global_or_weight = std::uint32_t(global_or_weight) + std::uint32_t(delta); };
        const auto rating = [&] { return std::int32_t(m.player_record().reputation().get()); };
        if (m.non_player() != 0) {
            leave(c, {{3, m.non_player()}});
            add(c.u8(m.non_player() + 0x3d));
        } else if (m.player() != 0) {
            if (Person{c, person}.age() > 0x14) {
                if (m.age() < 0x17) {
                    leave(c, {{3, m.player()}});
                    if (rating() < 0x109a) return false;
                }
            }
            leave(c, {{3, std::uint32_t(unsigned_byte(m.trait_69()))}, {4, list()}, {5, std::uint32_t(index) * 4}});
            add(unsigned_byte(m.trait_69()));
            if (m.age() < 0x14) add(-5);
            if (unsigned_byte(m.sportsmanship()) == 0x14) {
                leave(c, {{3, m.at}});
                add(1);
            } else if (unsigned_byte(m.sportsmanship()) == 1) {
                add(answer(c, tie::wanted, 0x1069a33c, {m.at, 0}) ? -0xf : -0xa);
                leave(c, {{3, m.player()}});
                if (rating() > 0x1676) add(-0xa);
            } else if (m.sportsmanship().get() < 6) {
                add(answer(c, tie::wanted, 0x1069a3a4, {m.at, 0}) ? -0xa : -5);
                leave(c, {{3, m.player()}});
                if (rating() > 0x1676) add(-5);
            } else leave(c, {{3, m.at}});
        } else leave(c, {{3, m.at}});
        if (half_word(kind) == 0x20 && m.loyalty().get() > 0xf && m.trait_69().get() < 6) add(-5);
        leave(c, {{3, m.at}});
        if (m.loyalty().get() > 0xf && unsigned_byte(m.sportsmanship()) != 0x14) add(-2);
        else if (m.loyalty().get() < 5) add(2);
        if (half_word(kind) == 0x1e) {
            const std::int32_t b67 = m.sportsmanship().get(), b66 = m.professionalism().get();
            if (b67 > 0xf) { if (b66 < 0xf) add(-1); }
            else if (b67 < 6) add(b66 < 6 ? -0xa : -5);
            else add(b66 < 0xf ? -3 : -1);
        }
        if (answer(c, tie::ties_a, 0x1069a54c, {person, m.at})) add(0x19);
        else if (answer(c, tie::ties_b, 0x1069a570, {person, m.at})) {
            // The weight comes back as a signed byte, at most 15.
            global_or_weight = std::uint32_t(byte_word(std::uint32_t(global_or_weight) - (0x19 - unsigned_byte(Person{c, person}.sportsmanship()))));
            if (compare(c, std::int32_t(global_or_weight), 0xf) > 0) global_or_weight = 0xf;
        }
        if (answer(c, tie::near_a, 0x1069a5a0, {person, club})) add(5);
        else if (answer(c, tie::near_b, 0x1069a5bc, {person, club})) add(-5);
        if (byte_word(global_or_weight) < 5) global_or_weight = 5;
        return true;
    }

    // The draws: the interest against the weight, then the contract's flag or mark.
    void draw_outcome(const Person& m) {
        const auto weight = byte_word(global_or_weight);
        const auto first = roll.by_count(0x1069a608, std::int32_t(interest), 15);
        const auto second = roll.by_byte(0x1069a774, weight, 16);
        if (compare(c, first, second) <= 0) return;
        Program{c}.call(tie::record, 0x1069a8ec, {m.at, person, 1});
        global_or_contract = Program{c}.call(tie::standing, 0x1069a904, {c.u32(c.r(14)), m.at, club});
        if (compare(c, std::int32_t(global_or_contract)) == 0) return;
        const auto chance = roll.by_byte(0x1069a93c, weight, 15);
        const auto digit = roll.by_ten(0x1069aab4);
        compare(c, chance, digit);
        if (chance >= digit) return;
        if (compare(c, half_word(kind), 0x1e) == 0 || compare(c, half_word(kind), 0x20) == 0) {
            Program{c}.call(tie::mark, 0x1069acbc, {global_or_contract, 0x800, 0});
            return;
        }
        const auto flag_roll = roll.by_byte(0x1069ab3c, weight, 15);
        compare(c, flag_roll);
        if (flag_roll != 0) Program{c}.call(tie::flag, 0x1069aca8, {global_or_contract, 1});
        else Program{c}.call(tie::mark, 0x1069acbc, {global_or_contract, 0x800, 0});
    }
};
}

void weigh_story_targets(Cpu& c, std::uint32_t kind, std::uint32_t person, std::uint32_t club, std::uint32_t rivals,
                         std::uint32_t rival_count) {
    Frame frame(c, 0x10699d10, 0x180, {.first_gpr = 14});
    const auto rng = Stream::world(c);
    keep(c, {{14, c.toc(-0x7b7c)}, {17, c.toc(-0x7a0c)}, {18, rivals}, {19, rival_count}, {20, rng.end}, {21, rng.start}, {23, kind},
             {24, c.toc(slot::floats)}, {25, rng.seed}, {26, person}, {27, club}, {28, rng.cursor}});
    Targets targets(c, frame, kind, person, club, rivals, rival_count);
    targets.run();
}
}
