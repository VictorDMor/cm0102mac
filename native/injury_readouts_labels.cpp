#include "injury_readouts.hpp"
#include "injury_readouts_detail.hpp"
#include "injury_readouts_choices.hpp"
#include <algorithm>

namespace cm::aot::injury_readouts {
namespace {
using namespace detail;
template<std::size_t N>
bool write_label(Call& a,const std::array<LabelChoice,N>& choices,std::uint32_t target,unsigned output,unsigned strings,std::uint32_t end) {
    const auto found=std::find_if(choices.begin(),choices.end(),[&](const auto& c){return target>=c.source && target<c.source+16 && !(target&3);});
    if(found==choices.end())return false;
    // Preserve changed-table destinations at a label's argument/call boundaries.
    a.at(target);
    if(target<=found->source)a.move(3,output);
    if(target<=found->source+4)a.offset(4,strings,found->token);
    if(target<=found->source+8)a.call(0x10727890);
    a.jump(end);return true;
}
bool write_type_choice(Call& a,std::uint32_t target,std::uint32_t caller) {
    if(target==0x1018d12c)return true;
    if(write_label(a,type_labels,target,27,30,0x1018d12c))return true;
    if(target==0x1018cb3c){report_invalid(a,target,30,29,0x70,0x40,0xed5);a.value(0,0);a.store<1>(31,0,0);a.jump(0x1018d12c);return true;}
    if(target==0x1018d0dc){report_invalid(a,target,30,29,0x50,0x38,0xf32);a.value(0,0);a.store<1>(31,0,0);return true;}
    a.resume(target,caller);return false;
}
}

void describe_injury_type(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,type_text_entry,d,o);
    a.link();const auto caller=a.r(0);a.multiple(26);a.compare(6,30);a.load(31,2,-0x7bac);a.load(29,2,-0x7ba8);a.move(26,4);a.load(30,2,-0x5438);a.move(27,5);a.move(28,7);a.save_link();a.frame(208);
    if(!a.at_least()) {
        report_invalid(a,0x1018c590,30,29,0x90,0x48,0xe76);a.value(0,0);a.store<1>(31,0,0);a.jump(0x1018d12c);
    } else {
        a.at(0x1018c5e4);a.sign8(0,28,true);bool ordinary=a.equal();
        if(!ordinary){a.call(0x10727880);a.sign8(0,3);a.compare(0,3);ordinary=a.equal();}
        std::uint32_t target=0x1018cb3c;
        if(ordinary) {
            a.at(0x1018c5fc);a.sign8(0,26);a.compare(0,81,false);
            if(a.nonpositive()){a.load(3,2,-0x5464);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(3,3,0);a.count_destination(3);target=a.count_branch();}
        } else {
            a.at(0x1018cb90);a.sign8(0,28);a.compare(0,1);
            if(!a.equal()){a.at(0x1018d12c);a.pop(208);a.multiple(26,true);a.finish();return;}
            a.at(0x1018cb9c);a.sign8(0,26);a.compare(0,81,false);target=0x1018d0dc;
            if(a.nonpositive()){a.load(3,2,-0x5468);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(3,3,0);a.count_destination(3);target=a.count_branch();}
        }
        if(!write_type_choice(a,target,caller))return;
    }
    a.at(0x1018d12c);a.pop(208);a.multiple(26,true);a.finish();
}

void describe_injury_category(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,category_text_entry,d,o);
    a.link();const auto caller=a.r(0);a.save(31);a.compare(6,20);a.load(31,2,-0x5438);a.save(30);a.load(30,2,-0x7ba8);a.save(29);a.load(29,2,-0x7bac);a.save_link();a.frame(160);
    if(!a.at_least()) {
        report_invalid(a,0x1018dadc,31,30,0x68,0x40,0x10b9);a.value(0,0);a.value(3,0);a.store<1>(29,0,0);a.jump(0x1018dc94);
    } else {
        a.at(0x1018db34);a.sign8(0,4);a.compare(0,12,false);std::uint32_t target=0x1018dc24;
        if(a.nonpositive()){a.load(3,2,-0x5478);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(3,3,0);a.count_destination(3);target=a.count_branch();}
        if(target!=0x1018dc74 && !write_label(a,category_labels,target,5,31,0x1018dc74)) {
            if(target==0x1018dc24){report_invalid(a,target,31,30,0x48,0x38,0x10d0);a.value(0,0);a.store<1>(29,0,0);}
            else{a.resume(target,caller);return;}
        }
        a.at(0x1018dc74);a.call(0x10727880);a.sign8(3,3);a.value(0,2);a.offset(4,3,-3);a.subtract_from(3,3,3);a.complement_union(3,4,3);a.shift_right(3,3,31);a.intersect(3,0,3);
    }
    a.at(0x1018dc94);a.pop(160);a.restore(31);a.restore(30);a.restore(29);a.finish();
}
}
