#include "exception_search_fixtures.hpp"
#include "code_fragment_adapter.hpp"
#include "cpp_object_storage_adapter.hpp"
#include "mac_abi.hpp"
#include <string_view>

using namespace cm::semantic_io_checks;
using namespace cm::exception_checks;
namespace cm::aot { bool exception_transfer_reference(State&); bool exception_capture_reference(State&); }
namespace {
constexpr std::uint32_t import_address = 280984548, parent_function = import_address - 0x100,
    child_function = import_address - 0x1100, marker = fixture + 0xf700,
    array = fixture + 0x6000, constructor = fixture + 0x8500, destructor = fixture + 0x8510,
    partial = fixture + 0x8520, constructor_toc = fixture + 0xf100, destructor_toc = fixture + 0xf200;
enum class Case { parent, child, generated_child, shared_frame, recursive_outer, recursive_inner, peer,
                  restore_fault, handler_fault, array_failure };
enum class Capture { vector, scalar, select_scalar, select_vector };
unsigned comparisons = 0, faults = 0, negatives = 0;
struct StorageObserver final : CppObjectStorageBoundaryObserver {
    std::vector<std::pair<std::uint32_t, bool>> boundaries;
    void enter(const State&, std::uint32_t entry, bool) override { boundaries.emplace_back(entry, false); }
    void leave(State&, std::uint32_t entry, bool, bool terminal) override {
        require(!terminal, "Catch fixture must not terminate"); boundaries.emplace_back(entry, true);
    }
};
struct Model final : CodeFragmentBoundaryObserver {
    bool original;
    Case kind;
    Capture capture_kind;
    unsigned stage = 0, parent_after = 0, child_after = 0, throw_after = 0, handlers = 0,
        constructors = 0, destructors = 0;
    std::uint32_t parent_frame = 0, child_frame = 0;
    std::unique_ptr<State> peer;
    std::unique_ptr<Image> peer_before;
    struct Import { std::string name; decltype(State::registers) registers; std::uint32_t pc;
        std::uint64_t calls; bool operator==(const Import&) const = default; };
    std::vector<Import> imports;
    std::vector<std::array<std::uint32_t, 2>> constructor_args, destructor_args;
    explicit Model(bool baseline, Case scenario, Capture capture = Capture::vector)
        : original(baseline), kind(scenario), capture_kind(capture) {}
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State& s, const char* name) override { imports.push_back({name, s.registers, s.pc, s.native_calls}); }
    void import_leave(const State& s, const char* name) override { import_enter(s, name); }
    bool inside_child() const { return kind == Case::child || kind == Case::generated_child ||
                                      kind == Case::shared_frame || kind == Case::recursive_inner; }
    std::uint32_t target_function() const {
        return kind == Case::recursive_outer || kind == Case::recursive_inner || !inside_child() ? parent_function : child_function;
    }
    void execute(State& s, bool capture = false) {
        if (capture) {
            s.wr<4>(12, actual); s.wr<4>(16, object); s.wr<4>(20, 0);
            s.pc = capture_kind == Capture::vector ? 0x10772b10 : capture_kind == Capture::scalar ? 0x10772ca0 : 0x10772ad0;
        } else s.pc = 0x10772810;
        if (original) {
            for (;;) if (!(capture ? cm::aot::exception_capture_reference(s) : cm::aot::exception_transfer_reference(s)))
                run_until(s, std::uint32_t(s.r<4>(4128)));
        } else {
            if (!capture) exception_runtime::search_throw(s);
            else if (capture_kind == Capture::vector) exception_runtime::throw_vector(s);
            else if (capture_kind == Capture::scalar) exception_runtime::throw_scalar(s);
            else exception_runtime::throw_dispatch(s);
        }
        ++throw_after; throw std::runtime_error("Context transfer returned to interrupted host caller");
    }
    void prepare(State& s) {
        const auto function = target_function(), target_frame = inside_child() ? child_frame : parent_frame;
        s.memory.write<4>(node + 8, function - 0x100); s.memory.write<4>(node + 12, function + 0x1000);
        s.memory.write<4>(context + 0x3a8, function + 0x20); s.memory.write<4>(context + 0x39c, target_frame);
        s.memory.write<4>(context + 0x394, target_frame);
        s.memory.write<4>(context + 0x3a4, kind == Case::restore_fault ? restored_stack : std::uint32_t(s.r<4>(4)));
        s.memory.write<2>(action + 6, import_address - function); stage = 3;
    }
    struct PeerImports final : CodeFragmentBoundaryObserver {
        Model& model; State& owner;
        PeerImports(Model& m, State& s) : model(m), owner(s) {}
        void enter(const State&, std::uint32_t, bool) override {}
        void leave(State&, std::uint32_t, bool) override {}
        bool import_fixture(State& s, const char*) override {
            s.pc = parent_function; GuestNativeScope scope(s); scope.frame(model.parent_frame);
            s.pc = import_address; model.execute(owner); return true;
        }
    };
    bool handler(State& s) {
        ++handlers;
        if (kind == Case::handler_fault) throw std::runtime_error("Fixture handler failure after transfer");
        if (inside_child()) {
            s.wr<4>(4, parent_frame); s.wr<4>(4128, parent_function + 0x40); s.pc = parent_function + 0x40;
        } else { s.wr<4>(4128, return_sentinel); s.pc = return_sentinel; }
        return true;
    }
    bool array_callback(State& s) {
        const auto a = std::uint32_t(s.r<4>(12)), b = std::uint32_t(s.r<4>(16));
        if (s.r<4>(8) == constructor_toc) {
            constructor_args.push_back({a, b}); ++constructors;
            s.memory.write<4>(a, constructors == 1 ? 0x55555555 : 0xeeeeeeee);
            if (constructors == 1) { s.pc = std::uint32_t(s.r<4>(4128)); return true; }
            stage = 3; execute(s, true); return true;
        }
        if (s.r<4>(8) == destructor_toc) {
            destructor_args.push_back({a, b}); ++destructors; s.memory.write<4>(a, 0xdddddddd);
            s.pc = std::uint32_t(s.r<4>(4128)); return true;
        }
        return handler(s);
    }
    bool import_fixture(State& s, const char* name) override {
        require(std::string_view(name) == "CarbonLib::Gestalt", "Explicit transfer fixture import");
        if (kind == Case::array_failure) return array_callback(s);
        if (stage == 3) return handler(s);
        std::uint64_t escapes = 0;
        if (!stage) {
            s.pc = parent_function; MacAbi parent(s, escapes); parent.capture_link(); parent.save_link(parent_function + 4);
            parent.frame(112, parent_function + 8); parent_frame = parent.r(1); stage = 1;
            parent.call(import_address, parent_function + 0x40); ++parent_after;
            parent.store(marker, 1, parent_function + 0x44); parent.pop(112, parent_function + 0x48); parent.restore_link(); parent.finish();
            return true;
        }
        if (stage == 1) {
            if (kind == Case::generated_child || kind == Case::shared_frame) {
                child_frame = kind == Case::shared_frame ? parent_frame : parent_frame - 112;
                if (kind != Case::shared_frame) { s.memory.write<4>(child_frame, parent_frame); s.wr<4>(4, child_frame); }
                prepare(s); execute(s); ++child_after; return true;
            }
            const auto function = kind == Case::recursive_outer || kind == Case::recursive_inner ? parent_function : child_function;
            s.pc = function; MacAbi child(s, escapes); child.capture_link(); child.save_link(function + 4);
            child.frame(112, function + 8); child_frame = child.r(1); stage = 2;
            child.call(import_address, function + 0x20); ++child_after;
            child.store(marker + 4, 1, function + 0x24); child.pop(112, function + 0x28); child.restore_link(); child.finish(); return true;
        }
        prepare(s);
        if (kind == Case::peer) {
            require(bool(peer), "Prepared peer state"); PeerImports other(*this, s); peer->code_fragment_observer = &other;
            try { run(*peer); } catch (...) { peer->code_fragment_observer = nullptr; throw; }
            peer->code_fragment_observer = nullptr; ++throw_after; return true;
        }
        execute(s); return true;
    }
};
std::unique_ptr<State> initial(const char* program, Case kind, bool vectors) {
    auto f = setup(program, false, vectors); auto s = std::move(f.state);
    if (kind == Case::restore_fault) s = without_byte(*s, restored_stack + 3);
    s->pc = import_address; return s;
}
std::unique_ptr<State> array_initial(const char* program, Capture capture) {
    auto f = setup(program); auto s = std::move(f.state);
    const auto caller_descriptor = descriptors + 0x300, caller_ranges = ranges + 0x100;
    s->memory.write<4>(node + 8, base); s->memory.write<4>(node + 12, base + 0x900000);
    s->memory.write<4>(node + 4, ranges + 24);
    s->memory.write<4>(ranges, 0x772ad0); s->memory.write<4>(ranges + 4, 0x34); s->memory.write<4>(ranges + 8, 0x400);
    s->memory.write<4>(ranges + 12, 0x773150); s->memory.write<4>(ranges + 16, 0x10c); s->memory.write<4>(ranges + 20, 0x200);
    s->memory.write<2>(descriptor, 5u << 11); s->memory.write<2>(descriptor + 2, 0x10);
    s->memory.write<2>(descriptor + 4, 0x100); s->memory.write<2>(descriptor + 6, 0x80);
    s->memory.write<1>(action, 0x82); s->memory.write<2>(action + 2, 56); s->memory.write<4>(action + 4, partial);
    for (auto [offset, value] : {std::pair{0u, caller_ranges}, {4u, caller_ranges + 12},
            {8u, parent_function - 0x100}, {12u, parent_function + 0x1000}, {16u, descriptors}, {24u, toc}})
        s->memory.write<4>(node + 28 + offset, value);
    s->memory.write<4>(caller_ranges, 0x100); s->memory.write<4>(caller_ranges + 4, 0x40); s->memory.write<4>(caller_ranges + 8, 0x300);
    s->memory.write<2>(caller_descriptor + 2, 0x10); s->memory.write<2>(caller_descriptor + 4, 0x30); s->memory.write<2>(caller_descriptor + 6, 0x80);
    handler(*s, caller_descriptor + 0x80, expected, 0x100);
    for (auto [vector, pc, table] : {std::tuple{constructor, import_address, constructor_toc},
            std::tuple{destructor, import_address, destructor_toc}, std::tuple{partial, 0x10773260u, toc}}) {
        s->memory.write<4>(vector, pc); s->memory.write<4>(vector + 4, table);
    }
    for (auto offset : {0x3b9c, 0x3b8c, 0x3b88, 0x3524, 0x3528, 0x3520})
        s->memory.write<4>(constructor_toc - offset, s->memory.read<4>(toc - offset));
    s->memory.write<1>(constructor_toc - 0x3529, 1);
    const auto vectors = capture == Capture::vector || capture == Capture::select_vector;
    s->memory.write<1>(constructor_toc - 0x352a, vectors ? 7 : 0); s->memory.write<1>(toc - 0x352a, vectors ? 7 : 0);
    s->memory.write<4>(stack, stack + 128);
    s->wr<4>(12, array); s->wr<4>(16, constructor); s->wr<4>(20, destructor);
    s->wr<4>(24, 4); s->wr<4>(28, 3); s->wr<4>(4128, parent_function + 0x20); s->pc = 0x10773150; return s;
}
void check(const char* program, Case kind, bool vectors = false, bool negative = false, Capture capture = Capture::vector) {
    auto a = kind == Case::array_failure ? array_initial(program, capture) : initial(program, kind, vectors); auto b = clone(*a);
    a->cpp_object_storage_original = kind == Case::array_failure;
    Model x(true, kind, capture), y(false, kind, capture); Effects ea, eb; ea.state = a.get(); eb.state = b.get();
    StorageObserver original_storage, semantic_storage;
    if (kind == Case::array_failure) {
        a->cpp_object_storage_observer = &original_storage; b->cpp_object_storage_observer = &semantic_storage;
    }
    if (kind == Case::peer) {
        x.peer = setup(program).state; y.peer = clone(*x.peer);
        for (auto* model : {&x, &y}) {
            model->peer->pc = import_address; model->peer->wr<4>(4, stack - 112);
            model->peer_before = std::make_unique<Image>(*model->peer);
        }
    }
    a->code_fragment_observer = &x; b->code_fragment_observer = &y; a->memory.date_observer = &ea; b->memory.date_observer = &eb;
    std::string ax, bx;
    const auto run_case = [](State& s, std::string& error) {
        try { run(s); } catch (const GuestContextTransfer&) { error = "Escaped context-transfer owner"; }
        catch (const std::exception& e) { error = e.what(); }
    };
    run_case(*a, ax); run_case(*b, bx);
    a->code_fragment_observer = b->code_fragment_observer = nullptr; a->memory.date_observer = b->memory.date_observer = nullptr;
    a->cpp_object_storage_observer = b->cpp_object_storage_observer = nullptr;
    const auto label = "Nested transfer case=" + std::to_string(unsigned(kind)) + " vectors=" + std::to_string(vectors) +
        " capture=" + std::to_string(unsigned(capture));
    require(ax == bx, label + " original=" + ax + " semantic=" + bx);
    const bool fault = kind == Case::restore_fault || kind == Case::handler_fault;
    require(fault == !ax.empty(), label + " expected outcome " + ax);
    if (kind == Case::restore_fault) require(ax.starts_with("Unmapped data access at "), label + " restoration fault");
    if (kind == Case::handler_fault) require(ax == "Fixture handler failure after transfer", label + " handler fault");
    Image image(*a); image.compare(*b, label); require(ea.bytes == eb.bytes, label + " ordered guest effects");
    require(x.imports == y.imports && ea.callbacks == eb.callbacks, label + " import/callback prefix");
    require(x.parent_after == y.parent_after && x.child_after == y.child_after && x.throw_after == y.throw_after && x.handlers == y.handlers,
        label + " surviving/abandoned host scopes");
    require(!x.child_after && !y.child_after && !x.throw_after && !y.throw_after, label + " interrupted child never resumes");
    require(x.parent_after == unsigned(x.inside_child()), label + " parent resumes only for its child's handler");
    require(x.handlers == unsigned(kind != Case::restore_fault), label + " restored handler entry count");
    require(!a->cpp_object_storage_original_resume && !b->cpp_object_storage_original_resume, label + " abandoned diagnostic selection resets");
    complete_context_transfer(*a); complete_context_transfer(*b); // No stale dispatch scope after return/fault.
    if (kind == Case::peer) {
        x.peer_before->compare(*x.peer, "Peer-state transfer cannot consume owner's signal");
        y.peer_before->compare(*y.peer, "Peer-state transfer leaves peer architecture intact");
    }
    if (kind == Case::array_failure) {
        require(original_storage.boundaries == semantic_storage.boundaries &&
            semantic_storage.boundaries == std::vector<std::pair<std::uint32_t, bool>>{{0x10773150, false}, {0x10773260, false}, {0x10773260, true}},
            "Interrupted constructor has no return boundary; partial cleanup returns once in both diagnostic modes");
        require(x.constructor_args == y.constructor_args && x.destructor_args == y.destructor_args, label + " construction/destruction arguments");
        require(x.constructor_args == std::vector<std::array<std::uint32_t, 2>>{{array + 16, 1}, {array + 20, 1}}, "Only first and throwing second elements are constructed");
        require(x.destructor_args == std::vector<std::array<std::uint32_t, 2>>{{array + 16, 0xffffffff}}, "Only completed first element is destroyed");
        require(b->memory.read<4>(array + 16) == 0xdddddddd && b->memory.read<4>(array + 20) == 0xeeeeeeee,
            "Completed destruction and failed-construction writes remain visible");
        require(b->memory.read<4>(stack - 112 + 72) == 0 && b->memory.read<4>(stack + 0x40) == object,
            "Array cleanup decrements completed count and publishes caller catch");
        require(b->cpp_object_storage_calls[0] == 1 && b->cpp_object_storage_calls[1] == 1 && !b->cpp_object_storage_dispatch_escapes,
            "Actual modern array constructor and partial-cleanup entries execute");
        require(b->native_calls == 3, "Throwing constructor import has no completed-import count");
    }
    if (negative) {
        b->wr<4>(4, b->r<4>(4) ^ 1); bool rejected = false;
        try { image.compare(*b, label + " corrupted transferred stack"); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Nested transfer negative accepted"); ++negatives;
    }
    ++comparisons; faults += fault;
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Usage: cm-exception-transfer-checks <program>");
        require(std::string_view(import_name(import_address)) == "CarbonLib::Gestalt", "Pinned fixture import address");
        for (bool vectors : {false, true}) for (auto kind : {Case::parent, Case::child, Case::generated_child,
                Case::shared_frame, Case::recursive_outer, Case::recursive_inner, Case::peer})
            check(argv[1], kind, vectors, !vectors && kind == Case::parent);
        check(argv[1], Case::restore_fault); check(argv[1], Case::handler_fault);
        for (auto capture : {Capture::vector, Capture::scalar, Capture::select_scalar, Capture::select_vector})
            check(argv[1], Case::array_failure, false, capture == Capture::vector, capture);
        require(negatives == 2, "Nested transfer and real array cleanup need discriminating negatives");
        std::cout << comparisons << " nested original/C++ transfer comparisons (" << faults << " faults), " << negatives
                  << " corruption negatives; actual throwing array callback and partial cleanup passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
