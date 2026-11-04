#include "transfer_manager_candidate_clubs.hpp"
#include "guest_fpu.hpp"
#include "guest_values.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;
constexpr std::uint32_t club_size = 0x248, staff_size = 0x70;
constexpr std::uint32_t primary_contract = 0x10082f20, loan_contract = 0x10083110,
    verify_club = 0x102cdbf0, list_next = 0x10738e00, loan_parent = 0x100ce320;
constexpr std::int32_t max_rating = 0x2616;
constexpr std::uint32_t buyer_source = 1, seller_source = 2, linked_source = 4, shortlist_source = 8,
    reputation_source = 0x10, firm_only = 0x20, regional_reputation = 0x40, national_reputation = 0x80;

std::uint32_t call(Cpu& c, std::uint32_t entry, std::uint32_t back,
                   std::initializer_list<std::uint32_t> args = {}) {
    unsigned r = 3;
    for (const auto arg : args) c.set(r++, arg);
    c.call(entry, back);
    return c.r(3);
}

bool record_byte(Cpu& c, std::uint32_t raw) {
    const auto value = std::int8_t(raw);
    c.set(0, std::uint32_t(value));
    return nonzero(c, value);
}

struct Overflow {
    std::uint32_t back, file, extension, line;
};
constexpr Overflow linked_overflow{0x103fbb24, 0xa4, 0x4c, 0x3935};
constexpr Overflow shortlist_overflow{0x103fbc94, 0x84, 0x44, 0x3958};
constexpr Overflow reputation_overflow{0x103fc30c, 0x64, 0x3c, 0x39f2};

struct Collection {
    Cpu& c;
    const Frame& frame;
    const Kept<15> offer;
    const Kept<16> output;
    const Kept<17> capacity;
    const Kept<26> person;
    Kept<24> count;
    const Kept<25> buyer;
    const Kept<28> seller;
    const Field<std::uint32_t> sources;

    Collection(Cpu& c, const Frame& frame, std::uint32_t offer, std::uint32_t output, std::uint32_t capacity,
               std::uint32_t person, std::uint32_t buyer, std::uint32_t seller)
        : c(c), frame(frame), offer(c, offer),
          output(c, output), capacity(c, capacity), person(c, person), count(c, 0), buyer(c, buyer), seller(c, seller),
          sources(c, frame.caller() + 0x20) {}

    bool source(std::uint32_t mask) const {
        const auto selected = std::uint32_t(sources) & mask;
        c.set(0, selected);
        return nonzero(c, selected);
    }

    void overflow(const Overflow& where) const {
        const auto pool = c.r(31), message = c.r(30);
        const auto file = frame.at(where.file), extension = frame.at(where.extension), back = where.back;
        call(c, 0x10739890, back, {pool + 0x37, 0, 0, file, extension});
        const auto version = call(c, 0x10768810, back + 4);
        call(c, 0x1077b720, back + 0x20, {message, pool + 0x6ff, version, file, extension, where.line});
        call(c, 0x10769b60, back + 0x30, {pool + 0x70c, message, 0});
        const auto valid = c.toc(-0x7bac);
        c.set({{0, 0}, {3, valid}});
        c.put8(valid, 0);
    }

    bool under_capacity(const Overflow& where) const {
        const auto size = std::int8_t(std::uint32_t(count)), limit = std::int8_t(std::uint32_t(capacity));
        c.set({{0, std::uint32_t(limit)}, {4, std::uint32_t(size)}});
        if (compare(c, size, limit) < 0) return true;
        overflow(where);
        return false;
    }

    std::uint32_t append_offset(unsigned offset_register = 0) {
        const auto slot = std::uint32_t(std::int8_t(std::uint32_t(count))) * 4;
        c.set(offset_register, slot);
        count = count + 1;
        return slot;
    }

    bool recognized(std::uint32_t club, std::uint32_t back) const {
        return record_byte(c, call(c, verify_club, back, {c.u32(c.r(19)), club}));
    }

    bool has_other_loan(std::uint32_t first, std::uint32_t repeat) const {
        const auto loans = c.r(18);
        auto contract = call(c, loan_contract, first, {c.u32(loans), person});
        if (!nonzero_logical(c, contract)) return false;
        if (!nonzero_logical(c, buyer)) return true;
        contract = call(c, loan_contract, repeat, {c.u32(loans), person});
        const auto buyer_id = c.u32(buyer), loan_id = c.u32(contract + 4);
        c.set({{0, buyer_id}, {3, loan_id}});
        return compare(c, buyer_id, loan_id) != 0;
    }

    void direct_clubs() {
        const auto include_seller = source(seller_source);
        c.put32(frame.at(0x178), c.r(0));
        if (include_seller && nonzero_logical(c, seller) && recognized(seller, 0x103fb934))
            c.put32(output + append_offset(), seller);
        const auto include_buyer = std::uint32_t(sources) & buyer_source;
        c.set({{0, sources}, {27, include_buyer}});
        if (nonzero(c, include_buyer) && recognized(buyer, 0x103fb964)) c.put32(output + append_offset(), buyer);
    }

    void linked_clubs();
    void shortlist_clubs();
    void reputation_clubs();
    std::uint32_t compact();
};

bool filtered_link(Cpu& c, std::uint32_t link) {
    const auto kind = c.u8(link + 0x34);
    c.set(6, kind);
    const auto near = [&](unsigned base) {
        const auto difference = std::uint8_t(kind - base);
        c.set(4, difference);
        return compare_logical(c, difference, 1) <= 0;
    };
    bool filtered = near(2);
    if (!filtered) {
        const auto signed_kind = std::int8_t(kind);
        c.set(5, std::uint32_t(signed_kind));
        filtered = compare(c, signed_kind, 20) == 0 || near(15) || compare(c, signed_kind, 4) == 0;
    }
    if (filtered) {
        c.set(4, std::uint32_t(std::int8_t(kind)));
        if (compare(c, std::int8_t(kind), 15) == 0) {
            const auto stage = c.u8(link + 0x36);
            c.set(4, stage);
            filtered = compare_logical(c, stage, 3) >= 0;
        }
    }
    c.set(4, filtered);
    return nonzero(c, filtered);
}

std::uint32_t linked_offer(Cpu& c, std::uint32_t index) {
    return c.u32(c.u32(c.r(20)) + index * 4);
}

void Collection::linked_clubs() {
    if (!source(linked_source)) return;
    if (has_other_loan(0x103fb994, 0x103fb9b0)) {
        const auto parent = call(c, loan_parent, 0x103fb9d0, {c.u32(person + 0x24), frame.at(0x38), 1});
        bool use_parent = nonzero_logical(c, parent);
        if (use_parent) use_parent = !record_byte(c, c.u8(frame.at(0x38)));
        const auto selected = use_parent ? parent : result(c, c.u32(person + 0x24));
        c.put32(output + append_offset(), selected);
    }
    const auto list = result(c, c.u32(offer));
    if (!nonzero_logical(c, list)) return;
    const auto head = c.u32(list + 4);
    c.set(0, head);
    c.put32(list + 0xc, head);
    for (;;) {
        const auto row = call(c, list_next, 0x103fbb68, {c.u32(offer)});
        c.set(21, row);
        if (!nonzero(c, std::int32_t(row))) return;
        const auto index = c.u32(row);
        const auto link = linked_offer(c, index);
        c.set({{0, index}, {3, link}, {4, c.u32(c.r(20))}});
        if (filtered_link(c, link)) continue;
        const auto self = c.u32(offer + 0x28);
        c.set(4, self);
        if (compare(c, index, self) == 0) continue;
        const auto club_offset = c.u32(link + 8) * club_size;
        c.set(0, club_offset);
        if (!recognized(c.u32(c.r(13)) + club_offset, 0x103fbac4) || !under_capacity(linked_overflow)) continue;
        const auto slot = append_offset();
        const auto club = c.u32(c.r(13)) + c.u32(linked_offer(c, c.u32(row)) + 8) * club_size;
        c.set({{3, club}, {4, c.u32(c.r(20))}, {5, c.u32(c.r(13))}});
        c.put32(output + slot, club);
    }
}

void Collection::shortlist_clubs() {
    if (!source(shortlist_source)) return;
    const auto current_club = c.u32(person + 0x24);
    c.set(0, current_club);
    if (nonzero_logical(c, current_club) && has_other_loan(0x103fbb94, 0x103fbbb0)) {
        const auto club = result(c, c.u32(person + 0x24));
        c.put32(output + append_offset(), club);
    }
    const auto list = frame.at(0x54);
    call(c, 0x10362ef0, 0x103fbbf0, {c.u32(c.r(20) + 0x988), person, list, 1, 1, 1});
    const auto head = c.u32(list + 4);
    c.set(0, head);
    c.put32(list + 0xc, head);
    for (;;) {
        const auto row = call(c, list_next, 0x103fbcd8, {list});
        c.set(18, row);
        if (!nonzero(c, std::int32_t(row))) return;
        const auto id = c.u32(row + 4);
        const auto base = c.u32(c.r(13));
        if (compare(c, c.r(27)) == 0) {
            c.set({{0, base + id * club_size}, {3, base}});
            if (compare_logical(c, base + id * club_size, buyer) == 0) continue;
        }
        const auto buyer_id = c.u32(buyer);
        c.set({{0, buyer_id}, {3, id}});
        if (compare(c, id, buyer_id) == 0) continue;
        const auto offset = id * club_size;
        c.set(0, offset);
        if (!recognized(base + offset, 0x103fbc44) || !under_capacity(shortlist_overflow)) continue;
        const auto slot = append_offset();
        c.set({{3, base + offset}, {4, base}});
        c.put32(output + slot, base + offset);
    }
}

std::uint32_t Collection::compact() {
    const auto size = std::int32_t(std::int8_t(std::uint32_t(count)));
    c.set({{0, std::uint32_t(size - 1)}, {5, std::uint32_t(size)}, {7, 0}, {10, 0}});
    for (std::int32_t index = 0; ; ++index) {
        const auto cursor = output + std::uint32_t(index * 4);
        c.set({{3, cursor}, {6, std::uint32_t(index)}, {10, std::uint32_t(index)}});
        if (compare(c, index, size - 1) >= 0) break;
        const auto club = c.u32(cursor);
        c.set(4, club);
        if (!nonzero_logical(c, club)) continue;
        for (auto other = index + 1; ; ++other) {
            const auto offset = std::uint32_t(other * 4), address = output + offset;
            c.set({{4, address}, {6, std::uint32_t(other)}, {9, offset}, {11, std::uint32_t(other)}});
            if (compare(c, other, size) >= 0) break;
            const auto stored = c.u32(cursor), candidate = c.u32(address);
            c.set({{6, candidate}, {8, stored}});
            if (compare_logical(c, stored, candidate) == 0) c.put32(address, 0);
        }
    }
    std::uint32_t unique = 0;
    c.set({{0, std::uint32_t(size)}, {13, unique}});
    for (std::int32_t index = 0; ; ++index) {
        const auto cursor = output + std::uint32_t(index * 4);
        c.set({{3, cursor}, {4, std::uint32_t(index)}, {6, std::uint32_t(index)}});
        if (compare(c, index, size) >= 0) break;
        const auto club = c.u32(cursor);
        c.set(5, club);
        if (!nonzero_logical(c, club)) continue;
        const auto offset = std::uint32_t(std::int8_t(unique)) * 4;
        c.set({{4, offset}, {13, ++unique}});
        c.put32(output + offset, club);
    }
    call(c, 0x10738c20, 0x103fc6ec, {frame.at(0x54), 0xffffffff});
    return result(c, unique);
}

struct Reputation {
    Collection& clubs;
    Cpu& c;
    const Frame& frame;
    const Field<std::int16_t> staff_rating, job_rating, world_rating;

    explicit Reputation(Collection& clubs)
        : clubs(clubs), c(clubs.c), frame(clubs.frame), staff_rating(c, frame.at(0x168)),
          job_rating(c, frame.at(0x166)), world_rating(c, frame.at(0x164)) {}

    std::uint32_t offer_value(std::uint32_t back) const {
        return call(c, 0x10416840, back, {clubs.offer, 0, 0, 0, 0, 1});
    }

    void start(std::uint32_t shortlist);
    void load_ratings();
    void scale_staff_ratings();
    std::int32_t rating_bonus();
    bool exceeds_statistical_rating(std::uint32_t back, std::uint32_t site, std::uint32_t intercept);
    void choose_ranges();
    void consider_buyer_nation(std::int32_t base);
    void choose_regional_targets();
    void scan_managers();
    bool match_manager(std::uint32_t club, std::uint32_t nationality);
};

void Reputation::start(std::uint32_t shortlist) {
    c.set(0, 0);
    for (unsigned offset = 0x171; offset <= 0x174; ++offset) c.put8(frame.at(offset), 0);
    for (unsigned reg : {13u, 18u, 19u, 20u, 21u, 22u, 23u}) c.set(reg, 0);
    const auto interest = call(c, 0x1036d240, 0x103fbd58, {shortlist, clubs.buyer, clubs.person, 0, 0});
    c.set(0, 0);
    c.put32(frame.at(0x180), interest);
    c.put32(frame.at(0x184), 0);
    const auto role = c.u8(clubs.offer + 0x11);
    if (record_byte(c, call(c, 0x100c7a20, 0x103fbd6c, {role}))) {
        c.set(0, interest);
        if (nonzero_logical(c, interest)) {
            const auto band = (c.u8(interest + 0x28) >> 3) & 15;
            c.set({{0, std::uint32_t(band)}, {3, interest}});
            if (compare_logical(c, band, 3) < 0) {
                const auto fee = c.i32(clubs.offer + 0x20);
                c.set({{0, 1000000}, {3, 0xf0000}, {4, std::uint32_t(fee)}});
                if (compare(c, fee, 1000000) >= 0) c.put32(frame.at(0x184), 1);
            }
        }
    }
    const auto employer = c.u32(clubs.person + 0x24);
    const auto interested = c.u32(frame.at(0x184));
    c.set({{0, interested}, {3, employer}});
    compare_logical(c, employer);
    c.put8(frame.at(0x170), interested);
    if (employer) c.set(13, c.u32(employer + 0x30));
    load_ratings();
}

void Reputation::load_ratings() {
    const auto player = c.u32(clubs.person + 0x14);
    c.set(3, player);
    const auto buyer_region = c.u32(clubs.buyer + 0x30);
    compare_logical(c, player);
    c.put32(frame.at(0x16c), buyer_region);
    c.set(0, 0);
    staff_rating = 0;
    job_rating = 0;
    world_rating = 0;
    if (player) {
        staff_rating = c.i16(player + 0xa);
        job_rating = c.i16(player + 8);
        world_rating = c.i16(player + 0xc);
        c.set(0, std::uint32_t(std::int32_t(world_rating)));
        return;
    }
    const auto employee = c.u32(clubs.person + 0x1c);
    c.set(4, employee);
    if (!nonzero_logical(c, employee)) return;
    staff_rating = c.i16(employee + 0x2a);
    job_rating = c.i16(employee + 0x28);
    world_rating = c.i16(employee + 0x2c);
    const auto type = c.i8(clubs.offer + 0x11);
    c.set({{0, std::uint32_t(std::int32_t(world_rating))}, {3, std::uint32_t(type)}});
    if (compare(c, type, 9) == 0 || compare(c, type, 8) == 0 || compare(c, type, 10) == 0)
        scale_staff_ratings();
}

void Reputation::scale_staff_ratings() {
    const auto constants = c.toc(-0x4ba0);
    constexpr auto upper = 0x43300000u;
    const auto low = [](std::int16_t value) { return std::uint32_t(std::int32_t(value)) ^ 0x80000000u; };
    const auto first = frame.at(0x15c), second = frame.at(0x14c), third = frame.at(0x13c);
    c.put32(first, upper);
    c.put32(first + 4, low(staff_rating));
    c.put32(second + 4, low(job_rating));
    c.put32(second, upper);
    c.put32(third + 4, low(world_rating));
    c.put32(third, upper);
    Fpu fpu(c);
    const auto bias = c.f64(constants + 8), rate = c.f64(constants + 0x168);
    c.set_f(3, bias);
    const auto staff = fpu.fsub(c.f64(first), bias);
    c.set_f(4, rate);
    const auto scaled_staff = fpu.fmul(rate, staff);
    const auto job = fpu.fsub(c.f64(second), bias);
    c.put64(frame.at(0x154), fpu.fctiwz(2, scaled_staff));
    const auto scaled_job = fpu.fmul(rate, job);
    const auto world = fpu.fsub(c.f64(third), bias);
    c.put64(frame.at(0x144), fpu.fctiwz(1, scaled_job));
    const auto scaled_world = fpu.fmul(rate, world);
    c.put64(frame.at(0x134), fpu.fctiwz(0, scaled_world));
    staff_rating = c.u32(frame.at(0x158));
    job_rating = c.u32(frame.at(0x148));
    const auto last = c.u32(frame.at(0x138));
    world_rating = last;
    c.set({{0, last}, {3, constants}, {4, c.u32(second + 4)}, {5, upper}});
}

std::int32_t Reputation::rating_bonus() {
    if (record_byte(c, c.u8(clubs.offer + 0x37))) {
        c.set(0, 500);
        return 500;
    }
    const auto value = offer_value(0x103fbf1c);
    const auto constants = c.toc(-0x4ba0), biased = frame.at(0x134);
    c.put32(biased + 4, value ^ 0x80000000u);
    c.put32(biased, 0x43300000u);
    const auto bias = c.f64(constants + 8), slope = c.f64(constants + 0x178);
    c.set({{0, 0x43300000u}, {3, constants}});
    Fpu fpu(c);
    c.set_f(3, bias);
    c.set_f(1, slope);
    c.set_f(2, fpu.fsub(c.f64(biased), bias));
    const auto adjustment = fpu.multiply_add(0x103fbf44, slope, c.f(2), c.f64(constants + 0x170));
    c.put64(frame.at(0x13c), fpu.fctiwz(0, adjustment));
    const auto whole = c.u32(frame.at(0x140));
    c.set(0, whole);
    return std::int16_t(whole);
}

std::int32_t capped(Cpu& c, std::int32_t value, unsigned destination) {
    const auto limit = compare(c, value, max_rating) >= 0 ? max_rating : value;
    c.set(destination, std::uint32_t(limit));
    return limit;
}

void Reputation::consider_buyer_nation(std::int32_t base) {
    c.set({{3, std::uint32_t(base)}, {5, 1}});
    c.put8(frame.at(0x172), 1);
    const auto job_cap = capped(c, base, 5);
    const auto job = std::int32_t(job_rating);
    c.set(4, std::uint32_t(job));
    if (compare(c, job, job_cap) > 0) c.set(20, c.u32(clubs.person + 0xc));
    const auto staff_cap = capped(c, base, 4);
    const auto staff = std::int32_t(staff_rating);
    c.set(5, std::uint32_t(staff));
    if (compare(c, staff, staff_cap) > 0 && nonzero_logical(c, clubs.seller))
        c.set(19, c.u32(clubs.seller + 0x2c));
    const auto country = c.u32(clubs.buyer + 0x2c);
    c.set(4, country);
    if (compare_logical(c, country, c.r(20)) == 0 || compare_logical(c, country, c.r(19)) == 0) return;
    const auto world_cap = capped(c, base, 4);
    if (compare(c, world_rating, world_cap) <= 0) {
        const auto fee = offer_value(0x103fc0bc);
        const auto status = c.i8(country + 0x11f);
        const auto threshold = status * 500000 - 5000000;
        c.set({{0, std::uint32_t(threshold)}, {4, std::uint32_t(status * 500000 - 0x4c0000)}, {5, country}});
        if (compare(c, std::int32_t(fee), threshold) < 0) {
            const auto record = result(c, c.u32(clubs.person + 0x20));
            if (!nonzero_logical(c, record)) return;
            const auto roles = c.u8(clubs.person + 0x5e);
            c.set(0, roles);
            if (compare_logical(c, roles, 20) <= 0) return;
            const auto rating = c.i16(record + 0x1ec);
            c.set(0, std::uint32_t(rating));
            if (compare(c, rating, 5500) <= 0) return;
        }
    }
    c.set(18, country);
}

bool Reputation::exceeds_statistical_rating(std::uint32_t back, std::uint32_t site, std::uint32_t intercept) {
    const auto player = c.u32(clubs.person + 0x14);
    const auto weights = c.toc(-0x4ba4);
    c.set_f(1, double(c.f32(weights + 0x2c)));
    call(c, 0x10303560, back, {c.u32(c.toc(-0x7b3c)), player, 2, 0x12});
    const auto biased = frame.at(0x134), constants = c.toc(-0x4ba0);
    const auto rating = std::int32_t(staff_rating);
    const auto low = std::uint32_t(rating) ^ 0x80000000u;
    c.put32(biased, 0x43300000u);
    c.put32(biased + 4, low);
    c.set({{0, 0x43300000u}, {3, constants}, {4, low}});
    if (back == 0x103fc138) c.set(5, std::uint32_t(rating));
    Fpu fpu(c);
    c.set_f(3, c.f64(constants + 8));
    c.set_f(4, c.f64(constants + 0x188));
    c.set_f(2, fpu.fsub(c.f64(biased), c.f(3)));
    c.set_f(0, fpu.multiply_add(site, c.f(4), c.f(2), c.f64(constants + intercept)));
    fpu.fcmp(0, c.f(1), c.f(0));
    return (c.cr(0) & 4) != 0;
}

void Reputation::choose_regional_targets() {
    if (!clubs.source(regional_reputation)) return;
    const auto region = c.r(13);
    if (nonzero_logical(c, region)) {
        const auto threshold = c.i16(region + 0x18) * 350;
        const auto rating = c.i32(frame.at(0x17c));
        c.set({{0, std::uint32_t(rating)}, {3, std::uint32_t(threshold)}});
        bool preferred = false;
        if (compare(c, rating, threshold) > 0) {
            c.set(0, std::uint32_t(std::int32_t(staff_rating)));
            preferred = compare(c, staff_rating, 2500) > 0;
        }
        if (!preferred) {
            const auto player = c.u32(clubs.person + 0x14);
            c.set(4, player);
            preferred = nonzero_logical(c, player) && exceeds_statistical_rating(0x103fc1f8, 0x103fc224, 0x190);
        }
        if (preferred) {
            c.set(0, 1);
            c.put8(frame.at(0x171), 1);
        }
    }
    const auto target = c.u32(frame.at(0x16c));
    c.set(0, target);
    if (!nonzero_logical(c, target)) return;
    for (unsigned index = 0; index < 5; ++index)
        call(c, 0x10715880, 0x103fc24c + index * 8, {frame.at(0xf8 + index * 8)});
    const auto country = c.u32(clubs.buyer + 0x2c);
    c.put32(frame.at(0xd4), 0);
    c.put32(frame.at(0xd0), country);
    c.put32(frame.at(0xe0), 0);
    c.put32(frame.at(0xd8), 0);
    c.put32(frame.at(0xd4), 0);
    c.put32(frame.at(0xe8), clubs.buyer);
    c.put32(frame.at(0xc4), 0);
    const auto non_player = c.u32(clubs.person + 0x14) == 0;
    c.set(0, non_player);
    const auto estimate = std::int16_t(call(c, 0x100cdef0, 0x103fc2ac, {clubs.person, frame.at(0xc4), non_player}));
    const auto threshold = c.i16(target + 0x18) * 350;
    c.set({{0, std::uint32_t(threshold)}, {3, std::uint32_t(estimate)}, {4, target}});
    if (compare(c, estimate, threshold) <= 0) return;
    c.set(0, std::uint32_t(std::int32_t(staff_rating)));
    if (compare(c, staff_rating, 1500) <= 0) return;
    c.set(0, 1);
    c.put8(frame.at(0x170), 1);
}

void Reputation::choose_ranges() {
    const auto bonus = rating_bonus();
    c.set({{0, std::uint32_t(bonus + 9200)}, {4, std::uint32_t(bonus)}});
    const auto broad = capped(c, bonus + 9200, 3);
    const auto world = std::int32_t(world_rating);
    c.set(0, std::uint32_t(world));
    if (compare(c, world, broad) > 0) {
        c.set(0, 1);
        c.put8(frame.at(0x174), 1);
        return;
    }
    c.set(5, std::uint32_t(bonus + 8300));
    const auto market = capped(c, bonus + 8300, 3);
    if (compare(c, world, market) > 0) {
        const auto passport = result(c, c.u32(clubs.person + 0xc));
        if (nonzero_logical(c, passport)) c.set(23, c.u32(passport + 0x74));
        if (nonzero_logical(c, clubs.seller)) {
            const auto nation = result(c, c.u32(clubs.seller + 0x2c));
            if (nonzero_logical(c, nation)) c.set(22, c.u32(nation + 0x74));
        }
        const auto nation = result(c, c.u32(clubs.buyer + 0x2c));
        if (nonzero_logical(c, nation)) c.set(21, c.u32(nation + 0x74));
        c.set(0, 1);
        c.put8(frame.at(0x173), 1);
        return;
    }
    const auto base = bonus + 7200;
    c.set(3, std::uint32_t(base));
    const auto job_cap = capped(c, base, 5);
    const auto job = std::int32_t(job_rating);
    c.set(4, std::uint32_t(job));
    bool high = compare(c, job, job_cap) > 0;
    if (!high) {
        const auto staff = std::int32_t(staff_rating);
        c.set(5, std::uint32_t(staff));
        c.put32(frame.at(0x17c), std::uint32_t(staff));
        const auto staff_cap = capped(c, base, 6);
        high = compare(c, staff, staff_cap) > 0;
    }
    if (high) {
        consider_buyer_nation(base);
        return;
    }
    const auto player = c.u32(clubs.person + 0x14);
    c.set(4, player);
    if (nonzero_logical(c, player) && exceeds_statistical_rating(0x103fc138, 0x103fc164, 0x180) &&
        compare(c, staff_rating, 3000) > 0 && clubs.source(national_reputation)) {
        c.set(0, 1);
        compare_logical(c, clubs.seller);
        c.put8(frame.at(0x172), 1);
        if (clubs.seller) c.set(19, c.u32(clubs.seller + 0x2c));
        return;
    }
    choose_regional_targets();
}

bool matches_one(Cpu& c, std::uint32_t value, std::initializer_list<unsigned> keys) {
    if (!nonzero_logical(c, value)) return false;
    for (unsigned key : keys) if (compare_logical(c, value, c.r(key)) == 0) return true;
    return false;
}

bool has_region(Cpu& c, std::uint32_t nationality, unsigned table_register, unsigned region_reg, unsigned target_reg) {
    if (!nonzero_logical(c, nationality)) return false;
    const auto region = c.u32(nationality + 0x68), target = c.u32(c.r(table_register));
    c.set({{region_reg, region}, {target_reg, target}});
    return compare(c, region, target) == 0;
}

bool Reputation::match_manager(std::uint32_t club, std::uint32_t nationality) {
    const auto country = c.u32(club + 0x2c);
    c.set({{5, nationality}, {7, country}});
    const auto passport_group = nonzero_logical(c, nationality) ? c.u32(nationality + 0x74) : 0;
    const auto country_group = nonzero_logical(c, country) ? c.u32(country + 0x74) : 0;
    c.set({{8, passport_group}, {9, country_group}});

    const auto flag = [&](std::uint32_t offset, unsigned reg) {
        const auto value = c.i8(frame.at(offset));
        c.set(reg, std::uint32_t(value));
        return nonzero(c, value);
    };
    if (flag(0x174, 6)) return true;
    if (flag(0x173, 6))
        return matches_one(c, passport_group, {23, 22, 21}) || matches_one(c, country_group, {23, 22, 21});
    if (flag(0x172, 6)) {
        if (matches_one(c, nationality, {20, 19, 18}) || matches_one(c, country, {20, 19, 18})) return true;
        if (has_region(c, nationality, 14, 7, 6)) {
            for (auto target : {20u, 19u, 18u}) if (has_region(c, c.r(target), 29, 7, 6)) return true;
        }
        if (has_region(c, nationality, 29, 6, 5)) {
            for (auto target : {20u, 19u, 18u}) if (has_region(c, c.r(target), 14, 6, 5)) return true;
        }
        return false;
    }
    if (flag(0x171, 5)) {
        const auto region = c.u32(club + 0x30);
        c.set(5, region);
        if (compare_logical(c, region, c.r(13)) == 0) return true;
    }
    if (!flag(0x170, 5)) return false;
    const auto region = c.u32(club + 0x30), target = c.u32(frame.at(0x16c));
    c.set({{5, target}, {6, region}});
    return compare_logical(c, region, target) == 0;
}

void Reputation::scan_managers() {
    const auto limit = std::int8_t(std::uint32_t(clubs.capacity));
    c.set(0, std::uint32_t(limit));
    for (std::uint32_t index = 0; ; ++index) {
        c.set({{3, index}, {4, index}});
        if (compare(c, index, 16) >= 0) return;
        const auto used = std::int8_t(std::uint32_t(clubs.count));
        c.set(3, std::uint32_t(used));
        if (compare(c, used, limit) == 0) {
            clubs.overflow(reputation_overflow);
            return;
        }
        const auto count = c.u32(c.toc(-0x7be4));
        const auto staff = c.u32(c.toc(-0x7be0));
        const auto manager = staff + (count - index - 1) * staff_size;
        const auto human = c.i8(manager + 0x6b);
        c.set({{3, std::uint32_t(human)}, {5, count}, {6, manager}, {7, staff}});
        if (!nonzero(c, human)) continue;
        const auto club = result(c, c.u32(manager + 0x24));
        if (!nonzero_logical(c, club)) continue;
        if (compare(c, c.r(27)) == 0 && compare_logical(c, club, clubs.buyer) == 0) continue;
        const auto include_seller = c.u32(frame.at(0x178));
        c.set(5, include_seller);
        if (compare(c, include_seller) == 0 && compare_logical(c, club, clubs.seller) == 0) continue;
        if (match_manager(club, c.u32(manager + 0xc))) c.put32(clubs.output + clubs.append_offset(5), club);
    }
}

void Collection::reputation_clubs() {
    if (!source(reputation_source) || (source(firm_only) && !record_byte(c, c.u8(offer + 0x35)))) return;
    Reputation reputation(*this);
    reputation.start(c.u32(c.r(20) + 0x988));
    reputation.choose_ranges();
    reputation.scan_managers();
}
}

std::uint32_t collect_candidate_clubs(Cpu& c, std::uint32_t manager, std::uint32_t offer,
                                      std::uint32_t sources, std::uint32_t output, std::uint32_t capacity) {
    Frame frame(c, 0x103fb840, 0x1e0, {.first_gpr = 13});
    c.set(0, c.lr());
    keep(c, {{13, c.toc(-0x7be8)}, {14, c.toc(-0x7a10)}, {15, offer}, {16, output}, {17, capacity},
             {18, c.toc(-0x7b7c)}, {19, c.toc(-0x7bf0)}, {20, manager}, {21, 0}, {22, c.toc(-0x7b78)},
             {29, c.toc(-0x7a1c)}, {30, c.toc(-0x7ba8)}, {31, c.toc(-0x4b9c)}});
    const auto offset = c.u32(offer + 0xc) * staff_size;
    const auto person = c.u32(c.toc(-0x7be0)) + offset;
    c.set({{0, offset}, {26, person}});
    c.put32(frame.caller() + 0x20, sources);
    const auto contract = call(c, primary_contract, 0x103fb8a8, {c.u32(c.r(18)), person});
    compare_logical(c, contract);
    c.put32(c.r(22), contract);
    bool has_seller = false;
    if (contract) {
        const auto type = c.i8(contract + 0x38);
        const auto category = type >> 2;
        c.s.wr<1>(reg::xer_ca, type < 0 && (std::uint8_t(type) & 2));
        c.set(0, std::uint32_t(category));
        has_seller = compare(c, category, 5) != 0;
    }
    c.set(21, has_seller);
    compare(c, has_seller);
    const auto base = c.u32(c.r(13));
    std::uint32_t seller = 0;
    if (has_seller) {
        const auto club_offset = c.u32(c.u32(c.r(22)) + 4) * club_size;
        c.set(0, club_offset);
        seller = base + club_offset;
    }
    const auto buyer_offset = c.u32(offer + 8) * club_size;
    c.set({{0, buyer_offset}, {4, base}});
    Collection clubs(c, frame, offer, output, capacity, person, base + buyer_offset, seller);
    call(c, 0x10738c00, 0x103fb910, {frame.at(0x54)});
    clubs.direct_clubs();
    clubs.linked_clubs();
    clubs.shortlist_clubs();
    clubs.reputation_clubs();
    return clubs.compact();
}
}
