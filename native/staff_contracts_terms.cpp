#include "staff_contracts.hpp"
#include "staff_contracts_detail.hpp"

namespace cm::aot::staff_contracts {
namespace {
using detail::block;
using detail::ContractRecord;
using guest::Cpu;

constexpr unsigned options = 5, bonuses = 3;
constexpr std::uint32_t no_bonus = UINT32_MAX;

bool has_kind(Cpu& c, std::uint32_t contract, std::int32_t wanted, std::uint32_t entry) {
    block(c, entry, 9);
    const auto kind = detail::unpack_kind(c.u8(contract + ContractRecord::packed_kind));
    c.s.wr<1>(guest::reg::xer_ca, std::uint32_t(kind) <= std::uint32_t(wanted));
    c.set(0, kind == wanted);
    return kind == wanted;
}

void write_bonus(Cpu& c, std::uint32_t contract, std::int8_t bonus, std::uint32_t amount) {
    const auto displacement = std::uint32_t(bonus * 4);
    const auto selected = contract + displacement;
    c.set({{0, displacement}, {3, selected}});
    c.put32(selected + ContractRecord::bonus, amount);
}
}

void enable_option(Cpu& c, std::uint32_t contract, std::int8_t option) {
    block(c, 0x10374af0, 1);
    c.set(0, std::uint32_t(option));
    block(c, 0x10374af4, 2);
    if (guest::compare(c, option, 1) == 0) {
        block(c, 0x10374afc, 7);
        c.set(0, 0);
        for (unsigned i = 0; i < options; ++i)
            c.put8(contract + ContractRecord::option + i, 0);
    } else {
        block(c, 0x10374b18, 2);
        c.set(0, 0);
        c.put8(contract + ContractRecord::option + 1, 0);
    }
    block(c, 0x10374b20, 5);
    const auto selected = contract + std::uint32_t(option);
    c.set({{0, std::uint32_t(option)}, {4, 1}, {3, selected}});
    c.put8(selected + ContractRecord::option, 1);
}

void clear_option(Cpu& c, std::uint32_t contract, std::int8_t option) {
    block(c, 0x10374b60, 1);
    c.set(0, std::uint32_t(option));
    block(c, 0x10374b64, 4);
    const auto selected = contract + std::uint32_t(option);
    c.set({{4, 0}, {3, selected}});
    c.put8(selected + ContractRecord::option, 0);
}

void set_bonus(Cpu& c, std::uint32_t contract, std::int8_t bonus, std::uint32_t amount) {
    block(c, 0x10374b40, 1);
    c.set(0, std::uint32_t(bonus));
    block(c, 0x10374b44, 4);
    write_bonus(c, contract, bonus, amount);
}

void clear_bonus(Cpu& c, std::uint32_t contract, std::int8_t bonus) {
    block(c, 0x10374b80, 1);
    c.set(0, std::uint32_t(bonus));
    block(c, 0x10374b84, 5);
    c.set(4, no_bonus);
    write_bonus(c, contract, bonus, no_bonus);
}

void reset_terms(Cpu& c, std::uint32_t contract) {
    block(c, 0x10374ba0, 13);
    c.set({{4, 0}, {0, no_bonus}});
    c.put8(contract + ContractRecord::terms_flag, 0);
    c.put32(contract + ContractRecord::terms_marker, no_bonus);
    for (unsigned i = 0; i < options; ++i)
        c.put8(contract + ContractRecord::option + i, 0);
    for (unsigned i = 0; i < bonuses; ++i)
        c.put32(contract + ContractRecord::bonus + i * 4, no_bonus);
}

bool is_expired(Cpu& c, std::uint32_t contract) {
    return has_kind(c, contract, 3, 0x10374ce0);
}

bool is_rolling(Cpu& c, std::uint32_t contract) {
    return has_kind(c, contract, 1, 0x10374d50);
}

bool supports_bonus(Cpu& c, std::uint32_t person, std::int8_t bonus) {
    constexpr unsigned attributes = 0x14, playing_role = 0x0f;
    constexpr unsigned keeper = 20, clean_sheet_bonus = 2;
    block(c, 0x103803b0, 3);
    const auto profile = c.u32(person + attributes);
    c.set(3, profile);
    if (guest::compare_logical(c, profile, 0) == 0) {
        block(c, 0x103803f8, 2);
        return false;
    }
    block(c, 0x103803bc, 3);
    const auto role = c.u8(profile + playing_role);
    c.set(0, role);
    const auto index = std::uint32_t(bonus);
    const bool clean_sheet = index == clean_sheet_bonus;
    if (guest::compare(c, role, keeper) == 0) {
        block(c, 0x103803c8, 6);
        c.s.wr<1>(guest::reg::xer_ca, index <= clean_sheet_bonus);
        c.set(0, clean_sheet);
        return clean_sheet;
    }
    block(c, 0x103803e0, 6);
    c.set(4, index);
    c.s.wr<1>(guest::reg::xer_ca, index <= clean_sheet_bonus);
    const auto distance = index - clean_sheet_bonus;
    c.set(0, distance | (0u - distance));
    return !clean_sheet;
}

bool has_release_terms(Cpu& c, std::uint32_t contract) {
    constexpr std::array tests{std::pair{0u, 2u}, std::pair{1u, 1u}, std::pair{3u, 2u}};
    bool found = false;
    std::uint32_t pc = 0x10380520;
    for (const auto& [index, enabled] : tests) {
        block(c, pc, 3);
        const auto option = c.u8(contract + ContractRecord::option + index);
        c.set(0, option);
        found = guest::compare(c, option, enabled) == 0;
        if (found) break;
        pc += 12;
    }
    block(c, found ? 0x10380544 : 0x1038054c, 2);
    return found;
}

void set_release_amount(Cpu& c, std::uint32_t contract, std::uint32_t amount) {
    block(c, 0x10380700, 2);
    c.put32(contract + ContractRecord::release_amount, amount);
}

void clear_status_bits(Cpu& c, std::uint32_t contract, std::uint32_t mask) {
    block(c, 0x1037fa50, 4);
    const auto address = contract + ContractRecord::status;
    const auto remaining = c.u32(address) & ~mask;
    c.set(0, remaining);
    c.put32(address, remaining);
}

void reset_status_bits(Cpu& c, std::uint32_t contract) {
    block(c, 0x1037fa60, 3);
    c.set(0, 0);
    c.put32(contract + ContractRecord::status, 0);
}

std::uint32_t status_bits(Cpu& c, std::uint32_t contract) {
    block(c, 0x1037fa70, 2);
    return c.u32(contract + ContractRecord::status);
}
}
