#include "club_shortlist_storage_detail.hpp"

namespace cm::aot::club_shortlist_storage {
using detail::Call;
void save_shortlist_storage(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,save_shortlist_storage_entry,d,o);a.link();a.multiple(21);a.move(28,4);a.move(27,3);a.load(29,2,-0x7bac);a.move(6,28);a.load(30,2,-0x7ba8);a.offset(3,27,0x10);a.load(31,2,-0x4dd8);a.value(4,4);a.load(26,2,-0x7b50);a.value(5,1);a.save_link();a.frame(0x180);a.call(0x10747cc0);a.compare(3,1);
    const auto finish=[&]{a.at(0x10358f00);a.pop(0x180);a.multiple(21,true);a.finish();};
    const auto fail=[&](std::uint32_t pc,unsigned first,unsigned second,unsigned line){detail::diagnostic(a,pc,31,30,first,second,line);a.value(0,0);a.store<1>(29,0,0);a.jump(0x10358f00);finish();};
    if(!a.equal())return fail(0x10358b74,0x130,0x68,0x3a1);
    a.at(0x10358bc8);a.load(3,27,0);a.move(6,28);a.load(5,27,0x10);a.value(4,8);a.call(0x10747cc0);a.load(0,27,0x10);a.compare_registers(0,3);
    if(!a.equal())return fail(0x10358be8,0x110,0x60,0x3a9);
    a.at(0x10358c3c);a.value(22,0);a.value(25,0);a.flags(0,0,0);
    do{
        a.at(0x10358c48);a.offset(3,25,0x98);a.move(6,28);a.add(3,27,3);a.value(4,4);a.value(5,1);a.call(0x10747cc0);a.compare(3,1);
        if(!a.equal())return fail(0x10358c68,0xf0,0x58,0x3b2);
        a.at(0x10358cbc);a.offset(22,22,1);a.offset(25,25,4);a.compare(22,16);
    }while(a.less());
    a.move(6,28);a.offset(3,27,0xd8);a.value(4,4);a.value(5,1);a.call(0x10747cc0);a.compare(3,1);
    if(!a.equal())return fail(0x10358ce8,0xd0,0x50,0x3ba);
    a.at(0x10358d3c);a.value(22,0);a.value(24,0);
    do{
        a.at(0x10358d44);a.value(21,0);a.value(25,0);
        do{
            a.at(0x10358d4c);a.add(3,24,25);a.offset(23,25,0x98);a.offset(0,3,0x18);a.indexed_load(5,27,23);a.indexed_load(3,27,0);a.move(6,28);a.value(4,12);a.call(0x10747cc0);a.indexed_load(0,27,23);a.compare_registers(0,3);
            if(!a.equal())return fail(0x10358d78,0xb0,0x48,0x3c6);
            a.at(0x10358dcc);a.offset(21,21,1);a.offset(25,25,4);a.compare(21,16);
        }while(a.less());
        a.offset(22,22,1);a.offset(24,24,64);a.compare(22,2);
    }while(a.less());
    a.load(3,27,0xdc);a.move(6,28);a.load(5,26,0);a.value(4,12);a.call(0x10747cc0);a.load(0,26,0);a.compare_registers(0,3);
    if(!a.equal())return fail(0x10358e0c,0x90,0x40,0x3d0);
    a.at(0x10358e60);a.value(23,0);a.move(24,23);a.jump(0x10358ef4);
    for(;;){
        a.at(0x10358ef4);a.load(0,26,0);a.compare_registers(23,0);if(!a.less())break;
        a.at(0x10358e70);a.load(3,27,0xdc);a.indexed_load(3,3,24);a.compare(3,0,false);
        if(!a.equal()){
            a.move(6,28);a.value(4,44);a.value(5,40);a.call(0x10747cc0);a.compare(3,40);
            if(!a.equal())return fail(0x10358e98,0x70,0x38,0x3de);
        }
        a.at(0x10358eec);a.offset(24,24,12);a.offset(23,23,1);
    }
    finish();
}
}
