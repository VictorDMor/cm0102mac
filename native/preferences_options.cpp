#include "preferences.hpp"
#include "semantic_guest_call.hpp"
namespace cm::aot::preferences {
namespace {
using Call=SemanticGuestCall;
void select_display_group(Call& a){
    a.at(0x1013f870);
    a.load(3,28,0x4);a.value(0,0xffffffe7);a.intersect(0,3,0);a.store(28,0x4,0);a.load(0,28,0x4);a.combine(0,0,4);
    a.store(28,0x4,0);a.jump(0x1013fb34);
}
void select_audio_feature_group(Call& a){
    a.at(0x1013f890);
    a.load(3,28,0x4);a.value(0,0xffffff9f);a.intersect(0,3,0);a.store(28,0x4,0);a.load(0,28,0x4);a.combine(0,0,4);
    a.store(28,0x4,0);a.jump(0x1013fb34);
}
void select_audio_device_group(Call& a){
    a.at(0x1013f8b0);
    a.load(3,28,0x4);a.value(0,0xfffffe7f);a.intersect(0,3,0);a.store(28,0x4,0);a.load(0,28,0x4);a.combine(0,0,4);
    a.store(28,0x4,0);a.load(3,28,0x4);a.mask(0,3,0x80,true,0);
    if(a.equal()){
        a.at(0x1013f970);
        a.mask(0,3,0x100,true,0);
        if(a.equal()){
        }else{
            a.at(0x1013f978);
            a.load(3,29,0x0);a.compare(3,0,false);
            if(a.equal()){
            }else{
                a.at(0x1013f984);
                a.call(0x10744700);a.load(3,29,0x0);a.value(4,0x1);a.call(0x107446b0);a.value(0,0x0);
                a.store(29,0x0,0);a.jump(0x1013fb34);
            }
        }
    }else{
        a.at(0x1013f8d8);
        a.load(0,29,0x0);a.compare(0,0,false);
        if(!a.equal()){
            a.at(0x1013f970);
            a.mask(0,3,0x100,true,0);
            if(a.equal()){
            }else{
                a.at(0x1013f978);
                a.load(3,29,0x0);a.compare(3,0,false);
                if(a.equal()){
                }else{
                    a.at(0x1013f984);
                    a.call(0x10744700);a.load(3,29,0x0);a.value(4,0x1);a.call(0x107446b0);a.value(0,0x0);
                    a.store(29,0x0,0);a.jump(0x1013fb34);
                }
            }
        }else{
            a.at(0x1013f8e4);
            a.value(3,0x2c);a.call(0x10773520);a.move(30,3,true);
            if(a.equal()){
            }else{
                a.at(0x1013f8f4);
                a.call(0x10744670);
            }
            a.at(0x1013f8f8);
            a.compare(30,0,false);a.store(29,0x0,30);
            if(a.equal()){
                a.at(0x1013f910);
                a.offset(3,31,0x33);a.offset(4,31,0x94);a.value(5,0x0);a.call(0x10769b60);a.move(3,28);
                a.value(4,0x100);a.call(0x1013f680);a.load(3,29,0x0);a.compare(3,0,false);
                if(a.equal()){
                }else{
                    a.at(0x1013f938);
                    a.value(4,0x1);a.call(0x107446b0);a.value(0,0x0);a.store(29,0x0,0);a.jump(0x1013fb34);
                }
            }else{
                a.at(0x1013f904);
                a.load<1>(0,30,0x28);a.sign8(0,0,true);
                if(!a.equal()){
                    a.at(0x1013f94c);
                    a.offset(3,31,0xed);a.offset(4,31,0xf8);a.call(0x10746280);a.move(5,3);a.load(3,29,0x0);
                    a.value(4,0x2);a.value(6,0x0);a.call(0x10744860);a.jump(0x1013fb34);
                }else{
                    a.at(0x1013f910);
                    a.offset(3,31,0x33);a.offset(4,31,0x94);a.value(5,0x0);a.call(0x10769b60);a.move(3,28);
                    a.value(4,0x100);a.call(0x1013f680);a.load(3,29,0x0);a.compare(3,0,false);
                    if(a.equal()){
                    }else{
                        a.at(0x1013f938);
                        a.value(4,0x1);a.call(0x107446b0);a.value(0,0x0);a.store(29,0x0,0);a.jump(0x1013fb34);
                    }
                }
            }
        }
    }
}
void select_low_triple_group(Call& a){
    a.at(0x1013f9a0);
    a.load(3,28,0x4);a.value(0,0xfffffff8);a.intersect(0,3,0);a.store(28,0x4,0);a.load(0,28,0x4);a.combine(0,0,4);
    a.store(28,0x4,0);a.jump(0x1013fb34);
}
void select_three_way_group(Call& a){
    a.at(0x1013f9c0);
    a.load(3,28,0x4);a.value(0,0xfffff1ff);a.intersect(0,3,0);a.store(28,0x4,0);a.load(0,28,0x4);a.combine(0,0,4);
    a.store(28,0x4,0);a.jump(0x1013fb34);
}
void select_two_way_group(Call& a){
    a.at(0x1013f9e0);
    a.load(3,28,0x4);a.value(0,0xffffcfff);a.intersect(0,3,0);a.store(28,0x4,0);a.load(0,28,0x4);a.combine(0,0,4);
    a.store(28,0x4,0);a.jump(0x1013fb34);
}
void select_high_pair_group(Call& a){
    a.at(0x1013fa00);
    a.value(3,0xffff0000);a.load(5,28,0x4);a.offset(0,3,0x3fff);a.intersect(0,5,0);a.store(28,0x4,0);
    a.load(0,28,0x4);a.combine(0,0,4);a.store(28,0x4,0);a.jump(0x1013fb34);
}
void select_top_pair_group(Call& a){
    a.at(0x1013fa24);
    a.value(3,0xa0000000);a.load(5,28,0x4);a.offset(0,3,-0x1);a.intersect(0,5,0);a.store(28,0x4,0);a.load(0,28,0x4);
    a.combine(0,0,4);a.store(28,0x4,0);a.jump(0x1013fb34);
}
void select_platform_pair_group(Call& a){
    a.at(0x1013fa48);
    a.value(3,0xfffd0000);a.load(5,28,0x4);a.offset(0,3,-0x1);a.intersect(0,5,0);a.store(28,0x4,0);a.load(0,28,0x4);
    a.combine(0,0,4);a.store(28,0x4,0);a.jump(0x1013fb34);
}
void select_platform_triple_group(Call& a){
    a.at(0x1013fa6c);
    a.value(3,0xffe40000);a.load(5,28,0x4);a.offset(0,3,-0x1);a.intersect(0,5,0);a.store(28,0x4,0);a.load(0,28,0x4);
    a.combine(0,0,4);a.store(28,0x4,0);a.jump(0x1013fb34);
}
void select_upper_triple_group(Call& a){
    a.at(0x1013fa90);
    a.value(3,0xff200000);a.load(5,28,0x4);a.offset(0,3,-0x1);a.intersect(0,5,0);a.store(28,0x4,0);a.load(0,28,0x4);
    a.combine(0,0,4);a.store(28,0x4,0);a.jump(0x1013fb34);
}
void select_capability_group(Call& a){
    a.at(0x1013fab4);
    a.value(3,0xf9000000);a.load(5,28,0x4);a.offset(0,3,-0x1);a.intersect(0,5,0);a.store(28,0x4,0);a.load(0,28,0x4);
    a.combine(0,0,4);a.store(28,0x4,0);a.jump(0x1013fb34);
}
void reject_option(Call& a){
    a.at(0x1013fad8);
    a.offset(3,31,0x16);a.offset(6,1,0x40);a.offset(7,1,0x38);a.value(4,0x0);a.value(5,0x0);a.call(0x10739890);
    a.call(0x10768810);a.move(5,3);a.move(3,30);a.offset(4,31,0x26);a.offset(6,1,0x40);a.offset(7,1,0x38);
    a.value(8,0x1f9);a.call(0x1077b720);a.move(4,30);a.offset(3,31,0x33);a.value(5,0x0);a.call(0x10769b60);
    a.load(4,2,-0x7bac);a.value(0,0x0);a.value(3,0x0);a.store<1>(4,0x0,0);a.jump(0x1013fb38);
}
// Preserve the signed comparison tree; option validation is not a bit-membership test.
bool choose_option(Call& a){
    a.at(0x1013f680);
    a.link();a.store(1,-0x4,31);a.value(5,0x10000);a.load(31,2,-0x54e8);a.store(1,-0x8,30);a.load(30,2,-0x7ba8);
    a.store(1,-0xc,29);a.load(29,2,-0x7468);a.store(1,-0x10,28);a.move(28,3);a.store(1,0x8,0);a.offset(0,5,-0x8000);
    a.compare_registers(4,0);a.frame(0x70);
    if(a.equal()){
        select_high_pair_group(a);return true;
    }
    a.at(0x1013f6bc);
    if(a.at_least()){
        a.at(0x1013f770);
        a.value(3,0x400000);a.offset(0,3,0x0);a.compare_registers(4,0);
        if(a.equal()){
            select_upper_triple_group(a);return true;
        }
        a.at(0x1013f780);
        if(a.at_least()){
            a.at(0x1013f7f8);
            a.value(3,0x4000000);a.offset(0,3,0x0);a.compare_registers(4,0);
            if(a.equal()){
                select_capability_group(a);return true;
            }
            a.at(0x1013f808);
            if(a.at_least()){
                a.at(0x1013f848);
                a.value(3,0x40000000);a.offset(0,3,0x0);a.compare_registers(4,0);
                if(a.equal()){
                    select_top_pair_group(a);return true;
                }
                a.at(0x1013f858);
                if(a.at_least()){
                    reject_option(a);return false;
                }
                a.at(0x1013f85c);
                a.value(3,0x20000000);a.offset(0,3,0x0);a.compare_registers(4,0);
                if(a.equal()){
                    select_top_pair_group(a);return true;
                }
                a.at(0x1013f86c);
                a.jump(0x1013fad8);
                reject_option(a);return false;
            }
            a.at(0x1013f80c);
            a.value(3,0x1000000);a.offset(0,3,0x0);a.compare_registers(4,0);
            if(a.equal()){
                select_capability_group(a);return true;
            }
            a.at(0x1013f81c);
            if(a.at_least()){
                a.at(0x1013f834);
                a.value(3,0x2000000);a.offset(0,3,0x0);a.compare_registers(4,0);
                if(a.equal()){
                    select_capability_group(a);return true;
                }
                a.at(0x1013f844);
                a.jump(0x1013fad8);
                reject_option(a);return false;
            }
            a.at(0x1013f820);
            a.value(3,0x800000);a.offset(0,3,0x0);a.compare_registers(4,0);
            if(a.equal()){
                select_upper_triple_group(a);return true;
            }
            a.at(0x1013f830);
            a.jump(0x1013fad8);
            reject_option(a);return false;
        }
        a.at(0x1013f784);
        a.value(3,0x80000);a.offset(0,3,0x0);a.compare_registers(4,0);
        if(a.equal()){
            select_platform_triple_group(a);return true;
        }
        a.at(0x1013f794);
        if(a.at_least()){
            a.at(0x1013f7d0);
            a.value(3,0x200000);a.offset(0,3,0x0);a.compare_registers(4,0);
            if(a.equal()){
                select_upper_triple_group(a);return true;
            }
            a.at(0x1013f7e0);
            if(a.at_least()){
                reject_option(a);return false;
            }
            a.at(0x1013f7e4);
            a.value(3,0x100000);a.offset(0,3,0x0);a.compare_registers(4,0);
            if(a.equal()){
                select_platform_triple_group(a);return true;
            }
            a.at(0x1013f7f4);
            a.jump(0x1013fad8);
            reject_option(a);return false;
        }
        a.at(0x1013f798);
        a.value(3,0x20000);a.offset(0,3,0x0);a.compare_registers(4,0);
        if(a.equal()){
            select_platform_pair_group(a);return true;
        }
        a.at(0x1013f7a8);
        if(a.at_least()){
            a.at(0x1013f7bc);
            a.value(3,0x40000);a.offset(0,3,0x0);a.compare_registers(4,0);
            if(a.equal()){
                select_platform_triple_group(a);return true;
            }
            a.at(0x1013f7cc);
            a.jump(0x1013fad8);
            reject_option(a);return false;
        }
        a.at(0x1013f7ac);
        a.offset(0,5,0x0);a.compare_registers(4,0);
        if(a.equal()){
            select_platform_pair_group(a);return true;
        }
        a.at(0x1013f7b8);
        a.jump(0x1013fad8);
        reject_option(a);return false;
    }
    a.at(0x1013f6c0);
    a.compare(4,256);
    if(a.equal()){
        select_audio_device_group(a);return true;
    }
    a.at(0x1013f6c8);
    if(a.at_least()){
        a.at(0x1013f728);
        a.compare(4,4096);
        if(a.equal()){
            select_two_way_group(a);return true;
        }
        a.at(0x1013f730);
        if(a.at_least()){
            a.at(0x1013f758);
            a.compare(4,16384);
            if(a.equal()){
                select_high_pair_group(a);return true;
            }
            a.at(0x1013f760);
            if(a.at_least()){
                reject_option(a);return false;
            }
            a.at(0x1013f764);
            a.compare(4,8192);
            if(a.equal()){
                select_two_way_group(a);return true;
            }
            a.at(0x1013f76c);
            a.jump(0x1013fad8);
            reject_option(a);return false;
        }
        a.at(0x1013f734);
        a.compare(4,1024);
        if(a.equal()){
            select_three_way_group(a);return true;
        }
        a.at(0x1013f73c);
        if(a.at_least()){
            a.at(0x1013f74c);
            a.compare(4,2048);
            if(a.equal()){
                select_three_way_group(a);return true;
            }
            a.at(0x1013f754);
            a.jump(0x1013fad8);
            reject_option(a);return false;
        }
        a.at(0x1013f740);
        a.compare(4,512);
        if(a.equal()){
            select_three_way_group(a);return true;
        }
        a.at(0x1013f748);
        a.jump(0x1013fad8);
        reject_option(a);return false;
    }
    a.at(0x1013f6cc);
    a.compare(4,16);
    if(a.equal()){
        select_display_group(a);return true;
    }
    a.at(0x1013f6d4);
    if(a.at_least()){
        a.at(0x1013f704);
        a.compare(4,64);
        if(a.equal()){
            select_audio_feature_group(a);return true;
        }
        a.at(0x1013f70c);
        if(a.at_least()){
            a.at(0x1013f71c);
            a.compare(4,128);
            if(a.equal()){
                select_audio_device_group(a);return true;
            }
            a.at(0x1013f724);
            a.jump(0x1013fad8);
            reject_option(a);return false;
        }
        a.at(0x1013f710);
        a.compare(4,32);
        if(a.equal()){
            select_audio_feature_group(a);return true;
        }
        a.at(0x1013f718);
        a.jump(0x1013fad8);
        reject_option(a);return false;
    }
    a.at(0x1013f6d8);
    a.compare(4,4);
    if(a.equal()){
        select_low_triple_group(a);return true;
    }
    a.at(0x1013f6e0);
    if(a.at_least()){
        a.at(0x1013f6f8);
        a.compare(4,8);
        if(a.equal()){
            select_display_group(a);return true;
        }
        a.at(0x1013f700);
        a.jump(0x1013fad8);
        reject_option(a);return false;
    }
    a.at(0x1013f6e4);
    a.compare(4,3);
    if(a.at_least()){
        reject_option(a);return false;
    }
    a.at(0x1013f6ec);
    a.compare(4,1);
    if(a.at_least()){
        select_low_triple_group(a);return true;
    }
    a.at(0x1013f6f4);
    a.jump(0x1013fad8);
    reject_option(a);return false;
}
}
void select_option(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){
    Call a(s,select_option_entry,d,o);
    if(choose_option(a)){a.at(0x1013fb34);a.value(3,1);}
    a.at(0x1013fb38);
    a.load(0,1,0x78);a.offset(1,1,0x70);a.restore_link();a.load(31,1,-0x4);a.load(30,1,-0x8);a.load(29,1,-0xc);a.load(28,1,-0x10);a.finish();
}
}
