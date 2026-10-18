#include "profile_qualification_detail.hpp"

namespace cm::aot::profile_qualification::detail {
void category_four_reference(Call& a) {
    a.at(0x1036f728);a.load(5,4,0x2c);a.compare(5,0,false);
    bool first_group=false;
    if(!a.equal()) {
        a.load(3,2,-0x7a14);a.load(6,5,0x68);a.load(0,3,0);a.compare_registers(6,0);
        if(a.equal())first_group=true;
        else {
            a.load(3,2,-0x7a10);a.load(0,3,0);a.compare_registers(6,0);
            if(a.equal())first_group=true;
            else {a.load(3,2,-0x7a18);a.load(0,3,0);a.compare_registers(6,0);first_group=a.equal();}
        }
    }
    if(first_group){a.at(0x1036f768);a.load<2>(0,4,0x1ec,true);a.multiply(3,0,550);a.finish();return;}
    a.at(0x1036f774);a.compare(5,0,false);
    bool second_group=false;
    if(!a.equal()) {
        a.load(3,2,-0x7af4);a.load(5,5,0x68);a.load(0,3,0);a.compare_registers(5,0);
        if(a.equal())second_group=true;
        else {a.load(3,2,-0x7a00);a.load(0,3,0);a.compare_registers(5,0);second_group=a.equal();}
    }
    if(second_group){a.at(0x1036f7a0);a.load<2>(0,4,0x1ec,true);a.multiply(3,0,475);a.finish();}
    else {a.at(0x1036f7ac);a.load<2>(0,4,0x1ec,true);a.multiply(3,0,330);a.finish();}
}
}
namespace cm::aot::profile_qualification {
void club_category_reference(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,club_category_reference_entry,d,o);a.sign8(0,6);a.compare(0,5);
    if(a.equal()){a.at(0x1036f7b8);a.load<2>(0,4,0x1ec,true);a.multiply(3,0,550);a.finish();return;}
    if(a.at_least()) {
        a.at(0x1036f714);a.compare(0,7);
        if(!a.at_least()){a.jump(0x1036f7c4);a.load<2>(0,4,0x1ec,true);a.multiply(3,0,110);a.finish();return;}
    } else {
        a.compare(0,4);
        if(a.at_least()){detail::category_four_reference(a);return;}
        a.compare(0,1);if(a.at_least()){a.at(0x1036f720);a.value(3,0xffffffff);a.finish();return;}
        a.jump(0x1036f7d0);
    }
    a.at(0x1036f7d0);a.value(3,0xffffffff);a.finish();
}
void candidate_category_reference(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,candidate_category_reference_entry,d,o);
    a.link();a.store(1,-4,31);a.compare(5,0,false);a.move(31,6);a.store(1,-8,30);a.move(30,4);a.store(1,-12,29);a.move(29,3);a.store(1,8,0);a.frame(0x50);
    bool has_profile=!a.equal();
    if(has_profile){a.load(3,5,0x14);a.compare(3,0,false);has_profile=!a.equal();}
    if(!has_profile){a.at(0x1036f6a8);a.value(3,0xffffffff);a.jump(0x1036f6cc);}
    else {a.at(0x1036f6b0);a.call(classify_profile_qualifications_entry);a.move(0,3);a.move(3,29);a.move(5,0);a.move(4,30);a.move(6,31);a.call(club_category_reference_entry);}
    a.at(0x1036f6cc);a.pop(0x50);a.load(31,1,-4);a.load(30,1,-8);a.load(29,1,-12);a.finish();
}
}
