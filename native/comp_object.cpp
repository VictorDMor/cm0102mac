// COMP.CPP: the base competition object's lifetime and its stage accessor. The base class's one-instruction
// virtual defaults have no body here: their lines in comp_routines.inc return the constant.
#include "comp_detail.hpp"

namespace cm::aot::comp {

std::uint32_t construct(Cpu& c, std::uint32_t object) {
    // A leaf with no frame: it writes the vtable and the base fields, nothing else (the derived constructor
    // writes the rest, the record at +4 included).
    c.put32(object + competition::vtable, c.toc(toc::vtable));
    for (const auto field : {competition::stats, competition::stages, competition::holder, competition::block_14,
                             competition::block_18, competition::word_28, competition::word_2c})
        c.put32(object + field, 0);
    for (const auto field : {competition::parent, competition::next, competition::word_24, competition::last_stage})
        c.put32(object + field, 0xffffffffu);
    for (const auto field : {competition::half_34, competition::half_36, competition::half_38, competition::code,
                             competition::club_count})
        c.put16(object + field, 0);
    c.put8(object + competition::type, 1);
    c.put8(object + competition::byte_45, 1);
    c.put8(object + competition::bench, 5);
    c.put8(object + competition::substitutes, 3);
    c.put8(object + competition::byte_48, 0);
    c.put8(object + competition::byte_49, 0);
    c.put8(object + competition::ruling_body, 0xff);
    c.put8(object + competition::byte_4b, 20);
    c.put8(object + competition::byte_4c, 0);
    // The constants the leaf loads (0x1044fed4..0x1044fee8) stay in r4-r8 at its blr (0x1044ff54), and the generated caller
    // at 0x10531d38 carries r6-r8 into a virtual call.
    c.set({{4, 5}, {5, 20}, {6, 0xffffffffu}, {7, 1}, {8, 0}});
    return object;
}

std::uint32_t destroy(Cpu& c, std::uint32_t object, std::uint32_t deleting) {
    const Frame frame(c, 0x1044ff60, 0x40, {.first_gpr = 30});
    if (!object) return object;
    // The original keeps the object in r30 and the flag word in r31 across its calls; the allocator's
    // calls can reach imports, where a replay compares the non-volatile registers.
    c.set({{30, object}, {31, deleting}});
    c.put32(object + competition::vtable, c.toc(toc::vtable));
    // +0x14 is freed and left as it was; +0x18 is freed and cleared.
    if (const auto block = c.u32(object + competition::block_14)) invoke(c, callee::free_block, 0x1044ff98, {block});
    if (const auto block = c.u32(object + competition::block_18)) {
        invoke(c, callee::free_block, 0x1044ffa8, {block});
        c.put32(object + competition::block_18, 0);
    }
    if (std::int16_t(deleting) > 0) invoke(c, callee::free_object, 0x1044ffc0, {object});
    return object;
}

std::uint32_t stage(Cpu& c, std::uint32_t competition, std::int32_t index) {
    const Frame frame(c, 0x10450b60, 0x70, {.first_gpr = 30});
    // r30: the message buffer, r31: the pool, as the original holds them at the report's calls.
    c.set({{30, c.toc(toc::message)}, {31, c.toc(toc::pool)}});
    return checked_stage(c, frame, competition, index, {0x10450ba4, 0x40, 0x38, 0x36e});
}

}
