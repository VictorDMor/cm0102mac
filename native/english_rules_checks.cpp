// Rule changes are checked against their explicit football contract. Shared
// original draw, home-selection and division-transfer bodies execute here;
// only allocation, file bytes and unrelated news/finance boundaries are modeled.
#include "english_competition_rules.hpp"
#include "eng_first.hpp"
#include "eng_prm.hpp"
#include "cup_stage.hpp"
#include "semantic_io_checks.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <iostream>
#include <set>

using namespace cm::aot;
using guest::Cpu;
using cm::semantic_io_checks::require;
namespace {
constexpr std::uint32_t data=0x60000000, first=data+0x1000, premier=data+0x1200,
    clubs=data+0x10000, definitions=data+0x3000, stack=0x71010000, stop=0xfffffff0;
std::uint32_t allocation=data+0x30000;
std::vector<std::uint8_t> file;
std::size_t position=0;
bool classify=false;
bool probe(State& s) {
    Cpu c(s); const auto entry=s.pc;
    if(entry==0x10775130 || entry==0x10773520 || entry==0x10775200) {
        const auto bytes=c.r(3)*(entry==0x10775200?c.r(4):1);
        require(bytes<0x10000,"unexpected allocation");
        const auto at=allocation; allocation+=(bytes+31)&~15u;
        require(allocation<data+0x180000,"fixture arena exhausted");
        for(unsigned i=0;i<bytes;++i)c.put8(at+i,0);
        c.set(3,at);
    } else if(entry==0x10775170 || entry==0x107735a0 || entry==0x1047ef50 ||
              entry==0x1007fe90 || entry==0x1007fd00 || entry==0x1007ffb0 ||
              entry==0x10381b70 || entry==0x103822e0 || entry==0x10384500 || entry==0x10382e50) {
        // Start-stage display/news and club movement notifications are external.
    } else if(entry==0x104ae570 || entry==0x104646d0) {
        c.set(3,0); // No foreign-player rule or continental qualification.
    } else if(classify && entry==0x1049eb30) {
        c.set(3,1); // Fixture standings already contain the final sorted order.
    } else if(classify && (entry==0x104a44f0 || entry==0x10451f80)) {
        c.set(3,0); // Classification news and Premier fixture-history display.
    } else if(entry==0x10775fe0 || entry==0x10776360) {
        const auto bytes=c.r(4)*c.r(5), at=c.r(3);
        if(entry==0x10776360) {
            const auto block=s.memory.copy_out(at,bytes); file.insert(file.end(),block.begin(),block.end());
        } else {
            require(position+bytes<=file.size(),"short serialized stage");
            s.memory.copy_in(at,std::vector<std::uint8_t>(file.begin()+position,file.begin()+position+bytes));
            position+=bytes;
        }
        c.set(3,c.r(5));
    } else if(entry==0x10459b50) c.set(3,data+0x8000);
    else return false;
    s.pc=c.lr(); return true;
}
void invoke(Cpu& c,std::uint32_t entry,std::initializer_list<std::uint32_t> args) {
    unsigned reg=3;for(auto a:args)c.set(reg++,a);
    c.set_lr(stop);c.s.pc=entry;c.s.budget=c.s.executed+2000000;
    try { run_until(c.s,stop); }
    catch(const std::exception& e) {
        throw std::runtime_error("entry " + hex_address(entry) + " at " + hex_address(c.s.pc) + ": " + e.what());
    }
    require(c.sp()==stack,"guest stack balance");
}
void setup(Cpu& c) {
    c.s.memory.map(data,0x200000);c.s.memory.map(0x71000000,0x20000);
    c.set(1,stack);c.set(2,0x108bba90);c.set_lr(stop);
    const auto variable=[&](int slot,unsigned offset,std::uint32_t value) {
        c.put32(c.r(2)+std::uint32_t(slot),data+offset);c.put32(data+offset,value);
    };
    variable(-0x67a0,0x100,0);variable(-0x67a4,0x104,1);variable(-0x67a8,0x108,2);
    variable(-0x7be8,0x10c,clubs);variable(-0x7bbc,0x110,definitions);
    variable(-0x7b5c,0x114,data+0x2000);variable(-0x7b7c,0x118,data+0x2200);
    variable(-0x7a98,0x11c,data+0x2300);variable(-0x7b48,0x120,data+0x2400);
    c.put32(data+0x2000,premier);c.put32(data+0x2004,first);
    for(unsigned i=0;i<3;++i)c.put32(definitions+i*0x6c,i);
    for(auto league:{first,premier}) {
        c.put32(league,c.toc(league==first?-0x6024:-0x6020));
        c.put32(league+4,definitions+(league==first?1:0)*0x6c);
        c.put16(league+0x40,2001);c.put8(league+0x45,1);
        c.put16(league+0x3e,league==first?24:20);
        c.put32(league+0xb0,data+(league==first?0x4000:0x6000));
        c.put32(league+0xc,data+0x2800);c.put32(league+0x30,0xffffffff);
        c.put32(league+0x2c,1);
        c.put16(league+0xdc,3);c.put16(league+0xa4,league==first?46:38);
        c.put8(league+0x44,0xff);
    }
    for(unsigned i=0;i<44;++i) {
        const auto club=clubs+i*0x248; c.put32(club,i);
        const auto division=i<24?first:premier;
        c.put32(club+0x30,c.u32(division+4));
        c.put32(c.u32(division+0xb0)+(i<24?i:i-24)*0x44,club);
        const auto row=c.u32(division+0xb0)+(i<24?i:i-24)*0x44;
        c.put8(row+0x40,0xff);
        c.put8(row+0x42,(i<24?i:i-24)+1);c.put8(row+0x43,(i<24?i:i-24)+1);
    }
    c.put32(c.toc(-0x7b90),123);c.put32(c.toc(-0x7b84),c.toc(-0x7b8c));
    c.put32(c.toc(-0x7b88),c.toc(-0x7b8c)+100);
}
std::uint32_t match(Cpu& c,std::uint32_t stage,unsigned round,unsigned tie) {
    return c.u32(c.u32(stage+0xac)+round*4)+tie*0x36;
}
unsigned place(Cpu& c,std::uint32_t stage,int entrant) {
    require(entrant>=0 && entrant<8,"entrant index bounds");
    return (c.u32(c.u32(stage+0xa4)+unsigned(entrant)*8)-clubs)/0x248+1;
}
void check_draw(Cpu& c,std::uint32_t stage) {
    require(c.u16(stage+0x3c)==3 && c.u16(stage+0x3e)==8,"eight clubs, three rounds");
    const auto rounds=c.u32(stage+0xa0);
    for(unsigned r=0;r<3;++r) {
        const auto row=rounds+r*0x78;
        require(c.u16(row+0x1c)==(4u>>r) && c.u8(row+0x23)==1,"seven single-leg games");
        require((c.u16(row+0xe)&7)==3 && c.u16(row+0x10)==0 && c.u16(row+0x12)==0,"ET/pens without replays");
        require(c.u16(row+0xc)==(r<2?8:0) && c.u32(row+0x14)==(r<2?0:4),"home and neutral venue rules");
        if(r)require(c.u16(row+4)>c.u16(row-0x78+4),"round dates ordered");
    }
    invoke(c,0x10481ad0,{stage,0,0xffffffff});
    constexpr unsigned expected[][2]={{4,11},{7,8},{5,10},{6,9}};
    for(unsigned i=0;i<4;++i) {
        const auto row=match(c,stage,0,i);
        require(place(c,stage,c.i16(row))==expected[i][0] && place(c,stage,c.i16(row+2))==expected[i][1],"seeded quarterfinal pairing");
        require(c.u16(row+6)==i/2 && c.u16(row+0x12)==1,"quarterfinal semifinal link");
    }
    // Every possible quarterfinal outcome: stronger club hosts each semifinal,
    // including all upsets. Run the original home sorter on actual draw records.
    for(unsigned outcomes=0;outcomes<16;++outcomes) {
        for(unsigned i=0;i<4;++i) {
            const auto q=match(c,stage,0,i), sf=match(c,stage,1,i/2);
            const auto winner=c.u16(q+((outcomes>>i)&1?2:0));
            c.put16(sf+(i%2)*2,winner);c.put16(sf+0xa+(i%2)*2,i);
            c.put16(sf + 0xe + (i%2)*2,0);
        }
        invoke(c,0x10481ad0,{stage,1,0xffffffff});
        for(unsigned i=0;i<2;++i) {
            const auto sf=match(c,stage,1,i);
            require(place(c,stage,c.i16(sf))<place(c,stage,c.i16(sf+2)),"semifinal host after upset");
            require(c.u16(sf+0xa)/2==i && c.u16(sf+0xc)/2==i,"inbound semifinal links");
        }
        for(unsigned finalists=0;finalists<4;++finalists) {
            const auto final=match(c,stage,2,0);
            for(unsigned i=0;i<2;++i)
                c.put16(final+i*2,c.u16(match(c,stage,1,i)+((finalists>>i)&1?2:0)));
            const auto home=c.u16(final),away=c.u16(final+2);
            invoke(c,0x10481ad0,{stage,2,0xffffffff});
            require(c.u16(final)==home && c.u16(final+2)==away,"neutral final does not award seed home advantage");
        }
    }
}
}
int main(int argc,char** argv) {
    if(argc!=2)return 2;
    try {
        auto state=std::make_unique<State>();load_program(*state,argv[1]);Cpu c(*state);
        dispatch_probe=probe;setup(c);
        invoke(c,0x10528e50,{first});invoke(c,0x1052bd60,{premier});
        require(c.u8(first+0xc0)==3 && c.u8(first+0xc1)==8 && c.u8(premier+0xc3)==4,"new season rules");
        invoke(c,0x10529280,{first});const auto stage=c.u32(c.u32(first+0xc));
        check_draw(c,stage);
        // Save and load the actual stage serializer with eight seeded entrants.
        const auto expected_rounds=state->memory.copy_out(c.u32(stage+0xa0),3*0x78);
        const auto expected_entrants=state->memory.copy_out(c.u32(stage+0xa4),8*8);
        std::vector<std::vector<std::uint8_t>> expected_matches;
        for(unsigned r=0;r<3;++r)expected_matches.push_back(state->memory.copy_out(match(c,stage,r,0),(4>>r)*0x36));
        file.clear();position=0;invoke(c,0x104bd550,{stage,7});
        const auto loaded=data+0x20000;
        c.put16(data+0x21000,4);c.put16(data+0x21002,2);c.put16(data+0x21004,1);
        invoke(c,0x104bc750,{loaded,8,3,data+0x21000});
        invoke(c,0x104bcb20,{loaded,7});
        require(position==file.size(),"complete serialized stage read");
        require(state->memory.copy_out(c.u32(loaded+0xa0),3*0x78)==expected_rounds,"round rules survive reload");
        require(state->memory.copy_out(c.u32(loaded+0xa4),8*8)==expected_entrants,"rank seeds survive reload");
        for(unsigned r=0;r<3;++r)
            require(state->memory.copy_out(match(c,loaded,r,0),(4>>r)*0x36)==expected_matches[r],"bracket links survive reload");
        check_draw(c,loaded);
        // A return leg accidentally enabled must be rejected by this contract.
        const auto round=c.u32(loaded+0xa0);c.put8(round+0x23,2);
        bool rejected=false;
        try {check_draw(c,loaded);}catch(const std::runtime_error& e) {
            rejected=std::string(e.what())=="seven single-leg games";
        }
        c.put8(round+0x23,1);require(rejected,"single-leg negative control");
        // Real league classification consumes the configured 3/8 and 4 fields.
        classify=true;
        invoke(c,0x104a1c30,{first});invoke(c,0x104a1c30,{premier});
        classify=false;
        for(unsigned i=0;i<24;++i) {
            const auto expected=i==0?5:i<3?0:i<11?1:i>=21?3:255;
            require(c.u8(c.u32(first+0xb0)+i*0x44+0x40)==expected,"First promotion/playoff/relegation boundary");
        }
        for(unsigned i=1;i<20;++i)
            require(c.u8(c.u32(premier+0xb0)+i*0x44+0x40)==(i>=16?3:255),"Premier relegation boundary");
        // The lowest seed wins: only its playoff event adds the fourth promoted club.
        invoke(c,0x10529470,{first,clubs+10*0x248,1,0,data+0x9100});
        require(c.u8(data+0x9100)==1 && c.u8(c.u32(first+0xb0)+10*0x44+0x40)==0,"playoff winner promoted");
        // Fixture display ranges move to 4..11 and retain sentinel bytes.
        for(unsigned rank=0;rank<256;++rank) {
            invoke(c,0x105296d0,{first,7,0,rank,rank,rank});
            const auto expected=rank>=1 && rank<=8?rank+3:rank;
            for(unsigned offset:{0x76u,0x77u,0x78u})require(c.u8(data+0x8000+offset)==expected,"playoff place remapping");
            invoke(c,0x105296d0,{first,7,0xffffffff,rank,rank,rank});
            require(c.u8(data+0x8076)==rank && c.u8(data+0x8077)==(rank>=4 && rank<=11?4:rank) &&
                    c.u8(data+0x8078)==(rank>=4 && rank<=11?11:rank),"league playoff range remapping");
        }
        // The unchanged season-transfer body must exchange all four clubs.
        invoke(c,0x104a9210,{premier,premier,first,0,1,4,4});
        unsigned top=0,second=0,promoted=0,relegated=0;
        for(unsigned i=0;i<44;++i) {
            const auto division=c.u32(clubs+i*0x248+0x30);
            top+=division==c.u32(premier+4);second+=division==c.u32(first+4);
            promoted+=i<24 && division==c.u32(premier+4);
            relegated+=i>=24 && division==c.u32(first+4);
        }
        require(promoted==4 && relegated==4 && top==20 && second==24,"balanced four-club division exchange");
        // Loaded legacy seasons still generate their four-club/two-round format.
        c.put8(first+0xc0,2);c.put8(first+0xc1,4);
        c.put8(premier+0xc3,3);
        invoke(c,0x10526c00,{first,0,data+0x9000,data+0x9002,0});
        require(c.u16(data+0x9000)==2 && c.u8(c.r(3)+0x23)==2,"legacy season retained");
        require(c.u8(first+0xc0)==2 && c.u8(first+0xc1)==4 && c.u8(premier+0xc3)==3,"legacy fields not migrated by calendar");
        c.put16(first+0x40,2002);c.put16(premier+0x40,2002);
        invoke(c,0x1052bd60,{premier});invoke(c,0x10528e50,{first});
        require(c.u8(first+0xc0)==3 && c.u8(first+0xc1)==8 && c.u8(premier+0xc3)==4,"next-season configuration adopts preset");
        std::cout<<"English rules: calendar, draw, 16 quarterfinal/64 final combinations, stage persistence, classification, four-club exchange, legacy season and negative control passed\n";
        dispatch_probe=nullptr;return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
