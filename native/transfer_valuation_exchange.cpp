#include "transfer_valuation_detail.hpp"

namespace cm::aot::transfer_valuation {
using detail::Call;
void staff_offer_status(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,staff_offer_status_entry,d,o);a.link();a.save(31);a.move(31,4);a.save(30);a.move(30,3);a.save_link();a.frame(64);a.load(0,3,12);a.compare_registers(31,0);
    if(a.equal()){a.load<1>(0,30,0x34);a.compare(0,3);if(a.equal()){a.value(0,4);a.store<1>(30,0x10,0);}a.at(0x10416d9c);a.load<1>(3,30,0x10);a.jump(0x10416dec);}
    else{a.at(0x10416da4);a.load(3,30,4);a.compare(3,0,false);bool found=false;if(!a.equal()){a.load(0,3,4);a.store(3,12,0);a.jump(0x10416dd4);bool initial=true;
        for(;;){a.at(initial?0x10416dd4:0x10416dd8);if(initial){a.flags(0,0,0);initial=false;}a.load(3,30,4);a.call(0x10738e00);a.compare(3,0,false);if(a.equal())break;
            a.at(0x10416dc0);a.load(0,3,0);a.compare_registers(0,31);if(a.equal()){a.load<1>(3,3,4);a.jump(0x10416dec);found=true;break;}}
        }if(!found){a.at(0x10416de8);a.value(3,4);}}
    a.at(0x10416dec);a.pop(64);a.restore(31);a.restore(30);a.finish();
}
void find_exchange_staff(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,find_exchange_staff_entry,d,o);a.link();a.save(31);a.move(31,4);a.save(30);a.move(30,3);a.save_link();a.frame(64);a.load(3,3,4);a.compare(3,0,false);
    if(a.equal()){a.value(3,0);a.jump(0x104172cc);}
    else{a.at(0x10417290);a.load(0,3,4);a.store(3,12,0);a.jump(0x104172b4);bool initial=true,found=false;
        for(;;){a.at(initial?0x104172b4:0x104172b8);if(initial){a.flags(0,0,0);initial=false;}a.load(3,30,4);a.call(0x10738e00);a.compare(3,0,false);if(a.equal())break;
            a.at(0x104172a0);a.load(4,3,0);a.load(0,31,0);a.compare_registers(4,0);if(a.equal()){a.jump(0x104172cc);found=true;break;}}
        if(!found){a.at(0x104172c8);a.value(3,0);}}
    a.at(0x104172cc);a.pop(64);a.restore(31);a.restore(30);a.finish();
}
void remove_exchange_staff(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,remove_exchange_staff_entry,d,o);a.link();a.multiple(26);a.move(29,3);a.load(31,2,-0x7b7c);a.load(26,2,-0x7b78);a.move(30,4);a.load(27,2,-0x7ba8);a.load(28,2,-0x4b88);a.save_link();a.frame(128);a.load(3,3,4);a.compare(3,0,false);
    if(a.equal()){detail::diagnostic(a,0x10417124,28,27,0x40,0x38,0x677);a.load(3,2,-0x7bac);a.value(0,0);a.store<1>(3,0,0);a.jump(0x1041724c);}
    else{a.at(0x1041717c);a.load(0,3,4);a.store(3,12,0);a.jump(0x104171a0);
        for(;;){a.at(0x104171a0);a.load(3,29,4);a.call(0x10738e00);a.move(4,3,true);if(a.equal())break;
            a.at(0x10417188);a.load(3,4,0);a.load(0,30,0);a.compare_registers(3,0);if(a.equal()){a.load(3,29,4);a.call(0x10738e60);}}
        a.at(0x104171b0);a.load(3,29,4);a.load<2>(0,3,0,true);a.sign16(0,0,true);if(a.equal()){a.value(4,1);a.call(0x10738c20);a.value(0,0);a.store(29,4,0);}
        a.at(0x104171d0);a.load(3,31,0);a.move(4,30);a.call(0x10082f20);a.compare(3,0,false);a.store(26,0,3);if(!a.equal()){a.load(0,3,4);a.load(3,2,-0x7be8);a.multiply(0,0,0x248);a.load(3,3,0);a.add(5,3,0);a.jump(0x10417204);}else{a.at(0x10417200);a.value(5,0);}
        a.at(0x10417204);a.load(3,31,0);a.move(4,30);a.value(6,0);a.value(7,0xffffffff);a.value(8,0);a.call(0x10084be0);a.load(0,29,0x20);a.load(4,2,-0x7b40);a.add(0,0,3);a.store(29,0x20,0);a.load(0,30,0);a.load(3,4,0);a.multiply(0,0,10);a.load(3,3,0x9a0);a.add(4,3,0);a.load<1>(3,4,2);a.offset(0,3,-1);a.store<1>(4,2,0);
    }
    a.at(0x1041724c);a.pop(128);a.multiple(26,true);a.finish();
}
}
