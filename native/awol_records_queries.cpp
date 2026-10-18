#include "awol_records.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::awol {
namespace {
using Call=SemanticGuestCall;
bool valid_staff(Call& a,std::uint32_t start,unsigned staff,std::uint32_t exit) {
    a.at(start);bool valid=a.equal();
    if(!valid){a.load(3,2,-0x7be4);a.load(4,staff,0);a.load(0,3,0);a.compare_registers(4,0);valid=a.less();}
    if(!valid){a.value(3,0);a.jump(exit);}return valid;
}
void validate_primary_club(Call& a) {
    a.at(0x10003e44);a.load(3,2,-0x7b7c);a.move(4,31);a.load(3,3,0);a.call(0x10082f20);a.compare(3,0,false);a.store(30,0,3);
    if(!a.equal()){a.load(0,3,4);a.load(3,2,-0x7be8);a.multiply(0,0,0x248);a.load(3,3,0);a.add(0,3,0);a.jump(0x10003e7c);}
    else {a.at(0x10003e78);a.value(0,0);}
    a.at(0x10003e7c);a.compare(0,0,false);
    if(a.equal()){a.move(3,29);a.move(4,31);a.call(remove_primary_staff_entry);}
}
void validate_secondary_club(Call& a) {
    a.at(0x10003f24);a.load(3,2,-0x7b7c);a.move(4,30);a.load(3,3,0);a.call(0x10082f20);a.compare(3,0,false);a.store(31,0,3);
    if(!a.equal()){a.load(0,3,4);a.load(3,2,-0x7be8);a.multiply(0,0,0x248);a.load(3,3,0);a.add(0,3,0);a.jump(0x10003f5c);}
    else {a.at(0x10003f58);a.value(0,0);}
    a.at(0x10003f5c);a.compare(0,0,false);
    if(a.equal()){a.move(3,29);a.move(4,30);a.call(remove_secondary_staff_entry);}
}
void remove_staff_record(State& s,std::uint32_t root,unsigned field,unsigned bit,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,root,d,o);a.link();a.save(31);a.save(30);a.move(30,4,true);a.save(29);a.move(29,3);a.save_link();a.frame(80);
    if(!a.equal()) {
        a.load(4,29,12);a.compare(4,0,false);
        if(!a.equal()) {
            a.load(3,30,0);a.indexed_load<1>(5,4,3);a.sign8(5,5);a.mask(0,5,bit,true);
            if(!a.equal()) {
                a.flags(0,5,bit);a.compare(0,0);
                if(!a.equal()){a.indexed_load<1>(0,4,3);a.exclusive_flags(0,0,bit);a.indexed_store<1>(0,4,3);}
                a.at(root+0x5c);a.load(3,29,int(field));a.compare(3,0,false);
                if(!a.equal()) {
                    a.load(0,3,4);a.value(31,0);a.store(3,12,0);a.jump(root+0xa4);
                    while(true) {
                        a.at(root+0xa4);a.load(3,29,int(field));a.load<2>(0,3,0,true);a.compare_registers(31,0);if(!a.less())break;
                        a.at(root+0x78);a.call(0x10738e00);a.move(4,3,true);if(a.equal())break;
                        a.load(3,4,0);a.load(0,30,0);a.compare_registers(3,0);
                        if(a.equal()){a.load(3,29,int(field));a.call(0x10738e60);a.jump(root+0xb4);break;}
                        a.at(root+0xa0);a.offset(31,31,1);
                    }
                }
            }
        }
    }
    a.at(root+0xb4);a.pop(80);a.restore(31);a.restore(30);a.restore(29);a.finish();
}
}
void primary_absence_status(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,primary_status_entry,d,o);a.link();a.save(31);a.move(31,4,true);a.save(30);a.load(30,2,-0x7b78);a.save(29);a.move(29,3);a.save_link();a.frame(80);
    if(a.equal()){a.value(3,0);a.jump(0x10003ea4);}
    else if(valid_staff(a,0x10003df0,31,0x10003ea4)) {
        a.at(0x10003e10);a.sign8(0,5,true);
        if(!a.equal()){a.load(3,29,12);a.load(0,31,0);a.indexed_load<1>(0,3,0);a.mask(0,0,1);a.sign8(3,0);a.jump(0x10003ea4);}
        else {a.at(0x10003e30);a.load(3,29,12);a.load(0,31,0);a.indexed_load<1>(0,3,0);a.mask(0,0,1,true);if(!a.equal())validate_primary_club(a);a.at(0x10003e90);a.load(3,29,12);a.load(0,31,0);a.indexed_load<1>(0,3,0);a.mask(0,0,1);a.sign8(3,0);}
    }
    a.at(0x10003ea4);a.pop(80);a.restore(31);a.restore(30);a.restore(29);a.finish();
}
void secondary_absence_status(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,secondary_status_entry,d,o);a.link();a.save(31);a.load(31,2,-0x7b78);a.save(30);a.move(30,4,true);a.save(29);a.move(29,3);a.save_link();a.frame(80);
    if(a.equal()){a.value(3,0);a.jump(0x10003f84);}
    else if(valid_staff(a,0x10003ef0,30,0x10003f84)) {
        a.at(0x10003f10);a.load(3,29,12);a.load(0,30,0);a.indexed_load<1>(0,3,0);a.mask(0,0,2,true);if(!a.equal())validate_secondary_club(a);
        a.at(0x10003f70);a.load(3,29,12);a.load(0,30,0);a.indexed_load<1>(0,3,0);a.mask(0,0,2);a.sign8(3,0);
    }
    a.at(0x10003f84);a.pop(80);a.restore(31);a.restore(30);a.restore(29);a.finish();
}
void has_pending_record(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,pending_status_entry,d,o);a.link();a.save(31);a.save(30);a.move(30,4,true);a.save(29);a.move(29,3);a.save_link();a.frame(80);
    if(a.equal()){a.value(3,0);a.jump(0x1000404c);}
    else if(valid_staff(a,0x10003fcc,30,0x1000404c)) {
        a.at(0x10003fec);a.load(3,29,8);a.compare(3,0,false);bool found=false;
        if(!a.equal()) {
            a.load(0,3,4);a.value(31,0);a.store(3,12,0);a.jump(0x10004038);
            while(true){a.at(0x10004038);a.load(3,29,8);a.load<2>(0,3,0,true);a.compare_registers(31,0);if(!a.less())break;a.at(0x10004008);a.call(0x10738e00);a.compare(3,0,false);if(a.equal()){a.value(3,0);a.jump(0x1000404c);found=true;break;}a.at(0x1000401c);a.load(3,3,0);a.load(0,30,0);a.compare_registers(3,0);if(a.equal()){a.value(3,1);a.jump(0x1000404c);found=true;break;}a.at(0x10004034);a.offset(31,31,1);}
        }
        if(!found){a.at(0x10004048);a.value(3,0);}
    }
    a.at(0x1000404c);a.pop(80);a.restore(31);a.restore(30);a.restore(29);a.finish();
}
void remove_primary_by_staff(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){remove_staff_record(s,remove_primary_staff_entry,0,1,d,o);}
void remove_secondary_by_staff(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){remove_staff_record(s,remove_secondary_staff_entry,4,2,d,o);}
void remove_primary_item(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,remove_primary_item_entry,d,o);a.link();a.save(31);a.move(31,4,true);a.save(30);a.move(30,3);a.save(29);a.load(29,2,-0x7b78);a.save(28);a.load(28,2,-0x7be0);a.save_link();a.frame(80);
    if(!a.equal()) {
        a.load(3,30,12);a.load(4,31,0);a.indexed_load<1>(0,3,4);a.sign8(0,0,true);
        if(!a.equal()) {
            a.load(3,2,-0x7b7c);a.multiply(0,4,0x70);a.load(4,28,0);a.load(3,3,0);a.add(4,4,0);a.call(0x10082f20);a.compare(3,0,false);a.store(29,0,3);
            if(!a.equal()){a.load(0,3,4);a.load(3,2,-0x7be8);a.multiply(0,0,0x248);a.load(3,3,0);a.add(0,3,0);a.jump(0x10006754);}else{a.at(0x10006750);a.value(0,0);}
            a.at(0x10006754);a.compare(0,0,false);
            if(!a.equal()){a.load(0,31,0);a.move(3,30);a.load(4,28,0);a.value(6,0);a.multiply(0,0,0x70);a.value(7,0xffffffff);a.add(4,4,0);a.load(5,4,0x24);a.call(notice_entry);}
            a.at(0x10006780);a.load(5,30,12);a.load(4,31,0);a.indexed_load<1>(3,5,4);a.sign8(0,3);a.flags(0,0,1);a.compare(0,0);
            if(!a.equal()){a.exclusive_flags(0,3,1);a.indexed_store<1>(0,5,4);}
            a.at(0x100067a4);a.load(3,30,0);a.move(4,31);a.call(0x10738e60);
        }
    }
    a.at(0x100067b0);a.pop(80);a.restore(31);a.restore(30);a.restore(29);a.restore(28);a.finish();
}
void remove_secondary_item(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,remove_secondary_item_entry,d,o);a.link();a.compare(4,0,false);a.save_link();a.frame(64);
    if(!a.equal()) {
        a.load(6,3,12);a.load(5,4,0);a.indexed_load<1>(7,6,5);a.sign8(0,7,true);
        if(!a.equal()) {a.sign8(0,7);a.flags(0,0,2);a.compare(0,0);if(!a.equal()){a.exclusive_flags(0,7,2);a.indexed_store<1>(0,6,5);}a.at(0x10006810);a.load(3,3,4);a.call(0x10738e60);}
    }
    a.at(0x10006818);a.pop(64);a.finish();
}
}
