#include "shortlist_links.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::shortlist_links {
using Call=SemanticGuestCall;
void contains_staff_club_link(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,contains_staff_club_link_entry,d,o);a.link();a.move(6,3);a.save_link();a.frame(0x40);a.load(0,3,0);a.compare(0,0,false);
    if(a.equal()){a.value(3,0);a.jump(0x10362de4);}
    else{
        a.at(0x10362d94);a.load(0,4,0);a.compare(5,0,false);a.store(1,0x38,0);
        if(!a.equal()){a.load(0,5,0);a.store(1,0x3c,0);a.jump(0x10362db8);}else{a.at(0x10362db0);a.value(0,0xffffffff);a.store(1,0x3c,0);}
        a.at(0x10362db8);a.load(4,6,0);a.offset(3,1,0x38);a.load(5,6,0x10);a.value(6,8);a.load(7,2,-0x62a8);a.call(0x10775600);a.compare(3,0,false);
        if(!a.equal()){a.value(3,1);a.jump(0x10362de4);}else{a.at(0x10362de0);a.value(3,0);}
    }
    a.at(0x10362de4);a.pop(0x40);a.finish();
}
void collect_club_links(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,collect_club_links_entry,d,o);a.link();a.save(31);a.load(7,2,-0x6298);a.value(6,8);a.save(30);a.move(30,3);a.save(29);a.move(29,5);a.save(28);a.move(28,4);a.save_link();a.value(0,0xffffffff);a.frame(0x50);a.load(3,4,0);a.store(1,0x3c,3);a.offset(3,1,0x38);a.store(1,0x38,0);a.load(4,30,4);a.load(5,30,0x14);a.call(0x10775600);a.move(31,3,true);
    if(!a.equal()){
        a.load(3,30,0x14);a.load(4,30,4);a.offset(0,3,-1);a.mask(0,0,0xfffffff8,false,3);a.add(30,4,0);a.jump(0x10362e90);
        a.at(0x10362e90);a.flags(0,0,0); // The indexed original no-op is counted once.
        for(;;){
            a.at(0x10362e94);a.compare_registers(31,4,false);if(!a.positive())break;
            a.at(0x10362e74);a.load(3,31,-4);a.offset(31,31,-8);a.load(0,28,0);a.compare_registers(3,0);
            if(!a.equal()){a.offset(31,31,8);a.jump(0x10362e9c);break;}
        }
        for(;;){
            a.at(0x10362e9c);a.load(3,31,4);a.load(0,28,0);a.compare_registers(3,0);if(!a.equal())break;
            a.move(3,29);a.move(4,31);a.value(5,8);a.call(0x10738d10);a.compare_registers(31,30,false);if(a.equal())break;
            a.offset(31,31,8);a.jump(0x10362e9c);
        }
    }
    a.at(0x10362ecc);a.pop(0x50);a.restore(31);a.restore(30);a.restore(29);a.restore(28);a.finish();
}
namespace {
void append_reserved_staff_links(Call& a) {
    a.at(0x10362f30);a.sign8(20,25);a.value(28,0);a.jump(0x10362fd0);
    for(;;){
        a.at(0x10362fd0);a.sign16(0,28);a.compare(0,16);if(a.at_least())break;
        a.compare(20,3);if(a.equal())break;
        a.at(0x10362f3c);a.load(0,30,0);a.sign16(3,28);a.load(4,31,0);a.subtract(3,3,0);a.offset(0,3,-1);a.multiply(0,0,0x70);a.add(27,4,0);a.load<1>(0,27,0x6b);a.sign8(0,0,true);bool append=!a.equal();
        if(append){a.load(3,27,0x24);a.compare(3,0,false);append=!a.equal();}
        if(append){a.value(4,0);a.value(5,1);a.call(0x1014c490);a.sign8(0,3,true);if(!a.equal()){a.sign8(0,25);a.compare(0,2);append=!a.equal();}}
        if(append){a.at(0x10362f90);a.load(3,29,0);a.move(4,22);a.move(5,27);a.call(0x1030fbf0);a.sign8(0,3,true);append=!a.equal();}
        if(append){a.load(0,22,0);a.move(3,23);a.offset(4,1,0x38);a.value(5,8);a.store(1,0x38,0);a.load(6,27,0x24);a.load(0,6,0);a.store(1,0x3c,0);a.call(0x10738d10);}
        a.at(0x10362fcc);a.offset(28,28,1);
    }
}
void append_cached_staff_links(Call& a) {
    a.at(0x10362fec);a.value(0,0xffffffff);a.load(7,2,-0x62a8);a.store(1,0x44,0);a.offset(3,1,0x40);a.value(6,8);a.load(0,22,0);a.store(1,0x40,0);a.load(4,21,0);a.load(5,21,0x10);a.call(0x10775600);a.move(29,3,true);
    if(a.equal())return;
    a.load(3,21,0x10);a.load(4,21,0);a.offset(0,3,-1);a.mask(0,0,0xfffffff8,false,3);a.add(30,4,0);a.jump(0x1036304c);
    for(;;){
        a.at(0x1036304c);a.compare_registers(29,4,false);if(!a.positive())break;
        a.at(0x10363034);a.load_update(3,29,-8);a.load(0,22,0);a.compare_registers(3,0);if(!a.equal()){a.offset(29,29,8);a.jump(0x10363054);break;}
    }
    for(;;){
        a.at(0x10363054);a.load(0,29,0);a.load(7,22,0);a.compare_registers(0,7);if(!a.equal())break;
        a.load(9,29,4);a.value(5,0);a.load(4,21,0xdc);a.value(0,40);a.multiply(3,9,12);a.move(6,5);a.add(8,4,3);a.count_destination(0);
        for(;;){
            a.at(0x10363084);a.load(4,8,0);a.offset(0,6,0x10);a.indexed_load(0,4,0);a.compare_registers(7,0);
            if(a.equal()){a.multiply(3,5,44);a.offset(0,3,8);a.indexed_load(3,4,0);a.jump(0x103630bc);break;}
            a.at(0x103630ac);a.offset(6,6,44);a.offset(5,5,1);if(!a.count_nonzero()){a.load(3,4,8);break;}
        }
        a.at(0x103630bc);a.sign8(0,24,true);bool check=!a.equal();
        if(!check){a.load<1>(0,8,9);a.compare(0,100,false);check=!a.at_least();}
        bool append=false;
        if(check){a.at(0x103630d0);a.load(0,22,8);a.compare_registers(3,0);if(!a.nonpositive()){a.load(3,22,0x24);a.compare(3,0,false);if(a.equal()){append=true;}else{a.load(0,3,0);a.compare_registers(0,9);append=!a.equal();}}}
        if(append){a.at(0x103630f4);a.move(3,23);a.move(4,29);a.value(5,8);a.call(0x10738d10);}
        a.at(0x10363104);a.compare_registers(29,30,false);if(a.equal())break;a.offset(29,29,8);a.jump(0x10363054);
    }
}
}
void collect_staff_links(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,collect_staff_links_entry,d,o);a.link();a.multiple(20);a.move(23,5,true);a.load(29,2,-0x7b1c);a.load(30,2,-0x7be4);a.move(21,3);a.load(31,2,-0x7be0);a.move(22,4);a.move(24,6);a.move(25,7);a.move(26,8);a.save_link();a.frame(0x80);
    if(!a.equal()){a.move(3,23);a.call(0x10738ca0);}append_reserved_staff_links(a);a.at(0x10362fe4);a.sign8(0,26,true);if(a.equal())append_cached_staff_links(a);
    a.at(0x10363114);a.pop(0x80);a.multiple(20,true);a.finish();
}
}
