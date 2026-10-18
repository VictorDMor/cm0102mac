#include "shortlist_policy_detail.hpp"

namespace cm::aot::shortlist_policy::detail {
void prepare_country_policy(Call& a) {
    a.at(0x1036f590);a.link();a.store<4>(1,-0x4,31);a.move(31,6);a.store<4>(1,-0x8,30);a.move(30,4);a.store<4>(1,0x8,0);a.frame(0x40);
    a.load<4>(3,4,0xc,false);a.compare(3,0x0,false);
    if(!a.equal()) {
        a.at(0x1036f5c0);a.load<4>(4,31,0x2c,false);a.load<1>(0,4,0x118,false);a.compare(0,0xe,true);
        if(a.equal()) {
            a.at(0x1036f5d8);a.compare_registers(3,4,false);
            if(a.equal()) {
                a.at(0x1036f5ec);a.value(3,0x1);a.jump(0x1036f64c);
            } else {
                a.at(0x1036f5e0);a.load<4>(0,30,0x10,false);a.compare_registers(0,4,false);
                if(!a.equal()) {
                    a.at(0x1036f5f4);a.move(3,30);a.call(0x100ca6a0);a.sign8(0,3,true);
                    if(a.equal()) {
                        a.at(0x1036f60c);a.load<4>(0,30,0x20,false);a.compare(0,0x0,false);
                        if(!a.equal()) {
                            a.at(0x1036f624);a.load<4>(3,2,-0x7b40,false);a.move(4,30);a.load<4>(5,31,0x2c,false);a.value(6,0x0);a.load<4>(3,3,0x0,false);
                            a.value(7,0x0);a.value(8,0x0);a.call(0x103ed520);a.jump(0x1036f64c);
                        } else {
                            a.at(0x1036f618);a.load<1>(0,30,0x5e,false);a.compare(0,0x0,false);
                            if(a.equal()) {
                                a.at(0x1036f648);a.value(3,0x0);
                            } else {
                                a.at(0x1036f624);a.load<4>(3,2,-0x7b40,false);a.move(4,30);a.load<4>(5,31,0x2c,false);a.value(6,0x0);
                                a.load<4>(3,3,0x0,false);a.value(7,0x0);a.value(8,0x0);a.call(0x103ed520);a.jump(0x1036f64c);
                            }
                        }
                    } else {
                        a.at(0x1036f604);a.value(3,0x1);a.jump(0x1036f64c);
                    }
                } else {
                    a.at(0x1036f5ec);a.value(3,0x1);a.jump(0x1036f64c);
                }
            }
        } else {
            a.at(0x1036f5d0);a.value(3,0x1);a.jump(0x1036f64c);
        }
    } else {
        a.at(0x1036f5b8);a.value(3,0x0);a.jump(0x1036f64c);
    }
}
}
namespace cm::aot::shortlist_policy {
void check_candidate_country_policy(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,check_candidate_country_policy_entry,d,o);detail::prepare_country_policy(a);
    a.at(0x1036f64c);
    a.load<4>(0,1,0x48,false);
    a.offset(1,1,0x40);
    a.restore_link();
    a.load<4>(31,1,-0x4,false);
    a.load<4>(30,1,-0x8,false);
    a.finish();
}
}
