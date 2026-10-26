// SHORTLIST_MANAGER.CPP: the day's approaches of one club (0x1036d6e0). For
// each shortlist row with a target and no approach yet: the checks on the
// target, then random draws (from the row's weight against the club's
// reputation) and, when the weight's draw is higher, a transfer offer (an
// offer object at sp+0x50) or, for a row with bit 7 set (inferred: a loan),
// an approach through 0x103636b0. A failed attempt drops the link (0x10365590
// mode 1).
#include "shortlist_manager_detail.hpp"
#include "shortlist_manager_random.hpp"

namespace cm::aot::shortlist_manager::daily {
using namespace detail;
namespace {
constexpr std::uint32_t contract_of = 0x10082f20;    // (contracts, person): his current contract
constexpr std::uint32_t contract_with = 0x10082f90;  // (contracts, person, club)
constexpr std::uint32_t club_flag = 0x1014c2e0;      // (club, which, 1, 0, 0): a club predicate (signed byte)
constexpr std::uint32_t transition = 0x10365590;     // (manager, person, club, mode, not a player): the link's transition
constexpr std::uint32_t histories_of = 0x1018c340;   // (histories, person): his history record (+0x34 int16)

namespace number {
constexpr double one = 1.0;                // +0x080
constexpr double ten = 10.0;               // +0x150
constexpr double zero = 0.0;               // +0x158
constexpr double one_point_two = 1.2;      // +0x170
constexpr double one_point_five = 1.5;     // +0x178
constexpr double unsigned_bias = 4503599627370496.0;  // +0x2b8: 2^52, the unsigned int-to-double idiom's
constexpr double one_point_four = 1.4;     // +0x2d0
constexpr double thirty = 30.0;            // +0x408
constexpr double ten_thousand = 10000.0;   // +0x460
}
}

void approach_targets(Cpu& c, std::uint32_t manager, std::uint32_t club) {
    Frame frame(c, 0x1036d6e0, 0x1e0, {.first_gpr = 13});
    Program program{c};
    const Kept<16> club_reg{c, club};
    Kept<17> person{c, c.toc(game_slot::humans)};  // the human managers' slot, then the approaches held, then the target
    const Kept<25> transfers{c, c.toc(game_slot::transfers)};
    const Kept<15> manager_reg{c, manager};
    const Kept<26> contracts{c, c.toc(slot::contracts)};
    const Kept<27> last_contract{c, c.toc(game_slot::last_contract)};
    const Kept<28> constants{c, c.toc(slot::constants)};
    const Kept<29> salt{c, c.toc(slot::random_salt)};
    const Kept<14> table{c, c.toc(slot::random_table)};
    const Kept<13> table_end{c, c.toc(slot::random_end)};
    const Kept<30> cursor{c, c.toc(slot::random_cursor)};
    const Kept<31> clubs{c, c.toc(slot::clubs)};
    const Club owner{c, club};
    const std::uint32_t humans = person;

    // A week from today, copied to sp+0x48 (five bytes), and an object at
    // sp+0x108 (0x103721f0) built and not used further.
    program.call(0x10717890, 0x1036d730, {frame.at(0x40), c.toc(slot::today), 7});
    {
        const std::uint32_t day = extended(c.i16(frame.at(0x40))), year = c.u16(frame.at(0x42));
        c.put16(frame.at(0x48), day);
        c.put16(frame.at(0x4a), year);
        c.put8(frame.at(0x4c), c.u8(frame.at(0x44)));
        c.set({{5, day}, {4, year}});
    }
    program.call(0x103721f0, 0x1036d750, {frame.at(0x108)});
    if (nonzero(c, std::int8_t(program.call(0x100c7d00, 0x1036d758, {club})))) return;
    if (nonzero(c, std::int8_t(program.call(club_flag, 0x1036d778, {club, 0, 1, 0, 0})))) return;
    if (nonzero(c, std::int8_t(program.call(club_flag, 0x1036d798, {club, 1, 1, 0, 0})))) {
        const std::uint32_t first = program.call(0x100c96e0, 0x1036d7a8, {club});
        if (nonzero(c, std::int8_t(program.call(0x10147e70, 0x1036d7b4, {c.u32(humans), first})))) {
            const std::uint32_t second = program.call(0x100c96e0, 0x1036d7c4, {club});
            if (compare(c, std::int8_t(program.call(0x10149df0, 0x1036d7d4, {c.u32(humans), 4, second})), 1) == 0) return;
        }
    }
    const std::uint32_t entries = Manager{c, manager}.clubs().get();
    c.set(3, entries);
    const Kept<20> entry{c, entries + std::uint32_t(owner.id()) * ClubEntry::size};
    if (compare(c, ClubEntry{c, entry}.status(), 1) != 0) return;
    if (compare_logical(c, ClubEntry{c, entry}.changes(), 100) >= 0) return;
    if (!nonzero_logical(c, program.call(0x10103510, 0x1036d81c, {c.u32(c.toc(game_slot::finances)), club}))) return;
    person = program.call(0x103660b0, 0x1036d834, {manager, club, 11});
    const std::uint32_t staff = program.call(0x100c9070, 0x1036d848, {club, 0, 0});
    // The approaches under way (inferred: the kind-11 links and the staff
    // count), at most 50 for player rows; the row index lives in the frame.
    const std::uint32_t index_at = frame.at(0x184);
    c.put8(index_at, 0);
    Kept<19> approaches{c, extended8(staff + person)};
    Kept<23> offset{c, 0};
    Kept<18> player{c, c.r(18)};  // the row is a player's (kind 11); a draw bound on the loan path
    Kept<21> weight{c, c.r(21)};  // the weight to beat, then draws
    Kept<22> chance{c, c.r(22)};  // a draw; the previous holder on the offer path

    const auto rows = [&] { return c.u32(entry); };
    const auto row = [&] { return Row{c, rows() + offset}; };
    // His contract's club (0: none), as the original reads it after each
    // contract lookup, which also sets the shared contract slot.
    const auto contract_club = [&](std::uint32_t back) {
        const std::uint32_t contract = program.call(contract_of, back, {c.u32(contracts), person});
        c.put32(last_contract, contract);
        if (!nonzero_logical(c, contract)) return 0u;
        const std::uint32_t base = c.u32(clubs);
        c.set(3, base);
        return base + c.u32(contract + 4) * Club::size;
    };
    // A random number below n (shortlist_manager_random.hpp) into R, inlined
    // at each site. `first` is the site whose wide path leaves n - 1 in r4
    // (the others leave the tenth's sign there).
    const RandomTable random{c, cursor, table_end, table, salt};
    const auto draw_below = [&]<unsigned R>(Kept<R>&, std::int32_t n, std::uint32_t refill_back, std::uint32_t division,
                                            std::uint32_t sample_back, bool first) {
        Fpu fpu{c};
        const DrawSite site{refill_back, division, sample_back, frame.at(0x16c), frame.at(0x174), frame.at(0x17c), first};
        detail::draw_below(c, fpu, random, site, n, R, [&](const WideDraw& d) { d.leave_a(c); });
    };
    const auto drop_link = [&](std::uint32_t back) {
        const std::int32_t kind = std::int8_t(row().kind());
        c.set(8, std::uint32_t(kind));
        program.call(transition, back, {manager, person, club, 1, kind != Row::player});
    };

    // The transfer offer (bit 7 of the row clear).
    const auto offer = [&] {
        if (row().positions() <= -1 && player) return;
        {
            const bool open = nonzero(c, std::int8_t(program.call(0x103f83c0, 0x1036dac0, {c.u32(transfers), person, club, frame.at(0x38), 0})));
            if (!open && c.i16(frame.at(0x38)) != 9) return;
        }
        if (nonzero(c, std::int8_t(program.call(0x103f5c00, 0x1036dae4, {c.u32(transfers), person, club})))) return;
        if (compare(c, std::int32_t(program.call(0x103eb020, 0x1036dafc, {c.u32(transfers), person, 0}))) != 0) return;
        // The weight to beat: the row's (with bit 2 twice it, at least
        // 20000), more for a player row with a previous holder.
        if (row().flags() & 4) {
            const std::uint32_t doubled = std::uint32_t(row().weight().get()) << 1;
            weight = std::int32_t(doubled) > 20000 ? doubled : 20000u;
        } else {
            weight = std::uint32_t(row().weight().get());
        }
        if (row().previous() > -1 && row().kind() == Row::player) {
            const std::uint32_t current = contract_club(0x1036db60);
            if (!nonzero(c, std::int8_t(program.call(0x103f8820, 0x1036dba0,
                                                     {c.u32(transfers), club, frame.at(0x48), person, current, 1, frame.at(0x38)}))) &&
                c.i16(frame.at(0x38)) != 9) {
                const std::int32_t row_weight = row().weight();
                weight = weight + std::uint32_t(row_weight > 10000 ? row_weight : 10000);
            }
            // His predecessor's history: reputation / 500 x its +0x34
            // value squared (read by two calls, as the original).
            const std::uint32_t persons = c.u32(c.toc(slot::persons));
            c.set({{4, rows()}, {5, persons}});
            chance = persons + std::uint32_t(row().previous().get()) * Person::size;
            const Kept<22>& previous = chance;
            const auto history = [&](std::uint32_t back) {
                return program.call(histories_of, back, {c.u32(c.toc(game_slot::histories)), previous});
            };
            if (nonzero_logical(c, history(0x1036dc00))) {
                const Kept<24> first{c, extended(c.i16(history(0x1036dc18) + 0x34))};
                const std::uint32_t second = extended(c.i16(history(0x1036dc2c) + 0x34));
                const std::uint32_t product = second * first;
                c.set({{5, second}, {4, product}});
                weight = weight + std::uint32_t(owner.reputation() / 500) * product;
            }
            Fpu fpu{c};
            const std::uint32_t record = c.u32(previous + 0x14);
            if (nonzero_logical(c, record)) {
                // Times 1.5 - his record's +0x45 / 30.
                const std::uint32_t word = signed_word(std::int32_t(weight));
                const double by = fpu.fdiv(
                    fpu.fsub(stacked_integer(c, frame.at(0x174), signed_word(c.i8(record + 0x45))), k::signed_bias), number::thirty);
                c.set({{3, 0x43300000}, {4, word}});
                const double value = fpu.fsub(stacked_integer(c, frame.at(0x17c), word), k::signed_bias);
                c.set_f(1, value);  // the original computes it into f1
                const auto bits = fpu.fctiwz(0, fpu.fmul(value, fpu.fsub(number::one_point_five, by)));
                c.put64(frame.at(0x16c), bits);
                weight = std::uint32_t(bits);
            }
            if (nonzero(c, std::int8_t(program.call(0x103f5e50, 0x1036dccc, {c.u32(transfers), previous, 0})))) {
                // At least 1.5 times the row's weight, at least 10000.
                c.set(4, rows());
                const double raised = fpu.fmul(
                    number::one_point_five, fpu.fsub(stacked_integer(c, frame.at(0x16c), signed_word(row().weight())), k::signed_bias));
                fpu.fcmp(0, raised, number::ten_thousand);
                const double chosen = (c.cr(0) & 4) ? raised : number::ten_thousand;
                c.set_f(1, chosen);  // the original computes it into f1 (fmul f1, or fmr f1)
                const auto bits = fpu.fctiwz(0, chosen);
                c.put64(frame.at(0x174), bits);
                weight = std::uint32_t(bits);
            }
        }
        // A player with a contract: the valuation (0x10374f50, nonzero) must
        // not exceed 1.3 times what the club would pay (0x100f2680), unless
        // his club fails the predicate and he is not at the club.
        if (nonzero_logical(c, contract_club(0x1036dd30)) && player) {
            const std::uint32_t other = contract_club(0x1036dd70);
            if (nonzero(c, std::int8_t(program.call(club_flag, 0x1036dda8, {other, 0, 1, 0, 0}))) ||
                Person{c, person}.club() == club) {
                program.call(0x103721f0, 0x1036ddc4, {frame.at(0xa4)});
                const std::uint32_t valued = program.call(0x10374f50, 0x1036ddf8,
                                                          {frame.at(0xa4), person, club, std::uint32_t(-1),
                                                           extended8(row().category()), row().kind(), 0, 0});
                Fpu fpu{c};
                const std::uint32_t word = signed_word(std::int32_t(valued));
                c.set(3, word);
                fpu.fcmp(0, number::zero, fpu.fsub(stacked_integer(c, frame.at(0x16c), word), k::signed_bias));
                c.set_f(1, k::signed_bias);  // the original loads the bias into f1 (lfd f1)
                if (c.cr(0) & 2) return;
                const std::uint32_t finances = program.call(0x10103510, 0x1036de30, {c.u32(c.toc(game_slot::finances)), club});
                const std::uint32_t price = program.call(0x100f2680, 0x1036de4c, {finances, person, extended8(row().category())});
                const double limit = fpu.fmul(
                    constant::one_point_three, fpu.fsub(stacked_integer(c, frame.at(0x174), signed_word(std::int32_t(price))), k::signed_bias));
                c.set(4, 0x43300000);
                const double asked = fpu.fsub(stacked_integer(c, frame.at(0x16c), signed_word(c.i32(frame.at(0xb0)))), k::signed_bias);
                c.set_f(1, asked);  // the original computes it into f1
                fpu.fcmp(0, asked, limit);
                if (c.cr(0) & 4) return;
            }
        }
        // The draws: below the weight, against one below 2 x reputation + 1000.
        draw_below(chance, std::int32_t(weight), 0x1036dec0, 0x1036dfd0, 0x1036df88, true);
        {
            const Kept<24> bound{c, std::uint32_t(owner.reputation()) * 2 + 1000};
            draw_below(weight, std::int32_t(bound), 0x1036e020, 0x1036e130, 0x1036e0e8, false);
        }
        if (compare(c, std::int32_t(chance), std::int32_t(weight)) <= 0) return;
        // The offer object at sp+0x50 (0x10411e60), made by 0x104126d0,
        // which leaves a deal (operator new'd) at sp+0x3c.
        c.put8(row().address + 0x28, c.u8(row().address + 0x28) & ~4u);
        const std::uint32_t bid = frame.at(0x50), deal = frame.at(0x3c);
        program.call(0x10411e60, 0x1036e170, {bid});
        c.put32(deal, 0);
        if (nonzero(c, std::int8_t(program.call(0x104126d0, 0x1036e19c,
                                                {bid, person, club, row().address, std::uint32_t(row().budget().get()), 0, deal})))) {
            program.call(0x103e25d0, 0x1036e1b4, {c.u32(transfers), bid, 0});
            if (nonzero_logical(c, c.u32(deal))) {
                program.call(0x1007f200, 0x1036e1e4,
                             {c.u32(contracts), c.u32(deal), c.u32(frame.at(0x78)), c.u32(frame.at(0x7c)), row().kind(), 0, 1, 0});
                weight = contract_club(0x1036e1f0);
                const Kept<21>& seller = weight;
                // His contract with his club (type 1-5, inferred: not yet a
                // free agent) gets a factor from the buyer's reputation
                // (0x10080d30, the double in f1).
                const auto rate = [&] {
                    if (!nonzero_logical(c, seller)) return;
                    if (nonzero(c, std::int8_t(program.call(club_flag, 0x1036e234, {seller, 0, 1, 0, 0})))) return;
                    if (nonzero_logical(c, program.call(0x1007f720, 0x1036e24c, {c.u32(contracts), c.u32(person), c.u32(seller)}))) return;
                    const std::uint32_t contract = program.call(contract_with, 0x1036e264, {c.u32(contracts), person, seller});
                    if (!nonzero_logical(c, contract)) return;
                    const std::int32_t type = c.u8(contract + 0x60) & 0xf;
                    const bool low = type >= 1 && type < 3;
                    if (!low && !(type >= 3 && type < 6)) return;
                    if (!nonzero_logical(c, program.call(0x1036d240, low ? 0x1036e2ac : 0x1036e394, {manager, seller, person, 0, 0})))
                        return;
                    const std::int32_t reputation = owner.reputation();
                    const std::int32_t scale = low ? reputation : std::int32_t(std::uint32_t(reputation) << 1);
                    const std::uint32_t roles = c.u32(c.toc(game_slot::staff_roles));
                    const std::uint32_t nibble = (c.u8(roles + c.u32(person) * 100 + 0xc) >> 4) & 0xf;
                    Fpu fpu{c};
                    const double ours = fpu.fsub(stacked_integer(c, frame.at(0x174), signed_word(reputation)), k::signed_bias);
                    const double part = fpu.fsub(stacked_integer(c, frame.at(0x16c), nibble), number::unsigned_bias);
                    const double divisor = fpu.fsub(stacked_integer(c, frame.at(0x17c), signed_word(scale < 1 ? 1 : scale)), k::signed_bias);
                    const double tenths = fpu.fdiv(part, number::ten);
                    double factor = fpu.fadd(tenths, fpu.fdiv(ours, divisor));
                    if (!low) factor = fpu.fadd(constant::half, factor);
                    // Clamped to 1.0-1.4 (types 1-2) or 1.0-1.2.
                    const double ceiling = low ? number::one_point_four : number::one_point_two;
                    double chosen = number::one;
                    fpu.fcmp(0, number::one, factor);
                    if (!(c.cr(0) & 4)) {
                        fpu.fcmp(0, ceiling, factor);
                        chosen = (c.cr(0) & 8) ? ceiling : factor;
                    }
                    // r8 and r9 are the double's GPR slots (the ABI's), left as
                    // the original has them.
                    c.set_f(1, chosen);
                    c.set({{8, 0x43300000}, {9, roles}, {10, 2}});
                    c.set({{3, c.u32(contracts)}, {4, person}, {5, 0}, {6, 0}, {7, std::uint32_t(-1)}});
                    c.call(0x10080d30, low ? 0x1036e378 : 0x1036e46c);
                };
                rate();
                program.operator_delete(c.u32(deal), 0x1036e474);
            }
            const std::uint32_t made = c.u32(frame.at(0x78));
            c.set({{5, made}, {4, rows()}});
            row().approach() = std::int32_t(made);
            if (player) approaches = approaches + 1;
        } else {
            drop_link(0x1036e4c8);
        }
        program.call(0x10411f10, 0x1036e4d4, {bid, std::uint32_t(-1)});
    };

    // The loan approach (bit 7 of the row set, inferred).
    const auto loan = [&] {
        const std::uint32_t nation = owner.nation();
        if (nonzero_logical(c, nation)) {
            const std::int32_t id = c.i32(nation + 0x68);
            c.set(4, std::uint32_t(id));
            if (id == c.i32(c.toc(-0x7a00)) && owner.status() == 3) return;
        }
        if (!nonzero(c, std::int8_t(program.call(0x103fa290, 0x1036e518, {c.u32(transfers), club, person, 0})))) return;
        if (nonzero_logical(c, contract_club(0x1036e52c))) {
            const std::uint32_t other = contract_club(0x1036e564);
            if (nonzero(c, std::int8_t(program.call(club_flag, 0x1036e59c, {other, 0, 1, 0, 0})))) {
                // (this lookup leaves the shared contract slot as it is)
                const std::uint32_t contract = program.call(contract_of, 0x1036e5b0, {c.u32(contracts), person});
                if (!(c.u8(contract + 0x5c) & 2)) return;
            }
        }
        if (nonzero(c, std::int8_t(program.call(0x103f5c00, 0x1036e5cc, {c.u32(transfers), person, club})))) return;
        if (compare(c, std::int32_t(program.call(0x103eb020, 0x1036e5e4, {c.u32(transfers), person, 0}))) != 0) return;
        // Wins if a draw below the weight beats one below reputation / 10,
        // or else one beats a draw below twice the reputation.
        player = std::uint32_t(row().weight().get());
        draw_below(weight, std::int32_t(player), 0x1036e628, 0x1036e738, 0x1036e6f0, false);
        player = std::uint32_t(owner.reputation() / 10);
        draw_below(chance, std::int32_t(player), 0x1036e798, 0x1036e8a8, 0x1036e860, false);
        if (compare(c, std::int32_t(weight), std::int32_t(chance)) <= 0) {
            player = std::uint32_t(row().weight().get());
            draw_below(weight, std::int32_t(player), 0x1036e904, 0x1036ea14, 0x1036e9cc, false);
            player = std::uint32_t(owner.reputation()) << 1;
            draw_below(chance, std::int32_t(player), 0x1036ea60, 0x1036eb70, 0x1036eb28, false);
            if (compare(c, std::int32_t(weight), std::int32_t(chance)) <= 0) return;
        }
        const std::int32_t made = std::int32_t(program.call(0x103636b0, 0x1036eba0, {manager, person, club}));
        c.set(4, rows());
        row().approach() = made;
        if (compare(c, made, -1) > 0) approaches = approaches + 1;
        else drop_link(0x1036ebf0);
    };

    // The checks every target passes first: the row's kind against the
    // approaches held, a staff role's free place at the club, his staff
    // record, the transfer manager's two vetoes and his status.
    const auto consider = [&] {
        player = row().kind() == Row::player;
        c.set(4, row().address);
        if (std::int8_t(std::uint32_t(approaches)) >= 50 && player) return;
        const std::int32_t target = row().target();
        if (target <= -1 || row().approach() != -1) return;
        person = c.u32(c.toc(slot::persons)) + std::uint32_t(target) * Person::size;
        if (contract_club(0x1036d8c8) == club) return;
        if (!player) {
            // Kinds 8, 9 and 10 need one of the club's 5, 7 or 3 places at
            // +0x160, +0x174, +0x190 free; kinds 6 and 7 the one at +0x94.
            const std::int32_t kind = std::int8_t(row().kind());
            const auto taken = [&](std::uint32_t at, std::int32_t places) {
                std::int32_t used = 0;
                while (used < places && c.u32(club + at + std::uint32_t(used) * 4)) ++used;
                c.set(4, std::uint32_t(used));
                return used == places;
            };
            if (kind == 8 && taken(0x160, 5)) return;
            if (kind == 9 && taken(0x174, 7)) return;
            if (kind == 10 && taken(0x190, 3)) return;
            if ((kind == 6 || kind == 7) && c.u32(club + 0x94)) return;
        }
        const std::uint32_t roles = c.u32(c.toc(game_slot::staff_roles));
        c.set(4, roles);
        if ((c.u8(roles + c.u32(person) * 100 + 0xc) >> 2) & 3 && player) return;
        if (nonzero(c, std::int8_t(program.call(0x103f43b0, 0x1036da48, {c.u32(transfers), person, 0})))) return;
        if (nonzero(c, std::int8_t(program.call(0x103ea290, 0x1036da60, {c.u32(transfers), person, 0})))) return;
        if (c.u32(person + 0x20) && c.u8(person + 0x60) == 5) return;
        if (row().flags() & 0x80) loan();
        else offer();
    };
    for (; compare(c, c.i8(index_at), 40) < 0; c.put8(index_at, c.u8(index_at) + 1u), offset = offset + Row::size) consider();
}
}
