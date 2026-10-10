#pragma once
#include "stream_io_adapter.hpp"
#include "mac_abi.hpp"

namespace cm::aot::stream_io {
struct Abi : MacAbi {
    explicit Abi(State& state):MacAbi(state,state.stream_io_dispatch_escapes) {}
    std::int32_t signed_r(unsigned n) const { return std::bit_cast<std::int32_t>(r(n)); }
    void decrement_record(unsigned dest,unsigned source) {
        const auto value=r(source);
        s.wr<1>(1027,value!=0); put(dest,value-1); record(r(dest));
    }
};
void byte_orientation(Abi&);
void read_byte_slow(Abi&);
void read_byte(Abi&);
void read_line(Abi&);
void push_back_byte(Abi&);
void read_items(Abi&);
void write_items(Abi&);
void items_facade(Abi&,bool writing);
}
