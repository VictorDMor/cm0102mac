// Queue, label and effect routines run together, with scripted external policy,
// formatting and club-classification boundaries.
#include "news_adapter.hpp"
#include "news_detail.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <iostream>
#include <map>

using namespace cm::aot;
using namespace cm::aot::guest;
namespace cm::aot { bool news_reference(State&); }
namespace {
constexpr std::uint32_t data=0x63000000, queue=data+0x1000, source=data+0x2000,
    count_at=data+0x3000, people=data+0x10000, inboxes=data+0x20000, clubs=data+0x30000,
    target=data+0x38000, home=data+0x39000, region=data+0x3a000, entity=data+0x3b000,
    records=data+0x40000, moved=data+0xa0000, log_at=data+0x100000,
    globals=data+0x110000, game=data+0x120000, copied=data+0x200000,
    stack=0x74010000, sentinel=0xfffffff0;
constexpr std::uint32_t manager(unsigned index=0) { return people+(16+index)*0x70; }
struct Scenario {
    std::uint32_t entry; std::string name;
    bool null_item=false, null_manager=false, null_target=false, unassigned=false,
        allocation_failure=false, all_unassigned=false, duplicate=false, relocate_action=false,
        no_primary=false, secondary=false, same_clubs=false, real_group=false, duplicate_assigned=false, change_next=false, zero_records=false,primary_continent_miss=false;
    unsigned retained=3, next=3, kind=4, routes=0, active=1, selected=0, fail_allocation=0;
    std::uint32_t id=17, read=0, priority=0, action=0, policy=0, force_policy=0, filter=0, propagation=0, unread=3;
    std::uint32_t last_id=99;
    int manager_delta=0, match=1;
};
Scenario active{0,""};
bool reference_mode=false;
unsigned allocations=0, action_calls=0, label_calls=0;
std::uint32_t allocation_size=0;
void trace(Cpu& c,std::uint32_t target_address) {
    auto size=c.u32(log_at);
    const auto add=[&](std::uint32_t v){ c.put32(log_at+4+size++*4,v); };
    add(target_address); add(c.lr());
    for(unsigned r=3;r<11;++r) add(c.r(r));
    for(unsigned r=13;r<32;++r) add(c.r(r));
    for(unsigned cr=0;cr<8;++cr) add(c.cr(cr));
    c.put32(log_at,size);
}
bool probe(State& s) {
    if(run_leaf(s)) return true;
    const auto at=s.pc;
    if(reference_mode && at>=0x102bdf90 && at<0x102ce710) return news_reference(s);
    if(news_lists(at)) { news_run_rewrite(s,at); return true; }
    if(at==0x1073ffe0 && active.real_group) return false;
    if(at!=0x107751b0 && at!=0x10775130 && at!=0x10147e70 && at!=0x1073ffe0 &&
        at!=0x100c7d00 && at!=0x10727890 && at!=0x10149df0 && at!=0x101b0100 &&
        at!=0x10739890 && at!=0x10768810 && at!=0x1077b720 && at!=0x10769b60) return false;
    Cpu c(s); trace(c,at);
    if(at==0x107751b0 || at==0x10775130) {
        ++allocations;
        const bool failure=active.allocation_failure || allocations==active.fail_allocation;
        const auto amount=at==0x107751b0?c.r(4):c.r(3);
        if(!failure) {
            const auto begin=at==0x107751b0?allocation_size:0;
            const auto destination=at==0x107751b0?records:copied;
            for(unsigned n=begin;n<amount;++n) c.put8(destination+n,0xcc);
            c.set(3,destination); allocation_size=amount;
        } else c.set(3,0);
    } else if(at==0x10727890) {
        ++label_calls; c.put32(c.r(3),0x4143544e);c.put8(c.r(3)+4,0);c.set(3,4);
    } else if(at==0x100c7d00) {
        ++action_calls;
        if(active.change_next&&action_calls==1)c.put32(inboxes+4,2);
        if(active.relocate_action && action_calls==1) {
            const auto bytes=c.s.memory.copy_out(records,1700*0xe4);
            for(unsigned n=0;n<bytes.size();++n) c.put8(moved+n,bytes[n]);
            c.put32(queue,moved); c.put32(inboxes+8,7);
            c.put32(manager()+0x20,clubs+0x248);
        }
        c.set(3,0);
    } else if(at==0x10147e70) c.set(3,active.policy);
    else if(at==0x10149df0||at==0x101b0100)c.set(3,1);
    else if(at==0x1073ffe0) c.set(3,active.routes==7?0xffffffff:c.r(4)==manager()?2:active.routes==6?3:2);
    else if(at==0x10739890) { c.put32(c.r(6),0x4e455753); c.put32(c.r(7),0x43505000); }
    else if(at==0x10768810) c.set(3,globals+0x300);
    else if(at==0x1077b720) c.put32(c.r(3),c.r(8));
    else c.set(3,0x2345);
    s.pc=c.lr(); return true;
}
void initialize(Cpu& c) {
    c.s.memory.map(data,0x400000); c.s.memory.map(stack-0x10000,0x20000);
    c.s.memory.copy_in(data,std::vector<std::uint8_t>(0x400000,0xa5));
    c.s.memory.copy_in(stack-0x10000,std::vector<std::uint8_t>(0x20000,0xa5));
    for(unsigned r=0;r<32;++r) c.set(r,0x56000000+r*0x101);
    for(unsigned cr=0;cr<8;++cr) c.set_cr(cr,cr+1);
    c.set(1,stack); c.set(2,0x108bba90); c.set_lr(sentinel);
    c.put32(log_at,0); allocations=action_calls=label_calls=0; allocation_size=1700*0xe4;
    c.put32(c.r(2)-0x7ba8,globals); c.put32(c.r(2)-0x7bac,globals+0x100); c.put8(globals+0x100,1);
    c.put32(c.r(2)-0x7be4,globals+0x104); c.put32(globals+0x104,32);
    c.put32(c.r(2)-0x7be0,globals+0x108); c.put32(globals+0x108,people);
    c.put32(c.r(2)-0x7be8,globals+0x10c); c.put32(globals+0x10c,clubs);
    c.put32(c.r(2)-0x7bb8,globals+0x1000); c.put32(c.r(2)-0x7bb0,game);
    c.put16(game+0x2581ac,active.force_policy); c.put32(c.r(2)-0x7a54,globals+0x110); c.put32(globals+0x110,game);
    c.put32(c.r(2)-0x79f0,globals+0x114);c.put32(globals+0x114,game);
    c.put32(queue,records); c.put32(queue+4,1700); c.put32(queue+8,inboxes); c.put32(queue+12,32); c.put32(queue+16,active.last_id);
    c.put32(count_at,0xdeadbeef);
    c.put32(entity+8,active.routes&1?home:0); c.put32(entity+4,active.routes&2?region:0);
    c.put32(home+0x74,active.primary_continent_miss?region+4:region);c.put32(entity+0x74,region);
    for(unsigned i=0;i<16;++i) {
        const auto person=manager(i),club=clubs+i*0x248;
        c.put32(person,16+i); c.put8(person+0x6b,i==active.selected||active.selected==16?active.active:0);
        c.put32(person+0x24,active.no_primary?0:club); c.put32(person+0x20,active.secondary?(active.same_clubs?club:clubs+0x248):0);
        c.put32(person+0xc,active.routes==4?(active.primary_continent_miss?entity:home):0);
        c.put32(club+0x30,active.routes==1?target:0); c.put32(club+0x2c,active.primary_continent_miss||active.routes==2||active.routes==3?home:0);
        c.put32(club+0x90,person);
        c.put32(inboxes+i*12,active.all_unassigned||(!i&&active.unassigned)?0xffffffff:i*100);
        c.put32(inboxes+i*12+4,i*100+active.next); c.put32(inboxes+i*12+8,active.unread);
    }
    c.put32(inboxes+16*12,1600); c.put32(inboxes+16*12+4,1603); c.put32(inboxes+16*12+8,active.unread);
    c.put32(manager(),16+active.manager_delta);
    if(active.null_manager) c.put32(clubs+0x90,0);
    c.put32(clubs+0x248+0x2c,home);
    if(active.all_unassigned) { c.put32(queue+4,0); allocation_size=0; }
    for(unsigned i=0;i<1700;++i) {
        const auto item=records+i*0xe4;
        c.put32(item,active.action?(active.action==1?5000:6004):i%2?2001:1); c.put8(item+4,active.priority);
        c.put32(item+0xc,0);
        c.put32(item+0xdc,i%100<active.retained?1000+i:0); c.put8(item+0xe0,active.read);
    }
    if(active.match>=0) c.put32(records+unsigned(active.match)*0xe4+0xdc,active.id);
    if(active.zero_records){c.s.memory.map(0,0x6000);c.s.memory.copy_in(0,c.s.memory.copy_out(records,100*0xe4));c.put32(queue,0);}
    c.put32(source,1); c.put8(source+4,0xff); c.put8(source+0xe0,1); c.put8(source+0xe1,0xaa); c.put32(source+0xdc,active.duplicate?1002:active.id);
    if(active.duplicate) c.put32(records+(active.next?active.next-1:99)*0xe4+0xdc,c.u32(source+0xdc));
    if(active.duplicate_assigned) c.put32(records+(active.next?active.next-1:99)*0xe4+0xdc,active.last_id+1);
    c.put16(game+0x2c58,0); c.put32(game+4,manager());
    for(unsigned n=0;n<16;++n) c.put16(game+0x2c38+n*2,0);
    c.put16(game+0x2c38,16);
    for(unsigned n=0;n<16;++n) c.put32(game+4+n*0x2c,manager(n));
    c.set({{3,queue},{4,active.null_item?0:source},{5,active.null_target?0:clubs},
           {6,active.kind==1?target:entity},{7,home},{8,region},{9,active.kind}});
    if(active.entry==0x102cbc10) c.set(5,active.null_manager?0:manager());
    if(active.entry==0x102cc280) c.set(5,active.null_target?0:target);
    if(active.entry==0x102cc400||active.entry==0x102cc5b0) c.set(5,active.null_target?0:home);
    if(active.entry==0x102cc730) c.set(5,active.null_target?0:region);
    if(active.entry==0x102cc9e0) c.set(5,active.null_target?0:entity);
    if(active.entry>=0x102ccb20) {
        c.set(4,active.null_manager?0:manager()); c.set(5,active.id); c.set(6,active.propagation);
        if(active.entry==0x102ccb20) { c.set(5,count_at); c.set(6,active.filter); }
        if(active.entry==0x102cd8b0) c.set(5,active.filter);
        if(active.entry==0x102cda60) c.set(5,clubs);
    }
    c.s.budget=20000000;
}
void returned(State& s) {
    Cpu c(s); using cm::semantic_io_checks::require;
    require(c.sp()==stack&&c.lr()==sentinel&&s.pc==sentinel,"queue routine did not return");
    if(active.entry==0x102cdc80&&!active.null_manager&&active.manager_delta>=0&&!active.relocate_action) {
        require(c.u32(inboxes+8)==0&&c.i32(inboxes+4)==(active.unassigned?-1:0),"cleared inbox header");
        if(!active.unassigned) for(unsigned i=0;i<100;++i) require(c.u32(records+i*0xe4+0xdc)==0&&c.u8(records+i*0xe4+0xe0)==1,"cleared news IDs/read flags");
    }
    if(active.entry==0x102cd5f0&&!active.null_manager&&active.manager_delta>=0&&active.id&&!active.unassigned&&!active.zero_records)
        require(c.r(3)==1,"assigned inbox acknowledgment result, including absent ID");
}
void original(State& s,std::uint32_t entry) { reference_mode=true; s.pc=entry; run_until(s,sentinel); returned(s); }
void rewrite(State& s,std::uint32_t entry) { reference_mode=false; news_run_rewrite(s,entry); returned(s); }
}
namespace cm::aot {
unsigned check_news_queue(const State& base) {
    struct Restore { bool(*previous)(State&); ~Restore(){dispatch_probe=previous;} } restore{dispatch_probe};
    dispatch_probe=probe; std::vector<Scenario> cases;
    const auto add=[&](Scenario test){cases.push_back(std::move(test));};
    for(auto entry:{0x102cbc10u,0x102cc140u,0x102cc280u,0x102cc400u,0x102cc5b0u,0x102cc730u,0x102cc8e0u,0x102cc9e0u}) {
        add({entry,"normal"}); Scenario no_item{entry,"null item"}; no_item.null_item=true; add(no_item);
        Scenario no_target{entry,"null target"}; no_target.null_target=true; no_target.null_manager=true; add(no_target);
        for(unsigned route=0;route<5;++route) for(unsigned selected:{0u,15u,16u}) {
            Scenario test{entry,"recipient route"}; test.routes=route; test.selected=selected; test.secondary=true; add(test);
        }
        for(unsigned status:{0u,2u,255u}) { Scenario test{entry,"inactive recipients"}; test.active=status; test.selected=16; add(test); }
    }
    for(unsigned kind:{0u,1u,2u,3u,4u,5u,6u,127u,128u,255u,256u,257u}) {
        Scenario test{0x102cc030,"dispatcher"}; test.kind=kind; test.selected=16; test.routes=3; add(test);
    }
    for(unsigned next:{0u,1u,99u,100u,101u}) for(unsigned read:{0u,1u,255u}) {
        Scenario test{0x102cbc10,"ring position / read byte"}; test.next=next; test.read=read; add(test);
        test.duplicate=true; test.name="duplicate prior item"; add(test);
    }
    for(int delta:{-1,0,15,16}) { Scenario test{0x102cbc10,"manager index"}; test.manager_delta=delta; add(test); }
    for(auto entry:{0x102cbc10u,0x102cc140u,0x102cc730u}) {
        Scenario test{entry,"no primary club / manager nation route"}; test.no_primary=true; test.force_policy=1; test.routes=4; add(test);
    }
    for(int delta:{-1,0}) { Scenario test{0x102cc140,"direct club manager threshold"}; test.manager_delta=delta; add(test); }
    Scenario managerless_club{0x102cc140,"club with no manager"}; managerless_club.null_manager=true; add(managerless_club);
    Scenario two_contexts{0x102cbc10,"same primary/secondary club"}; two_contexts.secondary=true; two_contexts.same_clubs=true; two_contexts.force_policy=1; add(two_contexts);
    for(auto last:{0u,99u,0xfffffffeu,0xffffffffu}) for(bool duplicate:{false,true}) {
        Scenario test{0x102cbc10,"assign identifier / counter wrap / duplicate"}; test.id=0; test.last_id=last; test.duplicate_assigned=duplicate; add(test);
    }
    for(unsigned policy:{0u,1u,255u,256u}) for(unsigned force:{0u,1u}) {
        Scenario test{0x102cbc10,"action policy"}; test.policy=policy; test.force_policy=force; test.secondary=true; add(test);
    }
    Scenario moved_case{0x102cbc10,"action relocates item array and unread"}; moved_case.force_policy=1; moved_case.relocate_action=true; add(moved_case);
    for(bool failure:{false,true}) { Scenario test{0x102cbc10,"allocate ring"}; test.unassigned=true; test.allocation_failure=failure; add(test); }
    for(unsigned step:{1u,8u,16u}) { Scenario test{0x102cc8e0,"publication allocation failure"}; test.all_unassigned=true; test.selected=16; test.fail_allocation=step; add(test); }
    for(auto entry:{0x102ccb20u,0x102cd5f0u,0x102cd8b0u,0x102cda60u,0x102cdc80u}) {
        add({entry,"normal queue state"}); Scenario empty{entry,"unassigned inbox"}; empty.unassigned=true; add(empty);
        Scenario absent{entry,"absent id"}; absent.match=-1; add(absent);
        Scenario bad{entry,"negative manager index"}; bad.manager_delta=-1; bad.filter=entry==0x102cd8b0?1:0; add(bad);
        Scenario null{entry,"null manager"}; null.null_manager=true; add(null);
        for(unsigned retained:{0u,1u,2u,99u,100u}) for(unsigned next:{0u,1u,99u,100u}) {
            // A full ring's next must be in [first,first+99]. next=100 is
            // still tested with a hole; without one the original never ends.
            if(retained==100&&next==100&&entry!=0x102cd5f0) continue;
            Scenario test{entry,"reverse ring / forward acknowledgement"}; test.retained=retained; test.next=next; test.match=-1; test.filter=entry==0x102cd8b0?1:0; add(test);
        }
    }
    for(unsigned filter:{0u,1u,2u,3u,7u,255u,256u,258u}) {
        Scenario test{0x102ccb20,"category filter"}; test.filter=filter; add(test);
    }
    Scenario no_copy{0x102ccb20,"copied array allocation failure"}; no_copy.allocation_failure=true; add(no_copy);
    // The original classifier explicitly returns only 0,1,2 on all paths.
    for(unsigned action:{0u,1u,2u}) for(unsigned read:{0u,1u,255u}) {
        Scenario test{0x102cd5f0,"classification / read"}; test.action=action; test.read=read; add(test);
        test.entry=0x102cd8b0; test.filter=1; test.priority=1; add(test);
    }
    for(unsigned priority:{0u,1u,255u}) for(unsigned read:{0u,1u,255u}) {
        Scenario test{0x102cd8b0,"priority byte / read attention"}; test.filter=1; test.priority=priority; test.read=read; add(test);
    }
    for(unsigned unread:{0u,1u,0x7fffffffu,0x80000000u}) { Scenario test{0x102cd5f0,"wrapping unread decrement"}; test.unread=unread; add(test); }
    for(unsigned propagation:{0u,1u,256u,257u}) for(unsigned group:{0u,6u,7u}) for(bool real:{false,true}) {
        Scenario test{0x102cd5f0,"acknowledgment propagation"}; test.propagation=propagation; test.routes=group; test.selected=16; test.real_group=real; add(test);
        test.match=-1; test.name="propagate absent identifier"; add(test);
    }
    for(bool secondary:{false,true}) for(bool equal:{false,true}) {
        Scenario test{0x102cdc80,"clear primary/secondary contexts"}; test.secondary=secondary; test.same_clubs=equal; add(test);
    }
    Scenario zero_ack{0x102cd5f0,"zero identifier assertion"};zero_ack.id=0;add(zero_ack);
    for(unsigned status:{0u,2u,255u}){
        Scenario test{0x102cd5f0,"propagation inactive/status guard"};test.propagation=1;test.selected=16;test.active=status;add(test);
    }
    Scenario deep_empty{0x102cd8b0,"deep unassigned inbox"};deep_empty.filter=1;deep_empty.unassigned=true;add(deep_empty);
    for(auto unread:{0u,1u,0xffffffffu,0x80000000u}){Scenario test{0x102cd8b0,"quick unread positivity/clamp"};test.unread=unread;add(test);}
    for(bool secondary:{false,true}){
        Scenario test{0x102cdc80,"clear absent primary club"};test.no_primary=true;test.secondary=secondary;add(test);
    }
    for(auto entry:{0x102cda60u,0x102cdc80u}){
        Scenario test{entry,"callee relocates array/changes secondary"};test.relocate_action=true;add(test);
        test.relocate_action=false;test.change_next=true;test.name="callee changes live inbox cursor";add(test);
    }
    for(unsigned route:{0u,4u}){Scenario test{0x102cc730,"primary continent mismatch / nationality fallback"};test.routes=route;test.primary_continent_miss=true;add(test);}
    Scenario no_comp_club{0x102cc280,"competition manager lacks primary club"};no_comp_club.no_primary=true;add(no_comp_club);
    // Explicit mapped-page-zero characterization, outside allocator-backed arrays.
    Scenario zero_array{0x102cd5f0,"mapped zero item-array base"};zero_array.zero_records=true;zero_array.match=0;zero_array.action=1;add(zero_array);
    unsigned failures=0; std::map<std::uint32_t,unsigned> returns;
    for(const auto& test:cases) {
        active=test; auto state=cm::semantic_io_checks::clone(base); Cpu c(*state); initialize(c);
        const auto difference=compare_routine_call(*state,test.entry,original,rewrite,RoutineChecker::result_r3);
        if(!difference.empty()) {
            ++failures; std::cout<<"FAIL queue "<<hex_address(test.entry)<<' '<<test.name<<" routes="<<test.routes<<" selected="<<test.selected<<" next="<<test.next<<" action="<<test.action<<'\n';
            for(const auto& line:difference) std::cout<<line<<'\n';
        } else ++returns[test.entry];
    }
    active=Scenario{0x102cbc10,"negative"}; auto state=cm::semantic_io_checks::clone(base); Cpu c(*state); initialize(c);
    if(compare_routine_call(*state,active.entry,original,rewrite,RoutineChecker::result_r3,true).empty()) ++failures;
    else std::cout<<"ok queue corrupted result detected\n";
    for(const auto& [entry,count]:returns) std::cout<<hex_address(entry)<<": "<<count<<" returned queue comparisons\n";
    std::cout<<cases.size()<<" queue comparisons with real owned actions, "<<failures<<" failed\n";
    return failures;
}
}
