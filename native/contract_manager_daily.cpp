#include "contract_manager_random.hpp"

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
// The date predicates retain the compiler's volatile values and carry bit:
// later callees can save them into guest memory even though C++ only needs
// the predicate's Boolean result.
bool daily_byte(Cpu& c, std::uint32_t value) {
    const auto byte = std::int8_t(value);
    c.set(0, std::uint32_t(std::int32_t(byte)));
    return nonzero(c, byte);
}
void date_image(Cpu& c, std::uint32_t destination, std::uint32_t source,
                unsigned first, unsigned second) {
    const auto date = c.u32(source), tail = c.u32(source + 4);
    c.set(first, date); c.set(second, tail);
    c.put32(destination, date); c.put32(destination + 4, tail);
}
bool dates_equal(Cpu& c, std::uint32_t first, std::uint32_t second, unsigned flag,
                 unsigned first_year, unsigned second_year, unsigned first_day, unsigned second_day) {
    bool equal = false; c.set(flag, 0);
    const auto a = c.u16(first + 2), b = c.u16(second + 2);
    c.set(first_year, a); c.set(second_year, b);
    if (compare_logical(c, a, b) == 0) {
        const auto x = c.i16(first), y = c.i16(second);
        c.set(first_day, extended(x)); c.set(second_day, extended(y));
        if (compare(c, x, y) == 0) { equal = true; c.set(flag, 1); }
    }
    compare(c, equal); return equal;
}
bool date_order(Cpu& c, std::uint32_t first, std::uint32_t second, bool greater,
                 unsigned first_year = 4, unsigned second_year = 3) {
    const auto a = c.u16(first + 2), b = c.u16(second + 2);
    c.set(first_year, a); c.set(second_year, b);
    const auto year_order = compare_logical(c, a, b);
    bool matches;
    if (year_order) matches = greater ? year_order > 0 : year_order < 0;
    else {
        const auto x = c.i16(greater ? first : second), y = c.i16(greater ? second : first);
        c.set(4, extended(x)); c.set(0, extended(y));
        const auto difference = extended(x) ^ extended(y);
        const auto half = std::int32_t(difference) >> 1;
        c.set(3, std::uint32_t(half));
        c.s.wr<1>(guest::reg::xer_ca, std::int32_t(difference) < 0 && (difference & 1));
        matches = (std::uint32_t(half) - (difference & extended(x))) >> 31;
    }
    c.set(0, matches); nonzero(c, matches); return matches;
}
std::int32_t daily_role(Cpu& c, std::uint32_t contract, unsigned output = 0, unsigned loaded = 0) {
    const auto flags = c.u8(contract + 0x38);
    c.set(loaded, flags);
    const auto role = std::int32_t(std::int8_t(flags)) >> 2;
    c.set(output, std::uint32_t(role));
    c.s.wr<1>(guest::reg::xer_ca, role < 0 && (flags & 2));
    return role;
}

class DailyContracts {
public:
    DailyContracts(Cpu& cpu, std::uint32_t manager, std::uint32_t date,
                    std::uint32_t phase, std::uint32_t scan)
        : c(cpu), frame(c, 0x10079050, 0x4c0, {.first_gpr = 13}),
          ninety_one(c, c.toc(-0x79d8)), scratch(c, c.toc(-0x7b78)), object(c, manager), today(c, date),
          contract_or_flag(c, scan), progress(c, c.toc(-0x79ac)), person(c, c.r(19)),
          primary(c, c.toc(-0x79b0)), person_index(c, c.toc(-0x7be0)), employer(c, c.toc(-0x79a4)),
          transfers(c, c.toc(-0x7b40)), club_registry(c, c.toc(-0x7bf0)), national(c, c.toc(-0x7b28)),
          renewal_date(c, c.toc(-0x79e8)), clubs(c, c.toc(-0x7be8)), constants(c, c.toc(-0x571c)),
          indices(c, c.toc(-0x7b9c)), one_year(c, c.toc(-0x79b8)), secondary(c, c.toc(-0x79a0)), p(c) {
        c.put8(frame.at(0x4e3), phase);
    }
    void run();

private:
    Cpu& c; Frame frame;
    Kept<13> ninety_one; Kept<14> scratch; Kept<15> object; Kept<16> today;
    Kept<17> contract_or_flag; Kept<18> progress; Kept<19> person; Kept<20> primary;
    Kept<21> person_index; Kept<22> employer; Kept<23> transfers; Kept<24> club_registry;
    Kept<25> national; Kept<26> renewal_date; Kept<27> clubs; Kept<28> constants;
    Kept<29> indices; Kept<30> one_year; Kept<31> secondary; Program p;

    void horizon(unsigned local, unsigned days, std::uint32_t back, std::uint32_t destination, unsigned layout = 0);
    void prune_relations();
    void update_value();
    bool review_person();
    bool review_status();
    void finish_expiry();
    void renewal_review();
    bool playing(std::uint32_t back) { return daily_byte(c, p.call(0x100c7a20, back, {c.u8(person + 0x62)})); }
    bool national_duty(std::uint32_t back) { return daily_byte(c, p.call(0x102de3c0, back, {c.u32(national), person})); }
    bool human(std::uint32_t back, std::uint32_t club, unsigned mode = 0) {
        return daily_byte(c, p.call(0x1014c2e0, back, {club, mode, 1, 0, 0}));
    }
    bool registered(std::uint32_t back, std::uint32_t club) {
        return daily_byte(c, p.call(0x102cdbf0, back, {c.u32(club_registry), club}));
    }
    std::uint32_t target_club();
    std::uint32_t current_employer(std::uint32_t back, unsigned output);
    std::int32_t status(std::uint32_t back);
    unsigned category();
    bool shortlist(std::uint32_t back);
    std::uint32_t offer(std::uint32_t back, unsigned factor = 0x18);
    void notice(std::uint32_t back, unsigned kind, unsigned risk, unsigned force) {
        p.call(0x1008c270, back, {object, primary, kind, risk, force});
    }
    void risk_notice(std::uint32_t age_back, std::uint32_t transfer_back, std::uint32_t news_back,
                      std::uint32_t cutoff, unsigned kind, unsigned force);
    void fallback_offer(unsigned local, std::uint32_t construct_back, std::uint32_t date_back,
                          std::uint32_t publish_back, bool flag_index);
    void mark_available(std::uint32_t back);
    void clear_roster(unsigned replacement);
};

void DailyContracts::horizon(unsigned local, unsigned days, std::uint32_t back,
                              std::uint32_t destination, unsigned layout) {
    p.call(0x10717890, back, {frame.at(local), today, days});
    const auto year = c.u16(frame.at(local + 2));
    if (layout >= 2) {
        c.set(4, year); c.set(3, layout == 3 ? std::uint32_t(today) : destination);
        const auto leap = c.u8(frame.at(local + 4)); c.set(0, leap);
        c.put16(destination + 2, year);
        const auto day = c.i16(frame.at(local)); c.set(4, extended(day));
        c.put16(destination, extended(day)); c.put8(destination + 4, leap);
    } else {
        const auto day = c.i16(frame.at(local));
        c.set(layout ? 3 : 5, year); c.set(layout ? 6 : 7, extended(day));
        if (!layout) c.set(3, destination);
        c.put16(destination + 2, year);
        if (!layout) c.set(6, destination);
        const auto leap = c.u8(frame.at(local + 4)); c.set(0, leap);
        c.put16(destination, extended(day)); c.put8(destination + 4, leap);
    }
}

std::uint32_t DailyContracts::target_club() {
    const auto offset = c.u32(primary + 4) * 0x248, base = c.u32(clubs);
    c.set(0, offset); c.set(4, base); return base + offset;
}
std::uint32_t DailyContracts::current_employer(std::uint32_t back, unsigned output) {
    const auto row = p.call(0x10082f20, back, {c.u32(c.toc(-0x7b7c)), person});
    const bool present = nonzero_logical(c, row); c.put32(scratch, row);
    std::uint32_t club = 0;
    if (present) {
        const auto id = c.u32(row + 4), base = c.u32(clubs);
        c.set(0, id * 0x248); c.set(3, base); club = base + id * 0x248;
    }
    c.set(output, club); return club;
}
std::int32_t DailyContracts::status(std::uint32_t back) {
    const auto value = std::int8_t(p.call(0x10380710, back, {primary}));
    c.set(0, std::uint32_t(std::int32_t(value))); return value;
}
unsigned DailyContracts::category() {
    const auto value = c.u8(primary + 0x60) & 15; c.set(0, value); return value;
}
bool DailyContracts::shortlist(std::uint32_t back) {
    const auto club = target_club();
    return nonzero_logical(c, p.call(0x1036d240, back,
        {c.u32(c.u32(transfers) + 0x988), club, person, 0, 0}));
}
std::uint32_t DailyContracts::offer(std::uint32_t back, unsigned factor) {
    c.set_f(1, c.f64(constants + factor)); c.set(10, 2);
    return p.call(0x10080d30, back, {object, person, 0, 0, 0xffffffff});
}
void DailyContracts::risk_notice(std::uint32_t age_back, std::uint32_t transfer_back,
                                  std::uint32_t news_back, std::uint32_t cutoff, unsigned kind, unsigned force) {
    contract_or_flag = 0;
    const auto age = std::int8_t(p.call(0x100c6a10, age_back, {person, cutoff}));
    c.set(0, std::uint32_t(std::int32_t(age)));
    if (compare(c, age, 24) >= 0 && daily_byte(c, p.call(0x103faff0, transfer_back, {c.u32(transfers), person})))
        contract_or_flag = 1;
    notice(news_back, kind, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(contract_or_flag)))), force);
}
void DailyContracts::clear_roster(unsigned replacement) {
    const auto club = c.u32(person + 0x24); c.set(0, club);
    if (nonzero_logical(c, club)) {
        c.set(3, 0); c.set(5, 0); c.set(7, 0);
        for (unsigned n = 0; n < 50; ++n) {
            const auto current_club = c.u32(person + 0x24), offset = 0x98 + n * 4;
            c.set(4, current_club); c.set(6, offset);
            const auto member = c.u32(current_club + offset); c.set(0, member);
            if (compare_logical(c, member, person) == 0) c.put32(current_club + offset, 0);
            c.set(5, (n + 1) * 4); c.set(7, n + 1);
        }
        c.set(0, 50); compare(c, 50, 50);
    }
    c.set(0, replacement); c.put8(person + 0x62, replacement);
}
void DailyContracts::mark_available(std::uint32_t back) {
    c.set(0, 1); c.put8(primary + 0x54, 1);
    const auto flags = c.u8(primary + 0x5c) & ~8u; c.set(0, flags); c.put8(primary + 0x5c, flags);
    p.call(0x10363ca0, back, {c.u32(c.u32(transfers) + 0x988), person, 1});
}

void DailyContracts::prune_relations() {
    primary = 0;
    while (true) {
        const auto count = c.i32(object + 0x2c); c.set(4, std::uint32_t(count));
        if (compare(c, std::int32_t(std::uint32_t(primary)), count) >= 0) break;
        progress = std::uint32_t(primary) * 24;
        const auto base = c.u32(object + 0x28), row = base + std::uint32_t(progress);
        c.set(0, base); c.set(8, c.u32(today)); c.set(7, c.u32(today + 4));
        c.put32(frame.at(0xf4), c.r(8)); c.put32(frame.at(0xf8), c.r(7));
        const auto person_id = c.u32(row + 8), people = c.u32(person_index), offset = person_id * 112;
        c.set(0, person_id); c.set(6, people); c.set(5, offset); c.set(0, offset + 0x65);
        const auto ambition = c.i8(people + offset + 0x65);
        const auto doubled = std::uint32_t(std::int32_t(ambition)) * 2;
        const auto wait = 50u - doubled; c.set(0, wait);
        c.s.wr<1>(guest::reg::xer_ca, 50u >= doubled);
        p.call(0x10717890, 0x10079338, {frame.at(0x144), row + 12, extended(std::int16_t(wait))});
        if (date_order(c, frame.at(0xf4), frame.at(0x144), true, 3, 0)) {
            const auto length = c.u32(object + 0x2c), records = c.u32(object + 0x28), last = (length - 1) * 24;
            c.set(0, last);
            p.call(0x10739ef0, 0x100793a4, {records + std::uint32_t(progress), records + last, 24});
            const auto remaining = c.u32(object + 0x2c) - 1;
            c.set(3, remaining + 1); primary = std::uint32_t(primary) - 1;
            c.set(0, remaining); c.put32(object + 0x2c, remaining);
        }
        primary = std::uint32_t(primary) + 1;
    }
    if (nonzero(c, c.i32(object + 0x2c)))
        p.call(0x10768300, 0x100793dc, {c.u32(object + 0x28), c.u32(object + 0x2c), 24, c.toc(-0x5720)});
}

void DailyContracts::update_value() {
    employer = current_employer(0x10079544, 22);
    contract_or_flag = p.call(0x10082f20, 0x10079574, {object, person});
    bool handled = false;
    if (nonzero_logical(c, employer) && nonzero_logical(c, contract_or_flag) &&
        human(0x100795a0, employer, 1) && playing(0x100795b0)) {
        const auto flags = c.u8(contract_or_flag + 0x5c) & 4; c.set(0, flags);
        if (nonzero(c, flags)) {
            const auto attributes = c.u32(person + 0x14); c.set(0, attributes);
            if (nonzero_logical(c, attributes)) {
                const auto value = p.call(0x10084be0, 0x100795ec, {object, person, employer, 0, 0xffffffff, 1});
                c.put32(person + 8, value);
            }
        }
        handled = true;
    }
    if (!handled) {
        bool ordinary_player = false;
        if (nonzero_logical(c, employer) && nonzero_logical(c, contract_or_flag) && playing(0x10079630)) {
            const auto day = std::int32_t(c.i16(today)); const auto sign = std::uint32_t(day) >> 31;
            const auto parity = ((std::uint32_t(day) & 1) ^ sign) - sign;
            c.set(3, sign); c.set(0, parity);
            if (!nonzero(c, std::int32_t(parity))) return;
            ordinary_player = true;
        }
        if (ordinary_player || (nonzero_logical(c, employer) && nonzero_logical(c, contract_or_flag))) {
            const auto flags = c.u8(contract_or_flag + 0x5c) | 4u; c.set(0, flags); c.put8(contract_or_flag + 0x5c, flags);
            const auto value = p.call(0x10084be0, ordinary_player ? 0x10079678 : 0x100796dc,
                                      {object, person, employer, 0, 0xffffffff, 0});
            c.put32(person + 8, value);
            if (!ordinary_player) return;
        } else { c.set(0, 0); c.put32(person + 8, 0); return; }
    }
    if (daily_byte(c, p.call(0x10380520, handled ? 0x100795f8 : 0x10079684, {contract_or_flag}))) {
        const auto value = c.i32(person + 8), maximum = c.i32(contract_or_flag + 0x24);
        c.set(0, std::uint32_t(value)); c.set(3, std::uint32_t(maximum));
        if (compare(c, value, maximum) > 0) c.put32(person + 8, std::uint32_t(maximum));
    }
}

void DailyContracts::renewal_review() {
    horizon(0x13c, 7, 0x1007982c, c.toc(-0x79c4));
    horizon(0x134, 30, 0x1007985c, c.toc(-0x79c8));
    horizon(0x12c, 42, 0x1007988c, c.toc(-0x79cc));
    horizon(0x124, 60, 0x100798bc, c.toc(-0x79d0));
    horizon(0x11c, 70, 0x100798ec, c.toc(-0x79d4));
    horizon(0x114, 91, 0x1007991c, ninety_one, 3);
    const auto days = p.call(0x10718130, 0x10079944, {today, c.u32(primary + 0x28), c.u32(primary + 0x2c)});
    if (compare(c, std::int32_t(days), 180) <= 0) return;
    date_image(c, frame.at(0xd4), primary + 0x30, 3, 0);
    if (!date_order(c, frame.at(0xd4), c.toc(-0x79c0), false)) return;
    bool due = false;
    for (auto [slot, local] : {std::pair{-0x79c4, 0xcc}, {-0x79c8, 0xc4}, {-0x79cc, 0xbc}, {-0x79d0, 0xb4}, {-0x79d4, 0xac}}) {
        c.set(3, c.toc(slot));
        date_image(c, frame.at(local), c.toc(slot), slot == -0x79c4 ? 0 : 6, 4);
        if (dates_equal(c, frame.at(local), renewal_date, 5, 3, 0, 4, 3)) { due = true; break; }
    }
    if (!due) {
        date_image(c, frame.at(0xa4), ninety_one, 3, 5);
        due = dates_equal(c, frame.at(0xa4), renewal_date, 4, 3, 0, 3, 0);
    }
    if (!due) return;
    contract_or_flag = 0;
    if (daily_byte(c, p.call(0x100d94d0, 0x10079b40, {person, employer}))) contract_or_flag = 1;
    const auto owner = c.u32(employer + 0x90); c.set(4, owner);
    if (nonzero_logical(c, owner) && daily_byte(c, p.call(0x100d9580, 0x10079b60, {person, owner}))) contract_or_flag = 1;
    c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(contract_or_flag)))));
    if (nonzero(c, std::int8_t(std::uint32_t(contract_or_flag))) || human(0x10079b8c, employer)) return;
    const auto age = std::int8_t(c.i16(person + 0x5c));
    c.set(0, extended(c.i16(person + 0x5c))); c.set(3, std::uint32_t(std::int32_t(age)));
    if (compare(c, age, 32) >= 0) return;
    const auto kind = category();
    const auto proposal_for_player = [&](std::uint32_t high_back, std::uint32_t young_back) {
        const auto attributes = c.u32(person + 0x14); c.set(4, attributes);
        const auto reputation = c.i16(attributes + 10); c.set(0, extended(reputation));
        if (compare(c, reputation, 6750) > 0) { offer(high_back); return; }
        if (compare(c, age, 24) >= 0) return;
        const auto ability = c.i16(attributes + 6); c.set(0, extended(ability));
        if (compare(c, ability, 100) > 0) offer(young_back);
    };
    if (compare(c, kind, 3) == 0) proposal_for_player(0x10079c94, 0x10079ccc);
    else if (kind < 3) {
        const auto relative = compare(c, kind, 1);
        if (!relative) offer(0x10079bf4);
        else if (relative > 0) proposal_for_player(0x10079c28, 0x10079c60);
    } else if (compare(c, kind, 5) == 0) {
        const auto attributes = c.u32(person + 0x14); c.set(3, attributes);
        const auto ability = c.i16(attributes + 6); c.set(0, extended(ability));
        if (compare(c, ability, 100) > 0) offer(0x10079d00);
    }
}

void DailyContracts::fallback_offer(unsigned local, std::uint32_t construct_back,
                                     std::uint32_t date_back, std::uint32_t publish_back, bool flag_index) {
    const auto proposal = frame.at(local);
    p.call(0x103721f0, construct_back, {proposal});
    p.call(0x10715880, date_back, {proposal + 0x68});
    const auto person_id = c.u32(person); c.set(3, person_id); c.set(0, 2);
    const auto role = (c.u8(proposal + 0x38) & ~0xfcu) | 8u;
    c.set(flag_index ? 9 : 6, role); c.put32(proposal, person_id);
    const auto previous_category = c.u8(proposal + 0x60); const auto club_id = c.u32(employer);
    if (flag_index) {
        c.set(3, person_id * 100); c.set(0, 1); c.set(6, previous_category); c.set(4, club_id); c.set(10, 0xffffffff);
        c.set(7, person_id * 100 + 12); c.set(8, c.u32(indices));
    } else { c.set(8, 1); c.set(7, 0xffffffff); c.set(0, previous_category); c.set(9, club_id); }
    c.put32(proposal + 4, club_id); c.put8(proposal + 0x64, 1); c.put32(proposal + 0x70, 0xffffffff);
    c.put8(proposal + 0x38, role);
    const auto kind = c.u8(person + 0x62); c.set(flag_index ? 9 : 6, kind); c.put8(proposal + 0x78, kind);
    const auto source_category = c.u8(primary + 0x60);
    const auto packed_category = (previous_category & ~15u) | (source_category & 15u);
    c.set(flag_index ? 9 : 6, source_category); c.set(flag_index ? 6 : 0, packed_category);
    c.put8(proposal + 0x60, packed_category);
    if (flag_index) {
        const auto offset = person_id * 100 + 12, base = c.u32(indices), flags = c.u8(base + offset) | 1u;
        c.set(6, flags); c.put8(base + offset, flags);
    }
    p.call(0x1008a350, publish_back, {object, proposal, 1});
}

bool DailyContracts::review_person() {
    update_value();
    secondary = p.call(0x10083110, 0x100796f8, {object, person});
    if (nonzero(c, std::int32_t(std::uint32_t(secondary)))) {
        date_image(c, frame.at(0xe4), today, 0, 5);
        if (dates_equal(c, frame.at(0xe4), secondary + 0x30, 4, 3, 0, 3, 0)) {
            const auto club = c.u32(person + 0x24); c.set(0, club);
            if (!nonzero_logical(c, club)) {
                const auto id = c.u32(person), base = c.u32(indices), offset = id * 100 + 4;
                c.set(0, offset); c.set(3, id * 100); c.set(4, base); c.set(5, 0xffffffff);
                c.put32(base + offset, 0xffffffff);
            } else {
                if (compare(c, daily_role(c, secondary), 7) != 0)
                    p.call(0x10090a00, 0x10079794, {object, person, 6, 0});
                p.call(0x100845b0, 0x100797a0, {object, person});
            }
        }
    }
    primary = p.call(0x10082f20, 0x100797ac, {object, person});
    if (!nonzero(c, std::int32_t(std::uint32_t(primary)))) return false;
    const auto attributes = c.u32(person + 0x14); c.set(0, attributes);
    if (nonzero_logical(c, attributes)) {
        date_image(c, frame.at(0xdc), renewal_date, 3, 0);
        if (date_order(c, frame.at(0xdc), today, true)) renewal_review();
    }
    c.set(3, c.toc(-0x79b4)); date_image(c, frame.at(0x9c), c.toc(-0x79b4), 4, 0);
    if (date_order(c, frame.at(0x9c), primary + 0x30, false) && compare(c, daily_role(c, primary), 2) == 0 && !nonzero_logical(c, secondary)) {
        if (daily_byte(c, p.call(0x10380f20, 0x10079d94, {primary, 0, frame.at(0x38), 0}))) return false;
        const auto possible = c.i8(frame.at(0x38)); c.set(0, std::uint32_t(std::int32_t(possible)));
        if (!nonzero(c, possible) || national_duty(0x10079db4)) return false;
        const auto type = c.u8(person + 0x62); c.set(0, type);
        if (compare(c, type, 11) != 0) return false;
        if (!human(0x10079de0, employer)) {
            const auto age = std::int8_t(c.i16(person + 0x5c));
            c.set(0, extended(c.i16(person + 0x5c))); c.set(3, std::uint32_t(std::int32_t(age)));
            if (compare(c, age, 32) < 0) {
                const auto kind = category();
                if (compare(c, kind, 4) != 0) {
                    if (kind < 4) {
                        if (compare(c, kind, 3) >= 0) {
                            const auto stats = c.u32(person + 0x14); c.set(4, stats);
                            const auto reputation = c.i16(stats + 10); c.set(0, extended(reputation));
                            if (compare(c, reputation, 6750) > 0) offer(0x10079e80);
                            else if (compare(c, age, 24) < 0) {
                                const auto ability = c.i16(stats + 6); c.set(0, extended(ability));
                                if (compare(c, ability, 100) > 0) offer(0x10079eb8);
                            }
                        } else if (compare(c, kind, 1) >= 0) offer(0x10079e4c);
                    } else if (compare(c, kind, 6) < 0) {
                        const auto stats = c.u32(person + 0x14); c.set(3, stats);
                        const auto ability = c.i16(stats + 6); c.set(0, extended(ability));
                        if (compare(c, ability, 100) > 0) offer(0x10079eec);
                    }
                }
            }
        }
        if (registered(0x10079f04, target_club()))
            risk_notice(0x10079f1c, 0x10079f34, 0x10079f58, c.toc(-0x79b4), 7, 1);
        return false;
    }
    if (compare(c, daily_role(c, primary), 1) == 0) {
        const auto other = p.call(0x10083110, 0x10079f80, {object, person});
        if (nonzero_logical(c, other)) return true;
        date_image(c, frame.at(0x94), today, 4, 0);
        p.call(0x10717890, 0x10079fa8, {frame.at(0x10c), primary + 0x30, 31});
        if (!date_order(c, frame.at(0x94), frame.at(0x10c), true, 3, 0)) return true;
        if (!daily_byte(c, p.call(0x1037f110, 0x10079ffc, {primary}))) {
            const auto active = c.i8(primary + 0x54); c.set(0, std::uint32_t(std::int32_t(active)));
            if (!nonzero(c, active)) return true;
        }
        if (nonzero(c, std::int32_t(p.call(0x103eaea0, 0x1007a024, {c.u32(transfers), person, 0, 0}))) ||
            nonzero(c, std::int32_t(p.call(0x103eb020, 0x1007a03c, {c.u32(transfers), person, 0}))) ||
            daily_byte(c, p.call(0x103e6470, 0x1007a050, {c.u32(transfers), person}))) return true;
        if (registered(0x1007a070, target_club())) p.call(0x10090a00, 0x1007a08c, {object, person, 1, 0});
        const auto type = c.i8(person + 0x62); c.set(0, std::uint32_t(std::int32_t(type)));
        if (compare(c, type, 12) == 0) { clear_roster(5); return false; }
        if (compare(c, type, 13) == 0 && !human(0x1007a11c, target_club())) { clear_roster(6); return false; }
        const auto again = c.u8(person + 0x62); c.set(0, again);
        if (compare(c, again, 15) == 0 && !human(0x1007a1ac, target_club())) { clear_roster(8); return false; }
        p.call(0x103e6290, 0x1007a21c, {c.u32(transfers), person, 1, 0xffffffff}); return false;
    }
    return review_status();
}

bool DailyContracts::review_status() {
    const auto attributes = c.u32(person + 0x14); c.set(0, attributes);
    bool listed_player = false;
    if (nonzero_logical(c, attributes)) {
        const auto type = c.u8(person + 0x62); c.set(0, type);
        if (compare(c, type, 11) == 0 && !national_duty(0x1007a244) &&
            !daily_byte(c, p.call(0x103f43b0, 0x1007a25c, {c.u32(transfers), person, 0}))) {
            const auto active = c.i8(primary + 0x54); c.set(0, std::uint32_t(std::int32_t(active)));
            if (nonzero(c, active)) listed_player = true;
        }
    }
    if (listed_player) {
        const auto state = status(0x1007a278);
        if (compare_logical(c, std::uint32_t(state), 13) <= 0) {
            const auto table = c.toc(-0x5728), offset = std::uint32_t(state) * 4;
            c.set(0, offset); c.set(3, table);
            const auto destination = c.u32(table + offset); c.set(3, destination); c.s.wr<4>(guest::reg::ctr, destination);
            if (state & 1) p.call(0x10363ca0, 0x1007a2ac, {c.u32(c.u32(transfers) + 0x988), person, 1});
        }
        return true;
    }
    const auto active = c.i8(primary + 0x54); c.set(0, std::uint32_t(std::int32_t(active)));
    if (nonzero(c, active) || daily_byte(c, p.call(0x103f43b0, 0x1007a2cc, {c.u32(transfers), person, 0}))) return true;
    if (compare(c, daily_role(c, primary), 2) != 0) return true;
    p.call(0x10717b20, 0x1007a2fc, {frame.at(0x104), today, 93});
    date_image(c, frame.at(0x8c), frame.at(0x104), 3, 0);
    if (!date_order(c, frame.at(0x8c), primary + 0x28, true)) return true;

    bool review_recruitment = compare(c, status(0x1007a360), 14) != 0;
    if (review_recruitment) review_recruitment = nonzero(c, status(0x1007a374));
    if (review_recruitment) review_recruitment = !national_duty(0x1007a388);
    if (review_recruitment) {
        const auto player = c.u32(person + 0x14); c.set(0, player);
        review_recruitment = nonzero_logical(c, player);
    }
    if (review_recruitment) {
        const auto kind = c.u8(person + 0x62); c.set(0, kind);
        review_recruitment = compare(c, kind, 11) == 0;
    }
    if (review_recruitment) {
        contract_or_flag = 0;
        const auto row = p.call(0x10082f20, 0x1007a3bc, {c.u32(c.toc(-0x7b7c)), person});
        const bool present = nonzero_logical(c, row); c.put32(scratch, row);
        if (present && compare(c, daily_role(c, row), 5) != 0) contract_or_flag = 1;
        std::uint32_t club = 0;
        if (nonzero(c, std::int32_t(std::uint32_t(contract_or_flag)))) {
            const auto current = c.u32(scratch), base = c.u32(clubs), offset = c.u32(current + 4) * 0x248;
            c.set(4, base); c.set(0, offset); club = base + offset;
        }
        c.set(3, club);
        if (!nonzero_logical(c, club)) return false;
        const auto division = c.u32(club + 0x2c); c.set(3, division);
        if (!nonzero_logical(c, division)) return false;
        const auto player = c.u32(person + 0x14); c.set(4, player);
        const auto reputation = c.i16(player + 10); c.set(0, extended(reputation));
        bool seek_transfer = compare(c, reputation, 3750) > 0;
        if (seek_transfer) { const auto international = c.i16(player + 12); c.set(0, extended(international)); seek_transfer = compare(c, international, 1750) > 0; }
        if (seek_transfer) { const auto ambition = c.i8(person + 0x65); c.set(0, std::uint32_t(std::int32_t(ambition))); seek_transfer = compare(c, ambition, 15) < 0; }
        if (seek_transfer) {
            const auto age = std::int8_t(c.i16(person + 0x5c)); c.set(0, std::uint32_t(std::int32_t(age)));
            seek_transfer = compare(c, age, 31) < 0 && compare(c, age, 18) > 0;
        }
        if (seek_transfer) seek_transfer = daily_byte(c, p.call(0x100d33d0, 0x1007a464));
        if (seek_transfer) {
            const auto home = c.u32(person + 0x24); c.set(3, home);
            if (nonzero_logical(c, home)) {
                const auto quality = c.i16(home + 0x1ec); c.set(0, extended(quality));
                if (compare(c, quality, 4750) > 0) {
                    const auto age = std::int8_t(c.i16(person + 0x5c)); c.set(0, std::uint32_t(std::int32_t(age)));
                    if (compare(c, age, 23) < 0) seek_transfer = false;
                }
            }
        }
        if (seek_transfer && !daily_byte(c, p.call(0x1037f110, 0x1007a49c, {primary}))) {
            const auto flags = c.u8(primary + 0x5c) & 8; c.set(0, flags); seek_transfer = nonzero(c, flags);
        }
        if (seek_transfer) {
            mark_available(0x1007a4d8);
            p.call(0x1008c8d0, 0x1007a4e8, {object, primary, 0}); return false;
        }
        const auto state = status(0x1007a4f4);
        if (compare_logical(c, std::uint32_t(state), 14) <= 0) {
            const auto table = c.toc(-0x572c), offset = std::uint32_t(state) * 4;
            c.set(3, table); c.set(0, offset);
            const auto destination = c.u32(table + offset); c.set(3, destination); c.s.wr<4>(guest::reg::ctr, destination);
            if (state == 14) {
                const auto cursor_holder = c.toc(-0x7b84), next = c.u32(cursor_holder) + 2, end = c.u32(c.toc(-0x7b88));
                c.set(3, cursor_holder); c.set(4, next); c.set(0, end);
                const bool wraps = compare_logical(c, next, end) > 0; c.put32(cursor_holder, next);
                if (wraps) {
                    c.set(0, c.toc(-0x7b8c)); c.put32(cursor_holder, c.r(0));
                    const auto entropy = p.call(0x1077b770, 0x1007a544); c.set(0, entropy & 0xffff);
                    c.set(3, c.toc(-0x7b90)); c.put32(c.r(3), entropy & 0xffff);
                }
                const auto sample_address = c.u32(cursor_holder), salt = c.u32(c.toc(-0x7b90));
                const auto draw = c.u16(sample_address) + salt;
                const auto high = multiply_high(0x66666667, std::int32_t(draw)), floor = high >> 3;
                const auto sign = std::uint32_t(floor) >> 31, quotient = std::uint32_t(floor) + sign;
                c.set(5, sample_address); c.set(3, sign); c.set(4, draw); c.set(0, draw - quotient * 20);
                c.s.wr<1>(guest::reg::xer_ca, high < 0 && (std::uint32_t(high) & 7));
                if (!nonzero(c, std::int32_t(draw - quotient * 20)))
                    p.call(0x10363ca0, 0x1007a5a0, {c.u32(c.u32(transfers) + 0x988), person, 1});
            } else if (state & 1) p.call(0x10363ca0, 0x1007a5b8, {c.u32(c.u32(transfers) + 0x988), person, 1});
        }
    }

    c.set(3, c.toc(-0x79bc)); date_image(c, frame.at(0x84), c.toc(-0x79bc), 0, 5);
    if (dates_equal(c, frame.at(0x84), primary + 0x30, 4, 3, 0, 3, 0) && playing(0x1007a604) && !national_duty(0x1007a618)) {
        if (human(0x1007a644, target_club())) return true;
        const auto kind = category();
        if (compare(c, kind, 4) < 0 && compare(c, kind, 1) >= 0 && shortlist(0x1007a690)) offer(0x1007a6b8);
        return true;
    }
    date_image(c, frame.at(0x7c), one_year, 6, 4);
    c.put32(frame.at(0x44), c.r(6)); c.put32(frame.at(0x48), c.r(4));
    bool annual = dates_equal(c, frame.at(0x7c), primary + 0x30, 5, 3, 0, 4, 3);
    if (!annual) {
        c.set(3, c.toc(-0x7a9c)); date_image(c, frame.at(0x74), c.toc(-0x7a9c), 4, 6);
        if (dates_equal(c, frame.at(0x74), today, 5, 4, 3, 4, 3)) {
            date_image(c, frame.at(0x6c), frame.at(0x44), 4, 3);
            annual = date_order(c, frame.at(0x6c), primary + 0x30, true, 4, 0);
        }
    }
    if (annual && playing(0x1007a7a8) && !national_duty(0x1007a7bc)) {
        date_image(c, frame.at(0x64), one_year, 0, 5);
        if (dates_equal(c, frame.at(0x64), primary + 0x30, 4, 3, 0, 3, 0))
            risk_notice(0x1007a814, 0x1007a82c, 0x1007a850, one_year, 6, 0);
        if (human(0x1007a874, target_club())) return true;
        const auto kind = category(); bool suitable = false;
        const auto compared = compare(c, kind, 4);
        if (compared < 0) suitable = compare(c, kind, 1) >= 0;
        else if (compared > 0) suitable = compare(c, kind, 6) < 0;
        if (suitable && shortlist(0x1007a8cc)) {
            if (!daily_byte(c, offer(0x1007a8f4))) fallback_offer(0x3c4, 0x1007a904, 0x1007a90c, 0x1007a964, false);
        }
        return true;
    }
    const bool near_end = compare(c, status(0x1007a970), 3) == 0 || compare(c, status(0x1007a984), 5) == 0;
    if (near_end && playing(0x1007a998) && !national_duty(0x1007a9ac)) {
        if (human(0x1007a9d8, target_club())) return true;
        const auto kind = category();
        if (compare(c, kind, 4) < 0) {
            if (compare(c, kind, 1) >= 0) {
                const bool selected = shortlist(0x1007aa30);
                offer(selected ? 0x1007aa58 : 0x1007aa7c, selected ? 0x20 : 0x18);
            }
        } else if (compare(c, kind, 6) < 0) offer(0x1007aaa0);
        return true;
    }

    if (compare(c, status(0x1007aaac), 1) == 0 && !national_duty(0x1007aac4)) {
        const auto player = c.u32(person + 0x14); c.set(0, player);
        bool has_player = nonzero_logical(c, player);
        if (has_player) { const auto type = c.u8(person + 0x62); c.set(0, type); has_player = compare(c, type, 11) == 0; }
        if (has_player) risk_notice(0x1007ab54, 0x1007ab6c, 0x1007ab90, c.toc(-0x79b4), 5, 1);
        else if (registered(0x1007aafc, target_club())) notice(0x1007ab1c, 5, 0, 0);
        else offer(0x1007ab40);
        if (!playing(0x1007ab98) || human(0x1007abc4, target_club())) return true;
        const auto kind = category();
        if (compare(c, kind, 4) < 0) {
            if (compare(c, kind, 1) >= 0) {
                const bool selected = shortlist(0x1007ac1c);
                if (!daily_byte(c, offer(selected ? 0x1007ac44 : 0x1007acf0, selected ? 0x28 : 0x18)))
                    fallback_offer(selected ? 0x33c : 0x2b4, selected ? 0x1007ac54 : 0x1007ad00,
                                   selected ? 0x1007ac5c : 0x1007ad08, selected ? 0x1007accc : 0x1007ad78, true);
            }
        } else if (compare(c, kind, 7) < 0) offer(0x1007ad9c);
        return true;
    }
    if (compare(c, status(0x1007ada8), 7) == 0 && !national_duty(0x1007adc0)) {
        const auto player = c.u32(person + 0x14); c.set(0, player);
        bool has_player = nonzero_logical(c, player);
        if (has_player) { const auto type = c.u8(person + 0x62); c.set(0, type); has_player = compare(c, type, 11) == 0; }
        if (!has_player) {
            if (registered(0x1007adf8, target_club())) notice(0x1007ae18, 1, 0, 0);
            else offer(0x1007ae3c);
            return true;
        }
        if (registered(0x1007ae58, target_club())) {
            risk_notice(0x1007ae70, 0x1007ae88, 0x1007aeac, c.toc(-0x79a8), 1, 0); return true;
        }
        if (!playing(0x1007aeb8)) return true;
        const auto kind = category();
        const auto compared = compare(c, kind, 6);
        if (!compared) offer(0x1007b108);
        else if (compared < 0) {
            if (compare(c, kind, 3) >= 0) {
                const bool selected = shortlist(0x1007b098);
                offer(selected ? 0x1007b0c0 : 0x1007b0e4, selected ? 0x28 : 0x18);
            } else if (compare(c, kind, 1) >= 0) {
                const bool selected = shortlist(0x1007af10);
                if (!daily_byte(c, offer(selected ? 0x1007af38 : 0x1007afe4, selected ? 0x30 : 0x18)))
                    fallback_offer(selected ? 0x22c : 0x1a4, selected ? 0x1007af48 : 0x1007aff4,
                                   selected ? 0x1007af50 : 0x1007affc, selected ? 0x1007afc0 : 0x1007b06c, true);
            }
        }
        return true;
    }
    const bool long_notice = compare(c, status(0x1007b114), 9) == 0 || compare(c, status(0x1007b128), 11) == 0;
    if (long_notice && playing(0x1007b13c) && !national_duty(0x1007b150)) {
        if (human(0x1007b17c, target_club())) return true;
        const auto kind = category(); const auto compared = compare(c, kind, 6);
        if (!compared) offer(0x1007b2bc);
        else if (compared < 0) {
            if (compare(c, kind, 3) >= 0) {
                const bool selected = shortlist(0x1007b24c);
                offer(selected ? 0x1007b274 : 0x1007b298, selected ? 0x20 : 0x18);
            } else if (compare(c, kind, 1) >= 0) {
                const bool selected = shortlist(0x1007b1d4);
                offer(selected ? 0x1007b1fc : 0x1007b220, selected ? 0x38 : 0x18);
            }
        }
        return true;
    }
    if (compare(c, status(0x1007b2c8), 13) == 0 && !national_duty(0x1007b2e0)) {
        const auto player = c.u32(person + 0x14); c.set(0, player);
        bool has_player = nonzero_logical(c, player);
        if (has_player) { const auto type = c.u8(person + 0x62); c.set(0, type); has_player = compare(c, type, 11) == 0; }
        if (!has_player) {
            if (registered(0x1007b318, target_club())) notice(0x1007b338, 4, 0, 0);
            else offer(0x1007b35c);
            return true;
        }
        if (registered(0x1007b378, target_club())) {
            risk_notice(0x1007b390, 0x1007b3a8, 0x1007b3cc, c.toc(-0x799c), 4, 0); return true;
        }
        const auto kind = category(); const auto compared = compare(c, kind, 6);
        if (!compared) offer(0x1007b508);
        else if (compared < 0) {
            if (compare(c, kind, 3) >= 0) {
                const bool selected = shortlist(0x1007b498);
                offer(selected ? 0x1007b4c0 : 0x1007b4e4, selected ? 0x20 : 0x18);
            } else if (compare(c, kind, 1) >= 0) {
                const bool selected = shortlist(0x1007b420);
                offer(selected ? 0x1007b448 : 0x1007b46c, selected ? 0x38 : 0x18);
            }
        }
        return true;
    }

    date_image(c, frame.at(0x5c), today, 3, 0);
    if (date_order(c, frame.at(0x5c), primary + 0x30, false)) return true;
    const auto role = (c.u8(primary + 0x38) & ~0xfcu) | 4u;
    c.set(0, role); c.put8(primary + 0x38, role);
    p.call(0x10090a00, 0x1007b58c, {object, person, 2, 0});
    const auto player = c.u32(person + 0x14); c.set(0, player);
    bool has_player = nonzero_logical(c, player);
    if (has_player) { const auto type = c.u8(person + 0x62); c.set(0, type); has_player = compare(c, type, 11) == 0; }
    if (!has_player) {
        if (!human(0x1007b5c8, target_club())) offer(0x1007b5f0);
        return true;
    }
    bool available = daily_byte(c, p.call(0x1037f110, 0x1007b5fc, {primary}));
    if (!available) { const auto flags = c.u8(primary + 0x5c) & 8; c.set(0, flags); available = nonzero(c, flags); }
    if (available) { mark_available(0x1007b638); return true; }
    if (nonzero_logical(c, current_employer(0x1007b64c, 0))) {
        const auto club = current_employer(0x1007b688, 4);
        if (registered(0x1007b6b4, club)) return true;
    }
    if (human(0x1007b6e0, target_club())) return true;
    const auto kind = category(); const auto compared = compare(c, kind, 6);
    if (compared <= 0 && compare(c, kind, 1) >= 0) {
        const auto id = c.u32(person), base = c.u32(indices), offset = id * 100 + 4;
        c.set(0, c.u32(base + offset)); c.set(3, id * 100); c.set(4, base);
        if (compare(c, c.i32(base + offset), -1) == 0) return true;
        if (kind == 6) offer(0x1007b874);
        else if (kind >= 3) {
            const bool selected = shortlist(0x1007b7e8);
            offer(selected ? 0x1007b810 : 0x1007b834, selected ? 0x20 : 0x18);
        } else {
            const bool selected = shortlist(0x1007b754);
            offer(selected ? 0x1007b77c : 0x1007b7a0, selected ? 0x38 : 0x18);
        }
        return true;
    }
    const auto club = c.u32(person + 0x24); c.set(0, club);
    if (!nonzero_logical(c, club)) return true;
    const auto stats = c.u32(person + 0x14); c.set(0, stats);
    if (!nonzero_logical(c, stats)) return true;
    const auto type = c.u8(person + 0x62); c.set(0, type);
    if (compare(c, type, 11) != 0 || !daily_byte(c, p.call(0x100d3040, 0x1007b8a8, {person, 0}))) return true;
    if (nonzero(c, std::int32_t(p.call(0x103eaea0, 0x1007b8c4, {c.u32(transfers), person, 0, 0}))) ||
        nonzero(c, std::int32_t(p.call(0x103eb020, 0x1007b8dc, {c.u32(transfers), person, 0}))) ||
        daily_byte(c, p.call(0x103e6470, 0x1007b8f0, {c.u32(transfers), person}))) return true;
    p.call(0x103e6290, 0x1007b90c, {c.u32(transfers), person, 1, 0xffffffff}); return true;
}

void DailyContracts::finish_expiry() {
    date_image(c, frame.at(0x54), today, 6, 4);
    c.put32(frame.at(0x3c), c.r(6)); c.put32(frame.at(0x40), c.r(4));
    const bool due = dates_equal(c, frame.at(0x54), primary + 0x30, 5, 3, 0, 4, 3);
    if (due && compare(c, daily_role(c, primary, 3, 3), 5) == 0) {
        p.call(0x10090a00, 0x1007b980, {object, person, 5, 0});
        p.call(0x10308350, 0x1007b9a4, {c.u32(c.toc(-0x7b3c)), person, 0xffffffff, 0, 0, 0xffffffff, 0});
        p.call(0x10084a80, 0x1007b9b0, {object, person});
        p.call(0x103afb90, 0x1007b9cc, {c.u32(c.toc(-0x7a6c)), c.u32(person + 0x24), person, 0, 1});
        p.call(0x103afb90, 0x1007b9e8, {c.u32(c.toc(-0x7a6c)), c.u32(person + 0x24), person, 1, 1});
        p.call(0x100c8930, 0x1007ba00, {c.u32(person + 0x24), person, 0, 1, 0}); return;
    }
    date_image(c, frame.at(0x4c), frame.at(0x3c), 3, 5);
    if (!dates_equal(c, frame.at(0x4c), primary + 0x30, 4, 3, 0, 3, 0)) return;
    const auto flags = c.u8(primary + 0x38);
    if (compare(c, daily_role(c, primary, 0, 3), 5) == 0) return;
    const auto expired = (flags & ~0xfcu) | 4u;
    c.set(0, 1); c.set(3, expired); c.put8(primary + 0x38, expired);
    const auto active = c.i8(primary + 0x54); c.set(0, std::uint32_t(std::int32_t(active)));
    if (!nonzero(c, active) || nonzero_logical(c, p.call(0x10083110, 0x1007ba7c, {object, person})) ||
        nonzero(c, std::int32_t(p.call(0x103eaea0, 0x1007ba98, {c.u32(transfers), person, 0, 0})))) return;
    p.call(0x10090a00, 0x1007bab4, {object, person, 2, 1});
    p.call(0x103e68c0, 0x1007bad0, {c.u32(transfers), person, c.u32(person + 0x24), 0, 0xffffffff, 0});
}

void DailyContracts::run() {
    c.set(0, 0); c.put8(frame.at(0x45c), 0);
    person = c.u32(person_index);
    p.call(0x107156f0, 0x100790d0, {frame.at(0x19c), 31, 11, c.u16(today + 2), 0xffffffff});
    date_image(c, frame.at(0xfc), c.r(3), 0, 5);
    if (dates_equal(c, frame.at(0xfc), today, 4, 3, 0, 3, 0)) { c.set(0, 1); c.put8(frame.at(0x45c), 1); }
    const auto phase = c.i8(frame.at(0x4e3)); c.set(0, std::uint32_t(std::int32_t(phase)));
    if (!nonzero(c, phase)) {
        horizon(0x194, 7, 0x10079134, c.toc(-0x799c));
        horizon(0x18c, 30, 0x10079164, secondary, 1);
        horizon(0x184, 60, 0x1007918c, employer, 1);
        horizon(0x17c, 91, 0x100791b4, c.toc(-0x79a8));
        horizon(0x174, 121, 0x100791e4, progress, 1);
        horizon(0x16c, 152, 0x1007920c, primary, 1);
        horizon(0x164, 182, 0x10079234, c.toc(-0x79b4));
        horizon(0x15c, 365, 0x10079264, one_year, 1);
        horizon(0x154, 550, 0x1007928c, c.toc(-0x79bc));
        horizon(0x14c, 1095, 0x100792bc, c.toc(-0x79c0), 2);
    }
    const auto raw_phase = c.u8(frame.at(0x4e3)); c.set(0, raw_phase);
    if (compare(c, raw_phase, 2) == 0) {
        prune_relations();
        c.set(0, 0x43300000); c.put32(frame.at(0x454), 0x43300000);
        const auto count_holder = c.toc(-0x7be4); c.set(3, count_holder); progress = 0;
        const auto annual = c.i8(frame.at(0x45c)); c.set(0, std::uint32_t(std::int32_t(annual)));
        const auto count = c.u32(count_holder); c.set(3, count); nonzero(c, annual);
        const auto bias = c.f64(constants + 8), scale = c.f64(constants + 0x10);
        c.set_f(1, bias); c.set_f(2, scale); c.set(0, count ^ 0x80000000u); c.put32(frame.at(0x458), count ^ 0x80000000u);
        guest::Fpu fp(c);
        c.set_f(0, c.f64(frame.at(0x454))); const auto converted = fp.fsub(c.f(0), bias); c.set_f(0, converted);
        const auto total = fp.fmul(scale, converted); c.set_f(0, total);
        c.put64(frame.at(0x44c), fp.fctiwz(0, total)); c.set(0, c.u32(frame.at(0x450))); c.put32(frame.at(0x460), c.r(0));
        if (annual) {
            p.call(0x10727890, 0x10079438, {c.toc(-0x7bb8), c.toc(-0x5718) + 0x228});
            p.call(0x10743120, 0x10079450, {c.toc(-0x7bb0), c.toc(-0x7bb8), 0, c.u32(frame.at(0x460)), 0xffffffff});
        }
        const auto scan = std::int8_t(std::uint32_t(contract_or_flag)); c.set(0, std::uint32_t(std::int32_t(scan)));
        person_index = 0; c.put32(frame.at(0x464), std::uint32_t(std::int32_t(scan)));
        while (true) {
            const auto holder = c.toc(-0x7be4); const auto people = c.i32(holder);
            c.set(3, holder); c.set(0, std::uint32_t(people));
            if (compare(c, std::int32_t(std::uint32_t(person_index)), people) >= 0) break;
            const auto enabled = c.i32(frame.at(0x464)); c.set(0, std::uint32_t(enabled));
            if (compare(c, enabled, 1) != 0) break;
            const auto show_progress = c.i8(frame.at(0x45c)); c.set(0, std::uint32_t(std::int32_t(show_progress)));
            if (nonzero(c, show_progress)) {
                const auto high = multiply_high(0x10624dd3, std::int32_t(std::uint32_t(person_index))), floor = high >> 4;
                const auto sign = std::uint32_t(floor) >> 31, remainder = std::uint32_t(person_index) - (std::uint32_t(floor) + sign) * 250;
                c.set(3, sign); c.set(0, remainder); c.s.wr<1>(guest::reg::xer_ca, high < 0 && (std::uint32_t(high) & 15));
                if (!nonzero(c, std::int32_t(remainder))) {
                    progress = std::uint32_t(progress) + 1;
                    p.call(0x10727890, 0x100794a4, {c.toc(-0x7bb8), c.toc(-0x5718) + 0x228});
                    p.call(0x10743120, 0x100794bc, {c.toc(-0x7bb0), c.toc(-0x7bb8), progress, c.u32(frame.at(0x460)), 0xffffffff});
                }
            }
            const auto running = c.toc(-0x79e4); c.set(3, running);
            const auto active = c.i8(running); c.set(0, std::uint32_t(std::int32_t(active)));
            if (!nonzero(c, active)) {
                const auto row = p.call(0x10082f20, 0x100794d8, {object, person});
                if (nonzero_logical(c, row)) {
                    date_image(c, frame.at(0xec), today, 0, 6);
                    if (dates_equal(c, frame.at(0xec), row + 0x30, 5, 4, 0, 4, 0)) {
                        const auto flags = (c.u8(row + 0x38) & ~0xfcu) | 4u;
                        c.set(4, 1); c.set(0, flags); c.put8(row + 0x38, flags);
                    }
                }
            } else if (review_person()) finish_expiry();
            person_index = std::uint32_t(person_index) + 1; person = std::uint32_t(person) + 112;
        }
        const auto day = c.i16(today); c.set(0, extended(day));
        if (compare(c, day, 36) == 0 || compare(c, day, 200) == 0) p.call(0x10080840, 0x1007bb10, {object});
    }
    const auto dirty = c.i32(object + 0x1c); c.set(0, std::uint32_t(dirty));
    if (nonzero(c, dirty)) {
        const auto cursor = c.u32(object + 0x18), length = c.u32(object + 0x14), base = c.u32(object + 12);
        c.set(0, length); c.set(7, base);
        p.call(0x10768300, 0x1007bb40, {base + cursor * 16, length - cursor, 16, c.toc(-0x5724)});
        c.set(0, 0); c.put32(object + 0x1c, 0);
    }
    p.call(0x10080bb0, 0x1007bb58, {object, today, c.u8(frame.at(0x4e3))});
}
}
void process_daily_contracts(Cpu& c, std::uint32_t manager, std::uint32_t date,
                             std::uint32_t phase, std::uint32_t scan) {
    DailyContracts(c, manager, date, phase, scan).run();
}
}
