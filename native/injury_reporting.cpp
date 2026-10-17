#include "injury_reporting.hpp"
#include "injury_reporting_detail.hpp"

namespace cm::aot::injury_reporting {
using namespace detail;
void has_special_injury_type(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,special_type_entry,d,o);
    a.link();a.save(31);a.load(3,2,-0x7ba0);a.save(30);a.load(30,2,-0x7ba8);a.save(29);a.load(31,2,-0x5438);a.save(28);a.move(28,4,true);a.save_link();a.frame(112);a.load(29,3,0);
    if(a.equal()) {
        report_invalid(a,0x1018f374,31,30,0x40,0x38,0xe23);a.load(3,2,-0x7bac);a.value(4,0);a.store<1>(3,0,4);a.jump(0x1018f428);
    } else {
        a.at(0x1018f3cc);bool valid=a.equal();
        if(!valid){a.load(3,2,-0x7be4);a.load(4,28,0);a.load(0,3,0);a.compare_registers(4,0);valid=a.less();}
        if(!valid){a.value(4,0);a.jump(0x1018f428);}
        else {
            a.at(0x1018f3ec);a.move(3,29);a.move(4,28);a.call(0x10187a40);a.load(0,28,0);a.load(3,29,0);a.multiply(0,0,0x24);a.add(3,3,0);a.load(0,3,0x10);a.compare(0,-1);
            if(!a.equal()){a.multiply(0,0,0x38);a.load(3,29,4);a.add(4,3,0);a.jump(0x1018f428);}
            else{a.at(0x1018f424);a.value(4,0);}
        }
    }
    a.at(0x1018f428);a.compare(4,0,false);bool special=false;
    if(!a.equal()){a.load<1>(0,4,0);a.compare(0,81);special=a.equal();}
    if(special){a.value(3,1);a.jump(0x1018f448);}else{a.at(0x1018f444);a.value(3,0);}
    a.at(0x1018f448);a.pop(112);a.restore(31);a.restore(30);a.restore(29);a.restore(28);a.finish();
}

void clear_injury_history(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,clear_history_entry,d,o);
    a.link();a.multiple(27);a.move(28,4,true);a.load(29,2,-0x7bac);a.load(30,2,-0x7ba8);a.move(27,3);a.load(31,2,-0x5438);a.save_link();a.frame(160);
    if(a.equal()) {
        report_invalid(a,0x1018fe18,31,30,0x68,0x40,0x1462);a.value(0,0);a.value(3,0xffffffff);a.store<1>(29,0,0);a.call(0x1076f7d0);
    }
    a.at(0x1018fe70);a.compare(28,0,false);
    if(a.equal()) {
        report_invalid(a,0x1018fe78,31,30,0x48,0x38,0x1466);a.value(0,0);a.value(3,0xffffffff);a.store<1>(29,0,0);a.call(0x1076f7d0);
    }
    a.at(0x1018fed0);a.load(0,28,0);a.load(4,27,0);a.multiply(0,0,0x24);a.load(3,27,12);a.add(5,4,0);a.offset(4,5,0x18);a.offset(5,5,0x1c);a.call(0x10749fc0);
    a.pop(160);a.multiple(27,true);a.finish();
}
}
