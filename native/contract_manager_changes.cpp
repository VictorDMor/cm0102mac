#include "contract_manager_random.hpp"

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
bool change_byte(Cpu& c, std::uint32_t value) {
    const auto byte = std::int8_t(value); c.set(0, std::uint32_t(std::int32_t(byte)));
    return nonzero(c, byte);
}
std::int32_t change_role(Cpu& c, std::uint32_t row) {
    const auto flags = c.u8(row + 0x38); const auto role = std::int32_t(std::int8_t(flags)) >> 2;
    c.set(0, std::uint32_t(role)); c.s.wr<1>(guest::reg::xer_ca, role < 0 && (flags & 2));
    return role;
}
std::uint32_t secondary_flag(Cpu& c, std::uint8_t flags, unsigned output) {
    const auto mask = flags & 2 ? 0xffffffffu : 0u;
    c.set(output, mask); c.s.wr<1>(guest::reg::xer_ca, (flags & 3) == 3); return mask;
}
void change_assertion(Cpu& c, Program& p, std::uint32_t pool, std::uint32_t buffer,
                       std::uint32_t success, std::uint32_t site, std::uint32_t name,
                       std::uint32_t extension, unsigned line) {
    p.call(0x10739890, site + 4, {pool + 0x37, 0, 0, name, extension});
    const auto version = p.call(0x10768810, site + 8);
    p.call(0x1077b720, site + 0x24, {buffer, pool + 0x23b, version, name, extension, line});
    p.call(0x10769b60, site + 0x34, {pool + 0x248, buffer, 0});
    c.set(0, 0); c.put8(success, 0);
}
void change_date(Cpu& c, std::uint32_t destination, std::uint32_t source) {
    const auto year = c.u16(source + 2); c.set(0, year); c.put16(destination + 2, year);
    const auto day = c.i16(source); c.set(0, extended(day)); c.put16(destination, extended(day));
    const auto flag = c.u8(source + 4); c.set(0, flag); c.put8(destination + 4, flag);
}
bool change_date_order(Cpu& c, std::uint32_t first, std::uint32_t second, bool greater) {
    const auto a = c.u16(first + 2), b = c.u16(second + 2); c.set(4, a); c.set(3, b);
    const auto year = compare_logical(c, a, b); bool matches;
    if (year) matches = greater ? year > 0 : year < 0;
    else {
        const auto x = c.i16(greater ? first : second), y = c.i16(greater ? second : first);
        c.set(4, extended(x)); c.set(0, extended(y));
        const auto bits = extended(x) ^ extended(y); const auto half = std::int32_t(bits) >> 1;
        c.set(3, std::uint32_t(half)); c.s.wr<1>(guest::reg::xer_ca, std::int32_t(bits) < 0 && (bits & 1));
        matches = (std::uint32_t(half) - (bits & extended(x))) >> 31;
    }
    c.set(0, matches); nonzero(c, matches); return matches;
}

// The embedded lookup is a leaf expression, not a call with its own frame.
// Keep the original validation, repeated id read and volatile values.
std::uint32_t inline_primary(Cpu& c, std::uint32_t object, std::uint32_t person,
                              std::uint32_t indices, std::uint32_t count, unsigned output) {
    std::uint32_t row = 0;
    if (nonzero_logical(c, person)) {
        const auto checked_id = c.i32(person), total = c.i32(count);
        c.set(3, std::uint32_t(checked_id)); c.set(0, std::uint32_t(total));
        if (compare(c, checked_id, total) < 0) {
            const auto offset = c.u32(person) * 100, base = c.u32(indices); c.set(0, offset); c.set(3, base);
            const auto index = c.i32(base + offset); c.set(0, std::uint32_t(index));
            if (compare(c, index) >= 0) {
                const auto records = c.u32(object), contract_offset = std::uint32_t(index) * 100;
                c.set(0, contract_offset); c.set(3, records); row = records + contract_offset;
            }
        }
    }
    c.set(output, row); return row;
}

// Copy the record's fields, retaining its padding and the original five-byte
// embedded string call. Paired word loads at +0x10/+0x14 precede both stores.
void copy_assignment(Cpu& c, Program& p, std::uint32_t destination, std::uint32_t source,
                       std::uint32_t back, bool pointer_before_store = false) {
    const auto word = [&](unsigned offset) { const auto v = c.u32(source + offset); c.set(0, v); c.put32(destination + offset, v); };
    const auto half = [&](unsigned offset, bool sign = false) {
        const auto v = sign ? extended(c.i16(source + offset)) : std::uint32_t(c.u16(source + offset));
        c.set(0, v); c.put16(destination + offset, v);
    };
    const auto byte = [&](unsigned offset) { const auto v = c.u8(source + offset); c.set(0, v); c.put8(destination + offset, v); };
    // The reference prepares the string arguments before its first store.
    // Keep that state even when a failed allocation makes the store fault.
    c.set(4, source + 0x1c); c.set(5, 5);
    if (pointer_before_store) c.set(3, destination + 0x1c);
    word(0);
    if (!pointer_before_store) c.set(3, destination + 0x1c);
    for (unsigned offset : {4u, 8u, 12u}) word(offset);
    const auto first = c.u32(source + 0x10), second = c.u32(source + 0x14);
    c.set(6, first); c.set(0, second); c.put32(destination + 0x10, first); c.put32(destination + 0x14, second);
    half(0x18); half(0x1a);
    p.call(0x10771110, back, {destination + 0x1c, source + 0x1c, 5});
    word(0x24); half(0x2a); half(0x28, true); byte(0x2c); half(0x32); half(0x30, true); byte(0x34);
    byte(0x38); word(0x3c); byte(0x40); half(0x42, true);
    for (unsigned offset : {0x44u, 0x45u, 0x46u}) byte(offset);
    word(0x48); byte(0x4c); word(0x50); byte(0x54); word(0x58); byte(0x5c); word(0x60);
}

void grow_assignment_storage(Cpu& c, Program& p, std::uint32_t object, std::uint32_t pool,
                               std::uint32_t errors, std::uint32_t success, std::uint32_t realloc_back,
                               std::uint32_t alloc_back, std::uint32_t assertion_site,
                               std::uint32_t name, std::uint32_t extension, unsigned line) {
    const auto count = c.i32(object + 8), capacity = c.i32(object + 4);
    c.set(0, std::uint32_t(count)); c.set(3, std::uint32_t(capacity));
    if (compare(c, count, capacity) < 0) return;
    const auto enlarged = std::uint32_t(capacity) + 100; c.set(0, enlarged); c.put32(object + 4, enlarged);
    const auto old = c.u32(object); c.set(3, old); std::uint32_t records;
    if (nonzero_logical(c, old)) {
        const auto size = c.u32(object + 4); c.set(0, size);
        records = p.call(0x107751b0, realloc_back, {old, size * 100});
    } else {
        const auto size = c.u32(object + 4); c.set(0, size);
        records = p.call(0x10775130, alloc_back, {size * 100});
    }
    c.put32(object, records); c.set(0, c.u32(object));
    if (!nonzero_logical(c, c.r(0)))
        change_assertion(c, p, pool, errors, success, assertion_site, name, extension, line);
    // The original continues after a failed allocation and its assertion.
}

void initialize_role_baseline(Cpu& c, std::uint32_t proposal) {
    const auto squad_role = c.u8(proposal + 0x60) & 15; c.set(0, squad_role);
    unsigned amount = 0; bool replace = true;
    if (compare(c, squad_role, 3) == 0) amount = 40;
    else if (squad_role < 3) {
        const auto relative = compare(c, squad_role, 1);
        if (!relative) amount = 75; else if (relative > 0) amount = 50; else replace = false;
    } else {
        const auto relative = compare(c, squad_role, 8);
        if (relative > 0) replace = false;
        else if (relative < 0 && compare(c, squad_role, 7) >= 0) replace = false;
    }
    if (replace) { c.set(0, amount); c.put8(proposal + 0x44, amount); }
}
}

std::uint32_t apply_primary_contract(Cpu& c, std::uint32_t manager_address,
                                      std::uint32_t proposal_address, std::uint32_t kind_value) {
    Frame frame(c, 0x10083180, 0x120, {.first_gpr = 16});
    Kept<28> proposal(c, proposal_address); Kept<27> object(c, manager_address);
    Kept<20> clubs(c, c.toc(-0x7be8)); Kept<18> countries_or_free(c, c.toc(-0x7b98));
    Kept<29> kind(c, kind_value); Kept<19> nation(c, c.toc(-0x79fc));
    Kept<16> national(c, c.toc(-0x7ab8)); Kept<31> success(c, c.toc(-0x7bac));
    Kept<21> salt(c, c.toc(-0x7b90)); Kept<22> cursor(c, c.toc(-0x7b84));
    Kept<23> indices_or_offset(c, c.toc(-0x7b9c)); Kept<24> errors(c, c.toc(-0x7ba8));
    Kept<25> today(c, c.toc(-0x7b80)); Kept<26> pool(c, c.toc(slot::pool));
    Kept<30> person(c, c.r(30)); Kept<17> target_or_row(c, c.r(17)); Program p(c);
    const auto person_offset = c.u32(proposal) * 112, people = c.u32(c.toc(-0x7be0));
    const auto target_id = c.u32(proposal + 4), club_base = c.u32(clubs);
    person = people + person_offset; target_or_row = club_base + target_id * 0x248;
    c.set(6, target_id); c.set(4, club_base); c.set(3, target_or_row);
    const auto attachment = c.u32(person + 0x24); c.set(0, attachment);
    if (!nonzero_logical(c, attachment) && compare(c, change_role(c, proposal), 5) != 0 &&
        !change_byte(c, p.call(0x100c82a0, 0x10083228, {target_or_row, person, kind, today, 1, 0}))) return result(c, 0);
    const auto original_type = c.i8(person + 0x62), requested_type = std::int8_t(std::uint32_t(kind));
    c.set(0, std::uint32_t(std::int32_t(original_type))); c.set(3, std::uint32_t(std::int32_t(requested_type)));
    if (compare(c, requested_type, original_type) != 0 && compare(c, change_role(c, proposal), 5) != 0 &&
        !change_byte(c, p.call(0x100c7dd0, 0x10083274, {c.u32(person + 0x24), kind, person}))) {
        p.call(0x1008bfc0, 0x1008328c, {object, person, c.u32(person + 0x24)}); return result(c, 0);
    }
    const auto current_club = c.u32(person + 0x24); c.set(3, current_club);
    if (nonzero_logical(c, current_club) && change_byte(c, p.call(0x1014c2e0, 0x100832b4, {current_club, 1, 1, 0, 0}))) {
        const auto home = c.u32(person + 0x24);
        p.call(0x1030f950, 0x100832d0, {c.u32(c.toc(-0x7b1c)), c.u32(home + 0x90)});
        p.call(0x103d2b30, 0x100832e4, {c.u32(c.toc(-0x7a7c)), c.u32(person + 0x24), 0});
    }
    if (compare(c, change_role(c, proposal), 5) == 0) {
        const auto count = std::int8_t(p.call(0x100c9070, 0x1008330c, {target_or_row, 0, 0}));
        c.set(3, std::uint32_t(std::int32_t(count))); c.set(0, std::uint32_t(std::int32_t(count) + 1));
        if (compare(c, std::int32_t(count) + 1, 50) > 0) {
            p.call(0x10091320, 0x1008332c, {object, person, target_or_row}); return result(c, 0);
        }
        const auto target = c.u32(clubs) + c.u32(proposal + 4) * 0x248; c.set(0, c.u32(proposal + 4) * 0x248);
        p.call(0x10430790, 0x10083354, {c.u32(c.toc(-0x7b20)), target, 0});
        const auto refreshed_target = c.u32(clubs) + c.u32(proposal + 4) * 0x248; c.set(0, c.u32(proposal + 4) * 0x248);
        p.call(0x100c82a0, 0x1008337c, {refreshed_target, person, 11, today, 1, 0});
        const auto duration = p.call(0x10718130, 0x1008338c, {proposal + 0x30, c.u32(proposal + 0x28), c.u32(proposal + 0x2c)});
        p.call(0x10717890, 0x1008339c, {frame.at(0x58), today, extended(std::int16_t(duration))});
        change_date(c, proposal + 0x30, frame.at(0x58));
        const auto squad_role = (c.u8(proposal + 0x60) & ~15u) | 8u; c.set(0, squad_role); c.put8(proposal + 0x60, squad_role);
        change_date(c, proposal + 0x28, today); c.set(5, 8);
        p.call(0x100ce200, 0x100833e8, {person, 1});
    }
    const auto start = c.u32(proposal + 0x28), start_tail = c.u32(proposal + 0x2c);
    c.set(3, start); c.set(0, start_tail); c.put32(frame.at(0x40), start); c.put32(frame.at(0x44), start_tail);
    if (!change_date_order(c, frame.at(0x40), proposal + 0x30, false)) {
        p.call(0x10717a10, 0x10083454, {frame.at(0x50), proposal + 0x28, 4});
        change_date(c, proposal + 0x30, frame.at(0x50));
    }
    change_date(c, person + 0x54, proposal + 0x30);
    const auto person_id = c.u32(proposal), index_rows = c.u32(indices_or_offset), index_offset = person_id * 100;
    c.set(0, index_offset); c.set(3, index_rows);
    const auto index = c.i32(index_rows + index_offset); c.set(0, std::uint32_t(index));
    if (compare(c, index, -1) > 0) {
        indices_or_offset = std::uint32_t(index) * 100;
        const auto records = c.u32(object), destination = records + std::uint32_t(indices_or_offset);
        const auto requested_club = c.i32(proposal + 4), existing_club = c.i32(destination + 4);
        c.set(0, std::uint32_t(existing_club)); c.set(4, std::uint32_t(requested_club)); c.set(3, destination);
        if (compare(c, requested_club, existing_club) == 0) {
            const auto retained = c.u8(destination + 0x40); c.set(3, retained); c.put8(proposal + 0x40, retained);
            c.set(6, std::uint32_t(indices_or_offset) + 0x60); c.set(4, 0); c.set(0, 0xffffffff);
            c.set(3, std::uint32_t(indices_or_offset) + 0x38);
            for (unsigned offset : {0x44u, 0x45u, 0x46u, 0x4cu, 0x50u, 0x5cu}) {
                const auto row = c.u32(object) + std::uint32_t(indices_or_offset);
                const auto value = offset == 0x50 ? c.u32(row + offset) : std::uint32_t(c.u8(row + offset)); c.set(5, value);
                if (offset == 0x50) c.put32(proposal + offset, value); else c.put8(proposal + offset, value);
            }
            const auto current_records = c.u32(object); const auto old_flags = c.u8(current_records + std::uint32_t(indices_or_offset) + 0x60);
            const auto flags = (c.u8(proposal + 0x60) & 15u) | (old_flags & 0xf0u);
            c.set(7, current_records); c.set(6, old_flags); c.set(5, flags); c.put8(proposal + 0x60, flags);
            c.put8(proposal + 0x54, 0); c.put32(proposal + 0x58, 0xffffffff);
            const auto old_role_flags = c.u8(c.u32(object) + std::uint32_t(indices_or_offset) + 0x38);
            c.set(4, c.u32(object)); const auto old_flag = secondary_flag(c, old_role_flags, 3);
            const auto updated = (c.u8(proposal + 0x38) & ~2u) | (old_flag & 2u); c.set(0, updated); c.put8(proposal + 0x38, updated);
            const auto old_row = c.u32(object) + std::uint32_t(indices_or_offset); c.set(3, old_row);
            const auto amount = c.i16(old_row + 0x42); c.set(0, extended(amount)); c.put16(proposal + 0x42, extended(amount));
            const auto division = c.u32(target_or_row + 0x2c); c.set(3, division);
            if (nonzero_logical(c, division)) {
                const auto country = c.i32(division + 0x68), eligible = c.i32(nation);
                c.set(3, std::uint32_t(country)); c.set(0, std::uint32_t(eligible));
                if (compare(c, country, eligible) == 0 && !change_byte(c, p.call(0x100ca6a0, 0x100835a0, {person, 0}))) {
                    p.call(0x10717cb0, 0x100835b8, {frame.at(0x48), today, 100});
                    const auto cutoff = c.u32(frame.at(0x48)), tail = c.u32(frame.at(0x4c));
                    c.set(3, cutoff); c.set(0, tail); c.put32(frame.at(0x38), cutoff); c.put32(frame.at(0x3c), tail);
                    if (change_date_order(c, frame.at(0x38), person + 0x4c, true)) {
                        bool grant = false;
                        const auto origin = c.u32(person + 12); c.set(4, origin);
                        if (nonzero_logical(c, origin)) {
                            const auto home_country = c.i32(origin + 0x68), watched = c.i32(c.toc(-0x7b68));
                            c.set(4, std::uint32_t(home_country)); c.set(0, std::uint32_t(watched));
                            grant = compare(c, home_country, watched) == 0;
                        }
                        if (!grant) {
                            const auto position = c.u32(cursor), last = c.u32(c.toc(-0x7b88)), next = position + 2;
                            c.set(4, position); c.set(3, next); c.set(0, last);
                            const bool wraps = compare_logical(c, next, last) > 0; c.put32(cursor, next);
                            if (wraps) {
                                c.set(0, c.toc(-0x7b8c)); c.put32(cursor, c.r(0));
                                const auto entropy = p.call(0x1077b770, 0x10083674); c.set(0, entropy & 0xffff); c.put32(salt, entropy & 0xffff);
                            }
                            const auto sample = c.u32(cursor), seed = c.u32(salt), draw = c.u16(sample) + seed;
                            const auto high = multiply_high(0x55555556, std::int32_t(draw));
                            const auto quotient = std::uint32_t(high) + (std::uint32_t(high) >> 31), remainder = draw - quotient * 3;
                            c.set(5, sample); c.set(4, draw); c.set(3, std::uint32_t(high)); c.set(0, remainder);
                            grant = !nonzero(c, std::int32_t(remainder));
                        }
                        if (grant) {
                            const auto country = c.u32(nation), countries = c.u32(countries_or_free), address = countries + country * 0x128;
                            c.set(0, address); c.set(3, countries); c.put32(person + 0x10, address);
                        }
                    }
                }
            }
        } else {
            initialize_role_baseline(c, proposal); c.set(3, 0); c.set(0, 0xffffffff);
            c.put8(proposal + 0x54, 0); c.put32(proposal + 0x58, 0xffffffff);
            const auto offset = c.u32(proposal + 4) * 0x248, base = c.u32(clubs); c.set(0, offset);
            p.call(0x1037f050, 0x10083758, {proposal, person, base + offset});
            c.set(0, 0); c.put16(proposal + 0x42, 0);
        }
        const auto records_now = c.u32(object); c.set(3, records_now); target_or_row = records_now + std::uint32_t(indices_or_offset);
        copy_assignment(c, p, target_or_row, proposal, 0x100837b8);
    } else {
        countries_or_free = c.u32(object); target_or_row = 0;
        for (;;) {
            const auto count = c.i32(object + 8); c.set(0, std::uint32_t(count));
            if (compare(c, std::int32_t(std::uint32_t(target_or_row)), count) >= 0) break;
            const auto id = c.i32(countries_or_free); c.set(0, std::uint32_t(id));
            if (compare(c, id, -1) == 0) {
                initialize_role_baseline(c, proposal); c.set(3, 0); c.set(0, 0xffffffff);
                c.put8(proposal + 0x54, 0); c.put32(proposal + 0x58, 0xffffffff);
                const auto offset = c.u32(proposal + 4) * 0x248, base = c.u32(clubs); c.set(0, offset);
                p.call(0x1037f050, 0x10083914, {proposal, person, base + offset});
                copy_assignment(c, p, countries_or_free, proposal, 0x10083964, true);
                const auto id_now = c.u32(countries_or_free), indices_now = c.u32(indices_or_offset), offset_now = id_now * 100;
                c.set(0, offset_now); c.set(3, indices_now); c.put32(indices_now + offset_now, target_or_row);
                const auto club_offset = c.u32(proposal + 4) * 0x248, club_records = c.u32(clubs); c.set(0, club_offset);
                if (nonzero_logical(c, p.call(0x10103510, 0x10083a34, {c.u32(national), club_records + club_offset})))
                    p.call(0x100f4f80, 0x10083a40);
                return result(c, 1);
            }
            target_or_row = std::uint32_t(target_or_row) + 1; countries_or_free = std::uint32_t(countries_or_free) + 100;
        }
        grow_assignment_storage(c, p, object, pool, errors, success, 0x10083a8c, 0x10083aa0,
                                 0x10083ac4, frame.at(0xb8), frame.at(0x70), 0x1622);
        initialize_role_baseline(c, proposal); c.set(3, 0); c.set(0, 0xffffffff);
        c.put8(proposal + 0x54, 0); c.put32(proposal + 0x58, 0xffffffff);
        const auto offset = c.u32(proposal + 4) * 0x248, base = c.u32(clubs); c.set(0, offset);
        p.call(0x1037f050, 0x10083b94, {proposal, person, base + offset});
        c.set(0, 0); c.put16(proposal + 0x42, 0);
        const auto id = c.u32(proposal), index_base = c.u32(indices_or_offset), count = c.u32(object + 8);
        c.set(0, id * 100); c.set(3, index_base); c.set(6, count); c.put32(index_base + id * 100, count);
        const auto previous = c.u32(object + 8), records = c.u32(object); c.set(0, previous + 1); c.put32(object + 8, previous + 1);
        c.set(3, previous * 100); c.set(6, records); target_or_row = records + previous * 100;
        copy_assignment(c, p, target_or_row, proposal, 0x10083c18);
    }
    const auto value = c.u32(proposal + 12); c.set(0, value); c.put32(person + 4, value);
    if (compare(c, change_role(c, proposal), 2) == 0) {
        const auto attributes = c.u32(person + 0x14); c.set(0, attributes);
        if (nonzero_logical(c, attributes)) p.call(0x100d3570, 0x10083cfc, {person, 20, 0});
    }
    const auto offset = c.u32(proposal + 4) * 0x248, base = c.u32(clubs); c.set(0, offset);
    if (nonzero_logical(c, p.call(0x10103510, 0x10083d14, {c.u32(national), base + offset}))) p.call(0x100f4f80, 0x10083d20);
    const auto current_type = c.i8(person + 0x62), final_type = std::int8_t(std::uint32_t(kind));
    c.set(0, std::uint32_t(std::int32_t(current_type))); c.set(3, std::uint32_t(std::int32_t(final_type)));
    if (compare(c, final_type, current_type) != 0) {
        if (!change_byte(c, p.call(0x100c8930, 0x10083d4c, {c.u32(person + 0x24), person, 0, 1, 0}))) {
            change_assertion(c, p, pool, errors, success, 0x10083d68, frame.at(0x98), frame.at(0x68), 0x165d); return result(c, 0);
        }
        const auto club_offset = c.u32(proposal + 4) * 0x248, club_records = c.u32(clubs); c.set(0, club_offset);
        if (!change_byte(c, p.call(0x100c82a0, 0x10083dd4, {club_records + club_offset, person, kind, person + 0x4c, 1, 0}))) {
            change_assertion(c, p, pool, errors, success, 0x10083df0, frame.at(0x78), frame.at(0x60), 0x1665); return result(c, 0);
        }
    }
    return result(c, 1);
}

void finish_secondary_contract(Cpu& c, std::uint32_t manager_address, std::uint32_t person_address) {
    Frame frame(c, 0x100845b0, 0xd0, {.first_gpr = 17});
    Kept<21> national(c, c.toc(-0x7ab8)); Kept<18> person(c, person_address);
    Kept<22> success(c, c.toc(-0x7bac)); Kept<17> object_or_affiliate(c, manager_address);
    Kept<23> records(c, c.toc(-0x7a6c)); Kept<24> transfers(c, c.toc(-0x7b40));
    Kept<25> managers(c, c.toc(-0x7b7c)); Kept<26> scratch(c, c.toc(-0x7b78));
    Kept<27> clubs(c, c.toc(-0x7be8)); Kept<28> indices(c, c.toc(-0x7b9c));
    Kept<29> errors(c, c.toc(-0x7ba8)); Kept<30> count(c, c.toc(-0x7be4));
    Kept<31> pool(c, c.toc(slot::pool)); Kept<19> previous_club(c, c.u32(person + 0x24));
    Kept<20> primary(c, c.r(20)); Program p(c);
    if (!nonzero_logical(c, previous_club)) {
        change_assertion(c, p, pool, errors, success, 0x10084614, frame.at(0x6c), frame.at(0x44), 0x1714); return;
    }
    const auto linked = p.call(0x100ce320, 0x10084664, {previous_club, frame.at(0x38), 1});
    if (nonzero_logical(c, linked)) {
        const auto direct = c.i8(frame.at(0x38)); c.set(0, std::uint32_t(std::int32_t(direct)));
        if (!nonzero(c, direct)) previous_club = p.call(0x100ce320, 0x10084688, {previous_club, 0, 1});
    }
    primary = inline_primary(c, object_or_affiliate, person, indices, count, 20);
    if (!nonzero_logical(c, primary)) {
        change_assertion(c, p, pool, errors, success, 0x10084700, frame.at(0x4c), frame.at(0x3c), 0x1720); return;
    }
    const auto employer = [&](unsigned output) {
        const auto manager = c.u32(managers); c.set(4, manager);
        const auto row = inline_primary(c, manager, person, indices, count, 3);
        const bool present = nonzero_logical(c, row); c.put32(scratch, row); std::uint32_t club = 0;
        if (present) {
            const auto offset = c.u32(row + 4) * 0x248, base = c.u32(clubs);
            c.set(0, offset); c.set(3, base); club = base + offset;
        }
        c.set(output, club); return club;
    };
    const auto current_club = employer(5);
    p.call(0x10308350, 0x100847e4, {c.u32(c.toc(-0x7b3c)), person, 0xffffffff, 0, 0, c.u32(current_club), 0});
    p.call(0x100df6d0, 0x100847f4, {c.u32(c.toc(-0x7aa4)), person});
    p.call(0x103afb90, 0x1008480c, {c.u32(records), c.u32(person + 0x24), person, 0, 1});
    p.call(0x103afb90, 0x10084824, {c.u32(records), c.u32(person + 0x24), person, 1, 1});
    p.call(0x10084b10, 0x10084830, {object_or_affiliate, person});
    p.call(0x100c8930, 0x10084848, {c.u32(person + 0x24), person, 0, 1, 0});
    p.call(0x10103510, 0x10084854, {c.u32(national), previous_club}); p.call(0x100f4f80, 0x10084858);
    const auto remaining = employer(4);
    p.call(0x10103510, 0x100848e0, {c.u32(national), remaining}); p.call(0x100f4f80, 0x100848e4);
    const auto item = p.call(0x1036d240, 0x10084900, {c.u32(c.u32(transfers) + 0x988), previous_club, person, 0, 0});
    if (nonzero_logical(c, item)) p.call(0x10365590, 0x10084924, {c.u32(c.u32(transfers) + 0x988), person, previous_club, 4, 0});
    const auto final_club = employer(0); c.put32(person + 0x24, final_club);
    if (change_byte(c, p.call(0x100d12d0, 0x100849b0, {c.u32(person + 0x24)}))) {
        const auto flag = secondary_flag(c, c.u8(primary + 0x38), 0);
        nonzero(c, std::int32_t(flag));
        p.call(0x100ce200, flag ? 0x100849e8 : 0x100849d8, {person, flag ? 2u : 1u});
    } else {
        object_or_affiliate = p.call(0x100ce320, 0x100849fc, {c.u32(person + 0x24), 0, 1});
        if (nonzero(c, std::int32_t(std::uint32_t(object_or_affiliate)))) {
            p.call(0x100ce200, 0x10084a10, {person, 1}); c.set(3, 0); c.set(4, 0);
            for (unsigned n = 0; n < 50; ++n) {
                const auto offset = n * 4, member = c.u32(object_or_affiliate + offset + 0x98);
                c.set(0, member);
                if (compare_logical(c, member, person) == 0) c.put32(person + 0x24, object_or_affiliate);
                c.set(3, offset + 4); c.set(4, n + 1); c.set(0, n + 1); compare(c, std::int8_t(n + 1), 50);
            }
        } else p.call(0x100ce200, 0x10084a54, {person, 1});
    }
    const auto state = c.u8(person + 0x6d); c.set(0, state);
    if (compare(c, state, 16) == 0) { c.set(0, 4); c.put8(person + 0x6d, 4); }
}

void apply_secondary_contract(Cpu& c, std::uint32_t manager_address, std::uint32_t proposal_address) {
    Frame frame(c, 0x10083e50, 0xc0, {.first_gpr = 20});
    Kept<21> national(c, c.toc(-0x7ab8)); Kept<28> object(c, manager_address);
    Kept<22> success_or_row(c, c.toc(-0x7bac)); Kept<29> proposal(c, proposal_address);
    Kept<23> records_or_index(c, c.toc(-0x7a6c)); Kept<30> clubs_or_target(c, c.toc(-0x7be8));
    Kept<24> indices(c, c.toc(-0x7b9c)); Kept<25> errors(c, c.toc(-0x7ba8));
    Kept<26> count_or_row(c, c.toc(-0x7be4)); Kept<27> pool(c, c.toc(slot::pool));
    Kept<31> person(c, c.r(31)); Kept<20> employer(c, c.r(20)); Program p(c);
    p.call(0x10715880, 0x10083e90, {frame.at(0x48)});
    const auto person_offset = c.u32(proposal) * 112, base = c.u32(c.toc(-0x7be0)), managers = c.u32(c.toc(-0x7b7c));
    c.set(0, person_offset); c.set(4, base); c.set(5, managers); person = base + person_offset;
    const auto primary = inline_primary(c, managers, person, indices, count_or_row, 4);
    c.set(3, c.toc(-0x7b78)); const bool present = nonzero_logical(c, primary); c.put32(c.toc(-0x7b78), primary);
    if (present) {
        const auto offset = c.u32(primary + 4) * 0x248, club_base = c.u32(clubs_or_target);
        c.set(0, offset); c.set(3, club_base); employer = club_base + offset;
    } else employer = 0;
    if (!nonzero_logical(c, employer)) return;
    change_date(c, frame.at(0x48), person + 0x4c);
    const auto new_club = c.u32(clubs_or_target) + c.u32(proposal + 4) * 0x248; clubs_or_target = new_club;
    p.call(0x10308350, 0x10083f7c, {c.u32(c.toc(-0x7b3c)), person, 0xffffffff, 0, 1, c.u32(new_club), 0});
    count_or_row = inline_primary(c, object, person, indices, count_or_row, 26);
    if (!nonzero_logical(c, count_or_row)) {
        change_assertion(c, p, pool, errors, success_or_row, 0x10083ff0, frame.at(0x70), frame.at(0x40), 0x1694); return;
    }
    bool secondary_flag_value = false;
    if (change_byte(c, p.call(0x100d12d0, 0x10084038, {employer}))) {
        const auto flags = c.u8(person + 0x6a) & 1; c.set(0, flags);
        secondary_flag_value = !nonzero(c, flags);
    } else {
        const auto linked = p.call(0x100ce320, 0x10084084, {employer, 0, 1});
        if (nonzero_logical(c, linked)) {
            const auto current = c.u32(person + 0x24); c.set(0, current);
            secondary_flag_value = compare_logical(c, employer, current) != 0;
        }
    }
    const auto flags = (c.u8(count_or_row + 0x38) & ~2u) | (secondary_flag_value ? 2u : 0u);
    c.set(3, secondary_flag_value); c.set(0, flags); c.put8(count_or_row + 0x38, flags);
    p.call(0x10430790, 0x100840e4, {c.u32(c.toc(-0x7b20)), clubs_or_target, 0});
    p.call(0x103afb90, 0x100840fc, {c.u32(records_or_index), c.u32(person + 0x24), person, 0, 1});
    p.call(0x103afb90, 0x10084114, {c.u32(records_or_index), c.u32(person + 0x24), person, 1, 1});
    p.call(0x100c82a0, 0x10084130, {clubs_or_target, person, 11, c.toc(-0x7b80), 1, 0});
    change_date(c, person + 0x4c, frame.at(0x48));
    const auto person_id = c.u32(person), index_rows = c.u32(indices), index_offset = person_id * 100;
    c.set(0, person_id); c.set(3, index_rows + index_offset);
    const auto index = c.i32(index_rows + index_offset + 4); c.set(0, std::uint32_t(index));
    if (compare(c, index, -1) > 0) {
        const auto offset = std::uint32_t(index) * 100, records = c.u32(object);
        c.set(3, offset); c.set(6, records); success_or_row = records + offset;
        copy_assignment(c, p, success_or_row, proposal, 0x100841c0);
    } else {
        count_or_row = c.u32(object); records_or_index = 0;
        for (;;) {
            const auto count = c.i32(object + 8); c.set(0, std::uint32_t(count));
            if (compare(c, std::int32_t(std::uint32_t(records_or_index)), count) >= 0) break;
            const auto id = c.i32(count_or_row); c.set(0, std::uint32_t(id));
            if (compare(c, id, -1) == 0) {
                copy_assignment(c, p, count_or_row, proposal, 0x100842d8, true);
                const auto copied_id = c.u32(person), current_indices = c.u32(indices), offset = copied_id * 100;
                c.set(0, offset); c.set(3, current_indices + offset);
                c.put32(current_indices + offset + 4, records_or_index); return;
            }
            records_or_index = std::uint32_t(records_or_index) + 1; count_or_row = std::uint32_t(count_or_row) + 100;
        }
        grow_assignment_storage(c, p, object, pool, errors, success_or_row, 0x100843d8, 0x100843ec,
                                 0x10084410, frame.at(0x50), frame.at(0x38), 0x16f3);
        const auto id = c.u32(person), current_indices = c.u32(indices), count = c.u32(object + 8);
        c.set(0, id * 100); c.set(3, current_indices + id * 100); c.set(6, count);
        c.put32(current_indices + id * 100 + 4, count);
        const auto previous_count = c.u32(object + 8), records = c.u32(object);
        c.set(0, previous_count + 1); c.put32(object + 8, previous_count + 1);
        c.set(3, previous_count * 100); c.set(6, records); success_or_row = records + previous_count * 100;
        copy_assignment(c, p, success_or_row, proposal, 0x100844cc);
    }
    p.call(0x10103510, 0x10084580, {c.u32(national), employer}); p.call(0x100f4f80, 0x10084584);
    p.call(0x10103510, 0x10084590, {c.u32(national), clubs_or_target}); p.call(0x100f4f80, 0x10084594);
}
}
