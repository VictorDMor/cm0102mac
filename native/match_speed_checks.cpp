#include "match_speed.hpp"
#include "match_presentation.hpp"
#include "semantic_io_checks.hpp"
#include "human_manager_adapter.hpp"
#include "turbo_presentation.hpp"
#include <array>
#include <utility>

using namespace cm::semantic_io_checks;
namespace {
void instruction(State& s, std::uint32_t pc) {
    s.pc = pc;
    s.budget = s.executed + 1;
    try { require(dispatch_page(s), "Missing original speed instruction"); }
    catch (const std::runtime_error& e) {
        require(std::string_view(e.what()) == "Native instruction budget exhausted", e.what());
    }
}
// Runs the loader's instruction after its read of the speed field: `stored`
// with a neighbour on each side, r0 and r3 as the loader has them.
std::uint32_t loaded_speed(State& s, std::uint32_t stored) {
    s.memory.write<2>(fixture + 0x1fe, 0xa5a5);
    s.memory.write<2>(fixture + 0x200, stored);
    s.memory.write<2>(fixture + 0x202, 0x5a5a);
    s.wr<4>(0, fixture);
    s.wr<4>(12, 0x202);
    instruction(s, 0x1014a854);
    require(s.r<4>(12) == fixture + 0x202 && s.memory.read<2>(fixture + 0x1fe) == 0xa5a5 &&
            s.memory.read<2>(fixture + 0x202) == 0x5a5a, "Career loader's next field or a neighbour changed");
    return s.memory.read<2>(fixture + 0x200);
}

// One human whose record the speed accessors reach: the game record's current
// human, its staff record and a table of one record, as HUMAN_MANAGER.CPP reads them.
constexpr std::uint32_t human_table = fixture + 0x1000, game_record = fixture + 0x1100, staff_record = fixture + 0x1200,
    staff_total = fixture + 0x1300, success_flag = fixture + 0x1400, message = fixture + 0x1500, pool = fixture + 0x1600,
    records = fixture + 0x2000, speed_field = records + 0x9a66, call_stack = stack + 0x80;
void human_fixture(State& s) {
    s.memory.map(toc - 0x8000, 0x10000);
    s.memory.map(stack - 0x1000, 0x2000);
    s.wr<4>(8, toc);
    for (const auto& [cell, value] : std::initializer_list<std::pair<std::int32_t, std::uint32_t>>{
             {-0x7a54, human_table}, {-0x7bb0, game_record}, {-0x7be4, staff_total}, {-0x7bac, success_flag},
             {-0x7ba8, message}, {-0x54a0, pool}})
        s.memory.write<4>(toc + cell, value);
    s.memory.write<4>(human_table, records);          // the records
    s.memory.write<4>(human_table + 4, 1);            // one of them
    s.memory.write<2>(game_record + 0x2c58, 0);       // human 0 is in play
    s.memory.write<4>(game_record + 4, staff_record); // with this staff record
    s.memory.write<4>(staff_record, 84);              // whose id less the staff count plus 16 is record 0
    s.memory.write<4>(staff_total, 100);
}
// Runs the speed accessor at `entry` for the current human, as the match screen calls it.
void call_accessor(State& s, std::uint32_t entry, std::uint32_t value) {
    s.wr<4>(4, call_stack);
    s.wr<4>(12, human_table);
    s.wr<4>(16, entry == 0x10147bc0 ? 0 : value);
    s.wr<4>(20, 0);
    s.wr<4>(4128, return_sentinel);
    s.pc = entry;
    run_until(s, return_sentinel);
}
// The menu marks an entry when it is the saved selection: Turbo by the preference,
// each original speed by the getter's answer. Exactly one may be marked.
void require_single_mark(State& s, const char* when) {
    call_accessor(s, 0x10147bc0, 0);
    const auto getter = std::uint32_t(s.r<4>(12));
    unsigned marked = native_match_speed() == turbo_match_speed ? 1 : 0;
    for (const auto speed : {20u, 55u, 90u, 125u, 150u}) marked += getter == speed;
    require(marked == 1, std::string("Speed menu does not mark exactly one entry ") + when);
}
}
int main() {
    try {
        auto state = std::make_unique<State>();
        auto& s = *state;
        s.memory.map(fixture, 0x10000);
        require(!native_match_speed_enabled(), "Headless reference unexpectedly enables presentation changes");
        require(match_simulation_speed(0) == 0, "Original zero speed changed outside the native UI");
        s.wr<4>(12, fixture + 0x659a);
        s.wr<4>(16, 0);
        instruction(s, 0x10147e50);
        require(s.memory.read<2>(fixture) == 0 && loaded_speed(s, 0) == 0,
                "The record's speed changed outside the native UI");
        initialize_native_match_speed();
        require(native_match_speed() == 0 && match_turbo_enabled(), "Native default is not Turbo");
        for (auto choice : {0u, 20u, 55u, 90u, 125u, 150u}) {
            select_native_match_speed(choice);
            // Exercise the generated getter, rather than just the preference.
            s.memory.write<2>(fixture, 125);
            s.wr<4>(12, fixture + 0x659a);
            instruction(s, 0x10147cf8);
            require(s.r<4>(12) == choice && s.memory.read<2>(fixture) == 125,
                    "Speed getter did not select the native preference");
            s.wr<4>(12, fixture);
            s.wr<4>(20, 125);
            instruction(s, 0x101fefc8);
            require(s.memory.read<1>(fixture + 0x4070) == choice && s.r<4>(20) == 125,
                    "New commentary object did not inherit the speed");
            s.wr<4>(92, fixture);
            instruction(s, 0x10203574);
            require(s.r<4>(0) == (choice == 0 ? 20 : choice), "Original cadence normalization failed");
            s.wr<4>(92, fixture);
            instruction(s, 0x102036ac);
            require(s.r<4>(20) == (choice == 0 ? 20 : choice), "Commentary cadence normalization failed");
            require(commentary_presentation_duration(s, fixture, 300) == (choice == 0 ? 20 : 300),
                    "Turbo changed an original speed's presentation delay");
            // Successful original setter is the only preference update edge.
            const auto next = choice == 150 ? 0u : 150u;
            s.wr<4>(12, fixture + 0x659a);
            s.wr<4>(16, next);
            instruction(s, 0x10147e50);
            // Turbo is the preference; the record, which is saved, holds 20.
            require(s.memory.read<2>(fixture) == (next == 0 ? 20 : next) && native_match_speed() == next,
                    "Original choice did not update the native speed or put Turbo's zero in the record");
        }
        // A career saved with Turbo's zero in the record gets an original
        // speed when it is loaded; every other value stays as read.
        for (const auto stored : {0u, 1u, 20u, 55u, 90u, 125u, 150u, 0xffffu}) {
            require(loaded_speed(s, stored) == (stored == 0 ? 20 : stored), "Career loader left or changed the wrong speed");
        }
        select_native_match_speed(55);
        select_native_match_speed(65535);
        require(native_match_speed() == 55, "Invalid speed replaced the preference");
        // The original menu selects each option by storing its predecessor.
        // Turbo is constructed immediately before the first original option.
        constexpr std::uint32_t page = 0x31000000;
        s.memory.map(page, 0x130000);
        s.memory.write<2>(fixture + 0x100, 0);
        s.memory.write<4>(fixture + 0x104, page);
        s.wr<4>(80, fixture + 0x100);
        s.wr<4>(76, fixture + 0x104);
        s.wr<4>(68, 11); s.wr<4>(56, 12); s.wr<4>(84, 13);
        s.wr<4>(88, 14); s.wr<4>(52, 15);
        for (const auto& [choice, previous] : std::array<std::pair<unsigned, unsigned>, 6>{{
                 {0, 15}, {20, 10}, {55, 11}, {90, 12}, {125, 13}, {150, 14}}}) {
            select_native_match_speed(choice);
            require(finish_turbo_match_speed_menu(s) && s.memory.read<2>(page + 0x12a902) == previous &&
                    s.memory.read<1>(page + 0x12a901) == 0x20 && s.pc == 0x1028d660,
                    "Speed menu selected the wrong predecessor");
        }
        select_native_match_speed(55);
        s.wr<4>(12, 0x8000659a);
        s.wr<4>(16, 20);
        s.pc = 0x10147e50;
        s.budget = s.executed + 2;
        bool fault = false;
        try { dispatch_page(s); } catch (const std::runtime_error&) { fault = true; }
        require(fault && native_match_speed() == 55, "Faulting original setter persisted a choice");
        // The record's speed accessors are HUMAN_MANAGER.CPP's, which skips the hooks
        // above: the generated entry applies the native choice around the rewrite.
        human_fixture(s);
        initialize_native_match_speed();
        s.memory.write<2>(speed_field, 125);
        require_single_mark(s, "after loading a career"); for (const auto from : {0u, 20u, 55u, 90u, 125u, 150u}) {
            for (const auto to : {0u, 20u, 55u, 90u, 125u, 150u}) {
                select_native_match_speed(std::uint16_t(from));
                call_accessor(s, 0x10147d20, to);
                require(native_match_speed() == to, "Setter through the rewrite did not update the preference");
                require(s.memory.read<2>(speed_field) == (to == 0 ? 20 : to), "Rewrite's record does not hold an original speed");
                require_single_mark(s, "after choosing a speed");
            }
        }
        // A value the menu never gives reaches the record, not the preference.
        select_native_match_speed(90);
        call_accessor(s, 0x10147d20, 77);
        require(native_match_speed() == 90 && s.memory.read<2>(speed_field) == 77, "Unlisted speed reached the preference");
        // The wrapper acts only when the human's record exists, as the original's hooks
        // do (a missing record reports and returns; the report needs the platform, which
        // a check does not have, so the lookup is checked on its own).
        require(human_record_found(s, human_table, 0) && human_record_found(s, human_table, staff_record),
                "Existing record not found");
        s.memory.write<4>(human_table + 4, 0);
        require(!human_record_found(s, human_table, 0) && !human_record_found(s, human_table, staff_record),
                "Missing record found");
        s.memory.write<4>(human_table + 4, 1);
        s.memory.write<4>(game_record + 4, 0);
        require(!human_record_found(s, human_table, 0) && human_record_found(s, human_table, staff_record),
                "No current human found");
        s.memory.write<4>(game_record + 4, staff_record);
        // A rewrite that declines leaves the original body (and its hooks) to run.
        select_native_match_speed(90);
        s.wr<4>(12, human_table);
        s.wr<4>(16, 55);
        s.wr<4>(20, 0);
        {
            struct Declining { Declining() { human_manager_declining = true; } ~Declining() { human_manager_declining = false; } } declining;
            s.wr<4>(16, 0);
            const bool getter_declined = !native_match_speed_through_human_manager(s, 0x10147bc0);
            s.wr<4>(16, 55);
            require(getter_declined && !native_match_speed_through_human_manager(s, 0x10147d20) &&
                    native_match_speed() == 90 && s.r<4>(16) == 55, "Declined accessor changed the preference or a register");
        }
        // Turbo's presentation pacing: frames are deferred only on Turbo's match screen, a
        // deferred frame falls due, and the game counts as animating only while it draws.
        {
            using ms = std::chrono::milliseconds;
            const auto t0 = TurboPresentation::Clock::now();
            TurboPresentation off;
            for (unsigned i = 0; i < 5; ++i)
                require(off.request(t0 + ms(i), false) && !off.deferred() && !off.animating(t0 + ms(i), false),
                        "A frame off Turbo's match screen was deferred");
            TurboPresentation turbo;
            require(turbo.request(t0, true), "The first Turbo frame was deferred");
            require(!turbo.request(t0 + ms(10), true) && turbo.deferred(), "A Turbo frame inside the interval was presented");
            require(!turbo.due(t0 + ms(20), true, false) && turbo.due(t0 + ms(40), true, false),
                    "A deferred frame is due at the wrong time");
            require(turbo.due(t0 + ms(11), true, true) && turbo.due(t0 + ms(11), false, false),
                    "A deferred frame is not due before a wait or after leaving the screen");
            turbo.presented(t0 + ms(40));
            require(!turbo.deferred() && !turbo.due(t0 + ms(41), true, true), "A presented frame is still deferred");
            require(turbo.animating(t0 + ms(99), true) && !turbo.animating(t0 + ms(111), true) && !turbo.animating(t0 + ms(99), false),
                    "Animation window is wrong");
            require(turbo.request(t0 + ms(80), false) && !turbo.deferred(), "Leaving the Turbo screen did not present at once");
            require(!turbo.request(t0 + ms(85), true) && turbo.deferred(), "A frame after a presentation was not deferred");
            turbo.discard();
            require(!turbo.deferred() && !turbo.due(t0 + ms(200), true, true), "A discarded frame is still due");
        }
        std::cout << "PASS: six native choices, Turbo default, original generated getter/setter, record kept original, "
                     "constructor, cadence, presentation isolation, fault-safe preference, one marked menu entry through the rewritten accessors and Turbo presentation pacing\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
