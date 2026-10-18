#include "shortlist_links.hpp"
#include "club_shortlist_storage_detail.hpp"

namespace cm::aot::shortlist_links {
using Call=SemanticGuestCall;
namespace {
void ensure_pair_index(Call& a,std::uint32_t pc,unsigned pointer,unsigned capacity,unsigned scratch,unsigned detail,unsigned line,bool initial) {
    a.at(pc);a.load(0,initial?3:25,pointer);a.compare(0,0,false);
    if(!a.equal())return;
    a.value(3,800);a.call(0x10775130);a.store(25,pointer,3);a.load(0,25,pointer);a.compare(0,0,false);
    if(a.equal())club_shortlist_storage::detail::fatal_allocation(a,pc+36,31,30,29,scratch,detail,line);
    a.at(pc+124);a.value(0,100);a.store(25,capacity,0);
}
void grow_pair_index(Call& a,std::uint32_t pc,unsigned pointer,unsigned capacity,unsigned count,unsigned scratch,unsigned detail,unsigned line) {
    a.at(pc);a.load(3,25,capacity);a.load(0,25,count);a.compare_registers(3,0);
    if(!a.equal())return;
    a.offset(0,3,100);a.load(3,25,pointer);a.mask(4,0,0xfffffff8,false,3);a.call(0x107751b0);a.store(25,pointer,3);a.load(0,25,pointer);a.compare(0,0,false);
    if(a.equal())club_shortlist_storage::detail::fatal_allocation(a,pc+48,31,30,29,scratch,detail,line);
    a.at(pc+136);a.load(3,25,capacity);a.offset(0,3,100);a.store(25,capacity,0);
}
}
void add_staff_club_link(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,add_staff_club_link_entry,d,o);a.link();a.multiple(25);a.load(28,2,-0x62a8);a.move(25,3);a.load(29,2,-0x7bac);a.move(26,4);a.load(30,2,-0x7ba8);a.move(27,5);a.load(31,2,-0x4dd8);a.save_link();a.frame(0x100);
    ensure_pair_index(a,0x1036331c,0,8,0xc0,0x58,0x157f,true);ensure_pair_index(a,0x103633a0,4,12,0xa0,0x50,0x1588,false);
    a.at(0x10363424);a.load(0,25,0);a.compare(0,0,false);
    if(a.equal()){a.value(0,0);a.jump(0x10363488);}
    else{
        a.at(0x10363438);a.load(0,26,0);a.compare(27,0,false);a.store(1,0x38,0);
        if(!a.equal()){a.load(0,27,0);a.store(1,0x3c,0);a.jump(0x1036345c);}else{a.at(0x10363454);a.value(0,0xffffffff);a.store(1,0x3c,0);}
        a.at(0x1036345c);a.load(4,25,0);a.move(7,28);a.load(5,25,0x10);a.offset(3,1,0x38);a.value(6,8);a.call(0x10775600);a.compare(3,0,false);
        if(!a.equal()){a.value(0,1);a.jump(0x10363488);}else{a.at(0x10363484);a.value(0,0);}
    }
    a.at(0x10363488);a.sign8(0,0,true);
    if(a.equal()){
        grow_pair_index(a,0x10363490,0,8,0x10,0x80,0x48,0x15a0);grow_pair_index(a,0x10363524,4,12,0x14,0x60,0x40,0x15a9);
        a.at(0x103635b8);a.load(0,25,0x10);a.move(6,28);a.load(4,26,0);a.value(5,8);a.load(3,25,0);a.mask(0,0,0xfffffff8,false,3);a.indexed_store(4,3,0);
        a.load(0,25,0x10);a.load(3,25,0);a.mask(0,0,0xfffffff8,false,3);a.load(4,27,0);a.add(3,3,0);a.store(3,4,4);a.load(3,25,0x10);a.offset(0,3,1);a.store(25,0x10,0);a.load(3,25,0);a.load(4,25,0x10);a.call(0x10768300);
        a.load(0,25,0x14);a.value(5,8);a.load(4,26,0);a.load(3,25,4);a.mask(0,0,0xfffffff8,false,3);a.load(6,2,-0x6298);a.indexed_store(4,3,0);
        a.load(0,25,0x14);a.load(3,25,4);a.mask(0,0,0xfffffff8,false,3);a.load(4,27,0);a.add(3,3,0);a.store(3,4,4);a.load(3,25,0x14);a.offset(0,3,1);a.store(25,0x14,0);a.load(3,25,4);a.load(4,25,0x14);a.call(0x10768300);
        a.move(3,27);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);
        if(a.equal()){a.load(0,26,0);a.load(4,2,-0x7b9c);a.multiply(3,0,100);a.load(5,4,0);a.offset(6,3,12);a.indexed_load<1>(4,5,6);a.mask(3,4,15,false,28);a.offset(0,3,1);a.insert_mask(4,0,0xf0,4);a.indexed_store<1>(4,5,6);}
    }
    a.at(0x10363698);a.pop(0x100);a.multiple(25,true);a.finish();
}
void reset_linked_club_rows(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,reset_linked_club_rows_entry,d,o);a.link();a.multiple(27);a.move(28,3);a.move(29,4);a.move(30,5);a.value(3,16);a.save_link();a.frame(0x50);a.call(0x10773520);a.move(27,3,true);if(!a.equal())a.call(0x10738c00);
    a.at(0x10363bb0);a.move(3,28);a.move(4,29);a.move(5,27);a.value(6,1);a.value(7,3);a.value(8,0);a.call(collect_staff_links_entry);a.load<2>(0,27,0,true);a.compare(0,0);
    if(!a.equal()){
        a.load(0,27,4);a.value(31,0);a.store(27,12,0);a.jump(0x10363c6c);
        for(;;){
            a.at(0x10363c6c);a.load<2>(0,27,0,true);a.compare_registers(31,0);if(!a.less())break;
            a.at(0x10363be8);a.move(3,27);a.call(0x10738e00);a.compare(3,0,false);
            if(!a.equal()){
                a.load(3,3,4);a.sign8(0,30,true);a.load(4,28,0xdc);a.value(0,100);a.multiply(3,3,12);a.add(7,4,3);a.store<1>(7,9,0);
                if(!a.equal()){
                    a.value(8,0);a.value(5,0);a.value(3,0xffffffff);a.jump(0x10363c5c);
                    for(;;){
                        a.at(0x10363c5c);a.sign8(0,8);a.compare(0,40);if(!a.less())break;
                        a.at(0x10363c2c);a.load(0,7,0);a.load(4,29,0);a.add(6,0,5);a.load(0,6,0x10);a.compare_registers(4,0);
                        if(a.equal()){a.load(0,6,0x18);a.compare(0,-1);if(!a.nonpositive())a.store(6,0x18,3);}
                        a.at(0x10363c54);a.offset(5,5,44);a.offset(8,8,1);
                    }
                }
            }
            a.at(0x10363c68);a.offset(31,31,1);
        }
    }
    a.at(0x10363c78);a.move(3,27);a.value(4,1);a.call(0x10738c20);a.pop(0x50);a.multiple(27,true);a.finish();
}
}
