// Original/rewrite action-label calls with explicit domain boundaries.
// These fixtures close the caller's decisions and ABI, not the internals
// of the scripted transfer/competition/date/decoder dependencies.
#include "news_adapter.hpp"
#include "news_detail.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <iostream>

using namespace cm::aot;
using namespace cm::aot::guest;
namespace cm::aot { bool news_reference(State&); }
namespace {
constexpr std::uint32_t data=0x64000000,item=data+0x1000,manager=data+0x2000,
    output=data+0x3000,callback=data+0x4000,log_at=data+0x8000,globals=data+0x18000,
    clubs=data+0x20000,people=data+0x30000,offer_slot=data+0x40000,offer=data+0x41000,
    secondary_slot=data+0x42000,secondary=data+0x43000,contract=data+0x44000,
    list=data+0x45000,list_entry=data+0x46000,competition=data+0x50000,
    stage_table=data+0x51000,rounds=data+0x52000,ties=data+0x54000,
    queue=data+0x60000,inboxes=data+0x61000,records=data+0x62000,
    stack=0x75010000,sentinel=0xfffffff0;
struct Scenario {
    unsigned type=1;
    std::array<std::uint32_t,50> payload{};
    bool no_manager=false,no_output=false,no_callback=false,no_club=false,
        missing_offer=false,stale_key=false,no_contract=false,no_list=false,
        employer_route=false,list_miss=false,special_club=false,decode_extra=false;
    unsigned capacity=2000,offer_status=0,helper_status=1,contract_status=3,offer_flag=0,eligibility=1;
    unsigned offer_byte10=0,offer_byte36=0,offer_byte37=0,item_day=7,nomination_count=1;
    std::uint32_t decoded_manager=0,decoded_club=0,decoded_competition=0,secondary_key=456;
    bool effects=false,no_associated_club=false,empty_selection=false;
    unsigned policy=1,player_attribute=40,manager_attribute=0,contract_reaction=0;
    std::uint32_t valuation=100,requested=50;
    unsigned tie_count=2,tie_status=0,previous_status=3,list_length=1,list_status=0,
        pool_cursor=0,pool_end=64,selected_flags=0,national_already_set=0,fixture_count=2;
    bool fixture_no_member=false,secondary_club=false;
    int mutate_descriptor=0;
    bool no_competition=false,no_competition_record=false,no_nomination_details=false,no_slot=false,
        person_registered=true,lower_employment_target=false;
    unsigned competition_active=1,child_active=1,competition_year=2001,child_year=2001,
        competition_response=1,item_year=2001,contract_flags=3,offer_byte24=0,second_policy=1;
    std::uint32_t offer_link=0,pool_bias=7;
};
Scenario active;
bool reference_mode=false;
bool corrupt_rng=false;
unsigned list_calls=0;
void trace(Cpu& c,std::uint32_t target) {
    auto size=c.u32(log_at);
    const auto add=[&](std::uint32_t value){c.put32(log_at+4+size++*4,value);};
    add(target); add(c.lr());
    for(unsigned r=3;r<11;++r) add(c.r(r));
    for(unsigned r=13;r<32;++r) add(c.r(r));
    for(unsigned cr=0;cr<8;++cr) add(c.cr(cr));
    for(unsigned n:{0u,1u,2u,3u,31u}) {const auto bits=c.s.r<8>(reg::fpr(n));add(std::uint32_t(bits>>32));add(std::uint32_t(bits));}
    if(target==0x1048b4e0)for(unsigned n=0x38;n<=0x60;n+=4)add(c.u32(c.sp()+n));
    if(target==0x1036f170)add(c.u32(c.sp()+0x38));
    c.put32(log_at,size);
}
bool probe(State& s) {
    if(run_leaf(s)) return true;
    const auto target=s.pc;
    if(reference_mode&&target>=0x102bdf90&&target<0x102ce710) return news_reference(s);
    if(news_lists(target)) {news_run_rewrite(s,target);return true;}
    switch(target) {
    case 0x10739890: case 0x10768810: case 0x1077b720: case 0x10769b60:
    case 0x10727890: case 0x10715880: case 0x107156b0:
    case 0x103eb440: case 0x103eb470: case 0x103fb390:
    case 0x1007f720: case 0x10082f20: case 0x10738e00:
    case 0x10497570: case 0x10498f70: case 0x104976b0: case 0x104991b0:
    case 0x102ae9c0: case 0x102aea20: case 0x100c7d00: case 0x100dbbe0:
    case 0x1030e2a0: case 0x1030dee0: break;
    case 0x10147e70: case 0x10149df0: case 0x101b0100:
    case 0x104942d0: case 0x10493ac0: case 0x104acee0: case 0x104ae160:
    case 0x10738c00: case 0x10738c20: case 0x1048b4e0: case 0x100ce250:
    case 0x102af620: case 0x102acf40: case 0x102acf10: case 0x102b8850:
    case 0x10084be0: case 0x10085860: case 0x100d3570: case 0x10363ca0: case 0x1008cd20: case 0x1037fa80:
    case 0x103e6cb0: case 0x104ad840: case 0x10080d30: case 0x1036f170:
    case 0x103e4c90: case 0x103e4390: case 0x103ee1b0: case 0x103e4d70:
    case 0x103f9d60: case 0x103e8500: case 0x103e80e0: case 0x103e44b0: case 0x103e4130: case 0x103e5810: break;
    default:return false;
    }
    Cpu c(s); trace(c,target);
    if(target==0x10739890) {c.put32(c.r(6),0x4e455753);c.put32(c.r(7),0x43505000);}
    else if(target==0x1077b720) c.put32(c.r(3),c.r(8));
    else if(target==0x10769b60) c.set(3,0x2345);
    else if(target==0x10727890) {
        c.put32(c.r(3),0x4143544e);c.put8(c.r(3)+4,0);c.set(3,4);
        if(active.mutate_descriptor)c.put32(c.r(2)+std::uint32_t(active.mutate_descriptor),0xabcdef04);
    }
    else if(target==0x10715880) {c.put16(c.r(3),0);c.put16(c.r(3)+2,0);c.put8(c.r(3)+4,0);}
    else if(target==0x107156b0) {c.put16(c.r(3),c.r(4));c.put16(c.r(3)+2,c.r(5));c.put8(c.r(3)+4,2);}
    else if(target==0x103eb440) c.set(3,active.no_slot?0:offer_slot);
    else if(target==0x103eb470) c.set(3,secondary_slot);
    else if(target==0x103fb390) c.set(3,active.offer_flag);
    else if(target==0x1007f720||target==0x10082f20) c.set(3,active.no_contract?0:contract);
    else if(target==0x10738e00) {
        const auto index=list_calls++,next=index<active.list_length?list_entry+index*0x100:0;
        c.put32(c.r(3)+0xc,index+1<active.list_length?list_entry+(index+1)*0x100:0);c.set(3,next);
    } else if(target==0x100c7d00) c.set(3,active.special_club);
    else if(target==0x100dbbe0) c.set(3,active.eligibility);
    else if(target==0x1030e2a0) {c.put32(c.r(5)+0xc,active.decoded_manager);c.put8(c.r(4),c.u8(c.r(4))+4);}
    else if(target==0x1030dee0) {c.put32(c.r(5)+8,active.decoded_competition);c.put32(c.r(5)+0x14,active.decoded_club);c.put8(c.r(4),c.u8(c.r(4))+8);}
    else if(target==0x10497570||target==0x104976b0) c.set(3,data+0x70000);
    else if(target==0x10147e70)c.set(3,active.policy);
    else if(target==0x10149df0)c.set(3,active.second_policy);
    else if(target==0x10084be0)c.set(3,active.valuation);
    else if(target==0x10085860)c.set(3,active.requested);
    else if(target==0x10738c20) {for(unsigned n=0;n<16;n+=4)c.put32(c.r(3)+n,0);}
    else if(target==0x1048b4e0) {
        const auto result=c.u32(c.sp()+0x50);c.put16(result,active.empty_selection?0:1);c.put32(result+4,list_entry);c.set(3,1);
    }
    else if(target==0x103f9d60){c.put16(c.r(6),12);c.set(3,active.helper_status);}
    else if(target==0x102af620)c.set(3,active.national_already_set);
    else c.set(3,active.helper_status);
    s.pc=c.lr();return true;
}
void initialize(Cpu& c) {
    c.s.memory.map(data,0x100000);c.s.memory.map(stack-0x10000,0x20000);
    c.s.memory.copy_in(data,std::vector<std::uint8_t>(0x100000,0xa5));
    c.s.memory.copy_in(stack-0x10000,std::vector<std::uint8_t>(0x20000,0xa5));
    for(unsigned r=0;r<32;++r)c.set(r,0x57000000+r*0x101);
    for(unsigned cr=0;cr<8;++cr)c.set_cr(cr,cr+1);
    c.set(1,stack);c.set(2,0x108bba90);c.set_lr(sentinel);c.put32(log_at,0);list_calls=0;
    for(unsigned n=0;n<32;++n){c.set_f(n,n+0.5);c.s.wr<1>(1280+n,0);}
    const auto global=[&](int toc,unsigned offset,std::uint32_t value) {
        c.put32(c.r(2)+std::uint32_t(toc),globals+offset);c.put32(globals+offset,value);
    };
    c.put32(c.r(2)-0x7ba8,globals+0x1000);c.put32(c.r(2)-0x7bac,globals+0x100);c.put8(globals+0x100,1);
    global(-0x7b78,0x104,0);global(-0x7b40,0x108,data+0x71000);global(-0x7b7c,0x10c,data+0x72000);
    global(-0x7be0,0x110,people);global(-0x7b54,0x114,data+0x73000);global(-0x7be8,0x118,clubs);
    global(-0x7b5c,0x11c,data+0x74000);c.put32(data+0x74000,active.no_competition?0:competition);c.put32(data+0x74004,competition);
    global(-0x6918,0x120,0);global(-0x7a48,0x124,data+0x75000);c.put16(data+0x75000+0x4b2,2001);
    global(-0x7a64,0x128,data+0x76000);global(-0x7bf0,0x12c,queue);global(-0x7be4,0x130,32);
    global(-0x7b50,0x134,16);global(-0x7b64,0x138,16);global(-0x7bbc,0x13c,data+0x77000);c.put32(data+0x77008,active.no_competition_record?0:1);
    global(-0x7bb4,0x140,data+0x78000);c.put32(data+0x78008,data+0x78100);c.put32(data+0x78100,data+0x78200);
    c.put32(c.r(2)-0x7bb8,output);
    global(-0x7a54,0x144,data+0x7a000);global(-0x79f0,0x148,data+0x7a100);
    global(-0x7a98,0x14c,data+0x7a200);global(-0x7a60,0x150,data+0x7a300);global(-0x7b98,0x154,data+0x7b000);
    c.put32(data+0x71000+0x988,data+0x7c000);
    const auto fp_constants=c.s.memory.copy_out(c.toc(-0x510c),32);
    c.put32(c.r(2)-0x510c,data+0x7d000);c.s.memory.copy_in(data+0x7d000,fp_constants);
    c.put32(c.r(2)-0x7b90,globals+0x220);c.put32(globals+0x220,active.pool_bias);
    c.put32(c.r(2)-0x7b84,globals+0x224);c.put32(globals+0x224,data+0x7e000+active.pool_cursor);
    c.put32(c.r(2)-0x7b88,globals+0x228);c.put32(globals+0x228,data+0x7e000+active.pool_end);c.put32(c.r(2)-0x7b8c,data+0x7e000);
    for(unsigned n=0;n<66;n+=2)c.put16(data+0x7e000+n,31+n);
    c.put32(c.r(2)-0x34c4,0x12345678);
    c.put32(data+0x78204,data+0x78300);c.put32(data+0x78300,data+0x78400);c.put32(data+0x78424,active.no_nomination_details?0:data+0x78500);c.put16(data+0x78504,active.nomination_count);
    c.put32(c.r(2)-0x7b80,globals+0x200);c.put16(globals+0x200,2001);c.put16(globals+0x202,7);c.put8(globals+0x204,2);
    c.put32(manager,16);c.put32(manager+0x24,active.no_club?0:clubs);c.put32(manager+0x20,active.secondary_club?clubs:0);
    c.put8(manager+0x65,active.manager_attribute);
    for(unsigned n=0;n<4;++n) {const auto club=clubs+n*0x248;c.put32(club,n);c.put32(club+0x90,n?0:manager);c.put32(club+0x2c,data+0x79000);}
    c.put16(clubs+0x1ec,2);c.put16(clubs+0x248+0x1ec,active.lower_employment_target?1:2);
    c.put32(people+0x24,clubs);
    c.put32(people+0x14,active.person_registered);c.put8(people+0x65,active.player_attribute);
    c.put32(clubs+0x30,competition);c.put32(competition,0);
    c.put32(item,active.type);for(unsigned n=0;n<50;++n)c.put32(item+8+n*4,active.payload[n]);
    c.put16(item+0xd0,active.item_year);c.put16(item+0xd2,active.item_day);c.put8(item+0xd4,2);c.put32(item+0xdc,17);
    if(active.decode_extra)c.put32(item+8+23*4,1);
    c.put32(offer_slot,active.missing_offer?0:offer);c.put32(secondary_slot,secondary);
    c.put32(offer+0x2c,active.stale_key?999:123);c.put32(secondary+0x2c,active.secondary_key);c.put32(offer+0x30,active.offer_link);c.put8(offer+0x24,active.offer_byte24);
    c.put32(offer+8,active.employer_route?1:0);c.put32(offer+0xc,0);c.put32(offer+4,active.no_list?0:list);
    c.put8(offer+0x34,active.offer_status);c.put8(offer+0x10,active.offer_byte10);c.put8(offer+0x36,active.offer_byte36);c.put8(offer+0x37,active.offer_byte37);
    c.put32(contract+4,0);c.put8(contract+0x64,active.contract_status);
    c.put8(contract+0x4c,active.contract_reaction);c.put8(contract+0x5c,active.contract_flags);
    c.put32(list+4,active.list_length?list_entry:0);c.put32(list+0xc,0xdeadbeef);c.put32(list_entry,active.list_miss?1:0);c.put8(list_entry+4,active.list_status);
    c.put8(list_entry+0x4f,active.selected_flags);c.put32(list_entry+0x100,0);c.put8(list_entry+0x104,0);
    c.put16(data+0x70000+0x14,active.fixture_count);c.put32(data+0x70000+0x1c,active.fixture_no_member?clubs+0x248:clubs);c.put8(data+0x70000+0x29,0);
    const auto child=data+0x55000;
    for(auto object:{competition,child}) {
        c.put32(object,0);c.put32(object+0x30,1);c.put32(object+0xc,stage_table);
        c.put8(object+0x43,object==competition?active.competition_active:active.child_active);c.put16(object+0x3a,object==competition?active.competition_year:active.child_year);c.put8(object+0xa8,0);c.put8(object+0xb4,active.competition_response);c.put32(object+8,active.no_competition_record?0:data+0x79000);
        c.put32(object+0xa0,rounds);c.put32(object+0xac,rounds+0x1000);
    }
    c.put32(stage_table,child);c.put32(stage_table+4,child);
    c.put32(rounds+0x1000,ties);c.put32(rounds+0x1004,ties+0x200);c.put16(rounds+0x1c,active.tie_count);c.put16(rounds+0x78+0x1c,active.tie_count);
    c.put8(ties+0x34,active.tie_status);c.put8(ties+0x36+0x34,active.payload[5]==1?active.previous_status:3);
    c.put8(ties+0x200+0x34,active.tie_status);c.put8(ties+0x200+0x36+0x34,3);
    c.put32(queue,records);c.put32(queue+4,100);c.put32(queue+8,inboxes);c.put32(queue+12,16);
    for(unsigned n=0;n<16;++n) {c.put32(inboxes+n*12,0);c.put32(inboxes+n*12+4,1);c.put32(inboxes+n*12+8,1);}
    for(unsigned n=0;n<100;++n) {c.put32(records+n*0xe4,1);c.put32(records+n*0xe4+0xdc,n?0:17);}
    c.set({{3,item},{4,active.no_manager?0:manager},{5,active.no_output?0:output},{6,active.capacity},{7,active.no_callback?0:callback}});
    if(active.effects)c.set(5,active.no_associated_club?0:clubs);
    c.s.budget=10000000;
}
void returned(State& s) {
    Cpu c(s);cm::semantic_io_checks::require(c.sp()==stack&&c.lr()==sentinel&&s.pc==sentinel,"action label did not return");
}
void original(State& s,std::uint32_t entry){reference_mode=true;s.pc=entry;run_until(s,sentinel);returned(s);}
void rewrite(State& s,std::uint32_t entry){
    reference_mode=false;news_run_rewrite(s,entry);returned(s);
    if(corrupt_rng){Cpu c(s);const auto cursor=c.toc(-0x7b84);c.put32(cursor,c.u32(cursor)^2);}
}
Scenario case_for(unsigned type) {
    Scenario test;test.type=type;auto& p=test.payload;
    if(type==6){p[3]=2001;p[4]=7;}
    if(type==9){p[5]=2001;p[6]=7;}
    if(type==22)p[2]=2001;
    if(type==39){p[1]=2001;p[4]=0xffffffff;}
    if(type==40)p[0]=1;
    if(type==2002||type==2003){p[4]=2001;p[5]=8;p[15]=2001;p[16]=7;p[10]=2;}
    if(type==3000){p[8]=0;p[9]=123;}
    if(type==4000||type==4002||type==4005||type==4021||type==4032)p[7]=123;
    if(type==4002||type==4032){p[9]=456;test.offer_status=1;}
    if(type==4012){p[8]=123;test.offer_status=0;}
    if(type==4004){p[6]=3;p[8]=123;test.offer_status=7;}
    if(type==4005)test.offer_status=11;
    if(type==4020){p[10]=1;p[12]=123;test.offer_status=1;}
    return test;
}
}
namespace cm::aot {
unsigned check_news_actions(const State& base) {
    struct Restore{bool(*previous)(State&);~Restore(){dispatch_probe=previous;}}restore{dispatch_probe};dispatch_probe=probe;
    std::vector<Scenario> cases;
    constexpr auto types=std::to_array<unsigned>({6004,6022,6,9,22,24,25,30,34,39,40,28,3007,3048,3046,3033,3039,3037,3024,3009,3010,3000,2002,2003,4000,4028,4002,4012,4004,4005,4008,4011,4020,4021,4022,4032,4033,5000});
    for(auto type:types) {
        cases.push_back(case_for(type));
        for(unsigned variant=0;variant<10;++variant) {
            auto test=case_for(type);
            switch(variant){
            case 0:test.no_club=true;break;
            case 1:test.missing_offer=true;break;
            case 2:test.stale_key=true;break;
            case 3:test.no_contract=true;break;
            case 4:test.no_list=true;break;
            case 5:test.employer_route=true;break;
            case 6:test.employer_route=true;test.list_miss=true;break;
            case 7:test.helper_status=0;break;
            case 8:test.offer_flag=1;break;
            case 9:test.decode_extra=true;break;
            }cases.push_back(test);
        }
    }
    for(unsigned type=1;type<=6032;++type) if(std::find(types.begin(),types.end(),type)==types.end()) {Scenario test;test.type=type;cases.push_back(test);}
    for(unsigned type:{0u,6033u,0x7fffffffu,0x80000000u,0xffffffffu}) {Scenario test;test.type=type;cases.push_back(test);}
    for(unsigned variant=0;variant<7;++variant) {
        auto test=case_for(5000);
        switch(variant){case 0:test.no_manager=true;break;case 1:test.no_output=true;break;case 2:test.capacity=50;break;case 3:test.capacity=0xffffffff;break;case 4:test.no_callback=true;break;case 5:test.capacity=51;break;case 6:test.capacity=0;break;}cases.push_back(test);
    }
    const std::map<unsigned,std::vector<unsigned>> gates{
        {6004,{1}},{6022,{1,2,5}},{6,{5}},{9,{3}},{22,{0}},{24,{1}},{25,{3}},{30,{2}},
        {34,{7}},{39,{6}},{40,{0}},{3007,{7}},{3048,{18,15}},{3046,{20}},{3033,{8,10}},
        {3039,{9,10}},{3037,{13}},{3024,{10}},{3009,{6}},{3010,{7}},{3000,{10}},
        {2002,{0,12,10}},{2003,{0,12,10}},{4000,{9}},{4028,{7}},{4012,{9}},
        {4004,{6,9}},{4005,{9}},{4011,{7}},{4020,{10,13}}};
    for(const auto& [type,slots]:gates) for(auto slot:slots)
        for(auto value:{0u,1u,2u,3u,4u,5u,6u,17u,62u,80u,81u,82u,94u,127u,128u,255u,256u,257u,0xffffffffu}) {
            auto test=case_for(type);test.payload[slot]=value;cases.push_back(test);
        }
    for(auto type:{4000u,4002u,4012u,4004u,4005u,4020u,4021u,4032u})
        for(auto status:{0u,1u,5u,6u,7u,8u,11u,12u,13u,15u,17u,255u})
            for(bool employer:{false,true}) {
                auto test=case_for(type);test.offer_status=status;test.employer_route=employer;cases.push_back(test);
            }
    for(auto type:{6u,9u,34u}) for(auto status:{0u,1u,2u,255u,256u,257u}) {
        auto test=case_for(type);test.helper_status=status;cases.push_back(test);
    }
    for(auto type:{2002u,2003u}) {
        for(auto second_day:{6u,7u,8u}) for(auto first_day:{6u,7u,8u}) for(auto year:{2000u,2001u,2002u}) {
            auto test=case_for(type);test.payload[16]=second_day;test.payload[5]=first_day;test.payload[4]=year;cases.push_back(test);
        }
        for(unsigned field=0;field<3;++field) for(auto id:{0xffffffffu,0u,15u,16u,31u,32u}) {
            auto test=case_for(type);test.decode_extra=true;
            if(field==0)test.decoded_manager=id;else if(field==1)test.decoded_club=id;else test.decoded_competition=id;
            cases.push_back(test);
        }
        auto disabled=case_for(type);disabled.eligibility=0;cases.push_back(disabled);
        disabled.eligibility=1;disabled.decode_extra=true;disabled.special_club=true;cases.push_back(disabled);
    }
    for(auto day:{6u,7u,8u}) {auto test=case_for(34);test.item_day=day;cases.push_back(test);}
    for(auto count:{0u,1u,0xffffu}) {auto test=case_for(28);test.nomination_count=count;cases.push_back(test);}
    for(auto type:{4002u,4032u}) {auto test=case_for(type);test.secondary_key=457;cases.push_back(test);}
    for(auto offer:{0u,0xffffffffu}) {auto test=case_for(3000);test.payload[8]=offer;cases.push_back(test);}
    for(auto mode:{3u,4u}) for(auto byte:{0u,2u,3u,255u}) {
        auto test=case_for(4004);test.payload[6]=mode;test.offer_status=mode==3?7:15;test.offer_byte36=byte;cases.push_back(test);
    }
    for(auto type:{4012u,4005u,4020u}) for(unsigned byte:{0u,1u,255u}) {
        auto test=case_for(type);test.offer_byte10=byte;test.offer_byte37=byte;cases.push_back(test);
    }
    const auto label_count=cases.size();
    for(unsigned n=0;n<label_count;++n) if(std::find(types.begin(),types.end(),cases[n].type)!=types.end()) {
        auto test=cases[n];test.effects=true;cases.push_back(test);
    }
    for(unsigned type:types)for(bool special:{false,true})for(bool no_club:{false,true}) {
        auto test=case_for(type);test.effects=true;test.special_club=special;test.no_associated_club=no_club;cases.push_back(test);
    }
    for(auto value:{0u,50u,100u,200u,0x80000000u,0x7fffffffu})for(auto requested:{0u,50u,100u,200u}) {
        auto test=case_for(3007);test.effects=true;test.valuation=value;test.requested=requested;cases.push_back(test);
    }
    for(auto player:{0u,1u,25u,40u,65u,127u,128u,255u})for(auto rating:{0u,100u,156u,255u}) {
        auto test=case_for(3007);test.effects=true;test.requested=0x7fffffff;test.player_attribute=player;test.contract_reaction=rating;cases.push_back(test);
    }
    for(auto stage:{0xffffffffu,0u,1u})for(auto round:{0u,1u})for(auto count:{0u,1u,2u})for(auto finished:{0u,3u}) {
        auto test=case_for(39);test.payload[4]=stage;test.payload[5]=round;test.tie_count=count;test.tie_status=finished;cases.push_back(test);
    }
    for(auto status:{0u,3u}) {auto test=case_for(39);test.payload[5]=1;test.previous_status=status;cases.push_back(test);}
    for(auto status:{0u,2u,3u,255u})for(auto type:{3000u,4004u}) {auto test=case_for(type);test.contract_status=status;cases.push_back(test);}
    for(auto count:{0u,1u,2u})for(auto status:{0u,1u,255u}) {
        auto test=case_for(4012);test.employer_route=true;test.list_length=count;test.list_status=status;cases.push_back(test);
        test.list_miss=true;cases.push_back(test);
    }
    for(auto type:{6u,9u}) {auto test=case_for(type);test.no_club=true;test.secondary_club=true;cases.push_back(test);}
    for(const auto& [type,slot]:std::array<std::pair<unsigned,int>,2>{{{5000,-0x63b0},{3048,-0x6378}}}) {
        auto test=case_for(type);test.mutate_descriptor=slot;cases.push_back(test);
    }
    for(auto manager_attribute:{0u,1u,127u,128u,255u,246u})for(auto cursor:{0u,62u,64u})for(auto end:{0u,2u,64u}) {
        auto test=case_for(3007);test.effects=true;test.requested=0x7fffffff;test.manager_attribute=manager_attribute;
        test.pool_cursor=cursor;test.pool_end=end;cases.push_back(test);
    }
    for(auto flags:{0u,64u,128u,192u})for(auto initialized:{0u,1u})for(auto result:{0u,1u}) {
        auto test=case_for(30);test.effects=true;test.special_club=true;test.selected_flags=flags;test.national_already_set=initialized;test.helper_status=result;cases.push_back(test);
    }
    auto no_selection=case_for(30);no_selection.effects=true;no_selection.special_club=true;no_selection.empty_selection=true;cases.push_back(no_selection);
    auto no_member=case_for(9);no_member.effects=true;no_member.fixture_no_member=true;cases.push_back(no_member);
    for(auto status:{17u,18u})for(bool effects:{false,true}) {auto test=case_for(4011);test.effects=effects;test.offer_status=status;test.payload[6]=status;cases.push_back(test);}
    // Coverage-directed conjunctions; these additions characterize labels
    // independently so a rejected action is never forced through effects.
    const auto label=[&](unsigned type,std::initializer_list<std::pair<unsigned,std::uint32_t>> words={}) -> Scenario& {
        cases.push_back(case_for(type));for(auto [slot,value]:words)cases.back().payload[slot]=value;return cases.back();
    };
    label(6022,{{2,1},{5,1}});
    label(6,{{1,1},{2,1}});
    label(22,{{1,1}});label(22,{{2,2000}});
    for(unsigned type:{24u,25u})label(type,{{0,1}});
    label(34).item_year=2000;
    label(39).no_competition=true;label(39,{{4,2}});
    label(39).competition_active=0;label(39,{{1,2000}});
    label(39,{{4,0}}).child_active=0;label(39,{{4,0},{1,2000}});
    label(40).competition_response=0;label(28).no_nomination_details=true;
    for(auto [type,slot]:std::array<std::pair<unsigned,unsigned>,10>{{{3007,0},{3048,8},{3046,9},{3033,4},{3039,4},{3037,4},{3024,4},{3009,0},{3010,0},{3000,4}}})label(type,{{slot,1}});
    label(3039,{{9,63}});label(3000,{{8,0xffffffffu},{10,5}});
    label(2002).decode_extra=true;cases.back().no_competition_record=true;
    label(4000,{{5,0xffffffffu}});label(4000,{{5,1}});
    label(4028,{{4,0xffffffffu}});label(4028,{{4,1}});
    for(unsigned type:{4002u,4032u})label(type).offer_link=0xffffffffu;
    for(unsigned status:{5u,6u}){auto& test=label(4012);test.offer_byte10=1;test.offer_status=status;}
    label(4012).employer_route=true;cases.back().no_contract=true;
    label(4012).employer_route=true;cases.back().no_list=true;
    label(4004,{{7,0xffffffffu}});
    label(4004,{{6,4},{7,0xffffffffu}});label(4004,{{6,4},{9,1}});
    label(4004,{{6,4}}).missing_offer=true;
    label(4004,{{6,4}}).stale_key=true;
    label(4004,{{6,4}}).no_club=true;
    label(4004,{{6,4},{5,1}});
    for(unsigned status:{12u,13u,14u,16u,18u})label(4005,{{9,status}});
    label(4005).employer_route=true;cases.back().no_contract=true;
    label(4005).employer_route=true;cases.back().offer_byte37=1;
    label(4011,{{0,1},{1,0}});label(4011,{{0,1},{1,2}});
    label(4020).no_slot=true; // Safe dependency-null caller boundary, not a registry invariant.
    label(4020,{{5,1}});label(4021,{{5,0xffffffffu}});label(4021,{{5,1}});
    for(unsigned which=0;which<3;++which){auto& test=label(6004);test.effects=true;if(which==0)test.policy=0;else if(which==1)test.second_policy=0;else{test.payload[0]=1;test.lower_employment_target=true;}}
    label(3007).effects=true;cases.back().person_registered=false;
    for(unsigned flags:{0u,8u}){auto& test=label(3007);test.effects=true;test.contract_flags=flags;test.valuation=100;test.requested=0;}
    for(bool absent:{false,true}){auto& test=label(4000);test.effects=true;test.offer_byte24=1;test.no_contract=absent;}
    // Mapped noncanonical seeds exercise accepted raw32-bit arithmetic; they
    // do not imply these seeds occur during an ordinary career.
    for(unsigned bias:{0xffffffdcu,0xffffffacu}){auto& test=label(3007);test.effects=true;test.requested=0x7fffffff;test.pool_bias=bias;}
    // The reference unconditionally dereferences a missing 4011 offer payload
    // after label2. Keep that matching fault in a separate failing diagnostic;
    // it is not a returned comparison or valid-state coverage evidence.
    if(!std::getenv("CM_NEWS_DIAGNOSTIC_FAULTS"))
        std::erase_if(cases,[](const auto& test){return test.effects&&test.type==4011&&test.missing_offer;});
    else std::erase_if(cases,[](const auto& test){return !(test.effects&&test.type==4011&&test.missing_offer);});
    if(std::getenv("CM_NEWS_EFFECTS_ONLY"))std::erase_if(cases,[](const auto& test){return !test.effects;});
    unsigned failures=0;std::map<std::uint32_t,unsigned> returned_counts;
    for(const auto& test:cases) {
        active=test;auto state=cm::semantic_io_checks::clone(base);Cpu c(*state);initialize(c);
        const auto entry=test.effects?0x102c8690u:0x102c6b10u;
        const auto difference=compare_routine_call(*state,entry,original,rewrite,RoutineChecker::result_r3|RoutineChecker::result_f1);
        if(!difference.empty()) {++failures;std::cout<<"FAIL action "<<hex_address(entry)<<" type="<<test.type<<" variant="<<(&test-cases.data())<<'\n';for(const auto& line:difference)std::cout<<line<<'\n';}
        else ++returned_counts[entry];
    }
    active=case_for(5000);auto state=cm::semantic_io_checks::clone(base);Cpu c(*state);initialize(c);
    if(compare_routine_call(*state,0x102c6b10,original,rewrite,RoutineChecker::result_r3,true).empty())++failures;
    else std::cout<<"ok action label corruption detected\n";
    active=case_for(3007);active.effects=true;active.requested=0x7fffffff;
    for(unsigned corruption=0;corruption<2;++corruption){
        corrupt_rng=corruption==1;auto effect_state=cm::semantic_io_checks::clone(base);Cpu effect_cpu(*effect_state);initialize(effect_cpu);
        if(compare_routine_call(*effect_state,0x102c8690,original,rewrite,RoutineChecker::result_r3|RoutineChecker::result_f1,corruption==0).empty())++failures;
        else std::cout<<"ok action effect corruption detected "<<corruption<<'\n';
    }
    corrupt_rng=false;
    for(const auto& [entry,count]:returned_counts)std::cout<<hex_address(entry)<<": "<<count<<" returned action comparisons\n";
    std::cout<<cases.size()<<" action comparisons with scripted domain boundaries, "<<failures<<" failed\n";
    return failures;
}
}
