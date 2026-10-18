#include "transfer_offer_detail.hpp"

namespace cm::aot::transfer_offer_queries {
using detail::Call;
void best_qualifying_offer(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,best_qualifying_offer_entry,d,o);a.link();a.multiple(26);a.move(27,3);a.move(28,5);a.value(30,0);a.value(29,0xffffffff);a.save_link();a.frame(80);
    a.load(6,4,0);a.load(3,3,0x9a0);a.multiply(0,6,10);a.add(3,3,0);a.load<1>(0,3,1);a.compare(0,0,false);bool stopped=false;
    if(a.equal()){a.value(3,0);a.jump(0x103f4390);stopped=true;}
    if(!stopped){a.at(0x103f4284);a.value(5,0);a.value(4,0);a.jump(0x103f4380);
        for(;;){a.at(0x103f4380);a.load(0,27,4);a.compare_registers(5,0);if(!a.less()){a.value(3,0);break;}
            a.at(0x103f4290);a.load(3,27,0);a.indexed_load(31,3,4);a.compare(31,0,false);bool matches=!a.equal();
            if(matches){a.load(0,31,12);a.compare_registers(0,6);matches=a.equal();}
            if(!matches){a.at(0x103f4378);a.offset(4,4,4);a.offset(5,5,1);continue;}
            a.at(0x103f42ac);a.load(3,31,0);a.compare(3,0,false);
            if(!a.equal()){a.load(0,3,4);a.store(3,12,0);a.jump(0x103f4324);
                for(;;){a.at(0x103f4324);a.load(3,31,0);a.call(0x10738e00);a.compare(3,0,false);if(a.equal())break;
                    a.at(0x103f42c4);a.load(0,3,0);a.load(4,27,0);a.mask(3,0,0xfffffffc,false,2);a.load(0,28,0);a.indexed_load(26,4,3);a.load(3,26,8);a.compare_registers(3,0);bool club=a.equal();
                    if(!club){a.load<1>(0,26,0x35);a.compare(0,1);club=a.equal();}
                    if(!club)continue;
                    a.at(0x103f42f0);a.load<1>(0,26,0x34);a.compare(0,2);if(!a.equal())continue;
                    a.move(3,26);a.value(4,0);a.value(5,0);a.value(6,0);a.value(7,1);a.call(0x10416710);a.compare_registers(3,29);if(a.nonpositive())continue;
                    a.move(30,26);a.move(29,3);
                }
                a.at(0x103f4334);a.move(3,30);a.jump(0x103f4390);
            }else{a.at(0x103f433c);a.load(3,31,8);a.load(0,28,0);a.compare_registers(3,0);bool club=a.equal();if(!club){a.load<1>(0,31,0x35);a.compare(0,1);club=a.equal();}
                bool qualifying=false;if(club){a.at(0x103f4358);a.load<1>(0,31,0x34);a.compare(0,2);qualifying=a.equal();}
                if(qualifying){a.move(3,31);a.jump(0x103f4390);}else{a.at(0x103f436c);a.value(3,0);a.jump(0x103f4390);}}
            break;
        }
    }
    a.at(0x103f4390);a.pop(80);a.multiple(26,true);a.finish();
}
namespace {
bool find_special_index(Call& a,std::uint32_t list_pc,std::uint32_t body_pc,std::uint32_t loop_pc) {
    a.at(list_pc);a.load(0,1,0x3c);a.store(1,0x44,0);a.jump(loop_pc);
    bool initial=true;
    for(;;){a.at(initial?loop_pc:loop_pc+4);if(initial){a.flags(0,0,0);initial=false;}a.offset(3,1,0x38);a.call(0x10738e00);a.compare(3,0,false);if(a.equal())return false;
        a.at(body_pc);a.load(4,3,0);a.load(3,28,0);a.mask(0,4,0xfffffffc,false,2);a.indexed_load(3,3,0);a.load<1>(0,3,0x34);a.compare(0,19);
        if(a.equal()){a.move(31,4);a.jump(0x103f4470);return true;}}
}
}
void query_special_staff_offer(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,query_special_staff_offer_entry,d,o);a.link();a.save(31);a.save(30);a.move(30,5);a.save(29);a.move(29,4);a.save(28);a.move(28,3);a.save_link();a.frame(96);a.offset(3,1,0x38);a.call(0x10738c00);a.load(0,29,0);a.value(31,0xffffffff);a.load(3,28,0x9a0);a.multiply(0,0,10);a.indexed_load<1>(0,3,0);a.mask(0,0,0x10,true);
    if(a.equal()){a.offset(3,1,0x38);a.value(4,0xffffffff);a.call(0x10738c20);a.value(3,0);a.jump(0x103f4574);}
    else {
        a.at(0x103f4410);a.move(3,28);a.move(4,29);a.offset(5,1,0x38);a.value(6,0);a.call(collect_staff_offer_indices_entry);a.compare(3,0);
        if(!a.equal())find_special_index(a,0x103f442c,0x103f4438,0x103f445c);
        a.at(0x103f4470);a.compare(31,-1);
        if(a.equal()){
            a.offset(3,1,0x38);a.call(0x10738ca0);a.move(3,28);a.move(4,29);a.offset(5,1,0x38);a.call(collect_club_staff_offer_indices_entry);a.compare(3,0);
            if(!a.equal()){
                // This search joins at 0x103f44e0 rather than the first search's join.
                a.load(0,1,0x3c);a.store(1,0x44,0);a.jump(0x103f44cc);
                bool initial=true;
                for(;;){a.at(initial?0x103f44cc:0x103f44d0);if(initial){a.flags(0,0,0);initial=false;}a.offset(3,1,0x38);a.call(0x10738e00);a.compare(3,0,false);if(a.equal())break;
                    a.at(0x103f44a8);a.load(4,3,0);a.load(3,28,0);a.mask(0,4,0xfffffffc,false,2);a.indexed_load(3,3,0);a.load<1>(0,3,0x34);a.compare(0,19);
                    if(a.equal()){a.move(31,4);a.jump(0x103f44e0);break;}}
            }
        }
        a.at(0x103f44e0);a.compare(31,-1);
        if(!a.equal()){
            a.compare(30,0,false);if(!a.equal()){
                a.value(5,0);a.value(3,0);a.jump(0x103f452c);
                for(;;){a.at(0x103f452c);a.load(0,28,0xa58);a.compare_registers(5,0);if(!a.less())break;
                    a.at(0x103f44fc);a.load(4,28,0xa50);a.offset(0,3,8);a.indexed_load(0,4,0);a.compare_registers(31,0);
                    if(a.equal()){a.multiply(0,5,12);a.add(0,4,0);a.store(30,0,0);a.jump(0x103f4538);break;}a.at(0x103f4524);a.offset(3,3,12);a.offset(5,5,1);}
            }
            a.at(0x103f4538);a.offset(3,1,0x38);a.value(4,0xffffffff);a.call(0x10738c20);a.value(3,1);a.jump(0x103f4574);
        }else{a.at(0x103f454c);a.load(0,29,0);a.offset(3,1,0x38);a.load(6,28,0x9a0);a.value(4,0xffffffff);a.multiply(5,0,10);a.indexed_load<1>(0,6,5);a.mask(0,0,0xffffffef);a.indexed_store<1>(0,6,5);a.call(0x10738c20);a.value(3,0);}
    }
    a.at(0x103f4574);a.pop(96);a.restore(31);a.restore(30);a.restore(29);a.restore(28);a.finish();
}
}
