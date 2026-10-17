#include "injury_persistence.hpp"
#include "injury_persistence_detail.hpp"

namespace cm::aot::injury_persistence {
using namespace detail;
void save_controller(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,save_entry,d,o);
    a.link();a.multiple(27);a.compare(4,0,false);a.load(27,2,-0x7be4);a.load(29,2,-0x7bac);a.move(28,3);a.load(30,2,-0x7ba8);a.load(31,2,-0x5438);a.save_link();a.frame(528);
    if(a.equal()){fail_save(a,0x10184e8c,0x1d8,0x90,0x210,false);return;}
    a.at(0x10184ee0);a.move(3,4);a.offset(4,31,0x71);a.value(5,1);a.value(6,0x16);a.value(7,4);a.call(0x107475b0);a.move(6,3,true);a.store(1,0x3c,3);
    if(a.equal()){fail_save(a,0x10184f04,0x1b8,0x88,0x218,false);return;}
    a.at(0x10184f58);a.load(3,28,0);a.value(4,0x24);a.load(5,27,0);a.call(0x10747cc0);a.load(0,27,0);a.compare_registers(0,3);
    if(!a.equal()){fail_save(a,0x10184f74,0x198,0x80,0x21f);return;}
    a.at(0x10184fd0);a.load(6,1,0x3c);a.offset(3,28,8);a.value(4,4);a.value(5,1);a.call(0x10747cc0);a.compare(3,1);
    if(!a.equal()){fail_save(a,0x10184fec,0x178,0x78,0x227);return;}
    a.at(0x10185048);a.load(3,28,4);a.value(4,0x38);a.load(5,28,8);a.load(6,1,0x3c);a.call(0x10747cc0);a.load(0,28,8);a.compare_registers(0,3);
    if(!a.equal()){fail_save(a,0x10185068,0x158,0x70,0x22f);return;}
    a.at(0x101850c4);a.load(6,1,0x3c);a.offset(3,28,0x18);a.value(4,4);a.value(5,1);a.call(0x10747cc0);a.compare(3,1);
    if(!a.equal()){fail_save(a,0x101850e0,0x138,0x68,0x237);return;}
    a.at(0x1018513c);a.load(5,28,0x18);a.compare(5,0);
    if(!a.equal()) {
        a.load(3,28,0x10);a.value(4,0x18);a.load(6,1,0x3c);a.call(0x10747cc0);a.load(0,28,0x18);a.compare_registers(0,3);
        if(!a.equal()){fail_save(a,0x10185164,0x118,0x60,0x242);return;}
    }
    a.at(0x101851c0);a.load(6,1,0x3c);a.offset(3,28,0x24);a.value(4,4);a.value(5,1);a.call(0x10747cc0);a.compare(3,1);
    if(!a.equal()){fail_save(a,0x101851dc,0xf8,0x58,0x24b);return;}
    a.at(0x10185238);a.load(5,28,0x24);a.compare(5,0);
    if(!a.equal()) {
        a.load(3,28,0x1c);a.value(4,0x14);a.load(6,1,0x3c);a.call(0x10747cc0);a.load(0,28,0x24);a.compare_registers(0,3);
        if(!a.equal()){fail_save(a,0x10185260,0xd8,0x50,0x257);return;}
    }
    a.at(0x101852bc);a.load(3,28,0x28);a.compare(3,0,false);
    if(!a.equal()){a.load<2>(0,3,0,true);a.jump(0x101852d4);}else {a.at(0x101852d0);a.value(0,0);}
    a.at(0x101852d4);a.store<2>(1,0x38,0);a.offset(3,1,0x38);a.load(6,1,0x3c);a.value(4,2);a.value(5,1);a.call(0x10747cc0);a.compare(3,1);
    if(!a.equal()){fail_save(a,0x101852f4,0xb8,0x48,0x266);return;}
    a.at(0x10185350);a.load<2>(0,1,0x38,true);a.compare(0,0);
    if(!a.equal()) {
        a.load(3,28,0x28);a.load(0,3,4);a.store(3,12,0);a.jump(0x101853d8);
        while(true) {
            a.at(0x101853d8);a.load(3,28,0x28);a.call(0x10738e00);a.compare(3,0,false);if(a.equal())break;
            a.at(0x1018536c);a.load(6,1,0x3c);a.value(4,8);a.value(5,1);a.call(0x10747cc0);a.compare(3,1);
            // This original late-write failure leaves the file open.
            if(!a.equal()){fail_save(a,0x10185384,0x98,0x40,0x278,false);return;}
        }
    }
    a.at(0x101853e8);a.offset(3,1,0x3c);a.call(0x10747900);finish_save(a);
}
}
