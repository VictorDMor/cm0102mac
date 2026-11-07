#pragma once
// DATABASE.CPP's loading block (tools/database_audit.py): typed functions, one per
// original routine, as database.hpp describes. The block holds the table-release and
// table-growth wrappers around the allocator, the language object (the game's text
// per language, loaded from a language file) with its seven accessors, and the
// release of every loaded table.
//
// Names are inferred from what the routines do; the language file's tables are named
// by record size and slot because no caller shows more.
#include "guest_abi.hpp"

namespace cm::aot::database {
using guest::Cpu;

// Frees the block a table's base word (TOC -0x74c4, then -0x74cc) points at, when it
// holds one, and zeroes the base word and its count word (-0x74c8, then -0x74d0). The
// free is the same call on both sides of the byte at TOC -0x39d0, with its own return
// address on each.
void release_table_74c4(Cpu&);                                         // 0x100a4c00
void release_table_74cc(Cpu&);                                         // 0x100a4c70

// Grows the table of 0x1c-byte records (base word TOC -0x7a34, count word -0x7a38) by
// `count` records through the allocator's resize; every new record gets its own index
// as its first word and zero elsewhere, the last-added record first. False when the
// resize fails, with the table unchanged.
bool grow_table_7a34(Cpu&, std::int32_t count);                        // 0x100a4ce0

// Frees the eight arrays of a language object (+0x20..+0x3c) and, when `delete_object`
// is positive as a signed halfword, the object itself. Returns the object.
std::uint32_t release_language_object(Cpu&, std::uint32_t object, std::int32_t delete_object);  // 0x100d4880

// Reads the language file ("eng.lng" in the data folder) into a language object whose
// +0x20..+0x3c words are zero: the 32-byte header (byte-swapped when its signature
// differs), a check of its counts against the loaded tables' counts, then the eight
// arrays. Reports an assertion and returns false when the file does not open or the
// header does not match.
bool load_language_object(Cpu&, std::uint32_t object);                 // 0x100d4970

// The language-object accessors. While the language byte (TOC -0x7ab0) is 0 or 1 the
// text lives inside the record itself at a fixed offset; otherwise the shared object
// (TOC -0x39e4) is created on first use and the text comes from its arrays.
std::uint32_t language_text_6d_first(Cpu&, std::uint32_t record);      // 0x100d4df0 (inline +0xae)
std::uint32_t language_text_6d_second(Cpu&, std::uint32_t record);     // 0x100d4ee0 (inline +0xa4)
std::uint32_t language_text_53(Cpu&, std::uint32_t index_word);        // 0x100d4fd0 (inline +0x1a)
std::uint32_t language_text_4f_3c(Cpu&, std::uint32_t index_word);     // 0x100d50e0 (inline +0x30)
std::uint32_t language_text_4f_20_mapped(Cpu&, std::uint32_t index_word);  // 0x100d51d0 (inline +0x221)
std::uint32_t language_text_4f_20(Cpu&, std::uint32_t index_word);     // 0x100d5310 (inline +0x1ee)
std::uint32_t language_text_34(Cpu&, std::uint32_t index_word);        // 0x100d5450 (inline +0x18)

// A small number (-1 and 0..4) for a 0..0x23 index and one of the kinds 3..10 (inferred),
// from tables of the original's switch; -1 for any other kind or an index out of range.
std::int32_t language_class(Cpu&, std::int32_t index, std::int32_t kind);  // 0x100d5540

// Whether the language file of a language number 0..10 exists in the data folder (UTILS file_exists,
// 0x10739d60, on the folder path with the language's file name; 0 and 1 share eng.lng); 0, with no call, for any
// other number.
std::uint32_t language_file_exists(Cpu&, std::int32_t language);         // 0x100b8b80

// Frees and clears every loaded table of the database, then the language object, sets the language byte to -1
// and deletes the temporary files (UTILS delete_temporary_files, 0x10747260, on the pattern at pool +0xc1).
// (0x107464d0, which the language loaders call, is UTILS read_file_marker.)
void release_all_tables(Cpu&);                                         // 0x100a4e50

// Loads the language `language` (0..10; English with a second argument of zero discards the shared language
// object, any other language loads English first with 1) into the game's tables from its file in the data folder:
// the header (byte-swapped when needed, checked like load_language_object's), then the eight arrays, each
// merged into its table (club names, ..., 0x4f/0xba/0x53/0x6d/0x34-byte records), in bulk when the allocation
// answers and record by record through a stack buffer when it does not. Nothing happens when no club table is
// loaded or the language is the current one.
void load_language_file(Cpu&, std::int32_t language, std::int32_t second);  // 0x100b6050
}
