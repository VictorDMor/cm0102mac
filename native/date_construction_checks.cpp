#include "calendar.hpp"
#include "aot_runtime.hpp"
#include "date_subsystem_adapter.hpp"
#include <fstream>
#include <iostream>
#include <vector>

using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture = 0x30000000, stack = 0x700fff00, toc = 0x108bba90;
void require(bool value, const std::string& message) {
    if (!value) throw std::runtime_error(message);
}
struct Effects final : MemoryObserver {
    std::vector<std::uint8_t> events;
    void number(std::uint64_t value, unsigned width) {
        for (unsigned i=width;i;--i) events.push_back(std::uint8_t(value >> ((i-1)*8)));
    }
    void mapped(std::uint32_t start, std::uint64_t size) override {
        number(1,1); number(start,4); number(size,8);
    }
    void written(std::uint32_t start, const std::uint8_t* bytes, std::size_t size) override {
        number(2,1); number(start,4); number(size,8);
        events.insert(events.end(),bytes,bytes+size);
    }
    void cleared(std::uint32_t start, std::uint64_t size) override {
        number(3,1); number(start,4); number(size,8);
    }
};
void initialize(State& s, std::uint32_t entry, unsigned index) {
    s.memory.observer=nullptr; s.memory.read_trace=nullptr;
    s.registers.fill(0x5b); s.unique.fill(0x6d);
    for(unsigned reg=0;reg<32;++reg) s.wr<4>(reg*4,0x12340000+reg);
    s.wr<4>(4,stack); s.wr<4>(8,toc); s.wr<4>(2440,0xbadc0ffe);
    s.wr<4>(4128,return_sentinel); s.wr<4>(4132,0x31415926);
    s.wr<1>(2304,index%2?0xf9:0xa6); s.wr<1>(1024,index%4); s.wr<1>(1027,(index/2)%2);
    s.memory.copy_in(fixture,std::vector<std::uint8_t>(512,0xa5));
    s.memory.copy_in(stack-8192,std::vector<std::uint8_t>(8256,0xa7));
    for(auto slot:{0x108b7ae4u,0x108b7adcu,0x108b7ae0u,0x108b7ad8u})
        s.memory.write<4>(slot,fixture+128+(slot&15)*8);
    for(auto flag:{0x108b84d5u,0x108b84d3u,0x108b84d4u,0x108b84d2u})
        s.memory.write<1>(flag,index%4==0?0:index%4==1?1:index%4==2?128:255);
    s.pc=entry; s.executed=0; s.native_calls=0; s.budget=200000;
    s.calendar_original=true;
}
void put_date(State& s,std::uint32_t address,unsigned year,int day,unsigned flag) {
    s.memory.write<2>(address,day); s.memory.write<2>(address+2,year); s.memory.write<1>(address+4,flag);
}
}
int main(int argc,char** argv) {
    if(argc!=3 && argc!=4) return 2;
    try {
        auto s=std::make_unique<State>(); load_program(*s,argv[1]); s->memory.map(fixture,512);
        auto semantic=std::make_unique<State>(); load_program(*semantic,argv[1]); semantic->memory.map(fixture,512);
        const bool compare_semantic=argc==4;
        std::ofstream evidence(argv[2],std::ios::binary);
        require(bool(evidence),"Cannot open private construction characterization");
        const bool composed=argc==4&&std::string_view(argv[3])=="--composed";
        unsigned cases=0;
        auto check=[&](std::uint32_t entry,auto setup,auto oracle) {
            initialize(*s,entry,cases); setup(*s);
            const auto input=s->registers;
            Effects effects; std::vector<std::pair<std::uint32_t,unsigned>> reads;
            s->memory.observer=&effects; s->memory.read_trace=&reads;
            run(*s); s->memory.observer=nullptr; s->memory.read_trace=nullptr;
            oracle(*s);
            require(s->pc==return_sentinel && !s->native_calls,"Original core boundary did not return without imports");
            if(compare_semantic) {
                initialize(*semantic,entry,cases); setup(*semantic);
                semantic->calendar_original=!composed;
                Effects actual; std::vector<std::pair<std::uint32_t,unsigned>> actual_reads;
                semantic->memory.observer=&actual;semantic->memory.read_trace=&actual_reads;
                if(composed)run(*semantic);
                else {semantic->step(entry);require(execute_semantic_date(*semantic,entry),"Missing semantic construction entry");}
                semantic->memory.observer=nullptr;semantic->memory.read_trace=nullptr;
                if(s->registers!=semantic->registers) {
                    for(unsigned byte=0;byte<s->registers.size();++byte) if(s->registers[byte]!=semantic->registers[byte])
                        std::cerr<<"register byte="<<byte<<" original="<<unsigned(s->registers[byte])<<" semantic="<<unsigned(semantic->registers[byte])<<'\n';
                    throw std::runtime_error("Construction ABI differs at "+hex_address(entry)+" case="+std::to_string(cases));
                }
                require(s->unique==semantic->unique && s->pc==semantic->pc,"Construction temporaries/PC differ");
                require(reads==actual_reads,"Construction ordered reads differ at "+hex_address(entry));
                require(effects.events==actual.events,"Construction ordered write/map/clear effects differ at "+hex_address(entry));
                require(s->memory.mapped_ranges()==semantic->memory.mapped_ranges(),"Construction mapped ranges differ");
                for(const auto& [start,size]:s->memory.mapped_ranges()) for(std::uint64_t offset=0;offset<size;offset+=65536) {
                    const auto length=std::size_t(std::min<std::uint64_t>(size-offset,65536));
                    require(s->memory.copy_out(start+std::uint32_t(offset),length)==semantic->memory.copy_out(start+std::uint32_t(offset),length),"Construction entire mapped memory differs");
                }
                require(s->allocated==semantic->allocated && s->available==semantic->available && s->heap_end==semantic->heap_end &&
                        s->memory_error==semantic->memory_error && s->native_calls==semantic->native_calls,"Construction allocator/import state differs");
            }
            auto bytes=[&](const auto& value) {evidence.write(reinterpret_cast<const char*>(&value),sizeof(value));};
            bytes(entry); bytes(input); bytes(s->registers); bytes(s->pc);
            const auto n=std::uint32_t(effects.events.size()); bytes(n);
            evidence.write(reinterpret_cast<const char*>(effects.events.data()),n);
            const auto nr=std::uint32_t(reads.size()); bytes(nr);
            for(const auto& [address,width]:reads) {bytes(address);bytes(width);}
            ++cases;
        };
        const std::vector<unsigned> years{0,1,4,1599,1600,1601,1900,2000,2001,2002,2100,32767,32768,65535};
        for(unsigned year:years) for(unsigned high:{0u,0xa5a50000u,0xffff0000u})
            check(0x10718440,[&](State& a){a.wr<4>(12,high|year);},[&](const State& a){
                require(a.r<4>(12)==cm::calendar::is_leap(year),"Original leap classification differs");
                require(a.r<4>(20)==year,"Original leap width differs");
            });
        for(int offset:{0,1,2,3,-1,-2,-3,28,32})
            check(0x10715880,[&](State& a){a.wr<4>(12,offset<20?fixture+32+offset:stack+offset);},[&](const State& a){
                const auto destination=std::uint32_t(a.r<4>(12));
                require(a.memory.read<2>(destination)==40 && a.memory.read<2>(destination+2)==1972 &&
                        a.memory.read<1>(destination+4)==1 && a.r<4>(0)==1 && a.r<4>(16)==1972,"Original initialization contract differs");
            });
        for(unsigned year:years) for(int day:{-32768,-1,0,40,59,365,32767}) for(unsigned high:{0u,0xa5a50000u})
            check(0x107156b0,[&](State& a){a.wr<4>(12,fixture+32);a.wr<4>(16,high|std::uint16_t(day));a.wr<4>(20,high|year);},[&](const State& a){
                require(a.r<4>(12)==fixture+32 && a.memory.read<2>(fixture+32)==std::uint16_t(day) &&
                        a.memory.read<2>(fixture+34)==year && a.memory.read<1>(fixture+36)==cm::calendar::is_leap(year),"Original ordinal contract differs");
            });
        for(auto entry:{0x107156f0u,0x107158a0u}) for(unsigned year:years) for(unsigned month:{0u,1u,2u,11u})
            for(unsigned day:{1u,28u,29u,31u})
                check(entry,[&](State& a){a.wr<4>(12,fixture+32);a.wr<4>(16,0xa5a50000|day);a.wr<4>(20,0xa5a50000|month);a.wr<4>(24,0xa5a50000|year);a.wr<4>(28,0x1234ffff);},[&](const State& a){
                    const auto table=std::uint32_t(a.memory.read<4>(toc-(cm::calendar::is_leap(year)?0x3fa4:0x3fa8)));
                    const auto ordinal=cm::calendar::narrow_day(day+signed_value<2>(a.memory.read<2>(table+month*2)));
                    require(a.memory.read<2>(fixture+32)==std::uint16_t(ordinal) && a.memory.read<2>(fixture+34)==year &&
                            a.memory.read<1>(fixture+36)==cm::calendar::is_leap(year),"Original civil threshold contract differs");
                    if(entry==0x107156f0) require(a.r<4>(12)==fixture+32,"Original civil A return differs");
                });
        for(unsigned requested=0;requested<7;++requested)
            check(0x10715a30,[&](State& a){put_date(a,fixture+16,2002,243,0);a.wr<4>(12,fixture+16);a.wr<4>(16,0xa5a50000|requested);a.wr<4>(20,fixture+32);},[&](const State& a){
                int delta=6-int(requested);if(delta>3)delta-=7;
                require(a.memory.read<2>(fixture+32)==unsigned(243-delta) && a.memory.read<2>(fixture+34)==2002 &&
                        a.memory.read<1>(fixture+36)==0,"Original nearest weekday contract differs");
            });
        for(auto entry:{0x10717a10u,0x10717cb0u}) for(unsigned year:{0u,1900u,2000u,2002u,65535u})
            for(unsigned weeks:{0u,1u,255u})
                check(entry,[&](State& a){put_date(a,fixture+16,year,59,255);a.wr<4>(12,fixture+32);a.wr<4>(16,fixture+16);a.wr<4>(20,0xa5a50000|weeks);},[&](const State& a){
                    const auto direction=entry==0x10717a10?cm::calendar::Direction::forward:cm::calendar::Direction::backward;
                    const auto result=cm::calendar::shift({59,std::uint16_t(year)},weeks*7,direction);
                    require(a.memory.read<2>(fixture+32)==std::uint16_t(result.day) && a.memory.read<2>(fixture+34)==result.year &&
                            a.memory.read<1>(fixture+36)==cm::calendar::is_leap(result.year),"Original unsigned8 weeks differs");
                });
        for(unsigned first_year:{0u,1599u,1900u,2000u,2002u,65535u})for(unsigned second_year:{0u,1600u,2000u,2002u,65535u})
            for(int first_day:{-32768,-1,0,59,365,32767})for(int second_day:{-32768,-1,0,243,365,32767})
                check(0x10717dd0,[&](State& a){
                    put_date(a,fixture+16,first_year,first_day,255);put_date(a,fixture+32,second_year,second_day,128);
                    a.wr<4>(12,fixture+64);a.wr<4>(16,fixture+16);a.wr<4>(20,a.memory.read<4>(fixture+32));a.wr<4>(24,a.memory.read<4>(fixture+36));
                },[](const State&){});
        for(auto entry:{0x107156b0u,0x107156f0u,0x107158a0u})for(auto output:{fixture+16,fixture+17,fixture+18,fixture+19,fixture+20,stack-40,stack+28})
            for(unsigned year:{1599u,1600u,2000u,2002u})
                check(entry,[&](State& a){
                    a.wr<4>(12,output);a.wr<4>(16,59);a.wr<4>(20,entry==0x107156b0?year:1);a.wr<4>(24,year);a.wr<4>(28,0xffff);
                    if(entry!=0x107156b0){a.wr<4>(16,31);auto table=std::uint32_t(a.memory.read<4>(toc-(cm::calendar::is_leap(year)?0x3fa4:0x3fa8)));a.memory.write<2>(table+2,61);}
                },[](const State&){});
        for(auto entry:{0x10715a30u,0x10717a10u,0x10717cb0u})for(int alias:{-4,-3,-2,-1,0,1,2,3,4,32})
            for(unsigned year:{1599u,1600u,2000u,2002u})for(unsigned flag:{0u,1u,128u,255u})
                check(entry,[&](State& a){
                    put_date(a,fixture+48,year,59,flag);
                    if(entry==0x10715a30){a.wr<4>(12,fixture+48);a.wr<4>(16,0xa5a50000|unsigned(cases%10));a.wr<4>(20,fixture+48+alias);}
                    else{a.wr<4>(12,fixture+48+alias);a.wr<4>(16,fixture+48);a.wr<4>(20,0xa5a50000|(cases%2?255:0));}
                },[](const State&){});
        for(int requested:{-32768,-32767,-1,0,1,3,4,6,7,32767})for(unsigned year:{0u,1599u,1600u,1601u,2000u,2100u,65535u})
            check(0x10715a30,[&](State& a){put_date(a,fixture+16,year,59,128);a.wr<4>(12,fixture+16);a.wr<4>(16,0xa5a50000|std::uint16_t(requested));a.wr<4>(20,fixture+32);},[](const State&){});
        for(int month:{-128,-1,0,3,9,127}) for(int parameter:{-32768,-1,0,4,5,6,7,32767})
            check(0x10718af0,[&](State& a){a.wr<4>(12,0xa5a50000|std::uint8_t(month));a.wr<4>(16,0xa5a50000|std::uint16_t(parameter));},[&](const State& a){
                const bool expected=month==9?(parameter<5||parameter>=7):month==3;
                require(a.r<4>(12)==expected,"Original month predicate differs");
            });
        require(bool(evidence),"Cannot finalize private construction characterization");
        std::cout<<cases<<(compare_semantic?" original/semantic":" original")<<" core cases: narrow/high-bit contracts, mutable table-based civil construction, February 31, nearest weekday, unsigned8 weeks, initialization and month predicate; complete raw ABI and ordered memory evidence retained privately\n";
        return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
