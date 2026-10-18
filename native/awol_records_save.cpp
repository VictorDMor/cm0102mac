#include "awol_records.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::awol {
namespace {
using Call=SemanticGuestCall;
struct WriteError {std::uint32_t pc;std::int32_t scratch,other;std::uint32_t line;};
void reject_write(Call& a,WriteError error,bool close=true) {
    a.at(error.pc);a.offset(3,31,9);a.offset(6,1,error.scratch);a.offset(7,1,error.other);a.value(4,0);a.value(5,0);
    a.call(0x10739890);a.call(0x10768810);a.move(5,3);a.move(3,30);a.offset(4,31,0x12);a.offset(6,1,error.scratch);a.offset(7,1,error.other);a.value(8,error.line);a.call(0x1077b720);
    a.move(4,30);a.offset(3,31,0x1f);a.value(5,0);a.call(0x10769b60);a.value(0,0);
    if(close){a.offset(3,1,0x3c);a.store<1>(29,0,0);a.call(0x10747900);a.value(3,0);}
    else {a.value(3,0);a.store<1>(29,0,0);}
    a.jump(0x10003dac);
}
struct ListArchive {
    unsigned field,width;std::uint32_t count;WriteError count_error;
    std::uint32_t setup,next;WriteError next_error;
    std::uint32_t write;WriteError write_error;std::uint32_t check;
};
// The three lists share archive operations but retain distinct source sites,
// diagnostic scratch areas, line numbers, record widths and iterator order.
bool write_list(Call& a,const ListArchive& list) {
    a.at(list.count);a.load(5,28,int(list.field));a.offset(3,1,0x38);a.load(6,1,0x3c);a.value(4,4);a.load<2>(0,5,0,true);a.value(5,1);a.store(1,0x38,0);a.call(0x10747cc0);a.compare(3,1);
    if(!a.equal()){reject_write(a,list.count_error);return false;}
    a.at(list.setup);a.load(3,28,int(list.field));a.value(27,0);a.load(0,3,4);a.store(3,12,0);a.jump(list.check);
    while(true) {
        a.at(list.check);a.load(3,28,int(list.field));a.load<2>(0,3,0,true);a.compare_registers(27,0);if(!a.less())break;
        a.at(list.next);a.call(0x10738e00);a.compare(3,0,false);
        if(a.equal()){reject_write(a,list.next_error);return false;}
        a.at(list.write);a.load(6,1,0x3c);a.value(4,list.width);a.value(5,1);a.call(0x10747cc0);a.compare(3,1);
        if(!a.equal()){reject_write(a,list.write_error);return false;}
        a.at(list.check-4);a.offset(27,27,1);
    }
    return true;
}
bool write_archive(Call& a) {
    if(a.equal()){reject_write(a,{0x1000370c,0x228,0xa0,0x132},false);return false;}
    a.at(0x10003764);a.move(3,4);a.move(4,31);a.value(5,1);a.value(6,22);a.value(7,4);a.call(0x107475b0);a.move(6,3,true);a.store(1,0x3c,3);
    if(a.equal()){reject_write(a,{0x10003788,0x208,0x98,0x13a},false);return false;}
    a.at(0x100037e0);a.move(3,27);a.value(4,4);a.value(5,1);a.call(0x10747cc0);a.compare(3,1);
    if(!a.equal()){reject_write(a,{0x100037f8,0x1e8,0x90,0x141});return false;}
    a.at(0x10003858);a.load(3,28,12);a.value(4,1);a.load(5,27,0);a.load(6,1,0x3c);a.call(0x10747cc0);a.load(0,27,0);a.compare_registers(0,3);
    if(!a.equal()){reject_write(a,{0x10003878,0x1c8,0x88,0x148});return false;}
    constexpr ListArchive lists[]{
        {0,12,0x100038d8,{0x10003900,0x1a8,0x80,0x151},0x10003960,0x10003974,{0x10003980,0x188,0x78,0x15a},0x100039e4,{0x100039fc,0x168,0x70,0x160},0x10003a60},
        {4,16,0x10003a70,{0x10003a98,0x148,0x68,0x16a},0x10003af8,0x10003b0c,{0x10003b18,0x128,0x60,0x173},0x10003b7c,{0x10003b94,0x108,0x58,0x179},0x10003bf8},
        {8,20,0x10003c08,{0x10003c30,0xe8,0x50,0x183},0x10003c90,0x10003ca4,{0x10003cb0,0xc8,0x48,0x18c},0x10003d14,{0x10003d2c,0xa8,0x40,0x192},0x10003d90}};
    for(const auto& list:lists)if(!write_list(a,list))return false;
    a.at(0x10003da0);a.offset(3,1,0x3c);a.call(0x10747900);a.value(3,1);return true;
}
}
void save_records(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,save_entry,d,o);a.link();a.multiple(27);a.compare(4,0,false);a.load(29,2,-0x7bac);a.load(30,2,-0x7ba8);a.move(28,3);a.load(31,2,-0x5a08);a.load(27,2,-0x7be4);a.save_link();a.frame(608);
    write_archive(a);a.at(0x10003dac);a.pop(608);a.multiple(27,true);a.finish();
}
}
