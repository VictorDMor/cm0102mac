// CUP.CPP: the cup object's lifetime, the virtuals a derived cup must override, and the small queries on its
// fields.
#include "cup_base_detail.hpp"

namespace cm::aot::cup_base {

std::uint32_t construct(Cpu& c, std::uint32_t object) {
    // The original keeps the object in r31 across its one call; the base constructor is a frameless C++ leaf
    // (comp_object.cpp) that reaches no import.
    const Frame frame(c, 0x10478f20, 0x40, {.first_gpr = 31});
    c.link(0x10478f38);
    comp::construct(c, object);
    c.put32(object + competition::vtable, c.toc(toc::vtable));
    c.put8(object + competition::kind, cup::kind);
    c.put8(object + cup::current_round, 0);
    c.put8(object + cup::byte_a9, 0);
    // r4 and r5 at the blr hold the kind and the vtable (0x10478f38..0x10478f3c); r6-r8 are what the base
    // constructor left. The derived constructors read r4-r10 after this call.
    c.set({{4, cup::kind}, {5, c.toc(toc::vtable)}});
    return object;
}

std::uint32_t destroy(Cpu& c, std::uint32_t object, std::uint32_t deleting) {
    const Frame frame(c, 0x10478f70, 0x40, {.first_gpr = 30});
    if (!object) return object;
    // The original keeps the object in r30 and the flag word in r31 across its calls; the allocator's calls can
    // reach imports, where a replay compares the non-volatile registers.
    c.set({{30, object}, {31, deleting}});
    c.put32(object + competition::vtable, c.toc(toc::vtable));
    // The base class's destructor, never deleting (r4 = 0, as the original passes it).
    c.set(4, 0);
    c.link(0x10478fa0);
    comp::destroy(c, object, 0);
    if (std::int16_t(deleting) > 0) invoke(c, callee::free_object, 0x10478fb0, {object});
    return object;
}

namespace {
// The two "not overridden" virtuals share one shape: a 0x70-byte frame saving r30/r31, which hold the message
// buffer and the pool at the report's calls, and the report's buffers at sp+0x40 and sp+0x38. After the report
// the original loads the success flag's address into r4 to clear it, and returns with it there.
void unsupported(Cpu& c, std::uint32_t entry, std::uint32_t at, std::uint32_t line) {
    const Frame frame(c, entry, 0x70, {.first_gpr = 30});
    c.set({{30, c.toc(toc::message)}, {31, c.toc(toc::pool)}});
    report(c, frame, {at, 0x40, 0x38, line});
    c.set(4, c.toc(toc::success));
}
}
std::uint32_t unsupported_44(Cpu& c) { unsupported(c, 0x10478fd0, 0x10479000, 0x54); return 0; }
std::uint32_t unsupported_94(Cpu& c) { unsupported(c, 0x10479060, 0x10479090, 0x60); return 0xffffffffu; }

void clear_b0(Cpu& c, std::uint32_t object) { c.put8(object + cup::byte_b0, 0); }

bool has_entrant(Cpu& c, std::uint32_t object, std::uint32_t club) {
    // The counter is a 32-bit register compared as its low halfword, signed (`extsh`), with the halfword
    // count re-read each pass. At the blr r5 and r6 hold: the entrant table and the index when found; the
    // index's halfword, sign-extended, and the index when not.
    std::uint32_t index = 0;
    for (; std::int32_t(std::int16_t(index)) < c.i16(object + competition::club_count); ++index) {
        const auto table = c.u32(object + cup::entrants);
        if (c.u32(table + std::uint32_t(std::int32_t(std::int16_t(index))) * cup::entrant_row) == club) {
            c.set({{5, table}, {6, index}});
            return true;
        }
    }
    c.set({{5, sext16(index)}, {6, index}});
    return false;
}

}
