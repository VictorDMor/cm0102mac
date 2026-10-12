#include "actor_motion.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_motion {
namespace {
using Call = SemanticGuestCall;

void coordinate_diagnostic(Call& a, bool full_cell) {
    // Keep the original diagnostic and termination dependencies explicit.
    // A damaged cell is not silently repaired or clamped by the native port.
    a.offset(3,31,0x8f); a.offset(6,1,full_cell ? 0x48 : 0x68); a.offset(7,1,full_cell ? 0x38 : 0x40);
    a.value(4,0); a.value(5,0); a.call(0x10739890); a.call(0x10768810); a.move(5,3); a.move(3,30);
    a.offset(4,31,0x9c); a.offset(6,1,full_cell ? 0x48 : 0x68); a.offset(7,1,full_cell ? 0x38 : 0x40);
    a.value(8,full_cell ? 0x132c : 0x1309); a.call(0x1077b720); a.move(4,30); a.offset(3,31,0xa9);
    a.value(5,0); a.call(0x10769b60); a.value(0,0);
    if (full_cell) a.value(3,0xffffffff);
    a.store<1>(29,0,0);
    if (full_cell) a.call(0x1076f7d0); else a.jump(0x1023781c);
}

void migrate_registered_cell(Call& a) {
    // The live cell stores an unordered per-side list. Remove the selected
    // member by moving the last member and updating that member's list index.
    a.at(0x10237604); a.load<1>(0,25,0x27); a.load<1>(6,25,0x27); a.sign8(5,0); a.load<1>(4,25,0x27);
    a.sign8(6,6); a.load<1>(0,25,0x28); a.add(5,3,5); a.sign8(4,4); a.multiply(6,6,0x2c);
    a.load<1>(5,5,0x58); a.sign8(0,0); a.sign8(5,5); a.add(6,3,6); a.mask(0,0,0xfffffffc,false,2);
    a.mask(5,5,0xfffffffc,false,2); a.multiply(4,4,0x2c); a.add(5,6,5); a.load(5,5,-4); a.add(0,4,0);
    a.indexed_store(5,3,0); a.load<1>(0,25,0x27); a.sign8(0,0); a.add(5,3,0); a.load<1>(4,5,0x58);
    a.offset(0,4,-1); a.store<1>(5,0x58,0); a.load<1>(4,25,0x27); a.load<1>(0,25,0x28); a.sign8(4,4);
    a.sign8(0,0); a.multiply(4,4,0x2c); a.mask(0,0,0xfffffffc,false,2); a.add(0,4,0); a.indexed_load(0,3,0);
    a.compare(0,0,false);
    if (!a.equal()) {
        a.load<1>(4,25,0x27); a.load<1>(0,25,0x28); a.sign8(4,4); a.load<1>(5,25,0x28);
        a.multiply(4,4,0x2c); a.sign8(0,0); a.mask(0,0,0xfffffffc,false,2); a.add(0,4,0); a.indexed_load(3,3,0);
        a.store<1>(3,0x28,5);
    }
    a.at(0x102376bc); a.load<1>(0,25,0x27); a.sign8(0,0); a.add(3,28,0); a.load<1>(0,3,0x58);
    a.store<1>(25,0x28,0); a.load<1>(0,25,0x27); a.sign8(0,0); a.add(4,28,0); a.load<1>(3,4,0x58);
    a.offset(0,3,1); a.store<1>(4,0x58,0); a.load<1>(3,25,0x27); a.load<1>(0,25,0x28); a.sign8(3,3);
    a.multiply(3,3,0x2c); a.sign8(0,0); a.mask(0,0,0xfffffffc,false,2); a.add(0,3,0); a.indexed_store(25,28,0);
    a.jump(0x102377ec);
}

void register_new_cell(Call& a) {
    a.at(0x1023770c); a.sign8(0,26); a.sign8(3,27); a.multiply(4,0,0x450); a.load<1>(0,25,0x27);
    a.load(5,25,0x1a4); a.sign8(0,0); a.multiply(3,3,0x5c); a.add(3,4,3); a.offset(28,3,0xa08);
    a.add(28,5,28); a.add(3,28,0); a.load<1>(0,3,0x58); a.store<1>(25,0x28,0); a.load<1>(0,25,0x27);
    a.sign8(0,0); a.add(4,28,0); a.load<1>(3,4,0x58); a.offset(0,3,1); a.store<1>(4,0x58,0);
    a.load<1>(0,25,0x27); a.sign8(0,0); a.add(3,28,0); a.load<1>(0,3,0x58); a.sign8(0,0); a.compare(0,11);
    if (!a.nonpositive()) coordinate_diagnostic(a,true);
    a.at(0x102377cc); a.load<1>(3,25,0x27); a.load<1>(0,25,0x28); a.sign8(3,3); a.multiply(3,3,0x2c);
    a.sign8(0,0); a.mask(0,0,0xfffffffc,false,2); a.add(0,3,0); a.indexed_store(25,28,0);
}

void set_coordinates(Call& a) {
    a.link(); a.multiple(25); a.move(26,4); a.move(25,3); a.load(29,2,-0x7bac); a.move(27,5);
    a.load(30,2,-0x7ba8); a.load(31,2,-0x51e4); a.save_link(); a.frame(0xb0);
    a.load<1>(0,3,0x105); a.sign8(3,26); a.sign8(0,0); a.compare_registers(3,0);
    bool changed = !a.equal();
    if (!changed) { a.load<1>(0,25,0x106); a.sign8(3,27); a.sign8(0,0); a.compare_registers(3,0); changed=!a.equal(); }
    if (changed) {
        a.at(0x10237540); a.load<1>(0,25,0x28); a.sign8(0,0); a.compare(0,-1);
        const bool registered = !a.nonpositive();
        bool migrate = true;
        if (registered) {
            a.load<1>(0,25,0x105); a.sign8(6,26); a.load<1>(3,25,0x106); a.sign8(5,27); a.sign8(4,0);
            a.load<1>(0,25,0x27); a.sign8(3,3); a.load(8,25,0x1a4); a.multiply(4,4,0x450); a.sign8(0,0);
            a.load(7,25,0x1a4); a.multiply(3,3,0x5c); a.add(3,4,3); a.offset(3,3,0xa08); a.add(3,8,3);
            a.add(4,3,0); a.load<1>(0,4,0x58); a.multiply(4,6,0x450); a.sign8(0,0,true); a.multiply(0,5,0x5c);
            a.add(4,4,0); a.offset(28,4,0xa08); a.add(28,7,28); migrate=!a.equal();
            if (!migrate) coordinate_diagnostic(a,false);
            else migrate_registered_cell(a);
        } else register_new_cell(a);
        if (migrate) {
            a.at(0x102377ec); a.store<1>(25,0x105,26); a.move(3,25); a.store<1>(25,0x106,27); a.call(0x102775c0);
            a.sign8(0,3,true);
            if (!a.equal()) {
                a.load<1>(0,25,0x105); a.load(3,25,0x1a4); a.store<1>(3,0x791d,0);
                a.load<1>(0,25,0x106); a.load(3,25,0x1a4); a.store<1>(3,0x791e,0);
            }
        }
    }
    a.at(0x1023781c); a.pop(0xb0); a.multiple(25,true); a.finish();
}

void advance_heading_stream(Call& a, bool traits = false) {
    a.load(3,31,0); a.load(0,30,0); a.offset(3,3,2);
    if (traits) { a.load<1>(26,27,0x108); a.load<1>(25,27,0x10f); }
    a.compare_registers(3,0,false); a.store(31,0,3);
    if (!a.nonpositive()) { a.store(31,0,29); a.call(0x1077b770); a.mask(0,3,0xffff); a.store(28,0,0); }
}

void apply_heading_jitter(Call& a) {
    a.at(0x1023793c); advance_heading_stream(a,true);
    a.at(0x1023796c); a.load(3,31,0); a.value(4,0x88890000); a.load(5,28,0); a.offset(4,4,-0x7777);
    a.load<2>(6,3,0); a.sign8(3,26); a.sign8(0,25); a.add(6,6,5); a.multiply_high(4,4,6); a.add(4,4,6);
    a.shift_right(4,4,6); a.mask(5,4,1,false,1); a.add(4,4,5); a.multiply(4,4,120); a.product(0,3,0);
    a.subtract(4,4,6); a.compare_registers(0,4);
    if (!a.at_least()) {
        advance_heading_stream(a);
        a.at(0x102379dc); a.load(5,31,0); a.value(4,0x88890000); a.load(3,31,0); a.offset(4,4,-0x7777);
        a.load<2>(6,5,0); a.load(5,28,0); a.offset(3,3,2); a.load(0,30,0); a.add(5,6,5); a.store(31,0,3);
        a.multiply_high(4,4,5); a.compare_registers(3,0,false); a.add(0,4,5); a.shift_right(0,0,4);
        a.mask(3,0,1,false,1); a.add(0,0,3); a.multiply(0,0,30); a.subtract(25,0,5);
        if (!a.nonpositive()) { a.store(31,0,29); a.call(0x1077b770); a.mask(0,3,0xffff); a.store(28,0,0); }
        a.at(0x10237a38); a.load(4,31,0); a.value(3,0x88890000); a.load<2>(0,27,0x19e,true); a.offset(3,3,-0x7777);
        a.load<2>(5,4,0); a.load(4,28,0); a.add(0,0,25); a.add(5,5,4); a.multiply_high(3,3,5); a.add(3,3,5);
        a.shift_right(3,3,4); a.mask(4,3,1,false,1); a.add(3,3,4); a.multiply(3,3,30); a.subtract(3,3,5);
        a.subtract(0,3,0); a.store<2>(27,0x19e,0);
    }
}

void apply_player_heading_bias(Call& a) {
    a.at(0x10237a7c); a.load(4,27,0x70); a.load(3,27,0x70); a.load<1>(4,4,0x30); a.load<1>(0,3,0x3b);
    a.sign8(3,4); a.load<2>(4,27,0x19e,true); a.sign8(0,0); a.subtract(0,3,0); a.mask(0,0,0xfffffffe,false,1);
    a.sign16(0,0); a.add(0,4,0); a.store<2>(27,0x19e,0); a.load<2>(0,27,0x19e,true); a.compare(0,0);
    if (!a.at_least()) { a.load<2>(3,27,0x19e,true); a.offset(0,3,360); a.store<2>(27,0x19e,0); a.jump(0x10237ae0); }
    else {
        a.at(0x10237ac8); a.load<2>(0,27,0x19e,true); a.compare(0,359);
        if (!a.nonpositive()) { a.load<2>(3,27,0x19e,true); a.offset(0,3,-360); a.store<2>(27,0x19e,0); }
    }
}

void set_heading(Call& a) {
    a.link(); a.multiple(25); a.load(28,2,-0x7b90); a.move(27,3); a.load(29,2,-0x7b8c); a.load(30,2,-0x7b88);
    a.load(31,2,-0x7b84); a.save_link(); a.subtract(0,4,6); a.sign8(4,0,true); a.subtract(0,5,7); a.frame(0x60); a.sign8(5,0);
    bool displaced=!a.equal();
    if (!displaced) { a.sign8(0,5,true); displaced=!a.equal(); }
    if (displaced) {
        a.at(0x10237870); a.sign8(3,4); a.sign8(0,5); a.multiply(3,3,48); a.load(4,2,-0x641c); a.mask(0,0,0xfffffffe,false,1);
        a.add(3,4,3); a.add(3,3,0); a.load<2>(0,3,0x1c8,true); a.store<2>(27,0x19e,0); a.jump(0x1023793c);
    } else {
        a.at(0x10237898); a.sign8(0,7); a.compare(0,11); bool goal=a.equal();
        if (goal) { a.sign8(0,6); a.compare(0,4); goal=a.equal(); }
        if (goal) { a.value(0,0); a.store<2>(27,0x19e,0); a.jump(0x1023793c); }
        else {
            a.at(0x102378bc); a.sign8(0,7,true); goal=a.equal();
            if (goal) { a.sign8(0,6); a.compare(0,4); goal=a.equal(); }
            if (goal) { a.value(0,180); a.store<2>(27,0x19e,0); a.jump(0x1023793c); }
            else {
                a.at(0x102378dc); advance_heading_stream(a);
                a.at(0x10237904); a.load(4,31,0); a.value(3,0xb60b0000); a.load(0,28,0); a.offset(3,3,0x60b7);
                a.load<2>(4,4,0); a.add(4,4,0); a.multiply_high(0,3,4); a.add(0,0,4); a.shift_right(0,0,8);
                a.mask(3,0,1,false,1); a.add(0,0,3); a.multiply(0,0,360); a.subtract(0,0,4); a.store<2>(27,0x19e,0);
            }
        }
    }
    apply_heading_jitter(a); apply_player_heading_bias(a);
    a.at(0x10237ae0); a.pop(0x60); a.multiple(25,true); a.finish();
}

void can_face_destination(Call& a) {
    a.link(); a.save(31); a.load(8,2,-0x51e8); a.save_link(); a.frame(0x50); a.load<1>(6,3,0x105); a.load<1>(0,3,0x106);
    a.subtract(4,6,4); a.sign8(4,4,true); a.subtract(0,0,5); a.sign8(5,0); bool displaced=!a.equal();
    if (!displaced) { a.sign8(0,5,true); displaced=!a.equal(); }
    if (!displaced) { a.value(3,0); a.jump(0x10245148); }
    else {
        a.at(0x10245090); a.sign8(0,4); a.sign8(4,5); a.multiply(6,0,48); a.load(7,2,-0x641c); a.load<2>(0,3,0x19e,true);
        a.mask(5,4,0xfffffffe,false,1); a.add(6,7,6); a.sign16(4,0); a.add(5,6,5); a.load<2>(6,5,0x1c8,true);
        a.sign16(0,6); a.compare_registers(4,0);
        if (a.equal()) { a.value(3,1); a.jump(0x10245148); }
        else {
            a.at(0x102450cc); a.load<1>(5,3,0x10f); a.value(0,0x43300000); a.store(1,0x40,0); a.sign16(4,6); a.sign8(5,5);
            a.load<2>(0,3,0x19e,true); a.exclusive_flags(3,5,0x80000000); a.floating_load(2,8,0x80); a.store(1,0x44,3);
            a.subtract(3,4,0); a.floating_load(3,8,0x1f8); a.floating_load(1,1,0x40); a.floating_load(0,8,0x338);
            a.floating_subtract(1,1,2); a.floating_product_sum(0,3,1,0); a.floating_integer(0,0); a.floating_store(0,1,0x38);
            a.load(31,1,0x3c); a.call(0x10775540); a.sign16(0,3); a.compare(0,180);
            if (!a.nonpositive()) { a.subtract_from(0,0,360); a.sign16(0,0); }
            a.at(0x1024512c); a.sign16(3,0); a.sign16(0,31); a.compare_registers(3,0);
            if (!a.at_least()) { a.value(3,1); a.jump(0x10245148); } else { a.at(0x10245144); a.value(3,0); }
        }
    }
    a.at(0x10245148); a.pop(0x50); a.restore(31); a.finish();
}
}

void invoke(State& state, Entry entry, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call call(state,entries[unsigned(entry)],dependencies,observer);
    switch (entry) {
    case Entry::set_coordinates: set_coordinates(call); break;
    case Entry::set_heading: set_heading(call); break;
    case Entry::heading: call.load<2>(3,3,0x19e,true); call.finish(); break;
    case Entry::can_face_destination: can_face_destination(call); break;
    case Entry::has_possession:
        call.load(4,3,0x1a4); call.offset(4,4,0x10000); call.load(0,4,-0x2000); call.subtract(0,3,0);
        call.leading_zeroes(0,0); call.mask(0,0,0x7ffffff,false,27); call.sign8(3,0); call.finish(); break;
    }
}
}
