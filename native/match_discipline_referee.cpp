#include "match_discipline.hpp"
#include "match_discipline_internal.hpp"

namespace cm::aot::match_discipline {
namespace {
using Call = DisciplineCall;
// Preserve the original wide-count arithmetic as well as the usual small
// byte-count path. The source compiler emitted this wider fallback even where
// current actor/official fields limit the count to a byte.
void sample_wide_count(Call& a, std::uint32_t site, unsigned count, unsigned result, bool first) {
    const auto buffer = first?0x48u:0x38u, result_buffer = first?0x38u:0x48u;
    a.at(site); a.value(4,0x43300000); a.exclusive_flags(0,count,0x80000000);
    a.store(1,buffer+4,0); a.value(5,0x80010000); a.load(7,22,0); a.value(3,0x10000);
    a.store(1,buffer,4); a.offset(6,5,-0x7fff); a.floating_load(2,31,8); a.multiply(0,count,10);
    a.floating_load(1,1,buffer); a.offset(5,3,-1); a.floating_load(0,31,0); a.floating_subtract(1,1,2);
    a.multiply_high(3,6,0); a.load<2>(8,7,0); a.load(7,19,0); a.floating_difference(1,1,0,true);
    a.store(1,0x40,4); a.add(7,8,7); a.multiply_high(4,6,7); a.add(0,3,0); a.add(3,4,7);
    a.shift_right(3,3,15); a.mask(4,3,1,false,1); a.shift_right(0,0,15); a.add(3,3,4);
    a.product(4,3,5); a.mask(3,0,1,false,1); a.add(3,0,3); a.subtract(0,4,7);
    a.exclusive_flags(0,0,0x80000000); a.store(1,0x44,0); a.floating_load(0,1,0x40);
    a.floating_subtract(0,0,2); a.floating_product(0,1,0); a.floating_integer(0,0);
    a.floating_store(0,1,result_buffer); a.load(result,1,result_buffer+4); a.call(0x1021b2a0);
    a.value(4,0x66660000); a.offset(3,3,5); a.offset(4,4,0x6667); a.offset(0,count,-1);
    a.multiply_high(3,4,3); a.shift_right(3,3,2); a.mask(4,3,1,false,1); a.add(3,3,4);
    a.add(result,result,3); a.compare_registers(result,0);
}
void sample_count(Call& a, std::uint32_t stream_site, unsigned count, unsigned result,
                  std::uint32_t wide_site, std::uint32_t small_site, std::uint32_t finish_site, bool first) {
    advance_discipline_random(a,stream_site,22,21,20,19);
    a.value(3,0x10000); a.offset(0,3,-1); a.compare_registers(count,0); bool wide = a.positive();
    if (!wide) { a.value(3,0xffff0000); a.offset(0,3,1); a.compare_registers(count,0); wide = !a.at_least(); }
    if (wide) {
        sample_wide_count(a,wide_site,count,result,first);
        if (!a.nonpositive()) { a.move(result,0); a.jump(finish_site); }
    } else {
        a.at(small_site); a.load(3,22,0); a.compare(count,0); a.load(0,19,0); a.load<2>(3,3,0);
        a.add(3,3,0); a.divide(0,3,count); a.product(0,0,count); a.subtract(0,0,3); a.negate(result,0);
        if (!a.nonpositive()) a.move(result,0);
    }
    a.at(finish_site);
}
void sample_prior_fouls(Call& a) {
    a.load<1>(29,4,0x14); a.compare(29,0);
    if (a.equal()) { a.value(18,0); a.jump(0x10223298); }
    else sample_count(a,0x10223150,29,18,0x10223198,0x1022326c,0x10223298,true);
}
void referee_strictness(Call& a) {
    a.load(3,23,4); a.sign8(29,18); a.compare(3,0,false);
    if (!a.equal()) {
        a.load<1>(0,3,0x25); a.sign8(0,0); a.compare(0,18);
        if (!a.nonpositive()) { a.offset(29,29,2); a.jump(0x102232ec); }
        else {
            a.at(0x102232c0); a.compare(0,12);
            if (!a.nonpositive()) { a.offset(29,29,1); a.jump(0x102232ec); }
            else {
                a.at(0x102232d0); a.compare(0,6);
                if (!a.positive()) {
                    a.compare(0,2);
                    if (!a.nonpositive()) { a.offset(29,29,-1); a.jump(0x102232ec); }
                    else { a.at(0x102232e8); a.offset(29,29,-2); }
                }
            }
        }
    }
    a.at(0x102232ec);
}
void incident_severity(Call& a) {
    a.sign8(0,26); a.compare(0,1); bool common = a.equal();
    if (!common) { a.compare(0,6); common = a.equal(); }
    if (common) { a.at(0x10223300); a.value(28,5); a.offset(29,29,4); a.jump(0x10223358); }
    else {
        a.at(0x1022330c); a.compare(0,2);
        if (a.equal()) { a.value(28,8); a.offset(29,29,10); a.jump(0x10223358); }
        else {
            a.at(0x10223320); a.compare(0,3);
            if (a.equal()) { a.value(28,1); a.offset(29,29,7); a.jump(0x10223358); }
            else {
                a.at(0x10223334); a.compare(0,4);
                if (a.equal()) { a.value(28,12); a.offset(29,29,100); a.jump(0x10223358); }
                else {
                    a.at(0x10223348); a.compare(0,5);
                    if (a.equal()) { a.value(28,12); a.offset(29,29,100); }
                }
            }
        }
    }
    a.at(0x10223358);
}
bool may_only_warn_booked_actor(Call& a) {
    a.load<1>(0,24,0x24); a.sign8(0,0,true); if (a.equal()) return false;
    a.sign8(0,26); a.compare(0,1); bool common = a.equal();
    if (!common) { a.compare(0,6); common = a.equal(); }
    if (!common) return false;
    a.at(0x10223378); a.load<1>(18,3,0x24); a.sign8(18,18,true);
    if (a.equal()) { a.value(17,0); a.jump(0x102234d4); }
    else sample_count(a,0x1022338c,18,17,0x102233d4,0x102234a8,0x102234d4,false);
    a.compare(17,0);
    if (a.equal()) return false;
    a.value(0,9); a.store<1>(25,0,0); a.jump(0x10223880); return true;
}
void store_red_reason(Call& a, bool active) {
    a.at(active?0x1022358c:0x102235d0); a.load<1>(3,24,0x24); a.load<1>(0,24,0x23);
    a.sign8(3,3); a.sign8(0,0);
    if (active) {
        a.add_record(0,3,0);
        if (a.equal()) { a.store<1>(24,0x25,28); a.jump(0x10223880); }
        else { a.at(0x102235ac); a.compare(0,0); if (!a.equal()) { a.store<1>(24,0x26,28); a.jump(0x10223880); } }
    } else {
        a.add(0,3,0); a.compare(0,1);
        if (!a.positive()) { a.store<1>(24,0x25,28); a.jump(0x10223880); }
        else { a.at(0x102235f4); a.compare(0,0); if (!a.equal()) { a.store<1>(24,0x26,28); a.jump(0x10223880); } }
    }
}
bool may_award_direct_red(Call& a) {
    a.at(0x102234e8); a.sign8(0,26); a.compare(0,1);
    bool common = a.equal(); if (!common) common = a.equal(); // Both original branch sites are retained.
    if (common) return false;
    advance_discipline_random(a,0x102234f8,22,21,20,19);
    a.load(5,22,0); a.value(3,0x88890000); a.load(4,19,0); a.offset(0,3,-0x7777);
    a.load<2>(3,5,0); a.sign8(5,29); a.add(4,3,4); a.multiply_high(0,0,4); a.add(0,0,4);
    a.shift_right(0,0,3); a.mask(3,0,1,false,1); a.add(0,0,3); a.multiply(0,0,15); a.subtract(3,0,4);
    a.offset(0,3,7); a.compare_registers(5,0); if (a.nonpositive()) return false;
    a.load<1>(0,24,0x19); a.sign8(0,0,true); bool active = a.at_least();
    if (active) { a.sign8(0,27,true); active = a.equal(); }
    if (active) {
        a.load<2>(3,24,0x36,true); a.value(0,4); a.offset(3,3,-1000); a.store<2>(24,0x36,3); a.store<1>(25,0,0);
        store_red_reason(a,true); return true;
    }
    a.at(0x102235bc); a.sign8(0,28); a.compare(0,12); bool record = a.equal();
    if (!record) { a.compare(0,8); record = a.equal(); }
    if (record) store_red_reason(a,false);
    return true;
}
void consider_booking(Call& a) {
    a.at(0x10223604); a.load<1>(0,24,0x24); a.sign8(0,0); a.multiply(3,0,3); a.offset_record(18,3,5);
    if (a.equal()) { a.value(17,0); a.jump(0x10223768); }
    else sample_count(a,0x10223620,18,17,0x10223668,0x1022373c,0x10223768,false);
    a.sign8(3,29); a.offset(0,17,2); a.compare_registers(3,0);
    if (a.nonpositive()) {
        a.at(0x10223864); a.load<1>(0,24,0x19); a.sign8(0,0,true);
        if (a.at_least()) { a.sign8(0,27,true); if (a.equal()) { a.value(0,9); a.store<1>(25,0,0); } }
        return;
    }
    a.load<1>(0,24,0x19); a.sign8(0,0,true); if (!a.at_least()) return;
    a.sign8(0,27,true); if (!a.equal()) return;
    time_scaled_booking_rating(a,0x1022378c,23,24,30,31,false,0x38,0x40);
    a.load<1>(3,24,0x24); a.sign8(0,3,true);
    if (!a.equal()) { a.offset(3,3,1); a.value(0,3); a.store<1>(24,0x24,3); a.store<1>(25,0,0); a.jump(0x10223830); }
    else { a.at(0x10223820); a.offset(3,3,1); a.value(0,2); a.store<1>(24,0x24,3); a.store<1>(25,0,0); }
    a.at(0x10223830); a.load<1>(3,24,0x24); a.load<1>(0,24,0x23); a.sign8(3,3); a.sign8(0,0);
    a.add(0,3,0); a.compare(0,1);
    if (a.equal()) { a.store<1>(24,0x25,28); a.jump(0x10223880); }
    else { a.at(0x10223854); a.compare(0,0); if (!a.equal()) { a.store<1>(24,0x26,28); a.jump(0x10223880); } }
}
}

void referee_response(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,entries[5],dependencies,observer);
    a.link(); a.multiple(17); a.load(30,2,-0x5268); a.move(24,4); a.load(31,2,-0x5264); a.move(23,3);
    a.load(19,2,-0x7b90); a.move(25,5); a.load(20,2,-0x7b8c); a.move(26,6); a.load(21,2,-0x7b88);
    a.move(27,7); a.load(22,2,-0x7b84); a.save_link(); a.frame(0x90);
    sample_prior_fouls(a); referee_strictness(a); incident_severity(a);
    if (!may_only_warn_booked_actor(a) && !may_award_direct_red(a)) consider_booking(a);
    a.at(0x10223880); a.load(0,1,0x98); a.offset(1,1,0x90); a.restore_link(); a.multiple(17,true); a.finish();
}
}
