#include "contract_manager_detail.hpp"
#include "contract_manager_adapter.hpp"
#include "guest_frame_observation.hpp"
#include "match_turbo.hpp"
#include "routine_check.hpp"

#include <optional>

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
// Keep the original validation and index loads in the leaf lookups.
std::uint32_t find_contract(Cpu& c, std::uint32_t manager_address, std::uint32_t person, bool second) {
    if (!nonzero_logical(c, person)) return result(c, 0);
    const auto count_address = c.toc(slot::person_count);
    c.set(5, count_address);
    const auto checked_id = c.i32(person);
    c.set(6, std::uint32_t(checked_id));
    const auto count = c.i32(count_address);
    c.set(0, std::uint32_t(count));
    if (compare(c, checked_id, count) >= 0) return result(c, 0);

    const auto person_offset = c.u32(person) * Contract::size;
    c.set(0, person_offset);
    const auto index_holder = c.toc(slot::contract_index);
    c.set(4, index_holder);
    const auto index_base = c.u32(index_holder);
    const auto index_row = index_base + person_offset;
    c.set(4, second ? index_row : index_base);
    const auto index = c.i32(index_row + (second ? 4 : 0));
    c.set(0, std::uint32_t(index));
    if (compare(c, index) < 0) return result(c, 0);
    const auto offset = std::uint32_t(index) * Contract::size;
    c.set(0, offset);
    return result(c, Manager{c, manager_address}.records().get() + offset);
}
}

std::uint32_t primary_contract(Cpu& c, std::uint32_t manager, std::uint32_t person) {
    return find_contract(c, manager, person, false);
}
std::uint32_t secondary_contract(Cpu& c, std::uint32_t manager, std::uint32_t person) {
    return find_contract(c, manager, person, true);
}
std::uint32_t primary_terms(Cpu& c, std::uint32_t manager, std::uint32_t person) {
    // This entry inlines the primary lookup; it has no call frame/link.
    const auto contract = find_contract(c, manager, person, false);
    return result(c, nonzero_logical(c, contract) ? c.u32(contract + 12) : 0);
}

namespace {
constexpr std::uint32_t club_contract_entry = 0x10082f90;
constexpr unsigned club_contract_frame_size = 0x50;
constexpr unsigned resolver_flag_offset = 0x38;

// A value, including zero, completes the lookup. A primary at a different
// club still requires the secondary lookup and possibly the resolver.
std::optional<std::uint32_t> lookup_primary_for_club(Cpu& c, std::uint32_t manager_address,
                                                    std::uint32_t person, std::uint32_t requested_club) {
    const auto count_address = c.toc(slot::person_count);
    c.set(6, count_address);
    Kept<31> club(c, requested_club);
    if (!nonzero_logical(c, person) || !nonzero_logical(c, club)) return result(c, 0);
    const bool observed = c.s.memory.date_observer || c.s.memory.read_trace;

    std::int32_t id;
    const auto valid_person = [&] {
        nonzero_logical(c, person);
        id = c.i32(person);
        const auto count = c.i32(count_address);
        c.set(5, std::uint32_t(id));
        c.set(0, std::uint32_t(count));
        return compare(c, id, count) < 0;
    };
    if (!valid_person()) return result(c, 0);

    // The frame has already been written. The repeated validations and
    // offset calculation have no calls or memory writes between them.
    Kept<30> primary(c, 0);
    if (!observed || valid_person()) {
        const auto offset = (observed ? c.u32(person) : std::uint32_t(id)) * Contract::size;
        c.set(0, offset);
        const auto index_holder = c.toc(slot::contract_index);
        c.set(5, index_holder);
        const auto indices = c.u32(index_holder);
        c.set(5, indices);
        const auto index = c.i32(indices + offset);
        c.set(0, std::uint32_t(index));
        if (compare(c, index) >= 0) {
            const auto contract_offset = std::uint32_t(index) * Contract::size;
            c.set(0, contract_offset);
            const auto records = Manager{c, manager_address}.records().get();
            c.set(5, records);
            primary = records + contract_offset;
        }
    }
    if (!nonzero_logical(c, primary)) return result(c, 0);
    const auto primary_club = Contract{c, primary}.club_id().get();
    const auto club_id = c.i32(club);
    c.set(5, std::uint32_t(primary_club));
    c.set(0, std::uint32_t(club_id));
    if (compare(c, primary_club, club_id) == 0) return result(c, primary);

    return std::nullopt;
}

std::uint32_t lookup_other_contract(Cpu& c, std::uint32_t manager_address, std::uint32_t person,
                                   std::uint32_t resolver_flag) {
    Kept<31> club(c, c.r(31));
    Kept<30> primary(c, c.r(30));
    Program program(c);
    // The secondary lookup is a real call of another rewritten routine;
    // the external club resolver retains its byte output and alias effects.
    Kept<29> secondary(c, program.call(0x10083110, 0x10083070, {manager_address, person}));
    if (nonzero(c, std::int32_t(std::uint32_t(secondary)))) {
        const auto secondary_club = Contract{c, secondary}.club_id().get();
        const auto id = c.i32(club);
        c.set(4, std::uint32_t(secondary_club));
        c.set(0, std::uint32_t(id));
        if (compare(c, secondary_club, id) == 0) return result(c, secondary);
    }
    const auto resolved = program.call(0x100ce320, 0x1008309c, {club, resolver_flag, 1});
    if (nonzero_logical(c, resolved)) {
        const auto flag = c.i8(resolver_flag);
        c.set(0, std::uint32_t(std::int32_t(flag)));
        if (!nonzero(c, flag)) club = resolved;
    }
    const auto final_primary_club = Contract{c, primary}.club_id().get();
    const auto resolved_id = c.i32(club);
    c.set(0, std::uint32_t(final_primary_club));
    c.set(3, std::uint32_t(resolved_id));
    if (compare(c, final_primary_club, resolved_id) == 0) return result(c, primary);
    if (nonzero_logical(c, secondary)) {
        const auto final_secondary_club = Contract{c, secondary}.club_id().get();
        c.set(0, std::uint32_t(final_secondary_club));
        if (compare(c, final_secondary_club, resolved_id) == 0) return result(c, secondary);
    }
    return result(c, 0);
}
}

std::uint32_t club_contract(Cpu& c, std::uint32_t manager_address, std::uint32_t person, std::uint32_t requested_club) {
    const auto sp = c.sp();
    const auto& memory = c.s.memory;
    const bool unobserved = sp >= club_contract_frame_size && std::uint64_t(sp) + 12 <= (UINT64_C(1) << 32) &&
        !memory.date_observer && !memory.read_trace &&
        !stack_frame_writes_observed(memory, sp, club_contract_frame_size) && !contract_manager_call_hook &&
        !dispatch_probe && !routine_checker && !match_pc_profile_enabled();
    if (!unobserved) {
        Frame frame(c, club_contract_entry, club_contract_frame_size, {.first_gpr = 29});
        if (const auto early = lookup_primary_for_club(c, manager_address, person, requested_club)) return *early;
        return lookup_other_contract(c, manager_address, person, frame.at(resolver_flag_offset));
    }
    // Publish the same frame stores. The primary prefix cannot call out or
    // change the save slots, so successful early returns reuse their values.
    const auto r29 = c.r(29), r30 = c.r(30), r31 = c.r(31), lr = c.lr();
    const auto base = sp - club_contract_frame_size;
    c.put32(sp - 12, r29);
    c.put32(sp - 8, r30);
    c.put32(sp - 4, r31);
    c.put32(sp + 8, lr);
    c.put32(base, sp);
    c.set(1, base);
    const auto early = lookup_primary_for_club(c, manager_address, person, requested_club);
    if (early) {
        c.set(30, r30);
        c.set(31, r31);
        c.set(0, lr);
        c.set(1, sp);
        return *early;
    }
    // A dependency can observe the current frame or replace a saved slot.
    GuestNativeScope scope(c.s, club_contract_entry);
    scope.frame(base);
    const auto value = lookup_other_contract(c, manager_address, person, base + resolver_flag_offset);
    c.set(29, c.u32(sp - 12));
    c.set(30, c.u32(sp - 8));
    c.set(31, c.u32(sp - 4));
    c.set(0, c.u32(sp + 8));
    c.set_lr(c.r(0));
    c.set(1, sp);
    return value;
}
}
