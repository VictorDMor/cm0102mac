#include "money_value_detail.hpp"
namespace cm::aot::money_value {
namespace {
using detail::Call;
bool next_threshold(Call& a,unsigned offset,unsigned target=0) {
    a.move(3,30);a.move(4,29);a.call(0x10773c80);a.floating_load(0,31,offset);
    a.floating_product(target,0,31);a.floating_compare(1,target);a.compare_less_or_equal_bit();return a.equal();
}
void constant_quantum(Call& a,unsigned offset) {
    a.floating_load(0,31,offset);a.floating_product(1,0,31);a.call(0x10773de0);a.jump(0x1000cb90);
}
void cached_quantum(Call& a,unsigned source) {
    a.floating_move(1,source);a.call(0x10773de0);a.jump(0x1000cb90);
}
void large_steps(Call& a) {
    a.call(0x10773c80);a.floating_load(0,31,0x148);a.floating_load(31,2,-0x3afc);
    a.floating_product(30,0,31);a.floating_compare(1,30);a.compare_less_or_equal_bit();
    if(a.equal()){constant_quantum(a,0xc0);return;}
    a.at(0x1000c7d4);if(next_threshold(a,0x150)){constant_quantum(a,0x158);return;}
    a.at(0x1000c804);if(next_threshold(a,0x160,29)){constant_quantum(a,0x168);return;}
    a.at(0x1000c834);if(next_threshold(a,0x170,28)){cached_quantum(a,30);return;}
    a.at(0x1000c860);if(next_threshold(a,0x138,30)){constant_quantum(a,0x178);return;}
    a.at(0x1000c890);if(next_threshold(a,0x180)){cached_quantum(a,29);return;}
    a.at(0x1000c8bc);if(next_threshold(a,0x188)){cached_quantum(a,28);return;}
    a.at(0x1000c8e8);if(next_threshold(a,0x190)){constant_quantum(a,0x198);return;}
    a.at(0x1000c918);cached_quantum(a,30);
}
void fine_steps(Call& a) {
    a.call(0x10773c80);a.floating_load(0,31,0x60);a.floating_load(31,2,-0x3afc);
    a.floating_product(28,0,31);a.floating_compare(1,28);a.compare_less_or_equal_bit();
    if(a.equal()){cached_quantum(a,31);return;}
    a.at(0x1000c954);if(next_threshold(a,0x40,29)){constant_quantum(a,0xf0);return;}
    a.at(0x1000c984);if(next_threshold(a,0x118,30)){cached_quantum(a,28);return;}
    a.at(0x1000c9b0);if(next_threshold(a,0xc0)){constant_quantum(a,0x1a0);return;}
    a.at(0x1000c9e0);if(next_threshold(a,0x168,28)){cached_quantum(a,29);return;}
    a.at(0x1000ca0c);if(next_threshold(a,0x1a8)){cached_quantum(a,30);return;}
    a.at(0x1000ca38);if(next_threshold(a,0x1b0)){constant_quantum(a,0x1b8);return;}
    a.at(0x1000ca68);if(next_threshold(a,0x160,29)){cached_quantum(a,28);return;}
    a.at(0x1000ca94);if(next_threshold(a,0x170,28)){constant_quantum(a,0x148);return;}
    a.at(0x1000cac4);if(next_threshold(a,0x138,30)){constant_quantum(a,0x178);return;}
    a.at(0x1000caf4);if(next_threshold(a,0x180)){cached_quantum(a,29);return;}
    a.at(0x1000cb20);if(next_threshold(a,0x188)){cached_quantum(a,28);return;}
    a.at(0x1000cb4c);if(next_threshold(a,0x190)){constant_quantum(a,0x198);return;}
    a.at(0x1000cb7c);cached_quantum(a,30);
}
}
void amount_quantum(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,amount_quantum_entry,d,o);
    a.link();a.move(11,1);a.call(0x10773808);a.store(1,-0x24,31);a.store(1,-0x28,30);a.store(1,-0x2c,29);
    a.save_link();a.frame(0x70);a.sign8(5,5);a.load(31,2,-0x59d4);a.mask(0,5,4,true);a.move(30,3);a.move(29,4);
    if(!a.equal())large_steps(a);
    else {
        a.at(0x1000c924);a.mask(0,5,8,true);
        if(!a.equal())fine_steps(a);
        else{a.at(0x1000cb88);a.value(4,1000);a.value(3,0);}
    }
    a.at(0x1000cb90);a.load(0,1,0x78);a.offset(1,1,0x70);a.move(11,1);a.call(0x10773858);a.restore_link();
    a.load(31,1,-0x24);a.load(30,1,-0x28);a.load(29,1,-0x2c);a.finish();
}
}
