#include "contract_review_detail.hpp"

namespace cm::aot::contract_review {
namespace {
using detail::Call;
bool emit_squad_case(Call& a,std::uint32_t target,std::uint32_t caller) {
    constexpr auto first=0x10380cb4u,last=0x10380d24u;
    if(target>=first&&target<last&&!(target&3)) {
        constexpr std::array offsets{0x419,0x423,0x427,0x432,0x43b,0x442,0x44b};
        const auto which=(target-first)/16,part=(target-first)%16;a.at(target);
        if(part==0)a.move(3,4);if(part<=4)a.offset(4,31,offsets[which]);if(part<=8)a.call(0x10727890);a.jump(0x10380d30);return true;
    }
    if(target>=last&&target<0x10380d30&&!(target&3)) {
        a.at(target);if(target==last)a.move(3,4);if(target<=last+4)a.offset(4,31,0x455);if(target<=last+8)a.call(0x10727890);return true;
    }
    if(target==0x10380d30)return true;
    a.resume(target,caller);return false;
}
void emit_release_label(Call& a,std::uint32_t pc,int offset) {
    a.at(pc);a.move(3,4);a.offset(4,31,offset);a.offset(5,31,0xf2);a.call(0x10727890);
}
void describe_release_fields(Call& a) {
    a.at(0x10380dd8);a.load<1>(3,29,0x1d);a.sign8(0,3,true);
    if(!a.equal()) {
        a.sign8(0,3);a.compare(0,1);if(!a.equal()){a.value(0,1);a.store<1>(29,0x1d,0);}
        emit_release_label(a,0x10380df8,0x460);a.load<1>(3,29,0x1d);a.jump(0x10380f00);return;
    }
    a.at(0x10380e10);a.load<1>(3,29,0x1c);a.sign8(0,3,true);
    bool both=!a.equal();if(both){a.load<1>(5,29,0x1f);a.sign8(0,5,true);both=!a.equal();}
    if(both){a.sign8(0,3);a.compare(0,2);if(a.equal()){emit_release_label(a,0x10380e34,0x4a4);a.value(3,2);a.jump(0x10380f00);return;}
        a.at(0x10380e4c);a.sign8(0,5);a.compare(0,2);if(a.equal()){emit_release_label(a,0x10380e58,0x4eb);a.value(3,2);a.jump(0x10380f00);return;}
        emit_release_label(a,0x10380e70,0x52b);a.value(3,1);a.jump(0x10380f00);return;
    }
    a.at(0x10380e88);a.sign8(0,3,true);if(!a.equal()){emit_release_label(a,0x10380e90,0x4a4);a.load<1>(3,29,0x1c);a.jump(0x10380f00);return;}
    a.at(0x10380ea8);a.load<1>(0,29,0x1f);a.sign8(0,0,true);if(!a.equal()){emit_release_label(a,0x10380eb4,0x4eb);a.load<1>(3,29,0x1f);a.jump(0x10380f00);return;}
    a.at(0x10380ecc);a.load<1>(0,29,0x20);a.sign8(0,0,true);if(!a.equal()){emit_release_label(a,0x10380ed8,0x582);a.load<1>(3,29,0x20);a.jump(0x10380f00);return;}
    a.at(0x10380ef0);a.move(3,4);a.offset(4,31,0x5c3);a.call(0x1077b720);a.value(3,0);
}
bool reference_after_start(Call& a) {
    a.at(0x10380f70);a.load(4,2,-0x79e8);a.load<2>(5,28,0x2a);a.load(3,4,0);a.load(0,4,4);a.store(1,0x38,3);a.load<2>(3,1,0x3a);a.store(1,0x3c,0);a.compare_registers(3,5,false);
    if(a.positive()){a.value(0,1);a.jump(0x10380fc8);}else{a.at(0x10380f9c);if(!a.at_least()){a.value(0,0);a.jump(0x10380fc8);}else{
        a.at(0x10380fa8);a.load<2>(4,1,0x38,true);a.load<2>(0,28,0x28,true);a.combine(0,4,0,false,true);a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,1,false,1);a.sign8(0,0);
    }}
    a.at(0x10380fc8);a.sign8(0,0,true);return !a.equal();
}
void calculate_window_threshold(Call& a) {
    a.at(0x10380fd8);a.load(5,28,0x28);a.move(4,31);a.load(6,28,0x2c);a.offset(3,1,0x48);a.call(0x10717dd0);a.load<1>(0,1,0x4c);a.offset(4,28,0x28);a.load<2>(3,1,0x4a);a.load<2>(5,1,0x48,true);a.store<2>(1,0x52,3);a.load(3,2,-0x7be0);a.store<2>(1,0x50,5);a.load(3,3,0);a.store<1>(1,0x54,0);a.load(0,28,0);a.multiply(0,0,0x70);a.add(3,3,0);a.call(0x100c6a10);a.sign8(4,3);a.value(0,28);a.exclusive_flags(3,0,0x80000000);a.compare(30,0,false);a.offset(0,4,-28);a.add_register_carry(0,0,3);a.subtract_extended(3,0,0);a.offset(27,3,3);
    if(!a.equal())a.store<1>(30,0,27);
}
}
void describe_squad_status(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,describe_squad_status_entry,d,o);a.link();const auto caller=a.r(0);a.save(31);a.compare(5,20);a.load(31,2,-0x4da4);a.save(30);a.load(30,2,-0x7ba8);a.save_link();a.frame(112);
    if(!a.at_least()){detail::report_diagnostic(a,0x10380c34,31,30,0x20ec);a.load(4,2,-0x7bac);a.value(0,0);a.value(3,0);a.store<1>(4,0,0);a.jump(0x10380d34);}
    else {
        a.at(0x10380c90);a.load<1>(0,3,0x60);a.mask(0,0,15);a.compare(0,7,false);
        if(a.positive()){a.at(0x10380d24);a.move(3,4);a.offset(4,31,0x455);a.call(0x10727890);}
        else {a.load(3,2,-0x4dc0);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(3,3,0);a.count_destination(3);const auto target=a.count_branch();if(!emit_squad_case(a,target,caller))return;}
        a.at(0x10380d30);a.value(3,1);
    }
    a.at(0x10380d34);a.pop(112);a.restore(31);a.restore(30);a.finish();
}
void describe_release_summary(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,describe_release_summary_entry,d,o);a.link();a.save(31);a.compare(5,20);a.load(31,2,-0x4da4);a.save(30);a.load(30,2,-0x7ba8);a.save(29);a.move(29,3);a.save_link();a.frame(112);
    if(!a.at_least()){detail::report_diagnostic(a,0x10380d7c,31,30,0x2125);a.load(4,2,-0x7bac);a.value(0,0);a.value(3,0);a.store<1>(4,0,0);a.jump(0x10380f00);}else describe_release_fields(a);
    a.at(0x10380f00);a.pop(112);a.restore(31);a.restore(30);a.restore(29);a.finish();
}
void query_start_date_window(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,query_start_date_window_entry,d,o);a.link();a.multiple(27);a.load(31,2,-0x7b80);a.move(28,3);a.move(29,5);a.move(30,6);a.save_link();a.frame(112);a.offset(3,1,0x50);a.call(0x10715880);a.compare(29,0,false);if(!a.equal()){a.value(0,0);a.store<1>(29,0,0);}
    a.at(0x10380f58);a.load(3,2,-0x7a68);a.load<1>(0,3,0);a.compare(0,1);if(!a.equal()){a.value(3,1);a.jump(0x103810bc);}
    else if(reference_after_start(a)){a.value(3,1);a.jump(0x103810bc);}
    else{
        calculate_window_threshold(a);a.at(0x10381048);a.load(5,28,0x28);a.move(4,31);a.load(6,28,0x2c);a.offset(3,1,0x40);a.call(0x10717dd0);a.load<2>(3,1,0x42);a.sign8(5,27);a.load<1>(0,1,0x44);a.store<2>(1,0x52,3);a.load<2>(3,1,0x40,true);a.load<2>(4,1,0x52);a.store<2>(1,0x50,3);a.compare_registers(4,5);a.store<1>(1,0x54,0);
        if(!a.at_least()){a.value(3,1);a.jump(0x103810bc);}else{
            a.at(0x1038108c);a.compare(29,0,false);if(!a.equal()){a.compare_registers(5,4);if(a.equal()){a.load<2>(0,1,0x50,true);a.compare(0,0);if(a.equal()){a.compare(29,0,false);if(!a.equal()){a.value(0,1);a.store<1>(29,0,0);}}}}
            a.at(0x103810b8);a.value(3,0);
        }
    }
    a.at(0x103810bc);a.pop(112);a.multiple(27,true);a.finish();
}
}
