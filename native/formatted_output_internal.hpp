#pragma once
#include "formatted_output_adapter.hpp"
namespace cm::aot::formatted_output {
inline std::uint32_t reg(const State& s,unsigned n){return std::uint32_t(s.r<4>(n*4));}
inline void set(State& s,unsigned n,std::uint32_t value){s.wr<4>(n*4,value);}
inline std::int32_t word(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
inline std::uint32_t byte(std::uint32_t value){return std::uint32_t(signed_value<1>(value));}
inline void compare(State& s,std::int64_t a,std::int64_t b=0,bool record=false){
    s.wr<1>(2304,(record?s.r<1>(2304)&0xf0:0)|(a<b?8:a>b?4:2)|(s.r<1>(1024)&1));
}
template<unsigned N> inline std::uint64_t read(State& s,std::uint32_t address,std::uint32_t pc){s.pc=pc;return s.memory.read<N>(address);}
template<unsigned N> inline void write(State& s,std::uint32_t address,std::uint64_t value,std::uint32_t pc){s.pc=pc;s.memory.write<N>(address,value);}
inline void frame(State& s,unsigned size,std::uint32_t pc){const auto previous=reg(s,1);write<4>(s,previous-size,previous,pc);set(s,1,previous-size);}
inline void call(State& s,std::uint32_t entry,std::uint32_t link){s.wr<4>(2440,reg(s,2));s.wr<4>(4128,link);s.pc=entry;run_until(s,link);}
inline void finish(State& s){s.pc=std::uint32_t(s.r<4>(4128));}
inline void add_record(State& s,unsigned destination,unsigned source,std::int32_t amount){
    const auto sum=std::uint64_t(reg(s,source))+std::uint32_t(amount);s.wr<1>(1027,sum>>32);set(s,destination,std::uint32_t(sum));compare(s,word(reg(s,destination)),0,true);
}
inline void words(State& s,unsigned first,std::uint32_t pc,bool restore,unsigned displacement=0){
    s.wr<4>(2432,reg(s,1)-(displacement?displacement:(32-first)*4));
    for(unsigned n=first;n<32;++n){const auto address=std::uint32_t(s.r<4>(2432));if(restore)set(s,n,read<4>(s,address,pc));else write<4>(s,address,reg(s,n),pc);s.wr<4>(2432,address+4);}
}
void parse_format(State&);
void format_word32(State&);
void format_integer(State&);
void format_hex_float(State&);
void round_decimal(State&);
void reset_decimal_tail(State&);
void reset_decimal_length(State&);
void format_decimal_float(State&);
void format_descriptor(State&);
void stream_callback(State&);
void bounded_callback(State&);
void standard_stream_facade(State&);
void explicit_stream_facade(State&);
void bounded_vector_facade(State&);
void unbounded_vector_facade(State&);
void buffer_variadic_facade(State&);
}
