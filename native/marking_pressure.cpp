#include "marking_pressure.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::marking_pressure {
namespace {
using Call=SemanticGuestCall;
constexpr std::uint32_t address(Entry entry){return entries[unsigned(entry)];}
void append_marker(Call& a){
    a.load<1>(0,3,0x104);a.sign8(0,0);a.compare(0,3);
    if(!a.at_least()){
        a.load<1>(5,3,0x104);a.offset(0,5,1);a.store<1>(3,0x104,0);a.sign8(0,5);a.mask(5,0,0xfffffffc,false,2);
        a.offset(0,5,0x1b4);a.indexed_store(4,3,0);
    }
    a.at(0x101d572c);a.finish();
}
void set_pressure_adjustment(Call& a){
    a.store<2>(3,0x1a0,4);a.load<2>(0,3,0x1a0,true);a.compare(0,200);
    if(!a.nonpositive()){a.value(0,200);a.store<2>(3,0x1a0,0);a.jump(0x101d5800);}
    else{
        a.at(0x101d57ec);a.load<2>(0,3,0x1a0,true);a.compare(0,-200);
        if(!a.at_least()){a.value(0,0xffffff38);a.store<2>(3,0x1a0,0);}
    }
    a.at(0x101d5800);a.finish();
}
void contains_marker(Call& a){
    a.value(6,0);a.jump(0x1028356c);
    while(true){
        a.at(0x1028356c);a.load<1>(0,3,0x104);a.sign8(5,6);a.sign8(0,0);a.compare_registers(5,0);if(!a.less())break;
        a.at(0x10283548);a.sign8(0,6);a.mask(0,0,0xfffffffc,false,2);a.add(5,3,0);a.load(0,5,0x1b4);a.compare_registers(0,4,false);
        if(a.equal()){a.value(3,1);a.finish();return;}
        a.at(0x10283568);a.offset(6,6,1);
    }
    a.at(0x10283580);a.value(3,0);a.finish();
}
void remove_marker(Call& a){
    a.value(6,0xffffffff);a.value(7,0);a.jump(0x102835c0);
    while(true){
        a.at(0x102835c0);a.load<1>(0,3,0x104);a.sign8(5,7);a.sign8(0,0);a.compare_registers(5,0);if(!a.less())break;
        a.at(0x1028359c);a.sign8(0,7);a.mask(0,0,0xfffffffc,false,2);a.add(5,3,0);a.load(0,5,0x1b4);a.compare_registers(0,4,false);
        if(a.equal()){a.move(6,7);a.jump(0x102835d4);break;}
        a.at(0x102835bc);a.offset(7,7,1);
    }
    a.at(0x102835d4);a.sign8(0,6);a.compare(0,-1);if(a.finish_if_equal())return;
    a.load<1>(4,3,0x104);a.sign8(0,6);a.mask(0,0,0xfffffffc,false,2);a.sign8(4,4);a.mask(5,4,0xfffffffc,false,2);
    a.add(4,3,0);a.add(5,3,5);a.load(0,5,0x1b0);a.store(4,0x1b4,0);a.load<1>(4,3,0x104);a.offset(0,4,-1);a.store<1>(3,0x104,0);a.finish();
}
void tactical_marking_trait(Call& a){
    a.link();a.save(31);a.move(31,3);a.save_link();a.frame(0x40);a.load<1>(0,3,0x27);a.load(5,3,0x1a4);
    a.sign8(0,0);a.load<1>(4,3,0x19);a.multiply(3,0,0x18e0);a.offset(3,3,0x7c6c);a.add(3,5,3);a.call(0x101149f0);a.mask(0,3,2,true);
    if(!a.equal()){a.load<1>(3,31,0x10d);a.jump(0x10237408);}
    else{a.at(0x10237404);a.load<1>(3,31,0x10b);}
    a.at(0x10237408);a.pop(0x40);a.restore(31);a.finish();
}
// Contributions use the mutable original double table. Integer conversion,
// instruction checkpoints and the signed low-halfword running sum remain
// observable; ranking doubles only the already-truncated marked contribution.
void marked_contribution(Call& a){
    a.offset(27,27,2);a.move(3,30);a.call(address(Entry::tactical_marking_trait));a.sign8(17,3);
    a.move(3,30);a.call(address(Entry::trait_10d));a.sign8(0,3);a.add(17,0,17);a.move(3,28);a.call(address(Entry::trait_10a));a.sign8(0,3);a.subtract(17,0,17);
    a.move(3,28);a.call(address(Entry::pressure_adjustment));a.sign16(0,3);a.value(3,0x66660000);a.offset(3,3,0x6667);a.multiply_high(0,3,0);
    a.shift_right(0,0,3);a.mask(3,0,1,false,1);a.add(0,0,3);a.subtract(0,0,17);a.floating_load(1,31,0x60);a.exclusive_flags(0,0,0x80000000);
    a.store(1,0x4c,0);a.value(0,0x43300000);a.store(1,0x48,0);a.floating_load(0,1,0x48);a.floating_subtract(1,0,1);
    a.floating_load(0,31,0xb8);a.floating_product(0,0,1);a.floating_integer(0,0);a.floating_store(0,1,0x40);a.load(29,1,0x44);
    a.load<2>(0,25,0,true);a.add(0,0,29);a.store<2>(25,0,0);a.mask(0,29,0xfffffffe,false,1);a.sign16(29,0);a.jump(0x101d803c);
}
void ordinary_contribution(Call& a){
    a.at(0x101d7f3c);a.load<1>(0,30,0x2c);a.sign8(0,0);a.compare(0,0);if(a.less())return;
    a.move(3,30);a.call(0x10289560);a.sign16(0,3);a.compare(0,1);if(a.equal())return;
    a.move(3,30);a.call(address(Entry::trait_10d));a.sign8(0,3);a.compare(0,0);bool eligible=!a.equal();
    if(!eligible){a.move(3,30);a.call(address(Entry::action_code));a.sign16(0,3);a.mask(0,0,0x100);a.compare(0,0);eligible=!a.equal();}
    if(!eligible)return;
    a.at(0x101d7f8c);a.offset(27,27,1);a.move(3,30);a.call(address(Entry::trait_10d));a.sign8(17,3);a.move(3,28);a.call(address(Entry::trait_10a));a.sign8(0,3);a.subtract(17,0,17);
    a.move(3,28);a.call(address(Entry::pressure_adjustment));a.sign16(0,3);a.value(3,0x66660000);a.offset(3,3,0x6667);a.multiply_high(0,3,0);
    a.shift_right(0,0,3);a.mask(3,0,1,false,1);a.add(0,0,3);a.subtract(0,0,17);a.floating_load(1,31,0x60);a.exclusive_flags(0,0,0x80000000);
    a.store(1,0x44,0);a.value(0,0x43300000);a.store(1,0x40,0);a.floating_load(0,1,0x40);a.floating_subtract(1,0,1);a.floating_load(0,31,0xb8);a.floating_product(2,0,1);
    a.load<1>(0,30,0x2c);a.sign8(3,0);a.offset(0,3,1);a.floating_load(1,31,0x60);a.exclusive_flags(0,0,0x80000000);
    a.store(1,0x4c,0);a.value(0,0x43300000);a.store(1,0x48,0);a.floating_load(0,1,0x48);a.floating_subtract(0,0,1);a.floating_difference(0,2,0,true);
    a.floating_integer(0,0);a.floating_store(0,1,0x38);a.load(29,1,0x3c);a.load<2>(0,25,0,true);a.add(0,0,29);a.store<2>(25,0,0);
}
void rank_candidate(Call& a){
    a.at(0x101d803c);a.compare(18,0,false);if(a.equal())return;
    a.sign16(0,29);a.compare(0,5);if(a.nonpositive())return;
    a.sign16(0,29);a.sign16(3,20);a.compare_registers(0,3);
    if(!a.nonpositive()){
        a.compare(26,0,false);if(!a.equal()){a.move(24,26);a.move(19,20);}
        a.at(0x101d8070);a.move(26,30);a.move(20,29);a.jump(0x101d8094);
    }else{
        a.at(0x101d807c);a.sign16(0,29);a.sign16(3,19);a.compare_registers(0,3);
        if(!a.nonpositive()){a.move(24,30);a.move(19,29);}
    }
}
void assess_cell(Call& a){
    a.link();a.multiple(17);a.save_link();a.frame(0x90);a.store(1,0xa8,3);a.move(28,4);a.move(25,5);a.move(18,6);a.load(31,2,-0x52c4);
    a.value(27,0);a.value(20,0xfffffc18);a.value(19,0xfffffc18);a.value(26,0);a.value(24,0);a.value(0,0);a.store<2>(25,0,0);
    a.move(3,28);a.call(0x101d5680);a.sign8(0,3);a.multiply(17,0,0x5c);a.move(3,28);a.call(0x101d5690);a.sign8(0,3);
    a.multiply(3,0,0x450);a.load(0,1,0xa8);a.add(3,3,17);a.offset(23,3,0xa08);a.add(23,0,23);a.move(3,28);a.call(address(Entry::team));a.sign8(0,3);a.compare(0,0);
    if(a.equal()){a.value(21,1);a.jump(0x101d7e50);}else{a.at(0x101d7e4c);a.value(21,0);}
    a.at(0x101d7e50);a.value(22,0);a.jump(0x101d8098);
    while(true){
        a.at(0x101d8098);a.sign8(3,21);a.offset(0,3,0x58);a.indexed_load<1>(0,23,0);a.sign8(0,0);a.compare_registers(22,0);if(!a.less())break;
        a.at(0x101d7e58);a.sign8(0,21);a.multiply(3,0,0x2c);a.mask(0,22,0xfffffffc,false,2);a.add(0,3,0);a.indexed_load(30,23,0);
        a.move(3,30);a.call(address(Entry::current_position));a.sign8(0,3);a.compare(0,0);
        if(!a.less()){
            a.value(29,0);a.move(3,28);a.move(4,30);a.call(address(Entry::contains_marker));a.sign8(0,3);a.compare(0,0);
            if(!a.equal())marked_contribution(a);else ordinary_contribution(a);
            rank_candidate(a);
        }
        a.at(0x101d8094);a.offset(22,22,1);
    }
    a.at(0x101d80b0);a.compare(18,0,false);
    if(!a.equal()){
        a.compare(26,0,false);if(!a.equal()){a.move(3,18);a.move(4,26);a.value(5,0x1c4);a.call(0x10738d10);}
        a.at(0x101d80d0);a.compare(24,0,false);if(!a.equal()){a.move(3,18);a.move(4,24);a.value(5,0x1c4);a.call(0x10738d10);}
    }
    a.at(0x101d80e8);a.move(3,27);a.pop(0x90);a.multiple(17,true);a.finish();
}
void eligible_marker(Call& a){
    a.link();a.multiple(23);a.save_link();a.frame(0x60);a.move(30,3);a.move(31,4);a.store<1>(1,0x83,5);a.store<1>(1,0x87,6);
    a.value(29,0);a.value(28,0);a.value(27,0);a.value(26,0);a.value(25,0);a.value(24,0);a.compare(31,0,false);
    if(!a.equal()){
        a.value(23,1);a.move(3,31);a.call(0x101d5690);a.sign8(3,3);a.load<1>(0,1,0x83);a.sign8(0,0);a.compare_registers(0,3);
        if(a.equal()){
            a.move(3,31);a.call(0x101d5680);a.sign8(3,3);a.load<1>(0,1,0x87);a.sign8(0,0);a.compare_registers(0,3);
            if(a.equal())a.value(23,0);
        }
        a.at(0x101d81a0);a.compare(23,0);if(!a.equal())a.value(24,1);
    }
    a.at(0x101d81ac);a.compare(24,0);
    if(!a.equal()){a.load<1>(0,31,0x2c);a.sign8(0,0);a.compare(0,2);if(!a.at_least())a.value(25,1);}
    a.at(0x101d81c8);a.compare(25,0);
    if(!a.equal()){a.offset(3,30,0x10000);a.load(0,3,-0x2000);a.compare_registers(31,0,false);if(!a.equal())a.value(26,1);}
    a.at(0x101d81e4);a.compare(26,0);
    if(!a.equal()){a.offset(3,30,0x10000);a.load(0,3,-0x2008);a.compare_registers(31,0,false);if(!a.equal())a.value(27,1);}
    a.at(0x101d8200);a.compare(27,0);
    if(!a.equal()){a.move(3,31);a.call(address(Entry::tactical_marking_trait));a.sign8(0,3);a.compare(0,0);if(!a.equal())a.value(28,1);}
    a.at(0x101d8220);a.compare(28,0);
    if(!a.equal()){a.load<1>(0,30,0x7928);a.sign8(0,0);a.compare(0,0);if(a.equal())a.value(29,1);}
    a.at(0x101d823c);a.sign8(3,29);a.pop(0x60);a.multiple(23,true);a.finish();
}
}
void invoke(State& state,Entry entry,DependencyDispatch* dependencies){
    if(unsigned(entry)>=entries.size())throw std::runtime_error("Unknown marking-pressure entry");
    Call a(state,address(entry),dependencies);
    switch(entry){
    case Entry::trait_10a:a.load<1>(3,3,0x10a);break;
    case Entry::action_code:a.load<2>(3,3,0x19c,true);break;
    case Entry::append_marker:append_marker(a);return;
    case Entry::set_pressure_adjustment:set_pressure_adjustment(a);return;
    case Entry::trait_10d:a.load<1>(3,3,0x10d);break;
    case Entry::assess_cell:assess_cell(a);return;
    case Entry::pressure_adjustment:a.load<2>(3,3,0x1a0,true);break;
    case Entry::current_position:a.load<1>(3,3,0x19);break;
    case Entry::eligible_marker:eligible_marker(a);return;
    case Entry::tactical_marking_trait:tactical_marking_trait(a);return;
    case Entry::contains_marker:contains_marker(a);return;
    case Entry::remove_marker:remove_marker(a);return;
    case Entry::team:a.load<1>(3,3,0x27);break;
    }
    a.finish();
}
}
