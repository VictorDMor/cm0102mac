#include "exception_runtime_context.hpp"
#include "mac_abi.hpp"

namespace cm::aot::exception_runtime {
namespace {
struct UnexpectedAbi : MacAbi {
    explicit UnexpectedAbi(State& state) : MacAbi(state, state.exception_runtime_dispatch_escapes, 0x107724c0) {}
    void child(std::uint32_t entry, std::uint32_t link) {
        s.wr<4>(2440, r(2)); s.wr<4>(4128, link); s.pc = entry; invoke_exception_runtime(s, entry);
    }
    bool accepted() const { return std::bit_cast<std::int32_t>(r(3)) != 0; }
    void match(std::uint32_t link) {
        put(4, r(26)); child(0x10772430, link);
        compare(std::bit_cast<std::int32_t>(r(3)));
    }
    void publish_bad_exception(unsigned offset, std::uint32_t pc) {
        store(r(31) + offset, r(30), pc); put(3, r(28)); put(5, r(27)); put(4, r(31) + offset);
        store(r(31) + offset, r(29), pc + 16); child(0x10772ad0, pc + 24);
    }
    void terminate_and_return() {
        child(0x10772eb0, 0x10772588);
        load(0, r(31) + 0x78, 0x10772588); load(1, r(1), 0x1077258c);
        restore_link(); words(26, 0x10772594, true); finish();
    }
};
}
void handle_unexpected(State& state) {
    UnexpectedAbi a(state);
    a.capture_link(); a.words(26, 0x107724c4);
    a.load(27, a.r(2) - 0x5abc, 0x107724c8); a.load(28, a.r(2) - 0x3b94, 0x107724cc);
    a.load(29, a.r(2) - 0x5ab8, 0x107724d0); a.load(30, a.r(2) - 0x5ab4, 0x107724d4);
    a.save_link(0x107724d8); a.frame(112, 0x107724dc); a.load(26, a.r(3) + 0x14, 0x107724e0);
    a.put(31, a.r(1)); a.store(a.r(1) + 0x54, a.r(1), 0x107724e8);
    a.child(0x10772ee0, 0x107724f0);
    if (state.pc == 0x107724f0) a.terminate_and_return();
}
bool resume_unexpected_continuation(State& state) {
    UnexpectedAbi a(state); a.guest_scope.frame(a.r(31));
    a.load(3, a.r(31) + 0x44, 0x107724f4); a.match(0x10772500);
    if (a.accepted()) {
        a.put(3, 0); a.put(4, 0); a.put(5, 0); a.child(0x10772ad0, 0x10772518);
        if (state.pc != 0x10772518) return false;
    }
    a.load(3, a.r(2) - 0x3b90, 0x10772518); a.match(0x10772524);
    if (a.accepted()) {
        a.publish_bad_exception(0x3c, 0x1077252c); if (state.pc != 0x10772544) return false;
    }
    a.load(3, a.r(2) - 0x3b98, 0x10772544); a.match(0x10772550);
    if (a.accepted()) {
        a.publish_bad_exception(0x38, 0x10772558); if (state.pc != 0x10772570) return false;
    }
    a.put(3, a.r(31) + 0x40); a.child(0x10772d80, 0x10772578);
    a.load(0, a.r(1), 0x10772578); a.load(1, a.r(31) + 0x54, 0x1077257c); a.store(a.r(1), a.r(0), 0x10772580);
    a.terminate_and_return();
    return true;
}
void resume_unexpected(State& state) { (void)resume_unexpected_continuation(state); }
}
