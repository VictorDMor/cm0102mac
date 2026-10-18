#include "shortlist_category.hpp"
#include "club_shortlist_storage_detail.hpp"

namespace cm::aot::shortlist_category {
namespace {
using Call=SemanticGuestCall;
struct Qualification {std::uint32_t first;unsigned role,side;};
// These are field layouts for the fifteen original qualification cases, not
// category labels. The live guest table selects the destination, preserving CTR.
constexpr std::array qualifications{
    Qualification{0x10365d74,15,0},Qualification{0x10365d8c,16,25},
    Qualification{0x10365db4,17,24},Qualification{0x10365ddc,17,25},
    Qualification{0x10365e04,17,23},Qualification{0x10365e2c,22,24},
    Qualification{0x10365e54,18,25},Qualification{0x10365e7c,22,23},
    Qualification{0x10365ea4,19,24},Qualification{0x10365ecc,19,25},
    Qualification{0x10365ef4,19,23},Qualification{0x10365f1c,20,24},
    Qualification{0x10365f44,20,25},Qualification{0x10365f6c,20,23},
    Qualification{0x10365f94,21,0}};
bool qualified(Call& a,const Qualification& q,std::uint32_t target) {
    // Preserve aligned partial destinations within a qualification case too.
    const auto part=target-q.first;a.at(target);
    if(part<16){
        if(part==0)a.load<1>(0,3,q.role);
        if(part<=4)a.sign8(0,0);
        if(part<=8)a.compare(0,15);
        if(a.less())return false;
    }
    const unsigned accepted=q.side?32:16;
    if(q.side&&part<32){
        if(part<=16)a.load<1>(0,3,q.side);
        if(part<=20)a.sign8(0,0);
        if(part<=24)a.compare(0,15);
        if(a.less())return false;
    }
    if(part<=accepted)a.value(3,1);a.jump(0x10366088);return true;
}
void staff_category(Call& a) {
    a.at(0x10365fac);a.load(3,29,0x24);a.compare(3,0,false);
    if(!a.equal()){
        a.load<1>(0,29,0x62);a.sign8(0,0);a.compare(0,11);bool check=a.equal();
        if(!check){a.compare(0,15);check=a.equal();if(!check){a.compare(0,13);check=a.equal();}}
        if(check){
            a.at(0x10365fd8);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);
            if(!a.equal()){a.value(3,0);a.jump(0x10366088);return;}
        }
    }
    a.at(0x10365ffc);a.load(0,29,0x1c);a.compare(0,0,false);
    if(!a.equal()){
        a.load(3,2,-0x7be4);a.load(4,29,0);a.load(3,3,0);a.offset(0,3,-16);a.compare_registers(4,0);
        if(!a.at_least()){a.value(3,1);a.jump(0x10366088);return;}
    }
    a.at(0x10366028);a.value(3,0);a.jump(0x10366088);
}
}
void eligible_for_category(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,eligible_for_category_entry,d,o);a.link();const auto caller=a.r(0);a.save(31);a.load(31,2,-0x4dd8);a.save(30);a.load(30,2,-0x7ba8);a.save(29);a.move(29,4);a.save_link();a.frame(0x70);a.load(3,4,0x14);a.compare(3,0,false);
    bool inspect=!a.equal();
    if(!inspect){a.sign8(0,5);a.compare(0,15);inspect=a.equal();if(!inspect){a.value(3,0);a.jump(0x10366088);}}
    if(inspect){
        a.at(0x10365d54);a.sign8(0,5);a.compare(0,15,false);
        if(a.positive()){
            club_shortlist_storage::detail::diagnostic(a,0x10366030,31,30,0x40,0x38,0x1c2a);a.load(3,2,-0x7bac);a.value(0,0);a.store<1>(3,0,0);a.value(3,0);
        }else{
            a.load(4,2,-0x4df4);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(4,4,0);a.count_destination(4);const auto target=a.count_branch();
            if(target==0x10365fac)staff_category(a);
            else if(target==0x10366084){a.at(target);a.value(3,0);}
            else if(target!=0x10366088){
                const auto found=std::find_if(qualifications.begin(),qualifications.end(),[&](const auto& q){return target>=q.first&&target<q.first+(q.side?40:24)&&!(target&3);});
                if(found==qualifications.end()){a.resume(target,caller);return;}
                if(!qualified(a,*found,target)){a.at(0x10366084);a.value(3,0);}
            }
        }
    }
    a.at(0x10366088);a.pop(0x70);a.restore(31);a.restore(30);a.restore(29);a.finish();
}
}
