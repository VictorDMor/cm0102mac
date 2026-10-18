#include "contract_queries.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::contract_queries {
namespace {
using Call = SemanticGuestCall;

void select_club_contract(Call& a) {
    bool missing = a.equal();
    if (!missing) { a.compare(31, 0, false); missing = a.equal(); }
    if (missing) { a.at(0x10082fc0); a.value(3, 0); a.jump(0x100830ec); return; }

    a.at(0x10082fc8); a.compare(4, 0, false);
    bool valid = a.equal();
    if (!valid) { a.load(5, 4, 0); a.load(0, 6, 0); a.compare_registers(5, 0); valid = a.less(); }
    if (!valid) { a.value(3, 0); a.jump(0x100830ec); return; }

    // Preserve the second live validation and lookup, including its null and
    // signed-index paths. The original does not cache the first staff ID read.
    a.at(0x10082fe8); a.compare(4, 0, false);
    if (a.equal()) { a.value(30, 0); a.jump(0x10083044); }
    else {
        a.at(0x10082ff8); valid = a.equal();
        if (!valid) { a.load(5, 4, 0); a.load(0, 6, 0); a.compare_registers(5, 0); valid = a.less(); }
        if (!valid) { a.value(30, 0); a.jump(0x10083044); }
        else {
            a.at(0x10083014); a.load(0, 4, 0); a.load(5, 2, -0x7b9c);
            a.multiply(0, 0, 100); a.load(5, 5, 0); a.indexed_load(0, 5, 0); a.compare(0, 0);
            if (!a.at_least()) { a.value(30, 0); a.jump(0x10083044); }
            else { a.at(0x10083038); a.multiply(0, 0, 100); a.load(5, 3, 0); a.add(30, 5, 0); }
        }
    }
    a.at(0x10083044); a.compare(30, 0, false);
    if (a.equal()) { a.value(3, 0); a.jump(0x100830ec); return; }

    a.at(0x10083054); a.load(5, 30, 4); a.load(0, 31, 0); a.compare_registers(5, 0);
    if (a.equal()) { a.move(3, 30); a.jump(0x100830ec); return; }

    a.at(0x1008306c); a.call(secondary_entry); a.move(29, 3, true);
    if (!a.equal()) {
        a.load(4, 29, 4); a.load(0, 31, 0); a.compare_registers(4, 0);
        if (a.equal()) { a.jump(0x100830ec); return; }
    }

    // The club resolver's byte output controls whether its returned club may
    // replace the requested one. Its register and memory effects stay live.
    a.at(0x1008308c); a.move(3, 31); a.offset(4, 1, 0x38); a.value(5, 1); a.call(0x100ce320);
    a.compare(3, 0, false);
    if (!a.equal()) {
        a.load<1>(0, 1, 0x38); a.sign8(0, 0, true);
        if (a.equal()) a.move(31, 3);
    }
    a.at(0x100830b4); a.load(0, 30, 4); a.load(3, 31, 0); a.compare_registers(0, 3);
    if (a.equal()) { a.move(3, 30); a.jump(0x100830ec); return; }
    a.at(0x100830cc); a.compare(29, 0, false);
    if (!a.equal()) {
        a.load(0, 29, 4); a.compare_registers(0, 3);
        if (a.equal()) { a.move(3, 29); a.jump(0x100830ec); return; }
    }
    a.at(0x100830e8); a.value(3, 0);
}
}

void find_primary(State& s, SemanticDependencyDispatch* d, SemanticGuestObserver* o) {
    Call a(s, primary_entry, d, o); a.compare(4, 0, false);
    if (a.equal()) { a.value(3, 0); a.finish(); return; }
    a.at(0x10082f30); bool valid = a.equal();
    if (!valid) { a.load(5, 2, -0x7be4); a.load(6, 4, 0); a.load(0, 5, 0); a.compare_registers(6, 0); valid = a.less(); }
    if (!valid) { a.value(3, 0); a.finish(); return; }
    a.at(0x10082f50); a.load(0, 4, 0); a.load(4, 2, -0x7b9c); a.multiply(0, 0, 100);
    a.load(4, 4, 0); a.indexed_load(0, 4, 0); a.compare(0, 0);
    if (!a.at_least()) { a.value(3, 0); a.finish(); return; }
    a.at(0x10082f74); a.multiply(0, 0, 100); a.load(3, 3, 0); a.add(3, 3, 0); a.finish();
}

void find_for_club(State& s, SemanticDependencyDispatch* d, SemanticGuestObserver* o) {
    Call a(s, club_entry, d, o); a.link(); a.save(31); a.compare(4, 0, false); a.load(6, 2, -0x7be4);
    a.save(30); a.move(31, 5); a.save(29); a.save_link(); a.frame(80);
    select_club_contract(a);
    a.at(0x100830ec); a.pop(80); a.restore(31); a.restore(30); a.restore(29); a.finish();
}

void find_secondary(State& s, SemanticDependencyDispatch* d, SemanticGuestObserver* o) {
    Call a(s, secondary_entry, d, o); a.compare(4, 0, false);
    if (a.equal()) { a.value(3, 0); a.finish(); return; }
    a.at(0x10083120); bool valid = a.equal();
    if (!valid) { a.load(5, 2, -0x7be4); a.load(6, 4, 0); a.load(0, 5, 0); a.compare_registers(6, 0); valid = a.less(); }
    if (!valid) { a.value(3, 0); a.finish(); return; }
    a.at(0x10083140); a.load(0, 4, 0); a.load(4, 2, -0x7b9c); a.multiply(0, 0, 100);
    a.load(4, 4, 0); a.add(4, 4, 0); a.load(0, 4, 4); a.compare(0, 0);
    if (!a.at_least()) { a.value(3, 0); a.finish(); return; }
    a.at(0x10083168); a.multiply(0, 0, 100); a.load(3, 3, 0); a.add(3, 3, 0); a.finish();
}
}
