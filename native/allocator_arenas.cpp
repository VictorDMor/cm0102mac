#include "allocator_internal.hpp"

namespace cm::aot::allocator {
void initialize_block(Abi& a) {
    a.put(0,a.r(5)|1); a.store(a.r(3)+4,a.r(0),0x107743e4);
    a.compare(std::bit_cast<std::int32_t>(a.r(6))); a.store(a.r(3),a.r(4),0x107743ec);
    if(a.r(6)) { a.load(0,a.r(3),0x107743f4); a.put(0,a.r(0)|previous_busy); a.store(a.r(3),a.r(0),0x107743fc); }
    a.compare(std::bit_cast<std::int32_t>(a.r(7)));
    if(a.r(7)) {
        a.load(0,a.r(3),0x10774408); a.put(0,a.r(0)|in_use); a.store(a.r(3),a.r(0),0x10774410);
        a.load(0,a.r(3)+a.r(4),0x10774414); a.put(0,a.r(0)|previous_busy); a.store(a.r(3)+a.r(4),a.r(0),0x1077441c);
    } else { a.put(0,a.r(4)-4); a.store(a.r(3)+a.r(0),a.r(4),0x10774428); }
    a.finish();
}
void initialize_arena(Abi& a) {
    a.save(31,0x107740f0); a.capture_link(); a.put(6,0); a.save(30,0x107740fc);
    a.put(30,a.r(4)); a.put(7,0); a.save(29,0x10774108); a.put(29,a.r(3)); a.put(31,a.r(29)+24);
    a.save_link(0x10774114); a.put(0,a.r(30)|3); a.put(5,a.r(29)); a.frame(80,0x10774120);
    a.store(a.r(3)+20,a.r(0),0x10774124); a.put(0,a.r(30)-8); a.load(4,a.r(3)+20,0x1077412c);
    a.put(3,a.r(31)); a.store(a.r(29)+a.r(0),a.r(4),0x10774134); a.put(4,a.r(30)-32);
    a.call(0x107743e0,0x10774140);
    a.put(0,a.r(30)-32); a.store(a.r(29)+16,a.r(0),0x10774144); a.put(6,0); a.put(3,a.r(29));
    a.load(0,a.r(29)+20,0x10774150); a.put(4,a.r(31)); a.put(5,a.r(0)&size_mask); a.put(0,a.r(5)-4);
    a.store(a.r(29)+a.r(0),a.r(6),0x10774160); a.call(0x10774270,0x10774168);
    a.pop(80,0x10774168); a.restore(31,0x10774170); a.restore(30,0x10774174); a.restore_link(); a.restore(29,0x1077417c); a.finish();
}
void mark_and_unlink(Abi& a) {
    a.load(5,a.r(4),0x10774360); a.put(0,a.r(5)|in_use); a.store(a.r(4),a.r(0),0x10774368);
    a.put(5,a.r(5)&size_mask); a.load(0,a.r(4)+a.r(5),0x10774370); a.put(0,a.r(0)|previous_busy);
    a.store(a.r(4)+a.r(5),a.r(0),0x10774378);
    a.load(0,a.r(3)+20,0x1077437c); a.put(5,(a.r(0)&size_mask)-4); a.load(0,a.r(3)+a.r(5),0x10774388);
    a.compare(a.r(0),a.r(4));
    if(a.r(0)==a.r(4)) { a.load(0,a.r(4)+12,0x10774394); a.store(a.r(3)+a.r(5),a.r(0),0x10774398); }
    a.load(0,a.r(3)+a.r(5),0x1077439c); a.compare(a.r(0),a.r(4));
    if(a.r(0)==a.r(4)) {
        a.put(0,0); a.store(a.r(3)+a.r(5),0,0x107743ac); a.store(a.r(3)+16,0,0x107743b0);
    } else {
        a.load(0,a.r(4)+8,0x107743b8); a.load(3,a.r(4)+12,0x107743bc); a.store(a.r(3)+8,a.r(0),0x107743c0);
        a.load(0,a.r(4)+12,0x107743c4); a.load(3,a.r(4)+8,0x107743c8); a.store(a.r(3)+12,a.r(0),0x107743cc);
    }
    a.finish();
}
void split_block(Abi& a) {
    a.words(25,0x10774430); a.capture_link(); a.put(25,a.r(3)); a.put(26,a.r(4)); a.put(28,a.r(25)+a.r(26));
    a.save_link(0x10774444); a.frame(96,0x10774448);
    a.load(6,a.r(3),0x1077444c); a.load(5,a.r(3)+4,0x10774450);
    a.put(0,a.r(6)&in_use); a.put(0,std::countl_zero(a.r(0))); a.put(31,a.r(0)>>5);
    a.put(0,std::countl_zero(a.r(31))); a.put(30,a.r(0)>>5);
    a.put(27,a.r(5)&~1u); a.put(29,a.r(6)&size_mask);
    a.put(5,a.r(27)); a.put(7,a.r(30)); a.put(6,a.r(6)&previous_busy); a.call(0x107743e0,0x10774480);
    a.put(3,a.r(28)); a.put(5,a.r(27)); a.put(6,a.r(30)); a.put(7,a.r(30)); a.put(4,a.r(29)-a.r(26));
    a.call(0x107743e0,0x10774498); a.compare(std::bit_cast<std::int32_t>(a.r(31)));
    if(a.r(31)) {
        a.load(0,a.r(25)+12,0x107744a0); a.store(a.r(28)+12,a.r(0),0x107744a4);
        a.load(3,a.r(28)+12,0x107744a8); a.store(a.r(3)+8,a.r(28),0x107744ac);
        a.store(a.r(28)+8,a.r(25),0x107744b0); a.store(a.r(25)+12,a.r(28),0x107744b4);
    }
    a.put(3,a.r(28)); a.pop(96,0x107744bc); a.restore_link(); a.words(25,0x107744c8,true); a.finish();
}
void allocate_block(Abi& a) {
    a.save(31,0x10774190); a.capture_link(); a.save(30,0x10774198); a.put(30,a.r(3));
    a.save_link(0x107741a0); a.frame(64,0x107741a4);
    a.load(0,a.r(3)+20,0x107741a8); a.put(3,a.r(0)&size_mask); a.put(0,a.r(3)-4);
    a.load(3,a.r(30)+a.r(0),0x107741b4); a.compare(a.r(3));
    bool found=a.r(3)!=0;
    if(found) {
        a.load(0,a.r(3),0x107741c8); a.put(31,a.r(3)); a.put(0,a.r(0)&size_mask); a.put(5,a.r(0));
        for(;;) {
            a.s.step(0x10774210); a.compare(a.r(0),a.r(4));
            if(a.r(0)>=a.r(4)) break;
            a.load(31,a.r(31)+12,0x107741e0); a.load(0,a.r(31),0x107741e4); a.put(0,a.r(0)&size_mask);
            a.compare(a.r(5),a.r(0)); if(a.r(5)<a.r(0)) a.put(5,a.r(0));
            a.compare(a.r(31),a.r(3));
            if(a.r(31)==a.r(3)) { a.store(a.r(30)+16,a.r(5),0x10774200); found=false; break; }
        }
    }
    if(found) {
        a.put(0,a.r(0)-a.r(4)); a.compare(a.r(0),96);
        if(a.r(0)>=96) { a.put(3,a.r(31)); a.call(0x10774430,0x1077422c); }
        a.load(0,a.r(30)+20,0x1077422c); a.put(3,a.r(30)); a.load(6,a.r(31)+12,0x10774234);
        a.put(4,a.r(31)); a.put(5,a.r(0)&size_mask); a.put(0,a.r(5)-4);
        a.store(a.r(30)+a.r(0),a.r(6),0x10774244); a.call(0x10774360,0x1077424c); a.put(3,a.r(31));
    } else a.put(3,0);
    a.pop(64,0x10774250); a.restore(31,0x10774258); a.restore_link(); a.restore(30,0x10774260); a.finish();
}
void coalesce_previous(Abi& a) {
    a.load(0,a.r(3),0x107744d0); a.put(0,a.r(0)&previous_busy); a.record(a.r(0));
    if(a.r(0)) { a.finish(); return; }
    a.load(6,a.r(3)-4,0x107744dc); a.put(0,a.r(6)&in_use); a.record(a.r(0));
    if(a.r(0)) { a.finish(); return; }
    a.put(7,a.r(3)-a.r(6)); a.load(0,a.r(7),0x107744ec); a.put(0,a.r(0)&15); a.store(a.r(7),a.r(0),0x107744f4);
    a.load(0,a.r(3),0x107744f8); a.load(5,a.r(7),0x107744fc);
    a.put(0,((a.r(6)+(a.r(0)&size_mask))&size_mask)|a.r(5)); a.store(a.r(7),a.r(0),0x10774510);
    a.load(0,a.r(7),0x10774514); a.put(0,a.r(0)&in_use); a.record(a.r(0));
    if(!a.r(0)) {
        a.load(0,a.r(3),0x10774520); a.put(0,a.r(0)&size_mask); a.put(5,a.r(6)+a.r(0)); a.put(0,a.r(5)-4);
        a.store(a.r(7)+a.r(0),a.r(5),0x10774530);
    }
    a.load(5,a.r(4),0x10774534); a.compare(a.r(5),a.r(3));
    if(a.r(5)==a.r(3)) { a.load(0,a.r(5)+12,0x10774540); a.store(a.r(4),a.r(0),0x10774544); }
    a.load(0,a.r(3)+8,0x10774548); a.load(4,a.r(3)+12,0x1077454c); a.store(a.r(4)+8,a.r(0),0x10774550);
    a.load(5,a.r(3)+12,0x10774554); a.put(3,a.r(7)); a.load(4,a.r(5)+8,0x1077455c); a.store(a.r(4)+12,a.r(5),0x10774560); a.finish();
}
void coalesce_next(Abi& a) {
    a.load(5,a.r(3),0x10774570); a.put(7,a.r(5)&size_mask); a.put(8,a.r(3)+a.r(7));
    a.load(6,a.r(8),0x1077457c); a.put(0,a.r(6)&in_use); a.record(a.r(0));
    if(a.r(0)) { a.finish(); return; }
    a.put(0,a.r(5)&15); a.store(a.r(3),a.r(0),0x1077458c); a.put(0,a.r(6)&size_mask); a.put(6,a.r(7)+a.r(0));
    a.load(5,a.r(3),0x10774598); a.put(0,(a.r(6)&size_mask)|a.r(5)); a.store(a.r(3),a.r(0),0x107745a4);
    a.load(0,a.r(3),0x107745a8); a.put(0,a.r(0)&in_use); a.record(a.r(0));
    if(!a.r(0)) { a.put(0,a.r(6)-4); a.store(a.r(3)+a.r(0),a.r(6),0x107745b8); }
    a.load(0,a.r(3),0x107745bc); a.put(0,a.r(0)&in_use); a.record(a.r(0));
    if(!a.r(0)) {
        a.load(5,a.r(3)+a.r(6),0x107745c8); a.put(0,a.r(5)&~previous_busy); a.store(a.r(3)+a.r(6),a.r(0),0x107745d4);
    } else {
        a.load(0,a.r(3)+a.r(6),0x107745dc); a.put(0,a.r(0)|previous_busy); a.store(a.r(3)+a.r(6),a.r(0),0x107745e4);
    }
    a.load(3,a.r(4),0x107745e8); a.compare(a.r(3),a.r(8));
    if(a.r(3)==a.r(8)) { a.load(0,a.r(3)+12,0x107745f4); a.store(a.r(4),a.r(0),0x107745f8); }
    a.load(0,a.r(4),0x107745fc); a.compare(a.r(0),a.r(8));
    if(a.r(0)==a.r(8)) { a.put(0,0); a.store(a.r(4),0,0x1077460c); }
    a.load(0,a.r(8)+8,0x10774610); a.load(3,a.r(8)+12,0x10774614); a.store(a.r(3)+8,a.r(0),0x10774618);
    a.load(0,a.r(8)+12,0x1077461c); a.load(3,a.r(8)+8,0x10774620); a.store(a.r(3)+12,a.r(0),0x10774624); a.finish();
}
void release_block(Abi& a) {
    a.save(31,0x10774270); a.capture_link(); a.put(5,~in_use); a.save(30,0x1077427c); a.put(30,a.r(3));
    a.save_link(0x10774284); a.put(0,~previous_busy); a.frame(64,0x1077428c);
    a.load(6,a.r(4),0x10774290); a.put(5,a.r(6)&a.r(5)); a.put(7,a.r(6)&size_mask); a.store(a.r(4),a.r(5),0x1077429c);
    a.put(6,a.r(4)+a.r(7)); a.load(5,a.r(6),0x107742a4); a.put(0,a.r(5)&a.r(0)); a.store(a.r(6),a.r(0),0x107742ac);
    a.store(a.r(6)-4,a.r(7),0x107742b0); a.load(0,a.r(3)+20,0x107742b4); a.put(3,a.r(0)&size_mask);
    a.put(31,a.r(30)+a.r(3)-4); a.load(3,a.r(31),0x107742c4); a.compare(a.r(3));
    if(a.r(3)) {
        a.load(0,a.r(3)+8,0x107742d0); a.store(a.r(4)+8,a.r(0),0x107742d4); a.load(3,a.r(4)+8,0x107742d8);
        a.store(a.r(3)+12,a.r(4),0x107742dc); a.load(0,a.r(31),0x107742e0); a.store(a.r(4)+12,a.r(0),0x107742e4);
        a.load(3,a.r(31),0x107742e8); a.store(a.r(3)+8,a.r(4),0x107742ec); a.store(a.r(31),a.r(4),0x107742f0);
        a.put(4,a.r(31)); a.load(3,a.r(31),0x107742f8); a.call(0x107744d0,0x10774300); a.store(a.r(31),a.r(3),0x10774300);
        a.put(4,a.r(31)); a.load(3,a.r(31),0x10774308); a.call(0x10774570,0x10774310);
    } else {
        a.store(a.r(31),a.r(4),0x10774314); a.store(a.r(4)+8,a.r(4),0x10774318); a.store(a.r(4)+12,a.r(4),0x1077431c);
    }
    a.load(3,a.r(31),0x10774320); a.load(4,a.r(30)+16,0x10774324); a.load(0,a.r(3),0x10774328);
    a.put(0,a.r(0)&size_mask); a.compare(a.r(4),a.r(0)); if(a.r(4)<a.r(0)) a.store(a.r(30)+16,a.r(0),0x10774338);
    a.pop(64,0x1077433c); a.restore(31,0x10774344); a.restore_link(); a.restore(30,0x1077434c); a.finish();
}
void insert_arena(Abi& a) {
    a.load(5,a.r(3),0x10774630); a.compare(a.r(5));
    if(a.r(5)) {
        a.load(0,a.r(5),0x1077463c); a.store(a.r(4),a.r(0),0x10774640); a.load(5,a.r(4),0x10774644);
        a.store(a.r(5)+4,a.r(4),0x10774648); a.load(0,a.r(3),0x1077464c); a.store(a.r(4)+4,a.r(0),0x10774650);
        a.load(5,a.r(3),0x10774654); a.store(a.r(5),a.r(4),0x10774658); a.store(a.r(3),a.r(4),0x1077465c);
    } else { a.store(a.r(3),a.r(4),0x10774664); a.store(a.r(4),a.r(4),0x10774668); a.store(a.r(4)+4,a.r(4),0x1077466c); }
    a.finish();
}
void remove_arena(Abi& a) {
    a.load(5,a.r(4)+4,0x10774680); a.compare(a.r(5),a.r(4)); if(a.r(5)==a.r(4)) a.put(5,0);
    a.load(0,a.r(3),0x10774690); a.compare(a.r(0),a.r(4)); if(a.r(0)==a.r(4)) a.store(a.r(3),a.r(5),0x1077469c);
    a.compare(a.r(5)); if(a.r(5)) {
        a.load(0,a.r(4),0x107746a8); a.store(a.r(5),a.r(0),0x107746ac); a.load(3,a.r(5),0x107746b0); a.store(a.r(3)+4,a.r(5),0x107746b4);
    }
    a.put(0,0); a.store(a.r(4)+4,0,0x107746bc); a.put(3,a.r(5)); a.store(a.r(4),0,0x107746c4); a.finish();
}
void acquire_arena(Abi& a) {
    a.save(31,0x107746d0); a.capture_link(); a.save(30,0x107746d8); a.save(29,0x107746dc); a.put(29,a.r(3));
    a.save_link(0x107746e4); a.put(0,a.r(4)+47); a.put(4,65536); a.put(30,a.r(0)&size_mask); a.frame(80,0x107746f4);
    a.put(0,a.r(4)); a.compare(a.r(30),a.r(0)); if(a.r(30)<a.r(0)) a.put(30,a.r(0));
    a.put(3,a.r(30)); a.call(0x10778d50,0x10774710); a.put(31,a.r(3)); a.record(a.r(31));
    if(!a.r(31)) a.put(3,0);
    else {
        a.put(4,a.r(30)); a.call(0x107740f0,0x10774728); a.put(3,a.r(29)); a.put(4,a.r(31));
        a.call(0x10774630,0x10774734); a.put(3,a.r(31));
    }
    a.pop(80,0x10774738); a.restore(31,0x10774740); a.restore(30,0x10774744); a.restore_link(); a.restore(29,0x1077474c); a.finish();
}
void allocate_growing(Abi& a) {
    a.save(31,0x10774760); a.capture_link(); a.save(30,0x10774768); a.save(29,0x1077476c); a.put(29,a.r(3));
    a.save_link(0x10774774); a.put(0,a.r(4)+23); a.put(30,a.r(0)&size_mask); a.compare(a.r(30),96); a.frame(80,0x10774784);
    if(a.r(30)<96) a.put(30,96);
    a.load(3,a.r(29),0x10774790); a.compare(a.r(3));
    if(!a.r(3)) { a.put(3,a.r(29)); a.put(4,a.r(30)); a.call(0x107746d0,0x107747ac); }
    a.compare(a.r(3)); a.put(31,a.r(3));
    if(!a.r(3)) a.put(3,0);
    else {
        bool found=false;
        do {
            a.s.step(0x107747c0); a.load(0,a.r(31)+16,0x107747c0); a.compare(a.r(30),a.r(0));
            if(a.r(30)<=a.r(0)) {
                a.put(3,a.r(31)); a.put(4,a.r(30)); a.call(0x10774190,0x107747d8); a.compare(a.r(3));
                if(a.r(3)) { a.store(a.r(29),a.r(31),0x107747e0); found=true; break; }
            }
            a.load(31,a.r(31)+4,0x107747e8); a.load(0,a.r(29),0x107747ec); a.compare(a.r(31),a.r(0));
        } while(a.r(31)!=a.r(0));
        if(!found) {
            a.put(3,a.r(29)); a.put(4,a.r(30)); a.call(0x107746d0,0x10774804); a.compare(a.r(3));
            if(a.r(3)) { a.put(4,a.r(30)); a.call(0x10774190,0x1077481c); found=true; } else a.put(3,0);
        }
        // The internal helper adds eight even if a malformed newly acquired
        // arena's block allocation returns zero. Preserve that original path.
        if(found) a.put(3,a.r(3)+8);
    }
    a.pop(80,0x10774820); a.restore(31,0x10774828); a.restore(30,0x1077482c); a.restore_link(); a.restore(29,0x10774834); a.finish();
}
void allocate_existing(Abi& a) {
    a.save(31,0x10774840); a.capture_link(); a.save(30,0x10774848); a.put(30,a.r(5)); a.save(29,0x10774850);
    a.save(28,0x10774854); a.put(28,a.r(3)); a.save_link(0x1077485c); a.put(0,a.r(4)+23); a.put(29,a.r(0)&size_mask);
    a.compare(a.r(29),96); a.frame(80,0x1077486c); if(a.r(29)<96) a.put(29,96);
    a.put(0,0); a.store(a.r(30),0,0x1077487c); a.load(31,a.r(28),0x10774880); a.compare(a.r(31));
    bool found=false;
    if(a.r(31)) do {
        a.s.step(0x10774898); a.load(0,a.r(31)+16,0x10774898); a.compare(a.r(29),a.r(0));
        if(a.r(29)<=a.r(0)) {
            a.put(3,a.r(31)); a.put(4,a.r(29)); a.call(0x10774190,0x107748b0); a.compare(a.r(3));
            if(a.r(3)) { a.store(a.r(28),a.r(31),0x107748b8); found=true; break; }
        }
        a.load(3,a.r(31)+16,0x107748c0); a.compare(a.r(3),8);
        if(a.r(3)>8) {
            a.load(0,a.r(30),0x107748cc); a.put(3,a.r(3)-8); a.compare(a.r(0),a.r(3));
            if(a.r(0)<a.r(3)) a.store(a.r(30),a.r(3),0x107748dc);
        }
        a.load(31,a.r(31)+4,0x107748e0); a.load(0,a.r(28),0x107748e4); a.compare(a.r(31),a.r(0));
    } while(a.r(31)!=a.r(0));
    a.put(3,found?a.r(3)+8:0);
    a.pop(80,0x107748fc); a.restore(31,0x10774904); a.restore(30,0x10774908); a.restore_link(); a.restore(29,0x10774910); a.restore(28,0x10774914); a.finish();
}
void release_ordinary(Abi& a) {
    a.save(31,0x10774920); a.capture_link(); a.put(4,a.r(4)-8); a.save(30,0x1077492c); a.put(30,a.r(3));
    a.save_link(0x10774934); a.frame(64,0x10774938); a.load(0,a.r(4)+4,0x1077493c); a.put(31,a.r(0)&~1u);
    a.put(3,a.r(31)); a.call(0x10774270,0x1077494c); a.load(3,a.r(31)+24,0x1077494c); a.put(5,0);
    a.put(0,a.r(3)&in_use); a.record(a.r(0));
    if(!a.r(0)) {
        a.load(0,a.r(31)+20,0x1077495c); a.put(4,a.r(3)&size_mask); a.put(3,a.r(0)&size_mask); a.put(0,a.r(3)-32);
        a.compare(a.r(4),a.r(0)); if(a.r(4)==a.r(0)) a.put(5,1);
    }
    a.compare(std::bit_cast<std::int32_t>(a.r(5)));
    if(a.r(5)) { a.put(3,a.r(30)); a.put(4,a.r(31)); a.call(0x10774680,0x1077498c); a.put(3,a.r(31)); a.call(0x10778d80,0x10774994); }
    a.pop(64,0x10774994); a.restore(31,0x1077499c); a.restore_link(); a.restore(30,0x107749a4); a.finish();
}
}
