#pragma once
// SHORTLIST_MANAGER.CPP: the daily processing that drives the others: shortlist creation, the day's passes and their entry points.
#include "guest_abi.hpp"

namespace cm::aot::shortlist_manager {
using guest::Cpu;
namespace daily {
using guest::Cpu;
// 0x103573c0: the new game's shortlists ("Assessing Squads", then "Creating
// Shortlists"): every club's squad assessment, each person's contract
// valuation, the stored-category reset and every club's budget share-out.
void create_shortlists(Cpu& c, std::uint32_t manager);
// 0x10357cb0: one club's shortlist refresh (the category counts, the squad
// assessment, the budget share-out).
void refresh_club_shortlist(Cpu& c, std::uint32_t manager, std::uint32_t club);
// 0x10359020: shares the club's budget out among its open shortlist rows,
// runs the candidate scan for each, then may set the rest aside on a later
// player row whose target plays at the club under another club's contract
// (inferred: a loanee it wants to keep; interest score >= 3.15, a random
// digit or the contract's end decides).
void share_out_budget(Cpu& c, std::uint32_t manager, std::uint32_t club);
// 0x1036d6e0: the day's approaches for one club: each row with a target
// and no approach yet is assessed and may get one (0x103636b0).
void approach_targets(Cpu& c, std::uint32_t manager, std::uint32_t club);
// 0x1036ec30: rebuilds a club's shortlist after a change of manager
// (inferred): drops stale and foreign targets, removes the club's links,
// reassesses the squad and the budgets and revalues the contracts.
void rebuild_club_shortlist(Cpu& c, std::uint32_t manager, std::uint32_t club);
// 0x1036f100: clears the approach (+0x18) of every row of the club whose
// target is *person.
void forget_approaches(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t person);
}
using daily::create_shortlists;
using daily::refresh_club_shortlist;
using daily::share_out_budget;
using daily::approach_targets;
using daily::rebuild_club_shortlist;
using daily::forget_approaches;
}

// What the daily routines' files share: the records they read, the TOC
// slots shortlist_manager_detail.hpp does not name, the constants and the
// inlined idioms.
#include "aot_runtime.hpp"
#include "guest_values.hpp"
#include "shortlist_manager_records.hpp"

namespace cm::aot::shortlist_manager::daily {
using guest::Field;
using namespace records;  // the game records (shortlist_manager_records.hpp)
// TOC slots (lwz rX,slot(r2)): the address of the game object or value.
namespace game_slot {
constexpr std::int32_t finances = -0x7ab8;       // the club finances lookup's object (0x10103510's first argument)
constexpr std::int32_t transfers = -0x7b40;      // the transfer manager (inferred: its bids and offers)
constexpr std::int32_t last_contract = -0x7b78;  // int32: the contract 0x10082f20 last returned, kept by the callers
constexpr std::int32_t humans = -0x7a54;         // the human managers' table (club_control.cpp)
constexpr std::int32_t club_order = -0x765c;     // int32[]: the club ids in processing order
constexpr std::int32_t staff_roles = -0x7b9c;    // 100-byte records per person id (inferred: staff roles)
constexpr std::int32_t histories = -0x7ba0;      // the object 0x1018c340 takes (inferred: player histories)
}
// The constants the routines read from the pool (slot::constants +offset).
namespace constant {
constexpr double half = 0.5;                  // +0x68
constexpr double one_point_three = 1.3;       // +0x70
constexpr double interest_threshold = 3.15;   // +0x78
}
}
