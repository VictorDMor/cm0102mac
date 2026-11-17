#include "match_phase_timing.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
using namespace cm::aot::match_phase_timing;
namespace {
std::uint64_t tick = 0;
std::uint64_t now() { return tick += 10; }
void require(bool value) { if (!value) throw std::runtime_error("Match phase timing contract failed"); }
const Total& total(const Recorder& recorder, Phase phase) { return recorder.totals()[static_cast<unsigned>(phase)]; }
}
int main() {
    try {
        { Scope disabled(Phase::advance); disabled.next(Phase::reports); }
        require(tick == 0 && !active);
        {
            Recorder recorder(now);
            { OptionalScope<false> erased(Phase::advance); erased.next(Phase::reports); }
            require(tick == 0 && total(recorder, Phase::advance).calls == 0);
            {
                OptionalScope<true> outer(Phase::advance);
                { Scope phases(Phase::clock_and_restarts); phases.next(Phase::marking); }
            }
            require(total(recorder, Phase::advance).calls == 1 && total(recorder, Phase::advance).nanoseconds == 40);
            require(total(recorder, Phase::clock_and_restarts).nanoseconds == 10 && total(recorder, Phase::marking).nanoseconds == 10);
            {
                Recorder nested(now);
                { Scope report(Phase::reports); }
                require(total(nested, Phase::reports).calls == 1 && total(nested, Phase::reports).nanoseconds == 10);
            }
            require(active == &recorder && total(recorder, Phase::reports).calls == 0);
            try { Scope throwing(Phase::actor_turns); throw 1; } catch (int) {}
            require(total(recorder, Phase::actor_turns).calls == 1 && total(recorder, Phase::actor_turns).nanoseconds == 10);
            bool isolated = false;
            std::thread other([&] { isolated = active == nullptr; Scope disabled(Phase::reports); });
            other.join();
            require(isolated && total(recorder, Phase::reports).calls == 0);
            std::ostringstream output;
            recorder.report(output);
            require(output.str().find("match_phase advance calls=1 ns=40\n") != std::string::npos);
        }
        require(!active);
        std::cout << "Erased scopes, disabled clocks, sequential phases, inclusive parent, nested recording, exception cleanup and thread isolation passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
