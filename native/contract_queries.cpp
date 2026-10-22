#include "contract_queries.hpp"
#include "semantic_guest_call.hpp"
#include <algorithm>
#include <array>
#include <optional>

namespace cm::aot::contract_queries {
namespace {
using Call = SemanticGuestCall;

// *(TOC - 0x7be4) is the number of players. *(TOC - 0x7b9c) is the contract
// index: a 100-byte row per player id, +0 the player's first contract and +4
// the second, negative when absent. Contracts are 100-byte records from
// *holder, with the club id at +4. Players and clubs keep their id at +0.
namespace layout {
constexpr std::uint32_t player_count = 0x7be4, contract_index = 0x7b9c, row = 100;
constexpr std::uint32_t first = 0, second = 4, contract_club = 4, id = 0;
}
// A read the original would make after writing that word of its frame.
struct FrameOverlap {};

class Native {
public:
    explicit Native(State& state) : s(state), toc(std::uint32_t(state.r<4>(8))) {}
    std::uint32_t r(unsigned n) const { return std::uint32_t(s.r<4>(n * 4)); }
    std::uint32_t word(std::uint32_t address) const {
        for (auto slot : frame)
            if (slot && std::uint64_t(address) < std::uint64_t(slot) + 4 && slot < std::uint64_t(address) + 4)
                throw FrameOverlap{};
        return std::uint32_t(s.memory.read<4>(address));
    }
    // cmpw / cmpwi and cmplw / cmplwi: CR0 is LT, GT, EQ and XER[SO].
    std::uint8_t compare(std::int32_t a, std::int32_t b) const { return field(a < b, a > b); }
    std::uint8_t compare_logical(std::uint32_t a, std::uint32_t b) const { return field(a < b, a > b); }

    // 0x10082f20 (first contract) and 0x10083110 (second): the player's
    // contract, or 0. Sets r0, r3, r4, r5, r6 as the listing does on the
    // path taken, and CR0.
    std::uint32_t lookup(std::uint32_t holder, std::uint32_t player, std::uint32_t slot) {
        cr0 = compare_logical(player, 0);                        // cmplwi r4,0
        if (!player) { charge += 4; return gpr(3, 0); }          // li r3,0; blr
        gpr(5, word(toc - layout::player_count));
        gpr(6, word(player + layout::id));
        gpr(0, word(value[5]));
        cr0 = compare(std::int32_t(value[6]), std::int32_t(value[0]));
        if (!(std::int32_t(value[6]) < std::int32_t(value[0]))) { charge += 10; return gpr(3, 0); }
        gpr(0, word(player + layout::id) * layout::row);
        gpr(4, word(word(toc - layout::contract_index)));
        if (slot == layout::first) gpr(0, word(value[4] + value[0]));            // lwzx r0,r4,r0
        else { gpr(4, value[4] + value[0]); gpr(0, word(value[4] + layout::second)); }
        cr0 = compare(std::int32_t(value[0]), 0);
        const unsigned path = slot == layout::first ? 0 : 1;
        if (std::int32_t(value[0]) < 0) { charge += 17 + path; return gpr(3, 0); }
        gpr(0, value[0] * layout::row);
        charge += 19 + path;
        return gpr(3, word(holder) + value[0]);
    }

    // 0x10082f90: the contract that ties the player to the club, or 0. Only
    // paths that settle without the club resolver; nullopt for the others.
    std::optional<std::uint32_t> club(std::uint32_t holder, std::uint32_t player, std::uint32_t club) {
        const auto sp = r(1);
        // The original stores these first; a store that would fault faults there.
        for (auto slot : {sp - 4, sp - 8, sp - 0xc, sp + 8, sp - 0x50}) s.memory.read<4>(slot);
        frame = {sp - 4, sp - 8, sp - 0xc, sp + 8, sp - 0x50};    // stw r31/r30/r29, LR; stwu
        gpr(6, word(toc - layout::player_count));
        cr0 = compare_logical(player, 0);
        if (!player) { charge += 19; return 0; }
        cr0 = compare_logical(club, 0);
        if (!club) { charge += 21; return 0; }
        // Both validations re-read the id and the count, as the original does.
        cr0 = compare_logical(player, 0);
        gpr(5, word(player + layout::id)); gpr(0, word(value[6]));
        cr0 = compare(std::int32_t(value[5]), std::int32_t(value[0]));
        if (!(std::int32_t(value[5]) < std::int32_t(value[0]))) { charge += 27; return 0; }
        cr0 = compare_logical(player, 0);
        gpr(5, word(player + layout::id)); gpr(0, word(value[6]));
        cr0 = compare(std::int32_t(value[5]), std::int32_t(value[0]));
        std::uint32_t primary = 0;
        if (!(std::int32_t(value[5]) < std::int32_t(value[0]))) charge += 27;
        else {
            gpr(0, word(player + layout::id) * layout::row);
            gpr(5, word(word(toc - layout::contract_index)));
            gpr(0, word(value[5] + value[0]));
            cr0 = compare(std::int32_t(value[0]), 0);
            if (std::int32_t(value[0]) < 0) charge += 34;
            else { gpr(0, value[0] * layout::row); gpr(5, word(holder)); primary = value[5] + value[0]; charge += 35; }
        }
        cr0 = compare_logical(primary, 0);
        if (!primary) { charge += 11; return 0; }
        gpr(5, word(primary + layout::contract_club)); gpr(0, word(club + layout::id));
        cr0 = compare(std::int32_t(value[5]), std::int32_t(value[0]));
        if (value[5] == value[0]) { charge += 15; return primary; }                 // 8, then the epilogue
        called = true;                                              // bl 0x10083110
        const auto secondary = lookup(holder, player, layout::second);
        // or. r29,r3,r3; the lookup's compares leave CR0's other bits clear.
        cr0 = compare(std::int32_t(secondary), 0);
        if (!secondary) return std::nullopt;
        gpr(4, word(secondary + layout::contract_club)); gpr(0, word(club + layout::id));
        cr0 = compare(std::int32_t(value[4]), std::int32_t(value[0]));
        if (value[4] != value[0]) return std::nullopt;
        charge += 21;                     // 7 to the bl, 7 after it, 7 epilogue; the callee charged its own
        return secondary;
    }

    // Publishes the effects; `frame_return` adds the epilogue of 0x10082f90.
    void apply(std::uint32_t result, bool frame_return) {
        if (frame_return) {
            const auto sp = r(1), link = std::uint32_t(s.r<4>(4128));
            s.memory.write<4>(sp - 4, r(31)); s.memory.write<4>(sp - 8, r(30));
            s.memory.write<4>(sp - 0xc, r(29)); s.memory.write<4>(sp + 8, link);
            s.memory.write<4>(sp - 0x50, sp);
            gpr(0, link);                                           // lwz r0,0x58(r1)
            if (called) s.wr<4>(2440, toc);
        }
        for (unsigned n : {0u, 4u, 5u, 6u}) if (set[n]) s.wr<4>(n * 4, value[n]);
        s.wr<4>(12, result);
        s.wr<1>(2304, cr0);
        s.executed += charge;
        s.pc = std::uint32_t(s.r<4>(4128));
    }
    unsigned charge = 0;
    bool called = false;   // 0x10082f90 ran 0x10083110

private:
    std::uint8_t field(bool less, bool greater) const {
        return std::uint8_t((less ? 8 : greater ? 4 : 2) | (s.r<1>(1024) & 1));
    }
    std::uint32_t gpr(unsigned n, std::uint32_t v) { value[n] = v; set[n] = true; return v; }
    State& s;
    std::uint32_t toc;
    std::array<std::uint32_t, 5> frame{};
    std::array<std::uint32_t, 7> value{};
    std::array<bool, 7> set{};
    std::uint8_t cr0 = 0;
};

void select_club_contract(Call& a) {
    bool missing = a.equal();
    if (!missing) { a.compare(31, 0, false); missing = a.equal(); }
    if (missing) { a.at(0x10082fc0); a.value(3, 0); a.jump(0x100830ec); return; }

    a.at(0x10082fc8); a.compare(4, 0, false);
    bool valid = a.equal();
    if (!valid) { a.load(5, 4, 0); a.load(0, 6, 0); a.compare_registers(5, 0); valid = a.less(); }
    if (!valid) { a.value(3, 0); a.jump(0x100830ec); return; }

    // Preserve the second live validation and lookup, including its null and
    // signed-index paths. The original does not cache the first staff ID read.
    a.at(0x10082fe8); a.compare(4, 0, false);
    if (a.equal()) { a.value(30, 0); a.jump(0x10083044); }
    else {
        a.at(0x10082ff8); valid = a.equal();
        if (!valid) { a.load(5, 4, 0); a.load(0, 6, 0); a.compare_registers(5, 0); valid = a.less(); }
        if (!valid) { a.value(30, 0); a.jump(0x10083044); }
        else {
            a.at(0x10083014); a.load(0, 4, 0); a.load(5, 2, -0x7b9c);
            a.multiply(0, 0, 100); a.load(5, 5, 0); a.indexed_load(0, 5, 0); a.compare(0, 0);
            if (!a.at_least()) { a.value(30, 0); a.jump(0x10083044); }
            else { a.at(0x10083038); a.multiply(0, 0, 100); a.load(5, 3, 0); a.add(30, 5, 0); }
        }
    }
    a.at(0x10083044); a.compare(30, 0, false);
    if (a.equal()) { a.value(3, 0); a.jump(0x100830ec); return; }

    a.at(0x10083054); a.load(5, 30, 4); a.load(0, 31, 0); a.compare_registers(5, 0);
    if (a.equal()) { a.move(3, 30); a.jump(0x100830ec); return; }

    a.at(0x1008306c); a.call(secondary_entry); a.move(29, 3, true);
    if (!a.equal()) {
        a.load(4, 29, 4); a.load(0, 31, 0); a.compare_registers(4, 0);
        if (a.equal()) { a.jump(0x100830ec); return; }
    }

    // The club resolver's byte output controls whether its returned club may
    // replace the requested one. Its register and memory effects stay live.
    a.at(0x1008308c); a.move(3, 31); a.offset(4, 1, 0x38); a.value(5, 1); a.call(0x100ce320);
    a.compare(3, 0, false);
    if (!a.equal()) {
        a.load<1>(0, 1, 0x38); a.sign8(0, 0, true);
        if (a.equal()) a.move(31, 3);
    }
    a.at(0x100830b4); a.load(0, 30, 4); a.load(3, 31, 0); a.compare_registers(0, 3);
    if (a.equal()) { a.move(3, 30); a.jump(0x100830ec); return; }
    a.at(0x100830cc); a.compare(29, 0, false);
    if (!a.equal()) {
        a.load(0, 29, 4); a.compare_registers(0, 3);
        if (a.equal()) { a.move(3, 29); a.jump(0x100830ec); return; }
    }
    a.at(0x100830e8); a.value(3, 0);
}
}

NativeRun run_native(State& s, std::uint32_t entry) {
    if (std::find(entries.begin(), entries.end(), entry) == entries.end() || s.memory.date_observer ||
        s.memory.read_trace) return {};
    Native native(s);
    std::uint32_t result;
    try {
        if (entry == club_entry) {
            const auto found = native.club(native.r(3), native.r(4), native.r(5));
            if (!found) return {};
            result = *found;
        } else result = native.lookup(native.r(3), native.r(4), entry == primary_entry ? layout::first : layout::second);
    } catch (const FrameOverlap&) { return {}; }
    catch (const std::runtime_error&) { return {}; }
    if (s.executed > s.budget || s.budget - s.executed < native.charge) return {};
    native.apply(result, entry == club_entry);
    return {true, native.called};
}

void find_primary(State& s, SemanticDependencyDispatch* d, SemanticGuestObserver* o) {
    Call a(s, primary_entry, d, o); a.compare(4, 0, false);
    if (a.equal()) { a.value(3, 0); a.finish(); return; }
    a.at(0x10082f30); bool valid = a.equal();
    if (!valid) { a.load(5, 2, -0x7be4); a.load(6, 4, 0); a.load(0, 5, 0); a.compare_registers(6, 0); valid = a.less(); }
    if (!valid) { a.value(3, 0); a.finish(); return; }
    a.at(0x10082f50); a.load(0, 4, 0); a.load(4, 2, -0x7b9c); a.multiply(0, 0, 100);
    a.load(4, 4, 0); a.indexed_load(0, 4, 0); a.compare(0, 0);
    if (!a.at_least()) { a.value(3, 0); a.finish(); return; }
    a.at(0x10082f74); a.multiply(0, 0, 100); a.load(3, 3, 0); a.add(3, 3, 0); a.finish();
}

void find_for_club(State& s, SemanticDependencyDispatch* d, SemanticGuestObserver* o) {
    Call a(s, club_entry, d, o); a.link(); a.save(31); a.compare(4, 0, false); a.load(6, 2, -0x7be4);
    a.save(30); a.move(31, 5); a.save(29); a.save_link(); a.frame(80);
    select_club_contract(a);
    a.at(0x100830ec); a.pop(80); a.restore(31); a.restore(30); a.restore(29); a.finish();
}

void find_secondary(State& s, SemanticDependencyDispatch* d, SemanticGuestObserver* o) {
    Call a(s, secondary_entry, d, o); a.compare(4, 0, false);
    if (a.equal()) { a.value(3, 0); a.finish(); return; }
    a.at(0x10083120); bool valid = a.equal();
    if (!valid) { a.load(5, 2, -0x7be4); a.load(6, 4, 0); a.load(0, 5, 0); a.compare_registers(6, 0); valid = a.less(); }
    if (!valid) { a.value(3, 0); a.finish(); return; }
    a.at(0x10083140); a.load(0, 4, 0); a.load(4, 2, -0x7b9c); a.multiply(0, 0, 100);
    a.load(4, 4, 0); a.add(4, 4, 0); a.load(0, 4, 4); a.compare(0, 0);
    if (!a.at_least()) { a.value(3, 0); a.finish(); return; }
    a.at(0x10083168); a.multiply(0, 0, 100); a.load(3, 3, 0); a.add(3, 3, 0); a.finish();
}
}
