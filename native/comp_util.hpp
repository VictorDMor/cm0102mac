#pragma once
// COMP_UTIL.CPP (tools/comp_util_audit.py): competition utilities shared by the national leagues, the cups and
// the screens: ids and round codes, club and fixture queries, the result texts. comp_util_detail.hpp names the
// records the routines read. Each block of the module declares its routines in its own header section below;
// a routine takes the `Cpu` where the original leaves registers a caller can read (comp_util_routines*.inc
// lines call it with the guest ABI's arguments).
//
// Dependencies still generated, called through the dispatcher (c.call): COMP_STATS.CPP 0x10459b50 (a club's row
// in a competition's statistics), the boundary routine 0x104abf40 (whether the statistics owner marks a
// competition record), and the league's virtual +0xa0 through the pointer glue 0x10781320.
#include "guest_abi.hpp"

namespace cm::aot::comp_util {
using guest::Cpu;

// Block `numeric_ids` (comp_util_ids.cpp): pure functions of a number.
// 0x10464010: the id of the series (the parent competition) a competition id belongs to, from fixed id ranges;
// -1 when it belongs to none. Leaves no register but r3.
std::int32_t parent_series(std::int32_t competition);
// 0x10464880: the last decimal digit of a value below 1000 (the remainder of the signed division by ten, as a
// signed byte), 0 from 1000 on. Leaves r4 as the original does (the sign of the quotient, below 1000).
std::int8_t last_digit(Cpu&, std::int32_t value);
// 0x104648c0: a value below 1000 without its last digit, a halfword; 1000 and above unchanged. Leaves r4 (below
// 1000) and r5 (the value, sign-extended) as the original does.
std::int16_t round_down_to_ten(Cpu&, std::int16_t value);

// Block `club_queries` (comp_util_clubs.cpp, comp_util_race.cpp).
// 0x104646d0: whether competition `competition` is one of the three competitions the words at TOC -0x6888,
// -0x67bc and -0x6734 name, or a league (kind 2) with clubs whose first standing row's club belongs to the
// league's own record (club +0x30 or +0x38). Leaves r4 (the last table row read) and r5 (the standing row, when
// reached) as the original does.
bool competition_is_active(Cpu&, std::int32_t competition);
// 0x10464900: the class (1..7) of a fixture for `club`, from the two clubs' reputations (+0x1ec, in steps of
// 500) and the fixture's goal bytes; which of the two sides `club` is selects the table. Leaves r4..r9 as the
// original does.
std::uint32_t fixture_class(Cpu&, std::uint32_t club, std::uint32_t fixture);
// 0x10464160: whether two clubs of one league (kind 2, without a parent) are contenders in the same race:
// 0, 1 or 3, from the two clubs' COMP_STATS rows (0x10459b50), the difference of their bytes +0x76 against a
// limit from the league's club count, a weighted average of the rows' bytes +0x75..+0x78 per club and the
// league's virtual +0xa0 (inferred: promotion places). Leaves the frame's locals as the original does
// (readers in STAFF_CONTRACTS.CPP, SHORTLIST_MANAGER.CPP, SPAIN_RULES.CPP and a boundary routine read them);
// r4-r10 are not left as the original does (no caller reads them: tools/register_contract.py --after).
std::uint32_t contenders_class(Cpu&, std::uint32_t first_club, std::uint32_t second_club);

// Block `comparators` (comp_util_compare.cpp): the unit's qsort comparators. The sort (QSORT4X.CPP 0x10768300)
// calls each through its transition vector with two pointers to the elements and reads r3 (the three-way
// result) and nothing else; the comparators still leave r4-r7 (and f0, f1) as the original does. A "slot" is
// the address of an array element holding a pointer to the record.
// 0x10463580: records (not slots) with a double at +4, the larger first; equal and unordered compare equal.
// Sets FPSCR as the original's fcmpo does.
std::int32_t compare_score_descending(Cpu&, std::uint32_t first, std::uint32_t second);
// 0x104647a0: slots of clubs, by reputation (+0x1ec) descending. Leaves r4 (the first reputation), r5 (the first club).
std::int32_t compare_club_reputation(Cpu&, std::uint32_t first_slot, std::uint32_t second_slot);
// 0x104647c0: records (not slots), by the signed byte at +4 ascending. Leaves r4 (the second byte, unsigned).
std::int32_t compare_tag_ascending(Cpu&, std::uint32_t first, std::uint32_t second);
// 0x104647e0: slots of clubs, empty slots last, by reputation descending. Leaves r4 (the second club).
std::int32_t compare_club_reputation_nulls_last(Cpu&, std::uint32_t first_slot, std::uint32_t second_slot);
// 0x10464830: slots of clubs, empty slots last, by reputation ASCENDING (the twin of 0x104647e0 with the
// difference the other way round). Leaves r5 (the first club).
std::int32_t compare_club_reputation_ascending(Cpu&, std::uint32_t first_slot, std::uint32_t second_slot);
// 0x10477cd0: slots of clubs, clubs without a division record (+0x34) last, then by that record's reputation
// descending, its id (word +0) descending and the club's byte +0x23c ascending. Leaves r4..r7 as the original.
std::int32_t compare_club_division(Cpu&, std::uint32_t first_slot, std::uint32_t second_slot);
// 0x104777c0: 0x50-byte rows of the COMP_SCREENS list (not slots) by their group byte (+0x42), halfwords +0x32,
// +0x34, +0x30 (each ascending) and the club names (UTILS 0x107465f0, case-sensitive: its r5 is 1). Frames its call.
std::int32_t compare_screen_rows(Cpu&, std::uint32_t first, std::uint32_t second);

// Block `interpolation` (comp_util_interpolation.cpp).
// 0x10476780: `index * (to - from) / (count - 1)` as a signed byte (the mean of the two ends for a count of 1);
// outside 0..100 the game's assertion (line 0x256e) is reported and the value clamped. Leaves r4, r6 and r7
// as the original does (r7 the count; r4 the index and r6 the first end when the count is not 1, r4 the clamp
// after an assertion).
std::int8_t interpolated_share(Cpu&, std::int8_t from, std::int8_t to, std::int16_t index, std::int16_t count);

// Block `rating_sort` (comp_util_rating.cpp).
// 0x10477870: a club's rating for seeding: -10,000,000 for no club; else the finance balance / 300 (the finance
// manager's account, TOC -0x7ab8) or, without a manager, the club's word +4 / 400; plus 16 x its reputation;
// plus the worth of the fifteen places the computer fills from the club's members with player data
// (0x10210c50, each pick rated by 0x10216830).
std::int32_t club_strength(Cpu&, std::uint32_t club);
// 0x10477b40: sorts `count` club pointers by club_strength, best first (a selection sort on a copy of the ratings,
// swapping while the earlier is lower; empty slots rate 0). A failed allocation reports the game's assertion
// (line 0x292b) and leaves the array. Leaves r4-r10 as the original's loops end (a caller's virtual call
// reads r8-r10); no result.
void sort_by_strength(Cpu&, std::uint32_t clubs, std::int32_t count);

// Block `comp_screens_list` (comp_util_screens.cpp).
// 0x10477490: orders the 0x50-byte rows of a COMP_SCREENS list (a NETWORK utility object: count +0, rows from +4,
// cursor +0xc): copies them out, sorts the copy with compare_screen_rows, empties the list and adds the rows back.
// Answers true; false after reporting the game's assertion for no list (line 0x2781), no rows (0x278c) or a failed
// allocation (0x2794), leaving the list as it was.
bool sort_screen_list(Cpu&, std::uint32_t list);
// Block `record_stream` (comp_util_stream.cpp): a fixture reference (the game's 0x50-byte match row) to and from a
// news item. `cursor` is the address of a byte holding the item's next word index; both routines advance it by
// 18, the number of words (a byte: it wraps at 256). The item's set-word and word routines report the game's
// assertion for an index outside 0..49 (as a signed byte): the writer then skips that word, the reader stores 0.
// 0x10463be0 (reference, item, cursor): writes eighteen of the row's fields as words.
void write_fixture_reference(Cpu&, std::uint32_t reference, std::uint32_t item, std::uint32_t cursor);
// 0x10463dc0 (item, reference, cursor): reads them back and rebuilds the club (+0x1c, +0x20), row (+0x18) and
// competition record (+0x14) pointers from the ids (0 for -1).
void read_fixture_reference(Cpu&, std::uint32_t item, std::uint32_t reference, std::uint32_t cursor);
// Block `fixture_log` (comp_util_log.cpp): the history log, a global table of 0x1c-byte rows (TOC cells -0x7a34
// rows, -0x7a38 count): the competition's record (+4), the winner, runner-up, third and host clubs (+8..+0x14)
// and the year (+0x18, a halfword).
// 0x10463990: appends a row for `competition` (an object; its record is stored), reusing the first row of the
// table's trailing run of empty rows (+4 of 0), else growing the table by 20 rows (DATABASE 0x100a4ce0; no row
// is written when it fails). The year is the game date's. Leaves r4 (the scan's row index, or the table after a
// growth), r5 (the row last tested) and r6 (the slot, 0 for none) as the original does; r7..r10 are untouched.
void append_history(Cpu&, std::uint32_t competition, std::uint32_t winner, std::uint32_t runner_up, std::uint32_t third, std::uint32_t host);
// 0x10463a90: the club that took `place` (0 the winner, 1 the runner-up, 2 the third, a signed byte in the
// register) in the competition with record `record`: sorts the table, searches it for the record (any year) and takes
// the first of the matching rows. 0 when there is none; the game's assertion (line 0x196) and 0 for another place.
// Leaves r4..r10 as the original does (callers read r7..r10).
std::uint32_t history_club(Cpu&, std::uint32_t record, std::uint32_t place_register);
// 0x104770c0: copies the rows in which `club` (or `second_club`, when not 0) placed into a new block, sorts it
// by compare_history_rows with the two clubs in the unit's context words, stores the block's address at `out`
// and answers the row count. A failed allocation reports the game's assertion (line 0x2702) and answers 0
// (`out` 0). Read for r3 only.
std::uint32_t club_history(Cpu&, std::uint32_t club, std::uint32_t second_club, std::uint32_t out);
// 0x10477270: the qsort comparator of those rows (two row addresses): rows whose competition record has no
// continent or nation sort first, then the current competitions (0x104646d0) before the others, then by the
// competition's reputation (higher first), its id, the place the context club took (winner, runner-up, other)
// and the year (older first). Leaves r4 and r5 as the original does.
std::int32_t compare_history_rows(Cpu&, std::uint32_t first, std::uint32_t second);
// Block `random_shuffle` (comp_util_shuffle.cpp): the pooled generator's draws (see the file's header for the
// draw's definition).
// 0x10463000: one draw below `bound` (0 for 0, no draw); the out-of-line copy of the game's Random(n), also for
// bounds of 0x10000 or more in magnitude (a scaled draw plus a second, smaller one). Leaves only r3 (no caller
// reads another register).
std::int32_t pooled_draw(Cpu&, std::int32_t bound);
// 0x10462cd0: shuffles `count` (a signed halfword in the register) words at `array` by 3 * count random swaps,
// each of two draws below count. Counts from 10923 on never end (the original's halfword counter wraps).
// Leaves r3..r5 as the last swap did and r7..r10 as the callee left them (callers read r7..r10).
void shuffle(Cpu&, std::uint32_t array, std::uint32_t count_register);
// 0x104635b0: the game's "pick `take` of `count`": validates 0 <= take <= count (else the game's assertion, line
// 0xf1), sorts the `count` words at `array` by the descriptor at TOC -0x6188 (club reputation, empty last) and
// shuffles min(3 * take, count) swaps over the whole array, each of two draws below that number. Leaves
// r3..r5 as the last swap and r7..r10 as the callees left them.
void shuffle_front(Cpu&, std::uint32_t array, std::uint32_t count_register, std::uint32_t take_register);

// Block `round_codes` and the text of a round (comp_util_rounds.cpp): data drafted by tools/comp_util_tables.py.
// 0x1046f1f0: a round code (a halfword in the register) and a kind byte to a small class (0..7; inferred: the
// grammatical gender of the round's name), under the preferences record's language byte (TOC -0x7ac8, +8):
// only languages 2, 3, 4, 5 and 9 have tables, one set for kind 3 and one for any other kind; every other
// language answers 0. A code outside a table's stretches (or a table's error arm) reports the error text into
// the game's shared buffer and answers 0 (language 2, kind 3, codes from 1156 on answer 0 without it). Leaves r5
// (the code, sign-extended) where a table's language was reached; the registers the report's sprintf leaves.
std::int32_t round_gender(Cpu&, std::uint32_t code_register, std::uint32_t kind_register);
// 0x10476ec0: the gender of a competition's name: of the first round code (`code_register`), else of the second,
// else the competition record's own byte (+0x6a for kind 1, +0x67 otherwise), 1 for no record. Leaves r3 (and
// what round_gender leaves when a code decides).
std::uint32_t competition_gender(Cpu&, std::uint32_t record, std::uint32_t second_code_register, std::uint32_t code_register, std::uint32_t kind_register);
// 0x10469bb0: the localized name of a round code into `buffer` (`size` bytes of it; below 30 the game's assertion,
// line 0x110f). The buffer first holds the empty text; kinds 0, 1 and 3 have tables (any other kind reports the
// assertion at line 0x15e2); each code has one localized format of a pool string, or (the out-of-range codes and
// the table's error arms) the error text reported into the game's shared buffer, leaving the caller's buffer as
// it was. Answers `buffer`.
std::uint32_t round_text(Cpu&, std::uint32_t buffer, std::uint32_t size_register, std::uint32_t code_register, std::uint32_t kind_register);

// Block `round_text` (comp_util_rounds_text.cpp): the sentences built around a round's name. A competition record's
// long name is at +0x1a and the short one at +0x4d, each with a gender byte (+0x6a, +0x67); a fixture row has two
// round codes, halfwords at +0x30 and +0x32 (0 for none). Every routine keeps the original's frame and calls.
// 0x10476f40: the name for one or two round codes into `buffer` (at least 200 bytes; a null buffer or a smaller size
// reports the assertion at line 0x26a0 and answers 0): the first code's name, else the second's (a code is tested as
// a whole register), else 0; with both, the two names in the pair format with each one's gender twice. Answers 1 once
// a name is made.
std::uint32_t round_pair_text(Cpu&, std::uint32_t first_code, std::uint32_t second_code, std::uint32_t buffer, std::uint32_t size, std::uint32_t kind);
// 0x104768a0: 0x104769d0 for the competition a fixture row names (+0x14 its record, the codes +0x32 and +0x30); a null
// fixture, buffer or record or a size below 0x67 reports the assertions at lines 0x2587 and 0x258f and answers 0.
std::uint32_t fixture_name_text(Cpu&, std::uint32_t fixture, std::uint32_t kind, std::uint32_t buffer, std::uint32_t size);
// 0x104769d0: a competition's name with its round(s) into `buffer` (`size` below 0x67: the assertion at line 0x25af, 0).
// `kind` 1 takes the record's long name, 2 and 3 its short one (3 asks for the other kind of round text); a record
// is null for none. The two special competitions (the id at TOC -0x6750) swap to another record when `code_32` is 0x453
// or 0x456. Language 8 (0x10727880) rewrites the names through 0x10728ac0 and the round texts through the byte table
// at TOC -0x618c. The buffer then gets the name and the round texts in one of four formats, or a plain copy; answers 1.
// The locals stay in its frame as the original's (the name at 0x138, the round texts at 0xd4 and 0x70).
std::uint32_t competition_name_text(Cpu&, std::uint32_t record, std::uint32_t code_32, std::uint32_t code_30, std::uint32_t kind, std::uint32_t buffer, std::uint32_t size);
// 0x10469710: a news item's sentence about a fixture's competition and round (kind 2, 4 or -1 in one format, else the
// other) naming the club, then the item's name and text rows (0x1074b830 begins it, 0x1074bab0 appends). No result.
void competition_round_news(Cpu&, std::uint32_t club, std::uint32_t fixture, std::uint32_t kind, std::uint32_t item);
// 0x104698c0: the same for a fixture's result line: kinds 6 and 7, or -2, with a round's text (`with_round`, a signed
// byte) or without; any other kind builds nothing. No result.
void fixture_round_news(Cpu&, std::uint32_t club, std::uint32_t code, std::uint32_t record, std::uint32_t kind, std::uint32_t with_round, std::uint32_t item);

// Block `result_headline` (comp_util_headline.cpp, comp_util_headline_tables.inc, drafted by tools/comp_util_headline.py).
// 0x10464d00: the news item about `club`'s result in a fixture row: the headline sentence (one of 173 localized texts, by
// who goes through and how: the tie's decider, the result's grade 1..7 that 0x10464900 answers, the two scores, a first
// leg, a round's name or the competition's), the two clubs, the score when the statistics know the fixture and, for a
// friendly, the competition's name and the round's text. `side` is any value but 2, 4 and -1 for a club that goes through;
// `round_code` a halfword (above 0 the round's name is asked for, -2 for the aggregate sentences). Answers nothing: r3 is what
// the last callee left. Keeps the original's 0x450-byte frame, the registers at each call and the formatter's stack words.
void result_headline(Cpu&, std::uint32_t club, std::uint32_t fixture, std::uint32_t grade, std::uint32_t side, std::uint32_t round_code, std::uint32_t item);
}
