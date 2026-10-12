#include "match_discipline.hpp"
#include "match_discipline_internal.hpp"

namespace cm::aot::match_discipline {
void advance_discipline_random(DisciplineCall& a, std::uint32_t site,
                               unsigned cursor, unsigned limit, unsigned buffer, unsigned seed) {
    a.at(site); a.load(3,cursor,0); a.load(0,limit,0); a.offset(3,3,2);
    a.compare_registers(3,0,false); a.store(cursor,0,3);
    if (!a.nonpositive()) {
        a.store(cursor,0,buffer); a.call(0x1077b770); a.mask(0,3,0xffff); a.store(seed,0,0);
    }
    a.at(site+0x28);
}

void time_scaled_booking_rating(DisciplineCall& a, std::uint32_t site,
                               unsigned context, unsigned actor, unsigned constants,
                               unsigned wide_constants, bool load_wide_constants,
                               unsigned buffer, unsigned result_buffer) {
    a.at(site); a.load(4,context,0); a.value(3,0x2e8c0000); a.offset(3,3,-0x5d17);
    a.floating_load(1,constants,0x10,true); a.load<2>(0,4,0x7948,true);
    a.multiply_high(0,3,0); a.shift_right(0,0,1); a.mask(3,0,1,false,1); a.add(0,0,3);
    a.sign16(0,0); a.compare(0,45);
    if (!a.nonpositive()) {
        a.exclusive_flags(3,0,0x80000000); a.value(0,0x43300000);
        if (load_wide_constants) a.load(4,2,-0x5264);
        a.store(1,buffer+4,3); a.floating_load(1,wide_constants,8); a.store(1,buffer,0);
        a.floating_load(2,constants,0x14,true); a.floating_load(0,1,buffer);
        a.floating_subtract(0,0,1,true); a.floating_difference(1,2,0,true,true);
    }
    a.at(site+(load_wide_constants?0x58:0x54)); a.floating_load(0,constants,0x18,true);
    a.load<2>(3,actor,0x36,true); a.floating_product_single(0,0,1); a.floating_integer(0,0);
    a.floating_store(0,1,result_buffer); a.load(0,1,result_buffer+4); a.add(0,3,0); a.store<2>(actor,0x36,0);
}

namespace {
using Call = DisciplineCall;
void finish_single(Call& a) {
    a.at(0x1021c240); a.load(0,1,0x88); a.offset(1,1,0x80); a.restore_link(); a.multiple(21,true); a.finish();
}
void finish_contact(Call& a) {
    a.at(0x1022227c); a.load(0,1,0x88); a.offset(1,1,0x80); a.restore_link(); a.multiple(19,true); a.finish();
}
void record_contact_fouls(Call& a, std::uint32_t site) {
    a.at(site); a.load<1>(3,20,0x14); a.offset(0,3,1); a.store<1>(20,0x14,0);
    a.load<1>(3,21,0x15); a.offset(0,3,1); a.store<1>(21,0x15,0);
}
void record_contact_team_foul(Call& a, std::uint32_t site, bool special) {
    a.at(site);
    if (special) { a.load<1>(0,20,0x27); a.load(3,19,0); a.sign8(0,0); a.multiply(0,0,0x36); a.offset(3,3,0x10000); a.add(4,3,0); }
    else { a.load<1>(3,20,0x27); a.offset(4,4,0x10000); a.sign8(0,9,true); a.sign8(0,3); a.multiply(0,0,0x36); a.add(4,4,0); }
    a.load<1>(3,4,-0x1fb7); a.offset(0,3,1); a.store<1>(4,-0x1fb7,0);
}
void contact_red_reason(Call& a, bool special) {
    a.at(special?0x10221f7c:0x10222110); a.load<1>(0,20,0x24); a.sign8(0,0,true);
    if (!a.equal()) {
        if (special) a.value(0,9);
        a.store<1>(20,0x26,special?0:23); a.jump(special?0x10222264:0x10222128);
    } else {
        a.at(special?0x10221f94:0x10222124); if (special) a.value(0,9);
        a.store<1>(20,0x25,special?0:23);
        if (special) a.jump(0x10222264);
    }
}
void contact_booking(Call& a, bool special) {
    const auto start = special?0x10221fc4u:0x10222194u;
    time_scaled_booking_rating(a,start,19,20,27,4,true,special?0x40:0x38,special?0x38:0x40);
    a.load<1>(3,20,0x24); a.sign8(0,3,true);
    if (!a.equal()) {
        a.offset(3,3,1); a.value(0,3); a.store<1>(20,0x24,3); a.floating_load(0,27,8,true);
        a.store<1>(22,0,0); a.floating_store_single(0,24,0); a.store<1>(20,0x26,23); a.jump(0x10222264);
    } else {
        a.at(special?0x10222068:0x10222238); a.offset(3,3,1); a.value(0,2); a.store<1>(20,0x24,3);
        a.floating_load(0,27,0xc,true); a.store<1>(22,0,0); a.floating_store_single(0,24,0);
        a.store<1>(20,0x25,23); a.jump(0x10222264);
    }
}
void single_booking(Call& a, bool special) {
    if (special) { a.at(0x1021c02c); a.load<2>(3,22,0x36,true); a.offset(0,3,-0x177); a.store<2>(22,0x36,0); }
    else time_scaled_booking_rating(a,0x1021c170,21,22,27,4,true,0x40,0x38);
    a.load<1>(3,22,0x24); a.sign8(0,3,true);
    if (!a.equal()) {
        a.offset(3,3,1); a.value(0,3); a.store<1>(22,0x24,3); a.floating_load(0,27,8,true);
        a.store<1>(23,0,0); a.floating_store_single(0,25,0); a.store<1>(22,0x26,24); a.jump(0x1021c240);
    } else {
        a.at(special?0x1021c064:0x1021c214); a.offset(3,3,1); a.value(0,2); a.store<1>(22,0x24,3);
        a.floating_load(0,27,0xc,true); a.store<1>(23,0,0); a.floating_store_single(0,25,0);
        a.store<1>(22,0x25,24); a.jump(0x1021c240);
    }
}
bool single_special(Call& a) {
    advance_discipline_random(a,0x1021bf64); a.load(5,31,0); a.value(3,0x05760000); a.load(4,28,0);
    a.offset(0,3,0x19f1); a.load<2>(3,5,0); a.add(4,3,4); a.multiply_high(0,0,4);
    a.shift_right(0,0,6); a.mask(3,0,1,false,1); a.add(0,0,3); a.multiply(0,0,3000);
    a.subtract(0,0,4); a.sign16(0,0); a.compare(0,475);
    if (!a.nonpositive()) {
        a.load<2>(3,22,0x36,true); a.value(0,6); a.floating_load(0,27,4,true); a.offset(3,3,-750);
        a.store<2>(22,0x36,3); a.store<1>(23,0,0); a.floating_store_single(0,25,0);
        a.load<1>(0,22,0x24); a.sign8(0,0,true);
        if (!a.equal()) { a.value(0,9); a.store<1>(22,0x26,0); a.jump(0x1021c240); }
        else { a.at(0x1021bffc); a.value(0,9); a.store<1>(22,0x25,0); a.jump(0x1021c240); }
        return true;
    }
    a.at(0x1021c008); a.compare(0,50);
    if (!a.positive()) {
        a.move(3,22); a.call(0x10283690); a.move(4,3); a.move(3,22); a.call(0x10239d10); a.sign8(0,3,true);
        if (a.equal()) return true;
    }
    single_booking(a,true); return true;
}
void single_regular(Call& a) {
    advance_discipline_random(a,0x1021c084); a.load(5,31,0); a.value(3,0x88890000); a.load(4,28,0);
    a.offset(0,3,-0x7777); a.load<2>(3,5,0); a.sign8(26,26); a.add(4,3,4);
    a.multiply_high(0,0,4); a.add(0,0,4); a.shift_right(0,0,3); a.mask(3,0,1,false,1);
    a.add(0,0,3); a.multiply(0,0,15); a.subtract(3,0,4); a.offset(0,3,9); a.compare_registers(26,0);
    if (!a.nonpositive()) {
        a.load<2>(3,22,0x36,true); a.value(0,4); a.floating_load(0,27,4,true); a.offset(3,3,-1000);
        a.store<2>(22,0x36,3); a.store<1>(23,0,0); a.floating_store_single(0,25,0); a.jump(0x1021c240); return;
    }
    a.at(0x1021c110); a.load(0,30,0); a.offset(3,5,2); a.store(31,0,3); a.compare_registers(3,0,false);
    if (!a.nonpositive()) { a.store(31,0,29); a.call(0x1077b770); a.mask(0,3,0xffff); a.store(28,0,0); }
    a.at(0x1021c134); a.load(5,31,0); a.value(3,0x66660000); a.load(4,28,0); a.offset(0,3,0x6667);
    a.load<2>(3,5,0); a.add(4,3,4); a.multiply_high(0,0,4); a.shift_right(0,0,1); a.mask(3,0,1,false,1);
    a.add(0,0,3); a.multiply(0,0,5); a.subtract(3,0,4); a.offset(0,3,3); a.compare_registers(26,0);
    if (!a.nonpositive()) single_booking(a,false);
    else { a.at(0x1021c234); a.load<2>(3,22,0x36,true); a.offset(0,3,-25); a.store<2>(22,0x36,0); }
}
bool contact_special(Call& a) {
    advance_discipline_random(a,0x10221efc); a.load(5,31,0); a.value(3,0x05760000); a.load(4,28,0);
    a.offset(0,3,0x19f1); a.load<2>(3,5,0); a.add(4,3,4); a.multiply_high(0,0,4);
    a.shift_right(0,0,6); a.mask(3,0,1,false,1); a.add(0,0,3); a.multiply(0,0,3000); a.subtract(0,0,4);
    a.sign16(0,0); a.compare(0,375);
    if (!a.nonpositive()) {
        a.load<2>(3,20,0x36,true); a.value(0,5); a.floating_load(0,27,4,true); a.offset(3,3,-750);
        a.store<2>(20,0x36,3); a.store<1>(22,0,0); a.floating_store_single(0,24,0); contact_red_reason(a,true); return true;
    }
    a.at(0x10221fa0); a.compare(0,50);
    if (!a.positive()) {
        a.move(3,20); a.call(0x10283690); a.move(4,3); a.move(3,20); a.call(0x10239d10); a.sign8(0,3,true);
        if (a.equal()) return true;
    }
    contact_booking(a,true); return true;
}
void contact_regular(Call& a) {
    advance_discipline_random(a,0x10222088); a.load(3,31,0); a.value(4,0x88890000); a.load(5,28,0); a.offset(0,4,-0x7777);
    a.load<2>(4,3,0); a.sign8(26,26); a.add(5,4,5); a.multiply_high(0,0,5); a.add(0,0,5); a.shift_right(0,0,3);
    a.mask(4,0,1,false,1); a.add(0,0,4); a.multiply(0,0,15); a.subtract(4,0,5); a.offset(0,4,7); a.compare_registers(26,0);
    if (!a.nonpositive()) {
        a.load<2>(3,20,0x36,true); a.value(0,4); a.floating_load(0,27,4,true); a.offset(3,3,-1000);
        a.store<2>(20,0x36,3); a.store<1>(22,0,0); a.floating_store_single(0,24,0); contact_red_reason(a,false);
        a.at(0x10222128); a.sign8(0,25,true);
        if (!a.equal()) { a.value(0,1); a.store<1>(20,0x16,0); a.jump(0x10222264); }
        return;
    }
    a.at(0x1022213c); a.load(0,30,0); a.offset(3,3,2); a.store(31,0,3); a.compare_registers(3,0,false);
    if (!a.nonpositive()) { a.store(31,0,29); a.call(0x1077b770); a.mask(0,3,0xffff); a.store(28,0,0); }
    a.at(0x10222160); a.load(5,31,0); a.value(3,0x2aab0000); a.load(4,28,0); a.offset(0,3,-0x5555);
    a.load<2>(3,5,0); a.add(4,3,4); a.multiply_high(3,0,4); a.mask(0,3,1,false,1); a.add(0,3,0);
    a.multiply(0,0,6); a.subtract(0,0,4); a.compare_registers(26,0);
    if (!a.nonpositive()) contact_booking(a,false);
    else { a.at(0x10222258); a.load<2>(3,20,0x36,true); a.offset(0,3,-25); a.store<2>(20,0x36,0); }
}
}

void single_actor_penalty(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,entries[3],dependencies,observer);
    a.link(); a.multiple(21); a.load(27,2,-0x5268); a.move(21,3); a.move(23,5); a.load(28,2,-0x7b90);
    a.floating_load(0,27,0,true); a.move(25,9); a.load(29,2,-0x7b8c); a.move(22,4); a.load(30,2,-0x7b88);
    a.move(24,6); a.load(31,2,-0x7b84); a.move(26,7); a.save_link(); a.frame(0x80); a.floating_store_single(0,9,0);
    a.load<1>(0,4,0x27); a.load(3,3,0); a.sign8(0,0); a.multiply(0,0,0x36); a.offset(3,3,0x10000); a.add(5,3,0);
    a.load<1>(3,5,-0x1fb7); a.offset(0,3,1); a.store<1>(5,-0x1fb7,0); a.load<1>(3,4,0x14);
    a.offset(0,3,1); a.store<1>(4,0x14,0); a.load<1>(0,23,0); a.compare(0,1);
    if (!a.equal()) { finish_single(a); return; }
    a.load(3,21,0); a.load<1>(0,3,0x7929); a.sign8(0,0,true);
    if (!a.equal()) { finish_single(a); return; }
    a.sign8(0,8,true); bool special = !a.equal();
    if (!special) { a.call(0x101e3d50); a.mask(0,3,1,true); special = !a.equal(); }
    if (special) (void)single_special(a); else single_regular(a);
    finish_single(a);
}

void contact_penalty(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,entries[4],dependencies,observer);
    a.link(); a.multiple(19); a.load(27,2,-0x5268); a.move(24,10); a.load(28,2,-0x7b90); a.move(20,4);
    a.floating_load(0,27,0,true); a.move(19,3); a.load(29,2,-0x7b8c); a.move(21,5); a.load(30,2,-0x7b88);
    a.move(22,6); a.load(31,2,-0x7b84); a.move(23,7); a.move(26,8); a.save_link(); a.frame(0x80);
    a.load<1>(25,1,0xbb); a.floating_store_single(0,10,0); a.load<1>(0,4,0x19); a.sign8(0,0,true);
    if (!a.at_least()) { a.floating_load(0,27,0x1c,true); a.floating_store_single(0,24,0); a.jump(0x1022227c); finish_contact(a); return; }
    a.at(0x10221e64); a.load<1>(0,22,0); a.sign8(0,0); a.compare(0,7);
    if (a.equal()) {
        a.floating_load(0,27,0x20,true); a.floating_store_single(0,24,0); record_contact_team_foul(a,0x10221e7c,true);
        record_contact_fouls(a,0x10221ea0); a.jump(0x1022227c); finish_contact(a); return;
    }
    a.at(0x10221ebc); a.compare(0,1);
    if (!a.equal()) { finish_contact(a); return; }
    a.load(4,19,0); a.load<1>(0,4,0x7929); a.sign8(0,0,true);
    if (!a.equal()) { finish_contact(a); return; }
    record_contact_team_foul(a,0x10221ed4,false);
    if (!a.equal()) (void)contact_special(a); else contact_regular(a);
    record_contact_fouls(a,0x10222264); finish_contact(a);
}
}
