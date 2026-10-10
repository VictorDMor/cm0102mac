#pragma once
#include "allocator_adapter.hpp"
#include "allocator_bridge.hpp"
#include <string_view>

namespace cm::aot::allocator {
inline constexpr std::uint32_t size_mask=0xfffffff0, in_use=2, previous_busy=4;
// The semantic heap operations retain the Mac call ABI and access order.
// GPR scratch is observable at returns and faults; it is not host heap state.
struct Abi {
    State& s;
    std::uint32_t r(unsigned n) const { return std::uint32_t(s.r<4>(n*4)); }
    void put(unsigned n,std::uint32_t value) { s.wr<4>(n*4,value); }
    void compare(std::int64_t a,std::int64_t b=0,bool record=false) {
        s.wr<1>(2304,(record?s.r<1>(2304)&0xf0:0)|(a<b?8:a>b?4:2)|(s.r<1>(1024)&1));
    }
    void record(std::uint32_t value) { compare(std::bit_cast<std::int32_t>(value),0,true); }
    template<unsigned N=4> void load(unsigned dest,std::uint32_t address,std::uint32_t pc) {
        s.pc=pc; put(dest,std::uint32_t(s.memory.read<N>(address)));
    }
    template<unsigned N=4> void store(std::uint32_t address,std::uint32_t value,std::uint32_t pc) {
        s.pc=pc; s.memory.write<N>(address,value);
    }
    void save(unsigned n,std::uint32_t pc) { store(r(1)-(32-n)*4,r(n),pc); }
    void restore(unsigned n,std::uint32_t pc) { load(n,r(1)-(32-n)*4,pc); }
    void words(unsigned first,std::uint32_t pc,bool restoring=false) {
        s.wr<4>(2432,r(1)-(32-first)*4);
        for(unsigned n=first;n<32;++n) {
            const auto address=std::uint32_t(s.r<4>(2432));
            if(restoring) load(n,address,pc); else store(address,r(n),pc);
            s.wr<4>(2432,address+4);
        }
    }
    void capture_link() { put(0,std::uint32_t(s.r<4>(4128))); }
    void save_link(std::uint32_t pc) { store(r(1)+8,r(0),pc); }
    void frame(unsigned size,std::uint32_t pc) { const auto old=r(1); store(old-size,old,pc); put(1,old-size); }
    void pop(unsigned size,std::uint32_t pc) { load(0,r(1)+size+8,pc); put(1,r(1)+size); }
    void restore_link() { s.wr<4>(4128,r(0)); }
    void finish() { s.pc=std::uint32_t(s.r<4>(4128)); }
    void call(std::uint32_t entry,std::uint32_t link) {
        s.wr<4>(2440,r(2)); s.wr<4>(4128,link); s.pc=entry;
        try { run_until(s,link); }
        catch(const std::runtime_error& error) {
            if(std::string_view(error.what()).starts_with("Uncompiled original address:"))
                ++s.allocator_dispatch_escapes;
            throw;
        }
    }
    bool count_down() { s.wr<4>(4132,std::uint32_t(s.r<4>(4132))-1); return s.r<4>(4132)!=0; }
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
