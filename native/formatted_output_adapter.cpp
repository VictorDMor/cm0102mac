#include "formatted_output_internal.hpp"
#include <sstream>
namespace cm::aot {
bool try_modern_formatted_output(State& s,std::uint32_t entry){
    if(s.formatted_output_original_resume==entry){s.formatted_output_original_resume=0;return false;}
    const auto found=std::find(formatted_output_entries.begin(),formatted_output_entries.end(),entry);
    if(found==formatted_output_entries.end())throw std::runtime_error("Unknown formatted output entry");
    const auto index=std::size_t(found-formatted_output_entries.begin());const bool original=s.formatted_output_original||s.formatted_output_entry_original[index];
    ++(original?s.formatted_output_original_calls[index]:s.formatted_output_calls[index]);if(original&&!s.formatted_output_observer)return false;
    const auto caller=std::uint32_t(s.r<4>(4128));if(s.formatted_output_observer)s.formatted_output_observer->enter(s,entry,original);
    if(original){s.formatted_output_original_resume=entry;try{run_until(s,caller);}catch(...){s.formatted_output_original_resume=0;throw;}}
    else{
        s.step(entry);using namespace formatted_output;
        switch(index){
        case 0:parse_format(s);break;
        case 1:format_word32(s);break;
        case 2:format_integer(s);break;
        case 3:format_hex_float(s);break;
        case 4:round_decimal(s);break;
        case 5:reset_decimal_length(s);break;
        case 6:format_decimal_float(s);break;
        case 7:format_descriptor(s);break;
        case 8:stream_callback(s);break;
        case 9:bounded_callback(s);break;
        case 10:standard_stream_facade(s);break;
        case 11:explicit_stream_facade(s);break;
        case 12:bounded_vector_facade(s);break;
        case 13:unbounded_vector_facade(s);break;
        case 14:buffer_variadic_facade(s);break;
        }
    }
    if(s.formatted_output_observer)s.formatted_output_observer->leave(s,entry,original);return true;
}
bool try_modern_formatted_output_shared_tail(State& s){
    if(s.formatted_output_original||s.formatted_output_entry_original[4])return false;
    s.step(0x10779ed8);formatted_output::reset_decimal_tail(s);return true;
}
void reset_formatted_output_diagnostics(State& s){
    s.formatted_output_original=false;s.formatted_output_entry_original.fill(false);s.formatted_output_calls.fill(0);s.formatted_output_original_calls.fill(0);s.formatted_output_dispatch_escapes=0;s.formatted_output_shared_tail_calls=0;s.formatted_output_original_resume=0;s.formatted_output_observer=nullptr;
}
std::string formatted_output_call_summary(const State& s){
    std::ostringstream out;for(unsigned i=0;i<formatted_output_entries.size();++i)out<<"output.entry="<<hex_address(formatted_output_entries[i])<<" mode="<<(s.formatted_output_original||s.formatted_output_entry_original[i]?"original":"modern")<<" modern="<<s.formatted_output_calls[i]<<" original="<<s.formatted_output_original_calls[i]<<'\n';
    out<<"output.shared_reset="<<s.formatted_output_shared_tail_calls<<" escapes="<<s.formatted_output_dispatch_escapes<<'\n';return out.str();
}
}
