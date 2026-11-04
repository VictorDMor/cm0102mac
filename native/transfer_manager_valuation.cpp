#include "transfer_manager_valuation.hpp"
#include "guest_values.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;

constexpr std::int32_t today_slot = -0x7b80, contracts_slot = -0x7b7c,
    clubs_slot = -0x7be8, message_buffer_slot = -0x7ba8, source_pool_slot = -0x4b9c,
    success_slot = -0x7bac, limited_contracts_slot = -0x7a68, valuation_nation_slot = -0x7514,
    current_contract_slot = -0x7b78;
constexpr std::uint32_t construct_date = 0x10715880, primary_contract = 0x10082f20,
    remaining_days = 0x10718130, base_value = 0x10084be0, age_on_date = 0x100c6a10,
    nation_accepts_fee = 0x100d34c0, nation_eligibility = 0x100d33d0, club_size = 0x248;

struct Staff {
    Cpu& cpu;
    std::uint32_t address;

    bool has_player_record() const { return nonzero_logical(cpu, cpu.u32(address + 0x14)); }
    bool has_staff_record() const { return nonzero_logical(cpu, cpu.u32(address + 0x1c)); }
};

struct Contract {
    Cpu& cpu;
    std::uint32_t address;

    std::uint32_t employer_index() const { return cpu.u32(address + 4); }
    std::uint32_t wage() const { return cpu.u32(address + 0xc); }
    std::uint32_t expiry() const { return address + 0x30; }

    std::int32_t kind() const {
        const auto packed = cpu.i8(address + 0x38);
        // The signed six-bit type comes from a rotate followed by srawi.
        cpu.s.wr<1>(reg::xer_ca, packed < 0 && (packed & 2));
        return std::int32_t(packed) >> 2;
    }
};

struct Club {
    Cpu& cpu;
    std::uint32_t address;

    std::uint32_t nation() const { return cpu.u32(address + 0x2c); }
};

std::uint32_t call(Cpu& c, std::uint32_t entry, std::uint32_t back,
                   std::initializer_list<std::uint32_t> args = {}) {
    unsigned r = 3;
    for (auto value : args) c.set(r++, value);
    c.call(entry, back);
    return c.r(3);
}

void bad_valuation_input(Cpu& c, const Frame& frame, std::uint32_t pool, std::uint32_t buffer,
                         std::uint32_t first_return, std::uint32_t line) {
    const auto name = frame.at(0x48), extension = frame.at(0x38);
    call(c, 0x10739890, first_return, {pool + 0x37, 0, 0, name, extension});
    const auto version = call(c, 0x10768810, first_return + 4);
    call(c, 0x1077b720, first_return + 0x20, {buffer, pool + 0x6ff, version, name, extension, line});
    call(c, 0x10769b60, first_return + 0x30, {pool + 0x70c, buffer, 0});
    const auto success = c.toc(success_slot);
    c.set({{4, success}, {3, 0}});
    c.put8(success, 0);
}

struct WeeklyWages {
    std::int32_t floor;
    std::uint32_t correction;

    std::uint32_t truncated() const { return std::uint32_t(floor) + correction; }
};

WeeklyWages scale_wage(Cpu& c, std::uint32_t wage, std::uint32_t days) {
    // The original multiplies modulo 2^32 and divides the signed amount by
    // seven. Its pre-rounding quotient and correction also reach r9/r10.
    const auto amount = std::int32_t(wage * days);
    constexpr std::int64_t reciprocal = -1840700269;
    const auto before_shift = amount + std::int32_t((amount * reciprocal) >> 32);
    const auto floor = before_shift >> 2;
    c.s.wr<1>(reg::xer_ca, before_shift < 0 && (before_shift & 3));
    return {floor, std::uint32_t(floor < 0)};
}

std::uint32_t half(Cpu& c, std::uint32_t amount) {
    const auto rounded = std::int32_t(amount + (amount >> 31));
    c.s.wr<1>(reg::xer_ca, rounded < 0 && (rounded & 1));
    return std::uint32_t(rounded >> 1);
}

std::uint32_t lookup_contract(Cpu& c, std::uint32_t holder, std::uint32_t person, std::uint32_t back) {
    return call(c, primary_contract, back, {c.u32(holder), person});
}

std::uint32_t contract_days(Cpu& c, const Contract& contract, std::uint32_t today, std::uint32_t back) {
    const auto day_year = c.u32(today);
    return call(c, remaining_days, back, {contract.expiry(), day_year, c.u32(today + 4)});
}

std::uint32_t call_base_value(Cpu& c, std::uint32_t holder, std::uint32_t person, std::uint32_t employer,
                              std::uint32_t back) {
    return call(c, base_value, back, {c.u32(holder), person, employer, 1, 0xffffffff, 0});
}
}

std::uint32_t transfer_price(Cpu& c, std::uint32_t staff_address, std::uint32_t destination,
                             std::uint32_t mode) {
    Frame frame(c, 0x103fac70, 0x90, {.first_gpr = 24});
    const Kept<29> today(c, c.toc(today_slot));
    Kept<27> person(c, staff_address);
    const Kept<30> contracts(c, c.toc(contracts_slot));
    Kept<28> buyer(c, destination);
    Kept<31> clubs(c, c.toc(clubs_slot));
    const Kept<24> valuation_mode(c, mode);
    Kept<25> buffer_or_wages(c, c.toc(message_buffer_slot));
    const Kept<26> pool(c, c.toc(source_pool_slot));
    const auto date = frame.at(0x40);
    call(c, construct_date, 0x103faca8, {date});
    if (!nonzero_logical(c, person) || !nonzero_logical(c, buyer)) {
        bad_valuation_input(c, frame, pool, buffer_or_wages, 0x103facd0, 0x372d);
        return 0;
    }

    const Staff staff{c, person};
    if (!staff.has_player_record() && staff.has_staff_record()) {
        person = lookup_contract(c, contracts, person, 0x103fad38);
        if (!nonzero(c, std::int32_t(std::uint32_t(person)))) return result(c, 0);
        const Contract contract{c, person};
        if (compare(c, contract.kind(), 2) != 0) return result(c, 0);
        const auto days = contract_days(c, contract, today, 0x103fad70);
        const auto wages = scale_wage(c, contract.wage(), days).truncated();
        const auto minimum = compare(c, std::int32_t(wages), 1000) >= 0 ? wages : 1000u;
        c.set(4, 0x000f0000);
        return result(c, compare(c, std::int32_t(minimum), 1'000'000) <= 0 ? minimum : 1'000'000u);
    }

    const auto limits = c.toc(limited_contracts_slot);
    c.set(3, limits);
    if (compare(c, c.u8(limits), 1) == 0 && !nonzero(c, std::int8_t(std::uint32_t(valuation_mode)))) {
        buyer = lookup_contract(c, contracts, person, 0x103faddc);
        if (!nonzero(c, std::int32_t(std::uint32_t(buyer)))) return result(c, 0);
        const Contract contract{c, buyer};
        if (compare(c, contract.kind(), 2) != 0) return result(c, 0);
        const auto days = contract_days(c, contract, today, 0x103fae14);
        const auto wage = contract.wage();
        const auto employer_offset = contract.employer_index() * club_size;
        const auto employer = c.u32(clubs) + employer_offset;
        const auto manager = c.u32(contracts);
        const auto wages = scale_wage(c, wage, days);
        c.set({{9, std::uint32_t(wages.floor)}, {10, wages.correction}, {0, employer_offset}});
        buffer_or_wages = wages.truncated();
        const auto value = call(c, base_value, 0x103fae60, {manager, person, employer, 1, 0xffffffff, 0});
        return result(c, half(c, buffer_or_wages + value));
    }

    const auto primary = lookup_contract(c, contracts, person, 0x103fae80);
    if (!nonzero_logical(c, primary)) return result(c, 0);
    const Contract contract{c, primary};
    const auto kind = contract.kind();
    c.set(6, std::uint32_t(kind));
    if (compare(c, kind, 5) == 0) return result(c, 0);

    const auto employer_offset = contract.employer_index() * club_size;
    const auto buyer_nation = Club{c, buyer}.nation();
    const auto club_table = c.u32(clubs);
    c.set({{4, employer_offset}, {5, club_table}});
    const bool buyer_has_nation = nonzero_logical(c, buyer_nation);
    clubs = club_table + employer_offset;
    const Club employer{c, clubs};
    if (!buyer_has_nation || !nonzero_logical(c, employer.nation())) return result(c, 0);

    if (compare(c, kind, 2) == 0) {
        c.put16(date + 2, c.u16(contract.expiry() + 2));
        c.put16(date, c.u16(contract.expiry()));
        c.put8(date + 4, c.u8(contract.expiry() + 4));
    } else {
        const auto leap = c.u8(today + 4);
        const auto year = c.u16(today + 2);
        const auto day = c.u16(today);
        c.put16(date + 2, year);
        c.put16(date, day);
        c.put8(date + 4, leap);
    }

    const auto employer_nation = employer.nation();
    const auto domestic = c.toc(valuation_nation_slot);
    const auto employer_id = c.i32(employer_nation + 0x68);
    const auto domestic_id = c.i32(domestic);
    c.set({{3, domestic}, {4, std::uint32_t(domestic_id)}});
    if (compare(c, employer_id, domestic_id) == 0) {
        const auto nation = Club{c, buyer}.nation();
        c.set(3, nation);
        if (compare(c, c.i32(nation + 0x68), domestic_id) == 0)
            return result(c, half(c, call_base_value(c, contracts, person, clubs, 0x103faf58)));
    }

    const auto age = std::int8_t(call(c, age_on_date, 0x103faf74, {person, date}));
    if (compare(c, age, 24) >= 0) return result(c, 0);
    if (!nonzero(c, std::int8_t(call(c, nation_accepts_fee, 0x103faf90, {Club{c, buyer}.nation()}))))
        return result(c, 0);
    if (!nonzero(c, std::int8_t(call(c, nation_accepts_fee, 0x103fafa0, {employer.nation()}))))
        return result(c, 0);
    return result(c, half(c, call_base_value(c, contracts, person, clubs, 0x103fafc4)));
}

std::uint32_t transfer_eligibility(Cpu& c, std::uint32_t staff_address) {
    Frame frame(c, 0x103faff0, 0x80, {.first_gpr = 28});
    Kept<31> pool_or_active(c, c.toc(source_pool_slot));
    const Kept<30> buffer(c, c.toc(message_buffer_slot));
    const Kept<29> current_contract(c, c.toc(current_contract_slot));
    const Kept<28> person(c, staff_address);
    call(c, construct_date, 0x103fb024, {frame.at(0x40)});
    if (!nonzero_logical(c, person)) {
        bad_valuation_input(c, frame, pool_or_active, buffer, 0x103fb044, 0x3797);
        return 0;
    }
    const auto manager = c.toc(contracts_slot);
    pool_or_active = 0;
    const auto address = lookup_contract(c, manager, person, 0x103fb09c);
    const bool found = nonzero_logical(c, address);
    c.put32(current_contract, address);
    if (found && compare(c, Contract{c, address}.kind(), 5) != 0) pool_or_active = 1;

    std::uint32_t employer = 0;
    if (nonzero(c, std::int32_t(std::uint32_t(pool_or_active)))) {
        const auto contract = c.u32(current_contract);
        const auto clubs = c.toc(clubs_slot);
        const auto employer_index = Contract{c, contract}.employer_index();
        c.set(4, clubs);
        employer = c.u32(clubs) + employer_index * club_size;
    }
    c.set(3, employer);
    if (!nonzero_logical(c, employer)) return result(c, 1);
    const auto nation = Club{c, employer}.nation();
    c.set(3, nation);
    if (!nonzero_logical(c, nation)) return result(c, 1);
    return call(c, nation_eligibility, 0x103fb10c, {nation});
}
}
