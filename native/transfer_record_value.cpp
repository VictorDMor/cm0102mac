#include "transfer_record_detail.hpp"

namespace cm::aot::transfer_records {
using detail::Call;
void offer_value(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,offer_value_entry,d,o);a.link();a.multiple(26);a.move(26,3);a.load(30,2,-0x7be0);a.load(31,2,-0x7b7c);a.move(27,6);a.save_link();a.sign8(0,4,true);a.frame(80);a.load(3,3,12);a.load(4,30,0);a.multiply(0,3,0x70);a.load(29,26,0x20);a.add(28,4,0);
    if(a.equal()){
        a.load(0,26,0x40);a.compare(0,0);if(!a.nonpositive())a.add(29,29,0);
        a.at(0x1041675c);a.load(0,26,0x48);a.compare(0,0);if(!a.nonpositive())a.add(29,29,0);
        a.at(0x1041676c);a.load<1>(3,26,0x50);a.sign8(0,3,true);if(!a.nonpositive()){a.sign8(0,7,true);if(!a.equal()){a.load(4,28,8);a.sign8(0,3);a.value(3,0x51ec0000);a.product(0,4,0);a.offset(3,3,-0x7ae1);a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,1,false,1);a.add(0,0,3);a.add(29,29,0);}}
    }
    a.at(0x104167a8);a.load(3,26,4);a.compare(3,0,false);bool exchange=!a.equal();if(exchange){a.sign8(0,5,true);exchange=a.equal();}
    if(exchange){a.load(0,3,4);a.store(3,12,0);a.jump(0x1041680c);
        for(;;){a.at(0x1041680c);a.load(3,26,4);a.call(0x10738e00);a.compare(3,0,false);if(a.equal())break;
            a.at(0x104167c8);a.sign8(0,27,true);
            if(!a.equal()){
                // This mode deliberately values the main target once per list item.
                a.load(3,31,0);a.move(4,28);a.value(5,0);a.value(6,0);a.value(7,0xffffffff);a.value(8,0);a.call(0x10084be0);a.add(29,29,3);a.jump(0x1041680c);
            }else{a.at(0x104167f4);a.load(0,3,0);a.load(4,30,0);a.multiply(3,0,0x70);a.offset(0,3,8);a.indexed_load(0,4,0);a.add(29,29,0);}
        }
    }
    a.at(0x1041681c);a.move(3,29);a.pop(80);a.multiple(26,true);a.finish();
}
}
