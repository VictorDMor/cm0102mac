#include "allocator_internal.hpp"
#include <algorithm>
#include <sstream>

namespace cm::aot {
bool try_modern_allocator(State& s,std::uint32_t entry) {
    s.pc=entry;
    if(s.allocator_original_resume==entry) { s.allocator_original_resume=0; return false; }
    const auto found=std::find(allocator_entries.begin(),allocator_entries.end(),entry);
    if(found==allocator_entries.end()) throw std::runtime_error("Unknown allocator entry");
    const auto index=std::size_t(found-allocator_entries.begin());
    const bool original=s.allocator_original||s.allocator_entry_original[index];
    ++(original?s.allocator_original_calls[index]:s.allocator_calls[index]);
    if(original&&!s.allocator_observer) return false;
    const auto caller=std::uint32_t(s.r<4>(4128));
    if(s.allocator_observer) s.allocator_observer->enter(s,entry,original);
    if(original) {
        s.allocator_original_resume=entry;
        try { run_until(s,caller); } catch(...) { s.allocator_original_resume=0; throw; }
    } else {
        s.step(entry); allocator::Abi a{s};
        using namespace allocator;
        switch(index) {
        case 0:initialize_arena(a);break;
        case 1:allocate_block(a);break;
        case 2:release_block(a);break;
        case 3:mark_and_unlink(a);break;
        case 4:initialize_block(a);break;
        case 5:split_block(a);break;
        case 6:coalesce_previous(a);break;
        case 7:coalesce_next(a);break;
        case 8:insert_arena(a);break;
        case 9:remove_arena(a);break;
        case 10:acquire_arena(a);break;
        case 11:allocate_growing(a);break;
        case 12:allocate_existing(a);break;
        case 13:release_ordinary(a);break;
        case 14:initialize_pool(a);break;
        case 15:clear_context(a);break;
        case 16:global_context(a);break;
        case 17:allocate_small(a);break;
        case 18:release_small(a);break;
        case 19:allocate(a);break;
        case 20:release(a);break;
        case 21:reallocate(a);break;
        case 22:allocate_clear(a);break;
        case 23:global_allocate(a,false);break;
        case 24:global_allocate(a,true);break;
        case 25:global_reallocate(a,false);break;
        case 26:global_reallocate(a,true);break;
        }
    }
    if(s.allocator_observer) s.allocator_observer->leave(s,entry,original);
    return true;
}
void reset_allocator_diagnostics(State& s) {
    s.allocator_original=false; s.allocator_entry_original.fill(false);
    s.allocator_calls.fill(0); s.allocator_original_calls.fill(0);
    s.allocator_original_resume=0; s.allocator_dispatch_escapes=0; s.allocator_observer=nullptr;
}
std::string allocator_call_summary(const State& s) {
    std::ostringstream out;
    for(unsigned i=0;i<allocator_entries.size();++i)
        out<<"allocator.entry="<<hex_address(allocator_entries[i])<<" mode="
           <<(s.allocator_original||s.allocator_entry_original[i]?"original":"modern")
           <<" modern="<<s.allocator_calls[i]<<" original="<<s.allocator_original_calls[i]<<'\n';
    out<<"allocator.dispatch_escapes="<<s.allocator_dispatch_escapes<<'\n';
    return out.str();
}
}
