#include "stream_buffer_internal.hpp"
#include <algorithm>
#include <sstream>

namespace cm::aot {
bool try_modern_stream_buffer(State& s,std::uint32_t entry) {
    s.pc=entry;
    if(s.stream_buffer_original_resume==entry) { s.stream_buffer_original_resume=0; return false; }
    const auto found=std::find(stream_buffer_entries.begin(),stream_buffer_entries.end(),entry);
    if(found==stream_buffer_entries.end()) throw std::runtime_error("Unknown stream buffer entry");
    const auto index=std::size_t(found-stream_buffer_entries.begin());
    const bool original=s.stream_buffer_original||s.stream_buffer_entry_original[index];
    ++(original?s.stream_buffer_original_calls[index]:s.stream_buffer_calls[index]);
    if(original&&!s.stream_buffer_observer) return false;
    const auto caller=std::uint32_t(s.r<4>(4128));
    if(s.stream_buffer_observer) s.stream_buffer_observer->enter(s,entry,original);
    if(original) {
        s.stream_buffer_original_resume=entry;
        try { run_until(s,caller); } catch(...) { s.stream_buffer_original_resume=0; throw; }
    } else {
        s.step(entry); stream_buffer::Abi a{s};
        using namespace stream_buffer;
        switch(index) {
        case 0:acquire(a);break;
        case 1:initialize(a);break;
        case 2:dispose_all(a);break;
        case 3:flush_all(a,false);break;
        case 4:flush_all(a,true);break;
        case 5:translate_newlines(a,false);break;
        case 6:translate_newlines(a,true);break;
        case 7:prepare(a);break;
        case 8:refill(a);break;
        case 9:drain(a);break;
        case 10:configure(a);break;
        case 11:close(a);break;
        case 12:flush(a);break;
        }
    }
    if(s.stream_buffer_observer) s.stream_buffer_observer->leave(s,entry,original);
    return true;
}
void reset_stream_buffer_diagnostics(State& s) {
    s.stream_buffer_original=false; s.stream_buffer_entry_original.fill(false);
    s.stream_buffer_calls.fill(0); s.stream_buffer_original_calls.fill(0);
    s.stream_buffer_original_resume=0; s.stream_buffer_dispatch_escapes=0; s.stream_buffer_observer=nullptr;
}
std::string stream_buffer_call_summary(const State& s) {
    std::ostringstream out;
    for(unsigned i=0;i<stream_buffer_entries.size();++i)
        out<<"stream_buffer.entry="<<hex_address(stream_buffer_entries[i])<<" mode="
           <<(s.stream_buffer_original||s.stream_buffer_entry_original[i]?"original":"modern")
           <<" modern="<<s.stream_buffer_calls[i]<<" original="<<s.stream_buffer_original_calls[i]<<'\n';
    out<<"stream_buffer.dispatch_escapes="<<s.stream_buffer_dispatch_escapes<<'\n';
    return out.str();
}
}
