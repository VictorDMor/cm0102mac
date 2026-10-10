#include "stream_io_internal.hpp"

namespace cm::aot::stream_io {
void read_items(Abi& a) {
    a.words(25,0x10776000); a.capture_link(); a.put(26,a.r(4)); a.put(27,a.r(6)); a.put(28,a.r(3)); a.put(25,a.r(5)); a.put(3,a.r(27)); a.put(4,0); a.save_link(0x10776020); a.frame(96,0x10776024);
    a.call(0x1077fa50,0x1077602c); a.compare(a.signed_r(3));
    if(!a.r(3)) { a.put(3,a.r(27)); a.put(4,UINT32_MAX); a.call(0x1077fa50,0x10776040); }
    a.put(29,a.r(26)*a.r(25)); a.record(a.r(29)); bool invalid=!a.r(29);
    if(!invalid) { a.load<1>(0,a.r(27)+13,0x10776048); a.compare(a.r(0)); invalid=a.r(0)!=0; }
    if(!invalid) { a.load<2>(0,a.r(27)+4,0x10776054); a.put(0,(a.r(0)>>6)&7); a.record(a.r(0)); invalid=!a.r(0); }
    if(invalid) { a.put(3,0); goto done; }
    a.load<1>(0,a.r(27)+5,0x10776068); a.put(31,1); a.put(0,(a.r(0)>>3)&1); a.record(a.r(0));
    if(a.r(0)) { a.load<1>(0,a.r(27)+4,0x10776078); a.put(0,(a.r(0)>>1)&3); a.compare(a.r(0),2); if(a.r(0)!=2) a.put(31,0); }
    a.load<1>(3,a.r(27)+8,0x1077608c); a.put(0,(a.r(3)>>5)&7); a.record(a.r(0));
    if(!a.r(0)) {
        a.load<1>(0,a.r(27)+4,0x10776098); a.put(0,(a.r(0)>>3)&1); a.record(a.r(0));
        if(a.r(0)) { a.put(0,2); a.put(3,(a.r(3)&~0xe0u)|0x40); a.store<1>(a.r(27)+8,a.r(3),0x107760ac); a.put(0,0); a.store(a.r(27)+44,a.r(0),0x107760b4); }
    }
    a.load<1>(0,a.r(27)+8,0x107760b8); a.put(0,(a.r(0)>>5)&7); a.compare(a.r(0),2);
    if(a.r(0)<2) { a.put(0,1); a.store<1>(a.r(27)+13,a.r(0),0x107760cc); a.put(0,0); a.put(3,0); a.store(a.r(27)+44,a.r(0),0x107760d8); goto done; }
    a.load<1>(0,a.r(27)+4,0x107760e0); a.put(0,(a.r(0)>>1)&1); a.record(a.r(0));
    if(a.r(0)) {
        a.call(0x107754b0,0x107760f0); a.compare(a.signed_r(3));
        if(a.r(3)) { a.put(0,1); a.store<1>(a.r(27)+13,a.r(0),0x107760fc); a.put(0,0); a.put(3,0); a.store(a.r(27)+44,a.r(0),0x10776108); goto done; }
    }
    a.compare(a.r(29)); a.put(30,a.r(28)); a.put(28,0);
    if(a.r(29)) {
        a.load<1>(0,a.r(27)+8,0x10776120); a.put(0,(a.r(0)>>5)&7); a.compare(a.r(0),3);
        if(a.r(0)>=3) {
            do {
                a.put(3,a.r(27)); a.put(4,0); a.call(0x1077fa50,0x10776140); a.compare(a.signed_r(3),1);
                if(a.r(3)==1) {
                    a.load<1>(0,a.r(27)+8,0x10776148); a.put(28,a.r(28)+2); a.put(29,a.r(29)-2); a.put(3,(a.r(0)>>5)&7); a.put(0,a.r(3)-3);
                    a.put(3,((a.r(0)<<1)|(a.r(0)>>31))&0xfffffffeu); a.put(0,a.r(3)+22); a.load<2>(0,a.r(27)+a.r(0),0x10776164); a.store<2>(a.r(30),a.r(0),0x10776168); a.put(30,a.r(30)+2);
                } else {
                    a.load<1>(0,a.r(27)+8,0x10776174); a.put(28,a.r(28)+1); a.put(29,a.r(29)-1); a.put(3,(a.r(0)>>5)&7); a.put(0,a.r(3)+16); a.load<1>(0,a.r(27)+a.r(0),0x10776188); a.store<1>(a.r(30),a.r(0),0x1077618c); a.put(30,a.r(30)+1);
                }
                a.load<1>(4,a.r(27)+8,0x10776194); a.compare(a.r(29)); const bool exhausted=!a.r(29);
                a.put(3,(a.r(4)>>5)&7); a.put(0,a.r(3)-1); a.put(4,(a.r(4)&~0xe0u)|((a.r(0)<<5)&0xe0)); a.store<1>(a.r(27)+8,a.r(4),0x107761a8);
                if(exhausted) break;
                a.load<1>(0,a.r(27)+8,0x107761b0); a.put(0,(a.r(0)>>5)&7); a.compare(a.r(0),3);
            } while(a.r(0)>=3);
            a.load<1>(0,a.r(27)+8,0x107761c0); a.put(0,(a.r(0)>>5)&7); a.compare(a.r(0),2);
            if(a.r(0)==2) { a.load(0,a.r(27)+52,0x107761d0); a.store(a.r(27)+44,a.r(0),0x107761d4); }
        }
    }
    a.compare(a.r(29));
    if(a.r(29)) {
        a.load(0,a.r(27)+44,0x107761e0); a.compare(a.r(0));
        if(!a.r(0)) { a.compare(a.signed_r(31)); if(!a.r(31)) goto after_buffer; }
        for(;;) {
            a.load(0,a.r(27)+44,0x107761f8); a.compare(a.r(0));
            if(!a.r(0)) {
                a.put(3,a.r(27)); a.put(4,0); a.put(5,0); a.call(0x107757d0,0x10776214); a.compare(a.signed_r(3));
                if(a.r(3)) {
                    a.compare(a.signed_r(3),1);
                    if(a.r(3)==1) { a.put(0,1); a.store<1>(a.r(27)+13,a.r(0),0x10776228); a.put(0,0); a.store(a.r(27)+44,a.r(0),0x10776230); }
                    else { a.load<1>(0,a.r(27)+8,0x10776238); a.put(3,0); a.put(0,a.r(0)&~0xe0u); a.store<1>(a.r(27)+8,a.r(0),0x10776244); a.put(0,1); a.store<1>(a.r(27)+12,a.r(0),0x1077624c); a.store(a.r(27)+44,a.r(3),0x10776250); }
                    a.put(29,0); break;
                }
            }
            a.load(0,a.r(27)+44,0x1077625c); a.compare(a.r(0),a.r(29)); a.store(a.r(1)+56,a.r(0),0x10776264);
            if(a.r(0)>a.r(29)) a.store(a.r(1)+56,a.r(29),0x1077626c);
            a.load(4,a.r(27)+40,0x10776270); a.put(3,a.r(30)); a.load(5,a.r(1)+56,0x10776278); a.call(0x10739ef0,0x10776280);
            a.load(4,a.r(1)+56,0x10776280); a.load(0,a.r(27)+40,0x10776284); a.put(29,a.r(29)-a.r(4)); a.record(a.r(29)); const bool exhausted=!a.r(29);
            a.put(0,a.r(0)+a.r(4)); a.store(a.r(27)+40,a.r(0),0x10776290); a.put(30,a.r(30)+a.r(4)); a.put(28,a.r(28)+a.r(4));
            a.load(3,a.r(1)+56,0x1077629c); a.load(0,a.r(27)+44,0x107762a0); a.put(0,a.r(0)-a.r(3)); a.store(a.r(27)+44,a.r(0),0x107762a8);
            if(exhausted) break;
            a.compare(a.signed_r(31)); if(!a.r(31)) break;
        }
    }
after_buffer:
    a.compare(a.r(29));
    if(a.r(29)) {
        a.compare(a.signed_r(31));
        if(!a.r(31)) {
            // Temporarily expose the caller's destination as a direct refill
            // buffer; restore the saved descriptor only after callback effects.
            a.load(31,a.r(27)+32,0x107762c8); a.put(3,a.r(27)); a.load(25,a.r(27)+36,0x107762d0); a.put(4,a.r(1)+56); a.put(5,1);
            a.store(a.r(27)+32,a.r(30),0x107762dc); a.store(a.r(27)+36,a.r(29),0x107762e0); a.call(0x107757d0,0x107762e8); a.compare(a.signed_r(3));
            if(a.r(3)) {
                a.compare(a.signed_r(3),1);
                if(a.r(3)==1) { a.put(0,1); a.store<1>(a.r(27)+13,a.r(0),0x107762fc); a.put(0,0); a.store(a.r(27)+44,a.r(0),0x10776304); }
                else { a.load<1>(0,a.r(27)+8,0x1077630c); a.put(3,0); a.put(0,a.r(0)&~0xe0u); a.store<1>(a.r(27)+8,a.r(0),0x10776318); a.put(0,1); a.store<1>(a.r(27)+12,a.r(0),0x10776320); a.store(a.r(27)+44,a.r(3),0x10776324); }
            }
            a.load(0,a.r(1)+56,0x10776328); a.put(3,a.r(27)); a.store(a.r(27)+32,a.r(31),0x10776330); a.put(28,a.r(28)+a.r(0)); a.store(a.r(27)+36,a.r(25),0x10776338);
            a.call(0x10775790,0x10776340); a.put(0,0); a.store(a.r(27)+44,a.r(0),0x10776344);
        }
    }
    a.s.pc=0x10776348; a.put(3,std::uint32_t(divide_unsigned(a.r(28),a.r(26))));
done:
    a.pop(96,0x1077634c); a.words(25,0x10776354,true); a.restore_link(); a.finish();
}
}
