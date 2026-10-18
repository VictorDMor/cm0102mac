#include "transfer_offer_detail.hpp"

namespace cm::aot::transfer_offer_queries {
using detail::Call;
void collect_staff_offer_indices(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,collect_staff_offer_indices_entry,d,o);a.link();a.multiple(25);a.compare(4,0,false);a.move(28,3);a.move(29,5);a.move(30,6);a.value(31,0);a.save_link();a.frame(96);a.load(27,4,0);bool stopped=false;
    if(!a.equal()){a.load(3,2,-0x7be4);a.load(0,3,0);a.compare_registers(27,0);if(!a.less()){a.value(3,0);a.jump(0x103eb000);stopped=true;}}
    if(!stopped){a.at(0x103eaee4);a.load(4,28,0);a.value(5,0);a.jump(0x103eaf18);
        for(;;){a.at(0x103eaf18);a.load(0,28,4);a.compare_registers(5,0);if(!a.less())break;
            a.at(0x103eaef0);a.load(3,4,0);a.compare(3,0,false);if(!a.equal()){a.load(0,3,12);a.compare_registers(0,27);if(a.equal()){a.move(31,3);a.jump(0x103eaf24);break;}}
            a.at(0x103eaf10);a.offset(5,5,1);a.offset(4,4,4);}
        a.at(0x103eaf24);a.compare(31,0,false);
        if(a.equal()){a.value(3,0);a.jump(0x103eb000);}
        else {a.at(0x103eaf34);a.load(3,31,0);a.compare(3,0,false);
            if(!a.equal()){a.load(0,3,4);a.value(25,0);a.store(3,12,0);a.jump(0x103eafa8);
                for(;;){a.at(0x103eafa8);a.load(3,31,0);a.call(0x10738e00);a.move(26,3,true);if(a.equal())break;
                    a.at(0x103eaf50);a.load(0,26,0);a.load(3,28,0);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(4,3,0);a.compare(4,0,false);if(a.equal())continue;
                    a.load(0,4,12);a.compare_registers(27,0);if(!a.equal())continue;
                    a.sign8(0,30,true);if(a.equal()){a.move(3,28);a.call(offer_is_filtered_entry);a.sign8(0,3,true);if(!a.equal())continue;}
                    a.at(0x103eaf8c);a.compare(29,0,false);if(!a.equal()){a.move(3,29);a.move(4,26);a.value(5,4);a.call(0x10738d10);}a.at(0x103eafa4);a.offset(25,25,1);
                }
                a.at(0x103eafb8);a.move(3,25);a.jump(0x103eb000);
            }else{a.at(0x103eafc0);a.sign8(0,30,true);bool reject=false;if(a.equal()){a.move(3,28);a.move(4,31);a.call(offer_is_filtered_entry);a.sign8(0,3,true);reject=!a.equal();}
                if(reject){a.at(0x103eaffc);a.value(3,0);}else{a.at(0x103eafdc);a.compare(29,0,false);if(!a.equal()){a.move(3,29);a.offset(4,31,0x28);a.value(5,4);a.call(0x10738d10);}a.at(0x103eaff4);a.value(3,1);a.jump(0x103eb000);}}
        }
    }
    a.at(0x103eb000);a.pop(96);a.multiple(25,true);a.finish();
}
void collect_club_staff_offer_indices(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,collect_club_staff_offer_indices_entry,d,o);a.link();a.multiple(25);a.move(26,4,true);a.load(31,2,-0x7b7c);a.load(30,2,-0x7b78);a.move(25,3);a.load(28,2,-0x7be8);a.move(27,5);a.value(29,0);a.save_link();a.frame(96);bool stopped=false;
    if(!a.equal()){a.load(3,2,-0x7be4);a.load(4,26,0);a.load(0,3,0);a.compare_registers(4,0);if(!a.less()){a.value(3,0);a.jump(0x103eb1d0);stopped=true;}}
    if(!stopped){a.at(0x103eb06c);a.load(0,26,0);a.load(3,25,0x9a0);a.multiply(0,0,10);a.add(3,3,0);a.load<1>(0,3,2);a.compare(0,0,false);
        if(a.equal()){a.value(3,0);a.jump(0x103eb1d0);stopped=true;}
    }
    if(!stopped){a.at(0x103eb090);a.load(3,31,0);a.move(4,26);a.call(0x10082f20);a.compare(3,0,false);a.store(30,0,3);
        if(!a.equal()){a.load(0,3,4);a.load(3,28,0);a.multiply(0,0,0x248);a.add(0,3,0);a.jump(0x103eb0c0);}else{a.at(0x103eb0bc);a.value(0,0);}
        a.at(0x103eb0c0);a.compare(0,0,false);if(a.equal()){a.value(3,0);a.jump(0x103eb1d0);stopped=true;}
    }
    if(!stopped){a.at(0x103eb0d0);a.load(3,31,0);a.move(4,26);a.call(0x10082f20);a.compare(3,0,false);a.store(30,0,3);
        if(!a.equal()){a.load(0,3,4);a.load(3,28,0);a.multiply(0,0,0x248);a.add(3,3,0);a.jump(0x103eb100);}else{a.at(0x103eb0fc);a.value(3,0);}
        a.at(0x103eb100);a.load(28,3,0);a.value(30,0);a.load(31,25,0);a.jump(0x103eb1c0);
        for(;;){a.at(0x103eb1c0);a.load(0,25,4);a.compare_registers(30,0);if(!a.less())break;
            a.at(0x103eb110);a.load(4,31,0);a.compare(4,0,false);bool candidate=!a.equal();
            if(candidate){a.load(0,4,8);a.compare_registers(0,28);candidate=a.equal();}
            if(candidate){a.move(3,25);a.call(offer_is_filtered_entry);a.sign8(0,3,true);candidate=a.equal();}
            bool enough=false;
            if(candidate){a.load(3,31,0);a.load(3,3,4);a.compare(3,0,false);if(!a.equal()){
                    a.load(0,3,4);a.store(3,12,0);a.jump(0x103eb188);
                    for(;;){a.at(0x103eb188);a.load(3,31,0);a.load(3,3,4);a.call(0x10738e00);a.compare(3,0,false);if(a.equal())break;
                        a.at(0x103eb154);a.load(3,3,0);a.load(0,26,0);a.compare_registers(3,0);if(!a.equal())continue;
                        a.compare(27,0,false);if(!a.equal()){a.load(4,31,0);a.move(3,27);a.value(5,4);a.offset(4,4,0x28);a.call(0x10738d10);}a.at(0x103eb180);a.offset(29,29,1);a.jump(0x103eb19c);break;
                    }
                }
                a.at(0x103eb19c);a.load(0,26,0);a.load(4,25,0x9a0);a.multiply(3,0,10);a.offset(0,3,2);a.indexed_load<1>(0,4,0);a.compare_registers(29,0);enough=a.equal();
            }
            if(enough)break;a.at(0x103eb1b8);a.offset(30,30,1);a.offset(31,31,4);
        }
        a.at(0x103eb1cc);a.move(3,29);
    }
    a.at(0x103eb1d0);a.pop(96);a.multiple(25,true);a.finish();
}
void collect_secondary_staff_offer_indices(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,collect_secondary_staff_offer_indices_entry,d,o);a.link();a.multiple(24);a.move(29,4,true);a.load(27,2,-0x7b78);a.move(28,3);a.move(30,5);a.value(31,0);a.save_link();a.frame(96);bool stopped=false;
    if(!a.equal()){a.load(3,2,-0x7be4);a.load(4,29,0);a.load(0,3,0);a.compare_registers(4,0);if(!a.less()){a.value(3,0);a.jump(0x103eb344);stopped=true;}}
    if(!stopped){a.at(0x103eb234);a.load(3,2,-0x7b7c);a.move(4,29);a.load(3,3,0);a.call(0x10082f20);a.compare(3,0,false);a.store(27,0,3);
        if(!a.equal()){a.load(0,3,4);a.load(3,2,-0x7be8);a.multiply(0,0,0x248);a.load(3,3,0);a.add(3,3,0);a.jump(0x103eb26c);}else{a.at(0x103eb268);a.value(3,0);}
        a.at(0x103eb26c);a.compare(3,0,false);if(a.equal()){a.value(3,0);a.jump(0x103eb344);stopped=true;}
    }
    if(!stopped){a.at(0x103eb27c);a.load(24,3,0);a.value(25,0);a.load(26,28,12);a.jump(0x103eb334);
        for(;;){a.at(0x103eb334);a.load(0,28,16);a.compare_registers(25,0);if(!a.less())break;
            a.at(0x103eb28c);a.load(27,26,0);a.compare(27,0,false);bool candidate=!a.equal();
            if(candidate){a.load(0,27,8);a.compare_registers(0,24);candidate=a.equal();}
            if(candidate){a.load(0,27,0x28);a.load(3,28,0);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(4,3,0);a.compare(4,0,false);candidate=!a.equal();}
            if(candidate){a.move(3,28);a.call(offer_is_filtered_entry);a.sign8(0,3,true);candidate=a.equal();}
            if(candidate){a.load(3,27,4);a.compare(3,0,false);if(!a.equal()){
                    a.load(0,3,4);a.store(3,12,0);a.jump(0x103eb318);
                    bool initial=true;
                    for(;;){a.at(initial?0x103eb318:0x103eb31c);if(initial){a.flags(0,0,0);initial=false;}a.load(3,27,4);a.call(0x10738e00);a.compare(3,0,false);if(a.equal())break;
                        a.at(0x103eb2e8);a.load(3,3,0);a.load(0,29,0);a.compare_registers(3,0);if(!a.equal())continue;
                        a.compare(30,0,false);if(!a.equal()){a.move(3,30);a.offset(4,27,0x28);a.value(5,4);a.call(0x10738d10);}a.at(0x103eb310);a.offset(31,31,1);a.jump(0x103eb32c);break;
                    }
                }}
            a.at(0x103eb32c);a.offset(25,25,1);a.offset(26,26,4);
        }
        a.at(0x103eb340);a.move(3,31);
    }
    a.at(0x103eb344);a.pop(96);a.multiple(24,true);a.finish();
}
void collect_club_offer_indices(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,collect_club_offer_indices_entry,d,o);a.link();a.save(31);a.save(30);a.value(30,0);a.save(29);a.move(29,5);a.save(28);a.move(28,3);a.save_link();a.value(0,0);a.frame(80);a.load(31,4,0);a.store(1,0x38,0);a.jump(0x103eb400);
    for(;;){a.at(0x103eb400);a.load(4,1,0x38);a.load(0,28,4);a.compare_registers(4,0);if(!a.less())break;
        a.at(0x103eb398);a.load(3,28,0);a.mask(0,4,0xfffffffc,false,2);a.indexed_load(4,3,0);a.compare(4,0,false);bool candidate=!a.equal();
        if(candidate){a.move(3,28);a.call(offer_is_filtered_entry);a.sign8(0,3,true);candidate=a.equal();}
        if(candidate){a.load(0,1,0x38);a.load(3,28,0);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(3,3,0);a.load(0,3,8);a.compare_registers(31,0);candidate=a.equal();}
        if(candidate){a.compare(29,0,false);if(!a.equal()){a.move(3,29);a.offset(4,1,0x38);a.value(5,4);a.call(0x10738d10);}a.at(0x103eb3f0);a.offset(30,30,1);}
        a.at(0x103eb3f4);a.load(3,1,0x38);a.offset(0,3,1);a.store(1,0x38,0);
    }
    a.at(0x103eb410);a.move(3,30);a.pop(80);a.restore(31);a.restore(30);a.restore(29);a.restore(28);a.finish();
}
}
