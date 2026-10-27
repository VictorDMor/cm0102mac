// Every SCRMAN entry is compared with isolated, hook-free original pages.
// Dependency stand-ins expose call order, continuations and preserved values;
// real-capture replays separately check the actual dependency implementations.
#include "screen_manager_adapter.hpp"
#include "screen_manager_detail.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <iostream>
#include <set>

using namespace cm::aot;
using guest::Cpu;
namespace cm::aot {bool screen_manager_reference(State&);}
namespace {
constexpr std::uint32_t data=0x60000000,object=data+0x10000,buffer=data+0x600000,
    sound=data+0x700000,nodes=data+0x400000,log_at=data+0xe00000,sentinel=0xfffffff0;
struct Scenario {
    std::uint32_t entry;
    std::string name="default";
    int index=1,event=-7,control=-1,current=0;
    std::uint32_t lookup=0x100;
    unsigned groups=2,rows=2,nodes=3,capacity=128,fail_allocation=0;
    bool active=false,defaults=false,network=true,callback=true,owned=false,sound_enabled=true,wide_sound=true,copy_string=false;
    bool client_receive=true,client_filter=false;
};
Scenario active{};
bool reference_mode=false;
unsigned allocations=0,calls=0,input_calls=0,network_calls=0;
std::uint32_t next_allocation=0;
const std::set<std::uint32_t> dependencies{
    0x1073ae10,0x10718b40,0x10718ba0,0x1073aee0,0x10718ef0,0x1073a120,0x10739f70,0x1073a220,0x1073a7f0,
    0x10719560,0x10718f30,0x10718fc0,0x1071c1d0,0x1071de80,0x10719350,0x1076fe80,0x1073a480,0x1073a4c0,
    0x10739890,0x10768810,0x1077b720,0x10769b60,0x1076f7d0,0x10773520,0x107735f0,0x107735a0,0x10773610,
    0x1077f900,0x10772df0,0x10719740,0x107145b0,0x1071c070,0x10746470,0x1074c5d0,0x1074eec0,0x1074eff0,
    0x1077fee0,0x10780918,0x10780978,0x10780990,0x10780948,0x1077faf0,0x1077fc58,0x1077fc70,0x10780a50,
    0x10780a38,0x107809f0,0x10780a08,0x10780a20,0x10780960,0x10780930,0x1076a580,0x10727890,0x1076e160,
    0x107692f0,0x10723cb0,0x10723a30};
unsigned arguments(std::uint32_t target) {
    switch(target) {
    case 0x10739890:return 5;case 0x1077b720:return 8;case 0x10719740:case 0x1076e160:return 8;
    case 0x10769b60:case 0x10739f70:case 0x1073a4c0:case 0x1071de80:case 0x1077fee0:return 4;
    case 0x1073a7f0:case 0x10772df0:case 0x10780990:case 0x10780918:case 0x10780960:case 0x10746470:
    case 0x1074eff0:case 0x10723cb0:return 3;
    case 0x10718b40:case 0x10718ba0:case 0x1073aee0:case 0x1073a120:case 0x10719560:case 0x1073a480:
    case 0x1071c1d0:case 0x10780948:case 0x10780a38:case 0x1074c5d0:case 0x1074eec0:case 0x10727890:return 2;
    case 0x1076fe80:case 0x10768810:case 0x10780a08:case 0x10780a20:return 0;
    case 0x10718fc0:return 5;case 0x1076a580:return 6;case 0x107692f0:return 4;
    default:return 1;
    }
}
void trace(Cpu& c,std::uint32_t target) {
    if(++calls>3000)throw std::runtime_error("fixture call limit");
    auto index=c.u32(log_at);
    auto add=[&](std::uint32_t value){c.put32(log_at+4+index++*4,value);};
    add(target);add(c.lr());for(unsigned r=3;r<3+arguments(target);++r)add(c.r(r));
    for(unsigned r=13;r<32;++r)add(c.r(r));
    // The dependencies that consume outgoing stack arguments expose them too.
    if(target==0x10719740)for(unsigned off=0x38;off<=0x60;off+=4)add(c.u32(c.sp()+off));
    c.put32(log_at,index);
}
std::uint32_t allocate(Cpu& c,std::uint32_t size) {
    if(++allocations==active.fail_allocation)return 0;
    auto result=next_allocation;next_allocation+=(size+31)&~15u;
    if(next_allocation>=data+0xd00000)throw std::runtime_error("fixture allocation limit");
    c.set(3,result);return result;
}
bool probe(State& s) {
    Cpu c(s);auto target=s.pc;
    if(target==0x10781320) {
        const auto descriptor=c.r(12);c.put32(c.sp()+0x14,c.r(2));target=c.u32(descriptor);
        trace(c,target);
        c.set(3,target==0x5f000008?(++input_calls==1?std::uint32_t(active.event):std::uint32_t(-7)):1);
        s.pc=c.lr();return true;
    }
    if(screen_manager_lists(target)) {
        if(reference_mode)return screen_manager_reference(s);
        screen_manager_run_rewrite(s,target);return true;
    }
    if(reference_mode && target>=0x1073c1b0 && target<0x10744fc0)return screen_manager_reference(s);
    if(target==0x10745e00)return false;
    if(run_leaf(s))return true;
    if(!dependencies.contains(target))return false;
    trace(c,target);
    const bool imported=target>=0x1077f000 && target!=0x1077f900;
    if(imported)c.put32(c.sp()+0x14,c.r(2));
    if(target==0x10773520 || target==0x107735f0 || target==0x1077faf0 || target==0x1077fc70) {
        const auto size=c.r(3);c.set(3,allocate(c,size));
    } else if(target==0x10739890) {
        c.put32(c.r(6),0x53435200);c.put32(c.r(7),0x43505000);
    } else if(target==0x1077b720) {c.put32(c.r(3),c.r(8));c.set(3,1);}
    else if(target==0x1076f7d0)throw std::runtime_error("fixture fatal assertion");
    else if(target==0x1073ae10){c.put32(c.r(3),buffer);c.put32(c.r(3)+4,0);c.put32(c.r(3)+8,active.capacity);}
    else if(target==0x10739f70){c.put8(c.r(4),1);}
    else if(target==0x1073a7f0){++network_calls;c.set(3,active.entry==0x1073da60 && active.client_receive?1:0);}
    else if(target==0x1071de80)c.set(3,std::uint32_t(-2));
    else if(target==0x10719350)c.set(3,std::uint32_t(active.control));
    else if(target==0x1077f900)c.put32(c.r(3),100);
    else if(target==0x10780918){c.put16(c.r(5),7);c.set(3,0);}
    else if(target==0x10780990){const auto output=c.r(3);const auto channel=allocate(c,0x100);c.put32(output,channel);c.set(3,0);}
    else if(target==0x1074eec0){c.put16(c.r(4)+8,2);c.put32(c.r(4)+10,16);c.put16(c.r(4)+14,16);c.set(3,0);}
    else if(target==0x107809f0){c.put16(c.r(3),1);c.put32(c.r(3)+2,2);c.put32(c.r(3)+6,3);c.set(3,0);}
    else if(target==0x1077fee0 || target==0x10780948 || target==0x1074eff0 || target==0x10780960)c.set(3,0);
    else if(target==0x10723cb0)c.set(3,0);
    else if(target==0x107735a0 || target==0x10773610 || target==0x1077fc58) {} // Free returns no value.
    else c.set(3,1);
    s.pc=c.lr();return true;
}
void initialize(Cpu& c) {
    c.s.memory.map(data,0x1000000);c.s.memory.map(0x71000000,0x20000);
    c.s.memory.copy_in(0x71000000,std::vector<std::uint8_t>(0x20000,0xa5));
    for(unsigned r=0;r<32;++r)c.set(r,0x51000000+r*0x101);
    c.set(1,0x71010000);c.set(2,0x108bba90);c.set_lr(sentinel);
    for(unsigned f=0;f<8;++f)c.set_cr(f,f+1);
    auto global=[&](int offset,std::uint32_t value){c.put32(c.r(2)+std::uint32_t(offset),value);};
    global(-0x7bb0,object);global(-0x7ba8,data+0x100);global(-0x7bac,data+0x200);global(-0x7bb8,data+0x300);
    global(-0x7468,data+0x400);c.put32(data+0x400,sound);global(-0x5c04,data+0x500);
    c.put32(data+0x500,buffer);c.put32(data+0x508,active.capacity);global(-0x5c0c,data+0x600);
    global(-0x5c24,data+0x700);c.put16(data+0x700,1);global(-0x5b98,data+0x800);c.put8(data+0x800,active.wide_sound);
    global(-0x7bd0,data+0x900);global(-0x7bd4,data+0xa00);global(-0x3e64,data+0xb00);
    for(unsigned i=0;i<4;++i){c.put32(data+0xc00+i*8,0x5f000000+i*4);c.put32(data+0xc04+i*8,c.r(2));}
    c.put32(object+0x2c64,active.network?data+0xd00:0);c.put32(object+0x2c2c,buffer);c.put32(object+0x2c30,0);c.put32(object+0x2c34,active.capacity);
    c.put16(object+0x2c58,active.current);c.put16(object+0x2c5a,0xffff);c.put16(object+0x2c5c,1);c.put8(object+0x2c5e,1);
    for(unsigned group=0;group<16;++group) {
        c.put16(object+0x2c38+group*2,group<active.groups?active.rows:0);
        for(unsigned row=0;row<16;++row) {
            const auto screen=object+group*0x2c0+row*0x2c;
            c.put16(screen,group);c.put32(screen+4,active.defaults && active.nodes==0?0:0x100+group*16+row);c.put8(screen+24,1);c.put8(screen+25,0);c.put8(screen+26,0);
            c.put32(screen+0x24,group?object+0x12d6b0:object+0x2c68);
            const auto first=nodes+(group*16+row)*0x1000;
            c.put32(screen+8,active.nodes?first:0);c.put32(screen+12,active.nodes?first+(active.nodes-1)*0x300:0);
            c.put32(screen+16,active.nodes?first:0);c.put16(screen+20,active.active?active.nodes:0);c.put16(screen+22,active.nodes);
            for(unsigned i=0;i<active.nodes;++i) {
                const auto node=first+i*0x300;
                c.put32(node,0x100+i);c.put32(node+4,active.callback?data+0xc08:0);c.put32(node+8,0x200+i);c.put32(node+12,active.callback?data+0xc0c:0);
                c.put8(node+16,active.active);c.put32(node+0x1f4,i?node-0x300:0);c.put32(node+0x1f8,i+1<active.nodes?node+0x300:0);
                for(unsigned field=0;field<60;++field){c.put32(node+20+field*8,data+0x3000);c.put8(node+24+field*8,active.owned);}
                c.put8(node+0x1fc,'A');c.put8(node+0x1fd,0);
            }
        }
    }
    // Screen callbacks live in the first four words of a history node.
    for(unsigned group=0;group<16;++group)for(unsigned row=0;row<16;++row)for(unsigned i=0;i<active.nodes;++i) {
        const auto node=nodes+(group*16+row)*0x1000+i*0x300;
        c.put32(node,data+0xc00);c.put32(node+4,active.callback?data+0xc08:0);c.put32(node+8,data+0xc10);c.put32(node+12,0);
    }
    c.put32(object+0x2c00,active.defaults?0x100:0);c.put32(object+0x2c04,0x200);c.put8(object+0x2c08,active.active);
    c.put32(object+0x2c14,data+0x3000);c.put16(object+0x2c18,2);
    c.put8(sound+0x28,active.sound_enabled);c.put32(sound,data+0x4000);c.put32(data+0x400c,data+0x5000);
    c.put8(data+0x5008,1);c.put8(data+0x5009,0);c.put8(data+0x500a,0);c.put16(data+0x500c,7);
    c.put32(data+0x5000,data+0x6000);c.put32(data+0x5004,data+0x7000);
    c.put8(data+0x3000,'X');c.put8(data+0x3001,0);
    c.set({{3,object},{4,std::uint32_t(active.index)},{5,data+0x3000},{6,0},{7,0},{8,0},{9,data+0x3000},{10,2}});
    const auto entry=active.entry;
    if(entry==0x1073e350)c.set({{4,0},{5,0x100},{6,0x200},{7,0},{8,0},{9,0}});
    if(entry==0x1073e4b0)c.set({{4,0x100},{5,0x200},{6,active.active},{7,0},{8,0}});
    if(entry==0x1073e990)c.set({{4,0x100},{5,0x200},{6,0},{7,0},{8,0},{9,data+0x3000},{10,2}});
    if(entry==0x1073f0a0)c.set({{4,0},{5,std::uint32_t(active.index)},{6,data+0x3000},{7,0}});
    if(entry==0x1073ede0)c.set(4,data+0x3000);
    if(entry==0x1073f1e0)c.set(6,active.copy_string);
    if(entry==0x1073f900 || entry==0x1073f7a0)c.set({{4,0x100},{5,entry==0x1073f900?0xffffffffu:0x101u}});
    if(entry==0x1073fc20 || entry==0x1073fd60 || entry==0x1073fdf0 || entry==0x1073fe80 || entry==0x1073ffe0)c.set(4,active.lookup);
    if(entry==0x1073f860)c.set({{4,active.callback?data+0xc08:0},{5,0},{6,0}});
    if(entry==0x1073fd00)c.set({{4,data+0x3100},{5,data+0x3200},{6,0}});
    if(entry==0x1073f530 || entry==0x1073f700)c.set(3,active.index);
    if(entry==0x1073da60 && !active.client_receive) {
        c.put8(object+0x257ff3,1);
        if(active.client_filter)c.put32(object+0x2c1c,data+0xc08);
    }
    if(entry==0x1073c460 || entry==0x107446b0)c.set(4,active.index);
    if(entry>=0x10744670)c.set(3,sound);
    if(entry==0x10744860)c.set({{4,0},{5,data+0x3000},{6,1}});
    if(entry==0x10744a60)c.set(4,data+0x4000);
    if(entry==0x10744ee0)c.set({{3,data+0x4000},{4,data+0x3100}});
    if(entry==0x10744e50 || entry==0x10744ca0)c.set({{4,entry==0x10744e50?std::uint32_t(active.index):0u},{5,entry==0x10744e50?std::uint32_t(active.control):100u}});
    if(entry==0x10742740)c.set({{4,data+0x3200},{5,0},{6,data+0x3000},{7,0}});
    if(entry==0x10742e60)c.set(5,data+0x3200);
    if(entry==0x107439b0)c.set(4,data+0x3000);
    if(entry==0x10743120)c.set({{4,data+0x3000},{5,0x123},{6,0x456},{7,0xffffffff}});
    c.s.budget=2000000;
}
void returned(State& s) {
    Cpu c(s);if(c.sp()!=0x71010000 || c.lr()!=sentinel)throw std::runtime_error("fixture did not return");
}
void original(State& s,std::uint32_t entry){reference_mode=true;s.pc=entry;run_until(s,sentinel);returned(s);}
void rewrite(State& s,std::uint32_t entry){reference_mode=false;screen_manager_run_rewrite(s,entry);returned(s);}
}
int main(int argc,char** argv) {
    if(argc<2){std::cerr<<"Usage: cm-screen-manager-checks <export> [entry]\n";return 2;}
    try {
        auto base=std::make_unique<State>();load_program(*base,argv[1]);dispatch_probe=probe;
        std::vector<Scenario> cases;
#define SCREEN_MANAGER_ROUTINE(entry,end,results,digest,...) cases.push_back(Scenario{entry});
#include "screen_manager_routines.inc"
#undef SCREEN_MANAGER_ROUTINE
        const auto defaults=cases;
        for(auto test:defaults) {
            if(test.entry>=0x10740070 && test.entry<=0x10743eb0)for(unsigned cap:{0u,1u,2u,3u,4u,7u,12u}) {
                test.capacity=cap;test.name="capacity "+std::to_string(cap);cases.push_back(test);
            }
        }
        for(auto test:defaults) {
            if(test.entry==0x10744e50)for(int value:{-99999,-100,-1,0,1,99,100,101,2147483647}) {
                test.index=value;test.name="volume "+std::to_string(value);cases.push_back(test);
            }
            if(test.entry==0x1073ef70 || test.entry==0x1073f1e0 || (test.entry>=0x10740620 && test.entry<=0x10742420) ||
               test.entry==0x10744c50 || test.entry==0x10744ca0 || test.entry==0x10742b80) {
                for(int index:{-128,-1,0,2,9,10,59,60,127,255}) {
                    if(test.entry==0x10742b80 && index!=-1 && (index<0 || index>=10))continue;
                    test.index=index;test.name="index "+std::to_string(index);cases.push_back(test);
                }
            }
            if(test.entry==0x1073fd60 || test.entry==0x1073fdf0 || test.entry==0x1073fe80 || test.entry==0x1073ffe0 || test.entry==0x1073fc20) {
                test.lookup=0x111;test.current=1;test.name="nonzero group and row";cases.push_back(test);
                test.lookup=0x1234;test.name="missing key";cases.push_back(test);
            }
            if(test.entry==0x1073c730)for(int event:{-13,-12,-11,-10,-9,-8,-6,-5,-4,-3,-2,-1,0,1,7}) {
                test.event=event;test.name="event "+std::to_string(event);cases.push_back(test);
            }
            if(test.entry==0x1073e350 || test.entry==0x1073e4b0 || test.entry==0x1073ee30 || test.entry==0x1073c4e0 ||
               test.entry==0x1073f900 || test.entry==0x107440c0 || test.entry==0x1073c730) {
                test.active=true;test.owned=true;test.name="active owned history";cases.push_back(test);
            }
            if(test.entry==0x1073c460 || test.entry==0x107446b0) {test.index=0;test.name="nondisposing destructor";cases.push_back(test);}
            if(test.entry==0x1073f530 || test.entry==0x1073f860 || test.entry==0x1073e4b0) {
                test.callback=false;test.defaults=true;test.name="defaults without callback";cases.push_back(test);
            }
        }
        for(auto test:defaults) {
            if(test.entry==0x1073c730)for(bool owned:{false,true})for(int event:{-13,-10,-9,-8,-6,-5,-3,-2,-1}) {
                test.active=true;test.owned=owned;test.event=event;
                test.name="active event "+std::to_string(event)+(owned?" owned":" borrowed");cases.push_back(test);
            }
            if(test.entry==0x1073da60)for(int control:{0,1,59}) {
                test.client_receive=false;test.control=control;test.name="local client input "+std::to_string(control);cases.push_back(test);
            }
            if(test.entry==0x1073da60) {
                test.client_receive=false;test.client_filter=true;test.control=1;test.event=-9;
                test.name="client filtered input and history removal";cases.push_back(test);
            }
            if(test.entry==0x1073f1e0)for(unsigned fail:{0u,1u}) {
                test.copy_string=true;test.fail_allocation=fail;test.name="owned string allocation "+std::to_string(fail);cases.push_back(test);
            }
            if(test.entry==0x1073e4b0) {
                test.defaults=true;test.nodes=0;test.name="default history from empty group";cases.push_back(test);
            }
            if(test.entry==0x10744e50)for(bool wide:{false,true})for(int side:{-1,0,1,2})for(int percent:{-99999,-100,-1,0,1,100,2147483647}) {
                test.wide_sound=wide;test.control=side;test.index=percent;
                test.name="volume wide="+std::to_string(wide)+" side="+std::to_string(side)+" percent="+std::to_string(percent);cases.push_back(test);
            }
            if(test.entry>=0x10740070 && test.entry<=0x10743eb0) {
                test.current=1;test.name="remote current group";cases.push_back(test);
                test.callback=false;test.name="remote without callback";cases.push_back(test);
            }
        }
        unsigned failures=0,checked=0;
        for(const auto& test:cases) {
            if(argc>2 && test.entry!=std::stoul(argv[2],nullptr,0))continue;
            std::cout<<"checking "<<hex_address(test.entry)<<" "<<test.name<<std::endl;
            auto state=cm::semantic_io_checks::clone(*base);Cpu c(*state);active=test;allocations=calls=input_calls=network_calls=0;next_allocation=data+0x800000;
            initialize(c);state->pc=test.entry;
            const auto differences=compare_routine_call(*state,test.entry,original,rewrite,RoutineChecker::result_r3);
            std::cout<<(differences.empty()?"ok ":"DIFFERS ")<<hex_address(test.entry)<<" "<<test.name<<'\n';
            for(const auto& line:differences)std::cout<<"  "<<line<<'\n';
            ++checked;failures+=!differences.empty();
        }
        active=defaults.front();allocations=calls=input_calls=network_calls=0;next_allocation=data+0x800000;
        auto negative_state=cm::semantic_io_checks::clone(*base);Cpu nc(*negative_state);initialize(nc);
        if(compare_routine_call(*negative_state,active.entry,original,rewrite,RoutineChecker::result_r3,true).empty()) {
            ++failures;std::cout<<"FAIL corrupted result undetected\n";
        }else std::cout<<"ok corrupted result detected\n";
        std::cout<<checked<<" comparisons, "<<failures<<" failed\n";return failures?1:0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 2;}
}
