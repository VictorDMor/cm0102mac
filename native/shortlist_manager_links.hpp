#pragma once
// SHORTLIST_MANAGER.CPP: the staff-club links: lookups, collection, insertion, the primary-contract marks and the links' transitions.
//
// The shortlist manager object (224 bytes) keeps, in its first 0x18 bytes,
// two sorted arrays of 8-byte (staff id, club id) pairs: the first sorted by
// staff then club (comparator 0x10366410), the second by club then staff
// (0x103663b0), each with a capacity and a used count. At +0xdc it holds a
// table with one 12-byte entry per club id; an entry points at the club's 40
// link rows (0x2c bytes each). The routines here search and change both.
#include "guest_abi.hpp"
#include "guest_values.hpp"
#include "shortlist_manager_records.hpp"

namespace cm::aot::shortlist_manager {
using guest::Cpu;
// The records these routines work on (shortlist_manager_records.hpp:
// Manager's link indexes and club table, ClubEntry, Row), and one of their own.
namespace links {
using namespace records;
// One (staff id, club id) pair of an index.
struct LinkPair {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t size = 8;
    guest::Field<std::int32_t> staff() const { return {c, address}; }
    guest::Field<std::int32_t> club() const { return {c, address + 4}; }
};

// TOC slots only these routines read (see shortlist_manager_detail.hpp for the module's).
namespace link_slot {
constexpr std::int32_t staff_service = -0x7b1c;      // the object whose pointer the staff predicate 0x1030fbf0 takes
constexpr std::int32_t transfer_manager = -0x7b40;   // the transfer manager (owns the shortlist manager at +0x988)
constexpr std::int32_t last_contract = -0x7b78;      // int32*: scratch holding the last contract looked up
constexpr std::int32_t staff_status = -0x7b9c;       // staff status records, 100 bytes per staff id
constexpr std::int32_t nation_a = -0x7a18, nation_b = -0x79fc, nation_c = -0x7a14;  // nation ids (inferred: special nations)
}
}

// 0x10362d70: whether the pair (staff id, club id) is in the staff-sorted
// index; a null club matches any club (key -1). `links` is the store.
std::uint32_t contains_staff_club_link(Cpu& c, std::uint32_t links, std::uint32_t staff, std::uint32_t club);
// 0x10362e00: appends to the list `out` every pair of the club-sorted index whose club is `club`'s.
void collect_club_links(Cpu& c, std::uint32_t links, std::uint32_t club, std::uint32_t out);
// 0x10362ef0: collects the staff's links into the list `out` (cleared first when non-null):
// the last 16 person records (unless mode is 3) that pass the staff predicate, then
// the cached links unless `skip_cached`. `force`, `mode` and `skip_cached` are
// byte flags as the caller's registers hold them (tested as signed bytes).
void collect_staff_links(Cpu& c, std::uint32_t links, std::uint32_t staff, std::uint32_t out, std::uint32_t force,
                         std::uint32_t mode, std::uint32_t skip_cached);
// 0x10363130: marks the staff's primary contract (flag one, status 7) and applies its first consequences.
void mark_primary_contract(Cpu& c, std::uint32_t recruitment, std::uint32_t staff, std::uint32_t follow_up);
// 0x103632f0: adds the pair (staff id, club id) to both indexes, allocating or growing them as needed.
void add_staff_club_link(Cpu& c, std::uint32_t links, std::uint32_t staff, std::uint32_t club);
// 0x103636b0: tries to start a link transition for (staff, club); returns -1 or the transfer call's result.
std::uint32_t schedule_staff_transition(Cpu& c, std::uint32_t links, std::uint32_t staff, std::uint32_t club);
// 0x10363b80: for every link of the staff, marks the club entry changed (100) and, with `clear_references`, drops the rows' reference.
void reset_linked_club_rows(Cpu& c, std::uint32_t links, std::uint32_t staff, std::uint32_t clear_references);
// 0x10365590: changes the staff's links to `club` according to `mode` (1-5) and `filter`; returns 1 (0 when an index is missing).
std::uint32_t transition_staff_links(Cpu& c, std::uint32_t links, std::uint32_t staff, std::uint32_t club,
                                     std::uint32_t mode, std::uint32_t filter);
// 0x10365c30, 0x10365c80, 0x10365cc0: the contracts' flag bits (+0x5c).
// (They take the staff in r4, which the routine passes on to the contract lookup as it finds it.)
std::uint32_t clear_primary_contract_flag_one(Cpu& c);
std::uint32_t set_primary_contract_flag_two(Cpu& c);
std::uint32_t clear_primary_contract_flag_two(Cpu& c);
// 0x1036d240: the club's link row for the staff: a row whose staff (or, with
// `either`, intervening) id is his, of the ordinary kind (type not 11) when
// `ordinary` is non-zero, of type 11 otherwise; 0 when none.
std::uint32_t find_link_row(Cpu& c, std::uint32_t links, std::uint32_t club, std::uint32_t staff, std::int8_t ordinary,
                            std::int8_t either);
// 0x1036d2f0: the link status (1-6) the game derives for a staff and a club from their ratings.
std::uint32_t derive_link_status(Cpu& c, std::uint32_t links, std::uint32_t staff, std::uint32_t club);
// 0x1036d510: the link status (0-15, 7: none) of the staff at the club; `type` (may be null) receives the row's type or the staff's byte at +0x62.
std::uint32_t link_status(Cpu& c, std::uint32_t links, std::uint32_t staff, std::uint32_t club, std::uint32_t type);
// 0x1036fa00: for each club the staff is linked to (not of the first kind), sets bit 2 of his link row's flags; with a
// `list` of earlier staff it looks the club up through them instead, and `skip_club` is left out.
void mark_linked_rows(Cpu& c, std::uint32_t links, std::uint32_t staff, std::uint32_t skip_club, std::uint32_t list);
}
