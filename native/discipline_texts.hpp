#pragma once
// DISCIPLINE.CPP texts block: the ban lines and status texts the staff screens, the news texts and RULING_BODY.CPP
// print. Each routine formats into a buffer the caller owns (game memory, compared byte for byte) through the
// localized formatter 0x10727890 and the pool of the module.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::discipline {
// 0x100e7490: describes the ruling bodies of a ban as text into `buffer` (`size` bytes at least 0x46, else an
// assertion report and -1). `bodies` points at the four signed body bytes of the ban (-1 unused), `owner` at
// the manager object (its word at +0 is the array of ruling bodies). With all four bodies one text, with three or
// two another (formats of the pool), with fewer the first body describes itself (its virtual method at 0x18 with
// `buffer`, `size` and `argument`). With `refuse_four` set and a fourth body it prints a fixed text and returns -1.
// The result classifies the first body's kind (9, 8, 0, 1, 2, 3, 6, 7 or -1) for the callers' texts.
std::uint32_t describe_ban_bodies(guest::Cpu& c, std::uint32_t owner, std::uint32_t bodies, std::uint32_t buffer,
                                  std::int32_t size, std::int8_t refuse_four, std::uint32_t argument);
// 0x100e7210: the ban record's line for the staff screens into `buffer` (`size` at least 0x32, else an
// assertion report): the length (matches or a date for a ban by days), the bodies (0x100e7490) and, for a ban
// whose date is still ahead of today's, the person's name. `owner` is what 0x100e7490 takes.
void ban_line(guest::Cpu& c, std::uint32_t owner, std::uint32_t ban, std::uint32_t buffer, std::int32_t size);
// 0x100e7780: the text for a ruling-body status record (kind at +0x10, count at +0x12) into `buffer` (`size` at
// least 0xf, else an assertion report). `owner` is not read.
void status_text(guest::Cpu& c, std::uint32_t owner, std::uint32_t status, std::uint32_t buffer, std::int32_t size);
}
