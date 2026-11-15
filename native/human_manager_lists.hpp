#pragma once
// HUMAN_MANAGER.CPP block "lists": each human's sorted list of 0x20-byte entries (record::entries,
// record::entry_count; the key is the entry's first word, the text starts at +4). `humans` is the
// human-manager table, `key` the address of a word holding the key, `staff` a human's staff record
// or null for the human whose turn it is. A failed check reports (the game's assertion dialog).
#include "guest_abi.hpp"

namespace cm::aot::human_manager {
// Removes the key from every human's list, re-sorting the list of the last record visited.
void remove_key_everywhere(guest::Cpu&, std::uint32_t humans, std::uint32_t key);                 // 0x1014b690
// Adds an entry (key, text of at most 26 characters) to the human's list.
void add_entry(guest::Cpu&, std::uint32_t humans, std::uint32_t key, std::uint32_t text, std::uint32_t staff);  // 0x1014b810
void remove_entry(guest::Cpu&, std::uint32_t humans, std::uint32_t key, std::uint32_t staff);      // 0x1014bb50
// The address of the entry's text, or 0.
std::uint32_t entry_text(guest::Cpu&, std::uint32_t humans, std::uint32_t key, std::uint32_t staff);  // 0x1014bdc0
std::int8_t has_entry(guest::Cpu&, std::uint32_t humans, std::uint32_t key, std::uint32_t staff);  // 0x1014bf80
// Overwrites the text of an existing entry.
void set_entry_text(guest::Cpu&, std::uint32_t humans, std::uint32_t key, std::uint32_t text, std::uint32_t staff);  // 0x1014c0e0
// The comparator the list's sort and search use (0x1014d520): first words of two entries.
std::uint32_t compare_entries(guest::Cpu&, std::uint32_t first, std::uint32_t second);
}
