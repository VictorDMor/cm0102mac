#include "stream_buffer_internal.hpp"

namespace cm::aot::stream_buffer {
void dispose_all(Abi& a) {
    a.save(31,0x107753b0); a.capture_link(); a.load(31,a.r(2)-0x5a68,0x107753b8); a.save_link(0x107753bc); a.frame(64,0x107753c0);
    for(;;) {
        a.compare(a.r(31)); if(!a.r(31)) break;
        a.load<2>(0,a.r(31)+4,0x107753c8); a.put(0,(a.r(0)>>6)&7); a.record(a.r(0));
        if(a.r(0)) { a.put(3,a.r(31)); a.call(0x107766f0,0x107753dc); }
        a.put(3,a.r(31)); a.load(31,a.r(31)+80,0x107753e0); a.load<1>(0,a.r(3)+16,0x107753e4); a.compare(a.r(0));
        if(a.r(0)) a.call(0x10775170,0x107753f4);
        else {
            a.load<2>(0,a.r(3)+4,0x107753f8); a.put(4,3); a.put(0,(a.r(0)&~0x1c0u)|0xc0); a.compare(a.r(31)); a.store<2>(a.r(3)+4,a.r(0),0x10775408);
            if(a.r(31)) { a.load<1>(0,a.r(31)+16,0x10775410); a.compare(a.r(0)); if(a.r(0)) { a.put(0,0); a.store(a.r(3)+80,a.r(0),0x10775420); } }
        }
    }
    a.pop(64,0x1077542c); a.restore(31,0x10775434); a.restore_link(); a.finish();
}
void flush_all(Abi& a,bool line_buffered) {
    const auto e=line_buffered?0x107754b0u:0x10775440u;
    a.save(31,e); a.capture_link(); a.put(31,0); a.save(30,e+12); a.load(30,a.r(2)-0x5a68,e+16); a.save_link(e+20); a.frame(64,e+24);
    for(;;) {
        a.compare(a.r(30)); if(!a.r(30)) break;
        a.load<2>(0,a.r(30)+4,e+32); a.put(0,(a.r(0)>>6)&7); a.record(a.r(0)); bool eligible=a.r(0)!=0;
        if(eligible&&line_buffered) {
            a.load<1>(0,a.r(30)+4,e+44); a.put(0,(a.r(0)>>1)&1); a.record(a.r(0)); eligible=a.r(0)!=0;
            if(eligible) { a.load<1>(0,a.r(30)+8,e+56); a.put(0,(a.r(0)>>5)&7); a.compare(a.r(0),1); eligible=a.r(0)==1; }
        }
        if(eligible) {
            a.put(3,a.r(30)); a.call(0x107767b0,line_buffered?e+80:e+52); a.compare(std::bit_cast<std::int32_t>(a.r(3)));
            if(a.r(3)) a.put(31,UINT32_MAX);
        }
        a.load(30,a.r(30)+80,line_buffered?e+92:e+64);
    }
    a.put(3,a.r(31)); const auto tail=line_buffered?e+108:e+80;
    a.pop(64,tail); a.restore_link(); a.restore(31,tail+12); a.restore(30,tail+16); a.finish();
}
void close(Abi& a) {
    a.save(31,0x107766f0); a.capture_link(); a.save(30,0x107766f8); a.save(29,0x107766fc); a.put(29,a.r(3)); a.record(a.r(29));
    a.save_link(0x10776704); a.frame(80,0x10776708);
    if(!a.r(29)) a.put(3,UINT32_MAX);
    else {
        a.load<2>(0,a.r(29)+4,0x10776718); a.put(0,(a.r(0)>>6)&7); a.record(a.r(0));
        if(!a.r(0)) a.put(3,0);
        else {
            a.call(0x107767b0,0x10776730); a.put(30,a.r(3)); a.load(3,a.r(29),0x10776734); a.load(12,a.r(29)+72,0x10776738); a.callback(0x10776740);
            a.load<2>(0,a.r(29)+4,0x10776744); a.put(4,0); a.put(0,a.r(0)&~0x1c0u); a.store<2>(a.r(29)+4,a.r(0),0x10776750);
            a.put(31,a.r(3)); a.store(a.r(29),a.r(4),0x10776758); a.load<1>(0,a.r(29)+8,0x1077675c); a.put(0,(a.r(0)>>4)&1); a.record(a.r(0));
            if(a.r(0)) { a.load(3,a.r(29)+32,0x10776768); a.call(0x10775170,0x10776770); }
            a.compare(std::bit_cast<std::int32_t>(a.r(30))); a.put(3,0);
            if(!a.r(30)) a.compare(std::bit_cast<std::int32_t>(a.r(31)));
            if(a.r(30)||a.r(31)) a.put(3,1);
            a.put(0,0-a.r(3)); a.put(0,a.r(0)|a.r(3)); a.s.wr<1>(1027,(a.r(0)&0x80000000u)&&(a.r(0)&0x7fffffffu)); a.put(3,a.r(0)&0x80000000u?UINT32_MAX:0);
        }
    }
    a.pop(80,0x10776794); a.restore(31,0x1077679c); a.restore(30,0x107767a0); a.restore_link(); a.restore(29,0x107767a8); a.finish();
}
void flush(Abi& a) {
    a.save(31,0x107767b0); a.capture_link(); a.put(31,a.r(3)); a.record(a.r(31)); a.save(30,0x107767bc); a.save_link(0x107767c0); a.frame(64,0x107767c4);
    if(!a.r(31)) { a.call(0x10775440,0x107767d0); goto done; }
    a.load<1>(0,a.r(31)+13,0x107767d4); a.compare(a.r(0));
    {
        bool invalid=a.r(0)!=0;
        if(!invalid) { a.load<2>(0,a.r(31)+4,0x107767e0); a.put(0,(a.r(0)>>6)&7); a.record(a.r(0)); invalid=!a.r(0); }
        if(invalid) { a.put(3,UINT32_MAX); goto done; }
    }
    a.load<1>(0,a.r(31)+4,0x107767f4); a.put(0,(a.r(0)>>3)&7); a.compare(a.r(0),1);
    if(a.r(0)==1) { a.put(3,0); goto done; }
    a.load<1>(3,a.r(31)+8,0x1077680c); a.put(0,(a.r(3)>>5)&7); a.compare(a.r(0),3);
    if(a.r(0)>=3) { a.put(0,2); a.put(3,(a.r(3)&~0xe0u)|0x40); a.store<1>(a.r(31)+8,a.r(3),0x10776824); }
    a.load<1>(0,a.r(31)+8,0x10776828); a.put(0,(a.r(0)>>5)&7); a.compare(a.r(0),2);
    if(a.r(0)==2) { a.put(0,0); a.store(a.r(31)+44,a.r(0),0x1077683c); }
    a.load<1>(3,a.r(31)+8,0x10776840); a.put(0,(a.r(3)>>5)&7); a.compare(a.r(0),1);
    if(a.r(0)!=1) { a.put(0,0); a.put(3,a.r(3)&~0xe0u); a.store<1>(a.r(31)+8,a.r(3),0x10776858); a.put(3,0); goto done; }
    a.load<2>(0,a.r(31)+4,0x10776864); a.put(0,(a.r(0)>>6)&7); a.compare(std::bit_cast<std::int32_t>(a.r(0)),1);
    if(a.r(0)!=1) a.put(30,0);
    else { a.put(3,a.r(31)); a.call(0x10777cc0,0x10776884); a.put(30,a.r(3)); }
    a.put(3,a.r(31)); a.put(4,0); a.call(0x107758a0,0x10776894); a.compare(std::bit_cast<std::int32_t>(a.r(3)));
    if(a.r(3)) { a.put(0,1); a.store<1>(a.r(31)+13,a.r(0),0x107768a0); a.put(0,0); a.put(3,UINT32_MAX); a.store(a.r(31)+44,a.r(0),0x107768ac); }
    else {
        a.load<1>(0,a.r(31)+8,0x107768b4); a.put(4,0); a.put(0,a.r(0)&~0xe0u); a.store<1>(a.r(31)+8,a.r(0),0x107768c0);
        a.put(3,0); a.store(a.r(31)+28,a.r(30),0x107768c8); a.store(a.r(31)+44,a.r(4),0x107768cc);
    }
done:
    a.pop(64,0x107768d0); a.restore(31,0x107768d8); a.restore_link(); a.restore(30,0x107768e0); a.finish();
}
}
