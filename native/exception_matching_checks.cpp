#include "exception_runtime_context.hpp"
#include "semantic_io_checks.hpp"
#include <set>

using namespace cm::semantic_io_checks;
namespace cm::aot { bool exception_context_reference(State&); }
namespace {
constexpr std::uint32_t actual = fixture + 0x100, expected = fixture + 0x200,
    output = fixture + 0x300, record = fixture + 0x400;
unsigned comparisons = 0, faults = 0, negatives = 0;
using Operation = void(*)(State&);
struct Monitor final : MemoryObserver {
    Effects effects;
    std::function<void(State&, Monitor&, std::uint32_t, std::size_t)> on_write;
    void mapped(std::uint32_t a, std::uint64_t n) override { effects.mapped(a, n); }
    void written(std::uint32_t a, const std::uint8_t* data, std::size_t n) override {
        effects.written(a, data, n); if (on_write) on_write(*effects.state, *this, a, n);
    }
    void cleared(std::uint32_t a, std::uint64_t n) override { effects.cleared(a, n); }
    void read(std::uint32_t a, const std::uint8_t* data, unsigned n) override { effects.read(a, data, n); }
};
using ReadMutation = decltype(Effects::on_read);
using WriteMutation = decltype(Monitor::on_write);
using Expect = std::function<void(const State&, const Effects&)>;
std::unique_ptr<State> setup(const char* program, unsigned seed = 0) {
    auto s = std::make_unique<State>(); load_program(*s, program);
    s->memory.copy_in(stack - 8192, std::vector<std::uint8_t>(8256, 0xa7));
    s->registers.fill(0x5b); s->unique.fill(0x6d);
    for (unsigned n = 0; n < 32; ++n) s->wr<4>(n * 4, 0xcafe0000 + n);
    s->wr<4>(4, stack); s->wr<4>(8, toc); s->wr<4>(4128, return_sentinel); s->wr<4>(4132, 0x31415926);
    s->wr<1>(2304, seed % 2 ? 0xf9 : 0xa6); s->wr<1>(1024, seed % 4); s->wr<1>(1027, (seed / 2) % 2);
    s->budget = 20000; return s;
}
void text(State& s, std::uint32_t address, const std::string& value, bool terminator = true, unsigned extra = 0) {
    std::vector<std::uint8_t> bytes(value.begin(), value.end()); if (terminator) bytes.push_back(0);
    bytes.resize(std::max(bytes.size(), std::size_t(extra)), 0xa5);
    s.memory.map(address, bytes.size()); s.memory.copy_in(address, bytes);
}
std::unique_ptr<State> type_fixture(const char* program, const std::string& from, const std::string& to,
        bool wildcard = false, unsigned seed = 0) {
    auto s = setup(program, seed); text(*s, actual, from); if (!wildcard) text(*s, expected, to);
    s->memory.map(output, 4); s->memory.write<4>(output, 0xaabbccdd);
    s->wr<4>(12, actual); s->wr<4>(16, wildcard ? 0 : expected); s->wr<4>(20, output); s->pc = 0x10772f10; return s;
}
std::unique_ptr<State> specification_fixture(const char* program, const std::string& from,
        const std::vector<std::optional<std::string>>& types, unsigned count, unsigned seed = 0) {
    auto s = setup(program, seed); text(*s, actual, from);
    s->memory.map(record, 12 + types.size() * 4); s->memory.clear(record, 12 + types.size() * 4);
    s->memory.write<2>(record + 2, count);
    for (unsigned n = 0; n < types.size(); ++n) {
        const auto address = fixture + 0x1000 + n * 0x100;
        if (types[n]) text(*s, address, *types[n]);
        s->memory.write<4>(record + 12 + n * 4, types[n] ? address : 0);
    }
    s->wr<4>(12, actual); s->wr<4>(16, record); s->pc = 0x10772430; return s;
}
std::unique_ptr<State> without_byte(const State& initial, std::uint32_t omitted) {
    auto s = std::make_unique<State>(); s->registers = initial.registers; s->unique = initial.unique; s->pc = initial.pc;
    s->budget = initial.budget; s->heap_end = initial.heap_end; s->memory_error = initial.memory_error;
    s->allocated = initial.allocated; s->available = initial.available;
    const auto copy = [&](std::uint32_t address, std::uint64_t size) {
        if (!size) return;
        s->memory.map(address, size); s->memory.copy_in(address, initial.memory.copy_out(address, size));
    };
    for (const auto& [address, size] : initial.memory.mapped_ranges()) {
        if (omitted >= address && std::uint64_t(omitted) < std::uint64_t(address) + size) {
            copy(address, omitted - address); const auto remaining = std::uint64_t(address) + size - omitted - 1;
            if (remaining) copy(omitted + 1, remaining);
        } else copy(address, size);
    }
    return s;
}
void reference_until(State& s) {
    while (s.pc != return_sentinel) {
        if (!cm::aot::exception_context_reference(s)) run_until(s, std::uint32_t(s.r<4>(4128)));
    }
}
Effects check(std::unique_ptr<State> original, Operation operation, const std::string& label,
        const Expect& expect = {}, const ReadMutation& read = {}, const WriteMutation& write = {},
        bool fault = false, bool negative = false) {
    auto semantic = clone(*original); Monitor a, b; a.effects.state = original.get(); b.effects.state = semantic.get();
    a.effects.on_read = b.effects.on_read = read; a.on_write = b.on_write = write;
    original->memory.date_observer = &a; semantic->memory.date_observer = &b;
    std::string x, y;
    try { reference_until(*original); } catch (const std::runtime_error& e) { x = e.what(); }
    try { operation(*semantic); } catch (const std::runtime_error& e) { y = e.what(); }
    original->memory.date_observer = semantic->memory.date_observer = nullptr;
    require(x == y, label + " outcome original=" + x + " semantic=" + y);
    require(fault == !x.empty(), label + " fault expectation " + x);
    if (fault) require(x.starts_with("Unmapped data access at "), label + " mapping fault");
    Image image(*original); image.compare(*semantic, label);
    require(a.effects.bytes == b.effects.bytes, label + " ordered access prefix");
    if (expect) expect(*semantic, b.effects);
    if (negative) {
        semantic->wr<4>(12, semantic->r<4>(12) ^ 1); bool rejected = false;
        try { image.compare(*semantic, label + " deliberately corrupted match return"); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected, label + " negative accepted"); ++negatives;
    }
    ++comparisons; faults += fault; b.effects.state = nullptr; b.effects.on_read = {}; return b.effects;
}
Expect result(bool matched, std::uint32_t offset = 0, std::uint32_t out = output) {
    return [=](const State& s, const Effects&) {
        require(s.r<4>(12) == unsigned(matched) && s.pc == return_sentinel, "Independent type-match result");
        require(s.memory.read<4>(out) == offset, "Independent published word32 type offset");
    };
}
void type_checks(const char* program) {
    struct Case { std::string from, to; bool matches; std::uint32_t offset = 0; };
    const std::vector<Case> cases{
        {"", "", true}, {"i", "i", true}, {"i", "l", false}, {"long_type", "long_type", true}, {"A", "AB", false},
        {"Pi", "Pi", true}, {"Pi", "PCi", true}, {"PCi", "Pi", false}, {"PVi", "Pi", false}, {"Pi", "PVi", true},
        {"PCVi", "PCVi", true}, {"PVi", "PCVi", true}, {"PCi", "PCVi", true}, {"PCVi", "PCi", false},
        {"Ri", "RCi", true}, {"RCi", "Ri", false}, {"Ri", "RVi", true}, {"RVi", "Ri", false}, {"Pi", "Ri", false},
        {"PPi", "PPCVi", true}, {"PPCi", "PPi", false}, {"RPCVi", "RPCVi", true}, {"RPPi", "RCPPCVi", true},
        {"Pi", "Pv", true}, {"PCi", "Pv", true}, {"PVi", "Pv", true}, {"Pi", "PCVv", true}, {"Pi", "PCv", true},
        {"Pi", "PVv", true}, {"Ri", "Pv", false}, {"i", "Pv", false}, {"Pi", "PCCi", false}, {"PCVi", "PVCi", false},
        {"!Derived!0!Base!16!", "!Derived!", true, 0}, {"!Derived!0!Base!16!", "!Base!", true, 16},
        {"!Derived!0!Base!16!", "!Other!", false}, {"*Derived!0!Base!32!", "*Base!", true, 32},
        {"*Derived!0!Base!32!", "!Base!", false}, {"!Derived!0!Base!32!", "*Base!", false}, {"*Broken", "Pv", true},
        {"!Root!0!Middle!8!Base!48!", "!Base!", true, 48}, {"!Root!0!Middle!8!Base!48!", "!Middle!", true, 8},
        {"!Base!!", "!Base!", true, 0}, {"!Base!4294967295!", "!Base!", true, 0xffffffff},
        {"!Base!4294967296!", "!Base!", true, 0}, {"!Base!12x!", "!Base!", true, 192},
        {"!Base!-1!", "!Base!", true, 0xffffffe3}, {"!Base!\x80!", "!Base!", true, 0xffffff50},
        {std::string(1, char(0x80)), std::string(1, char(0x80)), true},
        {std::string(1, char(0x80)), std::string(1, char(0x81)), false}, {"Qraw", "Qraw", true}};
    unsigned seed = 0;
    for (const auto& c : cases) {
        const auto index = seed++;
        check(type_fixture(program, c.from, c.to, false, index), exception_runtime::match_types,
            "Encoded type case=" + std::to_string(index), result(c.matches, c.offset), {}, {}, false, index == 0);
    }
    for (const auto& from : {std::string{}, std::string("i"), std::string("!Base!16!"), std::string("PCVi")})
        check(type_fixture(program, from, "", true, seed++), exception_runtime::match_types, "Catch-all type", result(true));
    auto null_actual = type_fixture(program, "", "", true); null_actual->wr<4>(12, 0);
    check(std::move(null_actual), exception_runtime::match_types, "Catch-all does not read null actual type", result(true));
}
void alias_checks(const char* program) {
    auto shared = type_fixture(program, "!Root!123!", ""); shared->wr<4>(16, actual); shared->wr<4>(20, actual + 1);
    check(std::move(shared), exception_runtime::match_types, "Shared class encoding and overlapping offset store", result(true, 123, actual + 1));
    auto overwrite_expected = setup(program); text(*overwrite_expected, actual, ""); text(*overwrite_expected, expected, "i", true, 4);
    overwrite_expected->wr<4>(12, actual); overwrite_expected->wr<4>(16, expected); overwrite_expected->wr<4>(20, expected);
    overwrite_expected->pc = 0x10772f10;
    check(std::move(overwrite_expected), exception_runtime::match_types, "Output reset aliases expected encoding", result(true, 0, expected));
    auto unaligned = setup(program); text(*unaligned, actual, "!Base!24!"); text(*unaligned, expected, "!Base!");
    unaligned->memory.map(output, 5); unaligned->memory.clear(output, 5);
    unaligned->wr<4>(12, actual); unaligned->wr<4>(16, expected); unaligned->wr<4>(20, output + 1); unaligned->pc = 0x10772f10;
    check(std::move(unaligned), exception_runtime::match_types, "Unaligned result word", result(true, 24, output + 1));
    auto digit = type_fixture(program, "!Base!12!", "!Base!");
    check(std::move(digit), exception_runtime::match_types, "Offset digit reread after probe", result(true, 92),
        [](State& s, Effects& e, std::uint32_t address, unsigned) {
            if (s.pc == 0x10773004 && address == actual + 6 && !e.mutated) {
                e.mutated = true; s.memory.write<1>(address, '9');
            }
        });
}
void specification_checks(const char* program) {
    struct Case { std::string from; std::vector<std::optional<std::string>> list; unsigned count; bool matches; };
    const std::vector<Case> cases{{"i", {}, 0, false}, {"i", {"i"}, 0, false}, {"i", {"i"}, 1, true},
        {"i", {"l", "i"}, 2, true}, {"i", {"l", "i"}, 1, false}, {"i", {"l", "f"}, 2, false},
        {"i", {"l", std::nullopt}, 2, true}, {"Pi", {"PVv"}, 1, true},
        {"!Derived!0!Base!16!", {"!Other!", "!Base!"}, 2, true}};
    unsigned seed = 0;
    for (const auto& c : cases) {
        const auto index = seed++;
        check(specification_fixture(program, c.from, c.list, c.count, index), exception_runtime::matches_specification,
            "Exception specification case=" + std::to_string(index), [=](const State& s, const Effects&) {
                require(s.r<4>(12) == unsigned(c.matches) && s.r<4>(4) == stack && s.pc == return_sentinel, "Independent specification result and restored frame");
            }, {}, {}, false, index == 0);
    }
    for (unsigned count : {0u, 2u}) {
        auto s = specification_fixture(program, "i", {"l", "i"}, 1);
        check(std::move(s), exception_runtime::matches_specification, "Live specification count becomes " + std::to_string(count),
            [=](const State& state, const Effects& effects) {
                require(effects.mutated && state.r<4>(12) == unsigned(count == 2) && state.memory.read<2>(record + 2) == count, "Live specification count affects next iteration");
            }, {}, [=](State& state, Monitor& monitor, std::uint32_t, std::size_t) {
                if (state.pc == 0x10772f18 && !monitor.effects.mutated) { monitor.effects.mutated = true; state.memory.write<2>(record + 2, count); }
            });
    }
    auto entry = specification_fixture(program, "i", {"l"}, 1);
    check(std::move(entry), exception_runtime::matches_specification, "Live specification entry becomes catch-all",
        [](const State& s, const Effects& e) { require(e.mutated && s.r<4>(12) == 1, "Live specification type pointer"); },
        [](State& s, Effects& e, std::uint32_t address, unsigned) {
            if (s.pc == 0x10772468 && !e.mutated) { e.mutated = true; s.memory.write<4>(address, 0); }
        });
}
void malformed_checks(const char* program) {
    for (const auto& [from, to] : std::vector<std::pair<std::string,std::string>>{
            {"!Base!12", "!Base!"}, {"!Broken!", "!Other!"}, {"*Broken", "*Other!"}, {"!", "!"}})
        check(type_fixture(program, from, to), exception_runtime::match_types, "Truncated class metadata mapping", {}, {}, {}, true);
    auto unterminated = setup(program); text(*unterminated, actual, "abc", false); text(*unterminated, expected, "abc");
    unterminated->memory.map(output, 4); unterminated->wr<4>(12, actual); unterminated->wr<4>(16, expected);
    unterminated->wr<4>(20, output); unterminated->pc = 0x10772f10;
    check(std::move(unterminated), exception_runtime::match_types, "Flat encoding without terminator", {}, {}, {}, true);
    auto null_actual = type_fixture(program, "i", "i"); null_actual->wr<4>(12, 0);
    check(std::move(null_actual), exception_runtime::match_types, "Null actual with concrete expected type", {}, {}, {}, true);
    auto null_out = type_fixture(program, "i", "", true); null_out->wr<4>(20, 0);
    check(std::move(null_out), exception_runtime::match_types, "Catch-all still resets null result word", {}, {}, {}, true);
}
void fault_checks(const char* program) {
    struct Witness { std::string from, to; };
    for (const auto& c : std::vector<Witness>{{"Pi", "PCVv"}, {"PCVi", "PCVi"}, {"PPi", "PPCi"},
            {"!Child!0!Base!16!", "!Base!"}, {"!Child!0!Base!16!", "!Other!"}, {"raw", "raw"}}) {
        auto s = type_fixture(program, c.from, c.to);
        const auto effects = check(clone(*s), exception_runtime::match_types, "Type access witness");
        std::set<std::uint32_t> addresses;
        for (auto access : effects.log) if (access.kind == 2 || access.kind == 4) addresses.insert(access.address + std::uint32_t(access.size) - 1);
        for (auto address : addresses) check(without_byte(*s, address), exception_runtime::match_types,
            "Type missing byte=" + hex_address(address), {}, {}, {}, true);
    }
    auto s = specification_fixture(program, "!Child!0!Base!16!", {"!Other!", "!Base!"}, 2);
    const auto effects = check(clone(*s), exception_runtime::matches_specification, "Specification access witness");
    std::set<std::uint32_t> addresses;
    for (auto access : effects.log) if (access.kind == 2 || access.kind == 4) addresses.insert(access.address + std::uint32_t(access.size) - 1);
    for (auto address : addresses) check(without_byte(*s, address), exception_runtime::matches_specification,
        "Specification missing byte=" + hex_address(address), {}, {}, {}, true);
}
void child_negative(const char* program) {
    auto original = specification_fixture(program, "!Child!0!Base!16!", {"!Base!"}, 1); auto semantic = clone(*original);
    Monitor a, b; a.effects.state = original.get(); b.effects.state = semantic.get();
    std::unique_ptr<Image> boundary; std::vector<std::uint8_t> prefix; bool rejected = false;
    a.on_write = [&](State& s, Monitor& m, std::uint32_t, std::size_t) {
        if (s.pc == 0x10773024) { boundary = std::make_unique<Image>(s); prefix = m.effects.bytes; }
    };
    original->memory.date_observer = &a; reference_until(*original); original->memory.date_observer = nullptr;
    require(bool(boundary), "Original matching child final offset-store boundary");
    b.on_write = [&](State& s, Monitor& m, std::uint32_t, std::size_t) {
        if (s.pc != 0x10773024) return;
        boundary->compare(s, "Matching child final effect before corruption"); require(prefix == m.effects.bytes, "Matching child ordered final-effect prefix");
        s.wr<4>(12, s.r<4>(12) ^ 1);
        try { boundary->compare(s, "Corrupted matching child final-effect ABI"); }
        catch (const std::runtime_error&) { rejected = true; throw std::runtime_error("Rejected matching child before specification parent resumed"); }
        throw std::runtime_error("Matching child negative accepted");
    };
    semantic->memory.date_observer = &b; std::string outcome;
    try { exception_runtime::matches_specification(*semantic); } catch (const std::runtime_error& e) { outcome = e.what(); }
    semantic->memory.date_observer = nullptr;
    require(rejected && outcome == "Rejected matching child before specification parent resumed" && b.effects.bytes == prefix,
        "Matching child negative rejects before parent effect: " + outcome); ++negatives;
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Usage: cm-exception-matching-checks <program>");
        type_checks(argv[1]); alias_checks(argv[1]); specification_checks(argv[1]); malformed_checks(argv[1]); fault_checks(argv[1]); child_negative(argv[1]);
        require(negatives == 3, "Both matching roots and the specification child need discriminating negatives");
        std::cout << comparisons << " C++ type/specification comparisons (" << faults << " mapping faults), "
                  << negatives << " own/child corruption negatives passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
