#include "allocator_internal.hpp"

namespace cm::aot::allocator {
void allocate(Abi& a) {
    a.capture_link(); a.compare(a.r(4)); a.save_link(0x10774e88); a.frame(64,0x10774e8c);
    if(!a.r(4)) a.put(3,0);
    else {
        a.put(0,0xffffffb7); a.compare(a.r(4),a.r(0));
        if(a.r(4)>a.r(0)) a.put(3,0);
        else { a.compare(a.r(4),76); a.call(a.r(4)<=76?0x10774b60:0x10774760,a.r(4)<=76?0x10774ebc:0x10774ec4); }
    }
    a.pop(64,0x10774ec4); a.restore_link(); a.finish();
}
void release(Abi& a) {
    a.capture_link(); a.compare(a.r(4)); a.save_link(0x10774ee8); a.frame(64,0x10774eec);
    if(a.r(4)) {
        a.load(5,a.r(4)-4,0x10774ef4); a.put(0,a.r(5)&1); a.record(a.r(0));
        if(!a.r(0)) a.load(5,a.r(5)+8,0x10774f00);
        else { a.load(0,a.r(4)-8,0x10774f08); a.put(5,(a.r(0)&size_mask)-8); }
        a.compare(a.r(5),76); a.call(a.r(5)<=76?0x10774d20:0x10774920,a.r(5)<=76?0x10774f20:0x10774f28);
    }
    a.pop(64,0x10774f28); a.restore_link(); a.finish();
}
namespace {
void move_reallocation(Abi& a) {
    a.put(3,a.r(29)); a.put(4,a.r(31)); a.call(0x10774e80,0x10775048); a.put(26,a.r(3)); a.record(a.r(26));
    if(!a.r(26)) a.put(3,0);
    else {
        a.put(4,a.r(30)); a.put(5,a.r(28)); a.call(0x10739ef0,0x10775064);
        a.put(3,a.r(29)); a.put(4,a.r(30)); a.call(0x10774ee0,0x10775070); a.put(3,a.r(26));
    }
}
}
void reallocate(Abi& a) {
    a.words(26,0x10774f40); a.capture_link(); a.put(30,a.r(4)); a.record(a.r(30)); a.put(29,a.r(3)); a.put(31,a.r(5));
    a.save_link(0x10774f54); a.frame(80,0x10774f58);
    if(!a.r(30)) { a.put(4,a.r(31)); a.call(0x10774e80,0x10774f68); }
    else {
        a.compare(a.r(31));
        if(!a.r(31)) { a.call(0x10774ee0,0x10774f78); a.put(3,0); }
        else {
            a.load(3,a.r(30)-4,0x10774f80); a.put(4,a.r(3)&1); a.record(a.r(4));
            if(!a.r(4)) a.load(28,a.r(3)+8,0x10774f8c);
            else { a.load(0,a.r(30)-8,0x10774f94); a.put(3,a.r(0)&size_mask); a.put(28,a.r(3)-8); }
            a.compare(a.r(31),a.r(28));
            if(a.r(31)>a.r(28)) {
                a.compare(a.r(4));
                if(!a.r(4)) move_reallocation(a);
                else {
                    a.put(0,0xffffffb7); a.compare(a.r(31),a.r(0));
                    if(a.r(31)>a.r(0)) a.put(3,0);
                    else {
                        a.put(0,a.r(31)+23); a.put(27,a.r(0)&size_mask); a.compare(a.r(27),96); if(a.r(27)<96) a.put(27,96);
                        a.put(26,a.r(30)-8); a.load(0,a.r(30)-4,0x10774fdc); a.put(3,a.r(26)); a.put(5,a.r(0)&~1u);
                        a.load(0,a.r(5)+20,0x10774fe8); a.put(4,a.r(5)+(a.r(0)&size_mask)-4); a.call(0x10774570,0x10774ffc);
                        a.load(0,a.r(26),0x10774ffc); a.put(0,a.r(0)&size_mask); a.compare(a.r(0),a.r(27));
                        if(a.r(0)<a.r(27)) move_reallocation(a);
                        else {
                            a.put(0,a.r(0)-a.r(27)); a.compare(a.r(0),96);
                            if(a.r(0)>=96) {
                                a.put(3,a.r(26)); a.put(4,a.r(27)); a.call(0x10774430,0x10775024);
                                a.load(0,a.r(26)+4,0x10775024); a.put(4,a.r(3)); a.put(3,a.r(0)&~1u); a.call(0x10774270,0x10775034);
                            }
                            a.put(3,a.r(30));
                        }
                    }
                }
            } else {
                a.compare(a.r(4));
                if(a.r(4)) {
                    a.put(0,a.r(31)+23); a.put(31,a.r(0)&size_mask); a.compare(a.r(31),96); if(a.r(31)<96) a.put(31,96);
                    a.put(27,a.r(30)-8); a.load(0,a.r(30)-8,0x10775098); a.put(0,(a.r(0)&size_mask)-a.r(31)); a.compare(a.r(0),96);
                    if(a.r(0)>=96) {
                        a.put(3,a.r(27)); a.put(4,a.r(31)); a.call(0x10774430,0x107750b8);
                        a.load(0,a.r(27)+4,0x107750b8); a.put(4,a.r(3)); a.put(3,a.r(0)&~1u); a.call(0x10774270,0x107750c8);
                    }
                }
                a.put(3,a.r(30));
            }
        }
    }
    a.pop(80,0x107750cc); a.words(26,0x107750d4,true); a.restore_link(); a.finish();
}
void allocate_clear(Abi& a) {
    a.save(31,0x107750e0); a.capture_link(); a.save(30,0x107750e8); a.put(30,a.r(4)); a.save_link(0x107750f0); a.frame(64,0x107750f4);
    a.call(0x10774e80,0x107750fc); a.put(31,a.r(3)); a.record(a.r(31));
    if(a.r(31)) { a.put(5,a.r(30)); a.put(4,0); a.call(0x10778300,0x10775110); }
    a.put(3,a.r(31)); a.pop(64,0x10775114); a.restore_link(); a.restore(31,0x10775120); a.restore(30,0x10775124); a.finish();
}
void global_allocate(Abi& a,bool releasing) {
    const auto entry=releasing?0x10775170u:0x10775130u;
    a.save(31,entry); a.capture_link(); a.put(31,a.r(3)); a.save_link(entry+12); a.frame(64,entry+16);
    a.call(0x10774b10,entry+24); a.put(4,a.r(31)); a.call(releasing?0x10774ee0:0x10774e80,entry+32);
    a.pop(64,entry+32); a.restore(31,entry+40); a.restore_link(); a.finish();
}
void global_reallocate(Abi& a,bool clearing) {
    const auto entry=clearing?0x10775200u:0x107751b0u;
    a.save(31,entry); a.capture_link(); a.put(31,a.r(4)); a.save(30,entry+12); a.put(30,a.r(3)); a.save_link(entry+20); a.frame(64,entry+24);
    a.call(0x10774b10,entry+32);
    if(clearing) { a.put(4,a.r(31)*a.r(30)); a.call(0x107750e0,entry+40); }
    else { a.put(4,a.r(30)); a.put(5,a.r(31)); a.call(0x10774f40,entry+44); }
    const auto tail=clearing?entry+40:entry+44;
    a.pop(64,tail); a.restore(31,tail+8); a.restore_link(); a.restore(30,tail+16); a.finish();
}
}
