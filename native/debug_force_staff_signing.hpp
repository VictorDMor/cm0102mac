#pragma once
#include <cstdint>
#include <unordered_map>

namespace cm::aot {
struct State;
namespace guest { class Cpu; }

struct StaffSigningClubs {
    // Club IDs plus one, so an untouched history page represents an ordinary offer.
    std::uint32_t employer{}, buyer{};
    explicit operator bool() const { return employer && buyer; }
};

struct ForcedStaffSigning {
    std::uint32_t offer, revision, person, role;
    StaffSigningClubs clubs;
};
using ForcedStaffSignings = std::unordered_map<std::uint32_t, ForcedStaffSigning>;
ForcedStaffSignings& forced_staff_signings(State&);
const ForcedStaffSignings& forced_staff_signings(const State&);
void clear_forced_staff_signings(State&);

std::uint32_t staff_signing_history(const guest::Cpu&, std::uint32_t screens);
StaffSigningClubs staff_signing_clubs(const guest::Cpu&, std::uint32_t history);
void set_staff_signing_clubs(guest::Cpu&, std::uint32_t history, StaffSigningClubs);
void record_forced_staff_signing(State&, StaffSigningClubs, std::uint32_t person, std::uint32_t buyer,
                                 std::uint32_t role, std::uint32_t index, std::uint32_t revision);
bool forced_staff_contract(State&, std::uint32_t contract, std::uint32_t person);
}
