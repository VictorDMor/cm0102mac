#include "stream_io_internal.hpp"
#include <algorithm>
#include <sstream>

namespace cm::aot {
bool try_modern_stream_io(State& s,std::uint32_t entry) {
    s.pc=entry;
    if(s.stream_io_original_resume==entry) { s.stream_io_original_resume=0; return false; }
    const auto found=std::find(stream_io_entries.begin(),stream_io_entries.end(),entry);
    if(found==stream_io_entries.end()) throw std::runtime_error("Unknown stream I/O entry");
    const auto index=std::size_t(found-stream_io_entries.begin());
    const bool original=s.stream_io_original||s.stream_io_entry_original[index];
    ++(original?s.stream_io_original_calls[index]:s.stream_io_calls[index]);
    if(original&&!s.stream_io_observer) return false;
    const auto caller=std::uint32_t(s.r<4>(4128));
    if(s.stream_io_observer) s.stream_io_observer->enter(s,entry,original);
    if(original) {
        s.stream_io_original_resume=entry;
        try { run_until(s,caller); } catch(...) { s.stream_io_original_resume=0; throw; }
    } else {
        s.step(entry); stream_io::Abi a{s};
        using namespace stream_io;
        switch(index) {
        case 0:read_byte_slow(a);break;
        case 1:read_byte(a);break;
        case 2:read_line(a);break;
        case 3:push_back_byte(a);break;
        case 4:items_facade(a,false);break;
        case 5:read_items(a);break;
        case 6:items_facade(a,true);break;
        case 7:write_items(a);break;
        case 8:byte_orientation(a);break;
        }
    }
    if(s.stream_io_observer) s.stream_io_observer->leave(s,entry,original);
    return true;
}
void reset_stream_io_diagnostics(State& s) {
    s.stream_io_original=false; s.stream_io_entry_original.fill(false);
    s.stream_io_calls.fill(0); s.stream_io_original_calls.fill(0);
    s.stream_io_original_resume=0; s.stream_io_dispatch_escapes=0; s.stream_io_observer=nullptr;
}
std::string stream_io_call_summary(const State& s) {
    std::ostringstream out;
    for(unsigned i=0;i<stream_io_entries.size();++i)
        out<<"stream_io.entry="<<hex_address(stream_io_entries[i])<<" mode="
           <<(s.stream_io_original||s.stream_io_entry_original[i]?"original":"modern")
           <<" modern="<<s.stream_io_calls[i]<<" original="<<s.stream_io_original_calls[i]<<'\n';
    out<<"stream_io.dispatch_escapes="<<s.stream_io_dispatch_escapes<<'\n';
    return out.str();
}
}
