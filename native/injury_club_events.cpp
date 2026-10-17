#include "injury_club_events.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::injury_club_events {
namespace {
using Call=SemanticGuestCall;
template<unsigned Cursor,unsigned Salt,unsigned End,unsigned Begin>
void advance_pool(Call& a,std::uint32_t start) {
    a.at(start);a.load(3,Cursor,0);a.load(0,End,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(Cursor,0,3);
    if(!a.nonpositive()){a.store(Cursor,0,Begin);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(Salt,0,0);}
}

bool eligible_event(Call& a) {
    a.sign8(0,23);a.compare(0,4);
    // The frame instruction follows these comparisons in the original body.
    a.frame(144);
    if(a.equal())return false;
    a.at(0x1018bb98);a.compare(0,3);
    if(a.equal()) {
        advance_pool<29,26,28,27>(a,0x1018bba0);
        a.at(0x1018bbc8);a.load(5,29,0);a.value(3,0x88890000);a.load(4,26,0);a.offset(0,3,-0x7777);a.load<2>(3,5,0);a.add(4,3,4);
        a.multiply_high(0,0,4);a.add(0,0,4);a.shift_right(0,0,3);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,15);a.subtract(0,0,4,true);
        if(!a.equal())return false;
    }
    a.at(0x1018bc00);a.sign8(0,23);a.compare(0,3);
    if(!a.equal()) {
        advance_pool<29,26,28,27>(a,0x1018bc0c);
        a.at(0x1018bc34);a.load(3,29,0);a.load(0,26,0);a.load<2>(3,3,0);a.add(3,3,0);a.mask(0,3,0xc0000000,false,30);
        a.mask(3,3,1,false,1);a.subtract(0,3,0);a.mask(0,0,0xffffffff,false,2);a.add_record(0,0,3);
        if(a.equal())return false;
    }
    a.at(0x1018bc5c);a.load(3,2,-0x7be8);a.multiply(0,24,0x248);a.load(3,3,0);a.add(3,3,0);a.call(0x100c7d00);a.sign8(0,3,true);
    if(!a.equal())return false;
    return true;
}

bool grow_event_queue(Call& a) {
    a.load(0,21,0x24);a.load(3,21,0x20);a.compare_registers(0,3);
    if(a.less())return true;
    a.offset(0,3,30);a.store(21,0x20,0);a.load(3,21,0x1c);a.compare(3,0,false);
    if(!a.equal()){a.load(0,21,0x20);a.multiply(4,0,20);a.call(0x107751b0);a.store(21,0x1c,3);a.jump(0x1018bcc0);}
    else{a.at(0x1018bcb0);a.load(0,21,0x20);a.multiply(3,0,20);a.call(0x10775130);a.store(21,0x1c,3);}
    a.at(0x1018bcc0);a.load(0,21,0x1c);a.compare(0,0,false);if(!a.equal())return true;
    a.offset(3,31,0x53);a.offset(6,1,0x40);a.offset(7,1,0x38);a.value(4,0);a.value(5,0);a.call(0x10739890);a.call(0x10768810);
    a.move(5,3);a.move(3,30);a.offset(4,31,0x5e);a.offset(6,1,0x40);a.offset(7,1,0x38);a.value(8,0xcea);a.call(0x1077b720);
    a.move(4,30);a.offset(3,31,0x6b);a.value(5,0);a.call(0x10769b60);a.load(3,2,-0x7bac);a.value(0,0);a.store<1>(3,0,0);a.jump(0x1018bdbc);return false;
}

void store_event(Call& a) {
    a.at(0x1018bd24);a.load(0,21,0x24);a.compare(25,0,false);a.load(4,21,0x1c);a.multiply(3,0,20);a.load<2>(0,22,2);a.add(3,4,3);
    a.store<2>(3,2,0);a.load<2>(0,22,0,true);a.store<2>(3,0,0);a.load<1>(0,22,4);a.store<1>(3,4,0);
    a.load(0,21,0x24);a.load(3,21,0x1c);a.multiply(0,0,20);a.add(3,3,0);a.store<1>(3,8,23);
    a.load(0,21,0x24);a.load(3,21,0x1c);a.multiply(0,0,20);a.add(3,3,0);a.store(3,12,24);
    if(!a.equal()){a.load(4,25,0x68);a.jump(0x1018bd88);}else{a.at(0x1018bd84);a.value(4,0xffffffff);}
    a.at(0x1018bd88);a.load(0,21,0x24);a.value(5,20);a.load(3,21,0x1c);a.multiply(0,0,20);a.load(6,2,-0x5460);a.add(3,3,0);a.store(3,16,4);
    a.load(3,21,0x24);a.offset(0,3,1);a.store(21,0x24,0);a.load(3,21,0x1c);a.load(4,21,0x24);a.call(0x10768300);
}

bool current_date_precedes_event(Call& a) {
    a.at(0x1018be40);a.load(4,27,0);a.load(0,31,0x1c);a.store(1,0x58,4);a.add(3,0,21);a.load(0,27,4);a.load<2>(6,1,0x5a);a.load<2>(5,3,2);
    a.store(1,0x40,4);a.compare_registers(6,5,false);a.store(1,0x44,0);a.store(1,0x5c,0);
    if(!a.at_least()){a.value(0,1);a.jump(0x1018bea4);}
    else{a.at(0x1018be78);if(!a.nonpositive()){a.value(0,0);a.jump(0x1018bea4);}else{
        a.at(0x1018be84);a.load<2>(0,1,0x58,true);a.load<2>(5,3,0,true);a.combine(0,5,0,false,true);a.shift_right(4,0,1);a.intersect(0,0,5);a.subtract(0,0,4);a.mask(0,0,1,false,1);a.sign8(0,0);
    }}
    a.at(0x1018bea4);a.sign8(0,0,true);return !a.equal();
}

bool event_precedes_cutoff(Call& a) {
    a.at(0x1018beac);a.store(1,0x50,28);a.load<2>(0,3,2);a.load<2>(4,1,0x52);a.store(1,0x54,29);a.compare_registers(4,0,false);
    if(!a.nonpositive()){a.value(4,1);a.jump(0x1018bef8);}
    else{a.at(0x1018becc);if(!a.at_least()){a.value(4,0);a.jump(0x1018bef8);}else{
        a.at(0x1018bed8);a.load<2>(6,1,0x50,true);a.load<2>(4,3,0,true);a.combine(4,6,4,false,true);a.shift_right(5,4,1);a.intersect(4,4,6);a.subtract(4,4,5);a.mask(4,4,1,false,1);a.sign8(4,4);
    }}
    a.at(0x1018bef8);a.sign8(4,4,true);if(a.equal())return false;
    a.offset(18,18,1);a.jump(0x1018c120);return true;
}

bool select_event_count(Call& a) {
    a.at(0x1018bf08);a.load<1>(4,3,8);a.compare(4,3);
    if(a.equal()) {
        a.load(4,1,0x40);a.value(5,0);a.load(6,1,0x44);a.store(1,0x48,4);a.load<2>(4,1,0x4a);a.store(1,0x4c,6);a.compare_registers(4,0,false);
        if(a.equal()){a.load<2>(4,1,0x48,true);a.load<2>(0,3,0,true);a.compare_registers(4,0);if(a.equal())a.value(5,1);}
        a.at(0x1018bf48);a.compare(5,0);if(a.equal())return false;
        advance_pool<25,22,24,23>(a,0x1018bf50);
        a.at(0x1018bf78);a.load(3,25,0);a.load(0,22,0);a.load<2>(3,3,0);a.add(3,3,0);a.mask(0,3,0xc0000000,false,30);a.mask(3,3,1,false,1);
        a.subtract(0,3,0);a.mask(0,0,0xffffffff,false,2);a.add(3,0,3);a.offset(0,3,1);a.sign8(17,0);a.jump(0x1018c000);
    } else {
        advance_pool<25,22,24,23>(a,0x1018bfa8);
        a.at(0x1018bfd0);a.load(5,25,0);a.value(3,0x55550000);a.load(4,22,0);a.offset(0,3,0x5556);a.load<2>(3,5,0);a.add(4,3,4);
        a.multiply_high(3,0,4);a.mask(0,3,1,false,1);a.add(0,3,0);a.multiply(0,0,3);a.subtract(0,0,4);a.sign8(17,0);
    }
    return true;
}

void apply_event_to_roster(Call& a) {
    a.at(0x1018c000);a.load(3,31,0x1c);a.offset(0,21,12);a.load(4,15,0);a.indexed_load(0,3,0);a.multiply(0,0,0x248);a.add(16,4,0);a.move(3,16);a.call(0x100c7d00);a.sign8(0,3,true);
    if(!a.equal())return;
    a.value(3,0x51ec0000);a.value(19,0);a.offset(30,3,-0x7ae1);a.jump(0x1018c110);
    for(;;) {
        a.at(0x1018c110);a.compare(19,20);if(a.at_least())return;
        a.sign8(0,17,true);if(!a.positive())return;
        advance_pool<25,22,24,23>(a,0x1018c038);
        a.at(0x1018c060);a.load(3,25,0);a.load(0,22,0);a.load<2>(3,3,0);a.add(4,3,0);a.multiply_high(0,30,4);a.shift_right(0,0,4);a.mask(3,0,1,false,1);
        a.add(0,0,3);a.multiply(0,0,50);a.subtract(0,0,4);a.sign8(0,0);a.mask(3,0,0xfffffffc,false,2);a.offset(0,3,0x98);a.indexed_load(6,16,0);a.compare(6,0,false);
        if(!a.equal()) {
            a.at(0x1018c0a0);bool valid=a.equal();
            if(!valid){a.load(3,6,0);a.load(0,26,0);a.compare_registers(3,0);valid=a.less();}
            if(valid) {
                a.at(0x1018c0b4);a.load(3,6,0);a.load(0,31,0x1c);a.multiply(3,3,0x24);a.load(5,31,0);a.add(7,21,0);a.load<1>(4,7,8);a.offset(0,3,0x21);
                a.indexed_load<1>(3,5,0);a.sign8(0,4);a.sign8(3,3);a.compare_registers(3,0);
                if(!a.equal()) {
                    a.value(0,0);a.move(3,31);a.store(1,0x38,0);a.value(5,0);a.value(8,0);a.value(10,0);a.load(9,7,16);a.value(7,0);a.call(0x10188cc0);a.offset(17,17,-1);
                }
            }
        }
        a.at(0x1018c10c);a.offset(19,19,1);
    }
}

void compact_expired_events(Call& a) {
    a.at(0x1018c134);a.compare(18,0);
    if(!a.equal()) {
        a.compare_registers(18,7);
        if(!a.equal()){a.subtract(0,18,7);a.load(4,31,0x1c);a.multiply(3,18,20);a.multiply(5,0,20);a.add(3,4,3);a.call(0x1077fc28);a.load(2,1,0x14);}
    }
    a.at(0x1018c160);a.load(0,31,0x24);a.subtract(0,18,0);a.store(31,0x24,0);
}
}

void enqueue_club_event(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,enqueue_entry,d,o);
    a.link();a.multiple(21);a.move(23,5);a.load(26,2,-0x7b90);a.load(27,2,-0x7b8c);a.move(21,3);a.load(28,2,-0x7b88);a.move(22,4);a.load(29,2,-0x7b84);
    a.move(24,6);a.load(30,2,-0x7ba8);a.move(25,7);a.load(31,2,-0x5438);a.save_link();
    if(eligible_event(a) && grow_event_queue(a))store_event(a);
    a.at(0x1018bdbc);a.pop(144);a.multiple(21,true);a.finish();
}

void process_club_events(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,process_entry,d,o);
    a.link();a.multiple(15);a.load(27,2,-0x7b80);a.move(31,3);a.load(22,2,-0x7b90);a.value(18,0);a.load(23,2,-0x7b8c);a.move(4,27);a.load(24,2,-0x7b88);a.value(5,5);
    a.load(25,2,-0x7b84);a.load(26,2,-0x7be4);a.load(15,2,-0x7be8);a.save_link();a.frame(192);a.offset(3,1,0x60);a.call(0x10717b20);
    a.load<2>(4,1,0x60,true);a.value(20,0);a.load<2>(3,1,0x62);a.value(21,0);a.load<1>(0,1,0x64);a.store<2>(1,0x68,4);a.store<2>(1,0x6a,3);a.store<1>(1,0x6c,0);a.load(28,1,0x68);a.load(29,1,0x6c);a.jump(0x1018c128);
    for(;;) {
        a.at(0x1018c128);a.load(7,31,0x24);a.compare_registers(20,7);if(!a.less())break;
        if(current_date_precedes_event(a))break;
        if(!event_precedes_cutoff(a) && select_event_count(a))apply_event_to_roster(a);
        a.at(0x1018c120);a.offset(21,21,20);a.offset(20,20,1);
    }
    compact_expired_events(a);
    a.at(0x1018c16c);a.pop(192);a.multiple(15,true);a.finish();
}
}
