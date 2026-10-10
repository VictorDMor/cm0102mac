#pragma once
#include "stream_buffer_adapter.hpp"
#include "mac_abi.hpp"

namespace cm::aot::stream_buffer {
struct Abi : MacAbi {
    explicit Abi(State& state):MacAbi(state,state.stream_buffer_dispatch_escapes) {}
    void callback(std::uint32_t link) { call(0x10781320,link); load(2,r(1)+20,link); }
};
void acquire(Abi&);
void initialize(Abi&);
void dispose_all(Abi&);
void flush_all(Abi&,bool line_buffered);
void translate_newlines(Abi&,bool input);
void prepare(Abi&);
void refill(Abi&);
void drain(Abi&);
void configure(Abi&);
void close(Abi&);
void flush(Abi&);
}
