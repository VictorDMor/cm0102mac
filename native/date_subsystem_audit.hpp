#pragma once
#include "aot_runtime.hpp"
namespace cm::aot {
inline constexpr std::array<std::uint32_t,26> date_subsystem_entries{
    0x107156b0,0x107156f0,0x10715880,0x107158a0,0x10715a30,
    0x10715bc0,0x10715e00,0x10716130,0x107164f0,0x107168a0,
    0x10716b00,0x10716eb0,0x10717270,0x10717620,
    0x10717a10,0x10717cb0,0x10717dd0,0x10718440,
    0x107184d0,0x10718570,0x10718650,0x10718700,0x107187d0,
    0x107188a0,0x107189d0,0x10718af0};
bool try_modern_date_subsystem(State&,std::uint32_t entry);
void open_date_subsystem_audit(const std::filesystem::path&,bool record,std::uint32_t perturb_entry=0);
void close_date_subsystem_audit(bool require_end=false);
std::string date_subsystem_call_summary(const State&);
}
