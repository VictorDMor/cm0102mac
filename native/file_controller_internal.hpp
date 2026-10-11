#pragma once
#include "mac_abi.hpp"

namespace cm::aot::file_controller {
struct Abi : MacAbi {
    Abi(State& state, std::uint64_t& escapes) : MacAbi(state, escapes) {}
    std::int32_t signed_r(unsigned n) const { return std::bit_cast<std::int32_t>(r(n)); }
    void callback(std::uint32_t link) {
        call(0x10781320, link);
        load(2, r(1) + 20, link);
    }
    void native_call(std::uint32_t entry, std::uint32_t link) {
        call(entry, link); load(2, r(1) + 20, link);
    }
    void signed_word(unsigned dest, std::uint32_t value) { put(dest, std::uint32_t(std::int32_t(std::int16_t(value)))); }
    void signed_word_record(unsigned dest, std::uint32_t value) { signed_word(dest, value); record(r(dest)); }
    void signed_byte_record(unsigned dest, std::uint32_t value) {
        put(dest, std::uint32_t(std::int32_t(std::int8_t(value)))); record(r(dest));
    }
    void boolean_error() {
        put(0, 0 - r(3)); put(0, r(0) | r(3)); put(3, r(0) >> 31);
    }
};
void filename_facade(Abi&, bool rename);
void open_stream(Abi&);
void reopen_stream(Abi&);
void set_classic_finder(Abi&);
void transfer_file(Abi&, bool writing);
void open_file(Abi&);
void close_file(Abi&);
void remove_file(Abi&);
void rename_file(Abi&);
void seek_file(Abi&);
void parse_mode(Abi&);
void fork_capability(Abi&);
void find_file_record(Abi&);
void alias_policy(Abi&);
void metadata_code(Abi&, bool creator);
void tell_position(Abi&);
void tell_facade(Abi&);
void get_position(Abi&);
void seek_position(Abi&);
void seek_facade(Abi&);
void set_position(Abi&);
void clear_status(Abi&);
void initialize_stdio(Abi&);
}
