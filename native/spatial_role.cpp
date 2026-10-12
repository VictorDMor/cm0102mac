#include "spatial_role.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::spatial_role {
namespace {
using Call = SemanticGuestCall;
constexpr std::uint32_t address(Entry entry) { return entries[unsigned(entry)]; }
void role_table(Call& a, Entry entry) {
    const auto root=address(entry), position=address(entry==Entry::previous_role?Entry::previous_position:Entry::current_position);
    a.link();a.save(31);a.save(30);a.save_link();a.frame(0x40);a.move(30,3);a.move(31,4);
    a.move(3,31);a.call(position);a.sign8(0,3);a.compare(0,0);bool valid=!a.less();
    if(valid){a.move(3,31);a.call(position);a.sign8(0,3);a.compare(0,10);valid=a.nonpositive();}
    if(!valid){a.at(root+0x44);a.value(3,0);a.jump(root+0x74);}
    else {
        a.at(root+0x4c);a.move(3,31);a.call(position);a.sign8(0,3);a.mask(4,0,0xfffffffe,false,1);
        a.load<1>(0,31,0x27);a.sign8(0,0);a.mask(3,0,0xfffffffc,false,2);
        a.offset(0,3,entry==Entry::secondary_role?0x793c:0x7934);a.indexed_load(3,30,0);a.indexed_load<2>(3,3,4,true);
    }
    a.at(root+0x74);a.pop(0x40);a.restore(31);a.restore(30);a.finish();
}
void owner_role(Call& a, Entry entry) {
    const auto root=address(entry);
    const auto target=address(entry==Entry::actor_current_role?Entry::current_role:entry==Entry::actor_previous_role?Entry::previous_role:Entry::secondary_role);
    a.link();a.move(4,3);a.save_link();a.frame(0x40);a.load(0,3,0x1a4);a.compare(0,0,false);
    if(!a.equal()){a.load(3,4,0x1a4);a.call(target);a.jump(root+0x2c);}
    else{a.at(root+0x28);a.value(3,0);}
    a.at(root+0x2c);a.pop(0x40);a.finish();
}
void register_center_cell(Call& a) {
    a.link();a.save(31);a.save(30);a.save_link();a.frame(0x40);a.store(1,0x58,3);a.move(31,4);
    a.load(3,1,0x58);a.offset(30,3,0x2eb0);a.move(3,31);a.value(4,8);a.call(address(Entry::set_row));
    a.move(3,31);a.value(4,6);a.call(address(Entry::set_column));
    a.load<1>(0,31,0x27);a.sign8(0,0);a.multiply(4,0,0x2c);a.load<1>(0,31,0x27);a.sign8(3,0);
    a.offset(0,3,0x58);a.indexed_load<1>(0,30,0);a.sign8(0,0);a.mask(0,0,0xfffffffc,false,2);
    a.add(0,4,0);a.indexed_store(31,30,0);
    a.load<1>(0,31,0x27);a.sign8(3,0);a.offset(0,3,0x58);a.indexed_load<1>(0,30,0);a.store<1>(31,0x28,0);
    a.load<1>(0,31,0x27);a.sign8(3,0);a.offset(4,3,0x58);a.indexed_load<1>(3,30,4);a.offset(0,3,1);a.indexed_store<1>(0,30,4);
    a.pop(0x40);a.restore(31);a.restore(30);a.finish();
}
void unlink_cell_membership(Call& a) {
    a.load<1>(0,31,0x27);a.sign8(0,0);a.multiply(0,0,0x2c);a.load<1>(3,31,0x27);a.sign8(3,3);
    a.offset(3,3,0x58);a.indexed_load<1>(3,30,3);a.sign8(3,3);a.offset(3,3,-1);a.mask(3,3,0xfffffffc,false,2);
    a.add(0,0,3);a.indexed_load(0,30,0);a.load<1>(3,31,0x27);a.sign8(3,3);a.multiply(4,3,0x2c);
    a.load<1>(3,31,0x28);a.sign8(3,3);a.mask(3,3,0xfffffffc,false,2);a.add(3,4,3);a.indexed_store(0,30,3);
    a.load<1>(0,31,0x27);a.sign8(3,0);a.offset(4,3,0x58);a.indexed_load<1>(3,30,4);a.offset(0,3,-1);a.indexed_store<1>(0,30,4);
    a.load<1>(0,31,0x27);a.sign8(0,0);a.multiply(3,0,0x2c);a.load<1>(0,31,0x28);a.sign8(0,0);
    a.mask(0,0,0xfffffffc,false,2);a.add(0,3,0);a.indexed_load(0,30,0);a.compare(0,0,false);
    if(!a.equal()){
        a.load<1>(0,31,0x28);a.load<1>(3,31,0x27);a.sign8(3,3);a.multiply(4,3,0x2c);
        a.load<1>(3,31,0x28);a.sign8(3,3);a.mask(3,3,0xfffffffc,false,2);a.add(3,4,3);a.indexed_load(3,30,3);a.store<1>(3,0x28,0);
    }
    a.at(0x101f1a74);
}
void unlink_mark_relations(Call& a) {
    a.move(3,31);a.call(address(Entry::marking_target));a.compare(3,0,false);
    if(!a.equal()){
        a.move(3,31);a.call(address(Entry::marking_target));a.move(4,31);a.call(0x10283540);a.sign8(0,3);a.compare(0,0);
        if(!a.equal()){a.move(3,31);a.call(address(Entry::marking_target));a.move(4,31);a.call(0x10283590);}
        a.at(0x101f1ab0);a.move(3,31);a.value(4,0);a.call(address(Entry::set_marking_target));
    }
    a.at(0x101f1abc);a.move(3,31);a.call(address(Entry::marking_count));a.sign8(0,3);a.compare(0,0);
    if(!a.equal()){
        a.value(25,0);a.jump(0x101f1b08);
        while(true){
            a.at(0x101f1b08);a.move(3,31);a.call(address(Entry::marking_count));a.sign8(3,3);a.sign8(0,25);a.compare_registers(0,3);
            if(!a.less())break;
            a.at(0x101f1ad8);a.move(3,31);a.move(4,25);a.call(address(Entry::marking_member));a.call(address(Entry::marking_target));a.compare_registers(3,31,false);
            if(a.equal()){a.move(3,31);a.move(4,25);a.call(address(Entry::marking_member));a.value(4,0);a.call(address(Entry::set_marking_target));}
            a.at(0x101f1b04);a.offset(25,25,1);
        }
        a.at(0x101f1b20);a.move(3,31);a.call(address(Entry::clear_marking_list));
    }
    a.at(0x101f1b28);
}
void inspect_role_mask(Call& a,unsigned actor,std::uint32_t mask){
    a.move(3,actor);a.call(address(Entry::actor_current_role));a.sign16(0,3);a.mask(0,0,mask);a.compare(0,0);
}
void no_replacement_diagnostic(Call& a){
    a.at(0x101f1d10);a.offset(3,23,0x45);a.value(4,0);a.value(5,0);a.offset(6,1,0x3d);a.offset(7,1,0x38);a.call(0x10739890);a.call(0x10768810);
    a.move(5,3);a.load(3,2,-0x7ba8);a.offset(4,23,0x53);a.offset(6,1,0x3d);a.offset(7,1,0x38);a.value(8,0x3abd);a.call(0x1077b720);
    a.offset(3,23,0x60);a.load(4,2,-0x7ba8);a.value(5,0);a.call(0x10769b60);a.value(0,0);a.load(3,2,-0x7bac);a.store<1>(3,0,0);a.value(3,0xffffffff);a.call(0x1076f7d0);
}
bool replacement_needed(Call& a){
    a.sign8(0,19);a.compare(0,-1);if(a.equal())return false;
    a.sign8(0,19);a.compare(0,-3);bool cached=a.equal();
    if(!cached){
        a.offset(3,28,0x10000);a.load(0,3,-0x1fe0);a.compare_registers(31,0,false);cached=a.equal();
        if(!cached){a.offset(3,28,0x10000);a.load(0,3,-0x1fe4);a.compare_registers(31,0,false);cached=a.equal();}
    }
    if(!cached)return false;
    a.at(0x101f1b60);inspect_role_mask(a,31,0x40);if(!a.equal()){a.at(0x101f1ba8);a.jump(0x101f1dac);return false;}
    inspect_role_mask(a,31,0x20);if(!a.equal()){a.at(0x101f1ba8);a.jump(0x101f1dac);return false;}
    a.move(3,28);a.load<1>(4,31,0x27);a.call(0x101da5a0);a.sign8(0,3);a.compare(0,0);
    if(!a.equal()){a.jump(0x101f1dac);return false;}
    a.at(0x101f1bac);return true;
}
bool excluded_replacement(Call& a){
    a.compare(27,0,false);if(a.equal()){a.at(0x101f1c40);return false;}
    inspect_role_mask(a,29,1);if(!a.equal())return true;
    inspect_role_mask(a,29,4);if(!a.equal())return true;
    inspect_role_mask(a,29,2);return !a.equal();
}
void score_replacement(Call& a){
    a.at(0x101f1c40);inspect_role_mask(a,29,0x40);
    if(!a.equal()){a.value(26,0x10000);a.offset(26,26,0x1170);a.jump(0x101f1cac);}
    else{
        a.at(0x101f1c64);inspect_role_mask(a,29,0x20);
        if(!a.equal()){a.value(26,0x10000);a.offset(26,26,-0x3cb0);a.jump(0x101f1cac);}
        else{
            a.at(0x101f1c88);inspect_role_mask(a,29,0x10);
            if(!a.equal()){a.value(26,15000);a.jump(0x101f1cac);}
            else{a.at(0x101f1ca8);a.value(26,0xffffec78);}
        }
    }
    a.at(0x101f1cac);inspect_role_mask(a,29,0x80);bool bonus=!a.equal();
    if(!bonus){inspect_role_mask(a,29,0x800);bonus=!a.equal();}
    if(bonus){a.at(0x101f1cdc);a.offset(26,26,5000);}
    a.at(0x101f1ce0);a.compare_registers(26,21);bool selected=a.positive();
    if(!selected){a.compare(27,0,false);selected=a.equal();}
    if(selected){a.at(0x101f1cf0);a.move(21,26);a.move(27,29);}
}
void appoint_replacement(Call& a){
    a.load<1>(22,31,0x27);a.value(24,0);a.jump(0x101f1cfc);
    while(true){
        a.at(0x101f1cfc);a.sign8(0,24);a.compare(0,20);if(!a.less())break;
        a.at(0x101f1bb8);a.sign8(0,22);a.multiply(3,0,0x2350);a.sign8(0,24);a.multiply(0,0,0x1c4);
        a.add(3,3,0);a.offset(29,3,0x311c);a.add(29,28,29);a.move(3,29);a.call(address(Entry::current_position));a.sign8(0,3);a.compare(0,0);
        if(!a.less()){
            a.compare_registers(31,29,false);
            if(!a.equal()&&!excluded_replacement(a))score_replacement(a);
        }
        a.at(0x101f1cf8);a.offset(24,24,1);
    }
    a.at(0x101f1d08);a.compare(27,0,false);if(a.equal())no_replacement_diagnostic(a);
    a.at(0x101f1d6c);a.move(3,27);a.move(4,20);a.call(0x101f1870);a.move(3,31);a.call(address(Entry::actor_current_role));a.sign16(0,3);a.compare(0,1);
    if(a.equal()){
        a.sign8(0,22);a.compare(0,0);
        if(a.equal()){a.offset(3,28,0x10000);a.store(3,-0x1fe4,27);a.jump(0x101f1dac);}
        else{a.at(0x101f1da4);a.offset(3,28,0x10000);a.store(3,-0x1fe0,27);}
    }
}
void remove_from_cell(Call& a){
    a.link();a.multiple(18);a.save_link();a.frame(0xa0);a.move(28,3);a.move(31,4);a.move(19,5);a.load(23,2,-0x52cc);a.value(27,0);a.value(21,0xffffd8f0);
    a.move(3,31);a.call(address(Entry::column));a.sign8(0,3);a.multiply(18,0,0x5c);a.move(3,31);a.call(address(Entry::row));a.sign8(0,3);
    a.multiply(0,0,0x450);a.add(3,0,18);a.offset(30,3,0xa08);a.add(30,28,30);a.move(3,31);a.call(address(Entry::current_position));a.move(20,3);
    a.sign8(0,20);a.compare(0,0);
    if(!a.less()){
        unlink_cell_membership(a);unlink_mark_relations(a);
        if(replacement_needed(a))appoint_replacement(a);
    }
    a.at(0x101f1dac);a.pop(0xa0);a.multiple(18,true);a.finish();
}
void clear_goalkeeper(Call& a){
    a.sign8(0,4);a.compare(0,0);
    if(a.equal()){a.value(0,0);a.offset(5,3,0x10000);a.store(5,-0x1fe4,0);a.jump(0x101f1e08);}
    else{a.at(0x101f1dfc);a.value(0,0);a.offset(5,3,0x10000);a.store(5,-0x1fe0,0);}
    a.at(0x101f1e08);a.finish();
}
// Both sides are inspected in original order, independently of the requested
// return side. Position/role queries remain separate calls and can observe
// distinct values under a guest callback or an aliased table.
void repair_goalkeeper(Call& a,bool away){
    const auto start=away?0x101f1fd8u:0x101f1e28u;
    const auto cached=away?-0x1fe0:-0x1fe4;
    const auto after=away?0x101f2188u:0x101f1fd8u;
    a.at(start);a.offset(3,31,0x10000);a.load(0,3,cached);a.compare(0,0,false);
    if(!a.equal()){
        a.offset(3,31,0x10000);a.load(3,3,cached);a.call(address(Entry::current_position));a.sign8(0,3);a.compare(0,0);
        if(!a.at_least()){a.value(0,0);a.offset(3,31,0x10000);a.store(3,cached,0);}
    }
    a.at(start+0x34);a.offset(3,31,0x10000);a.load(0,3,cached);a.compare(0,0,false);if(!a.equal())return;
    a.offset(30,31,away?0x546c:0x311c);a.value(28,0);a.value(29,0);a.jump(start+0xe4);
    const auto body=start+0x54, loop=start+0xe4;
    while(true){
        a.at(loop);a.sign8(0,29);a.compare(0,20);if(!a.less())break;
        a.at(body);a.offset(3,31,0x10000);a.load(0,3,cached);a.compare(0,0,false);
        if(a.equal()){
            a.move(3,30);a.call(address(Entry::current_position));a.sign8(0,3);a.compare(0,0);
            if(!a.less()){
                a.move(3,31);a.move(4,30);a.call(address(Entry::current_role));a.sign16(0,3);a.compare(0,1);
                if(a.equal()){a.offset(3,31,0x10000);a.store(3,cached,30);}
            }
        }
        a.at(body+0x44);a.move(3,31);a.move(4,30);a.call(address(Entry::previous_role));a.sign16(0,3);a.compare(0,1);
        if(a.equal()){
            a.move(3,30);a.call(address(Entry::current_position));a.sign8(0,3);a.compare(0,-2);
            if(a.equal()){
                a.load<1>(0,30,0x20);a.sign8(0,0);a.compare(0,-1);if(a.equal())a.move(28,30);
            }
        }
        a.at(body+0x84);a.offset(29,29,1);a.move(away?24:26,30);a.offset(30,30,0x1c4);
    }
    a.at(loop+0xc);a.offset(3,31,0x10000);a.load(0,3,cached);a.compare(0,0,false);if(!a.equal())return;
    a.offset(30,31,away?0x546c:0x311c);a.value(27,0);a.value(29,0);a.jump(start+0x1a4);
    const auto fallback=start+0x110, fallback_loop=start+0x1a4;
    while(true){
        a.at(fallback_loop);a.sign8(0,29);a.compare(0,20);if(!a.less())break;
        a.at(fallback);a.move(3,30);a.call(address(Entry::current_position));a.sign8(0,3);a.compare(0,0);
        if(!a.less()){
            a.compare(28,0,false);
            if(!a.equal()){
                a.move(3,28);a.call(address(Entry::previous_position));a.move(27,3);a.move(3,30);a.call(address(Entry::current_position));a.move(4,3);a.move(3,28);a.call(address(Entry::set_previous_position));
            }
            a.at(fallback+0x3c);a.offset(3,31,0x10000);a.store(3,cached,30);a.offset(3,31,0x10000);a.load(3,3,cached);
            a.load<1>(0,3,0x27);a.sign8(0,0);a.mask(3,0,0xfffffffc,false,2);a.offset(0,3,0x7934);a.indexed_load(3,31,0);
            a.sign8(0,27);a.mask(0,0,0xfffffffe,false,1);a.indexed_load<2>(0,3,0,true);a.compare(0,1);
            if(!a.equal())return;
            a.offset(3,31,0x10000);a.load(3,3,cached);a.move(4,27);a.call(0x101f1870);a.jump(after);return;
        }
        a.at(fallback+0x88);a.offset(29,29,1);a.move(away?23:25,30);a.offset(30,30,0x1c4);
    }
}
void resolve_goalkeeper(Call& a){
    a.link();a.multiple(23);a.save_link();a.frame(0x60);a.move(31,3);a.store<1>(1,0x7f,4);
    repair_goalkeeper(a,false);repair_goalkeeper(a,true);
    a.at(0x101f2188);a.load<1>(0,1,0x7f);a.sign8(0,0);a.compare(0,1);
    if(a.equal()){a.offset(3,31,0x10000);a.load(3,3,-0x1fe0);a.jump(0x101f21ac);}
    else{a.at(0x101f21a4);a.offset(3,31,0x10000);a.load(3,3,-0x1fe4);}
    a.at(0x101f21ac);a.pop(0x60);a.multiple(23,true);a.finish();
}
}
void invoke(State& state,Entry entry,DependencyDispatch* dependencies){
    if(unsigned(entry)>=entries.size())throw std::runtime_error("Unknown spatial-role entry");
    Call a(state,address(entry),dependencies);
    switch(entry){
    case Entry::current_position:a.load<1>(3,3,0x19);break;
    case Entry::column:a.load<1>(3,3,0x106);break;
    case Entry::row:a.load<1>(3,3,0x105);break;
    case Entry::marking_target:a.load(3,3,0x1b0);break;
    case Entry::set_marking_target:a.store(3,0x1b0,4);break;
    case Entry::clear_marking_list:a.value(0,0);a.store<1>(3,0x104,0);break;
    case Entry::marking_count:a.load<1>(3,3,0x104);break;
    case Entry::current_role:case Entry::secondary_role:case Entry::previous_role:role_table(a,entry);return;
    case Entry::previous_position:a.load<1>(3,3,0x1a);break;
    case Entry::register_center_cell:register_center_cell(a);return;
    case Entry::set_column:a.store<1>(3,0x106,4);break;
    case Entry::set_row:a.store<1>(3,0x105,4);break;
    case Entry::remove_from_cell:remove_from_cell(a);return;
    case Entry::marking_member:a.sign8(0,4);a.mask(5,0,0xfffffffc,false,2);a.offset(0,5,0x1b4);a.indexed_load(3,3,0);break;
    case Entry::clear_goalkeeper:clear_goalkeeper(a);return;
    case Entry::resolve_goalkeeper:resolve_goalkeeper(a);return;
    case Entry::set_previous_position:a.store<1>(3,0x1a,4);break;
    case Entry::cached_role:a.load<2>(3,3,0x1c2,true);break;
    case Entry::actor_current_role:case Entry::actor_previous_role:case Entry::actor_secondary_role:owner_role(a,entry);return;
    }
    a.finish();
}
}
