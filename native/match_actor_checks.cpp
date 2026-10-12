#include "match_actor.hpp"
#include "semantic_io_checks.hpp"
#include <set>
#include <string_view>

namespace cm::aot { bool match_actor_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace actor = cm::aot::match_actor;
constexpr std::uint32_t match = fixture + 0x1000, player = fixture + 0x18000;
struct Options {
    std::uint32_t book = 0, period = 495, keeper = 0, mode = 0, team = 0, tactics = 0, gate = 0, flag = 0;
    std::int16_t clock = 1;
    bool selected_keeper = false;
    unsigned dependency_fault = 0;
};
struct Call {
    std::uint32_t target, continuation;
    decltype(State::registers) before, after;
    bool operator==(const Call&) const = default;
};
struct Model final : actor::DependencyDispatch {
    Options options;
    bool original;
    bool child_negative = false;
    std::vector<Call> calls;
    explicit Model(Options o, bool old) : options(o), original(old) {}
    void execute_original(State& state, std::uint32_t until) {
        bool first = true;
        while (state.pc != until) {
            if (!first && (state.pc == actor::entries[1] || state.pc == actor::entries[2])) {
                call(state, state.pc, std::uint32_t(state.r<4>(4128))); continue;
            }
            first = false;
            if (!match_actor_reference(state)) call(state, state.pc, std::uint32_t(state.r<4>(4128)));
        }
    }
    void call(State& state, std::uint32_t target, std::uint32_t continuation) override {
        calls.push_back({target, continuation, state.registers, {}});
        if (target == actor::entries[1] || target == actor::entries[2]) {
            if (original) execute_original(state, continuation);
            else if (target == actor::entries[1]) actor::read_position(state);
            else actor::write_position(state);
            if (!original && child_negative && target == actor::entries[2])
                state.memory.write<1>(state.r<4>(12) + 0x19, 0);
        } else {
            for (unsigned reg = 3; reg <= 12; ++reg) state.wr<4>(reg * 4, 0x0bad0000 + reg * 37);
            state.wr<4>(0, 0xbeef0055);
            const auto outcome = target == 0x101e8900 ? options.period : target == 0x10289560 ? options.keeper :
                target == 0x101da5a0 ? options.gate : target == 0x101d3b60 ? (options.selected_keeper ? player : player + 0x400) : 0x12345678;
            state.wr<4>(12, outcome);
            state.pc = continuation;
        }
        calls.back().after = state.registers;
        if (options.dependency_fault && calls.size() == options.dependency_fault) {
            state.pc = target; throw std::runtime_error("Fixture match-actor dependency interruption");
        }
    }
};
std::unique_ptr<State> seed(const Options& o, unsigned index = 0) {
    auto s = std::make_unique<State>(); s->registers.fill(0xa5); s->unique.fill(0xb7);
    s->memory.map(fixture, 0x30000); s->memory.map(0x700fe000, 0x2000); s->memory.map(toc - 0x8000, 0x1000);
    s->wr<4>(4, stack); s->wr<4>(8, toc); s->wr<4>(4128, return_sentinel);
    s->wr<4>(12, index ? player : match); s->wr<4>(16, index ? o.book : player); s->wr<4>(20, o.book);
    s->memory.write<1>(match, o.mode); s->memory.write<1>(match + 0x1bc, o.flag); s->memory.write<2>(match + 0x7948, std::uint16_t(o.clock));
    s->memory.write<1>(player + 0x19, 8); s->memory.write<1>(player + 0x21, 0xff); s->memory.write<1>(player + 0x23, 255);
    s->memory.write<1>(player + 0x27, o.team); s->memory.write<1>(match + 0x7aac + std::uint32_t(signed_value<1>(o.team)) * 0xd8 + 0x19, o.tactics);
    s->memory.write<4>(toc - 0x7a6c, fixture + 0x400); s->memory.write<4>(fixture + 0x400, fixture + 0x500);
    s->pc = actor::entries[index]; return s;
}
unsigned comparisons = 0, interruptions = 0, negatives = 0;
std::vector<Effects::Access> compare(const State& input, const Options& options, unsigned index = 0, bool negative = false, bool child_negative = false) {
    auto original = clone(input), modern = clone(input);
    Model old(options, true), next(options, false); Effects x, y; x.state = original.get(); y.state = modern.get();
    next.child_negative = child_negative;
    original->memory.date_observer = &x; modern->memory.date_observer = &y;
    const auto execute = [&](State& state, Model& model, bool previous) {
        try {
            if (previous) model.execute_original(state, return_sentinel);
            else if (index == 0) actor::dismiss_player(state, &model);
            else if (index == 1) actor::read_position(state);
            else actor::write_position(state);
        } catch (const std::exception& error) { return std::string(error.what()); }
        return std::string{};
    };
    const auto expected = execute(*original, old, true), actual = execute(*modern, next, false);
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    const auto label = hex_address(actor::entries[index]) + " component " + std::to_string(comparisons);
    require(expected == actual, label + " original=" + expected + " modern=" + actual);
    if (negative) modern->wr<4>(12, modern->r<4>(12) ^ 1);
    bool rejected = false;
    try { Image(*original).compare(*modern, label); } catch (const std::runtime_error&) { rejected = true; if (!negative && !child_negative) throw; }
    require(rejected == (negative || child_negative), "Own/child negative must be observable");
    if (!negative && !child_negative) {
        require(x.bytes == y.bytes, label + " ordered effects");
        if (old.calls != next.calls) {
            for (unsigned n = 0; n < std::min(old.calls.size(), next.calls.size()); ++n) {
                if (old.calls[n] != next.calls[n]) {
                    const auto& first = old.calls[n]; const auto& second = next.calls[n];
                    const auto before = std::mismatch(first.before.begin(), first.before.end(), second.before.begin());
                    const auto after = std::mismatch(first.after.begin(), first.after.end(), second.after.begin());
                    throw std::runtime_error(label + " dependency " + std::to_string(n) + " target=" + hex_address(first.target) + "/" + hex_address(second.target) +
                        " continuation=" + hex_address(first.continuation) + "/" + hex_address(second.continuation) +
                        " before-byte=" + std::to_string(before.first - first.before.begin()) + " after-byte=" + std::to_string(after.first - first.after.begin()));
                }
            }
            throw std::runtime_error(label + " dependency count");
        }
        require(original->executed == modern->executed, label + " accounting original=" + std::to_string(original->executed) + " modern=" + std::to_string(modern->executed));
    }
    ++comparisons; interruptions += !expected.empty(); negatives += negative || child_negative;
    return x.log;
}
}
int main() {
    try {
        const Options standard{}; auto initial = seed(standard);
        const auto accesses = compare(*initial, standard);
        for (auto book : {0u, 1u, 255u, 0x100u})
            for (auto mode : {0u, 1u}) for (auto keeper : {0u, 1u}) for (auto tactics : {0u, 1u, 255u})
                for (bool selected : {false, true}) {
                    Options options; options.book = book; options.mode = mode; options.keeper = keeper;
                    options.tactics = tactics; options.selected_keeper = selected; compare(*seed(options), options);
                }
        for (auto period : {495u, 990u, 1155u, 1320u, 0u, 989u, 1156u, 0xbeef01efu})
            for (auto clock : {-32768, -1, 0, 10, 11, 495, 506, 990, 1155, 1320, 2000}) {
                Options options; options.period = period; options.clock = std::int16_t(clock); compare(*seed(options), options);
            }
        for (auto team : {0u, 1u, 2u, 128u, 255u}) { Options options; options.team = team; compare(*seed(options), options); }
        for (auto gate : {0u, 1u, 255u}) for (auto flag : {0u, 1u, 255u}) {
            Options options; options.gate = gate; options.flag = flag; compare(*seed(options), options);
        }
        for (unsigned budget = 0; budget <= 220; ++budget) { auto limited = seed(standard); limited->budget = budget; compare(*limited, standard); }
        for (unsigned fault = 1; fault <= 13; ++fault) { Options options; options.dependency_fault = fault; compare(*seed(options), options); }
        // Every byte read or written by the original body independently becomes
        // an unmapped fault; no neighboring mapped bytes can mask that hole.
        std::set<std::uint32_t> bytes;
        for (auto access : accesses) for (unsigned n = 0; n < access.size; ++n) bytes.insert(access.address + n);
        for (auto hole : bytes) {
            auto sparse = std::make_unique<State>(); sparse->registers = initial->registers; sparse->unique = initial->unique; sparse->pc = initial->pc;
            for (auto address : bytes) if (address != hole) { sparse->memory.map(address, 1); sparse->memory.write<1>(address, initial->memory.read<1>(address)); }
            compare(*sparse, standard);
        }
        for (unsigned index = 1; index < 3; ++index) {
            for (auto value : {0u, 1u, 11u, 127u, 128u, 253u, 255u, 0xabcdefedu}) {
                auto leaf = seed(standard, index); leaf->wr<4>(16, value); leaf->memory.write<1>(player + 0x19, value); compare(*leaf, standard, index);
                for (unsigned budget = 0; budget <= 2; ++budget) { leaf->budget = budget; compare(*leaf, standard, index); }
            }
            auto invalid = seed(standard, index); invalid->wr<4>(12, 0); compare(*invalid, standard, index);
            compare(*seed(standard, index), standard, index, true);
        }
        compare(*initial, standard, 0, true);
        compare(*initial, standard, 0, false, true);
        auto aliased = seed(standard); aliased->wr<4>(12, player);
        aliased->memory.write<2>(player + 0x7948, 495); compare(*aliased, standard);
        for (unsigned index = 1; index < 3; ++index) {
            auto wrapped = seed(standard, index); wrapped->wr<4>(12, 0xffffffe7);
            wrapped->memory.map(0, 1); wrapped->memory.write<1>(0, 253); compare(*wrapped, standard, index);
        }
        std::cout << "PASS: match-actor comparisons=" << comparisons << " interrupted=" << interruptions << " own/child-negatives=" << negatives
                  << "; full ABI/memory/ordered effects/dependency boundaries/accounting, periods, dismissal modes, keeper/tactics, budgets, byte holes and position widths\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
