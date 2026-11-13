#include "national_callup.hpp"
#include "match_debugging.hpp"
#include "debug_localization.hpp"
#include "match_replay.hpp"
#include "guest_abi.hpp"
#include "semantic_io_checks.hpp"
#include <iostream>
#include <set>

namespace {
using namespace cm::aot;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::uint32_t invoke(State& s, unsigned entry, std::initializer_list<unsigned> args) {
    const auto message_pointer = std::uint32_t(s.memory.read<4>(s.r<4>(8) - 0x7ba8));
    unsigned reg = 3;
    for (auto value : args) s.wr<4>(reg++ * 4, value);
    s.wr<4>(4128, return_sentinel); s.pc = entry; s.budget = s.executed + 100000000;
    try { run_until(s, return_sentinel); }
    catch (const std::exception& e) {
        guest::Cpu c(s); std::string message;
        for (unsigned i = 0; i < 250 && c.u8(message_pointer + i); ++i) message += char(c.u8(message_pointer + i));
        throw std::runtime_error(hex_address(entry) + " at " + hex_address(s.pc) + ": " + e.what() + " last assertion: " + message);
    }
    return std::uint32_t(s.r<4>(12));
}
struct Writes final : MemoryObserver {
    std::uint32_t sp;
    std::set<std::uint32_t> bytes;
    explicit Writes(std::uint32_t stack) : sp(stack) {}
    void mapped(std::uint32_t, std::uint64_t) override { throw std::runtime_error("Call-up allocated guest memory"); }
    void written(std::uint32_t at, const std::uint8_t*, std::size_t size) override {
        for (std::size_t i = 0; i < size; ++i)
            if (at + i < sp - 0x10000 || at + i >= sp) bytes.insert(at + unsigned(i));
    }
    void cleared(std::uint32_t at, std::uint64_t size) override { written(at, nullptr, std::size_t(size)); }
};
std::vector<std::uint8_t> rng(const State& s) {
    auto result = s.memory.copy_out(0x108b3f00, 16);
    for (const auto pointer : {0x108b3f00u, 0x108b3f08u, 0x108b3f0cu}) {
        const auto bytes = s.memory.copy_out(s.memory.read<4>(pointer), 4);
        result.insert(result.end(), bytes.begin(), bytes.end());
    }
    const auto samples = s.memory.copy_out(s.memory.read<4>(0x108b3f04), 102000);
    result.insert(result.end(), samples.begin(), samples.end());
    return result;
}
std::set<unsigned> roster(guest::Cpu& c, unsigned club, unsigned mask = 0) {
    std::set<unsigned> ids;
    for (unsigned i = 0; i < 50; ++i) if (const auto staff = c.u32(club + 0x98 + i * 4); staff && (!mask || (c.u8(staff + 0x6a) & mask)))
        require(ids.insert(c.u32(staff)).second, "Duplicate national call-up");
    return ids;
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Expected existing career snapshot");
        auto state = std::make_unique<State>(); load_match_snapshot(*state, argv[1]);
        auto& s = *state; guest::Cpu c(s); c.set(2, c.u32(c.sp() + 0x14));
        initialize_native_debugging(false);
        const auto clubs = c.u32(c.toc(-0x7be8)), count = c.u32(c.toc(-0x7b50)), nations = c.u32(c.toc(-0x7b04));
        const auto senior = clubs + (count - 2 * nations + 1) * 0x248;
        const auto youth = senior + nations * 0x248, foreign_team = senior + 0x248;
        const auto country = c.u32(senior + 0x2c), foreign = c.u32(foreign_team + 0x2c);
        require(is_national_squad(s, senior) && is_national_squad(s, youth) && !is_national_squad(s, clubs) &&
                !is_national_squad(s, 0) && !is_national_squad(s, senior + 4), "National-team scope");
        const auto staff_count = c.u32(c.toc(-0x7be4));
        // Controlled records in an existing snapshot; retain its tables/services,
        // human count and tactics layout. No career file is loaded or overwritten.
        const auto people = invoke(s, 0x10773520, {staff_count * 0x70});
        s.memory.copy_in(people, std::vector<std::uint8_t>(staff_count * 0x70));
        const auto players = invoke(s, 0x10773520, {40 * 0x48});
        s.memory.copy_in(players, std::vector<std::uint8_t>(40 * 0x48));
        const auto histories = invoke(s, 0x10773520, {40 * 0x64});
        s.memory.copy_in(histories, std::vector<std::uint8_t>(40 * 0x64));
        const auto medical = invoke(s, 0x10773520, {8 + 40 * 0x24 + 0x38});
        s.memory.copy_in(medical, std::vector<std::uint8_t>(8 + 40 * 0x24 + 0x38));
        c.put32(c.toc(-0x7be0), people); c.put32(c.toc(-0x7b9c), histories); c.put32(c.toc(-0x7ba0), medical);
        c.put32(medical, medical + 8); c.put32(medical + 4, medical + 8 + 40 * 0x24);
        const auto staff = [&](unsigned id) { return people + id * 0x70; };
        const auto today = c.toc(-0x7b80), tactics = c.u32(c.toc(-0x7a6c));
        const auto national_teams = c.u32(c.toc(-0x7a64));
        const auto national_row = c.u32(national_teams + 4) + c.u32(country + 0x68) * 0x74;
        for (unsigned kind = 0; kind < 3; ++kind) c.put8(national_row + kind * 0x14 + 0x4a, 0);
        for (unsigned id = 0; id < 40; ++id) {
            const auto p = staff(id);
            c.put32(p, id); c.put32(p + 0xc, country); c.put32(p + 0x14, players + id * 0x48);
            c.put32(p + 0x24, clubs); c.put8(p + 0x62, 11); c.put8(p + 0x6a, 2);
            c.put16(p + 0x5c, 20);
            c.put16(p + 0x34, c.u16(today)); c.put16(p + 0x36, c.u16(today + 2) - 20);
            c.put16(players + id * 0x48 + 4, id < 30 ? 100 + id : 200);
            c.put32(medical + 8 + id * 0x24 + 0x10, -1);
        }
        c.put16(players + 7 * 0x48 + 4, 108); // Boundary tie: lower id 7 wins over 8.
        c.put32(staff(30) + 0xc, foreign);
        for (auto id : {31u, 35u, 36u}) { c.put32(staff(id) + 0xc, foreign); c.put32(staff(id) + 0x10, country); }
        c.put8(staff(31) + 0x5e, 1); c.put8(histories + 36 * 0x64 + 0xd, 0x40);
        c.put8(histories + 32 * 0x64 + 0xc, 4); // International retirement.
        c.put32(medical + 8 + 33 * 0x24 + 0x10, 0);
        const auto injury = c.u32(medical + 4);
        s.memory.copy_in(injury + 0x28, s.memory.copy_out(today, 8));
        c.put16(injury + 0x30, 14); c.put16(injury + 0x32, 7);
        c.put8(staff(34) + 0x62, 0);
        c.put32(staff(37) + 0x20, foreign_team); c.put8(staff(37) + 0x60, 11);
        const auto dislikes = invoke(s, 0x10773520, {0x34});
        s.memory.copy_in(dislikes, std::vector<std::uint8_t>(0x34));
        c.put32(staff(38) + 0x18, dislikes); c.put32(dislikes + 0x10, senior);
        c.put32(staff(39) + 0x14, 0);
        for (const auto club : {senior, youth}) s.memory.copy_in(club + 0x98, std::vector<std::uint8_t>(200));
        for (auto id : {0u, 10u, 29u}) invoke(s, 0x102b8c50, {0, country, 0, staff(id)});
        invoke(s, 0x102b8c50, {0, country, 2, staff(35)});
        // A dismissed player holds a tactical slot in the original team record.
        require(invoke(s, 0x103af9d0, {tactics, senior, 0, staff(0), 0}), "Could not seed tactical slot");
        for (bool deadline_today : {false, true}) {
            auto locked = cm::semantic_io_checks::clone(s); guest::Cpu l(*locked);
            const auto lock = national_row + 2 * 0x14;
            l.put8(lock + 0x4a, 1); l.put16(lock + 0x40, c.u16(today));
            l.put16(lock + 0x42, c.u16(today + 2) + (deadline_today ? 0 : 1));
            if (deadline_today) {
                // A real same-day youth fixture stops the original calendar
                // search before it needs an archive outside this snapshot.
                const auto calendar = l.u32(l.toc(-0x7a60));
                const auto region = l.u32(calendar + 12 * l.u16(today));
                const auto list = l.u32(region);
                require(list, "Missing current-day fixture list");
                const auto fixture = invoke(*locked, 0x10773520, {0x50});
                locked->memory.copy_in(fixture, std::vector<std::uint8_t>(0x50));
                l.put32(fixture + 0xc, l.u32(youth)); l.put32(fixture + 0x10, l.u32(foreign_team));
                l.put8(fixture + 0x40, 1); l.put8(fixture + 0x41, 1);
                invoke(*locked, 0x10738d10, {list, fixture, 0x50});
            }
            const bool allowed = invoke(*locked, 0x102af090, {national_teams, country, 2, staff(35), 0, 0, 0}) != 0;
            require(allowed == deadline_today, "Original future/exact-day release permission fixture");
            const auto locked_rng = rng(*locked);
            require(call_up_national_squad(*locked, senior), "Sibling-lock call-up failed");
            require(roster(l, senior).size() == 23 && roster(l, senior).contains(35) == allowed &&
                    roster(l, youth).contains(35) != allowed, "Sibling lock differed from original manual permission");
            require(locked_rng == rng(*locked), "Sibling permission consumed RNG");
        }
        const auto retained = s.memory.copy_out(staff(10), 0x70), club_roster = s.memory.copy_out(clubs, 0x248);
        std::set<unsigned> expected{7, 35};
        for (unsigned id = 9; id < 30; ++id) expected.insert(id);
        auto reference = cm::semantic_io_checks::clone(s);
        const auto registers = s.registers; const auto unique = s.unique; const auto pc = s.pc;
        const auto random = rng(s), health = s.memory.copy_out(medical, 8 + 40 * 0x24 + 0x38);
        Writes actual(c.sp()); s.memory.observer = &actual;
        require(call_up_national_squad(s, senior), "Call-up rejected national team with debugging off");
        s.memory.observer = nullptr;
        require(s.registers == registers && s.unique == unique && s.pc == pc, "Call-up clobbered guest caller");
        require(roster(c, senior) == expected && roster(c, youth).empty(), "Wrong top 23 or youth promotion");
        require(s.memory.copy_out(staff(10), 0x70) == retained, "Existing selected player changed");
        require(c.u32(staff(0) + 0x20) == 0 && c.u8(staff(0) + 0x60) == 0 && c.u8(staff(0) + 0x6a) == 2,
                "Released player retained national links/flags");
        require(s.memory.copy_out(clubs, 0x248) == club_roster && health == s.memory.copy_out(medical, health.size()) &&
                random == rng(s), "Call-up changed club, medical state or game RNG");
        for (auto id : expected) require(c.u32(staff(id) + 0x20) == senior && c.u8(staff(id) + 0x60) == 11 &&
                                         c.u32(staff(id) + 0x24) == clubs, "Called-up player links");
        // Compare every non-stack write with the original manual release/add
        // sequence, including tactics, dates, national links and roster slots.
        Writes original(c.sp()); reference->memory.observer = &original;
        reference->wr<4>(4, c.sp() - 0x400);
        invoke(*reference, 0x102b8d60, {0, country, 0, staff(0)});
        invoke(*reference, 0x102b8d60, {0, country, 2, staff(35)});
        invoke(*reference, 0x102b8c50, {0, country, 0, staff(35)});
        for (int id = 28; id >= 9; --id) if (id != 10) invoke(*reference, 0x102b8c50, {0, country, 0, staff(unsigned(id))});
        invoke(*reference, 0x102b8c50, {0, country, 0, staff(7)});
        reference->memory.observer = nullptr;
        if (actual.bytes != original.bytes) {
            for (auto at : actual.bytes) if (!original.bytes.contains(at)) std::cerr << "Only call-up wrote " << hex_address(at) << '\n';
            for (auto at : original.bytes) if (!actual.bytes.contains(at)) std::cerr << "Only manual wrote " << hex_address(at) << '\n';
            throw std::runtime_error("Call-up changed fields outside original manual operations");
        }
        for (auto at : actual.bytes) require(s.memory.read<1>(at) == reference->memory.read<1>(at), "Manual call-up state differs");
        require(!invoke(s, 0x103af800, {tactics, senior, 0, 0}), "Dismissed player remained in tactics");
        Writes repeated(c.sp()); s.memory.observer = &repeated;
        require(call_up_national_squad(s, senior), "Repeated call-up failed");
        s.memory.observer = nullptr;
        require(repeated.bytes.empty(), "Repeated call-up wrote game state");
        const auto before = s.executed;
        require(!call_up_national_squad(s, clubs) && !call_up_national_squad(s, 0) && s.executed == before,
                "Ordinary club accepted call-up");
        // Ability changes are read on every click; potential is irrelevant.
        c.put16(players + 4, 199); c.put16(players + 8 * 0x48 + 6, 200);
        require(call_up_national_squad(s, senior), "Refreshing call-up failed");
        expected.erase(7); expected.insert(0);
        require(roster(c, senior) == expected && !c.u32(staff(7) + 0x20), "Changed ability did not replace cutoff player");
        {
            auto sides = cm::semantic_io_checks::clone(s); guest::Cpu b(*sides);
            invoke(*sides, 0x102b8c50, {0, country, 1, staff(8)});
            const auto reserve = sides->memory.copy_out(staff(8), 0x70);
            require(call_up_national_squad(*sides, senior) && roster(b, senior, 4) == expected &&
                    roster(b, senior, 8) == std::set<unsigned>({8}) && sides->memory.copy_out(staff(8), 0x70) == reserve,
                    "Senior call-up disturbed unselected reserves");
            b.put16(players + 8 * 0x48 + 4, 201);
            auto promoted = expected; promoted.erase(9); promoted.insert(8);
            require(call_up_national_squad(*sides, senior) && roster(b, senior, 4) == promoted && roster(b, senior, 8).empty(),
                    "Selected reserve did not join the senior squad");
            invoke(*sides, 0x102b8c50, {0, country, 0, staff(7)});
            const auto senior_left = sides->memory.copy_out(staff(7), 0x70);
            require(call_up_national_squad(*sides, senior, 1) && roster(b, senior, 8) == promoted &&
                    roster(b, senior, 4) == std::set<unsigned>({7}) && sides->memory.copy_out(staff(7), 0x70) == senior_left,
                    "Reserves-view call-up changed the wrong squad");
            require(random == rng(*sides), "Reserve-side call-up consumed RNG");
        }
        // Short pool and the youth age limit: retain exactly the available set.
        for (unsigned id = 5; id < 40; ++id) c.put8(staff(id) + 0x62, 0);
        c.put16(staff(4) + 0x5c, 22);
        require(call_up_national_squad(s, youth) && roster(c, youth) == std::set<unsigned>({0,1,2,3}), "Youth age limit or short pool");
        require(call_up_national_squad(s, senior) && roster(c, senior) == std::set<unsigned>({0,1,2,3,4}), "Senior short pool");
        for (unsigned id = 0; id < 5; ++id) c.put8(staff(id) + 0x62, 0);
        require(call_up_national_squad(s, senior) && roster(c, senior).empty(), "Empty pool retained old call-ups");
        c.put8(staff(35) + 0x62, 11); c.put32(staff(35) + 0x18, dislikes);
        c.put32(dislikes + 0x10, foreign_team);
        require(call_up_national_squad(s, senior) && roster(c, senior).empty(), "Primary-nation refusal bypassed");
        c.put32(dislikes + 0x10, senior); c.put32(staff(35) + 0x24, 0);
        require(call_up_national_squad(s, senior) && roster(c, senior) == std::set<unsigned>({35}),
                "Eligible dual national/free agent was rejected");
        const auto special = c.u32(c.toc(-0x79fc)); c.put32(c.toc(-0x79fc), c.u32(country + 0x68));
        require(call_up_national_squad(s, senior) && roster(c, senior).empty(), "Original special-nationality rule bypassed");
        c.put32(c.toc(-0x79fc), special); c.put8(staff(35) + 0x62, 0);
        c.put8(staff(33) + 0x62, 11);
        // Compare the read-only injury interval with the game's actual interval
        // operation, including equal/future dates, year borrowing and leap years.
        const auto result = invoke(s, 0x10773520, {8});
        for (const auto dates : std::array<std::array<unsigned,4>,8>{{
                 {100,2001,100,2001}, {100,2001,86,2001}, {100,2001,87,2001}, {100,2001,114,2001},
                 {3,2004,350,2003}, {3,2003,350,2002}, {350,2003,3,2004}, {100,2005,100,2001}}}) {
            c.put16(today, dates[0]); c.put16(today + 2, dates[1]);
            c.put16(injury + 0x28, dates[2]); c.put16(injury + 0x2a, dates[3]);
            invoke(s, 0x10717dd0, {result, today, c.u32(injury + 0x28), c.u32(injury + 0x2c)});
            const int elapsed = int(c.u16(result + 2)) * 365 + c.i16(result);
            for (const auto threshold : {elapsed, elapsed + 1}) {
                c.put16(injury + 0x30, threshold); c.put16(injury + 0x32, threshold);
                require(call_up_national_squad(s, senior), "Injury-date case failed");
                require(roster(c, senior) == (elapsed >= threshold ? std::set<unsigned>({33}) : std::set<unsigned>{}),
                        "Injury eligibility differs from original date operation at its boundary");
            }
        }
        require(random == rng(s), "Refresh consumed game RNG");
        require(debug_text(9, DebugText::call_up) == "Convocar" && debug_text(255, DebugText::call_up) == "Call Up", "Call-up labels");
        for (unsigned language = 0; language < 11; ++language) require(!debug_text(language, DebugText::call_up).empty(), "Missing translation");
        std::cout << "National call-up: top 23, ties, eligibility, promotion, unchanged incumbents, original manual bytes, "
                     "tactics cleanup, repeat no-op, current ability, short/empty pool, youth age, caller and RNG passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
