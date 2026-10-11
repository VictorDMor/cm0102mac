#include "native_service.hpp"
#include "guest_context_transfer.hpp"
#include <algorithm>

namespace cm::aot::native_service {
namespace {
// Guest registers and saved words stay live across native calls and observers.
// Checkpoints retain the original instruction accounting and fault PCs; the
// service itself remains owned by the existing import dispatcher and bridges.
class ServiceCall {
    State& state;
    std::uint32_t next;
    GuestNativeScope scope;
    void checkpoint() { state.step(next); next += 4; }
    std::uint32_t stack() const { return std::uint32_t(state.r<4>(4)); }
public:
    ServiceCall(State& s, std::uint32_t entry) : state(s), next(entry), scope(s, entry) {}
    void preserve_caller() {
        checkpoint(); state.wr<4>(0, state.r<4>(4128));
        checkpoint(); state.memory.write<4>(stack() + 8, state.r<4>(0));
        checkpoint();
        const auto previous = stack(), frame = previous - 64;
        state.memory.write<4>(frame, previous);
        state.wr<4>(4, frame); scope.frame(frame);
    }
    void dispatch(std::uint32_t glue) {
        checkpoint();
        const auto continuation = next;
        state.wr<4>(2440, state.r<4>(8));
        state.wr<4>(4128, continuation); state.pc = glue;
        run_until(state, continuation);
    }
    void resume_caller() {
        checkpoint(); state.wr<4>(8, state.memory.read<4>(stack() + 20));
        checkpoint(); state.wr<4>(0, state.memory.read<4>(stack() + 72));
        checkpoint(); state.wr<4>(4, stack() + 64);
        checkpoint(); state.wr<4>(4128, state.r<4>(0));
        checkpoint(); state.pc = std::uint32_t(state.r<4>(4128));
    }
};
}
unsigned entry_index(std::uint32_t entry) {
    const auto found = std::find(entries.begin(), entries.end(), entry);
    if (found == entries.end()) throw std::runtime_error("Unknown native-service facade");
    return unsigned(found - entries.begin());
}
void invoke(State& state, Service service) {
    const auto index = unsigned(service);
    if (index >= entries.size()) throw std::runtime_error("Unknown native-service index");
    ServiceCall call(state, entries[index]);
    call.preserve_caller(); call.dispatch(import_glues[index]); call.resume_caller();
}
}
