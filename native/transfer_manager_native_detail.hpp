#pragma once
// TRANSFER_MANAGER.CPP routines written native-first: the calls they make and the records they share.
#include "guest_abi.hpp"
#include "transfer_manager_native_adapter.hpp"
#include <initializer_list>

namespace cm::aot::transfer_manager_native {
// Words of the table of contents (Cpu::toc).
namespace toc {
inline constexpr std::int32_t manager = -0x7b40,   // the cell holding the transfer manager object
                              contracts = -0x7b7c, // the cell holding the contract manager (0x10082f20's first argument)
                              found_contract = -0x7b78, // a cell where callers store what 0x10082f20 found
                              news = -0x7bf0,      // the cell holding the news manager (0x102cc140's first argument)
                              people = -0x7be0,    // the cell holding the person table (0x70 each)
                              clubs = -0x7be8;     // the cell holding the club table (0x248 each)
}
inline constexpr std::uint32_t person_size = 0x70, club_size = 0x248;

// A call out of this family (another module, or a TRANSFER_MANAGER.CPP routine of the earlier
// blocks or still generated): sets r3... to `arguments`, calls at the original return address
// `back`, returns r3.
inline std::uint32_t invoke(guest::Cpu& c, std::uint32_t entry, std::uint32_t back,
                            std::initializer_list<std::uint32_t> arguments = {}) {
    unsigned n = 3;
    for (const auto argument : arguments) c.set(n++, argument);
    c.call(entry, back);
    return c.r(3);
}
// The same for a native-first routine of this family in another file: runs its C++ in place
// (through the hook in a checker, so nested calls are checked).
inline std::uint32_t invoke_native(guest::Cpu& c, std::uint32_t entry, std::uint32_t back,
                                   std::initializer_list<std::uint32_t> arguments = {}) {
    unsigned n = 3;
    for (const auto argument : arguments) c.set(n++, argument);
    call(c, entry, back);
    return c.r(3);
}
// A news item being built in the caller's frame (NEWS.CPP): `NewsItem item(c, frame.at(offset), kind,
// back)` constructs it (0x102bdf90), `item.field(slot, value, back)` fills a numbered field
// (0x102cb4b0) and `item.post(club, back)` hands it to the news manager (0x102cc140). Each `back`
// is the original return address of that call. The constructor's third argument is 0 at every
// site seen so far; pass another value where the listing does.
struct NewsItem {
    guest::Cpu& c;
    std::uint32_t at;
    NewsItem(guest::Cpu& c, std::uint32_t at, std::uint32_t kind, std::uint32_t back, std::uint32_t third = 0) : c(c), at(at) {
        invoke(c, 0x102bdf90, back, {at, kind, third});
    }
    void field(std::uint32_t slot, std::uint32_t value, std::uint32_t back) const { invoke(c, 0x102cb4b0, back, {at, slot, value}); }
    void post(std::uint32_t club, std::uint32_t back) const { invoke(c, 0x102cc140, back, {c.u32(c.toc(toc::news)), at, club}); }
};
// The original's value of a non-volatile register from the point where it sets it: callees save
// r13-r31 and f14-f31 in their frames, and later code reads those frames as stale stack, so every
// call this family makes holds the original's values there (the checker compares them at each call).
inline void keep(guest::Cpu& c, unsigned reg, std::uint32_t value) { c.set(reg, value); }
inline void keep_signed(guest::Cpu& c, unsigned reg, std::int32_t value) { c.set(reg, std::uint32_t(value)); }
}
