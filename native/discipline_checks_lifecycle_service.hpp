#pragma once
// The lifecycle block's scripted services (discipline_checks.hpp Case::service): a file layer, the folder lookup,
// the history file object and the allocators, answered on both sides identically. Everything the scripts count
// or log lives in guest memory (the scratch area), which the comparison covers and which each side starts from
// the same state: the bytes each write receives (as a checksum), the arguments of each call, the order of calls.
#include "discipline_checks.hpp"
#include <cstdint>

namespace cm::aot::discipline_checks::lifecycle {
// Which transfer of the manager's file a script makes fail (a short count, or none for a count word).
enum class Role { none, table, record_count, records, index_count, index, pending_count, item, body };
inline constexpr unsigned every_object = 0xffffffffu;
struct Script {
    std::uint32_t manager = 0, people = 0;
    unsigned spec_failures = 0;       // FSMakeFSSpec answers "not found" this many times first
    bool open_fails = false;          // the file layer's open gives no handle
    unsigned fail_object = 0;         // the n-th object allocation made by the module (1-based) fails; every_object: all
    std::uint32_t fail_size = 0;      // a raw allocation of this size, made by the module, fails
    std::int16_t records = 3, index = 2, pending = 3;  // the counts the file holds (a load) or the manager holds
    Role fail = Role::none;           // the transfer that comes up short or fails
    unsigned fail_item = 0;           // for Role::item: which one (0-based)
    unsigned fail_body = 0;           // for Role::body: which body transfer (1-based) comes up short
};
// Installs the folder lookup's fixture on the case list's state (once per list); a case turns it on with memory().
void install(const Scenario& scenario);
// The writes that turn on and set the folder lookup for a case (memory is the case's, undone afterwards).
std::vector<Write> memory(const Script& script);
// The service for one script (the case's lambda).
std::function<bool(guest::Cpu&, std::uint32_t)> service(Script script);
// Where the log of calls starts and how many records it holds, and addresses a case may use.
inline constexpr std::uint32_t control = scratch_base + 0x80010, log_base = scratch_base + 0x80100,
                               name_log = scratch_base + 0x89000, file_argument = scratch_base + 0x40000,
                               new_manager = scratch_base + 0x20000, handle_value = scratch_base + 0x30000;
}
