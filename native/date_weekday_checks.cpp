#include "calendar.hpp"
#include "calendar_adapter.hpp"
#include "callbacks.hpp"
#include <chrono>
#include <iostream>
#include <fstream>
#include <unistd.h>

using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture = 0x30000000, entry = 0x10715b20;
void require(bool test, const std::string& message) { if (!test) throw std::runtime_error(message); }
struct NoWrites final : MemoryObserver {
    void mapped(std::uint32_t, std::uint64_t) override { throw std::runtime_error("Weekday mapped memory"); }
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override { throw std::runtime_error("Weekday wrote memory"); }
    void cleared(std::uint32_t, std::uint64_t) override { throw std::runtime_error("Weekday cleared memory"); }
};
void same_memory(const State& a, const State& b) {
    require(a.memory.mapped_ranges() == b.memory.mapped_ranges(), "Weekday mapped ranges differ");
    for (const auto& [start,size] : a.memory.mapped_ranges())
        for (std::uint64_t offset=0; offset<size; offset+=65536) {
            const auto length=std::size_t(std::min<std::uint64_t>(size-offset,65536));
            require(a.memory.copy_out(start+std::uint32_t(offset),length)==
                    b.memory.copy_out(start+std::uint32_t(offset),length), "Weekday entire memory differs");
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
    s.wr<1>(2304,index%2?0xf9:0xa6); s.wr<1>(1024,index%4); s.wr<1>(1027,(index/2)%2);
    s.wr<4>(8,index%2?0xdeadbeef:0x108bba90); s.wr<4>(2440,0xbadc0ffe);
    s.pc=entry; s.executed=0; s.budget=160; s.native_calls=0;
    s.calendar_calls={}; s.date_difference_calls=0; s.date_ordering_calls=0; s.date_before_calls=0; s.date_day_of_month_calls=0; s.date_weekday_calls=0;
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
                throw std::runtime_error("Weekday direct ABI mismatch case="+std::to_string(count));
            }
            require(a->unique==b->unique && a->pc==b->pc,"Weekday temporaries/PC differ"); same_memory(*a,*b);
            require(a->allocated==b->allocated && a->available==b->available && a->heap_end==b->heap_end &&
                    a->memory_error==b->memory_error && !a->native_calls && !b->native_calls,"Weekday allocator/import effects differ");
        };
        auto check=[&](unsigned year,int day,unsigned flag) {
            std::vector<std::pair<std::uint32_t,unsigned>> ar,br;
            NoWrites observer;
            for(auto* s:{a.get(),b.get()}) {
                initialize(*s,year,day,flag,count);
                s->memory.read_trace=s==a.get()?&ar:&br; s->memory.observer=&observer;
                run(*s); s->memory.read_trace=nullptr; s->memory.observer=nullptr;
            }
            const std::vector<std::pair<std::uint32_t,unsigned>> expected{{fixture+18,2},{fixture+20,1},{fixture+16,2}};
            require(ar==expected && br==expected,"Weekday ordered read footprint differs");
            equivalent();
            const auto result=signed_value<4>(b->r<4>(12));
            require(result==cm::calendar::weekday({std::int16_t(day),std::uint16_t(year)},std::uint8_t(flag)),"Weekday semantic return differs");
            require(b->date_weekday_calls==1 && !a->date_weekday_calls && !b->date_month_calls &&
                    !b->date_day_of_month_calls && !b->date_before_calls && !b->date_ordering_calls &&
                    !b->date_difference_calls && b->calendar_calls==std::array<std::uint64_t,2>{},"Weekday own coverage differs");
            // Independent civil-date oracle is confined to the proven epoch domain.
            if(year>=1601 && year<=32767 && day>=0 && day<int(cm::calendar::year_length(year)) && flag==cm::calendar::is_leap(year)) {
                using namespace std::chrono;
                const auto date=sys_days{std::chrono::year{int(year)}/January/1}+days{day};
                require(result==int(std::chrono::weekday{date}.iso_encoding())-1,"Gregorian weekday oracle differs"); ++oracle;
            }
            ++count;
        };
        for(unsigned year:{0u,1u,4u,1599u,1600u,1601u,1604u,1699u,1700u,1899u,1900u,1999u,2000u,2001u,2002u,2099u,2100u,32767u,32768u,65535u}) {
            std::vector<int> days{-32768,-32767,-8,-7,-6,-1,0,1,2,3,4,5,6,7,8,32766,32767};
            const std::array<int,12> lengths{31,cm::calendar::is_leap(year)?29:28,31,30,31,30,31,31,30,31,30,31};
            int boundary=0;
            for(int length:lengths) {boundary+=length; for(int delta:{-1,0,1}) days.push_back(boundary+delta);}
            for(int day:days) for(unsigned flag:{0u,1u,128u,255u}) check(year,day,flag);
        }
        // All signed days at three epochs; compare an independent truncation
        // reference and characterize negative remainders without normalization.
        unsigned pure=0;
        for(unsigned year:{0u,1600u,2000u}) for(unsigned flag:{0u,1u,128u,255u})
            for(unsigned bits=0;bits<=65535;++bits) {
                const auto day=cm::calendar::narrow_day(bits);
                const std::int64_t n=int(year)-1600;
                const auto total=n*365+n/4-n/100+n/400+day-(flag!=0);
                const auto rem=total-(total/7)*7;
                const auto expected=rem>0?rem-1:rem==0?6:rem;
                require(cm::calendar::weekday({day,std::uint16_t(year)},flag)==expected,"Pure signed weekday domain differs"); ++pure;
            }
        // Gregorian Jan 1 and leap/month boundaries across the complete
        // unsigned year range above 1600, using an independent civil day count.
        unsigned wide_oracle=0;
        for(unsigned year=1601;year<=65535;++year) for(int day:{0,58,59,60,364}) {
            const auto previous=std::int64_t(year)-1;
            const auto ordinal=365*previous+previous/4-previous/100+previous/400+day;
            require(cm::calendar::weekday({std::int16_t(day),std::uint16_t(year)},cm::calendar::is_leap(year))==ordinal%7,"Wide Gregorian domain differs"); ++wide_oracle;
        }
        require(cm::calendar::weekday({0,1600},1)==-1 && cm::calendar::weekday({1,1600},1)==6 &&
                cm::calendar::weekday({0,1599},0)==-1,"Historical pre-epoch behavior was normalized");
        // Stack, TOC, tables and all other program data can remain unmapped.
        for(auto day:{-32768,-1,0,1,7,59,32767}) for(unsigned flag:{0u,1u,128u,255u}) {
            auto sa=std::make_unique<State>(), sb=std::make_unique<State>(); sa->calendar_original=true;
            std::vector<std::pair<std::uint32_t,unsigned>> ar,br; NoWrites observer;
            for(auto* s:{sa.get(),sb.get()}) {
                s->registers.fill(0xaf); s->unique.fill(0x82);
                s->memory.map(fixture,2); s->memory.map(fixture+2,2); s->memory.map(fixture+4,1);
                s->memory.write<2>(fixture,day); s->memory.write<2>(fixture+2,1600); s->memory.write<1>(fixture+4,flag);
                s->wr<4>(12,fixture); s->wr<4>(4128,return_sentinel); s->pc=entry; s->budget=100;
                s->memory.observer=&observer; s->memory.read_trace=s==sa.get()?&ar:&br;
                run(*s); s->memory.observer=nullptr; s->memory.read_trace=nullptr;
            }
            require(ar==br && ar==std::vector<std::pair<std::uint32_t,unsigned>>{{fixture+2,2},{fixture+4,1},{fixture,2}},"Sparse weekday reads differ");
            require(sa->registers==sb->registers && sa->unique==sb->unique && sa->pc==sb->pc && sb->date_weekday_calls==1,"Sparse weekday ABI differs");
            same_memory(*sa,*sb);
        }
        for(unsigned day:{0u,59u,243u,244u,65535u}) {
            std::array<std::uint32_t,2> results{}; unsigned index=0;
            for(auto* s:{a.get(),b.get()}) {
                initialize(*s,2002,int(cm::calendar::narrow_day(day)),0,count);
                s->memory.write<4>(fixture,entry); s->memory.write<4>(fixture+4,0x108bba90);
                const auto registers=s->registers; const auto unique=s->unique;
                results[index++]=call_original(*s,fixture,std::array<std::uint32_t,1>{fixture+16});
                require(s->registers==registers && s->unique==unique,"Weekday callback restoration differs");
            }
            require(results[0]==results[1] && b->date_weekday_calls==1,"Weekday callback result differs"); same_memory(*a,*b);
        }
        // Actual instructions, with controlled inputs: synthetic caller evidence.
        for(auto pc:{0x1006655cu,0x10067fd4u,0x101197e4u,0x104946c4u,0x10715a54u,0x1000a494u}) {
            for(auto* s:{a.get(),b.get()}) {
                initialize(*s,2002,243,0,count); s->pc=pc; s->wr<4>(4132,entry);
                require(dispatch_page(*s),"Weekday caller unavailable");
                if(pc!=0x10715a54u) {
                    require(s->pc==entry,"Weekday caller missed target");
                    require(dispatch_page(*s),"Weekday target unavailable");
                }
            }
            require(b->pc==(pc==0x1000a494u?return_sentinel:pc+4) && b->r<4>(12)==6 && b->date_weekday_calls==1,"Weekday caller result differs"); equivalent();
        }
        auto pattern=(std::filesystem::temp_directory_path()/"cm-weekday-checks-XXXXXX").string();
        require(mkdtemp(pattern.data())!=nullptr,"Cannot create private weekday stream directory");
        const auto stream=std::filesystem::path(pattern)/"original-weekday.gz";
        initialize(*a,2000,59,1,count); open_weekday_audit(stream,true); run(*a); close_weekday_audit();
        initialize(*b,2000,59,1,count); open_weekday_audit(stream,false); run(*b); close_weekday_audit(true); equivalent();
        initialize(*b,2000,59,1,count); open_weekday_audit(stream,false,true);
        bool rejected=false;
        try {run(*b);} catch(const std::runtime_error& error) {
            rejected=std::string(error.what()).find("weekday edge 0 at 0x10715b20 field=weekday return byte=20")!=std::string::npos;
        }
        close_weekday_audit(); require(rejected,"Weekday first-return negative was accepted");
        initialize(*b,2000,59,1,count); run(*b);
        require(b->r<4>(12)==1,"Private weekday perturbation leaked into normal execution");
        open_weekday_audit(stream,false); rejected=false;
        try {close_weekday_audit(true);} catch(const std::runtime_error&) {rejected=true;}
        require(rejected,"Unread weekday edges were accepted");
        const auto corrupted=std::filesystem::path(pattern)/"corrupted.gz";
        std::filesystem::copy_file(stream,corrupted);
        {
            std::fstream file(corrupted,std::ios::in|std::ios::out|std::ios::binary);
            file.seekg(-8,std::ios::end); char byte{}; file.read(&byte,1); byte^=1;
            file.seekp(-8,std::ios::end); file.write(&byte,1);
        }
        rejected=false;
        try {initialize(*b,2000,59,1,count); open_weekday_audit(corrupted,false); run(*b); close_weekday_audit(true);}
        catch(const std::runtime_error&) {rejected=true;}
        try {close_weekday_audit();} catch(const std::runtime_error&) {rejected=true;}
        require(rejected,"Weekday checksum damage was accepted");
        std::filesystem::resize_file(stream,std::filesystem::file_size(stream)/2); rejected=false;
        try {initialize(*b,2000,59,1,count); open_weekday_audit(stream,false); run(*b); close_weekday_audit(true);}
        catch(const std::runtime_error&) {rejected=true;}
        try {close_weekday_audit();} catch(const std::runtime_error&) {rejected=true;}
        require(rejected,"Weekday truncated body was accepted");
        std::filesystem::resize_file(stream,4); rejected=false;
        try {open_weekday_audit(stream,false);} catch(const std::runtime_error&) {rejected=true;}
        require(rejected,"Truncated weekday header was accepted");
        initialize(*b,2000,59,1,count); bool invalid_record=false;
        open_weekday_audit(std::filesystem::path(pattern)/"candidate.gz",true);
        try {run(*b);} catch(const std::runtime_error&) {invalid_record=true;}
        close_weekday_audit(); require(invalid_record,"Candidate weekday oracle recording was accepted");
        std::filesystem::remove_all(pattern);
        std::cout<<count<<" direct original/modern weekday cases; "<<oracle<<" chrono and "<<wide_oracle<<" wide Gregorian oracle cases; "<<pure<<" pure signed-day checks; ordered year/flag/day reads; full registers/temporaries/PC/mapped memory/allocator; sparse maps, TOC independence, synthetic same-page/cross-page/indirect callers, callbacks, stream EOF/checksum/truncation and first-return negative\n";
        return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
