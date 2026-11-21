#pragma once
// PLAYER_REGEN.CPP (tools/player_regen_audit.py): the regen object (retirements of players and staff, the queue of
// regenerated people, the pools of newgen players and non-player staff). player_regen_detail.hpp names the records the
// routines read. Each block of the module declares its routines in its own section below; a routine takes the `Cpu`
// where the original leaves registers a caller can read (player_regen_routines*.inc lines call it with the guest ABI's
// arguments).
//
// Dependencies still generated, called through the dispatcher (c.call): none of the pilots'. The pooled generator's
// refill 0x1077b770 is C++ (rng_core) and is called through c.call at the original's return addresses.
#include "guest_abi.hpp"

namespace cm::aot::player_regen {
using guest::Cpu;

// Block `regen_queries` (player_regen_queries.cpp): small queries over the regen object's lists and the people records.
// 0x102de3c0: the class (0..3) of the person whose id word `id_pointer` points at: bits 2..3 of byte +0xc of the 100-byte
// kind-table record for that id; nonzero marks a person already retired or inactive (inferred). Leaves r4 (the table's
// base) as the original does.
std::int32_t person_class(Cpu&, std::uint32_t id_pointer);
// 0x102de3f0: the first entry of a list A whose id equals the word `key` points at, if it also passes the test `kind` selects
// (nonzero: its kind byte is 8, a pending injury retirement; zero: its announced byte is nonzero), else 0 (a later entry
// with the same id is not looked at). Leaves r6 (the key), r7 (the entry reached) and r8 (its index) as the original does.
std::uint32_t find_retirement_entry(Cpu&, std::uint32_t list, std::uint32_t key, std::int8_t kind);
// 0x102dbe90: whether the word at +0x20 of the regen object is nonzero (inferred: the non-player staff pool has entries).
bool staff_pool_has_entries(Cpu&, std::uint32_t regen_object);
// 0x102eb4e0: qsort comparator of the list B entries (the sort passes the entries themselves, null ones first): the
// unsigned halfword at +6 ascending, then the signed halfword at +4 ascending, then the id (word +0) ascending. Leaves r4
// (when the halfwords tie) and r5 as the original does.
std::int32_t compare_queue_entries(Cpu&, std::uint32_t first, std::uint32_t second);
// 0x102eb550: comparator of the list A entries (null ones first): each entry's due date is its first date (halfword day +8,
// halfword year +0xa) unless that is earlier than the game's current date (TOC -0x7b80), then the second (+0x10, +0x12);
// the due dates are compared by year then day, then the ids. Leaves r4..r8 as the original does.
std::int32_t compare_retirement_entries(Cpu&, std::uint32_t first, std::uint32_t second);
// 0x102eb610: whether a list A has an entry whose id equals the word `key` points at (false for a null key or an empty
// list). Leaves r5..r7 as the original does.
bool retirement_queued(Cpu&, std::uint32_t list, std::uint32_t key);

// Block `candidate_score` (player_regen_random.cpp, player_regen_candidates.cpp).
// 0x102da630: the game's pooled Random(bound): 0 for a bound of 0 (no draw); else one draw (the cursor moves two bytes; past
// the pool's end it returns to the start and the salt is renewed from the generator's refill) giving the sample halfword plus
// the salt, taken modulo the bound (the remainder's sign made that of the bound); for a bound of 0x10000 or more in magnitude
// the draw is scaled instead: n / 65535 times the remainder by 65535 (doubles, truncated) plus a tenth (rounded) of a second
// draw below n * 10 / 65535 (the product wraps), capped at n - 1. The original inlines four levels and then calls itself; the
// second bound never exceeds 32768 in magnitude, so only two levels run. Leaves r4..r9 as the original does for a scaled draw.
std::int32_t draw_below(Cpu&, std::int32_t bound);
// 0x102dc410: the score of a person as the replacement for a retiring one: -10000 unless the person is a pure player
// (the staff word +0x1c is zero, the kind-table class is 0, age at least 31); otherwise a draw below (age - 31) * 150, the
// club's reputation against the player's value, a cubed term of the player record's byte +0x2f (pow, drawn from), weights of
// the attribute bytes (two of them squared through pow), 75 more for a club with no word +0x90, for an unattached person the
// role byte squared and a doubling when a draw modulo 10000 is below the player's +0xc, and last a draw below the score.
// The first argument register is ignored. Callers read r3 only (tools/register_contract.py --after).
std::int32_t candidate_score(Cpu&, std::uint32_t person);
// 0x102dbf00: the person whose candidate score is highest and positive among those whose nation record's word +0x68 equals
// `key` (the people table without its last 16), or when there is none, among all of them; 0 when no score is positive.
// Callers read r3 only.
std::uint32_t best_candidate(Cpu&, std::uint32_t key);

// Block `regen_lists` (player_regen_lists.cpp): the regen object's four lists (A retirement entries of 0x1c bytes at +4/+8/+0xc, B
// regen queue of 0xc at +0x10/+0x14/+0x18, C non-player staff pool of 0x10 at +0x1c/+0x20/+0x24, D player pool of 0x10 at
// +0x3c/+0x40/+0x44: entries, count, capacity). An appender with a full list (count = capacity - 1) reallocates the block
// 250 entries larger (0x107751b0) and, when that gives no block, reports the game's assertion and ends the game (0x1076f7d0).
// 0x102deb70 / 0x102ded90: removes the first entry of list C / D whose first word equals `key` (shifting the later entries
// down, filling the vacated last entry's first three words with -1, count - 1); nothing when the key is absent.
void remove_staff_pool_entry(Cpu&, std::uint32_t regen, std::int32_t key);
void remove_player_pool_entry(Cpu&, std::uint32_t regen, std::int32_t key);
// 0x102dbeb0: takes the first entry of list C: its three words go to the three out pointers (in that order) and the entry with
// the id the first out pointer then holds is removed from C. Callers (CONTRACT_MANAGER) read nothing it leaves.
void pop_staff_pool_entry(Cpu&, std::uint32_t regen, std::uint32_t out_id, std::uint32_t out_nation, std::uint32_t out_third);
// 0x102dea50: appends {first, second, third} to list C. Its growth test reads list D's count and capacity (a quirk of the
// original: the entry is appended to C whichever list is full, and C grows only when D is).
void append_staff_pool(Cpu&, std::uint32_t regen, std::uint32_t first, std::uint32_t second, std::uint32_t third);
// 0x102dec50: appends to list D {the first word of the player record of person number `person`, second, third}. Leaves
// r4 (that word), r5 (the people table's base) and r7 (the capacity, before the growth) as the original does.
void append_player_pool(Cpu&, std::uint32_t regen, std::uint32_t person, std::uint32_t second, std::uint32_t third);
// 0x102dee70: appends a copy of the retirement entry `entry` points at to list A: the words +0 and +4, the halfwords at +8, +0xa,
// +0x10 and +0x12 and the bytes +0x18 and +0x19 (the rest of the 0x1c bytes is not copied). Leaves r4 (the byte +0x19) and r5
// (the capacity, before the growth) as the original does.
void append_retirement(Cpu&, std::uint32_t regen, std::uint32_t entry);
// 0x102df020: removes from list A the entry bsearch finds for the key entry (comparator 0x102eb550): later entries move down
// one (the words +0 and +4, the halfwords +8, +0xa, +0x10, +0x12 and the bytes +0xc, +0x14, +0x18, +0x19), and the vacated
// last entry gets id -1, byte +0x18 = 0 and the four halfwords zero (the byte +0x19 stays), count - 1. Leaves r4 (0), r5
// (-1) and r6 (the entry index it reached) when it removed one.
void remove_retirement(Cpu&, std::uint32_t regen, std::uint32_t key_entry);
// 0x102eb670: whether a list A entry has the same id as the word `key` points at (0 for a null key or no entries); if so it
// removes that entry (0x102df020) and returns 1.
bool remove_retirement_by_id(Cpu&, std::uint32_t regen, std::uint32_t key);
// 0x102df1a0: registers a regenerated person (`record` is {id word, halfword +4, halfword +6}): an id outside 0..the people
// count (the test accepts the count itself) reports the assertion (line 0xa9d) and returns. The record joins list B; a person
// with a staff record (word +0x1c) joins list C as {the staff record's first word, the nation's word +0x68 or, with no
// nation, a draw below the nation count, 0 (not written), the person's id}, one with a player record (+0x14) list D as
// {the player record's first word, the nation's word +0x68, 0 (not written), the person's id}. Leaves r4..r8 as the original does.
void register_regen(Cpu&, std::uint32_t regen, std::uint32_t record);
// 0x102df6d0: removes the first entry of list B (the later ones move down: the word +0, halfwords +6 and +4, the byte +8); the
// vacated last entry gets id -1 and zero halfwords at +4 and +6. Leaves r4..r7 as the original does.
void pop_queue_entry(Cpu&, std::uint32_t regen);

// Block `club_placement` (player_regen_clubs.cpp): the club choice of a regenerated player, its scores, the per-nation lists and the
// signing. The scores are shorts (the original's registers hold them untruncated until it compares or returns them).
// 0x102ea670: the simple score of `club` for `person`: 5000 less the distances of the player record's halfwords +0xa, +6 x 50 and
// +4 x 50 from the club's reputation (+0x1ec), plus the reputation, then a draw below that (0 for a score of 0, none drawn).
std::int32_t club_rating(Cpu&, std::uint32_t person, std::uint32_t club);
// 0x102e9b90: the full score (the simple one's terms, a human manager's and the youth setup's bonuses by the club's reputation and
// the byte `level` (r6), the same nation, the special clubs and nations), then a draw below it. Draws: a mod-20 test, a draw below
// the club's kind byte (+0x240) with a second mod-20 test (unless the kind is 0x14), a mod-75 test, and the last.
std::int32_t club_score(Cpu&, std::uint32_t person, std::uint32_t club, std::uint32_t level_register);
// 0x102e99f0: the club (a pointer, 0 for none) of the highest score among the clubs but the last two of each nation that the
// person may join (0x100cab10 gate, club code, an English-exclusion test, 0x103f83c0, fewer than 50 members); `owner` is unused.
std::uint32_t best_destination_club(Cpu&, std::uint32_t owner, std::uint32_t person);
// 0x102ea3c0: the same over the club list of one nation (the person's nation, the home one for two special nations or, with a
// draw mod 5 below 2, a third): a list with no entry returns 0; a missing club (0x10738e00 returns null) reports the assertion
// (line 0x19e9) and returns 0. `lists` is the object whose word +0 holds the per-nation list pointers.
std::uint32_t best_club_of_nation_list(Cpu&, std::uint32_t lists, std::uint32_t person);
// 0x102e98a0: builds the per-nation club lists: one object per nation, and every club with a nation record that takes it joins its
// nation's list (0x10738d10).
void init_nation_club_lists(Cpu&, std::uint32_t lists);
// 0x102dc000: signs `person` to `club`: false when the club's 50 squad slots (+0x98) are full; else the slot, the club link, the
// contract (value, 1 in 4 a favour, a cap of 5000 for a club of level 3), a contract end date and the human club's flags.
bool sign_new_player(Cpu&, std::uint32_t owner, std::uint32_t person, std::uint32_t club);

// Block `detach` (player_regen_detach.cpp): a retiring person leaves his club (link +0x24) and national team (+0x20).
// 0x102e1f20: a player: both links' tactics entries are cleared (0x103afb90) and the person's slot among the record's 50 squad
// slots (+0x98); then the job bytes decide (club side: byte +0x62, national side: +0x60; signed): 1..10 end the routine with
// the links untouched, 12, 13 and 15 set the club byte to 5, 6 and 8 and keep the link of their side, 16 formats a message
// about a player already retired (0x1077b720), 11, 14 and values outside 0..16 set the byte to 0x10; the links not kept are
// cleared. `owner` is unused.
void detach_player(Cpu&, std::uint32_t owner, std::uint32_t person);
// 0x102e21c0: a staff member: for each side with a link, the case of its job byte (0..15; others only clear the link) releases
// the person from the record (the manager slot with the manager manager's calls, the training, scouting and transfer
// managers', the slot arrays at +0x84, +0x160, +0x174, +0x190, the words +0x80 and +0x94), clears the link, and both job bytes
// are set to 0 at the end.
void detach_staff(Cpu&, std::uint32_t owner, std::uint32_t person);
}
