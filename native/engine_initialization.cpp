#include "engine_initialization.hpp"
#include "guest_context_transfer.hpp"
#include <algorithm>

namespace cm::aot::engine_initialization {
namespace {
// The ABI exposes intermediate register values, live TOC slots and distinct
// fault PCs. Named initialization operations keep these checkpoints without
// interpreting instructions or replaying the replaced guest body.
class InitializerCall {
    State& state;
    std::uint32_t next;
    GuestNativeScope scope;
    DependencyDispatch* dependencies;
    void checkpoint() { state.step(next); next += 4; }
    std::uint32_t stack() const { return std::uint32_t(state.r<4>(4)); }
public:
    InitializerCall(State& s, std::uint32_t entry, DependencyDispatch* d)
        : state(s), next(entry), scope(s, entry), dependencies(d) {}
    void capture_link() { checkpoint(); state.wr<4>(0, state.r<4>(4128)); }
    void save_link() { checkpoint(); state.memory.write<4>(stack() + 8, state.r<4>(0)); }
    void save_owner() { checkpoint(); state.memory.write<4>(stack() - 4, state.r<4>(124)); }
    void global(unsigned reg, std::int32_t displacement) {
        checkpoint(); const auto address = std::uint32_t(state.r<4>(8) + displacement);
        state.wr<4>(reg * 4, state.memory.read<4>(address));
    }
    void argument(unsigned reg, std::uint32_t value) { checkpoint(); state.wr<4>(reg * 4, value); }
    void offset(unsigned reg, std::int32_t amount) {
        checkpoint(); state.wr<4>(reg * 4, state.r<4>(reg * 4) + amount);
    }
    void frame(unsigned size = 64) {
        checkpoint(); const auto previous = stack(), current = previous - size;
        state.memory.write<4>(current, previous); state.wr<4>(4, current); scope.frame(current);
    }
    void stack_object(unsigned displacement) {
        checkpoint(); state.wr<4>(12, stack() + displacement);
    }
    void call(std::uint32_t target) {
        checkpoint(); state.wr<4>(2440, state.r<4>(8)); state.wr<4>(4128, next); state.pc = target;
        if (dependencies) dependencies->call(state, target, next);
        else run_until(state, next);
    }
    void construct_array() { call(0x10773320); }
    void register_destructor() { call(0x10772df0); }
    void clear_date() { call(0x10715880); }
    void construct_date() { call(0x107156f0); }
    void owner_destructor() { checkpoint(); state.wr<4>(16, state.r<4>(124)); }
    void publish_initial_date() {
        checkpoint(); state.wr<4>(0, std::uint64_t(signed_value<2>(state.memory.read<2>(stack() + 56))));
        checkpoint(); state.wr<4>(12, state.memory.read<2>(stack() + 58));
        checkpoint(); state.memory.write<2>(state.r<4>(124), state.r<4>(0));
        checkpoint(); state.wr<4>(0, state.memory.read<1>(stack() + 60));
        checkpoint(); state.memory.write<2>(std::uint32_t(state.r<4>(124) + 2), state.r<4>(12));
        checkpoint(); state.memory.write<1>(std::uint32_t(state.r<4>(124) + 4), state.r<4>(0));
    }
    void clear_registered_object() {
        checkpoint(); state.memory.write<4>(std::uint32_t(state.r<4>(12) + 12), state.r<4>(0));
        checkpoint(); state.memory.write<1>(state.r<4>(12), state.r<4>(0));
        checkpoint(); state.memory.write<4>(std::uint32_t(state.r<4>(12) + 8), state.r<4>(0));
    }
    void restore_link(unsigned size = 64) {
        checkpoint(); state.wr<4>(0, state.memory.read<4>(stack() + size + 8));
        checkpoint(); state.wr<4>(4, stack() + size);
        checkpoint(); state.wr<4>(4128, state.r<4>(0));
    }
    void restore_owner() { checkpoint(); state.wr<4>(124, state.memory.read<4>(stack() - 4)); }
    void return_to_caller() { checkpoint(); state.pc = std::uint32_t(state.r<4>(4128)); }
    void return_from_root() {
        // The root increments SP before loading its saved link. An observer
        // during that read can change SP; the children restore in another order.
        checkpoint(); state.wr<4>(4, stack() + 64);
        checkpoint(); state.wr<4>(0, state.memory.read<4>(stack() + 8));
        checkpoint(); state.wr<4>(4128, state.r<4>(0)); return_to_caller();
    }
};
void initialize_date_group(InitializerCall& call, std::int32_t first, unsigned count) {
    call.capture_link(); call.global(3, first); call.save_link(); call.frame(); call.clear_date();
    for (unsigned i = 1; i < count; ++i) { call.global(3, first - std::int32_t(i * 4)); call.clear_date(); }
}
void initialize_date_pair(InitializerCall& call, std::int32_t first, std::int32_t second) {
    initialize_date_group(call, first, 1); call.global(3, second); call.clear_date();
}
void initialize_plain_array(InitializerCall& call, std::int32_t object, std::int32_t constructor,
                            unsigned size, unsigned count) {
    call.capture_link(); call.global(3, object); call.save_link(); call.argument(5, 0);
    call.global(4, constructor); call.argument(6, size); call.frame(); call.argument(7, count); call.construct_array();
}
void initialize_registered_array(InitializerCall& call, std::int32_t object, std::int32_t constructor,
                                 std::int32_t destructor, unsigned size, unsigned count,
                                 std::int32_t finalizer, std::int32_t record) {
    call.capture_link(); call.global(3, object); call.save_link(); call.argument(6, size);
    call.global(4, constructor); call.argument(7, count); call.frame(); call.global(5, destructor); call.construct_array();
    call.global(4, finalizer); call.argument(3, 0); call.global(5, record); call.register_destructor();
}
void initialize_registered_object(InitializerCall& call, std::int32_t object, std::uint32_t constructor,
                                  std::int32_t destructor, std::int32_t record) {
    call.capture_link(); call.global(3, object); call.save_link(); call.frame(); call.call(constructor);
    call.global(4, destructor); call.global(5, record); call.register_destructor();
}
void register_global_object(InitializerCall& call, std::int32_t object, std::int32_t destructor, std::int32_t record) {
    call.capture_link(); call.global(3, object); call.save_link(); call.global(4, destructor);
    call.frame(); call.global(5, record); call.register_destructor();
}
}
unsigned entry_index(std::uint32_t entry) {
    const auto found = std::find(entries.begin(), entries.end(), entry);
    if (found == entries.end()) throw std::runtime_error("Unknown engine initializer");
    return unsigned(found - entries.begin());
}
void invoke(State& state, unsigned index, DependencyDispatch* dependencies) {
    if (index >= entries.size()) throw std::runtime_error("Unknown engine initializer index");
    InitializerCall call(state, entries[index], dependencies);
    switch (index) {
    case 0:
        call.capture_link(); call.save_link(); call.frame();
        for (unsigned child = 1; child < entries.size(); ++child) call.call(entries[child]);
        call.return_from_root(); return;
    case 1:
        call.capture_link(); call.global(3, -0x5810); call.save_link(); call.argument(5, 0);
        call.global(4, -0x7acc); call.argument(6, 0x664); call.frame(); call.argument(7, 2); call.offset(3, 0x120); call.construct_array();
        call.global(3, -0x5870); call.argument(6, 0x2c); call.global(4, -0x7b2c); call.argument(7, 16);
        call.global(5, -0x7b30); call.construct_array();
        call.global(4, -0x598c); call.argument(3, 0); call.global(5, -0x5990); call.register_destructor(); break;
    case 2: initialize_date_group(call, -0x799c, 16); break;
    case 3:
        call.capture_link(); call.save_owner(); call.global(31, -0x79e8); call.argument(4, 31); call.save_link();
        call.argument(5, 7); call.argument(6, 2001); call.argument(7, UINT32_MAX); call.frame(80); call.stack_object(56);
        call.construct_date(); call.publish_initial_date(); call.restore_link(80); call.restore_owner(); call.return_to_caller(); return;
    case 4: initialize_date_pair(call, -0x7b80, -0x7434); break;
    case 5: initialize_plain_array(call, -0x54d4, -0x7404, 0x78, 0xa8c); break;
    case 6: initialize_plain_array(call, -0x7b44, -0x64cc, 0x4c, 0x22); break;
    case 7: initialize_registered_array(call, -0x5158, -0x63e8, -0x63ec, 0xfc, 16, -0x5164, -0x5168); break;
    case 8: initialize_date_pair(call, -0x5038, -0x503c); break;
    case 9: initialize_registered_object(call, -0x6324, 0x10738c00, -0x6314, -0x502c); break;
    case 10: initialize_date_pair(call, -0x7a9c, -0x63c8); break;
    case 11:
        call.capture_link(); call.global(3, -0x628c); call.save_link(); call.argument(4, 23); call.argument(5, 6);
        call.argument(6, 1971); call.frame(); call.argument(7, UINT32_MAX); call.construct_date(); break;
    case 12: initialize_registered_object(call, -0x6178, 0x10738c00, -0x6314, -0x47a8); break;
    case 13: register_global_object(call, -0x6340, -0x5d84, -0x4178); break;
    case 14: register_global_object(call, -0x633c, -0x5d74, -0x4160); break;
    case 15:
        call.capture_link(); call.save_owner(); call.argument(4, 65536); call.global(31, -0x5c28); call.save_link();
        call.offset(4, -0x3cb0); call.global(3, -0x5c04); call.frame(); call.call(0x1073ae10);
        call.global(5, -0x3f98); call.owner_destructor(); call.register_destructor();
        call.global(3, -0x5c08); call.argument(4, 0xc00); call.call(0x1073ae10);
        call.global(5, -0x3f9c); call.owner_destructor(); call.register_destructor();
        call.restore_link(); call.restore_owner(); call.return_to_caller(); return;
    case 16:
        call.capture_link(); call.global(3, -0x3e70); call.save_link(); call.global(4, -0x5c38); call.frame();
        call.global(5, -0x5bc0); call.call(0x1074dec0);
        call.global(4, -0x5bc4); call.global(5, -0x3e88); call.register_destructor(); break;
    case 17: initialize_registered_object(call, -0x7bb0, 0x1073c1b0, -0x5ba4, -0x3e64); break;
    case 18:
        call.capture_link(); call.global(3, -0x7408); call.save_link(); call.argument(0, 0); call.global(4, -0x5b94);
        call.frame(); call.global(5, -0x3e1c); call.clear_registered_object(); call.register_destructor(); break;
    case 19: initialize_registered_array(call, -0x3bd8, -0x5b18, -0x5b1c, 4, 8, -0x3bf8, -0x3bfc); break;
    default: throw std::runtime_error("Unknown engine initializer index");
    }
    call.restore_link(); call.return_to_caller();
}
}
