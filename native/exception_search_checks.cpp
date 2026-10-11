#include "exception_search_fixtures.hpp"
#include "exception_runtime_context.hpp"
#include "code_fragment_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <set>

using namespace cm::semantic_io_checks;
namespace cm::aot { bool exception_search_reference(State&); }
namespace {
using namespace cm::exception_checks;
unsigned comparisons = 0, faults = 0, negatives = 0, callbacks = 0;
void reference_until(State& s, std::uint32_t stop) {
    while (s.pc != stop) if (!cm::aot::exception_search_reference(s)) run_until(s, std::uint32_t(s.r<4>(4128)));
}
Expect expectation(const Fixture& f) {
    const auto block = f.block, stop = f.stop, selected = f.selected, published = f.published,
        backchain = f.restored_backchain, selected_frame = f.selected_frame, catch_toc = f.catch_toc;
    const auto specification = f.specification; const auto callback_count = f.callback_count;
    return [=](const State& s, const Effects& e) {
        require(s.pc == stop && s.r<4>(4128) == stop && s.r<4>(4) == restored_stack && s.r<4>(8) == catch_toc,
            "Independent restored handler PC/LR/SP/TOC");
        require(s.memory.read<4>(restored_stack) == backchain && s.memory.read<4>(context + 0x3a0) == selected_frame,
            "Independent restored backchain and selected frame");
        require(s.memory.read<4>(block) == object && s.memory.read<4>(block + 4) == actual, "Installed exception object/type");
        if (specification) require(s.memory.read<4>(block + 0x14) == selected, "Installed live specification record");
        else require(s.memory.read<4>(block + 0xc) == published, "Installed adjusted catch pointer");
        require(e.callbacks.size() == callback_count, "Independent completed cleanup callback count");
    };
}
Effects check(Fixture f, const std::string& label, const Mutation& mutation = {}, bool fault = false,
        bool negative = false, const Expect& expected_result = {}) {
    auto semantic = clone(*f.state); Effects a, b; a.state = f.state.get(); b.state = semantic.get(); a.on_read = b.on_read = mutation;
    f.state->memory.date_observer = &a; semantic->memory.date_observer = &b; std::string x, y;
    try { reference_until(*f.state, f.stop); } catch (const std::runtime_error& e) { x = e.what(); }
    try { f.operation(*semantic); } catch (const std::runtime_error& e) { y = e.what(); }
    f.state->memory.date_observer = semantic->memory.date_observer = nullptr;
    require(x == y, label + " outcome original=" + x + " semantic=" + y);
    require(fault == !x.empty(), label + " fault expectation " + x);
    if (fault) require(x.starts_with("Unmapped data access at "), label + " mapping fault");
    Image image(*f.state); image.compare(*semantic, label); require(a.bytes == b.bytes, label + " ordered access prefix");
    require(a.callbacks == b.callbacks, label + " CFM callback ABI/history");
    if (!fault) (expected_result ? expected_result : expectation(f))(*semantic, b);
    if (negative) {
        semantic->wr<4>(4, semantic->r<4>(4) ^ 1); bool rejected = false;
        try { image.compare(*semantic, label + " corrupted transferred stack"); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, label + " root negative accepted"); ++negatives;
    }
    ++comparisons; faults += fault; callbacks += b.callbacks.size(); b.state = nullptr; b.on_read = {}; return b;
}
void direct_checks(const char* program) {
    for (unsigned count : {0u, 1u, 3u}) {
        auto f = specification_fixture(program, count, false, true);
        check(std::move(f), "Direct specification transfer count=" + std::to_string(count), {}, false, count == 0);
    }
    auto negative_offset = specification_fixture(program, 0, false, true);
    negative_offset.state->memory.write<4>(action + 8, 0xfffffff0); negative_offset.block = frame - 16;
    check(std::move(negative_offset), "Direct specification signed word32 frame offset");
    auto wrapped = specification_fixture(program, 0, false, true);
    wrapped.state->memory.write<4>(action + 4, 0xffffff00); wrapped.stop = base;
    check(std::move(wrapped), "Direct specification landing word32 wrapping");
    auto tail = specification_fixture(program, 0, false, true);
    tail.state->memory.write<4>(action + 4, 0x107727ec - (base + 0x100));
    tail.state->memory.write<4>(context + 0x3a4, stack - 80); tail.stop = return_sentinel;
    check(std::move(tail), "Direct specification restoration selects returning epilogue", {}, false, false,
        [](const State& s, const Effects&) {
            require(s.pc == return_sentinel && s.r<4>(4) == stack && s.r<4>(4128) == return_sentinel,
                "Specification returning tail restores caller PC/LR/SP");
            for (unsigned n = 29; n < 32; ++n) require(s.r<4>(n * 4) == 0xcafe0000 + n, "Specification tail restores its saved GPRs");
        });
}
void search_checks(const char* program) {
    unsigned seed = 0;
    for (bool selected_indirect : {false, true}) for (bool vectors : {false, true}) {
        auto f = setup(program, selected_indirect, vectors, seed++);
        check(std::move(f), "Catch selected frame/vector=" + std::to_string(seed), {}, false, seed == 1);
    }
    for (bool pointer : {false, true}) {
        auto f = setup(program); text(*f.state, actual, pointer ? "*Child!0!Base!32!" : "!Child!0!Base!16!");
        text(*f.state, expected, pointer ? "*Base!" : "!Base!");
        if (pointer) f.published = f.block + 0x10; else f.published = object + 16;
        check(std::move(f), "Catch class base adjustment pointer=" + std::to_string(pointer), {}, false, false,
            [pointer](const State& s, const Effects& e) {
                require(s.pc == landing && s.r<4>(4) == restored_stack, "Class catch restored destination");
                require(s.memory.read<4>(frame + 0x4c) == (pointer ? frame + 0x50 : object + 16), "Independent class catch pointer");
                if (pointer) require(s.memory.read<4>(frame + 0x50) == 0xa0b0c0f0, "Indirect exception pointer plus base offset");
                require(e.callbacks.empty(), "Class catch has no destructor calls");
            });
    }
    auto catch_all = setup(program); catch_all.state->memory.write<4>(action + 2, 0);
    check(std::move(catch_all), "Catch-all handler");
    auto second = setup(program); text(*second.state, expected, "l"); text(*second.state, expected + 0x40, "i");
    second.selected = action + 10; handler(*second.state, second.selected, expected + 0x40);
    check(std::move(second), "Mismatching catch followed by matching handler");
    auto negative_offset = setup(program); handler(*negative_offset.state, action, expected, 0x200, -16); negative_offset.block = frame - 16;
    check(std::move(negative_offset), "Catch signed-halfword frame offset");
    auto relocated = setup(program, false, false, seed++, fixture + 0xf000);
    check(std::move(relocated), "Relocated live search TOC");
    auto tail = setup(program); constexpr auto tail_base = 0x10772000u;
    tail.state->memory.write<4>(node + 8, tail_base); tail.state->memory.write<4>(node + 12, tail_base + 0x1000);
    tail.state->memory.write<4>(context + 0x3a8, tail_base + 0x120); tail.state->memory.write<2>(action + 6, 0x9b0);
    tail.state->memory.write<4>(context + 0x3a4, stack - 128); tail.stop = return_sentinel;
    check(std::move(tail), "Search restoration selects returning epilogue", {}, false, false,
        [](const State& s, const Effects&) {
            require(s.pc == return_sentinel && s.r<4>(4) == stack && s.r<4>(4128) == return_sentinel,
                "Search returning tail restores caller PC/LR/SP");
            for (unsigned n = 30; n < 32; ++n) require(s.r<4>(n * 4) == 0xcafe0000 + n, "Search tail restores its saved GPRs");
        });
    for (unsigned count : {0u, 1u, 3u}) for (bool accepted : {false, true}) {
        if (!count && accepted) continue;
        check(specification_fixture(program, count, accepted), "Specification search count=" + std::to_string(count) + " accepted=" + std::to_string(accepted));
    }
}
Fixture frame_fixture(const char* program, bool selected_indirect) {
    auto f = setup(program); auto& s = *f.state; constexpr auto caller = indirect, alternate = fixture + 0xd000;
    const auto next_descriptor = descriptors + 0x300;
    s.memory.write<2>(descriptor, (2u << 11) | (3u << 6) | 4u | 32u);
    s.memory.write<1>(descriptor + 2, (3u << 3) | 4u); s.memory.write<2>(descriptor + 4, 0);
    s.memory.write<4>(frame, caller); s.memory.write<4>(caller, 0x55667788); s.memory.write<4>(caller + 4, 0x2468ace0);
    s.memory.write<4>(caller + 8, base + 0x220);
    for (unsigned n = 0; n < 192; ++n) s.memory.write<1>(caller - 192 + n, n * 23 + 17);
    s.memory.write<4>(caller - 40, 0x11223344); s.memory.write<4>(caller - 36, alternate);
    s.memory.write<4>(node + 4, ranges + 24); s.memory.write<4>(ranges + 12, 0x200);
    s.memory.write<4>(ranges + 16, 0x40); s.memory.write<4>(ranges + 20, 0x300);
    s.memory.write<1>(next_descriptor + 1, selected_indirect ? 16 : 0);
    s.memory.write<2>(next_descriptor + 2, 0x10); s.memory.write<2>(next_descriptor + 4, 0x30);
    s.memory.write<2>(next_descriptor + 6, 0x80); f.selected = next_descriptor + 0x80; handler(s, f.selected);
    f.stop = base + 0x400; f.selected_frame = selected_indirect ? alternate : caller;
    f.block = f.selected_frame + 0x40; f.restored_backchain = 0x55667788; return f;
}
Fixture cleanup_fixture(const char* program, bool rethrow = false, bool pointer = false) {
    auto f = setup(program); auto& s = *f.state;
    s.memory.write<1>(action, 2); s.memory.write<2>(action + 2, 0x20); s.memory.write<4>(action + 4, callback);
    f.callback_count = 1; f.selected = action + 8;
    if (rethrow) {
        s.memory.write<1>(f.selected, 13); s.memory.write<2>(f.selected + 2, 0x100); f.selected += 4;
        s.memory.write<4>(frame + 0x100, object); s.memory.write<4>(frame + 0x104, actual); s.memory.write<4>(frame + 0x108, callback);
        s.memory.write<4>(context + 0x3ac, 0); s.memory.write<4>(context + 0x3b0, 0);
        if (pointer) { text(s, actual, "*Child!0!Base!32!"); text(s, expected, "*Base!"); f.published = f.block + 16; }
    }
    handler(s, f.selected); return f;
}
void composed_checks(const char* program) {
    for (bool rethrow : {false, true}) {
        auto f = cleanup_fixture(program, rethrow); const auto result = expectation(f);
        check(std::move(f), "Inline cleanup and rethrow=" + std::to_string(rethrow), {}, false, false,
            [=](const State& s, const Effects& e) {
                result(s, e); require(e.callbacks[0][0] == frame + 0x20 && e.callbacks[0][1] == 0xffffffff,
                    "Inline destructor precedes catch with original arguments");
                require(s.memory.read<4>(context + 0x3b8) == (rethrow ? frame + 0x100 : 0), "Rethrow active record identity");
                require(s.memory.read<4>(frame + 0x48) == callback, "Retained object callback or rethrow cleanup deferral");
            });
    }
    auto direct_rethrow = setup(program); direct_rethrow.state->memory.write<4>(context + 0x3ac, 0);
    direct_rethrow.state->memory.write<1>(action, 13); direct_rethrow.state->memory.write<2>(action + 2, 0x100);
    direct_rethrow.state->memory.write<4>(frame + 0x100, object); direct_rethrow.state->memory.write<4>(frame + 0x104, actual);
    direct_rethrow.state->memory.write<4>(frame + 0x108, callback); direct_rethrow.selected = action + 4; handler(*direct_rethrow.state, direct_rethrow.selected);
    check(std::move(direct_rethrow), "Rethrow starts at active exception record");
    auto pointer = cleanup_fixture(program, true, true); check(std::move(pointer), "Rethrow pointer base adjustment");
    for (bool selected_indirect : {false, true}) {
        auto f = frame_fixture(program, selected_indirect); const auto result = expectation(f);
        check(std::move(f), "Search and cleanup caller recovery indirect=" + std::to_string(selected_indirect), {}, false, false,
            [=](const State& s, const Effects& e) {
                result(s, e); require(s.memory.read<4>(context + 0x39c) == indirect && s.memory.read<4>(context + 0x394) == fixture + 0xd000,
                    "Composed search/cleanup recovers caller and saved r31");
                require(s.memory.read<4>(context + 0x398) == 0x2468ace0 && s.r<4>(120) == 0x11223344,
                    "Composed search/cleanup restores saved condition and general registers");
            });
    }
    auto redirected = setup(program); redirected.selected = action + 10; redirected.stop = landing + 4;
    handler(*redirected.state, redirected.selected, expected, 0x204);
    const auto search_table = std::uint32_t(redirected.state->memory.read<4>(toc - 0x3b9c));
    check(std::move(redirected), "Live search destination skips first matching catch",
        [=](State& s, Effects& e, std::uint32_t address, unsigned) {
            if (s.pc == 0x107729a8 && address == search_table + 48 && !e.mutated) {
                e.mutated = true; s.memory.write<4>(address, 0x10772a0c);
            } else if (s.pc == 0x107717ac && e.mutated) s.memory.write<4>(search_table + 48, 0x107729b4);
        });
    auto returning = setup(program); returning.state->memory.write<1>(action, 14);
    returning.state->memory.write<4>(stack - 128 + 0x38, 7); returning.published = object + 7; returning.callback_count = 1;
    check(std::move(returning), "Returning termination callback resumes handler publication",
        [](State& s, Effects& e, std::uint32_t, unsigned) {
            if (s.pc == 0x10781320 && !e.mutated) { e.mutated = true; s.memory.write<1>(action, 12); }
        });
    auto no_metadata = setup(program); no_metadata.state->memory.write<4>(toc - 0x3524, 0);
    check(std::move(no_metadata), "Missing initial metadata after returning termination callback", {}, true);
}
void fault_checks(const char* program) {
    for (unsigned n = 0; n < 5; ++n) {
        auto f = n == 0 ? setup(program, true, true) : n == 1 ? cleanup_fixture(program, true, true) :
            n == 4 ? frame_fixture(program, true) : specification_fixture(program, 1, false, n == 3);
        Fixture witness; witness.state = clone(*f.state); witness.operation = f.operation; witness.stop = f.stop;
        witness.block = f.block; witness.selected = f.selected; witness.published = f.published;
        witness.selected_frame = f.selected_frame; witness.specification = f.specification; witness.callback_count = f.callback_count;
        witness.restored_backchain = f.restored_backchain;
        auto effects = check(std::move(witness), "Search/catch access witness=" + std::to_string(n));
        std::set<std::uint32_t> addresses;
        for (auto event : effects.log) if ((event.kind == 2 || event.kind == 4) && event.size)
            addresses.insert(event.address + std::uint32_t(event.size) - 1);
        for (auto address : addresses) {
            Fixture missing; missing.state = without_byte(*f.state, address); missing.operation = f.operation; missing.stop = f.stop;
            check(std::move(missing), "Search/catch missing byte=" + hex_address(address), {}, true);
        }
    }
}
struct MetadataNegative final : CodeFragmentBoundaryObserver {
    Effects* effects = nullptr;
    std::unique_ptr<Image> boundary;
    std::vector<std::uint8_t> prefix;
    bool rejected = false;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State& s, std::uint32_t entry, bool original) override {
        if (entry != 0x107713e0) return;
        if (original) { if (!boundary) { boundary = std::make_unique<Image>(s); prefix = effects->bytes; } return; }
        require(bool(boundary), "Original metadata child return"); boundary->compare(s, "Search metadata child before corruption");
        require(prefix == effects->bytes, "Search metadata ordered child prefix"); s.wr<4>(16, s.r<4>(16) ^ 1);
        try { boundary->compare(s, "Corrupted metadata return before search parent"); }
        catch (const std::runtime_error&) { rejected = true; throw std::runtime_error("Rejected metadata child before search resumed"); }
        throw std::runtime_error("Metadata child negative accepted");
    }
};
void child_negative(const char* program) {
    auto f = setup(program); auto semantic = clone(*f.state); f.state->code_fragment_original = true;
    MetadataNegative observer; Effects a, b; a.state = f.state.get(); b.state = semantic.get(); observer.effects = &a;
    f.state->memory.date_observer = &a; f.state->code_fragment_observer = &observer;
    reference_until(*f.state, f.stop); f.state->memory.date_observer = nullptr; f.state->code_fragment_observer = nullptr;
    require(bool(observer.boundary), "Search metadata child negative witness"); observer.effects = &b;
    semantic->memory.date_observer = &b; semantic->code_fragment_observer = &observer; std::string error;
    try { exception_runtime::search_throw(*semantic); } catch (const std::runtime_error& e) { error = e.what(); }
    semantic->memory.date_observer = nullptr; semantic->code_fragment_observer = nullptr;
    require(observer.rejected && error == "Rejected metadata child before search resumed" && b.bytes == observer.prefix,
        "Metadata negative must precede parent effect: " + error); ++negatives;
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Usage: cm-exception-search-checks <program>");
        direct_checks(argv[1]); search_checks(argv[1]); composed_checks(argv[1]); fault_checks(argv[1]); child_negative(argv[1]);
        require(negatives == 3, "Both roots and the metadata child need discriminating transferred-state negatives");
        std::cout << comparisons << " C++ search/catch comparisons (" << faults << " mapping faults), "
                  << negatives << " own/child corruption negatives, " << callbacks << " CFM callback boundary witnesses passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
