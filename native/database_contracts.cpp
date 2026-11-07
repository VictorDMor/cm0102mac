// DATABASE.CPP's contract-flag queries (database.hpp).
#include "database.hpp"
#include "database_detail.hpp"
#include "guest_values.hpp"
#include <initializer_list>

namespace cm::aot::database {
namespace {
using guest::compare;
using guest::compare_logical;
constexpr std::uint32_t primary_contract = 0x10082f20, club_contract = 0x10082f90;  // CONTRACT_MANAGER.CPP
constexpr std::int32_t manager_slot = -0x7b7c;   // the contract manager object's word
constexpr std::int32_t last_primary_slot = -0x7b78;

// 0x100d3150, 0x100d3230, 0x100d3300: one body, three tests of the club
// contract's flags. A frame of 0x50 bytes saving r29-r31 and LR; r29 holds
// the person, r30 and r31 the two words' addresses across the calls.
bool contract_flag(Cpu& c, std::uint32_t entry, std::uint32_t person, std::uint32_t the_club,
                   std::initializer_list<std::uint32_t> masks) {
    c.set(0, c.lr());  // mfspr r0,LR: what r0 holds at the first call
    guest::Frame frame(c, entry, 0x50, {.first_gpr = 29});
    const auto manager = c.toc(manager_slot), last_primary = c.toc(last_primary_slot);
    c.set(5, the_club);
    guest::keep(c, {{31, manager}, {30, last_primary}, {29, person}});
    const auto answer = [&](bool value) {
        c.set(3, value);
        return value;
    };
    if (compare(c, std::int32_t(person)) == 0) return answer(false);  // or. r29,r3,r3
    if (compare_logical(c, the_club) == 0) {
        // No club: the club of the primary contract, which is published first.
        c.set(3, c.u32(manager));
        c.set(4, person);
        c.call_in_place(primary_contract, entry + 0x48);
        const auto primary = c.r(3);
        compare_logical(c, primary);
        c.put32(last_primary, primary);
        std::uint32_t club_address = 0;
        if (primary != 0) {
            const auto table = c.toc(club_table::base_slot);
            const auto index = c.u32(c.u32(last_primary) + contract::club_index);
            c.set(4, table);
            c.set(3, c.u32(table));
            club_address = c.r(3) + index * club_table::stride;  // 32-bit wrap, as mulli/add
        }
        c.set(0, club_address);
        c.set(5, club_address);
        if (compare(c, std::int32_t(club_address)) == 0) return answer(false);  // or. r5,r0,r0
    }
    c.set(3, c.u32(manager));
    c.set(4, person);
    c.call_in_place(club_contract, entry + 0x90);
    const auto found = c.r(3);
    if (compare_logical(c, found) == 0) return answer(false);
    for (const auto mask : masks) {
        const auto bits = std::uint32_t(c.u8(found + contract::flags)) & mask;
        c.set(0, bits);
        if (compare(c, std::int32_t(bits)) != 0) return answer(true);  // rlwinm. r0
    }
    return answer(false);
}
}

bool contract_flag_09(Cpu& c, std::uint32_t p, std::uint32_t k) { return contract_flag(c, 0x100d3150, p, k, {0x1, 0x8}); }
bool contract_flag_02(Cpu& c, std::uint32_t p, std::uint32_t k) { return contract_flag(c, 0x100d3230, p, k, {0x2}); }
bool contract_flag_08(Cpu& c, std::uint32_t p, std::uint32_t k) { return contract_flag(c, 0x100d3300, p, k, {0x8}); }
}
