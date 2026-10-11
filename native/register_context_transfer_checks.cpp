#include "register_context_adapter.hpp"
#include "register_context_fixtures.hpp"
#include "code_fragment_adapter.hpp"
#include "guest_context_transfer.hpp"
#include "mac_abi.hpp"
#include "match_turbo.hpp"

namespace {
using namespace cm::register_context_checks;
constexpr std::uint32_t import_address = 280984548, parent_function = import_address - 0x100,
    child_function = import_address - 0x1100, marker = fixture + 0x700, child_buffer = fixture + 0x300,
    copied_buffer = fixture + 0x500;
enum class Case { parent, child, generated_child, shared_frame, recursive_outer, recursive_inner,
                  copied, peer, restore_fault, handler_fault };
unsigned transfer_cases = 0, transfer_faults = 0;
struct Model final : CodeFragmentBoundaryObserver {
    Case kind;
    unsigned stage = 0, parent_after = 0, child_after = 0, jump_after = 0, handlers = 0;
    std::uint32_t parent_frame = 0, child_frame = 0, target_buffer = buffer;
    std::unique_ptr<State> peer;
    std::unique_ptr<Image> peer_before;
    struct Import { std::string name; decltype(State::registers) registers; std::uint32_t pc;
        std::uint64_t calls; bool operator==(const Import&) const = default; };
    std::vector<Import> imports;
    explicit Model(Case scenario) : kind(scenario) {}
    bool inside_child() const { return kind == Case::child || kind == Case::generated_child ||
        kind == Case::shared_frame || kind == Case::recursive_inner; }
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State& s, const char* name) override { imports.push_back({name, s.registers, s.pc, s.native_calls}); }
    void import_leave(const State& s, const char* name) override { import_enter(s, name); }
    void capture(State& s, MacAbi& abi, std::uint32_t address) {
        s.wr<4>(12, address); abi.call(0x10773eb0, import_address);
        require(s.r<4>(12) == 0, "Actual capture initially returns zero");
    }
    void jump(State& s) {
        stage = 3;
        if (kind == Case::copied) {
            s.memory.copy_in(copied_buffer, s.memory.copy_out(target_buffer, 248)); target_buffer = copied_buffer;
        }
        s.wr<4>(12, kind == Case::restore_fault ? fixture + 0x7fe : target_buffer);
        s.wr<4>(16, kind == Case::recursive_inner ? 0 : 0x80000001);
        invoke_register_context(s, 0x10773f30);
        ++jump_after; throw std::runtime_error("Restored context returned to interrupted host caller");
    }
    struct Peer final : CodeFragmentBoundaryObserver {
        Model& model; State& owner;
        Peer(Model& m, State& s) : model(m), owner(s) {}
        void enter(const State&, std::uint32_t, bool) override {}
        void leave(State&, std::uint32_t, bool) override {}
        bool import_fixture(State&, const char*) override { model.jump(owner); return true; }
    };
    bool handler(State& s) {
        ++handlers;
        require(s.r<4>(12) == (kind == Case::recursive_inner ? 1 : 0x80000001), "Actual resumed capture returns nonzero");
        require(s.r<4>(4) == (inside_child() ? child_frame : parent_frame), "Restored target frame");
        if (kind == Case::handler_fault) throw std::runtime_error("Fixture resumed-handler fault");
        if (inside_child()) { s.wr<4>(4, parent_frame); s.wr<4>(4128, parent_function + 0x40); s.pc = parent_function + 0x40; }
        else { s.wr<4>(4128, incoming_link); s.pc = incoming_link; }
        return true;
    }
    bool import_fixture(State& s, const char* name) override {
        require(std::string_view(name) == "CarbonLib::Gestalt", "Explicit scalar transfer fixture import");
        if (stage == 3) return handler(s);
        std::uint64_t escapes = 0;
        if (!stage) {
            s.pc = parent_function; MacAbi parent(s, escapes); parent.capture_link(); parent.save_link(parent_function + 4);
            parent.frame(112, parent_function + 8); parent_frame = parent.r(1);
            capture(s, parent, buffer); stage = 1;
            parent.call(import_address, parent_function + 0x40); ++parent_after;
            parent.store(marker, 1, parent_function + 0x44); parent.pop(112, parent_function + 0x48); parent.restore_link(); parent.finish();
            return true;
        }
        if (stage == 1) {
            if (kind == Case::generated_child || kind == Case::shared_frame) {
                child_frame = kind == Case::shared_frame ? parent_frame : parent_frame - 112;
                if (kind != Case::shared_frame) { s.memory.write<4>(child_frame, parent_frame); s.wr<4>(4, child_frame); }
                s.wr<4>(4128, import_address); s.wr<4>(12, child_buffer);
                invoke_register_context(s, 0x10773eb0); target_buffer = child_buffer;
                jump(s); ++child_after; return true;
            }
            const auto function = kind == Case::recursive_outer || kind == Case::recursive_inner ? parent_function : child_function;
            s.pc = function; MacAbi child(s, escapes); child.capture_link(); child.save_link(function + 4);
            child.frame(112, function + 8); child_frame = child.r(1);
            if (inside_child()) { capture(s, child, child_buffer); target_buffer = child_buffer; }
            stage = 2; child.call(import_address, function + 0x20); ++child_after;
            child.store(marker + 4, 1, function + 0x24); child.pop(112, function + 0x28); child.restore_link(); child.finish(); return true;
        }
        if (kind == Case::peer) {
            Peer other(*this, s); peer->code_fragment_observer = &other;
            try { run_until(*peer, incoming_link); } catch (...) { peer->code_fragment_observer = nullptr; throw; }
            peer->code_fragment_observer = nullptr; ++jump_after; return true;
        }
        jump(s); return true;
    }
};
void check(Case kind, bool turbo, unsigned mode) {
    auto a = fixture_state(28), b = clone(*a); a->pc = b->pc = import_address;
    a->register_context_original = true;
    if (mode == 0) b->register_context_original = true;
    if (mode == 2) b->register_context_entry_original[28] = true;
    if (mode == 3) b->register_context_entry_original[29] = true;
    Model x(kind), y(kind); Monitor ea, eb; ea.effects.state = a.get(); eb.effects.state = b.get();
    a->memory.date_observer = &ea; b->memory.date_observer = &eb;
    a->code_fragment_observer = &x; b->code_fragment_observer = &y;
    if (kind == Case::peer) for (auto* model : {&x, &y}) {
        model->peer = fixture_state(28); model->peer->pc = import_address;
        model->peer_before = std::make_unique<Image>(*model->peer);
    }
    const auto run_case = [](State& s) {
        try { run_until(s, incoming_link); }
        catch (const GuestContextTransfer&) { return std::string("Escaped transfer owner"); }
        catch (const std::exception& error) { return std::string(error.what()); }
        return std::string{};
    };
    set_match_turbo_enabled(false); const auto ax = run_case(*a);
    set_match_turbo_enabled(turbo); const auto bx = run_case(*b);
    a->memory.date_observer = b->memory.date_observer = nullptr; a->code_fragment_observer = b->code_fragment_observer = nullptr;
    const auto label = "Scalar transfer case=" + std::to_string(unsigned(kind)) + "/mode=" + std::to_string(mode);
    require(ax == bx, label + " outcome original=" + ax + " modern=" + bx);
    const bool fault = kind == Case::restore_fault || kind == Case::handler_fault;
    require(!ax.empty() == fault, label + " expected outcome " + ax);
    if (kind == Case::restore_fault) require(ax.starts_with("Unmapped data access at "), label + " restore prefix");
    if (kind == Case::handler_fault) require(ax == "Fixture resumed-handler fault", label + " handler fault");
    Image(*a).compare(*b, label); require(ea.effects.bytes == eb.effects.bytes && x.imports == y.imports, label + " ordered effects/imports");
    require(!x.child_after && !y.child_after && !x.jump_after && !y.jump_after, label + " abandoned calls never resume");
    require(x.parent_after == unsigned(x.inside_child()) && x.parent_after == y.parent_after &&
        x.handlers == unsigned(kind != Case::restore_fault) && x.handlers == y.handlers, label + " surviving parent and handler counts");
    require(b->memory.read<4>(marker) == (x.inside_child() ? 1u : 0xa7a7a7a7u) &&
        b->memory.read<4>(marker + 4) == 0xa7a7a7a7, label + " stale epilogues cannot write markers");
    require(!a->register_context_unknown_transfers && !b->register_context_unknown_transfers &&
        !b->register_context_dispatch_escapes && !b->register_context_original_body, label + " known live ownership");
    if (mode == 1) require(b->register_context_calls[28] == (x.inside_child() ? 2u : 1u) &&
        b->register_context_calls[29] == 1 && b->register_context_transfers[29] == unsigned(kind != Case::restore_fault),
        label + " actual semantic captures and transfer");
    if (kind == Case::peer) { x.peer_before->compare(*x.peer, label + " peer original"); y.peer_before->compare(*y.peer, label + " peer semantic"); }
    complete_scalar_context_transfer(*a, a->register_context_unknown_transfers);
    complete_scalar_context_transfer(*b, b->register_context_unknown_transfers);
    require(!a->register_context_unknown_transfers && !b->register_context_unknown_transfers, label + " no stale dispatch after completion/fault");
    ++transfer_cases; transfer_faults += fault;
}
void expired_owner() {
    auto s = fixture_state(28); std::uint64_t unknown = 0;
    { GuestDispatchScope owner(*s); GuestScalarContextCapture capture(s.get());
      scalar_context_capture_link(*s, incoming_link); scalar_context_capture_stack(*s, stack); capture.commit(); }
    s->wr<4>(4128, incoming_link); s->wr<4>(4, stack);
    GuestDispatchScope fresh(*s);
    try { complete_scalar_context_transfer(*s, unknown); require(false, "Active scalar restore did not transfer"); }
    catch (const GuestContextTransfer& transfer) { require(fresh.resumes(transfer) && unknown == 1, "Expired owner was reused"); }
}
}
int main() {
    try {
        require(!match_pc_profile_enabled(), "Transfer acceptance requires profiling disabled");
        for (auto kind : {Case::parent, Case::child, Case::generated_child, Case::shared_frame,
                Case::recursive_outer, Case::recursive_inner, Case::copied, Case::peer, Case::restore_fault, Case::handler_fault})
            for (bool turbo : {false, true}) for (unsigned mode = 0; mode < 4; ++mode) check(kind, turbo, mode);
        expired_owner(); std::cout << transfer_cases << " nested original/semantic/mixed transfers; " << transfer_faults
            << " restore/handler faults; copied contexts, recursion, shared frames, peer states and expired owners verified\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
