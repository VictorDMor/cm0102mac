#include "match_event.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::match_event {
using Call = SemanticGuestCall;
namespace {
void classification_rule_1(Call&);
void classification_rule_2(Call&);
void classification_rule_3(Call&);
void classification_rule_4(Call&);
void classification_rule_5(Call&);
void classification_rule_5(Call& a) {
    a.at(0x10201edc); a.value(3,0x0); a.finish();
}
void classification_rule_4(Call& a) {
    a.at(0x10201ed4); a.value(3,0xffffffff); a.finish();
}
void classification_rule_3(Call& a) {
    a.at(0x10201ecc); a.value(3,0xfffffffe); a.finish();
}
void classification_rule_2(Call& a) {
    a.at(0x10201e68); a.sign16(0,4); a.compare(0,8496);
    if (a.at_least()) {
        a.at(0x10201ea0); a.compare(0,8510);
        if (a.equal()) {
            a.at(0x10201ecc);
            classification_rule_3(a);
        } else {
            a.at(0x10201ea8);
            if (a.at_least()) {
                a.at(0x10201eb8); a.compare(0,8519);
                if (a.equal()) {
                    a.at(0x10201ecc);
                    classification_rule_3(a);
                } else {
                    a.at(0x10201ec0); a.jump(0x10201edc);
                    classification_rule_5(a);
                }
            } else {
                a.at(0x10201eac); a.compare(0,8498);
                if (a.at_least()) {
                    a.at(0x10201edc);
                    classification_rule_5(a);
                } else {
                    a.at(0x10201eb4); a.jump(0x10201ed4);
                    classification_rule_4(a);
                }
            }
        }
    } else {
        a.at(0x10201e74); a.compare(0,8053);
        if (a.equal()) {
            a.at(0x10201ed4);
            classification_rule_4(a);
        } else {
            a.at(0x10201e7c);
            if (a.at_least()) {
                a.at(0x10201e94); a.compare(0,8057);
                if (a.equal()) {
                    a.at(0x10201ed4);
                    classification_rule_4(a);
                } else {
                    a.at(0x10201e9c); a.jump(0x10201edc);
                    classification_rule_5(a);
                }
            } else {
                a.at(0x10201e80); a.compare(0,8008);
                if (a.at_least()) {
                    a.at(0x10201edc);
                    classification_rule_5(a);
                } else {
                    a.at(0x10201e88); a.compare(0,8006);
                    if (a.at_least()) {
                        a.at(0x10201ec4); a.value(3,0x1); a.finish();
                    } else {
                        a.at(0x10201e90); a.jump(0x10201edc);
                        classification_rule_5(a);
                    }
                }
            }
        }
    }
}
void classification_rule_1(Call& a) {
    a.at(0x10201e4c); a.sign16(0,4); a.compare(0,8225);
    if (a.nonpositive()) {
        a.at(0x10201e68);
        classification_rule_2(a);
    } else {
        a.at(0x10201e58); a.compare(0,8400);
        if (a.at_least()) {
            a.at(0x10201e68);
            classification_rule_2(a);
        } else {
            a.at(0x10201e60); a.value(3,0x1); a.finish();
        }
    }
}
}
void classify_event(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x10201e30,dependencies,observer);
    a.at(0x10201e30); a.sign16(0,4); a.compare(0,8070);
    if (a.nonpositive()) {
        a.at(0x10201e4c);
        classification_rule_1(a);
    } else {
        a.at(0x10201e3c); a.compare(0,8126);
        if (a.at_least()) {
            a.at(0x10201e4c);
            classification_rule_1(a);
        } else {
            a.at(0x10201e44); a.value(3,0x0); a.finish();
        }
    }
}
}
