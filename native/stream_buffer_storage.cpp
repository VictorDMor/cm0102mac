#include "stream_buffer_internal.hpp"

namespace cm::aot::stream_buffer {
void acquire(Abi& a) {
    a.save(31,0x10775240); a.capture_link(); a.load(3,a.r(2)-0x5a68,0x10775248);
    a.save(30,0x1077524c); a.save_link(0x10775250); a.frame(64,0x10775254);
    a.load(3,a.r(3)+248,0x10775258);
    for(;;) {
        a.compare(a.r(3)); if(!a.r(3)) break;
        a.load<2>(0,a.r(3)+4,0x10775260); a.put(0,(a.r(0)>>6)&7); a.record(a.r(0));
        if(!a.r(0)) goto done;
        a.put(30,a.r(3)); a.load(3,a.r(3)+80,0x10775274);
    }
    a.put(3,84); a.call(0x10775130,0x10775288); a.put(31,a.r(3)); a.record(a.r(31));
    if(a.r(31)) {
        a.put(4,0); a.put(5,84); a.call(0x10778300,0x1077529c); a.put(0,1);
        a.store<1>(a.r(31)+16,a.r(0),0x107752a0); a.put(3,a.r(31)); a.store(a.r(30)+80,a.r(31),0x107752a8);
    } else a.put(3,0);
done:
    a.pop(64,0x107752b4); a.restore(31,0x107752bc); a.restore_link(); a.restore(30,0x107752c4); a.finish();
}
void initialize(Abi& a) {
    a.save(31,0x107752d0); a.capture_link(); a.compare(a.r(6)); const bool external=a.r(6)!=0;
    a.save_link(0x107752dc); a.put(31,a.r(3)); a.frame(64,0x107752e4);
    a.store(a.r(1)+92,a.r(4),0x107752e8); a.put(4,0); a.store(a.r(3),a.r(4),0x107752f0);
    a.load<2>(0,a.r(1)+92,0x107752f4); a.store<2>(a.r(3)+4,a.r(0),0x107752f8);
    a.load<2>(0,a.r(1)+94,0x107752fc); a.store<2>(a.r(3)+6,a.r(0),0x10775300);
    a.load<1>(0,a.r(3)+8,0x10775304); a.put(0,a.r(0)&~0xe0u); a.store<1>(a.r(3)+8,a.r(0),0x1077530c);
    a.load<1>(0,a.r(3)+8,0x10775310); a.put(0,a.r(0)&~0x10u); a.store<1>(a.r(3)+8,a.r(0),0x10775318);
    a.store<1>(a.r(3)+12,a.r(4),0x1077531c); a.store<1>(a.r(3)+13,a.r(4),0x10775320); a.store(a.r(3)+28,a.r(4),0x10775324);
    if(external) { a.put(4,a.r(5)); a.put(5,2); a.call(0x10775960,0x10775338); }
    else { a.put(4,0); a.put(5,0); a.put(6,0); a.call(0x10775960,0x1077534c); }
    a.load(3,a.r(31)+32,0x1077534c); a.put(0,0); a.store(a.r(31)+40,a.r(3),0x10775354); a.store(a.r(31)+44,a.r(0),0x10775358);
    a.load<2>(0,a.r(31)+4,0x1077535c); a.put(0,(a.r(0)>>6)&7); a.compare(std::bit_cast<std::int32_t>(a.r(0)),1);
    if(a.r(0)==1) {
        a.load(3,a.r(2)-0x5a6c,0x1077536c); a.load(0,a.r(2)-0x5a70,0x10775370); a.store(a.r(31)+60,a.r(3),0x10775374);
        a.load(3,a.r(2)-0x5a74,0x10775378); a.store(a.r(31)+64,a.r(0),0x1077537c); a.load(0,a.r(2)-0x5a78,0x10775380);
        a.store(a.r(31)+68,a.r(3),0x10775384); a.store(a.r(31)+72,a.r(0),0x10775388);
    }
    a.put(0,0); a.store(a.r(31)+76,a.r(0),0x10775390); a.pop(64,0x10775394); a.restore_link(); a.restore(31,0x107753a0); a.finish();
}
void prepare(Abi& a) {
    a.load(0,a.r(3)+32,0x10775790); a.store(a.r(3)+40,a.r(0),0x10775794);
    a.load(0,a.r(3)+36,0x10775798); a.store(a.r(3)+44,a.r(0),0x1077579c);
    a.load(5,a.r(3)+28,0x107757a0); a.load(4,a.r(3)+48,0x107757a4); a.load(0,a.r(3)+44,0x107757a8);
    a.put(4,a.r(5)&a.r(4)); a.put(0,a.r(0)-a.r(4)); a.store(a.r(3)+44,a.r(0),0x107757b4);
    a.load(0,a.r(3)+28,0x107757b8); a.store(a.r(3)+56,a.r(0),0x107757bc); a.finish();
}
void configure(Abi& a) {
    a.words(27,0x10775960); a.capture_link(); a.put(28,a.r(5)); a.record(a.r(28)); const bool unbuffered=!a.r(28);
    a.put(30,a.r(3)); a.put(27,a.r(4)); a.put(31,a.r(6)); a.save_link(0x10775978); a.frame(80,0x1077597c);
    a.load<2>(0,a.r(3)+4,0x10775980); a.put(29,(a.r(0)>>6)&7);
    if(unbuffered) a.call(0x107767b0,0x10775990);
    a.load<1>(0,a.r(30)+8,0x10775990); a.put(0,(a.r(0)>>5)&7); a.record(a.r(0));
    if(!a.r(0)) a.compare(std::bit_cast<std::int32_t>(a.r(29)));
    if(a.r(0)||!a.r(29)) a.put(3,UINT32_MAX);
    else {
        a.compare(std::bit_cast<std::int32_t>(a.r(28)));
        if(a.r(28)!=0) { a.compare(std::bit_cast<std::int32_t>(a.r(28)),1); if(a.r(28)!=1) a.compare(std::bit_cast<std::int32_t>(a.r(28)),2); }
        if(a.r(28)>2) a.put(3,UINT32_MAX);
        else {
            a.load(3,a.r(30)+32,0x107759cc); a.compare(a.r(3));
            if(a.r(3)) { a.load<1>(0,a.r(30)+8,0x107759d8); a.put(0,(a.r(0)>>4)&1); a.record(a.r(0)); if(a.r(0)) a.call(0x10775170,0x107759e8); }
            a.load<1>(0,a.r(30)+4,0x107759e8); a.put(0,(a.r(0)&~6u)|((a.r(28)<<1)&6)); a.put(5,0); a.store<1>(a.r(30)+4,a.r(0),0x107759f4);
            a.put(3,a.r(30)+17); a.put(0,1); a.load<1>(4,a.r(30)+8,0x10775a00); a.put(4,a.r(4)&~0x10u); a.compare(std::bit_cast<std::int32_t>(a.r(28)));
            a.store<1>(a.r(30)+8,a.r(4),0x10775a0c); a.store(a.r(30)+32,a.r(3),0x10775a10); a.store(a.r(30)+40,a.r(3),0x10775a14);
            a.store(a.r(30)+36,a.r(0),0x10775a18); a.store(a.r(30)+44,a.r(5),0x10775a1c); a.store(a.r(30)+48,a.r(5),0x10775a20);
            if(a.r(28)) a.compare(a.r(31),1);
            if(!a.r(28)||a.r(31)<1) {
                a.load(4,a.r(30)+40,0x10775a30); a.put(0,0); a.put(3,0); a.store<1>(a.r(4),a.r(0),0x10775a3c);
            } else {
                a.compare(a.r(27));
                if(!a.r(27)) {
                    a.put(3,a.r(31)); a.call(0x10775130,0x10775a54); a.put(27,a.r(3)); a.record(a.r(27));
                    if(!a.r(27)) { a.put(3,UINT32_MAX); goto done; }
                    a.load<1>(0,a.r(30)+8,0x10775a64); a.put(3,1); a.put(0,(a.r(0)&~0x10u)|0x10); a.store<1>(a.r(30)+8,a.r(0),0x10775a70);
                }
                a.store(a.r(30)+32,a.r(27),0x10775a74); a.put(0,0); a.load(3,a.r(30)+32,0x10775a7c); a.store(a.r(30)+40,a.r(3),0x10775a80);
                a.store(a.r(30)+36,a.r(31),0x10775a84); a.store(a.r(30)+48,a.r(0),0x10775a88); a.load<2>(0,a.r(30)+4,0x10775a8c);
                a.put(0,(a.r(0)>>6)&7); a.compare(std::bit_cast<std::int32_t>(a.r(0)),1);
                if(a.r(0)==1) { a.put(0,a.r(31)&511); a.record(a.r(0)); if(!a.r(0)) { a.put(0,511); a.store(a.r(30)+48,a.r(0),0x10775aa8); } }
                a.put(3,0);
            }
        }
    }
done:
    a.pop(80,0x10775ab0); a.words(27,0x10775ab8,true); a.restore_link(); a.finish();
}
}
