#include "transfer_offer_detail.hpp"

namespace cm::aot::transfer_offer_queries {
using detail::Call;
void append_staff_entry(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,append_staff_entry_entry,d,o);a.link();a.multiple(26);a.load(7,2,-0x7b7c);a.move(26,3);a.load(30,2,-0x7ba8);a.move(27,4);a.load(31,2,-0x4b9c);a.move(28,5);a.move(29,6);a.save_link();a.frame(144);
    a.load(3,7,0);a.call(0x10083110);a.compare(3,0,false);
    if(!a.equal()){detail::diagnostic(a,0x103e62cc,31,30,0x4c,0x38,0xbe6);a.load(3,2,-0x7bac);a.value(0,0);a.store<1>(3,0,0);a.jump(0x103e6344);}
    else {a.at(0x103e6324);a.load(0,27,0);a.offset(3,26,0xa5c);a.offset(4,1,0x40);a.value(5,12);a.store(1,0x40,0);a.store<1>(1,0x44,28);a.store(1,0x48,29);a.call(0x10738d10);}
    a.at(0x103e6344);a.pop(144);a.multiple(26,true);a.finish();
}
void count_club_staff_entries(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,count_club_staff_entries_entry,d,o);a.link();a.multiple(26);a.move(27,4,true);a.load(29,2,-0x7be0);a.load(30,2,-0x7ba8);a.move(26,3);a.load(31,2,-0x4b9c);a.value(28,0);a.save_link();a.frame(128);
    if(a.equal()){detail::diagnostic(a,0x103e638c,31,30,0x40,0x38,0xc04);a.load(4,2,-0x7bac);a.value(0,0);a.value(3,0);a.store<1>(4,0,0);a.jump(0x103e6450);}
    else {
        a.at(0x103e63e8);a.move(3,27);a.value(4,0);a.value(5,1);a.call(0x100ce320);a.load(0,26,0xa60);a.move(31,3);a.store(26,0xa68,0);a.jump(0x103e643c);
        for(;;){a.at(0x103e643c);a.offset(3,26,0xa5c);a.call(0x10738e00);a.compare(3,0,false);if(a.equal())break;
            a.at(0x103e640c);a.load(0,3,0);a.load(4,29,0);a.multiply(3,0,0x70);a.offset(0,3,0x24);a.indexed_load(0,4,0);a.compare_registers(0,27,false);bool match=a.equal();
            if(!match){a.compare_registers(0,31,false);if(a.equal()){a.compare(31,0,false);match=!a.equal();}}
            if(match){a.at(0x103e6438);a.offset(28,28,1);}
        }
        a.at(0x103e644c);a.move(3,28);
    }
    a.at(0x103e6450);a.pop(128);a.multiple(26,true);a.finish();
}
void contains_staff_entry(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,contains_staff_entry_entry,d,o);a.link();a.save(31);a.move(31,4);a.save(30);a.move(30,3);a.save_link();a.frame(64);a.load(0,3,0xa60);a.store(3,0xa68,0);a.jump(0x103e64b0);
    for(;;){a.at(0x103e64b0);a.offset(3,30,0xa5c);a.call(0x10738e00);a.compare(3,0,false);if(a.equal()){a.value(3,0);break;}
        a.at(0x103e6498);a.load(4,31,0);a.load(0,3,0);a.compare_registers(4,0);if(a.equal()){a.value(3,1);a.jump(0x103e64c4);break;}}
    a.at(0x103e64c4);a.pop(64);a.restore(31);a.restore(30);a.finish();
}
void set_staff_contract_flag(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,set_staff_contract_flag_entry,d,o);a.load(0,4,0);a.load(4,3,0x9a0);a.multiply(3,0,10);a.indexed_load<1>(0,4,3);a.flags(0,0,0x40);a.indexed_store<1>(0,4,3);a.finish();
}
void clear_staff_contract_flag(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,clear_staff_contract_flag_entry,d,o);a.load(0,4,0);a.load(4,3,0x9a0);a.multiply(3,0,10);a.indexed_load<1>(0,4,3);a.mask(0,0,0xffffffbf);a.indexed_store<1>(0,4,3);a.finish();
}
void query_staff_contract_flag(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,query_staff_contract_flag_entry,d,o);a.link();a.save(31);a.move(31,5);a.save(30);a.move(30,4);a.save(29);a.move(29,3);a.save_link();a.frame(80);
    a.load(4,4,0);a.load(3,3,0x9a0);a.multiply(0,4,10);a.indexed_load<1>(0,3,0);a.mask(0,0,0x40,true);
    if(a.equal()){a.at(0x103ea328);a.value(3,0);}
    else {a.load(3,2,-0x7b7c);a.load(3,3,0);a.call(0x1007f7b0);a.compare(3,0,false);
        if(!a.equal()){a.compare(31,0,false);if(!a.equal()){a.load(0,3,4);a.load(3,2,-0x7be8);a.multiply(0,0,0x248);a.load(3,3,0);a.add(0,3,0);a.store(31,0,0);}a.at(0x103ea300);a.value(3,1);a.jump(0x103ea32c);}
        else {a.at(0x103ea308);a.load(0,30,0);a.value(3,0);a.load(5,29,0x9a0);a.multiply(4,0,10);a.indexed_load<1>(0,5,4);a.mask(0,0,0xffffffbf);a.indexed_store<1>(0,5,4);a.jump(0x103ea32c);}}
    a.at(0x103ea32c);a.pop(80);a.restore(31);a.restore(30);a.restore(29);a.finish();
}
void primary_offer_slot(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,primary_offer_slot_entry,d,o);a.load(0,3,4);a.compare_registers(4,0);if(!a.at_least()){a.load(3,3,0);a.mask(0,4,0xfffffffc,false,2);a.add(3,3,0);a.finish();}else{a.at(0x103eb45c);a.load(3,3,0);a.finish();}
}
void secondary_offer_slot(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,secondary_offer_slot_entry,d,o);a.load(0,3,0x10);a.compare_registers(4,0);if(!a.at_least()){a.load(3,3,0xc);a.mask(0,4,0xfffffffc,false,2);a.add(3,3,0);a.finish();}else{a.at(0x103eb48c);a.load(3,3,0xc);a.finish();}
}
void offer_is_filtered(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,offer_is_filtered_entry,d,o);a.load<1>(5,4,0x34);a.offset(0,5,-2);a.mask(0,0,255);a.compare(0,1,false);bool filtered=a.nonpositive();
    if(!filtered){a.sign8(3,5);a.compare(3,20);filtered=a.equal();if(!filtered){a.offset(0,5,-15);a.mask(0,0,255);a.compare(0,1,false);filtered=a.nonpositive();if(!filtered){a.compare(3,4);filtered=a.equal();}}}
    if(filtered){a.at(0x103fb3c8);a.sign8(0,5);a.compare(0,15);if(a.equal()){a.load<1>(0,4,0x36);a.compare(0,3,false);if(!a.at_least()){a.value(3,0);a.finish();return;}}a.at(0x103fb3e8);a.value(3,1);a.finish();}
    else{a.at(0x103fb3f0);a.value(3,0);a.finish();}
}
}
