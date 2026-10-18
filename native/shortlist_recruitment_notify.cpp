#include "shortlist_recruitment.hpp"
#include "shortlist_recruitment_detail.hpp"

namespace cm::aot::shortlist_recruitment {
using detail::Call;
namespace {
bool staff_can_be_distributed(Call& a) {
    a.load(4,4,0x14);a.compare(4,0,false);if(a.equal())return false;
    a.sign8(0,5,true);
    if(a.equal()){a.load<2>(0,4,10,true);a.compare(0,5500);if(a.less())return false;}
    a.at(0x10363cec);a.load(0,26,0);a.load(4,27,0);a.multiply(3,0,100);a.offset(0,3,12);a.indexed_load<1>(0,4,0);a.mask(0,0,3,false,30);a.compare(0,1,false);if(a.equal())return false;
    a.load<1>(0,26,0x62);a.compare(0,11);if(!a.equal())return false;
    a.load(0,26,0x20);a.compare(0,0,false);
    if(!a.equal()){a.load<1>(0,26,0x60);a.compare(0,5);if(!a.equal())a.at(0x10363d34);else{a.jump(0x10363f48);return false;}}
    a.at(0x10363d34);a.load(4,26,12);a.compare(4,0,false);
    if(!a.equal()){a.load(3,2,-0x7588);a.load(4,4,0x68);a.load(0,3,0);a.compare_registers(4,0);if(a.equal())return false;}
    a.at(0x10363d54);a.load(3,2,-0x7b40);a.move(4,26);a.value(5,0);a.load(3,3,0);a.call(0x103f43b0);a.sign8(0,3,true);return a.equal();
}
bool contract_allows_distribution(Call& a) {
    a.at(0x10363d70);a.load(0,26,0x24);a.compare(0,0,false);if(a.equal())return true;
    a.load(4,2,-0x7b80);a.offset(3,1,0x40);a.value(5,200);a.call(0x10717b20);a.load(3,1,0x40);a.load(0,1,0x44);a.store(1,0x38,3);a.load<2>(3,26,0x4e);a.load<2>(4,1,0x3a);a.store(1,0x3c,0);a.compare_registers(4,3,false);
    if(!a.at_least()){a.value(0,1);a.jump(0x10363de0);}
    else{a.at(0x10363db4);if(!a.nonpositive()){a.value(0,0);a.jump(0x10363de0);}else{a.at(0x10363dc0);a.load<2>(0,1,0x38,true);a.load<2>(4,26,0x4c,true);a.combine(0,4,0,false,true);a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,1,false,1);a.sign8(0,0);}}
    a.at(0x10363de0);a.sign8(0,0,true);if(a.equal())return true;
    a.load(3,28,0);a.move(4,26);a.call(0x10082f20);a.compare(3,0,false);if(a.equal())return true;
    a.load<1>(0,3,0x60);a.mask(0,0,15);a.compare(0,7);if(a.equal())return true;
    a.load(0,3,0x50);a.compare(0,0);return !a.equal();
}
void distribute_to_ordered_clubs(Call& a) {
    a.at(0x10363e18);a.load(3,28,0);a.move(4,26);a.call(0x10082f20);a.compare(3,0,false);a.store(29,0,3);
    if(!a.equal()){a.load(0,3,4);a.load(3,30,0);a.multiply(0,0,0x248);a.add(28,3,0);a.jump(0x10363e48);}else{a.at(0x10363e44);a.value(28,0);}
    a.at(0x10363e48);a.load(0,26,0);a.load(5,27,0);a.multiply(4,0,100);a.load(3,26,0x14);a.offset(0,4,12);a.indexed_load<1>(0,5,0);a.mask(0,0,15,false,28);a.sign8(29,0);a.call(0x10117490);detail::expand_role_qualifications(a,0x10363e6c,27);
    a.load(3,2,-0x765c);a.value(23,0);a.load(24,3,0);a.jump(0x10363f3c);
    for(;;){
        a.at(0x10363f3c);a.load(0,31,0);a.compare_registers(23,0);if(!a.less())break;
        a.at(0x10363ed4);a.load(3,24,0);a.sign8(0,29);a.compare(0,5);a.load(4,30,0);a.multiply(0,3,0x248);a.add(22,4,0);if(a.at_least())break;
        a.move(3,22);a.call(0x100c7d00);a.sign8(0,3,true);bool consider=a.equal();
        if(consider){a.compare_registers(28,22,false);consider=!a.equal();}
        if(consider){a.load(0,26,0x24);a.compare_registers(0,22,false);consider=!a.equal();}
        if(consider){a.move(3,25);a.move(4,26);a.move(5,22);a.move(6,27);a.call(register_club_interest_entry);a.sign8(0,3,true);if(!a.equal())a.offset(29,29,1);}
        a.at(0x10363f34);a.offset(23,23,1);a.offset(24,24,4);
    }
}
}
void distribute_staff_interest(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,distribute_staff_interest_entry,d,o);a.link();a.multiple(22);a.move(26,4);a.load(27,2,-0x7b9c);a.load(28,2,-0x7b7c);a.move(25,3);a.load(29,2,-0x7b78);a.load(30,2,-0x7be8);a.load(31,2,-0x7b50);a.save_link();a.frame(0x70);
    if(staff_can_be_distributed(a)&&contract_allows_distribution(a))distribute_to_ordered_clubs(a);
    a.at(0x10363f48);a.pop(0x70);a.multiple(22,true);a.finish();
}
}
