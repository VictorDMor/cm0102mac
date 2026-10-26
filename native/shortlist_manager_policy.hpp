#pragma once
// SHORTLIST_MANAGER.CPP: category eligibility, the club rows' counts and refresh, and the contract/country/row policies.
//
// The routines of this group (0x10365d10-0x1036ff60, scattered between the
// module's other groups) decide who may be shortlisted for a category, which
// rows of a club's shortlist are still live, what a contract offer is worth
// accepting, and a few small helpers the screens and the recruitment pass
// share. Each leaves what the original leaves (guest_abi.hpp).
#include "guest_abi.hpp"
#include "guest_values.hpp"
#include "shortlist_manager_records.hpp"

namespace cm::aot::shortlist_manager {
using guest::Cpu;

// The game records this group reads: shortlist_manager_records.hpp.
namespace policy {
using namespace records;
}

// 0x10365d10: whether the person `staff` (r4) may be shortlisted for
// `category` (r5, 0-15; only its low byte counts): a player record with the
// category's two rating bytes at least 15, or for category 15 (no player
// record needed) a club check and a staff id below person_count - 16. Above
// 15 it reports the assertion. Result 0 or 1 (r3).
std::uint32_t eligible_for_category(Cpu& c, std::uint32_t staff, std::uint32_t category);
// 0x103660b0: counts the live rows (staff and link both set) of `category`
// (low byte; -1: any) in the club's row table (`manager`'s table at +0xdc,
// 12-byte header per club id, 40 rows of 0x2c bytes). In a kind-2 header,
// a row whose staff the transfer manager (0x103eaea0) reports as gone, with
// the club human-controlled (0x1014c2e0) and its manager there over 20 days,
// loses its two references instead of counting. Result: the count (r3).
std::uint32_t count_and_prune_club_rows(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t category);
// 0x10366250: for each of the 40 rows of the club's `header` (a 12-byte
// entry of the manager's table): clears a staff index past the person count;
// for a row with a staff, no link and a contract elsewhere (0x10082f20) that
// is not at `club`, registers the staff for the club through 0x10365590.
// No result (r3 is not read by the caller).
void refresh_unassigned_rows(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t header);
// 0x1036f170: what to do with a contract offer for `person` at `club`:
// 0, 1 or 2 (the recruitment caller rejects 2). `offer` and `reference` are
// signed 64-bit amounts as (high, low) pairs; a reference with a negative
// high word (-1) asks the game for the value instead; the ninth argument,
// a byte on the caller's stack (sp+0x38+3), is read inside.
std::uint32_t classify_contract_offer(Cpu& c, std::uint32_t club, std::uint32_t person, std::uint32_t offer_high,
                                      std::uint32_t offer_low, std::uint32_t considered_club,
                                      std::uint32_t reference_high, std::uint32_t reference_low);
// 0x1036f590: whether the candidate `staff` (r4) passes the country policy
// for `club` (r6): only clubs of a nation whose rule byte is 14 restrict;
// the answer of the transfer manager's check (0x103ed520) is returned as it
// is.
std::uint32_t check_candidate_country_policy(Cpu& c, std::uint32_t staff, std::uint32_t club);
// 0x1036f670: the reference amount of `club` (r4) for the candidate
// `person` (r5) in `category` (r6); -1 for a person without a player record.
// r3 is passed on and not read.
std::uint32_t candidate_category_reference(Cpu& c, std::uint32_t unused, std::uint32_t club, std::uint32_t person,
                                           std::uint32_t category);
// 0x1036f6f0: the reference amount of `club` (r4) for `category` (r6): the
// club's reputation times 0x226 / 0x6e (categories 5, 6), or by the club's
// nation 0x226, 0x1db or 0x14a (category 4); -1 for the others. A leaf
// that leaves r5/r6 as the original does; r5 on entry is not read.
std::uint32_t club_category_reference(Cpu& c, std::uint32_t club, std::uint32_t category);
// 0x1036f7e0: writes the name of shortlist role `kind` (r4, 0-8 by jump
// table, others the last one) to `destination` (r5) with 0x10727890; a
// `limit` (r6) below 30 reports the assertion instead.
std::uint32_t write_role_name(Cpu& c, std::uint32_t kind, std::uint32_t destination, std::uint32_t limit);
// 0x1036f920: whether `person` (r4) fits `category` (r5): categories 5 and 6
// need the low byte of the halfword at +0x5c below 24. A person with no
// player record reports the assertion.
std::uint32_t fits_category(Cpu& c, std::uint32_t person, std::uint32_t category);
// 0x1036fbf0: whether the row's `contract` passes the gate (1: yes);
// `amounts` (r4) holds the pair at +4 and +0xc. Writes the price the
// transfer manager puts on the person to *`price_out` (r7), cleared first.
std::uint32_t evaluate_row_contract_gate(Cpu& c, std::uint32_t unused, std::uint32_t amounts, std::uint32_t contract,
                                         std::uint32_t club, std::uint32_t price_out);
// 0x1036ff00: whether the name of `record` (r3) equals `text` (r4); 0 for no record.
std::uint32_t record_name_equals(Cpu& c, std::uint32_t record, std::uint32_t text);
}
