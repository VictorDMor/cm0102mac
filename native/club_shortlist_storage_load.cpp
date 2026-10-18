#include "club_shortlist_storage_detail.hpp"

namespace cm::aot::club_shortlist_storage {
using detail::Call;
namespace {
void initialize_legacy_tail(Call& a) {
    a.at(0x1035886c);a.load(3,18,0xdc);a.offset(0,24,2);a.value(8,0);a.offset(14,24,4);a.indexed_load(3,23,3);a.offset(15,24,8);a.offset(16,24,12);a.offset(17,24,16);a.indexed_store<2>(8,3,0);
    a.value(6,0xffffffff);a.offset(11,24,20);a.offset(0,24,0x28);a.load(3,18,0xdc);a.value(12,255);a.offset(10,24,24);a.offset(9,24,0x24);a.indexed_load(7,23,3);a.offset(3,1,0x3c);a.value(4,1);a.value(5,1997);a.indexed_store(8,7,14);
    a.load(7,18,0xdc);a.indexed_load(7,23,7);a.indexed_store(8,7,15);a.load(7,18,0xdc);a.indexed_load(7,23,7);a.indexed_store(8,7,16);a.load(7,18,0xdc);a.indexed_load(7,23,7);a.indexed_store(6,7,17);
    a.load(7,18,0xdc);a.indexed_load(7,23,7);a.indexed_store(6,7,11);a.load(7,18,0xdc);a.indexed_load(11,23,7);a.indexed_load<1>(7,11,0);a.insert_mask(7,12,0x78,3);a.indexed_store<1>(7,11,0);
    a.load(7,18,0xdc);a.indexed_load(7,23,7);a.indexed_store(6,7,10);a.load(7,18,0xdc);a.indexed_load(10,23,7);a.indexed_load<1>(7,10,0);a.insert_mask(7,8,0x80,7);a.indexed_store<1>(7,10,0);
    a.load(7,18,0xdc);a.indexed_load(7,23,7);a.indexed_store<1>(6,7,9);a.load(6,18,0xdc);a.indexed_load(7,23,6);a.indexed_load<1>(6,7,0);a.insert_mask(6,8,4,2);a.indexed_store<1>(6,7,0);a.call(0x107156b0);
    a.load(3,18,0xdc);a.offset(4,24,0x1c);a.load<2>(0,1,0x3e);a.indexed_load(3,23,3);a.add(4,3,4);a.store<2>(4,2,0);a.load<2>(0,1,0x3c,true);a.store<2>(4,0,0);a.load<1>(0,1,0x40);a.store<1>(4,4,0);a.jump(0x10358a90);
}
void copy_legacy_row(Call& a) {
    a.at(0x10358974);a.offset(4,1,0x274);a.load(3,18,0xdc);a.add(4,4,24);a.offset(0,24,2);a.load<2>(5,4,0,true);a.offset(11,24,4);a.indexed_load(3,23,3);a.offset(10,24,8);a.offset(9,24,12);a.offset(8,24,16);a.indexed_store<2>(5,3,0);
    a.offset(7,24,20);a.offset(0,24,0x28);a.offset(6,24,24);a.load(12,18,0xdc);a.offset(5,24,0x24);a.load(17,4,4);a.offset(3,24,0x1c);a.indexed_load(12,23,12);a.indexed_store(17,12,11);
    a.load(11,18,0xdc);a.load(12,4,8);a.indexed_load(11,23,11);a.indexed_store(12,11,10);a.load(10,18,0xdc);a.load(11,4,12);a.indexed_load(10,23,10);a.indexed_store(11,10,9);
    a.load(9,18,0xdc);a.load(10,4,16);a.indexed_load(9,23,9);a.indexed_store(10,9,8);a.load(8,18,0xdc);a.load(9,4,20);a.indexed_load(8,23,8);a.indexed_store(9,8,7);
    a.load(7,18,0xdc);a.load<1>(9,4,0x28);a.indexed_load(8,23,7);a.indexed_load<1>(7,8,0);a.insert_mask(7,9,0x78,0);a.indexed_store<1>(7,8,0);
    a.load(7,18,0xdc);a.load(8,4,24);a.indexed_load(7,23,7);a.indexed_store(8,7,6);a.load(6,18,0xdc);a.load<1>(8,4,0x28);a.indexed_load(7,23,6);a.indexed_load<1>(6,7,0);a.insert_mask(6,8,0x80,0);a.indexed_store<1>(6,7,0);
    a.load(6,18,0xdc);a.load<1>(7,4,0x24);a.indexed_load(6,23,6);a.indexed_store<1>(7,6,5);a.load(5,18,0xdc);a.load<1>(7,4,0x28);a.indexed_load(6,23,5);a.indexed_load<1>(5,6,0);a.insert_mask(5,7,4,0);a.indexed_store<1>(5,6,0);
    a.load(5,18,0xdc);a.load<2>(0,4,0x1e);a.indexed_load(5,23,5);a.add(3,5,3);a.store<2>(3,2,0);a.load<2>(0,4,0x1c,true);a.store<2>(3,0,0);a.load<1>(0,4,0x20);a.store<1>(3,4,0);
}
}
void load_shortlist_storage(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,load_shortlist_storage_entry,d,o);a.link();a.multiple(14);a.move(19,4);a.move(18,3);a.load(25,2,-0x629c);a.move(6,19);a.load(26,2,-0x7b74);a.offset(3,18,8);a.load(27,2,-0x7be8);a.value(4,4);a.load(29,2,-0x7bac);a.value(5,1);a.load(30,2,-0x7ba8);a.load(31,2,-0x4dd8);a.load(28,2,-0x7b50);a.save_link();a.frame(0x710);a.call(0x10747a80);a.compare(3,1);
    const auto finish=[&]{a.at(0x10358adc);a.pop(0x710);a.multiple(14,true);a.finish();};
    const auto fail=[&](std::uint32_t pc,unsigned first,unsigned second,unsigned line){detail::diagnostic(a,pc,31,30,first,second,line);a.value(0,0);a.store<1>(29,0,0);a.jump(0x10358adc);finish();};
    if(!a.equal())return fail(0x10358070,0x254,0xac,0x2be);
    a.at(0x103580c4);a.load(0,18,8);a.store(18,16,0);a.load(0,18,8);a.store(18,20,0);a.load(0,18,8);a.store(18,12,0);a.load(0,18,8);a.mask(3,0,0xfffffff8,false,3);a.call(0x10775130);a.store(18,0,3);a.load(0,18,12);a.mask(3,0,0xfffffff8,false,3);a.call(0x10775130);a.store(18,4,3);
    a.load(3,18,0);a.compare(3,0,false);bool allocated=!a.equal();if(allocated){a.load(0,18,4);a.compare(0,0,false);allocated=!a.equal();}
    if(!allocated){a.at(0x10358114);a.load(0,18,8);a.compare(0,0);if(!a.equal())return fail(0x10358120,0x234,0xa4,0x2ce);}
    a.at(0x10358174);a.load(5,18,16);a.move(6,19);a.value(4,8);a.call(0x10747a80);a.load(0,18,16);a.compare_registers(0,3);
    if(!a.equal())return fail(0x10358190,0x214,0x9c,0x2d5);
    a.at(0x103581e4);a.load(3,18,0);a.mask(5,0,0xfffffff8,false,3);a.load(4,18,4);a.call(0x1077fc28);a.load(2,1,0x14);a.load(3,18,4);a.value(5,8);a.load(4,18,20);a.load(6,2,-0x6298);a.call(0x10768300);
    a.load(3,26,0);a.call(0x10775550);a.value(4,0x40000);a.offset(0,4,-0x1fea);a.compare_registers(3,0);
    if(!a.at_least()){
        a.move(6,19);a.offset(3,1,0x38);a.value(4,4);a.value(5,1);a.call(0x10747a80);a.compare(3,1);
        if(!a.equal())return fail(0x10358240,0x1f4,0x94,0x2e7);
        a.at(0x10358294);a.load(0,1,0x38);a.multiply(3,0,24);a.call(0x10775130);a.move(14,3,true);
        if(a.equal()){a.load(0,1,0x38);a.compare(0,0);if(!a.equal())return fail(0x103582b4,0x1d4,0x8c,0x2f0);}
        a.at(0x10358308);a.load(5,1,0x38);a.move(3,14);a.move(6,19);a.value(4,24);a.call(0x10747a80);a.load(0,1,0x38);a.compare_registers(0,3);
        if(!a.equal())return fail(0x10358328,0x1b4,0x84,0x2f8);
        a.at(0x1035837c);a.move(3,14);a.call(0x10775170);
    }
    a.at(0x10358384);a.value(16,0);a.value(3,0x40000);a.move(14,16);a.value(17,0);a.offset(15,3,-0x1fea);
    do{
        a.at(0x10358398);a.load(3,26,0);a.call(0x10775550);a.compare_registers(3,15);bool read=a.positive();
        if(!read){a.compare(17,15);read=!a.at_least();}
        if(read){
            a.at(0x103583b0);a.offset(3,16,0x98);a.move(6,19);a.add(3,18,3);a.value(4,4);a.value(5,1);a.call(0x10747a80);a.compare(3,1);
            if(!a.equal())return fail(0x103583d0,0x194,0x7c,0x307);
        }else{a.at(0x10358428);a.offset(0,16,0x98);a.indexed_store(14,18,0);}
        a.at(0x10358430);a.offset(17,17,1);a.offset(16,16,4);a.compare(17,16);
    }while(a.less());
    a.move(6,19);a.offset(3,18,0xd8);a.value(4,4);a.value(5,1);a.call(0x10747a80);a.compare(3,1);
    if(!a.equal())return fail(0x1035845c,0x174,0x74,0x314);
    a.at(0x103584b0);a.value(3,0x40000);a.value(23,0);a.offset(14,3,-0x1fe9);a.value(21,0);
    do{
        a.at(0x103584c0);a.value(22,0);a.move(20,22);a.flags(0,0,0);
        do{
            a.at(0x103584cc);a.load(0,18,0xd8);a.multiply(3,0,12);a.call(0x10775130);a.add(4,21,20);a.offset(15,4,0x18);a.indexed_store(3,18,15);a.indexed_load(0,18,15);a.compare(0,0,false);
            if(a.equal())detail::fatal_allocation(a,0x103584f0,31,30,29,0x154,0x6c,0x322);
            a.at(0x10358548);a.load(3,26,0);a.call(0x10775550);a.compare_registers(3,14);bool read=a.at_least();
            if(!read){a.compare(22,15);read=!a.equal();}
            if(read){
                a.at(0x10358570);a.offset(16,20,0x98);a.indexed_load(3,18,15);a.indexed_load(5,18,16);a.move(6,19);a.value(4,12);a.call(0x10747a80);a.indexed_load(0,18,16);a.compare_registers(0,3);
                if(!a.equal())return fail(0x10358594,0x134,0x64,0x330);
            }else{a.at(0x10358560);a.load(3,18,0x54);a.value(0,0xffffffff);a.store(3,0,0);a.jump(0x103585ec);}
            a.at(0x103585ec);a.offset(22,22,1);a.offset(20,20,4);a.compare(22,16);
        }while(a.less());
        a.offset(23,23,1);a.offset(21,21,64);a.compare(23,2);
    }while(a.less());
    a.load(3,18,0xdc);a.move(6,19);a.load(5,28,0);a.value(4,12);a.call(0x10747a80);a.load(0,28,0);a.compare_registers(0,3);
    if(!a.equal())return fail(0x1035862c,0x114,0x5c,0x33a);
    a.at(0x10358680);a.value(20,0);a.move(23,20);a.move(22,20);a.jump(0x10358aac);
    for(;;){
        a.at(0x10358aac);a.load(0,28,0);a.compare_registers(20,0);if(!a.less())break;
        a.at(0x10358690);a.load(0,18,0xdc);a.add(5,0,23);a.load(0,5,0);a.compare(0,0,false);
        if(!a.equal()){
            a.load(3,27,0);a.offset(0,22,0x1ec);a.load<2>(4,5,6,true);a.indexed_load<2>(0,3,0,true);a.compare_registers(4,0);if(!a.at_least())a.store<2>(5,6,0);
            a.at(0x103586c0);a.value(3,0x6e0);a.call(0x10775130);a.load(4,18,0xdc);a.indexed_store(3,4,23);a.load(3,18,0xdc);a.indexed_load(0,3,23);a.compare(0,0,false);
            if(a.equal())return fail(0x103586e0,0xf4,0x54,0x34d);
            a.at(0x10358734);a.load(3,26,0);a.call(0x10775550);a.value(4,0x40000);a.offset(0,4,-0x1fea);a.compare_registers(3,0);
            if(!a.nonpositive()){
                a.load(3,18,0xdc);a.move(6,19);a.value(4,44);a.value(5,40);a.indexed_load(3,3,23);a.call(0x10747a80);a.compare(3,40);
                if(!a.equal())return fail(0x1035876c,0xd4,0x4c,0x357);
            }else{
                a.at(0x103587c0);a.move(4,25);a.offset(3,1,0x274);a.value(5,0);a.value(6,44);a.value(7,25);a.call(0x10773320);a.move(6,19);a.offset(3,1,0x274);a.value(4,44);a.value(5,25);a.call(0x10747a80);a.compare(3,25);
                if(!a.equal())return fail(0x103587f4,0xb4,0x44,0x363);
                a.at(0x10358848);a.value(21,0);a.move(24,21);a.flags(0,0,0);
                do{
                    a.at(0x10358854);a.load(0,18,0xdc);a.compare(21,25);a.value(4,11);a.indexed_load(3,23,0);a.indexed_store<1>(4,3,24);
                    if(!a.less())initialize_legacy_tail(a);else copy_legacy_row(a);
                    a.at(0x10358a90);a.offset(21,21,1);a.offset(24,24,44);a.compare(21,40);
                }while(a.less());
            }
        }
        a.at(0x10358aa0);a.offset(23,23,12);a.offset(22,22,0x248);a.offset(20,20,1);
    }
    a.at(0x10358ab8);a.load(3,26,0);a.call(0x10775550);a.value(4,0x40000);a.offset(0,4,-0x1fe9);a.compare_registers(3,0);if(!a.at_least()){a.move(3,18);a.value(4,0);a.call(reset_shortlist_storage_entry);}
    finish();
}
}
