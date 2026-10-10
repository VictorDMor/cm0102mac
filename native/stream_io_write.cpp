#include "stream_io_internal.hpp"

namespace cm::aot::stream_io {
void write_items(Abi& a) {
    a.words(25,0x10776380); a.capture_link(); a.put(25,a.r(4)); a.put(26,a.r(6)); a.put(27,a.r(3)); a.put(28,a.r(5)); a.put(3,a.r(26)); a.put(4,0); a.save_link(0x107763a0); a.frame(96,0x107763a4);
    a.call(0x1077fa50,0x107763ac); a.compare(a.signed_r(3));
    if(!a.r(3)) { a.put(3,a.r(26)); a.put(4,UINT32_MAX); a.call(0x1077fa50,0x107763c0); }
    a.put(29,a.r(25)*a.r(28)); a.record(a.r(29)); bool invalid=!a.r(29);
    if(!invalid) { a.load<1>(0,a.r(26)+13,0x107763c8); a.compare(a.r(0)); invalid=a.r(0)!=0; }
    if(!invalid) { a.load<2>(0,a.r(26)+4,0x107763d4); a.put(0,(a.r(0)>>6)&7); a.record(a.r(0)); invalid=!a.r(0); }
    if(invalid) { a.put(3,0); goto done; }
    a.compare(a.signed_r(0),2); if(a.r(0)==2) a.call(0x107784b0,0x107763f4);
    a.load<1>(0,a.r(26)+5,0x107763f4); a.put(31,1); a.put(3,0); a.put(0,(a.r(0)>>3)&1); a.record(a.r(0));
    if(a.r(0)) { a.load<1>(0,a.r(26)+4,0x10776408); a.put(0,(a.r(0)>>1)&3); a.compare(a.r(0),2); if(a.r(0)==2) a.put(3,1); }
    else a.put(3,1);
    a.compare(a.signed_r(3));
    if(!a.r(3)) { a.load<1>(0,a.r(26)+4,0x10776424); a.put(0,(a.r(0)>>1)&3); a.compare(a.r(0),1); if(a.r(0)!=1) a.put(31,0); }
    a.load<1>(0,a.r(26)+8,0x10776438); a.put(0,(a.r(0)>>5)&7); a.record(a.r(0));
    if(!a.r(0)) {
        a.load<1>(3,a.r(26)+4,0x10776444); a.put(0,(a.r(3)>>3)&2); a.record(a.r(0)); const bool writable=a.r(0)!=0; a.put(0,(a.r(3)>>3)&7);
        if(writable) {
            a.put(0,a.r(0)&4); a.record(a.r(0));
            if(a.r(0)) { a.put(3,a.r(26)); a.put(4,0); a.put(5,2); a.call(0x10777f10,0x1077646c); a.compare(a.signed_r(3)); if(a.r(3)) { a.put(3,0); goto done; } }
            a.load<1>(0,a.r(26)+8,0x1077647c); a.put(3,1); a.put(0,(a.r(0)&~0xe0u)|0x20); a.store<1>(a.r(26)+8,a.r(0),0x10776488); a.put(3,a.r(26)); a.call(0x10775790,0x10776494);
        }
    }
    a.load<1>(0,a.r(26)+8,0x10776494); a.put(0,(a.r(0)>>5)&7); a.compare(a.r(0),1);
    if(a.r(0)!=1) { a.put(0,1); a.store<1>(a.r(26)+13,a.r(0),0x107764a8); a.put(0,0); a.put(3,0); a.store(a.r(26)+44,a.r(0),0x107764b4); goto done; }
    a.compare(a.r(29)); a.put(30,a.r(27)); a.put(28,0);
    if(a.r(29)) {
        a.load(4,a.r(26)+40,0x107764cc); a.load(3,a.r(26)+32,0x107764d0); a.compare(a.r(4),a.r(3));
        if(a.r(4)==a.r(3)) { a.compare(a.signed_r(31)); if(!a.r(31)) goto after_buffer; }
        a.load(0,a.r(26)+36,0x107764e4); a.put(3,a.r(4)-a.r(3)); a.put(0,a.r(0)-a.r(3)); a.store(a.r(26)+44,a.r(0),0x107764f0);
        for(;;) {
            a.load(0,a.r(26)+44,0x107764f4); a.put(27,0); a.compare(a.r(0),a.r(29)); a.store(a.r(1)+56,a.r(0),0x10776500);
            if(a.r(0)>a.r(29)) a.store(a.r(1)+56,a.r(29),0x10776508);
            a.load<1>(0,a.r(26)+4,0x1077650c); a.put(0,(a.r(0)>>1)&3); a.compare(a.r(0),1);
            if(a.r(0)==1) {
                a.load(5,a.r(1)+56,0x1077651c); a.compare(a.r(5));
                if(a.r(5)) { a.put(3,a.r(30)); a.put(4,10); a.call(0x10778360,0x10776534); a.put(27,a.r(3)); a.record(a.r(27)); if(a.r(27)) { a.put(0,a.r(27)+1); a.put(0,a.r(0)-a.r(30)); a.store(a.r(1)+56,a.r(0),0x10776544); } }
            }
            a.load(5,a.r(1)+56,0x10776548); a.compare(a.r(5));
            if(a.r(5)) {
                a.load(3,a.r(26)+40,0x10776554); a.put(4,a.r(30)); a.call(0x10739ef0,0x10776560); a.load(3,a.r(1)+56,0x10776560); a.load(0,a.r(26)+40,0x10776564);
                a.put(30,a.r(30)+a.r(3)); a.put(0,a.r(0)+a.r(3)); a.store(a.r(26)+40,a.r(0),0x10776570); a.put(28,a.r(28)+a.r(3)); a.put(29,a.r(29)-a.r(3));
                a.load(3,a.r(1)+56,0x1077657c); a.load(0,a.r(26)+44,0x10776580); a.put(0,a.r(0)-a.r(3)); a.store(a.r(26)+44,a.r(0),0x10776588);
            }
            a.load(0,a.r(26)+44,0x1077658c); a.compare(a.r(0)); bool flush=!a.r(0);
            if(!flush) { a.compare(a.r(27)); flush=a.r(27)!=0; }
            if(!flush) { a.load<1>(0,a.r(26)+4,0x107765a0); a.put(0,(a.r(0)>>1)&3); a.record(a.r(0)); flush=!a.r(0); }
            if(flush) {
                a.put(3,a.r(26)); a.put(4,0); a.call(0x107758a0,0x107765b8); a.compare(a.signed_r(3));
                if(a.r(3)) { a.put(0,1); a.store<1>(a.r(26)+13,a.r(0),0x107765c4); a.put(0,0); a.put(29,0); a.store(a.r(26)+44,a.r(0),0x107765d0); break; }
            }
            a.compare(a.r(29)); if(!a.r(29)) break;
            a.compare(a.signed_r(31)); if(!a.r(31)) break;
        }
    }
after_buffer:
    a.compare(a.r(29));
    if(a.r(29)) {
        a.compare(a.signed_r(31));
        if(!a.r(31)) {
            a.load(27,a.r(26)+32,0x107765f8); a.put(0,a.r(30)+a.r(29)); a.load(31,a.r(26)+36,0x10776600); a.put(3,a.r(26)); a.put(4,a.r(1)+56);
            a.store(a.r(26)+32,a.r(30),0x1077660c); a.store(a.r(26)+36,a.r(29),0x10776610); a.store(a.r(26)+40,a.r(0),0x10776614); a.call(0x107758a0,0x1077661c); a.compare(a.signed_r(3));
            if(a.r(3)) { a.put(0,1); a.store<1>(a.r(26)+13,a.r(0),0x10776628); a.put(0,0); a.store(a.r(26)+44,a.r(0),0x10776630); }
            a.load(0,a.r(1)+56,0x10776634); a.put(3,a.r(26)); a.store(a.r(26)+32,a.r(27),0x1077663c); a.put(28,a.r(28)+a.r(0)); a.store(a.r(26)+36,a.r(31),0x10776644);
            a.call(0x10775790,0x1077664c); a.put(0,0); a.store(a.r(26)+44,a.r(0),0x10776650);
        }
    }
    a.load<1>(0,a.r(26)+4,0x10776654); a.put(0,(a.r(0)>>1)&3); a.compare(a.r(0),2);
    if(a.r(0)!=2) { a.put(0,0); a.store(a.r(26)+44,a.r(0),0x10776668); }
    // The Mac contract credits a partial final item on writes.
    a.put(0,a.r(25)-1); a.put(0,a.r(28)+a.r(0)); a.s.pc=0x10776674; a.put(3,std::uint32_t(divide_unsigned(a.r(0),a.r(25))));
done:
    a.pop(96,0x10776678); a.words(25,0x10776680,true); a.restore_link(); a.finish();
}
}
