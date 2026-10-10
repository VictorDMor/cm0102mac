#pragma once
#include "allocator_adapter.hpp"
#include "allocator_bridge.hpp"
#include "mac_abi.hpp"

namespace cm::aot::allocator {
inline constexpr std::uint32_t size_mask=0xfffffff0, in_use=2, previous_busy=4;
struct Abi : MacAbi {
    explicit Abi(State& state):MacAbi(state,state.allocator_dispatch_escapes) {}
};
void initialize_arena(Abi&);
void allocate_block(Abi&);
void release_block(Abi&);
void mark_and_unlink(Abi&);
void initialize_block(Abi&);
void split_block(Abi&);
void coalesce_previous(Abi&);
void coalesce_next(Abi&);
void insert_arena(Abi&);
void remove_arena(Abi&);
void acquire_arena(Abi&);
void allocate_growing(Abi&);
void allocate_existing(Abi&);
void release_ordinary(Abi&);
void initialize_pool(Abi&);
void clear_context(Abi&);
void global_context(Abi&);
void allocate_small(Abi&);
void release_small(Abi&);
void allocate(Abi&);
void release(Abi&);
void reallocate(Abi&);
void allocate_clear(Abi&);
void global_allocate(Abi&,bool releasing);
void global_reallocate(Abi&,bool clearing);
}
