// LEAGUE.CPP: the league object's lifetime, the virtuals a derived league must override, and the small
// queries on its fields. The one-instruction virtuals have no body here: their lines in
// league_base_routines.inc return the constant.
#include "date.hpp"
#include "league_base_detail.hpp"

namespace cm::aot::league_base {

std::uint32_t construct(Cpu& c, std::uint32_t object) {
    // The original is unoptimized code: it keeps the object in its parameter slot, entry sp+0x18 in the
    // caller's frame (`stw r3,0x58(r1)` after its 0x40-byte frame), and reloads it before every store.
    const Frame frame(c, 0x1049e4f0, 0x40, {});
    c.put32(frame.at(0x58), object);
    c.link(0x1049e508);
    comp::construct(c, object);
    c.put32(object + competition::vtable, c.toc(toc::vtable));
    c.link(0x1049e520);
    date::set_default(c, object + league::date_cc);
    c.link(0x1049e52c);
    date::set_default(c, object + league::date_d4);
    c.put16(object + league::half_a8, 0);
    c.put32(object + league::word_ac, 0);
    c.put8(object + competition::kind, league::kind);
    c.put32(object + competition::stages, 0);
    c.put8(object + league::byte_b4, 0);
    c.put32(object + league::standings, 0);
    c.put16(object + league::half_de, 0);
    c.put16(object + league::flags, 3);
    c.put8(object + league::byte_c6, 2);
    c.put8(object + league::byte_e1, 2);
    c.put8(object + league::byte_e6, 0);
    c.put32(object + league::preset_fixtures, 0);
    c.put16(object + league::half_b6, 0xffff);
    c.put16(object + league::half_b8, 0);
    c.put16(object + league::half_a4, 0xffff);
    c.put8(object + league::byte_e0, 0);
    c.put16(object + league::half_e2, 0xffff);
    c.put16(object + league::half_e4, 0xffff);
    // The constants the original loads at 0x1049e530..0x1049e548 are still in r4-r8 at its memset call, and
    // most of the 116 generated derived constructors read r6-r10 after this routine returns
    // (tools/register_contract.py --after 0x1049e4f0): what memset leaves of them reaches those callers.
    c.set({{4, 0}, {5, 4}, {6, 3}, {7, 2}, {8, 0}});
    invoke(c, callee::memset, 0x1049e5e0, {object + league::tie_breaks});
    c.put32(object + league::word_a0, 0);
    return object;
}

std::uint32_t destroy(Cpu& c, std::uint32_t object, std::uint32_t deleting) {
    const Frame frame(c, 0x1049e600, 0x40, {.first_gpr = 30});
    if (!object) return object;
    // The original keeps the object in r30 and the flag word in r31 across its calls; the allocator's calls can
    // reach imports, where a replay compares the non-volatile registers.
    c.set({{30, object}, {31, deleting}});
    c.put32(object + competition::vtable, c.toc(toc::vtable));
    // The preset fixtures are a new[] array with a 16-byte header before its first row. The field is left as it is.
    if (const auto fixtures = c.u32(object + league::preset_fixtures))
        invoke(c, callee::release_array, 0x1049e640, {fixtures - league::array_header});
    // The base class's destructor, never deleting (r4 = 0, as the original passes it).
    c.set(4, 0);
    c.link(0x1049e64c);
    comp::destroy(c, object, 0);
    if (std::int16_t(deleting) > 0) invoke(c, callee::free_object, 0x1049e65c, {object});
    return object;
}

namespace {
// The three "not overridden" virtuals share one shape: a 0x70-byte frame saving r30/r31, which hold the message
// buffer and the pool at the report's calls, and the report's buffers at sp+0x40 and sp+0x38.
void unsupported(Cpu& c, std::uint32_t entry, std::uint32_t at, std::uint32_t line) {
    const Frame frame(c, entry, 0x70, {.first_gpr = 30});
    c.set({{30, c.toc(toc::message)}, {31, c.toc(toc::pool)}});
    report(c, frame, {at, 0x40, 0x38, line});
}
}
std::uint32_t unsupported_44(Cpu& c) { unsupported(c, 0x1049e970, 0x1049e9a0, 0xb8); return 0; }
std::uint32_t unsupported_94(Cpu& c) { unsupported(c, 0x1049ea10, 0x1049ea40, 0xcf); return 0xffffffffu; }
std::uint32_t unsupported_10(Cpu& c) { unsupported(c, 0x1049eaa0, 0x1049ead0, 0xe1); return c.toc(toc::success); }

std::uint32_t standing(Cpu& c, std::uint32_t object, std::uint32_t index) {
    return c.u32(object + league::standings) + index * league::standing_row;
}
bool promotes(Cpu& c, std::uint32_t object) {
    return c.i8(object + league::automatic_promotion) != 0 && (c.u16(object + league::flags) & 2) != 0;
}
bool relegates(Cpu& c, std::uint32_t object) { return c.i8(object + league::relegation) != 0; }

}
