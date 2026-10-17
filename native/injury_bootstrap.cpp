#include "injury_bootstrap.hpp"
#include "injury_bootstrap_detail.hpp"

namespace cm::aot::injury_bootstrap {
using namespace detail;
namespace {
void verify_controller_resource(Call& a) {
    bool retry;
    do {
        a.at(0x10184108);a.load(10,31,0);a.offset(11,1,0xcd);a.load(9,31,4);a.offset(12,1,0x71);a.load(8,31,8);a.offset(24,1,0x41);a.load(7,31,12);a.value(27,0);a.load<1>(6,31,0x10);
        a.load(5,30,0);a.load(4,30,4);a.load(3,30,8);a.load(0,30,12);a.store(1,0x70,10);a.store(1,0x74,9);a.store(1,0x78,8);a.store(1,0x7c,7);a.store<1>(1,0x80,6);a.store(1,0x40,5);a.store(1,0x44,4);a.store(1,0x48,3);a.store(1,0x4c,0);a.jump(0x10184184);
        while(true) {
            a.at(0x10184184);a.load<1>(0,12,0);a.sign8(0,0,true);if(a.equal())break;
            a.at(0x10184168);a.load<1>(3,12,0);a.offset(12,12,1);a.load<1>(0,24,0);a.offset(24,24,1);a.store<1>(11,0,3);a.store<1>(11,1,0);a.offset(11,11,2);
        }
        a.at(0x10184190);a.load<1>(7,1,0x70);a.offset(5,1,0xcc);a.load<1>(0,1,0x40);a.offset(6,1,0x84);a.value(3,0);a.value(4,0);a.add(0,7,0);a.store<1>(1,0xcc,0);a.call(0x1077fee0);a.load(2,1,0x14);a.sign16(0,3,true);
        if(a.equal())a.value(27,1);
        a.at(0x101841c4);a.compare(27,0);
        if(a.equal()){a.move(4,29);a.offset(3,1,0x294);a.call(0x10727890);a.offset(3,1,0x1cc);a.offset(4,29,0xa);a.call(0x10727890);a.offset(3,1,0x294);a.offset(4,1,0x1cc);a.value(5,1);a.call(0x10769b60);}
        a.at(0x101841f4);a.compare(27,0);retry=a.equal();
    }while(retry);
}
}
void construct_controller(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,constructor_entry,d,o);
    a.link();a.multiple(24);a.load(28,2,-0x7ba8);a.move(25,3);a.load(29,2,-0x5438);a.move(26,4);a.load(30,2,-0x5434);a.load(31,2,-0x5430);a.save_link();a.frame(896);
    verify_controller_resource(a);
    a.at(0x101841fc);a.value(3,8);a.call(0x10773520);a.move(24,3,true);
    if(!a.equal()){a.offset(3,29,0x37);a.offset(4,29,0x40);a.call(0x10746280);a.move(4,3);a.move(3,24);a.value(5,0x34);a.call(0x10748bb0);}
    a.at(0x10184228);a.store(25,12,24);a.load(0,25,12);a.compare(0,0,false);
    if(a.equal()) {
        report_allocation_failure(a,0x10184238,29,28,0x50,0x38,0xb4);a.load(4,2,-0x7bac);a.value(0,0);a.value(3,0xffffffff);a.store<1>(4,0,0);a.call(0x1076f7d0);a.move(3,25);a.jump(0x101842f4);
    } else {
        a.at(0x1018429c);a.value(0,0);a.compare(26,0,false);for(auto offset:{4,8,16,20,24,28,32,36,40})a.store(25,offset,0);
        bool populate=a.equal();
        if(!populate){a.move(3,25);a.move(4,26);a.call(0x10184530);a.sign8(0,3,true);populate=a.equal();if(!populate){a.move(3,25);a.jump(0x101842f4);}}
        if(populate){a.at(0x101842e8);a.move(3,25);a.call(population_entry);a.move(3,25);}
    }
    a.at(0x101842f4);a.pop(896);a.multiple(24,true);a.finish();
}

void destroy_controller(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,destructor_entry,d,o);
    a.link();a.save(31);a.move(31,4);a.save(30);a.move(30,3,true);a.save_link();a.frame(64);
    if(!a.equal()) {
        a.load(3,30,0);a.compare(3,0,false);if(!a.equal())a.call(0x10775170);
        a.at(0x10184340);a.load(3,30,4);a.compare(3,0,false);if(!a.equal())a.call(0x10775170);
        a.at(0x10184350);a.load(3,30,12);a.compare(3,0,false);if(!a.equal()){a.value(4,1);a.call(0x10748c80);}
        a.at(0x10184364);a.load(3,30,16);a.compare(3,0,false);if(!a.equal())a.call(0x10775170);
        a.at(0x10184374);a.load(3,30,40);a.compare(3,0,false);if(!a.equal()){a.value(4,1);a.call(0x10738c20);}
        a.at(0x10184388);a.load(3,30,28);a.compare(3,0,false);if(!a.equal())a.call(0x10775170);
        a.at(0x10184398);a.sign16(0,31,true);if(!a.nonpositive()){a.move(3,30);a.call(0x107735a0);}
    }
    a.at(0x101843a8);a.move(3,30);a.pop(64);a.restore(31);a.restore(30);a.finish();
}

void set_staff_injury_flag(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,staff_flag_entry,d,o);a.load(0,4,0);a.load(3,3,0);a.multiply(0,0,0x24);a.add(3,3,0);a.store<1>(3,0x22,5);a.finish();
}
}
