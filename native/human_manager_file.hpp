#pragma once
// HUMAN_MANAGER.CPP block "file": the humans' records read from and written to a saved game's file
// (human_manager_file.cpp). `humans` is the human-manager table ({records, count, two more words});
// `file` the saved game's file specification (null: the routine reports and fails). Both routines
// open the file named "human_manager.dat" through ZIPDIR.CPP (0x107475b0), move the table's words
// and each record's fields in the order the file holds them, and close it. A failed open, allocation
// or transfer reports the assertion, clears the shared success flag and returns 0; 1 is success.
#include "guest_abi.hpp"

namespace cm::aot::human_manager {
using guest::Cpu;
// Reads the file into the table: the count, the allocated record array (built by the record
// constructor), each record's tactic slots, fields, list and sorted entries, then the table's two
// trailing words and the last three tactic slots of every record. The saved game's version (the word in the
// cell at TOC -0x7b74, through its absolute value 0x10775550) selects the older layouts: below 0x3e021 the tactic slots
// come as 0x1e5c-byte images, below 0x3e005 the list as sixteen words, below 0x3e001 the staff rows'
// byte +0x65 is set to 0x14. The records' links (competition, clubs, rating table) are rebuilt after.
std::uint32_t read_humans(Cpu&, std::uint32_t humans, std::uint32_t file);   // 0x1014a300
// Writes the table and its records in the order read_humans reads them.
std::uint32_t write_humans(Cpu&, std::uint32_t humans, std::uint32_t file);  // 0x1014b0a0
}
