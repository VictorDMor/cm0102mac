#include "injury_daily.hpp"
#include "injury_daily_detail.hpp"

namespace cm::aot::injury_daily {
using namespace detail;
namespace {
void decrement_injury_days(Call& a) {
    a.at(0x10185448);a.load(3,3,0x28);a.compare(3,0,false);if(!a.equal()){a.value(4,1);a.call(0x10738c20);a.value(0,0);a.store(15,0x28,0);}
    a.at(0x10185464);a.load(5,15,4);a.value(4,0);a.jump(0x10185490);
    while(true){a.at(0x10185490);a.load(0,15,8);a.compare_registers(4,0);if(!a.less())break;a.at(0x10185470);a.load<2>(3,5,0x34,true);a.offset(4,4,1);a.offset(0,3,-1);a.store<2>(5,0x34,0);a.load<2>(3,5,0x36,true);a.offset(0,3,-1);a.store<2>(5,0x36,0);a.offset(5,5,0x38);}
}
void refresh_season_boundary(Call& a) {
    a.at(0x1018549c);a.load(4,2,-0x7a9c);a.load<2>(5,14,2);a.load(3,4,0);a.load(0,4,4);a.store(1,0x40,3);a.load<2>(3,1,0x42);a.store(1,0x44,0);a.compare_registers(3,5,false);
    if(!a.nonpositive()){a.value(0,1);a.jump(0x101854f4);}
    else {a.at(0x101854c8);if(!a.at_least()){a.value(0,0);a.jump(0x101854f4);}else {a.at(0x101854d4);a.load<2>(4,1,0x40,true);a.load<2>(0,14,0,true);a.combine(0,4,0,false,true);a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,1,false,1);a.sign8(0,0);}}
    a.at(0x101854f4);a.sign8(0,0,true);if(a.equal())return;a.load<2>(0,14,0,true);a.compare(0,180);if(!a.equal())return;
    a.load(17,30,0);a.value(16,0);a.jump(0x10185528);
    while(true){a.at(0x10185528);a.load(0,13,0);a.compare_registers(16,0);if(!a.less())break;a.at(0x10185514);a.move(3,15);a.move(4,17);a.call(0x10187a40);a.offset(16,16,1);a.offset(17,17,0x70);}
}
bool prepare_daily_sample(Call& a) {
    a.at(0x10185534);a.move(3,14);a.call(0x10715b20);a.sign16(0,3);a.compare(0,6);if(a.equal())return false;
    a.move(3,14);a.call(0x10715b20);a.sign16(0,3);a.compare(0,5);if(a.equal())return false;
    a.value(3,0x1b4f0000);a.load(0,13,0);a.offset(3,3,-0x7e4b);a.multiply_high(0,3,0);a.shift_right(0,0,4);a.mask(3,0,1,false,1);a.add(0,0,3);a.sign16(0,0);a.compare(0,10);if(!a.at_least())a.value(0,10);
    a.at(0x10185588);a.value(3,0x92490000);a.sign16(20,0);a.offset(0,3,0x2493);a.value(19,0);a.multiply_high(0,0,20);a.add(0,0,20);a.shift_right(0,0,3);a.mask(3,0,1,false,1);a.add(0,0,3);a.sign16(14,0);a.jump(0x10186474);return true;
}
}
void process_daily_injuries(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,entry,d,o);a.link();const auto caller=a.r(0);a.multiple(13);a.move(15,3);a.load(24,2,-0x64d0);a.load(25,2,-0x543c);a.load(26,2,-0x7b90);a.load(27,2,-0x7b8c);a.load(28,2,-0x7b88);a.load(29,2,-0x7b84);a.load(30,2,-0x7be0);a.load(13,2,-0x7be4);a.load(14,2,-0x7b80);a.save_link();a.frame(176);
    decrement_injury_days(a);refresh_season_boundary(a);
    if(prepare_daily_sample(a))while(true) {
        a.at(0x10186474);a.compare_registers(19,20);if(!a.less())break;
        a.at(0x101855b4);a.load(16,13,0);a.compare(16,0);if(a.equal()){a.value(17,0);a.jump(0x10185710);}else bounded_draw<16,true>(a,0x101855c8);
        if(eligible_staff(a)) {
            choose_injury_type(a);const auto target=classify_candidate(a);
            if(target!=0x10186470) {
                if(!apply_attribute_effects(a,target,caller))return;
                a.at(0x10186440);a.value(0,0);a.move(3,15);a.store(1,0x38,0);a.move(4,17);a.value(5,0);a.value(7,0);a.load(0,30,0);a.value(8,1);a.value(9,0xffffffff);a.value(10,0);a.add(6,0,21);a.call(0x10188cc0);
            }
        }
        a.at(0x10186470);a.offset(19,19,1);
    }
    a.at(0x1018647c);a.move(3,15);a.call(0x1018b2b0);a.move(3,15);a.call(0x1018bdd0);a.pop(176);a.multiple(13,true);a.finish();
}
}
