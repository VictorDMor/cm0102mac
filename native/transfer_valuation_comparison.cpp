#include "transfer_valuation_detail.hpp"

namespace cm::aot::transfer_valuation {
using detail::Call;
void equivalent_offer_terms(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,equivalent_offer_terms_entry,d,o);a.link();a.multiple(27);a.move(30,3);a.move(31,4);a.save_link();a.frame(112);a.load(0,4,0x20);a.offset(3,1,0x48);a.value(4,4);a.store(1,0x4c,0);a.load(0,30,0x20);a.store(1,0x48,0);a.call(0x10009f20);a.move(28,4);a.move(27,3);a.offset(3,1,0x4c);a.value(4,4);a.call(0x10009f20);a.combine(4,4,28,false,true);a.combine(0,3,27,false,true);a.combine(0,4,0);a.offset_carry_at(0x10416e64,4,0,-1);a.subtract_record_at(0x10416e68,0,4,0);
    const auto finish=[&]{a.at(0x104170dc);a.pop(112);a.multiple(27,true);a.finish();};
    if(!a.equal()){a.value(3,0);a.jump(0x104170dc);return finish();}
    a.at(0x10416e78);a.load(0,31,0x40);a.offset(3,1,0x40);a.value(29,0);a.value(4,4);a.store(1,0x44,0);a.load(0,30,0x40);a.store(1,0x40,0);a.call(0x10009f20);a.move(28,4);a.move(27,3);a.offset(3,1,0x44);a.value(4,4);a.call(0x10009f20);a.combine(4,4,28,false,true);a.combine(0,3,27,false,true);a.combine(0,4,0,true);
    bool different=!a.equal();if(!different){a.load<1>(3,31,0x44);a.load<1>(0,30,0x44);a.sign8(3,3);a.sign8(0,0);a.compare_registers(3,0);different=!a.equal();}if(different){a.at(0x10416ed4);a.value(29,1);}
    a.at(0x10416ed8);a.compare(29,0);if(!a.equal()){a.load(0,31,0x40);a.compare(0,0);bool positive=a.positive();if(!positive){a.load(0,30,0x40);a.compare(0,0);positive=!a.nonpositive();}if(positive){a.at(0x10416ef8);a.value(3,0);a.jump(0x104170dc);return finish();}}
    a.at(0x10416f00);a.load(0,31,0x48);a.offset(3,1,0x38);a.value(27,0);a.value(4,4);a.store(1,0x3c,0);a.load(0,30,0x48);a.store(1,0x38,0);a.call(0x10009f20);a.move(28,4);a.move(29,3);a.offset(3,1,0x3c);a.value(4,4);a.call(0x10009f20);a.combine(4,4,28,false,true);a.combine(0,3,29,false,true);a.combine(0,4,0,true);
    different=!a.equal();if(!different){a.load<1>(3,31,0x4c);a.load<1>(0,30,0x4c);a.sign8(3,3);a.sign8(0,0);a.compare_registers(3,0);different=!a.equal();}if(different){a.at(0x10416f5c);a.value(27,1);}
    a.at(0x10416f60);a.compare(27,0);if(!a.equal()){a.load(0,31,0x48);a.compare(0,0);bool positive=a.positive();if(!positive){a.load(0,30,0x48);a.compare(0,0);positive=!a.nonpositive();}if(positive){a.at(0x10416f80);a.value(3,0);a.jump(0x104170dc);return finish();}}
    a.at(0x10416f88);a.load<1>(4,31,0x50);a.load<1>(5,30,0x50);a.sign8(3,4);a.sign8(0,5);a.compare_registers(3,0);if(!a.equal()){a.sign8(0,4,true);bool positive=a.positive();if(!positive){a.sign8(0,5,true);positive=!a.nonpositive();}if(positive){a.at(0x10416fb0);a.value(3,0);a.jump(0x104170dc);return finish();}}
    a.at(0x10416fb8);a.load<1>(4,31,0x51);a.load<1>(5,30,0x51);a.sign8(3,4);a.sign8(0,5);a.compare_registers(3,0);if(!a.equal()){a.sign8(0,4,true);bool positive=a.positive();if(!positive){a.sign8(0,5,true);positive=!a.nonpositive();}if(positive){a.at(0x10416fe0);a.value(3,0);a.jump(0x104170dc);return finish();}}
    a.at(0x10416fe8);a.load<1>(3,31,0x38);a.load<1>(0,30,0x38);a.mask(3,3,0xf8000000,false,24);a.shift_right(3,3,28);a.mask(0,0,0xf8000000,false,24);a.shift_right(0,0,28);a.sign8(3,3);a.sign8(0,0);a.compare_registers(3,0);if(!a.equal()){a.value(3,0);a.jump(0x104170dc);return finish();}
    a.at(0x10417018);a.load(4,31,4);a.compare(4,0,false);if(a.equal()){a.load(0,30,4);a.compare(0,0,false);if(a.equal()){a.value(3,1);a.jump(0x104170dc);return finish();}}
    a.at(0x10417038);a.compare(4,0,false);bool both=!a.equal();if(both){a.load(5,30,4);a.compare(5,0,false);both=!a.equal();}if(!both){a.at(0x1041704c);a.value(3,0);a.jump(0x104170dc);return finish();}
    a.at(0x10417054);a.load<2>(3,4,0,true);a.load<2>(0,5,0,true);a.compare_registers(3,0);if(!a.equal()){a.value(3,0);a.jump(0x104170dc);return finish();}
    a.at(0x1041706c);a.load(0,4,4);a.store(4,12,0);a.jump(0x104170c8);
    for(;;){a.at(0x104170c8);a.load(3,31,4);a.call(0x10738e00);a.move(29,3,true);if(a.equal())break;
        a.at(0x10417078);a.load(3,30,4);a.value(27,0);a.load(0,3,4);a.store(3,12,0);a.jump(0x104170a8);
        for(;;){a.at(0x104170a8);a.load(3,30,4);a.call(0x10738e00);a.compare(3,0,false);if(a.equal())break;
            a.at(0x10417090);a.load(4,29,0);a.load(0,3,0);a.compare_registers(4,0);if(a.equal()){a.value(27,1);a.jump(0x104170b8);break;}}
        a.at(0x104170b8);a.sign8(0,27,true);if(a.equal()){a.value(3,0);a.jump(0x104170dc);return finish();}
    }
    a.at(0x104170d8);a.value(3,1);finish();
}
}
