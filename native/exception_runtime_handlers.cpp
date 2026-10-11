#include "exception_runtime_context.hpp"
#include "mac_abi.hpp"

namespace cm::aot::exception_runtime {
namespace {
struct HandlerAbi : MacAbi {
    explicit HandlerAbi(State& state) : MacAbi(state, state.exception_runtime_dispatch_escapes) {}
    bool equal() const { return (s.r<1>(2304) & 2) != 0; }
    bool greater() const { return (s.r<1>(2304) & 4) != 0; }
    void deleting_flag() { put(0, std::uint32_t(std::int32_t(std::int16_t(r(4))))); record(r(0)); }
    void callback(std::uint32_t link) { call(0x10781320, link); load(2, r(1) + 20, link); }
    void child(std::uint32_t entry, std::uint32_t link) {
        s.wr<4>(2440, r(2)); s.wr<4>(4128, link); s.pc = entry; invoke_exception_runtime(s, entry);
    }
};
void invoke_handler(State& state, std::uint32_t entry, std::uint32_t slot) {
    HandlerAbi a(state);
    a.capture_link(); a.save_link(entry + 4); a.frame(64, entry + 8);
    a.load(12, a.r(2) - slot, entry + 12); a.callback(entry + 20);
    a.pop(64, entry + 24); a.restore_link(); a.finish();
}
}
void destroy_derived_exception(State& state) {
    HandlerAbi a(state);
    a.capture_link(); a.save(31, 0x107725a4); a.put(31, a.r(3)); a.record(a.r(31));
    a.save_link(0x107725ac); a.frame(64, 0x107725b0);
    if (!a.equal()) {
        a.load(0, a.r(2) - 0x5ab8, 0x107725b8); a.store(a.r(31), a.r(0), 0x107725bc);
        // This second branch consumes the still-live CR, rather than repeating
        // the pointer test. Guest observers can expose that distinction.
        if (!a.equal()) { a.load(0, a.r(2) - 0x5ab4, 0x107725c4); a.store(a.r(31), a.r(0), 0x107725c8); }
        a.deleting_flag();
        if (a.greater()) { a.put(3, a.r(31)); a.call(0x107735a0, 0x107725dc); }
    }
    a.put(3, a.r(31)); a.pop(64, 0x107725e0); a.restore_link(); a.restore(31, 0x107725ec); a.finish();
}
void destroy_base_exception(State& state) {
    HandlerAbi a(state);
    a.capture_link(); a.save(31, 0x10772604); a.put(31, a.r(3)); a.record(a.r(31));
    a.save_link(0x1077260c); a.frame(64, 0x10772610);
    if (!a.equal()) {
        a.load(5, a.r(2) - 0x5ab4, 0x10772618); a.deleting_flag(); a.store(a.r(31), a.r(5), 0x10772620);
        if (a.greater()) a.call(0x107735a0, 0x1077262c);
    }
    a.put(3, a.r(31)); a.pop(64, 0x10772630); a.restore_link(); a.restore(31, 0x1077263c); a.finish();
}
void cleanup_exception_object(State& state) {
    HandlerAbi a(state);
    a.capture_link(); a.save_link(0x10772d84); a.frame(64, 0x10772d88);
    a.load(0, a.r(3), 0x10772d8c); a.compare(a.r(0));
    if (!a.equal()) {
        a.load(12, a.r(3) + 8, 0x10772d98); a.compare(a.r(12));
        if (!a.equal()) { a.put(3, a.r(0)); a.put(4, UINT32_MAX); a.callback(0x10772db0); }
    }
    a.pop(64, 0x10772db4); a.restore_link(); a.finish();
}
void derived_exception_type(State& state) {
    HandlerAbi a(state); a.load(3, a.r(2) - 0x3ba0, 0x10772dd0); a.finish();
}
void base_exception_type(State& state) {
    HandlerAbi a(state); a.load(3, a.r(2) - 0x3ba4, 0x10772de0); a.finish();
}
void abort_facade(State& state) {
    HandlerAbi a(state);
    a.capture_link(); a.save_link(0x10772e74); a.frame(64, 0x10772e78);
    a.call(0x10773fb0, 0x10772e80); a.pop(64, 0x10772e80); a.restore_link(); a.finish();
}
void termination_facade(State& state) {
    HandlerAbi a(state);
    a.capture_link(); a.save_link(0x10772e94); a.frame(64, 0x10772e98);
    a.child(0x10772eb0, 0x10772ea0); a.pop(64, 0x10772ea0); a.restore_link(); a.finish();
}
void invoke_termination(State& state) { invoke_handler(state, 0x10772eb0, 0x3520); }
void invoke_unexpected(State& state) { invoke_handler(state, 0x10772ee0, 0x351c); }
}
