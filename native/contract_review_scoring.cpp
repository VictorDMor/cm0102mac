#include "contract_review_detail.hpp"

namespace cm::aot::contract_review {
namespace {
using detail::Call;
bool exempt_or_empty(Call& a) {
    a.load(0,28,4);a.load(7,3,0);a.multiply(6,0,0x248);a.load(4,28,0);a.load(5,5,0);a.add(3,7,6);a.load(3,3,0x90);a.multiply(0,4,0x70);a.compare(3,0,false);a.add(31,5,0);
    if(!a.equal()){a.compare_registers(31,3,false);if(a.equal())return true;}
    a.at(0x1037f170);a.add(3,7,6);a.load(0,3,0x94);a.compare(0,0,false);
    if(!a.equal()){a.compare_registers(31,0,false);if(a.equal())return true;}
    return false;
}
void calculate_response_score(Call& a) {
    a.at(0x1037f19c);a.value(3,0x92490000);a.load(7,31,0);a.offset(3,3,0x2493);a.load<1>(9,31,0x67);a.multiply_high(6,3,7);a.load<1>(8,31,0x65);a.load<1>(4,31,0x66);a.sign8(5,9);a.sign8(3,8);a.sign8(4,4);a.add(6,6,7);a.add(3,3,5);a.shift_right(5,6,2);a.compare(4,1);a.mask(6,5,1,false,1);a.add(5,5,6);a.multiply(5,5,7);a.subtract(5,5,7);a.add(3,3,5);a.add(3,4,3);a.sign8(30,3);
    if(a.equal())a.offset(30,30,-2);
    a.at(0x1037f1f8);a.sign8(3,8);a.compare(3,1);if(a.equal()){a.offset(30,30,-3);a.jump(0x1037f218);}else{a.at(0x1037f20c);a.compare(3,6);if(!a.at_least())a.offset(30,30,-1);}
    a.at(0x1037f218);a.sign8(3,9);a.compare(3,1);if(a.equal()){a.offset(30,30,-3);a.jump(0x1037f238);}else{a.at(0x1037f22c);a.compare(3,6);if(!a.at_least())a.offset(30,30,-1);}
    a.at(0x1037f238);a.load<1>(3,31,0x69);a.compare(3,1);if(a.equal())a.offset(30,30,-2);
    a.at(0x1037f248);a.load<1>(4,28,0x4c);a.sign8(3,4);a.compare(3,75);
    if(a.positive()){a.offset(30,30,10);a.jump(0x1037f27c);}else{a.at(0x1037f260);a.compare(3,50);if(a.positive()){a.offset(30,30,5);a.jump(0x1037f27c);}else{a.at(0x1037f270);a.sign8(3,4,true);if(!a.nonpositive())a.offset(30,30,2);}}
    a.at(0x1037f27c);a.load(3,31,0x14);a.compare(3,0,false);if(!a.equal()){a.load<2>(3,3,10,true);a.compare(3,7250);if(a.positive()){a.offset(30,30,-5);a.jump(0x1037f2a8);}else{a.at(0x1037f29c);a.compare(3,5750);if(!a.nonpositive())a.offset(30,30,-3);}}
}
void count_condition_groups(Call& a) {
    a.at(0x1037f2a8);a.mask(3,0,0x20000,true);if(!a.equal())a.value(29,1);
    a.at(0x1037f2b4);a.mask(3,0,0x40000,true);if(!a.equal()){a.offset(3,29,1);a.sign8(29,3);}
    a.at(0x1037f2c4);a.value(3,0x1000000);a.offset(3,3,2);a.intersect_record(3,0,3);if(!a.equal()){a.offset(3,29,1);a.sign8(29,3);}
    a.at(0x1037f2dc);a.value(3,0x200000);a.offset(3,3,0x600);a.intersect_record(3,0,3);if(!a.equal()){a.offset(3,29,1);a.sign8(29,3);}
    a.at(0x1037f2f4);a.mask(3,0,0x2840,true);if(!a.equal()){a.offset(3,29,1);a.sign8(29,3);}
    a.at(0x1037f304);a.mask(0,0,0x14,true);if(!a.equal()){a.offset(0,29,1);a.sign8(29,0);}
    a.at(0x1037f314);a.move(3,28);a.call(status_bits_entry);a.mask(0,3,8,true);
    bool more=!a.equal();if(!more){a.value(3,0x4580000);a.load(4,28,0x50);a.offset(0,3,0x2208);a.intersect_record(0,4,0);more=!a.equal();}
    if(more){a.at(0x1037f338);a.offset(29,29,1);}
}
void apply_group_penalties(Call& a) {
    a.at(0x1037f33c);a.sign8(0,29);a.compare(0,1);if(a.nonpositive())return;
    a.load<1>(0,31,0x67);a.offset(30,30,-3);a.sign8(0,0);a.compare(0,6);if(!a.at_least()){a.subtract(0,29,30);a.sign8(30,0);}
    a.at(0x1037f364);a.load<1>(0,31,0x69);a.sign8(0,0);a.compare(0,6);if(!a.at_least()){a.subtract(0,29,30);a.sign8(30,0);}
    a.at(0x1037f37c);a.load<1>(0,31,0x65);a.sign8(0,0);a.compare(0,1);
    if(a.equal()){a.sign8(0,29);a.mask(0,0,0xfffffffe,false,1);a.subtract(0,0,30);a.sign8(30,0);a.jump(0x1037f3ac);}else{a.at(0x1037f3a0);a.compare(0,6);if(!a.at_least())a.subtract(30,29,30);}
    a.at(0x1037f3ac);a.load<1>(0,31,0x66);a.sign8(0,0);a.compare(0,6);if(!a.at_least())a.subtract(30,29,30);
}
bool crosses_trigger_threshold(Call& a) {
    a.at(0x1037f3c0);a.load<1>(4,31,0x65);a.sign8(0,4);a.compare(0,15);
    if(!a.at_least()){a.load<1>(0,28,0x5c);a.mask(0,0,1,true);if(!a.equal()){a.value(3,1);a.jump(0x1037f568);return true;}}
    a.at(0x1037f3e4);a.value(3,0x1000000);a.load(5,28,0x50);a.offset(0,3,2);a.intersect_record(0,5,0);
    if(!a.equal()){a.sign8(0,4);a.compare(0,10);if(!a.at_least()){a.sign8(0,30);a.compare(0,25);if(!a.at_least()){a.value(3,1);a.jump(0x1037f568);return true;}}}
    a.at(0x1037f418);a.value(3,0x20000);a.offset(0,3,0x20);a.intersect_record(0,5,0);
    if(!a.equal()){a.sign8(0,30);a.compare(0,25);if(!a.at_least()){a.value(3,1);a.jump(0x1037f568);return true;}}
    a.at(0x1037f43c);a.mask(0,5,0x200,true);if(!a.equal()){a.sign8(0,30);a.compare(0,18);if(!a.at_least()){a.value(3,1);a.jump(0x1037f568);return true;}}
    a.at(0x1037f458);a.mask(0,5,4,true);if(!a.equal()){a.load<1>(0,31,0x63);a.sign8(0,0);a.compare(0,15);if(!a.nonpositive()){a.load<1>(0,31,0x67);a.sign8(0,0);a.compare(0,10);if(!a.at_least()){a.value(3,1);a.jump(0x1037f568);return true;}}}
    a.at(0x1037f488);a.mask(0,5,0x2808,true);if(!a.equal()){a.sign8(0,4);a.compare(0,10);if(!a.at_least()){a.load<1>(0,31,0x66);a.sign8(0,0);a.compare(0,6);if(!a.at_least()){a.sign8(0,30);a.compare(0,30);if(!a.at_least()){a.value(3,1);a.jump(0x1037f568);return true;}}}}
    a.at(0x1037f4c0);a.value(3,0x80000);a.offset(0,3,0x2200);a.intersect_record(0,5,0);bool group=!a.equal();
    if(!group){a.move(3,28);a.call(status_bits_entry);a.mask(0,3,8,true);group=!a.equal();}
    if(group){a.at(0x1037f4e0);a.load<1>(0,31,0x67);a.sign8(0,0);a.compare(0,10);if(!a.at_least()){a.sign8(0,30);a.compare(0,20);if(!a.at_least()){a.value(3,1);a.jump(0x1037f568);return true;}}}
    a.at(0x1037f504);a.load(3,28,0x50);a.mask(0,3,1,true);if(!a.equal()){a.value(3,1);a.jump(0x1037f568);return true;}
    a.at(0x1037f518);a.mask(0,3,0x10,true);if(!a.equal()){a.value(3,1);a.jump(0x1037f568);return true;}
    a.at(0x1037f528);a.mask(0,3,0x100,true);if(!a.equal()){a.value(3,1);a.jump(0x1037f568);return true;}
    a.at(0x1037f538);a.compare(3,0);if(a.equal())return false;
    a.load<1>(0,31,0x65);a.load<1>(3,28,0x4c);a.sign8(0,0);a.sign8(3,3);a.subtract_from(0,0,0xffffffba);a.compare_registers(3,0);if(a.at_least())return false;
    a.value(3,1);a.jump(0x1037f568);return true;
}
}
void has_review_trigger(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,has_review_trigger_entry,d,o);a.link();a.save(31);a.load(5,2,-0x7be0);a.save(30);a.save(29);a.value(29,0);a.save(28);a.move(28,3);a.load(3,2,-0x7be8);a.save_link();a.frame(80);
    if(exempt_or_empty(a)){a.at(0x1037f564);a.value(3,0);}
    else {a.at(0x1037f188);a.load(0,28,0x50);a.compare(0,0);if(a.equal()){a.value(3,0);a.jump(0x1037f568);}else{calculate_response_score(a);count_condition_groups(a);apply_group_penalties(a);if(!crosses_trigger_threshold(a)){a.at(0x1037f564);a.value(3,0);}}}
    a.at(0x1037f568);a.pop(80);a.restore(31);a.restore(30);a.restore(29);a.restore(28);a.finish();
}
}
