#pragma once
// DATABASE.CPP's text block (tools/database_audit.py): names, job titles, languages,
// positions and attribute names as text. One typed function per original routine;
// like database.hpp's, each also leaves what the original leaves where the game can
// see it (r3-r10, cr0, XER[CA], CTR, the stack words it stores).
#include "guest_abi.hpp"

namespace cm::aot::database {
using guest::Cpu;

// Writes a person's name into `buffer` in one of the layouts 1-7 (the layout number is
// the low byte of `mode`; 0 and above 7 report an assertion). The record keeps up to
// three strings (+0x30, +0x28, +0x2c: inferred common name, first name, surname; the
// common name comes from the object the TOC word -0x7a54 points at when it has a
// lookup for the record). Answers whether the layout was valid.
bool person_name(Cpu&, std::uint32_t person, std::uint32_t buffer, std::uint32_t limit, std::uint32_t mode);  // 0x100caeb0

// Comparators over two pointers to records (qsort style). Each builds the two keys
// in stack buffers and asks 0x107465f0 to compare them.
std::int32_t compare_by_surname_format(Cpu&, std::uint32_t a, std::uint32_t b);   // 0x100cd2a0 (layout 4)
std::int32_t compare_by_name_format(Cpu&, std::uint32_t a, std::uint32_t b);      // 0x100cd6b0 (layout 1)
std::int32_t compare_by_text_221(Cpu&, std::uint32_t a, std::uint32_t b);         // 0x100cd410
std::int32_t compare_by_text_e1(Cpu&, std::uint32_t a, std::uint32_t b);          // 0x100cd4a0
std::int32_t compare_by_date(Cpu&, std::uint32_t a, std::uint32_t b);             // 0x100cd530
std::int32_t compare_by_formatted_id(Cpu&, std::uint32_t a, std::uint32_t b);     // 0x100d5ce0

// Whether a file name is plain: a name of two or more characters without a dot
// (one character, or a dot anywhere, answers false; null and empty answer true).
bool plain_file_name(Cpu&, std::uint32_t name);                                   // 0x100c4b00

// The title of a staff role, and a position's name, copied to shared text buffers;
// both can store a kind (1 or 4) through an output pointer.
std::uint32_t job_title(Cpu&, std::int8_t role, std::uint32_t out);                       // 0x100c6af0
std::uint32_t position_name(Cpu&, std::int8_t index, std::uint32_t long_form, std::uint32_t plural,
                            std::uint32_t out);                                        // 0x100c6bf0

// The same layouts over three strings given directly (first name, surname, common name).
bool name_from_parts(Cpu&, std::uint32_t first_name, std::uint32_t surname, std::uint32_t common,
                     std::uint32_t buffer, std::uint32_t limit, std::uint32_t mode);       // 0x100cb750
// The name of the person whose text records are the given table indices (-1: none).
bool name_from_indices(Cpu&, std::uint32_t first_index, std::uint32_t surname_index, std::uint32_t common_index,
                       std::uint32_t buffer, std::uint32_t limit, std::uint32_t mode,
                       std::uint32_t lookup_index);                                        // 0x100cb590

// Attribute names (the long form carries its comment key) and language names, as text in a buffer.
bool attribute_text_12(Cpu&, std::int8_t value, std::uint32_t buffer, std::uint32_t limit, std::uint32_t flag);   // 0x100d2720
bool attribute_text_31(Cpu&, std::int8_t value, std::uint32_t buffer, std::uint32_t limit, std::uint32_t flag);   // 0x100d15f0
std::uint32_t language_name(Cpu&, std::int8_t index);                                                               // 0x100ca140

// A record's name in layouts 1-6 (first name at +4, surname at +8).
bool record_name(Cpu&, std::uint32_t record, std::uint32_t buffer, std::uint32_t limit, std::uint32_t mode);   // 0x100cbf60

// The label of a position mask: texts chosen by its bits, and a code from the kind byte.
std::uint32_t position_mask_text(Cpu&, std::uint32_t mask, std::uint32_t buffer, std::uint32_t limit);   // 0x100d6300

// The label of a role's flags, through the formatter into a buffer.
std::uint32_t role_label(Cpu&, std::uint32_t index, std::uint32_t roles, std::uint32_t flags, std::uint32_t buffer,
                         std::uint32_t limit, std::uint32_t kind_default);   // 0x100d6af0

// The label of a flag word (bit sets name roles), through the formatter into a buffer.
std::uint32_t flags_label(Cpu&, std::uint32_t flags, std::uint32_t buffer, std::uint32_t variant);   // 0x100db410

// A record's name in the layout of selector 1-4 (inferred: a club's or team's name with its
// grammar byte), through the formatter into a buffer.
bool club_name_text(Cpu&, std::uint32_t record, std::uint32_t long_form, std::uint32_t buffer, std::uint32_t limit,
                    std::uint32_t selector, std::uint32_t second_flag);   // 0x100d0940

// A player's strong positions as a comma-separated phrase (see database_text_summary.cpp).
bool player_summary_text(Cpu&, std::uint32_t record, std::uint32_t buffer, std::uint32_t limit, std::uint32_t mode);   // 0x100cc4a0

// A record's rating scaled by its team's table entry and adjusted for a contract (see database_text_scores.cpp).
std::int32_t scaled_rating(Cpu&, std::uint32_t record, std::uint32_t other);   // 0x100d5de0
}
