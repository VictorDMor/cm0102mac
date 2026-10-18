#include "transfer_record_detail.hpp"

namespace cm::aot::transfer_records {
using detail::Call;
void read_offer(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,read_offer_entry,d,o);a.link();a.multiple(24);a.move(26,4);a.move(27,5);a.load(29,2,-0x7bac);a.move(25,3);a.load(30,2,-0x7ba8);a.move(6,26);a.load(31,2,-0x4b88);a.value(28,0);a.value(4,0x54);a.value(5,1);a.save_link();a.frame(384);a.call(0x10747a80);a.compare(3,1);
    const auto finish=[&]{a.at(0x104123c0);a.pop(384);a.multiple(24,true);a.finish();};
    const auto fail=[&](std::uint32_t pc,unsigned first,unsigned second,unsigned line){detail::diagnostic(a,pc,31,30,first,second,line);a.value(0,0);a.store<1>(29,0,0);a.jump(0x104123c0);finish();};
    if(!a.equal())return fail(0x10411fe4,0x140,0x78,0x69);
    a.at(0x10412038);a.move(6,26);a.offset(3,1,0x38);a.value(4,2);a.value(5,1);a.call(0x10747a80);a.compare(3,1);
    if(!a.equal())return fail(0x10412054,0x120,0x70,0x70);
    a.at(0x104120a8);a.load<2>(0,1,0x38,true);a.compare(0,0);
    if(!a.equal()){
        a.compare(27,0,false);if(a.equal())return fail(0x104120bc,0x100,0x68,0x7a);
        a.at(0x10412110);a.value(0,0);a.value(3,16);a.store(25,0,0);a.call(0x10773520);a.move(28,3,true);if(!a.equal())a.call(0x10738c00);
        a.at(0x1041212c);a.value(24,0);a.jump(0x1041223c);
        for(;;){a.at(0x1041223c);a.load<2>(0,1,0x38,true);a.sign16(3,24);a.compare_registers(3,0);if(!a.less())break;
            a.at(0x10412138);a.move(6,26);a.offset(3,1,0x3c);a.value(4,4);a.value(5,1);a.call(0x10747a80);a.compare(3,1);
            if(!a.equal())return fail(0x10412154,0xe0,0x60,0x8d);
            a.at(0x104121ac);a.load(3,1,0x3c);a.load(0,25,0x28);a.compare_registers(3,0);
            if(!a.at_least()){
                a.mask(0,3,0xfffffffc,false,2);a.indexed_load(3,27,0);a.compare(3,0,false);
                if(a.equal()){detail::diagnostic(a,0x104121cc,31,30,0xc0,0x58,0x99);a.value(0,0);a.store<1>(29,0,0);a.jump(0x10412228);}
                else{a.at(0x10412220);a.load(0,3,0);a.store(25,0,0);}
            }
            a.at(0x10412228);a.move(3,28);a.offset(4,1,0x3c);a.value(5,4);a.call(0x10738d10);a.offset(24,24,1);
        }
        a.at(0x1041224c);a.load(0,25,0);a.compare(0,0,false);
        if(a.equal()){a.store(25,0,28);a.value(28,0);a.jump(0x1041226c);}else{a.at(0x10412264);a.move(3,28);a.call(0x10738ca0);}
    }
    a.at(0x1041226c);a.compare(28,0,false);if(!a.equal()){a.move(3,28);a.value(4,1);a.call(0x10738c20);}
    a.at(0x10412280);a.move(6,26);a.offset(3,1,0x38);a.value(4,2);a.value(5,1);a.call(0x10747a80);a.compare(3,1);
    if(!a.equal())return fail(0x1041229c,0xa0,0x50,0xbe);
    a.at(0x104122f0);a.load<2>(0,1,0x38,true);a.compare(0,0);
    if(!a.equal()){
        a.value(3,16);a.call(0x10773520);a.move(24,3,true);if(!a.equal())a.call(0x10738c00);
        a.at(0x10412310);a.store(25,4,24);a.value(27,0);a.jump(0x104123a4);
        for(;;){a.at(0x104123a4);a.load<2>(0,1,0x38,true);a.sign16(3,27);a.compare_registers(3,0);if(!a.less())break;
            a.at(0x1041231c);a.move(6,26);a.offset(3,1,0x48);a.value(4,8);a.value(5,1);a.call(0x10747a80);a.compare(3,1);
            if(!a.equal())return fail(0x10412338,0x80,0x40,0xd0);
            a.at(0x10412390);a.load(3,25,4);a.offset(4,1,0x48);a.value(5,8);a.call(0x10738d10);a.offset(27,27,1);
        }
        a.at(0x104123b4);a.jump(0x104123c0);
    }else{a.at(0x104123b8);a.value(0,0);a.store(25,4,0);}
    finish();
}
void write_offer(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,write_offer_entry,d,o);a.link();a.multiple(27);a.move(28,4);a.load(31,2,-0x7bac);a.load(29,2,-0x7ba8);a.move(27,3);a.load(30,2,-0x4b88);a.move(6,28);a.value(4,0x54);a.value(5,1);a.save_link();a.frame(288);a.call(0x10747cc0);a.compare(3,1);
    const auto finish=[&]{a.at(0x104126b8);a.pop(288);a.multiple(27,true);a.finish();};
    const auto fail=[&](std::uint32_t pc,unsigned first,unsigned second,unsigned line){detail::diagnostic(a,pc,30,29,first,second,line);a.value(0,0);a.store<1>(31,0,0);a.jump(0x104126b8);finish();};
    if(!a.equal())return fail(0x1041241c,0xe4,0x5c,0xee);
    a.at(0x10412470);a.load(3,27,0);a.compare(3,0,false);if(!a.equal()){a.load<2>(0,3,0,true);a.jump(0x10412488);}else{a.at(0x10412484);a.value(0,0);}
    a.at(0x10412488);a.store<2>(1,0x38,0);a.move(6,28);a.offset(3,1,0x38);a.value(4,2);a.value(5,1);a.call(0x10747cc0);a.compare(3,1);
    if(!a.equal())return fail(0x104124a8,0xc4,0x54,0x102);
    a.at(0x104124fc);a.load(3,27,0);a.compare(3,0,false);
    if(!a.equal()){
        a.load(0,3,4);a.store(3,12,0);a.jump(0x10412584);
        for(;;){a.at(0x10412584);a.load(3,27,0);a.call(0x10738e00);a.compare(3,0,false);if(a.equal())break;
            a.at(0x10412518);a.move(6,28);a.value(4,4);a.value(5,1);a.call(0x10747cc0);a.compare(3,1);if(!a.equal())return fail(0x10412530,0xa4,0x4c,0x110);
        }
    }
    a.at(0x10412594);a.load(3,27,4);a.compare(3,0,false);if(!a.equal()){a.load<2>(0,3,0,true);a.jump(0x104125ac);}else{a.at(0x104125a8);a.value(0,0);}
    a.at(0x104125ac);a.store<2>(1,0x38,0);a.move(6,28);a.offset(3,1,0x38);a.value(4,2);a.value(5,1);a.call(0x10747cc0);a.compare(3,1);if(!a.equal())return fail(0x104125cc,0x84,0x44,0x125);
    a.at(0x10412620);a.load(3,27,4);a.compare(3,0,false);
    if(!a.equal()){
        a.load(0,3,4);a.store(3,12,0);a.jump(0x104126a8);
        for(;;){a.at(0x104126a8);a.load(3,27,4);a.call(0x10738e00);a.compare(3,0,false);if(a.equal())break;
            a.at(0x1041263c);a.move(6,28);a.value(4,8);a.value(5,1);a.call(0x10747cc0);a.compare(3,1);if(!a.equal())return fail(0x10412654,0x64,0x3c,0x133);
        }
    }
    finish();
}
}
