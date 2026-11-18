// COMP.CPP: the nation descriptors' codes (toc::descriptors, 34 rows of 0x4c bytes). A lookup matches a row
// whose nation is loaded (its +0x120 byte nonzero) and answers the row's code byte; no match answers 3.
#include "comp_detail.hpp"

namespace cm::aot::comp {

namespace {
// The row lookup the module inlines three times: the first row whose nation is loaded (its +0x120 byte
// nonzero) and equals `nation`. `index` is descriptor::count when none matches; `candidate` is the nation word
// of the last row examined (the register the original's loop leaves holding it).
struct DescriptorScan { std::uint32_t index, candidate; };
DescriptorScan scan_descriptors(Cpu& c, std::uint32_t rows, std::uint32_t nation) {
    std::uint32_t candidate = 0;
    for (std::uint32_t n = 0; n < descriptor::count; ++n) {
        candidate = c.u32(rows + n * descriptor::size + descriptor::nation);
        if (candidate && c.i8(candidate + descriptor::nation_loaded) != 0 && candidate == nation) return {n, candidate};
    }
    return {descriptor::count, candidate};
}
}

std::uint32_t descriptor_code(Cpu& c, std::uint32_t competition) {
    if (!competition) return descriptor::none;
    const auto holder = c.u32(competition + competition::holder);
    if (!holder) return descriptor::none;
    const auto nation = c.u32(holder + 0x18);
    if (!nation) return descriptor::none;
    const auto rows = c.toc(toc::descriptors);
    const auto scan = scan_descriptors(c, rows, nation);
    return scan.index < descriptor::count ? c.u8(rows + scan.index * descriptor::size + descriptor::code) : descriptor::none;
}

// ---- the calendar block's lookups (comp_calendar.hpp) ----

std::uint32_t descriptor_index(Cpu& c, std::int8_t code) {
    const Frame frame(c, 0x10451720, 0x70, {.first_gpr = 30});
    const auto rows = c.toc(toc::descriptors);
    // `char` counter and compare: the signed byte of the code against the row's code byte, signed.
    for (std::uint32_t n = 0; n < descriptor::count; ++n)
        if (std::int8_t(c.u8(rows + n * descriptor::size + descriptor::code)) == code) return n;
    // r30: the message buffer, r31: the pool, as the original holds them at the report's calls.
    c.set({{30, c.toc(toc::message)}, {31, c.toc(toc::pool)}});
    report(c, frame, {0x10451798, 0x40, 0x38, 0x541});
    return descriptor::none;
}

std::uint32_t nation_descriptor_code(Cpu& c, std::uint32_t club) {
    const Frame frame(c, 0x10451990, 0x40, {});
    // Leftovers the generated callers (FIX_MAN) read: r5 the rows and r6 the nation from 0x10451994/0x104519a0; with a nation
    // the scan leaves r4 the address of the row it stopped at, r7 that row's nation word and r8 its index (0x104519ac..0x104519f4;
    // a match returns from there, no match falls through to 0x100c7d00 with 34, the end row's address and row 33's nation).
    const auto rows = c.toc(toc::descriptors), nation = c.u32(club + 0x2c);
    c.set({{5, rows}, {6, nation}});
    if (nation) {
        const auto scan = scan_descriptors(c, rows, nation);
        c.set({{4, rows + scan.index * descriptor::size}, {7, scan.candidate}, {8, scan.index}});
        if (scan.index < descriptor::count) return c.u8(rows + scan.index * descriptor::size + descriptor::code);
    }
    // 0x100c7d00 (r3 still the club) decides: nonzero answers 0, zero answers 3.
    invoke(c, 0x100c7d00, 0x10451a08, {club});
    return std::uint8_t(c.r(3)) != 0 ? 0 : descriptor::none;
}

std::uint32_t fixture_descriptor_code(Cpu& c, std::uint32_t fixture, std::uint32_t context) {
    const Frame frame(c, 0x10451800, 0x50, {.first_gpr = 26});
    const auto rules = c.toc(toc::fixture_rules);   // r31: the cell of the object that rules on a fixture's competition
    const auto no_club = c.toc(-0x7aa8);            // r29: the cell of a word that means "none" (compared with a club's word +0)
    const auto rows = c.toc(toc::descriptors);      // r30
    const auto nation = c.u32(fixture + 8);         // r28
    c.set({{26, fixture}, {27, context}, {28, nation}, {29, no_club}, {30, rows}, {31, rules}});
    invoke(c, 0x10494d20, 0x10451834, {c.u32(rules), fixture});
    const bool ruled = std::uint8_t(c.r(3)) != 0;
    // The fixture belongs to the rules' competition when it is ruled so or names the "none" word, and the
    // context's word +0x14 is the rules object's +4 or points at that word.
    bool in_competition = (ruled || c.u32(fixture) == c.u32(no_club)) && context != 0;
    if (in_competition) {
        const auto holder = c.u32(context + 0x14);
        c.set(4, holder);  // 0x10451858
        in_competition = holder == c.u32(c.u32(rules) + 4) || c.u32(holder) == c.u32(no_club);
    }
    if (in_competition) {
        invoke(c, 0x100c7d00, 0x10451880, {c.u32(context + 0x1c)});
        if (std::uint8_t(c.r(3)) != 0) return 1;
        invoke(c, 0x100c7d00, 0x10451890, {c.u32(context + 0x20)});
        if (std::uint8_t(c.r(3)) != 0) return 1;
        const auto home = c.u32(context + 0x1c), away = c.u32(context + 0x20);
        const auto home_nation = c.u32(home + 0x2c), away_nation = c.u32(away + 0x2c);
        // r4 the second club and r5 the first's nation (0x104518a0), as the callee or the answer 3 leave them.
        c.set({{4, away}, {5, home_nation}});
        if (home_nation != away_nation || c.i8(home_nation + descriptor::nation_loaded) == 0) return descriptor::none;
        // r3 is still the first club.
        c.link(0x104518c0);
        return nation_descriptor_code(c, home);
    }
    if (c.u32(fixture + 8) != 0) {
        // The inline scan leaves r4 the last nation word it loaded and r5 the row index (0x104518e4..0x10451938).
        const auto scan = scan_descriptors(c, rows, nation);
        c.set({{4, scan.candidate}, {5, scan.index}});
        return scan.index < descriptor::count ? c.u8(rows + scan.index * descriptor::size + descriptor::code) : descriptor::none;
    }
    invoke(c, 0x10192740, 0x1045194c, {fixture});
    if (std::uint8_t(c.r(3)) != 0) return 0;
    invoke(c, 0x10192860, 0x10451964, {fixture});
    if (std::uint8_t(c.r(3)) != 0) return 1;
    invoke(c, 0x10192130, 0x1045197c, {fixture});
    return c.r(3);  // the callee's whole r3
}

}
