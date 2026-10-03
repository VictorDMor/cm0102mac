#include "calendar.hpp"
#include "calendar_adapter.hpp"
#include "callbacks.hpp"
#include <chrono>
#include <iostream>
#include <unistd.h>

using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture = 0x30000000, entry = 0x10718360;
constexpr std::array<std::int16_t,12> common{-1,30,58,89,119,150,180,211,242,272,303,333};
constexpr std::array<std::int16_t,12> leap{-1,30,59,90,120,151,181,212,243,273,304,334};
void require(bool test, const std::string& message) { if (!test) throw std::runtime_error(message); }
struct NoWrites final : MemoryObserver {
    void mapped(std::uint32_t, std::uint64_t) override { throw std::runtime_error("Month mapped memory"); }
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override { throw std::runtime_error("Month wrote memory"); }
    void cleared(std::uint32_t, std::uint64_t) override { throw std::runtime_error("Month cleared memory"); }
};
void same_memory(const State& a, const State& b) {
    require(a.memory.mapped_ranges() == b.memory.mapped_ranges(), "Month mapped ranges differ");
    for (const auto& [start,size] : a.memory.mapped_ranges())
        for (std::uint64_t offset=0; offset<size; offset+=65536) {
            const auto length=std::size_t(std::min<std::uint64_t>(size-offset,65536));
            require(a.memory.copy_out(start+std::uint32_t(offset),length)==
                    b.memory.copy_out(start+std::uint32_t(offset),length), "Month entire memory differs");
        }
}
void initialize(State& s, unsigned year, int day, unsigned flag, unsigned index) {
    const auto stack=s.r<4>(4);
    constexpr std::uint32_t toc=0x108bba90;
    s.registers.fill(0x5b); s.unique.fill(0x6d);
    for (unsigned r=0;r<32;++r) s.wr<4>(r*4,0x12340000+r);
    s.wr<4>(4,stack); s.wr<4>(8,toc); s.wr<4>(4128,return_sentinel);
    s.memory.copy_in(fixture,std::vector<std::uint8_t>(256,0xa5));
    s.memory.write<2>(fixture+16,std::uint16_t(day)); s.memory.write<2>(fixture+18,year);
    s.memory.write<1>(fixture+20,flag); s.wr<4>(12,fixture+16);
    s.wr<1>(2304,index%2?0xf9:0x06); s.wr<1>(1024,index%2); s.wr<1>(1027,(index/2)%2);
    s.pc=entry; s.executed=0; s.budget=160; s.native_calls=0;
    s.calendar_calls={}; s.date_difference_calls=0; s.date_ordering_calls=0; s.date_before_calls=0; s.date_month_calls=0;
}
}
int main(int argc,char** argv) {
    if (argc!=2) return 2;
    try {
        auto a=std::make_unique<State>(), b=std::make_unique<State>();
        load_program(*a,argv[1]); load_program(*b,argv[1]); a->calendar_original=true;
        for (auto* s:{a.get(),b.get()}) s->memory.map(fixture,256);
        unsigned count=0, oracle=0;
        auto equivalent=[&] {
            if(a->registers!=b->registers) {
                for(unsigned i=0;i<a->registers.size();++i) if(a->registers[i]!=b->registers[i])
                    std::cerr<<"register byte "<<i<<" original="<<unsigned(a->registers[i])<<" modern="<<unsigned(b->registers[i])<<'\n';
                throw std::runtime_error("Month direct ABI mismatch case="+std::to_string(count));
            }
            require(a->unique==b->unique && a->pc==b->pc,"Month temporaries/PC differ"); same_memory(*a,*b);
            require(a->allocated==b->allocated && a->available==b->available && a->heap_end==b->heap_end &&
                    a->memory_error==b->memory_error && !a->native_calls && !b->native_calls,"Month allocator/import effects differ");
        };
        auto check=[&](unsigned year,int day,unsigned flag,bool mutated=false,bool relocated=false) {
            for(auto* s:{a.get(),b.get()}) {
                initialize(*s,year,day,flag,count);
                if(relocated) {
                    s->wr<4>(8,fixture+0x4000);
                    s->memory.write<4>(fixture+0x4000-0x3fa8,0x108e14ac);
                    s->memory.write<4>(fixture+0x4000-0x3fa4,0x108e14c4);
                }
            }
            std::array<std::int16_t,12> thresholds{};
            const auto slot=std::uint32_t(a->r<4>(8))-(flag?0x3fa4u:0x3fa8u);
            const auto table=std::uint32_t(a->memory.read<4>(slot));
            if(mutated) for(auto* s:{a.get(),b.get()}) {
                s->memory.write<4>(slot,fixture+64);
                for(unsigned i=0;i<12;++i) s->memory.write<2>(fixture+64+i*2,std::uint16_t(i%3?int(i)*7-8:300-int(i)*2));
            }
            const auto effective_table=std::uint32_t(a->memory.read<4>(slot));
            for(unsigned i=0;i<12;++i) thresholds[i]=cm::calendar::narrow_day(int(a->memory.read<2>(effective_table+i*2)));
            if(!mutated) require(thresholds==(flag?leap:common),"Original table values changed");
            NoWrites observer; std::vector<std::pair<std::uint32_t,unsigned>> ar,br;
            a->memory.read_trace=&ar; b->memory.read_trace=&br;
            a->memory.observer=&observer; b->memory.observer=&observer;
            run(*a); run(*b);
            a->memory.read_trace=nullptr; b->memory.read_trace=nullptr;
            a->memory.observer=nullptr; b->memory.observer=nullptr;
            if(ar!=br) {
                for(const auto& [address,width]:ar) std::cerr<<"original read "<<hex_address(address)<<"/"<<width<<'\n';
                for(const auto& [address,width]:br) std::cerr<<"modern read "<<hex_address(address)<<"/"<<width<<'\n';
                throw std::runtime_error("Month ordered reads differ case="+std::to_string(count));
            }
            require(ar.size()>=4 && ar[0]==std::pair(fixture+20,1u) && ar[1]==std::pair(slot,4u) &&
                    ar[2]==std::pair(effective_table+22,2u) && ar[3]==std::pair(fixture+16,2u),"Month initial read order differs");
            equivalent();
            require(b->date_month_calls==1 && !b->date_difference_calls && !b->date_ordering_calls && !b->date_before_calls &&
                    b->calendar_calls==std::array<std::uint64_t,2>{},"Month dispatch/counters differ");
            const auto result=std::bit_cast<std::int32_t>(std::uint32_t(a->r<4>(12)));
            require(result==cm::calendar::month_index(std::int16_t(day),thresholds),"Month semantic result differs");
            if(!mutated && year>=1 && year<=2100 && flag==unsigned(cm::calendar::is_leap(std::uint16_t(year))) &&
               day>=0 && day<int(cm::calendar::year_length(std::uint16_t(year)))) {
                using namespace std::chrono;
                const year_month_day date{sys_days(std::chrono::year(int(year))/January/1)+days(day)};
                require(result==int(unsigned(date.month()))-1,"Month independent Gregorian oracle differs"); ++oracle;
            }
            // Pointer and contents are mutable mapped data, restored privately.
            for(auto* s:{a.get(),b.get()}) s->memory.write<4>(slot,table);
            ++count;
        };
        for(unsigned year:{0u,1u,4u,1900u,1999u,2000u,2001u,2004u,2100u,32767u,65535u})
            for(unsigned flag:{0u,1u,128u,255u}) {
                for(int day:{-32768,-2,-1,0,1,58,59,60,364,365,366,367,32767}) check(year,day,flag);
                for(auto threshold:(flag?leap:common))
                    for(int delta:{0,1}) check(year,int(threshold)+delta,flag);
            }
        for(unsigned flag:{0u,1u,128u,255u})
            for(int day:{-32768,-1,0,59,100,299,32767}) {check(2000,day,flag,true); check(1900,day,flag,false,true);}
        for(const auto& thresholds:{common,leap})
            for(unsigned bits=0;bits<=65535;++bits) {
                const auto day=cm::calendar::narrow_day(bits); int expected=-1;
                for(int i=0;i<12;++i) if(day>thresholds[i]) expected=i;
                require(cm::calendar::month_index(day,thresholds)==expected,"Month pure signed domain differs");
            }
        for(unsigned day:{0u,59u,334u,65535u}) {
            std::array<std::uint32_t,2> results{}; unsigned index=0;
            for(auto* s:{a.get(),b.get()}) {
                initialize(*s,2000,int(cm::calendar::narrow_day(day)),1,count);
                s->memory.write<4>(fixture,entry); s->memory.write<4>(fixture+4,s->r<4>(8));
                const auto registers=s->registers; const auto unique=s->unique;
                results[index++]=call_original(*s,fixture,std::array<std::uint32_t,1>{fixture+16});
                require(s->registers==registers && s->unique==unique,"Month callback restoration differs");
            }
            require(results[0]==results[1] && b->date_month_calls==1,"Month callback return differs"); same_memory(*a,*b);
        }
        // Actual caller instructions with controlled inputs: synthetic evidence.
        for(auto pc:{0x1002a12cu,0x1002a138u,0x100746e0u,0x10089e80u,0x1000a494u}) {
            for(auto* s:{a.get(),b.get()}) {
                initialize(*s,2000,59,1,count); s->pc=pc; s->wr<4>(4132,entry);
                require(dispatch_page(*s) && s->pc==entry,"Month caller missed dispatch");
                require(dispatch_page(*s),"Month target unavailable");
            }
            require(b->pc==(pc==0x1000a494u?return_sentinel:pc+4) && b->r<4>(12)==1 && b->date_month_calls==1,"Month caller result differs"); equivalent();
        }
        auto pattern=(std::filesystem::temp_directory_path()/"cm-month-checks-XXXXXX").string();
        require(mkdtemp(pattern.data())!=nullptr,"Cannot create private month stream directory");
        const auto stream=std::filesystem::path(pattern)/"original-month.gz";
        initialize(*a,2000,59,1,count); open_month_audit(stream,true); run(*a); close_month_audit();
        initialize(*b,2000,59,1,count); open_month_audit(stream,false); run(*b); close_month_audit(true); equivalent();
        initialize(*b,2000,59,1,count); open_month_audit(stream,false,true);
        bool rejected=false;
        try {run(*b);} catch(const std::runtime_error& error) {
            rejected=std::string(error.what()).find("month edge 0 at 0x10718360 field=month return byte=48")!=std::string::npos;
        }
        close_month_audit(); require(rejected,"Month first-return negative was accepted");
        initialize(*b,2000,59,1,count); run(*b);
        require(b->r<4>(12)==1,"Private month perturbation leaked into normal execution");
        open_month_audit(stream,false); rejected=false;
        try {close_month_audit(true);} catch(const std::runtime_error&) {rejected=true;}
        require(rejected,"Unread month edges were accepted");
        std::filesystem::resize_file(stream,4); rejected=false;
        try {open_month_audit(stream,false);} catch(const std::runtime_error&) {rejected=true;}
        require(rejected,"Truncated month header was accepted");
        std::filesystem::remove_all(pattern);
        std::cout<<count<<" direct original/modern month cases; "<<oracle<<" Gregorian oracle cases; 131072 pure signed-day checks; full registers/temporaries/PC/mapped memory/allocator, ordered reads, no writes/imports/RNG; mutable tables/TOC, synthetic callers, indirect dispatch and callbacks\n";
        return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
