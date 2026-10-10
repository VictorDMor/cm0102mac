#include "allocator_internal.hpp"

namespace cm::aot::allocator {
void initialize_pool(Abi& a) {
    // Header, neighbors, live class index, first slot and slot-byte extent
    // arrive in r3/r4/r5/r6/r7/r8. Extent division is Mac word32.
    a.load(9,a.r(2)-0x3b70,0x107749b0); a.put(0,a.r(6)<<2);
    a.store(a.r(3),a.r(4),0x107749b8); a.load(6,a.r(9)+a.r(0),0x107749bc); a.put(9,a.r(7));
    a.store(a.r(3)+4,a.r(5),0x107749c4); a.put(10,a.r(6)+4);
    a.s.pc=0x107749cc; a.put(8,divide_unsigned(a.r(8),a.r(10)));
    a.store(a.r(4)+4,a.r(3),0x107749d0); a.store(a.r(5),a.r(3),0x107749d4); a.store(a.r(3)+8,a.r(6),0x107749d8);
    a.put(0,a.r(8)-1); a.compare(a.r(0)); a.put(6,0);
    if(a.r(0)>0) {
        a.compare(a.r(0),8); a.put(4,a.r(8)-9);
        if(a.r(0)>8) {
            a.put(0,(a.r(4)+7)>>3); a.compare(a.r(4)); a.s.wr<4>(4132,a.r(0));
            if(a.r(4)>0) do {
                a.s.step(0x10774a0c);
                // Eight links retain the store groups and scratch commit points
                // even when a fixture aliases slots and the pool header.
                a.store(a.r(9),a.r(3),0x10774a0c); a.put(0,a.r(9)+a.r(10)); a.put(4,a.r(0)); a.store(a.r(9)+4,a.r(0),0x10774a18);
                a.put(0,a.r(0)+a.r(10)); a.put(5,a.r(0)); a.store(a.r(4),a.r(3),0x10774a24); a.put(6,a.r(6)+8); a.store(a.r(4)+4,a.r(0),0x10774a2c);
                a.put(0,a.r(0)+a.r(10)); a.put(4,a.r(0)); a.store(a.r(5),a.r(3),0x10774a38); a.store(a.r(5)+4,a.r(0),0x10774a3c);
                a.put(0,a.r(0)+a.r(10)); a.put(5,a.r(0)); a.store(a.r(4),a.r(3),0x10774a48); a.store(a.r(4)+4,a.r(0),0x10774a4c);
                a.put(0,a.r(0)+a.r(10)); a.put(4,a.r(0)); a.store(a.r(5),a.r(3),0x10774a58); a.store(a.r(5)+4,a.r(0),0x10774a5c);
                a.put(0,a.r(0)+a.r(10)); a.put(5,a.r(0)); a.store(a.r(4),a.r(3),0x10774a68); a.store(a.r(4)+4,a.r(0),0x10774a6c);
                a.put(0,a.r(0)+a.r(10)); a.put(4,a.r(0)); a.store(a.r(5),a.r(3),0x10774a78); a.store(a.r(5)+4,a.r(0),0x10774a7c);
                a.put(0,a.r(0)+a.r(10)); a.put(9,a.r(0)); a.store(a.r(4),a.r(3),0x10774a88); a.store(a.r(4)+4,a.r(0),0x10774a8c);
            } while(a.count_down());
        }
        a.put(4,a.r(8)-1); a.put(0,a.r(4)-a.r(6)); a.compare(a.r(6),a.r(4)); a.s.wr<4>(4132,a.r(0));
        if(a.r(6)<a.r(4)) do {
            a.s.step(0x10774aa8); a.store(a.r(9),a.r(3),0x10774aa8); a.put(0,a.r(9)+a.r(10));
            a.store(a.r(9)+4,a.r(0),0x10774ab0); a.put(9,a.r(0));
        } while(a.count_down());
    }
    a.store(a.r(9),a.r(3),0x10774abc); a.put(0,0); a.store(a.r(9)+4,0,0x10774ac4);
    a.store(a.r(3)+12,a.r(7),0x10774ac8); a.store(a.r(3)+16,0,0x10774acc); a.finish();
}
void clear_context(Abi& a) {
    a.capture_link(); a.put(4,0); a.save_link(0x10774ae8); a.put(5,52); a.frame(64,0x10774af0);
    a.call(0x10778300,0x10774af8); a.pop(64,0x10774af8); a.restore_link(); a.finish();
}
void global_context(Abi& a) {
    a.save(31,0x10774b10); a.capture_link(); a.load(31,a.r(2)-0x3b74,0x10774b18); a.save_link(0x10774b1c); a.frame(64,0x10774b20);
    a.load<1>(0,a.r(2)-0x3510,0x10774b24); a.compare(a.r(0));
    if(!a.r(0)) { a.put(3,a.r(31)); a.call(0x10774ae0,0x10774b38); a.put(0,1); a.store<1>(a.r(2)-0x3510,1,0x10774b3c); }
    a.put(3,a.r(31)); a.pop(64,0x10774b44); a.restore_link(); a.restore(31,0x10774b50); a.finish();
}
void allocate_small(Abi& a) {
    a.words(24,0x10774b60); a.capture_link(); a.load(31,a.r(2)-0x3b70,0x10774b68);
    a.put(27,a.r(3)); a.put(30,0); a.put(3,a.r(31)); a.save_link(0x10774b78); a.frame(96,0x10774b7c);
    for(;;) {
        a.s.step(0x10774b90); a.load(0,a.r(3),0x10774b90); a.compare(a.r(4),a.r(0));
        if(a.r(4)<=a.r(0)) break;
        a.put(3,a.r(3)+4); a.put(30,a.r(30)+1);
    }
    a.put(3,a.r(30)<<3); a.put(29,a.r(27)+a.r(3)+4); a.load(3,a.r(29)+4,0x10774ba8); a.compare(a.r(3));
    bool available=a.r(3)!=0;
    if(available) { a.load(0,a.r(3)+12,0x10774bb4); a.compare(a.r(0)); available=a.r(0)!=0; }
    bool failed=false;
    if(!available) {
        a.put(26,a.r(30)<<2); a.load(3,a.r(31)+a.r(26),0x10774bc4); a.put(4,0xfe4); a.put(0,a.r(3)+4);
        a.s.pc=0x10774bd0; a.put(25,divide_unsigned(a.r(4),a.r(0))); a.compare(a.r(25),256);
        if(a.r(25)>256) a.put(25,256);
        a.put(24,a.r(25));
        for(;;) {
            a.compare(a.r(25),10); if(a.r(25)<10) break;
            a.s.step(0x10774be8); a.load(4,a.r(31)+a.r(26),0x10774be8); a.put(3,a.r(27)); a.put(5,a.r(1)+56);
            a.put(0,a.r(4)+4); a.put(4,a.r(25)*a.r(0)+28); a.call(0x10774840,0x10774c04);
            a.put(28,a.r(3)); a.record(a.r(28)); if(a.r(28)) break;
            a.load(4,a.r(1)+56,0x10774c0c); a.compare(a.r(4),28);
            if(a.r(4)>28) {
                a.load(3,a.r(31)+a.r(26),0x10774c18); a.put(4,a.r(4)-28); a.put(0,a.r(3)+4);
                // Same correction at the same failed-allocation branch; the
                // private original path retains the audited generator guard.
                a.s.pc=0x10774c24; a.put(25,next_pool_attempt(a.r(25),divide_unsigned(a.r(4),a.r(0))));
            } else a.put(25,0);
        }
        a.compare(a.r(28));
        if(!a.r(28)) {
            a.compare(a.r(25),a.r(24));
            if(a.r(25)<a.r(24)) {
                a.load(4,a.r(31)+a.r(26),0x10774c48); a.put(3,a.r(27)); a.put(0,a.r(4)+4);
                a.put(4,a.r(24)*a.r(0)+28); a.call(0x10774760,0x10774c60); a.put(28,a.r(3)); a.record(a.r(28));
                if(!a.r(28)) { a.put(3,0); failed=true; }
            }
        }
        if(!failed) {
            a.load(3,a.r(28)-4,0x10774c70); a.put(0,a.r(3)&1); a.record(a.r(0));
            if(!a.r(0)) a.load(8,a.r(3)+8,0x10774c7c);
            else { a.load(0,a.r(28)-8,0x10774c84); a.put(3,a.r(0)&size_mask); a.put(8,a.r(3)-8); }
            a.load(0,a.r(29)+4,0x10774c90); a.compare(a.r(0));
            if(!a.r(0)) { a.store(a.r(29)+4,a.r(28),0x10774c9c); a.store(a.r(29),a.r(28),0x10774ca0); }
            a.load(4,a.r(29),0x10774ca4); a.put(3,a.r(28)); a.load(5,a.r(29)+4,0x10774cac);
            a.put(6,a.r(30)); a.put(7,a.r(28)+28); a.put(8,a.r(8)-28); a.call(0x107749b0,0x10774cc0);
            a.store(a.r(29)+4,a.r(28),0x10774cc0);
        }
    }
    if(!failed) {
        a.load(3,a.r(29)+4,0x10774cc4); a.load(5,a.r(3)+12,0x10774cc8); a.load(0,a.r(5)+4,0x10774ccc);
        a.store(a.r(3)+12,a.r(0),0x10774cd0); a.load(4,a.r(29)+4,0x10774cd4); a.load(3,a.r(4)+16,0x10774cd8);
        a.put(0,a.r(3)+1); a.store(a.r(4)+16,a.r(0),0x10774ce0); a.load(3,a.r(29)+4,0x10774ce4);
        a.load(0,a.r(3)+12,0x10774ce8); a.compare(a.r(0));
        if(!a.r(0)) {
            a.load(0,a.r(3)+4,0x10774cf4); a.store(a.r(29)+4,a.r(0),0x10774cf8);
            a.load(3,a.r(29),0x10774cfc); a.load(0,a.r(3)+4,0x10774d00); a.store(a.r(29),a.r(0),0x10774d04);
        }
        a.put(3,a.r(5)+4);
    }
    a.pop(96,0x10774d0c); a.words(24,0x10774d14,true); a.restore_link(); a.finish();
}
void release_small(Abi& a) {
    // r5 is the usable size read from the object's pool by the dispatcher.
    a.capture_link(); a.load(6,a.r(2)-0x3b70,0x10774d24); a.save_link(0x10774d28); a.put(7,0); a.frame(64,0x10774d30);
    for(;;) {
        a.s.step(0x10774d40); a.load(0,a.r(6),0x10774d40); a.compare(a.r(5),a.r(0));
        if(a.r(5)<=a.r(0)) break;
        a.put(6,a.r(6)+4); a.put(7,a.r(7)+1);
    }
    a.put(8,a.r(4)-4); a.load(4,a.r(4)-4,0x10774d50); a.put(5,(a.r(7)<<3)+4);
    a.load(0,a.r(4)+12,0x10774d5c); a.put(5,a.r(3)+a.r(5)); a.compare(a.r(0));
    if(!a.r(0)) {
        a.load(6,a.r(5)+4,0x10774d6c); a.compare(a.r(6),a.r(4));
        if(a.r(6)!=a.r(4)) {
            a.load(0,a.r(5),0x10774d78); a.compare(a.r(0),a.r(4));
            if(a.r(0)==a.r(4)) {
                a.load(0,a.r(6),0x10774d84); a.store(a.r(5)+4,a.r(0),0x10774d88);
                a.load(6,a.r(5),0x10774d8c); a.load(0,a.r(6),0x10774d90); a.store(a.r(5),a.r(0),0x10774d94);
            } else {
                a.load(0,a.r(4)+4,0x10774d9c); a.load(6,a.r(4),0x10774da0); a.store(a.r(6)+4,a.r(0),0x10774da4);
                a.load(0,a.r(4),0x10774da8); a.load(6,a.r(4)+4,0x10774dac); a.store(a.r(6),a.r(0),0x10774db0);
                a.load(0,a.r(5)+4,0x10774db4); a.store(a.r(4)+4,a.r(0),0x10774db8); a.load(6,a.r(4)+4,0x10774dbc);
                a.load(0,a.r(6),0x10774dc0); a.store(a.r(4),a.r(0),0x10774dc4); a.load(6,a.r(4),0x10774dc8);
                a.store(a.r(6)+4,a.r(4),0x10774dcc); a.load(6,a.r(4)+4,0x10774dd0); a.store(a.r(6),a.r(4),0x10774dd4);
                a.store(a.r(5)+4,a.r(4),0x10774dd8);
            }
        }
    }
    a.load(0,a.r(4)+12,0x10774ddc); a.store(a.r(8)+4,a.r(0),0x10774de0); a.store(a.r(4)+12,a.r(8),0x10774de4);
    a.load(6,a.r(4)+16,0x10774de8); a.s.wr<1>(1027,a.r(6)!=0); a.put(0,a.r(6)-1); a.record(a.r(0));
    a.store(a.r(4)+16,a.r(0),0x10774df0);
    if(!a.r(0)) {
        a.load(0,a.r(5)+4,0x10774df8); a.compare(a.r(0),a.r(4));
        if(a.r(0)==a.r(4)) { a.load(0,a.r(4)+4,0x10774e04); a.store(a.r(5)+4,a.r(0),0x10774e08); }
        a.load(0,a.r(5),0x10774e0c); a.compare(a.r(0),a.r(4));
        if(a.r(0)==a.r(4)) { a.load(0,a.r(4),0x10774e18); a.store(a.r(5),a.r(0),0x10774e1c); }
        a.load(0,a.r(4)+4,0x10774e20); a.load(6,a.r(4),0x10774e24); a.store(a.r(6)+4,a.r(0),0x10774e28);
        a.load(0,a.r(4),0x10774e2c); a.load(6,a.r(4)+4,0x10774e30); a.store(a.r(6),a.r(0),0x10774e34);
        a.load(0,a.r(5)+4,0x10774e38); a.compare(a.r(0),a.r(4));
        if(a.r(0)==a.r(4)) { a.put(0,0); a.store(a.r(5)+4,0,0x10774e48); }
        a.load(0,a.r(5),0x10774e4c); a.compare(a.r(0),a.r(4));
        if(a.r(0)==a.r(4)) { a.put(0,0); a.store(a.r(5),0,0x10774e5c); }
        a.call(0x10774920,0x10774e64);
    }
    a.pop(64,0x10774e64); a.restore_link(); a.finish();
}
}
