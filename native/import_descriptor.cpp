#include "import_descriptor.hpp"

namespace cm::aot::import_descriptor {
namespace {
// These leaf tails allocate no frame and create no callback owner. The caller's
// dispatcher owns the destination, including any nonlocal guest transfer there.
struct TailAbi {
    State& state;
    std::uint32_t next;
    void checkpoint() { state.step(next); next += 4; }
    void resolve_slot(std::int16_t displacement) {
        checkpoint();
        const auto slot = std::uint32_t(state.r<4>(8) + std::int32_t(displacement));
        state.wr<4>(48, state.memory.read<4>(slot));
    }
    void preserve_caller_toc() {
        checkpoint();
        const auto saved = std::uint32_t(state.r<4>(4) + 20);
        state.memory.write<4>(saved, state.r<4>(8));
    }
    void read_destination() {
        checkpoint();
        const auto descriptor = std::uint32_t(state.r<4>(48));
        state.wr<4>(0, state.memory.read<4>(descriptor));
    }
    void install_destination_toc() {
        checkpoint();
        const auto toc = std::uint32_t(state.r<4>(48) + 4);
        state.wr<4>(8, state.memory.read<4>(toc));
    }
    void select_destination() { checkpoint(); state.wr<4>(4132, state.r<4>(0)); }
    void tail() { checkpoint(); state.pc = std::uint32_t(state.r<4>(4132)); }
};
void call_import(TailAbi& abi, std::int16_t displacement) {
    abi.resolve_slot(displacement);
    abi.preserve_caller_toc();
    abi.read_destination();
    abi.install_destination_toc();
    abi.select_destination();
    abi.tail();
}
void call_descriptor(TailAbi& abi) {
    // Generic CFM callbacks capture the function before saving caller TOC and
    // publish CTR before reading callee TOC. Aliasing makes this order visible.
    abi.read_destination();
    abi.preserve_caller_toc();
    abi.select_destination();
    abi.install_destination_toc();
    abi.tail();
}
}
unsigned entry_index(std::uint32_t entry) {
    if (entry == generic_entry) return import_count;
    if (entry >= first_entry && entry < generic_entry && (entry - first_entry) % 24 == 0)
        return (entry - first_entry) / 24;
    throw std::runtime_error("Unknown import-descriptor entry");
}
void invoke(State& state, unsigned index) {
    if (index >= entry_count) throw std::runtime_error("Unknown import-descriptor index");
    TailAbi abi{state, entries[index]};
    if (index == import_count) call_descriptor(abi);
    else call_import(abi, toc_displacements[index]);
}
}
