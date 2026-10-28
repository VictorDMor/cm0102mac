// Whole NEWS renderer calls with scripted external text/service boundaries.
// All local compositions and service delegates are included. Both calls
// must return; matching internal faults are failures.
#include "news_adapter.hpp"
#include "news_message_detail.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <iostream>

using namespace cm::aot;
using namespace cm::aot::guest;
namespace cm::aot { bool news_reference(State&); }
namespace {
constexpr std::uint32_t data=0x65000000,item=data+0x2000,output=data+0x3000,today=data+0x4000,
    scratch=data+0x5000,diagnostic=data+0x6000,success=data+0x7000,queue=data+0x8000,
    records=data+0x9000,clubs=data+0x10000,names=data+0x20000,objects=data+0x30000,
    competition=data+0x31000,vtable=data+0x32000,descriptor=data+0x32100,
    main_buffer=data+0x40000,temp_buffer=data+0x42000,people=data+0x50000,
    manager=data+0x52000,origin=data+0x60000,destination=data+0x61000,
    city=data+0x62000,service=data+0x70000,log_at=data+0x80000,
    stack=0x76010000,sentinel=0xfffffff0;
struct Dependency {std::uint32_t target;unsigned output_argument;};
constexpr auto dependencies=std::to_array<Dependency>({
#include "news_message_fixture_delegates.inc"
});
struct Scenario {
    unsigned type,detail=1,suppress=0;
    std::array<std::uint32_t,50> payload{};
    bool no_manager=false,no_output=false,no_object=false,no_record=false,missing_geography=false,no_reply=false,distinct_owners=false,
        no_origin=false,no_destination=false,no_names=false,quoted_self_reply=false;
    unsigned quoted_type=5009,service_reply=0,language_variant=0,normalized_round=0;
    unsigned replacement_injury=0xffffffffu,score_a=2,score_b=1,round_name_variant=0,venue_name_variant=0;
    bool no_active_manager=false,no_active_club=false,no_club_table=false,person_exists=true,match_exists=true,hide_competition=false;
    std::string weekday="dayC";
};
Scenario active{0};bool reference_mode=false;unsigned helper_calls=0;
void cstring(Cpu& c,std::uint32_t at,const std::string& text) {for(unsigned n=0;n<=text.size();++n)c.put8(at+n,n==text.size()?0:text[n]);}
std::string string_at(Cpu& c,std::uint32_t at) {
    std::string text;
    for(unsigned n=0;n<4096;++n){const auto ch=c.u8(at+n);if(!ch)return text;text.push_back(char(ch));}
    throw std::runtime_error("Unterminated NEWS fixture string");
}
void set_text(Cpu& c,std::uint32_t object,const std::string& text) {
    const auto buffer=object==output?main_buffer:temp_buffer;c.put32(object,buffer);cstring(c,buffer,text);
}
void trace(Cpu& c,std::uint32_t target) {
    auto size=c.u32(log_at);const auto add=[&](std::uint32_t v){c.put32(log_at+4+size++*4,v);};
    add(target);add(c.lr());for(unsigned r=3;r<11;++r)add(c.r(r));
    for(unsigned r=13;r<32;++r)add(c.r(r));for(unsigned cr=0;cr<8;++cr)add(c.cr(cr));
    if(target==0x10727890)for(unsigned offset=0x38;offset<=0x7c;offset+=4)add(c.u32(c.sp()+offset));
    if(target==0x10781320){add(c.r(12));for(unsigned offset=0x38;offset<=0x40;offset+=4)add(c.u32(c.sp()+offset));}
    c.put32(log_at,size);++helper_calls;
}
bool probe(State& s) {
    if(run_leaf(s))return true;
    const auto target=s.pc;
    if(reference_mode&&target>=0x102bdf90&&target<0x102ce710)return news_reference(s);
    if(news_lists(target)){news_run_rewrite(s,target);return true;}
    // Use the existing standard-library implementations for memory/string
    // operations, including the quoted item's inline constructor/accessor.
    if(target==0x10778300||target==0x107783e0||target==0x1077dc10)return false;
    Cpu c(s);trace(c,target);
    if(target==0x10739890){c.put32(c.r(6),0x4e455753);c.put32(c.r(7),0x43505000);}
    else if(target==0x10768810)c.set(3,service+0x100);
    else if(target==0x1077b720)cstring(c,c.r(3),"formatted");
    else if(target==0x1077dbf0){cstring(c,c.r(3),string_at(c,c.r(4)));}
    else if(target==0x10769b60)c.set(3,0x2345);
    else if(target==0x1074c570){c.put32(c.r(3),0);c.put32(c.r(3)+4,0);c.set(3,1);}
    else if(target==0x1074b830){const auto text=string_at(c,c.r(4));set_text(c,c.r(3),text);c.set(3,0x41);}
    else if(target==0x1074b960){const auto text=(c.u32(c.r(3))?string_at(c,c.u32(c.r(3))):"")+string_at(c,c.r(4));set_text(c,c.r(3),text);c.set(3,0x42);}
    else if(target==0x1074bab0){c.put32(c.r(3)+4,c.r(5));c.set(3,0x43);}
    else if(target==0x1074b6e0){c.put32(c.r(3),0);c.put32(c.r(3)+4,0);}
    else if(target==0x1074b7c0){c.put32(c.r(3),0);c.put32(c.r(3)+4,0);c.set(3,0x44);}
    else if(target==0x10727890){cstring(c,c.r(3),"F"+std::to_string(c.r(4))+"/"+std::to_string(helper_calls));c.set(3,0x45);}
    else if(target==0x10715880){c.put16(c.r(3),0);c.put16(c.r(3)+2,0);c.put8(c.r(3)+4,0);}
    else if(target==0x107156b0){c.put16(c.r(3),c.r(4));c.put16(c.r(3)+2,c.r(5));c.put8(c.r(3)+4,2);}
    else if(target==0x1077dc90)c.set(3,string_at(c,c.r(3))==string_at(c,c.r(4))?0:1);
    else if(target==0x1030dee0){const auto record=c.r(5);for(unsigned n=0;n<0x28;++n)c.put8(record+n,0);c.put16(record,2001);c.put16(record+2,7);c.put32(record+8,0);c.put16(record+0xc,2);c.put16(record+0xe,3);c.put32(record+0x14,0);c.put32(record+0x18,1);c.put8(record+0x1c,active.score_a);c.put8(record+0x1d,active.score_b);c.put32(record+0x20,5);c.put8(c.r(4),30);c.set(3,1);}
    else if(target==0x100cb590){cstring(c,c.r(6),"player");c.set(3,1);}
    else if(target==0x100cbde0)c.set(3,active.person_exists);
    else if(target==0x1018c2f0)c.set(3,active.replacement_injury);
    else if(target==0x1018d140)c.set(3,(c.r(4)&3)+1);
    else if(target==0x1018c560){cstring(c,c.r(5),"injury");c.set(3,1);}
    else if(target==0x1030e880){cstring(c,c.r(4),"duration");c.set(3,1);}
    else if(target==0x10715b20)c.set(3,2);
    else if(target==0x107187d0)c.set(3,data+0x69000);
    else if(target==0x10718af0)c.set(3,3);
    else if(target==0x1077dbd0)c.set(3,unsigned(string_at(c,c.r(3)).size()));
    else if(target==0x104abf40)c.set(3,active.hide_competition);
    else if(target==0x1029eb20)c.set(3,active.match_exists);
    else if(target==0x10464010)c.set(3,active.normalized_round);
    else if(target==0x10469bb0){cstring(c,c.r(3),"group");c.set(3,1);}
    else if(target==0x10727880)c.set(3,active.language_variant);
    else if(target==0x10192130)c.set(3,1);
    else if(target==0x104976b0)c.set(3,active.no_record?0:service+0x300);
    else if(target==0x10497570)c.set(3,2);
    else if(target==0x10498f70||target==0x104991b0){cstring(c,c.r(target==0x10498f70?10:6),"service reply");c.set(3,active.service_reply);}
    else if(target==0x10718570)c.set(3,service+0x200);
    else if(target==0x10718650)c.set(3,3);
    else if(target==0x10716eb0||target==0x10717270||target==0x10717620){cstring(c,c.r(4),"date");c.set(3,0x4b);}
    else if(target==0x10463dc0){c.put16(c.r(4)+0x32,3);c.put8(c.r(4)+0x42,2);c.put8(c.r(5),9);c.set(3,1);}
    else if(target==0x10781320){c.put32(c.sp()+0x14,c.r(2));c.set(2,0xdeadbeef);set_text(c,output,"virtual");c.set(3,0x46);}
    else if(target==0x104769d0){cstring(c,c.r(7),active.round_name_variant?string_at(c,c.toc(-0x5100)+(active.round_name_variant==1?0x1c:0x25)):"round name");c.set(3,0x47);}
    else if(target==0x10476ec0)c.set(3,2);
    else if(target==0x100c76c0)c.set(3,clubs);
    else if(target==0x100caeb0){cstring(c,c.r(4),"sender");c.set(3,0x48);}
    else if(target==0x10452290||target==0x1045c9e0){set_text(c,output,"competition");c.set(3,0x49);}
    else {
        const auto found=std::find_if(dependencies.begin(),dependencies.end(),[&](const auto& row){return row.target==target;});
        if(found==dependencies.end())throw std::runtime_error("Unscripted NEWS message helper "+hex_address(target));
        set_text(c,c.r(found->output_argument),"delegate");c.set(3,0x4a);c.set_f(1,target&255);
    }
    s.pc=c.lr();return true;
}
void initialize(Cpu& c) {
    c.s.memory.map(data,0x100000);c.s.memory.map(stack-0x10000,0x20000);
    c.s.memory.copy_in(data,std::vector<std::uint8_t>(0x100000,0xa5));
    c.s.memory.copy_in(stack-0x10000,std::vector<std::uint8_t>(0x20000,0xa5));
    for(unsigned r=0;r<32;++r){c.set(r,0x58000000+r*0x101);c.set_f(r,r+0.5);c.s.wr<1>(1280+r,0);}
    for(unsigned cr=0;cr<8;++cr)c.set_cr(cr,cr+1);c.set(1,stack);c.set(2,0x108bba90);c.set_lr(sentinel);
    helper_calls=0;c.put32(log_at,0);
    unsigned cell=0x100;
    for(int slot:{-0x7b40,-0x7b7c,-0x7ab8,-0x7ba0,-0x7310,-0x7548,-0x7b98,-0x7be8,-0x7bbc,
        -0x79f0,-0x7b94,-0x7b5c,-0x7a64,-0x7b54,-0x7b4c,-0x7bb4,-0x7b34,-0x7a48,-0x7aac,-0x7a98,-0x7afc,
        -0x7aa4,-0x7ba4,-0x7b28,-0x7484,-0x7b24,-0x633c,-0x7440,-0x6340,-0x7b1c,-0x7a2c,-0x6918,-0x6960,-0x7be0,-0x7bf0}){
        c.put32(c.r(2)+std::uint32_t(slot),data+cell);c.put32(data+cell,service+(active.distinct_owners?(cell-0x100)*0x100:0));cell+=4;
    }
    c.put32(c.toc(-0x7be8),clubs);c.put32(c.toc(-0x7bbc),names);c.put32(c.toc(-0x7b5c),objects);c.put32(c.toc(-0x7be0),people);
    c.put32(c.toc(-0x7b98),data+0x63000);
    for(unsigned n=0;n<8;++n){const auto nation=data+0x63000+n*0x128;
        c.put8(nation+0x115,n%3+1);c.put8(nation+0x116,n%3+2);cstring(c,nation+0xae,"nation");cstring(c,nation+0xe1,"short nation");}
    c.put8(data+0x63000-0x128+0x115,1);cstring(c,data+0x63000-0x128+0xae,"missing nation");
    if(active.no_names)c.put32(c.toc(-0x7bbc),0);
    c.put32(c.toc(-0x7548),data+0x64000);c.put32(data+0x64000+0x10,active.no_origin?0:origin);cstring(c,data+0x64000+0x18,"venue");c.put8(data+0x64000+0x4b,3);
    c.put32(c.toc(-0x7310),0);c.put32(c.toc(-0x7a2c),0);c.put32(c.toc(-0x6918),0);c.put32(c.toc(-0x6960),1);
    c.put32(c.r(2)-0x633c,service+(active.distinct_owners?0x8800:0));c.put32(c.r(2)-0x6340,service+(active.distinct_owners?0x8900:0));
    c.put32(c.r(2)-0x7ba8,diagnostic);c.put32(c.r(2)-0x7bac,success);c.put8(success,1);c.put32(c.r(2)-0x7bb8,scratch);
    c.put32(c.r(2)-0x7b80,today);c.put16(today,2001);c.put16(today+2,7);c.put8(today+4,2);
    c.put32(c.r(2)-0x79ec,data+0x4f00);c.put8(data+0x4f00,4);
    c.put32(item,active.type);for(unsigned n=0;n<50;++n)c.put32(item+8+n*4,active.payload[n]);
    if(active.type==5000)cstring(c,item+8+3*4,"body");
    c.put32(manager,16);c.put32(manager+0x24,active.no_active_club?0:clubs);c.put32(manager+0x20,0);
    for(unsigned n=0;n<4;++n){
        const auto club=clubs+n*0x248,name=names+n*0x6c;
        c.put32(club,n);c.put8(club+0x23b,n+2);c.put8(club+0x242,n+3);cstring(c,club+0x1ee,"full club");cstring(c,club+0x221,"short club");
        c.put32(club+0x3c,active.missing_geography?0:city);
        c.put32(name+8,active.no_destination?0:destination);c.put8(name+0x67,4);c.put8(name+0x6a,5);cstring(c,name+0x1a,"full competition");cstring(c,name+0x4d,"short competition");
        const auto object=competition+(active.distinct_owners?n*0x100:0);
        c.put32(objects+n*4,active.no_object?0:object);c.put32(object,vtable);c.put32(object+8,active.no_record?0:service);
    }
    c.put32(c.r(2)-0x5104,data+0x68000);cstring(c,data+0x68000,"default title");cstring(c,data+0x69000,active.weekday);
    c.put32(c.r(2)-0x7bb0,data+0x65000);c.put16(data+0x65000+0x2c58,0);c.put32(data+0x65000+4,active.no_active_manager?0:manager);
    c.put32(c.r(2)-0x7a50,data+0x4e00);c.put32(data+0x4e00,2);
    c.put32(c.r(2)-0x7b44,service+0x1000);c.put8(service+0x1000+0x4c+0x18,active.no_record?0:1);
    c.put32(city+0x10,active.no_origin?0:origin);c.put8(origin+0x36,6);cstring(c,origin+0x1c,"origin");
    if(active.venue_name_variant)cstring(c,data+0x64000+0x18,active.venue_name_variant==1?string_at(c,origin+0x1c):string_at(c,c.toc(-0x5100)+0x1a65));
    c.put8(destination+0x115,7);c.put8(destination+0x116,8);cstring(c,destination+0xae,"destination");cstring(c,destination+0xe1,"short destination");
    c.put32(competition,vtable);c.put32(vtable+0x50,descriptor);c.put32(descriptor,0x10450000);c.put32(descriptor+4,c.r(2));
    c.put32(competition+8,active.no_record?0:service);
    c.put32(output,main_buffer);c.put32(output+4,0x1234);cstring(c,main_buffer,"old");cstring(c,scratch,"scratch");
    c.put32(c.toc(-0x7bf0),queue);c.put32(queue,records);c.put32(queue+4,1);
    c.put32(records,active.quoted_type);c.put32(records+0xdc,active.no_reply?0:5);
    for(unsigned n=0;n<50;++n)c.put32(records+8+n*4,0);c.put8(records+0xe0,0);c.put8(records+0xe1,0);
    if(active.quoted_type==5000){c.put32(records+0xc,1);c.put32(records+0x10,active.quoted_self_reply?5:0);cstring(c,records+0x14,"quoted body");}
    if(active.no_club_table)c.put32(c.toc(-0x7be8),0);
    c.set({{3,item},{4,active.no_manager?0:manager},{5,active.no_output?0:output},{6,active.detail},{7,active.suppress}});c.s.budget=10000000;
}
void returned(State& s){Cpu c(s);cm::semantic_io_checks::require(c.sp()==stack&&c.lr()==sentinel&&s.pc==sentinel,"message renderer did not return");}
void original(State& s,std::uint32_t entry){reference_mode=true;s.pc=entry;run_until(s,sentinel);returned(s);}
void rewrite(State& s,std::uint32_t entry){reference_mode=false;news_run_rewrite(s,entry);returned(s);}
}
namespace cm::aot {
unsigned check_news_messages(const State& base) {
    struct Restore{bool(*previous)(State&);~Restore(){dispatch_probe=previous;}}restore{dispatch_probe};dispatch_probe=probe;
    std::vector<Scenario> cases;
    constexpr std::array<unsigned,0> missing{};
    constexpr auto local=std::to_array<unsigned>({1,3,4,5,6,7,8,9,10,11,13,20,21,38,12,16,17,18,19,22,23,26,39,40,1001,2000,5000});
    for(unsigned type=0;type<=6033;++type) {
        if(std::find(missing.begin(),missing.end(),type)!=missing.end())continue;
        for(unsigned detail:{0u,1u,256u,255u}){Scenario test{type,detail};if(type==5000)test.payload[2]=5;
            if(type==8){test.payload[1]=1;test.payload[2]=2001;test.payload[3]=7;}
            if(type==11){test.payload[1]=2001;test.payload[2]=7;}cases.push_back(test);}
    }
    for(auto type:local)for(unsigned variant=0;variant<5;++variant){Scenario test{type};
        switch(variant){case 0:test.no_object=true;break;case 1:test.no_record=true;break;case 2:test.missing_geography=true;break;case 3:test.no_reply=true;break;case 4:test.suppress=1;break;}
        if(type==5000)test.payload[2]=5;cases.push_back(test);
    }
    for(auto type:{17u,18u,22u,40u})for(auto value:{0u,1u,2u,3u,4u,127u,255u,256u}){Scenario test{type};test.payload[type==17||type==18?1:0]=value;cases.push_back(test);}
    Scenario no_manager{5000};no_manager.no_manager=true;cases.push_back(no_manager);
    Scenario no_output{5000};no_output.no_output=true;cases.push_back(no_output);
    for(unsigned type:{27u,28u,29u,31u,35u,37u,47u,46u,42u,41u,24u,1000u,2001u,2002u,6009u,6020u,
        3003u,3000u,3049u,3013u,3011u,5009u,5011u,5010u,5006u,5001u}) {
        Scenario test{type};test.distinct_owners=true;test.payload[0]=1;test.payload[1]=2;cases.push_back(test);
    }
    for(unsigned accepted:{1u,255u,256u}){Scenario test{22};test.payload[0]=1;test.payload[3]=accepted;cases.push_back(test);}
    for(unsigned detail:{0u,1u})for(unsigned status:{0u,3u,4u,5u,255u})for(unsigned requester:{0u,1u,2u}){
        Scenario test{7,detail};test.payload[1]=1;test.payload[2]=requester;test.payload[3]=2001;test.payload[4]=7;test.payload[5]=status;cases.push_back(test);}
    for(unsigned detail:{0u,1u})for(auto first:{0xffffffffu,0xfffffffeu,0u,1u})for(auto second:{0xffffffffu,0u,1u}){
        Scenario test{5,detail};test.payload[1]=2001;test.payload[2]=first;test.payload[3]=second;cases.push_back(test);}
    for(unsigned type:{1u,3u})for(unsigned full:{0u,1u})for(unsigned variant=0;variant<3;++variant){Scenario test{type,full};test.payload[1]=1;test.payload[5]=2001;test.payload[6]=7;
        if(type==1){test.payload[7]=variant==0?0xffffffffu:2;test.payload[8]=3;test.payload[9]=1;test.payload[12]=2002;test.payload[13]=8;}
        else{test.payload[7]=3;test.payload[8]=variant==0?2002:2001;test.payload[9]=8;test.payload[10]=variant==2?3:4;}cases.push_back(test);}
    for(unsigned full:{0u,1u})for(unsigned joint:{0u,1u}){Scenario test{4,full};test.payload[1]=2001;test.payload[2]=0;test.payload[3]=joint?1:0xffffffffu;test.payload[4]=2;test.payload[5]=0xffffffffu;test.payload[6]=3;test.payload[7]=0xffffffffu;cases.push_back(test);}
    for(unsigned type:{9u,10u})for(unsigned full:{0u,1u})for(unsigned status:{0u,2u,3u,4u,255u})for(unsigned found:{0u,1u}){Scenario test{type,full};test.payload[1]=1;test.no_record=!found;test.payload[type==9?3:4]=status;test.payload[type==9?5:2]=2001;test.payload[type==9?6:3]=7;test.service_reply=status&1;cases.push_back(test);}
    for(unsigned type:{7u,8u,10u,11u})for(unsigned year:{0x7fffu,0x8000u,0xffffu,0x123407d1u}){Scenario test{type};test.payload[1]=type==11?year:1;test.payload[type==7?3:type==11?1:2]=year;test.payload[type==7?4:type==11?2:3]=0x12340007;test.payload[type==7?5:4]=3;cases.push_back(test);}
    for(unsigned detail:{0u,1u})for(unsigned status:{0u,3u,4u,255u})for(unsigned requester:{0u,1u})for(unsigned stadium:{0u,1u}){Scenario test{6,detail};test.payload[1]=1;test.payload[2]=requester;test.payload[3]=2001;test.payload[4]=7;test.payload[5]=status;test.missing_geography=!stadium;test.service_reply=(requester+stadium)&1;cases.push_back(test);}
    for(unsigned detail:{0u,1u})for(unsigned special:{0u,1u}){Scenario test{13,detail};test.no_record=!special;test.payload[1]=3;test.payload[2]=2001;for(unsigned n=0;n<3;++n){test.payload[3+n*4]=2001+n;test.payload[4+n*4]=7+n;test.payload[5+n*4]=n;test.payload[6+n*4]=n%2;}cases.push_back(test);}
    for(unsigned detail:{0u,1u})for(unsigned venue:{0xffffffffu,0u})for(unsigned flag:{0u,1u})for(unsigned language:{0u,1u,2u,255u}){Scenario test{20,detail};test.payload[3]=1;test.payload[4]=2;test.payload[5]=flag;test.payload[6]=venue;test.payload[7]=2001;test.payload[8]=7;test.language_variant=language;test.no_origin=flag==0;cases.push_back(test);}
    for(unsigned detail:{0u,1u})for(unsigned first_pair:{0u,1u})for(unsigned second_pair:{0u,1u}){Scenario test{38,detail};test.payload[3]=0;test.payload[4]=first_pair?1:0xffffffffu;test.payload[5]=2;test.payload[6]=second_pair?3:0xffffffffu;if(!first_pair&&!second_pair)continue;cases.push_back(test);}
    for(unsigned count:{0u,1u,2u,3u,4u,5u,6u,7u,8u,9u,255u})for(unsigned day:{0u,7u}){Scenario test{21};test.payload[2]=3;test.payload[3]=2001;test.payload[4]=day;test.payload[5]=count;for(unsigned n=0;n<8;++n)test.payload[n+6]=n%4;cases.push_back(test);}
    for(unsigned normalized:{0u,0x41cu,10u,20u,0x82u,0x449u,0x44au}){Scenario test{21,0};test.normalized_round=normalized;cases.push_back(test);}
    // INJURY classification uses the real leaf/table:82 records, codes0..81.
    // Code82 characterizes adjacent mapped bytes outside that domain; scripted
    // "kind" returns would be bypassed.
    for(unsigned full:{0u,1u})for(unsigned code=0;code<=82;++code)for(unsigned person:{0u,1u,2u})for(unsigned duration:{7u,101u}){Scenario test{2000,full};test.payload[0]=person;test.payload[4]=code;test.payload[5]=2001;test.payload[6]=7;test.payload[7]=duration;test.payload[8]=2001;test.payload[9]=7;test.payload[10]=0xffffffffu;test.payload[13]=0xffffffffu;test.payload[14]=0xffffffffu;test.person_exists=person!=1;cases.push_back(test);}
    for(unsigned code:{59u,77u,78u,79u,80u,81u})for(unsigned replacement:{0xffffffffu,2u}){Scenario test{2000};test.payload[4]=code;test.payload[5]=2001;test.payload[6]=7;test.payload[7]=7;test.payload[10]=0xffffffffu;test.payload[13]=1;test.payload[14]=1;test.replacement_injury=replacement;cases.push_back(test);}
    for(unsigned record:{0u,1u})for(unsigned score:{0u,1u,2u})for(unsigned language:{0u,10u}){Scenario test{2000};test.payload[4]=59;test.payload[5]=2001;test.payload[6]=7;test.payload[7]=30;test.payload[10]=0;test.payload[11]=1;test.payload[12]=1;test.payload[13]=0xffffffffu;test.payload[14]=0;test.payload[15]=record?0x420e9:0;test.score_a=score;test.score_b=1;test.language_variant=language;cases.push_back(test);}
    for(const auto& weekday:std::array<std::string,4>{"dayC","dayA","dayz",""}){Scenario test{2000};test.payload[4]=59;test.payload[10]=0xffffffffu;test.payload[14]=0xffffffffu;test.payload[13]=0xffffffffu;test.language_variant=10;test.weekday=weekday;cases.push_back(test);}
    for(unsigned type:{1u,3u,20u,38u})for(unsigned full:{0u,1u})for(unsigned variant:{1u,2u}){Scenario test{type,full};test.round_name_variant=variant;cases.push_back(test);}
    for(unsigned variant:{1u,2u})for(unsigned language:{0u,2u}){Scenario test{20};test.payload[6]=0;test.venue_name_variant=variant;test.language_variant=language;cases.push_back(test);}
    for(unsigned id:{1u,3u})for(unsigned round:{1u,0x41cu,0x8000u,0xffffu}){Scenario test{21};test.payload[0]=id;test.payload[1]=round;test.payload[2]=3;test.payload[5]=2;test.payload[6]=1;test.payload[7]=2;cases.push_back(test);}
    for(unsigned season:{99u,1999u,65535u,0x123407d1u})for(unsigned home:{128u,256u}){Scenario test{13};test.payload[1]=3;test.payload[2]=season;for(unsigned n=0;n<3;++n){test.payload[3+n*4]=0xffff8000u;test.payload[4+n*4]=0x12340007;test.payload[5+n*4]=n;test.payload[6+n*4]=home;}cases.push_back(test);}
    for(unsigned type:{1u,3u,6u,9u})for(unsigned year:{0x8000u,0xffffu,0x123407d1u}){Scenario test{type};test.payload[1]=1;if(type==1){test.payload[5]=year;test.payload[6]=0x12340007;test.payload[7]=0xffffffffu;test.payload[12]=year;test.payload[13]=0x12340008;}else if(type==3){test.payload[5]=year;test.payload[8]=year;test.payload[9]=0x12340007;}else{test.payload[type==6?3:5]=year;test.payload[type==6?4:6]=0x12340007;}cases.push_back(test);}
    for(unsigned nation:{0xffffffffu,1u})for(unsigned club:{0xffffffffu,0u,1u})for(unsigned absent:{0u,1u}){Scenario test{2000};test.payload[10]=0xffffffffu;test.payload[13]=nation;test.payload[14]=club;test.no_active_manager=absent;test.person_exists=!absent;cases.push_back(test);}
    for(unsigned code:{59u,77u,81u}){Scenario test{2000};test.payload[4]=code;test.payload[10]=0xffffffffu;test.payload[13]=1;test.payload[14]=0;cases.push_back(test);}
    for(unsigned score:{0u,255u})for(unsigned flags=0;flags<8;++flags){Scenario test{2000};test.payload[4]=59;test.payload[10]=0;test.payload[11]=1;test.payload[12]=2;test.payload[15]=0x420e9;test.score_a=score;test.score_b=1;test.person_exists=!(flags&1);test.match_exists=!(flags&2);test.hide_competition=flags&4;cases.push_back(test);}
    for(char suffix:std::string("CSXZcsxzDB!")){Scenario test{2000};test.payload[4]=71;test.payload[10]=0xffffffffu;test.payload[14]=0xffffffffu;test.language_variant=10;test.weekday=std::string("day")+suffix;cases.push_back(test);}
    for(unsigned variant=0;variant<5;++variant){Scenario test{2000};test.payload[4]=59;test.payload[10]=0;test.payload[11]=1;test.payload[12]=2;test.payload[15]=0x420e9;if(variant<3)test.round_name_variant=variant;else if(variant==3)test.no_active_club=true;else test.payload[4]=71;test.language_variant=variant==4?10:0;cases.push_back(test);}
    for(unsigned duration:{7u,101u}){Scenario test{2000,0};test.payload[0]=0xfffffffau;test.payload[4]=5;test.payload[7]=duration;test.payload[10]=0xffffffffu;test.payload[14]=0xffffffffu;test.person_exists=false;cases.push_back(test);}
    Scenario no_club_table{19};no_club_table.no_club_table=true;cases.push_back(no_club_table);
    Scenario no_quote{5000};no_quote.payload[2]=0;cases.push_back(no_quote);
    // Incomplete bid records retain mapped predecessor names, as the original
    // still formats these values after skipping the optional nation links.
    for(unsigned position:{0u,2u,4u}){Scenario test{4};test.payload[3]=0xffffffffu;test.payload[2+position]=0xffffffffu;cases.push_back(test);}
    Scenario unknown_draw{39};unknown_draw.payload[7]=1;cases.push_back(unknown_draw);
    Scenario private_caption{5000,0};private_caption.payload[1]=1;cases.push_back(private_caption);
    Scenario quoted_message{5000};quoted_message.payload[2]=5;quoted_message.quoted_type=5000;quoted_message.quoted_self_reply=true;cases.push_back(quoted_message);
    for(unsigned detail:{0u,1u})for(unsigned absent=0;absent<3;++absent){Scenario test{19,detail};
        if(absent==0)test.no_origin=true;else if(absent==1)test.no_destination=true;else test.no_names=true;cases.push_back(test);
    }
    if(const auto selection=std::getenv("CM_NEWS_MESSAGE_TYPES")) {
        std::vector<unsigned> selected;std::string list(selection);std::size_t begin=0;
        do {const auto end=list.find(',',begin);selected.push_back(unsigned(std::stoul(list.substr(begin,end-begin))));
            if(end==std::string::npos)break;begin=end+1;}while(begin<list.size());
        std::erase_if(cases,[&](const auto& test){return std::find(selected.begin(),selected.end(),test.type)==selected.end();});
    }
    unsigned failures=0,returned_count=0;
    for(const auto& test:cases){active=test;auto state=cm::semantic_io_checks::clone(base);Cpu c(*state);initialize(c);
        const auto difference=compare_routine_call(*state,0x102be040,original,rewrite,RoutineChecker::result_r3|RoutineChecker::result_f1);
        if(!difference.empty()){++failures;std::cout<<"FAIL message type="<<test.type<<" detail="<<test.detail<<" case="<<(&test-cases.data())<<'\n';for(const auto& line:difference)std::cout<<line<<'\n';}
        else ++returned_count;
    }
    active=Scenario{5009};auto state=cm::semantic_io_checks::clone(base);Cpu c(*state);initialize(c);
    if(compare_routine_call(*state,0x102be040,original,rewrite,RoutineChecker::result_r3,true).empty())++failures;
    else std::cout<<"ok message corrupted result detected\n";
    std::cout<<cases.size()<<" message fixture comparisons, "<<returned_count<<" returned, "<<missing.size()<<" unimplemented local types excluded, "<<failures<<" failed\n";
    return failures;
}
}
