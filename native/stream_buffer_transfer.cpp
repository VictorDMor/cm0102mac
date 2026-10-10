#include "stream_buffer_internal.hpp"

namespace cm::aot::stream_buffer {
void translate_newlines(Abi& a,bool input) {
    const auto entry=input?0x10775740u:0x107756f0u;
    a.load(0,a.r(4),entry); a.put(4,input?13:10); a.compare(a.r(0));
    // The length is captured before any aliased byte store, as on Mac.
    a.s.wr<4>(4132,a.r(0)); a.put(0,input?10:13);
    if(a.s.r<4>(4132)) do {
        a.load<1>(5,a.r(3),entry+24); a.compare(a.r(5),input?10:13);
        if(a.r(5)==(input?10u:13u)) a.store<1>(a.r(3),a.r(4),entry+36);
        else { a.compare(a.r(5),input?13:10); if(a.r(5)==(input?13u:10u)) a.store<1>(a.r(3),a.r(0),entry+52); }
        a.put(3,a.r(3)+1);
    } while(a.count_down());
    a.finish();
}
void refill(Abi& a) {
    a.save(31,0x107757d0); a.capture_link(); a.put(31,a.r(5)); a.save(30,0x107757dc); a.put(30,a.r(4)); a.save(29,0x107757e4); a.put(29,a.r(3));
    a.save_link(0x107757ec); a.frame(80,0x107757f0); a.call(0x10775790,0x107757f8); a.compare(std::bit_cast<std::int32_t>(a.r(31)),1);
    if(a.r(31)==1) { a.load(0,a.r(29)+36,0x10775800); a.store(a.r(29)+44,a.r(0),0x10775804); }
    a.load(3,a.r(29),0x10775808); a.put(5,a.r(29)+44); a.load(4,a.r(29)+32,0x10775810); a.load(6,a.r(29)+76,0x10775814); a.load(12,a.r(29)+64,0x10775818);
    a.callback(0x10775820); a.compare(std::bit_cast<std::int32_t>(a.r(3)),2);
    if(a.r(3)==2) { a.put(0,0); a.store(a.r(29)+44,a.r(0),0x10775830); }
    a.compare(a.r(30)); if(a.r(30)) { a.load(0,a.r(29)+44,0x1077583c); a.store(a.r(30),a.r(0),0x10775840); }
    a.compare(std::bit_cast<std::int32_t>(a.r(3)));
    if(!a.r(3)) {
        a.load(3,a.r(29)+28,0x10775850); a.load(0,a.r(29)+44,0x10775854); a.put(0,a.r(3)+a.r(0)); a.store(a.r(29)+28,a.r(0),0x1077585c);
        a.load<1>(0,a.r(29)+5,0x10775860); a.put(0,(a.r(0)>>3)&1); a.record(a.r(0));
        if(!a.r(0)) { a.load(3,a.r(29)+32,0x1077586c); a.put(4,a.r(29)+44); a.call(0x10775740,0x10775878); }
        a.put(3,0);
    }
    a.pop(80,0x1077587c); a.restore(31,0x10775884); a.restore(30,0x10775888); a.restore_link(); a.restore(29,0x10775890); a.finish();
}
void drain(Abi& a) {
    a.save(31,0x107758a0); a.capture_link(); a.put(31,a.r(4)); a.save(30,0x107758ac); a.put(30,a.r(3)); a.save_link(0x107758b4); a.frame(64,0x107758b8);
    a.load(3,a.r(3)+32,0x107758bc); a.load(0,a.r(30)+40,0x107758c0); a.put(0,a.r(0)-a.r(3)); a.record(a.r(0));
    if(a.r(0)) {
        a.store(a.r(30)+44,a.r(0),0x107758cc); a.load<1>(0,a.r(30)+5,0x107758d0); a.put(0,(a.r(0)>>3)&1); a.record(a.r(0));
        if(!a.r(0)) { a.load(3,a.r(30)+32,0x107758dc); a.put(4,a.r(30)+44); a.call(0x107756f0,0x107758e8); }
        a.load(3,a.r(30),0x107758e8); a.put(5,a.r(30)+44); a.load(4,a.r(30)+32,0x107758f0); a.load(6,a.r(30)+76,0x107758f4); a.load(12,a.r(30)+68,0x107758f8);
        a.callback(0x10775900); a.compare(a.r(31)); if(a.r(31)) { a.load(0,a.r(30)+44,0x1077590c); a.store(a.r(31),a.r(0),0x10775910); }
        a.compare(std::bit_cast<std::int32_t>(a.r(3))); if(a.r(3)) goto done;
        a.load(3,a.r(30)+28,0x10775920); a.load(0,a.r(30)+44,0x10775924); a.put(0,a.r(3)+a.r(0)); a.store(a.r(30)+28,a.r(0),0x1077592c);
    }
    a.put(3,a.r(30)); a.call(0x10775790,0x10775938); a.put(3,0);
done:
    a.pop(64,0x1077593c); a.restore(31,0x10775944); a.restore_link(); a.restore(30,0x1077594c); a.finish();
}
}
