#pragma once
// DATABASE.CPP's load block (tools/database_audit.py): typed functions, one per
// original routine, as database.hpp describes.
#include "guest_abi.hpp"

namespace cm::aot::database {
using guest::Cpu;

// 0x100a55a0: "Loading database". Opens the index file `name` ("index.dat", then the packed ".dam" tables) and loads
// every table the index lists into the TOC's table words, then fixes up the people (inferred names throughout).
// `single_block` (a byte, TOC -0x39d0 keeps it): the tables are sized first and carved out of one allocation,
// with default records written below the loaded ones; zero allocates every table on its own. `packed` (a signed
// byte): the file calls are the packed reads rather than plain stdio. Answers 1 when everything loaded, 0 at the
// first file or read failure (closing the file); an allocation that fails reports through the assert macro and
// goes on.
bool load_database(Cpu&, std::uint32_t name, std::uint32_t single_block, std::uint32_t packed);    // 0x100a55a0
}
