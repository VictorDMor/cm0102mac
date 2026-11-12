#pragma once
// DISCIPLINE.CPP lifecycle block: the manager's constructor and destructor, the load from and save to the
// career file (discipline.dat), and the creation of the 42 ruling bodies.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::discipline {
// 0x100de410: builds the manager object `manager` (the SETUP.CPP callers allocate it). Asks for the temp folder
// until FSMakeFSSpec finds it (a dialog each time it does not), clears the arrays, creates the pending list and
// the ban history file object, then loads from `file` (0 for a new game) or builds the person table and bodies.
void construct_manager(guest::Cpu& c, std::uint32_t manager, std::uint32_t file);
// 0x100de660: frees the arrays, destroys the bodies, the history file and the list, and the object itself when
// `deleting` (a signed halfword) is positive.
void destroy_manager(guest::Cpu& c, std::uint32_t manager, std::int16_t deleting);
// 0x100de760: loads the manager from the career file `file`; false (0) after an assertion on a short read or
// a failed allocation. A short pending-list read still gives true.
bool load_manager(guest::Cpu& c, std::uint32_t manager, std::uint32_t file);
// 0x100deda0: saves the manager to `file`; every failure reports an assertion and stops (or, for the pending
// counts and items, goes on).
void save_manager(guest::Cpu& c, std::uint32_t manager, std::uint32_t file);
// 0x100e7cd0: creates the 42 ruling bodies (the array at the manager's +0), each country's only when its
// competition is active. `file_argument` is passed to every body's constructor (the address of the file handle
// while loading, else 0).
void create_bodies(guest::Cpu& c, std::uint32_t manager, std::uint32_t file_argument);
}
