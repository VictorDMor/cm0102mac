#include "aot_runtime.hpp"
#include "code_fragment_adapter.hpp"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>
#include <unistd.h>

namespace {
using namespace cm::aot;
constexpr std::uint32_t service = 0x10bf7c34;
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
struct Capture {
    FILE* file = std::tmpfile();
    int previous = -1;
    Capture() {
        require(file != nullptr, "Cannot create private trace capture");
        std::fflush(stderr);
        previous = dup(STDERR_FILENO);
        require(previous >= 0 && dup2(fileno(file), STDERR_FILENO) >= 0, "Cannot capture trace output");
    }
    ~Capture() {
        std::fflush(stderr);
        if (previous >= 0) { dup2(previous, STDERR_FILENO); close(previous); }
        if (file) std::fclose(file);
    }
    std::string text() {
        std::fflush(stderr);
        require(std::fseek(file, 0, SEEK_SET) == 0, "Cannot read trace capture");
        std::string result;
        for (int c; (c = std::fgetc(file)) != EOF;) result.push_back(char(c));
        return result;
    }
};
void prepare(State& s, unsigned argument) {
    s.pc = service; s.wr<4>(4128, return_sentinel); s.wr<4>(12, argument);
}
struct Inputs final : CodeFragmentBoundaryObserver {
    State* peer = nullptr;
    unsigned enters = 0, leaves = 0, fixtures = 0;
    bool fail = false;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State&, const char*) override { ++enters; }
    void import_leave(const State&, const char*) override { ++leaves; }
    bool import_fixture(State& s, const char* name) override {
        require(std::string_view(name) == "CarbonLib::Microseconds", "Unexpected traced import");
        ++fixtures;
        if (fail) throw std::runtime_error("fixture failure");
        if (peer) { prepare(*peer, 2); run_until(*peer, return_sentinel); }
        s.wr<4>(12, s.r<4>(12) + 10); s.pc = s.r<4>(4128);
        return true;
    }
};
std::string call_line(unsigned argument) {
    return "CarbonLib::Microseconds pc=10bf7c34 lr=" + hex_address(return_sentinel).substr(2) +
        " r3=" + hex_address(argument).substr(2) +
        " r4=00000000 r5=00000000 r6=00000000 r7=00000000 r8=00000000 r9=00000000 r10=00000000\n";
}
std::string return_line(unsigned result) {
    return "CarbonLib::Microseconds returned r3=" + hex_address(result).substr(2) + "\n";
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Expected enabled or disabled trace policy");
        const bool enabled = std::string_view(argv[1]) == "enabled";
        require(enabled || std::string_view(argv[1]) == "disabled", "Unknown policy");
        require((std::getenv("CM_AOT_TRACE_IMPORTS") != nullptr) == enabled, "Incorrect initial test environment");
        require(std::string_view(import_name(service)) == "CarbonLib::Microseconds", "Incorrect import address");
        auto state = std::make_unique<State>(), peer = std::make_unique<State>();
        Inputs outer, inner; outer.peer = peer.get();
        state->code_fragment_observer = &outer; peer->code_fragment_observer = &inner;
        // Even an empty first dispatch captures the launch policy. Changing the
        // host environment later cannot split tracing between nested states.
        state->pc = return_sentinel; run_until(*state, return_sentinel);
        require((enabled ? unsetenv("CM_AOT_TRACE_IMPORTS") : setenv("CM_AOT_TRACE_IMPORTS", "1", 1)) == 0,
                "Cannot vary host environment after first dispatch");
        std::string output;
        {
            Capture capture;
            prepare(*state, 1); run_until(*state, return_sentinel);
            require(state->r<4>(12) == 11 && peer->r<4>(12) == 12 &&
                    state->native_calls == 1 && peer->native_calls == 1,
                    "Tracing altered nested import results or accounting");
            outer.peer = nullptr; outer.fail = true;
            prepare(*state, 3);
            bool failed = false;
            try { run_until(*state, return_sentinel); }
            catch (const std::runtime_error& e) { failed = std::string_view(e.what()) == "fixture failure"; }
            require(failed && state->pc == service && state->native_calls == 1,
                    "Tracing altered an import exception");
            outer.fail = false; prepare(*state, 4); run_until(*state, return_sentinel);
            output = capture.text();
        }
        const auto expected = call_line(1) + call_line(2) + return_line(12) + return_line(11) +
            call_line(3) + call_line(4) + return_line(14);
        require(output == (enabled ? expected : ""), "Import trace policy, text or nested ordering differs");
        require(outer.enters == 3 && outer.leaves == 2 && outer.fixtures == 3 &&
                inner.enters == 1 && inner.leaves == 1 && inner.fixtures == 1 &&
                state->native_calls == 2 && state->r<4>(12) == 14,
                "Import observer order or recovery differs");
        std::cout << "Import trace " << (enabled ? "enabled" : "disabled")
                  << ": nested states, exact output, exception recovery and stable process policy pass\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
