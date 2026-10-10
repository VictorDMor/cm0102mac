#include "stream_io_internal.hpp"

namespace cm::aot::stream_io {
void byte_orientation(Abi& a) {
    a.compare(a.r(3));
    if(a.r(3)) { a.load<2>(0,a.r(3)+4,0x1077fa58); a.put(0,(a.r(0)>>6)&7); a.record(a.r(0)); }
    if(!a.r(3)||!a.r(0)) { a.put(3,0); a.finish(); return; }
    a.load<1>(5,a.r(3)+5,0x1077fa6c); a.put(0,(a.r(5)>>4)&3); a.compare(a.signed_r(0),1);
    if(a.r(0)==1) a.put(3,UINT32_MAX);
    else if(a.signed_r(0)>=1) { a.compare(a.signed_r(0),3); if(a.signed_r(0)<3) a.put(3,1); }
    else {
        a.compare(a.signed_r(0));
        if(a.signed_r(0)>=0) {
            a.compare(a.signed_r(4));
            if(a.signed_r(4)>0) { a.put(0,2); a.put(5,(a.r(5)&~0x30u)|0x20); a.store<1>(a.r(3)+5,a.r(5),0x1077faa8); }
            else if(a.signed_r(4)<0) { a.put(0,1); a.put(5,(a.r(5)&~0x30u)|0x10); a.store<1>(a.r(3)+5,a.r(5),0x1077fabc); }
            a.put(3,a.r(4));
        }
    }
    a.finish();
}
void read_byte_slow(Abi& a) {
    a.save(31,0x10775ad0); a.capture_link(); a.put(31,a.r(3)); a.save_link(0x10775adc); a.put(0,0); a.frame(64,0x10775ae4);
    a.store(a.r(3)+44,a.r(0),0x10775ae8); a.load<1>(0,a.r(3)+13,0x10775aec); a.compare(a.r(0));
    bool invalid=a.r(0)!=0;
    if(!invalid) { a.load<2>(0,a.r(31)+4,0x10775af8); a.put(0,(a.r(0)>>6)&7); a.record(a.r(0)); invalid=!a.r(0); }
    if(invalid) a.put(3,UINT32_MAX);
    else {
        a.load<1>(4,a.r(31)+8,0x10775b0c); a.put(5,(a.r(4)>>5)&7); a.compare(a.signed_r(5),1);
        bool reject=a.r(5)==1;
        if(!reject) { a.load<1>(0,a.r(31)+4,0x10775b1c); a.put(0,(a.r(0)>>3)&1); a.record(a.r(0)); reject=!a.r(0); }
        if(reject) { a.put(0,1); a.store<1>(a.r(31)+13,a.r(0),0x10775b2c); a.put(0,0); a.put(3,UINT32_MAX); a.store(a.r(31)+44,a.r(0),0x10775b38); }
        else {
            a.compare(a.signed_r(5),3);
            if(a.signed_r(5)>=3) {
                a.put(0,a.r(5)-1); a.put(4,(a.r(4)&~0xe0u)|((a.r(0)<<5)&0xe0)); a.store<1>(a.r(31)+8,a.r(4),0x10775b50);
                if(a.r(5)==3) { a.load(0,a.r(31)+52,0x10775b58); a.store(a.r(31)+44,a.r(0),0x10775b5c); }
                a.put(3,a.r(5)+a.r(31)); a.load<1>(3,a.r(3)+16,0x10775b64);
            } else {
                a.put(0,2); a.put(4,(a.r(4)&~0xe0u)|0x40); a.store<1>(a.r(31)+8,a.r(4),0x10775b74); a.put(4,0); a.put(5,0); a.call(0x107757d0,0x10775b84);
                a.compare(a.signed_r(3)); bool filled=!a.r(3);
                if(filled) { a.load(4,a.r(31)+44,0x10775b8c); a.compare(a.r(4)); filled=a.r(4)!=0; }
                if(filled) {
                    a.put(0,a.r(4)-1); a.store(a.r(31)+44,a.r(0),0x10775bdc); a.load(3,a.r(31)+40,0x10775be0); a.put(0,a.r(3)+1); a.store(a.r(31)+40,a.r(0),0x10775be8); a.load<1>(3,a.r(3),0x10775bec);
                } else {
                    a.compare(a.signed_r(3),1);
                    if(a.r(3)==1) { a.put(0,1); a.store<1>(a.r(31)+13,a.r(0),0x10775ba4); a.put(0,0); a.store(a.r(31)+44,a.r(0),0x10775bac); }
                    else { a.load<1>(0,a.r(31)+8,0x10775bb4); a.put(3,0); a.put(0,a.r(0)&~0xe0u); a.store<1>(a.r(31)+8,a.r(0),0x10775bc0); a.put(0,1); a.store<1>(a.r(31)+12,a.r(0),0x10775bc8); a.store(a.r(31)+44,a.r(3),0x10775bcc); }
                    a.put(3,UINT32_MAX);
                }
            }
        }
    }
    a.pop(64,0x10775bf0); a.restore(31,0x10775bf8); a.restore_link(); a.finish();
}
void read_byte(Abi& a) {
    a.save(31,0x10775c10); a.capture_link(); a.put(31,a.r(3)); a.save_link(0x10775c1c); a.put(4,UINT32_MAX); a.frame(64,0x10775c24); a.call(0x1077fa50,0x10775c2c); a.compare(a.signed_r(3));
    if(a.signed_r(3)>=0) a.put(3,UINT32_MAX);
    else {
        a.load(3,a.r(31)+44,0x10775c3c); a.put(0,a.r(3)-1); a.compare(a.r(3)); a.store(a.r(31)+44,a.r(0),0x10775c48);
        if(a.r(3)) { a.load(3,a.r(31)+40,0x10775c50); a.put(0,a.r(3)+1); a.store(a.r(31)+40,a.r(0),0x10775c58); a.load<1>(3,a.r(3),0x10775c5c); }
        else { a.put(3,a.r(31)); a.call(0x10775ad0,0x10775c6c); }
    }
    a.pop(64,0x10775c6c); a.restore(31,0x10775c74); a.restore_link(); a.finish();
}
void read_line(Abi& a) {
    a.save(31,0x10775c80); a.capture_link(); a.save(30,0x10775c88); a.put(30,a.r(5)); a.save(29,0x10775c90); a.decrement_record(29,4);
    a.save(28,0x10775c98); a.put(28,a.r(3)); a.put(31,a.r(28)); a.save_link(0x10775ca4); a.frame(80,0x10775ca8);
    if(a.signed_r(29)<0) { a.put(3,0); goto done; }
    a.compare(a.signed_r(29));
    while(a.r(29)) {
        a.put(3,a.r(30)); a.put(4,UINT32_MAX); a.call(0x1077fa50,0x10775ccc); a.compare(a.signed_r(3));
        if(a.signed_r(3)>=0) a.put(3,UINT32_MAX);
        else {
            a.load(3,a.r(30)+44,0x10775cdc); a.put(0,a.r(3)-1); a.compare(a.r(3)); a.store(a.r(30)+44,a.r(0),0x10775ce8);
            if(a.r(3)) { a.load(3,a.r(30)+40,0x10775cf0); a.put(0,a.r(3)+1); a.store(a.r(30)+40,a.r(0),0x10775cf8); a.load<1>(3,a.r(3),0x10775cfc); }
            else { a.put(3,a.r(30)); a.call(0x10775ad0,0x10775d0c); }
        }
        a.compare(a.signed_r(3),-1);
        if(a.r(3)==UINT32_MAX) {
            a.load<1>(0,a.r(30)+12,0x10775d14); a.compare(a.r(0)); bool partial=a.r(0)!=0;
            if(partial) { a.compare(a.r(31),a.r(28)); partial=a.r(31)!=a.r(28); }
            if(!partial) { a.put(3,0); goto done; }
            break;
        }
        a.compare(a.signed_r(3),10); a.store<1>(a.r(31),a.r(3),0x10775d38); a.put(31,a.r(31)+1);
        if(a.r(3)==10) break;
        a.decrement_record(29,29);
    }
    a.put(0,0); a.store<1>(a.r(31),a.r(0),0x10775d50); a.put(3,a.r(28));
done:
    a.pop(80,0x10775d58); a.restore(31,0x10775d60); a.restore(30,0x10775d64); a.restore_link(); a.restore(29,0x10775d6c); a.restore(28,0x10775d70); a.finish();
}
void push_back_byte(Abi& a) {
    a.save(31,0x10775d80); a.capture_link(); a.save(30,0x10775d88); a.put(30,a.r(4)); a.save(29,0x10775d90); a.put(29,a.r(3)); a.put(3,a.r(30)); a.save_link(0x10775d9c); a.frame(80,0x10775da0);
    a.load<1>(0,a.r(4)+8,0x10775da4); a.put(4,UINT32_MAX); a.put(31,(a.r(0)>>5)&7); a.call(0x1077fa50,0x10775db4); a.compare(a.signed_r(3));
    if(a.signed_r(3)>=0) a.put(3,UINT32_MAX);
    else {
        a.compare(a.signed_r(31),1); bool reject=a.r(31)==1;
        if(!reject) { a.compare(a.signed_r(31),4); reject=a.r(31)==4; }
        if(!reject) { a.compare(a.signed_r(29),-1); reject=a.r(29)==UINT32_MAX; }
        if(reject) a.put(3,UINT32_MAX);
        else {
            a.compare(a.signed_r(31),3);
            if(a.signed_r(31)<3) {
                a.load(3,a.r(30)+44,0x10775dec); a.put(0,0); a.put(4,3); a.store(a.r(30)+52,a.r(3),0x10775df8); a.store(a.r(30)+44,a.r(0),0x10775dfc);
                a.load<1>(0,a.r(30)+8,0x10775e00); a.put(0,(a.r(0)&~0xe0u)|0x60); a.store<1>(a.r(30)+8,a.r(0),0x10775e08); a.put(4,(a.r(0)>>5)&7);
            } else {
                a.load<1>(0,a.r(30)+8,0x10775e14); a.put(3,(a.r(0)>>5)&7); a.put(4,a.r(3)+1); a.put(0,(a.r(0)&~0xe0u)|((a.r(4)<<5)&0xe0)); a.store<1>(a.r(30)+8,a.r(0),0x10775e24); a.put(4,(a.r(0)>>5)&7);
            }
            a.put(3,a.r(4)+a.r(30)); a.store<1>(a.r(3)+16,a.r(29),0x10775e30); a.put(0,0); a.put(3,a.r(29)&255); a.store<1>(a.r(30)+12,a.r(0),0x10775e3c);
        }
    }
    a.pop(80,0x10775e40); a.restore(31,0x10775e48); a.restore(30,0x10775e4c); a.restore_link(); a.restore(29,0x10775e54); a.finish();
}
void items_facade(Abi& a,bool writing) {
    const auto entry=writing?0x10776360u:0x10775fe0u;
    a.capture_link(); a.save_link(entry+4); a.frame(64,entry+8); a.call(writing?0x10776380:0x10776000,entry+16);
    a.pop(64,entry+16); a.restore_link(); a.finish();
}
}
