#include "injury_readouts.hpp"
#include "injury_readouts_detail.hpp"
#include "injury_readouts_choices.hpp"
#include <algorithm>

namespace cm::aot::injury_readouts {
namespace {
using namespace detail;
bool choose_form(Call& a,std::uint32_t target,std::uint32_t caller) {
    if(target==0x1018d938)return true;
    if(target==0x1018d934){a.at(target);a.value(3,0);return true;}
    const auto choice=std::find_if(type_forms.begin(),type_forms.end(),[&](const auto& c){return target==c.source || target==c.source+4;});
    if(choice!=type_forms.end()){a.at(target);if(target==choice->source)a.value(3,choice->value);a.jump(0x1018d938);return true;}
    a.resume(target,caller);return false;
}
std::uint32_t lookup_form(Call& a,std::uint32_t start,std::int32_t slot,std::uint32_t fallback) {
    a.at(start);a.sign8(0,31);a.compare(0,76,false);
    if(!a.nonpositive())return fallback;
    a.load(3,2,slot);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(3,3,0);a.count_destination(3);return a.count_branch();
}
}
void injury_type_form(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,type_form_entry,d,o);
    a.link();const auto caller=a.r(0);a.save(31);a.move(31,4);a.save_link();a.frame(64);a.call(0x10727880);a.sign8(0,3);a.compare(0,9);
    std::uint32_t target=0x1018d934;
    if(a.equal())target=lookup_form(a,0x1018d164,-0x546c,0x1018d3ec);
    else {
        a.at(0x1018d3f4);a.call(0x10727880);a.sign8(0,3);a.compare(0,3);
        if(a.equal())target=lookup_form(a,0x1018d404,-0x5470,0x1018d68c);
        else {
            a.at(0x1018d694);a.call(0x10727880);a.sign8(0,3);a.compare(0,5);
            if(a.equal())target=lookup_form(a,0x1018d6a4,-0x5474,0x1018d92c);
        }
    }
    if(!choose_form(a,target,caller))return;
    a.at(0x1018d938);a.pop(64);a.restore(31);a.finish();
}
}
