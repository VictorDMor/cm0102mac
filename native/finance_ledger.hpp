#pragma once
// FINANCE.CPP: a club's account. Finding it, booking income and expenses into its ledger, the
// question "how well off is this club", and the manager's array of accounts.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::finance {
using guest::Cpu;

// A line of an account's ledger: its offset, whether it is 64 bits wide, and whether an amount
// booked on it is money coming in.
struct LedgerLine { std::uint32_t offset; bool wide, income; };
namespace ledger {
// Names from who books on a line and from the news texts; the ones marked (inferred) are guesses.
inline constexpr LedgerLine transfer_fees_paid{0x70, true, false},      // 0x100f70e0 (TRANSFER_MANAGER.CPP)
                            transfer_fees_received{0x78, true, true},   // 0x100f7050 (TRANSFER_MANAGER.CPP)
                            expense_80{0x80, false, false},             // 0x100f7230
                            expense_88{0x88, false, false},             // 0x100f70a0 (MANAGER_MANAGER.CPP, COMP_STATS.CPP)
                            expense_90{0x90, false, false},             // 0x100f7170 (TRANSFER_MANAGER.CPP)
                            contract_costs{0x98, false, false},         // 0x100f71b0 (CONTRACT_MANAGER.CPP)
                            expense_9c{0x9c, false, false},             // 0x100f7130 (TRANSFER_MANAGER.CPP)
                            prize_money{0xa8, false, true},             // 0x100f71f0 (the cups and leagues)
                            league_income{0xb8, false, true};           // 0x100f5730 (LEAGUE.CPP); broadcast share (inferred)
// Lines the other files of the unit book on directly: 0x84 overdraft cost, 0x8c ground and running costs,
// 0x94 the squad budget, 0xa0 match income, 0xa4 earnings, 0xac gate income, 0xb0 interest, 0xb4 donations and
// owner funding, 0xbc matchday extras (all inferred from what the code computes).
}
// Adds `amount` to a line in both of its copies and moves the balance by it. A word line takes the
// low word; its income is signed and its expense unsigned when it reaches the 64-bit balance.
void book(Cpu&, std::uint32_t account, const LedgerLine&, std::uint64_t amount);

std::uint32_t account_of_club(Cpu&, std::uint32_t manager, std::uint32_t club);      // 0x10103510
// 2 rich, 1 comfortable, 0 sound, -1 short, -2 in trouble, by the balance against the club's reputation.
std::int32_t wealth(Cpu&, std::uint32_t account);                                     // 0x100f55b0
void scale_attendance(Cpu&, std::uint32_t account, std::int8_t percent);             // 0x100f4f20
std::uint32_t destroy_account(Cpu&, std::uint32_t account, std::int16_t deleting);   // 0x100f1010
std::uint32_t destroy_manager(Cpu&, std::uint32_t manager, std::uint32_t deleting);     // 0x100f8720
// Refreshes (0x100f4f80) the accounts of the clubs before the first inactive one.
void refresh_leading_clubs(Cpu&, std::uint32_t manager);                             // 0x100fd1a0
}
