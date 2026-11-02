#include "cup_stage_detail.hpp"

namespace cm::aot::cup_stage {
using namespace detail;
namespace {
// Match links, scores and discipline slots. Bytes +4/+5 and +0x35 are untouched.
void clear_match(Cpu& c,const Stage& stage,std::uint32_t round,std::uint32_t index) {
    const auto offset=index*0x36;
    const auto half=[&](unsigned field,unsigned value){c.put16(stage.match(round,index)+field,value);};
    const auto byte=[&](unsigned field,unsigned value){c.put8(stage.match(round,index)+field,value);};
    half(0,0xffff);half(2,0xffff);half(6,0xffff);half(8,0);
    half(10,0xffff);half(12,0xffff);byte(0x32,0);half(14,0xffff);half(16,0xffff);half(18,0xffff);byte(0x34,0);
    for(unsigned field:{0x14u,0x15u,0x1cu,0x1du,0x16u,0x17u,0x18u,0x19u,0x1au,0x1bu,
                        0x1eu,0x1fu,0x26u,0x27u,0x20u,0x21u,0x22u,0x23u,0x24u,0x25u,
                        0x28u,0x29u,0x30u,0x31u,0x2au,0x2bu,0x2cu,0x2du,0x2eu,0x2fu})byte(field,0xff);
    byte(0x33,0);
    keep(c,{{14,0},{15,offset+0x26},{16,offset+0x27},{17,offset+0x20},{18,offset+0x21},
            {19,offset+0x22},{20,offset+0x23},{21,offset+0x24},{22,offset+0x25},
            {23,offset+0x28},{24,offset+0x29},{25,offset+0x30},{26,offset+0x31},{27,offset+0x2a}});
    leave(c,{{3,c.u32(stage.matches()+round*4)},{4,offset+0x36},{6,index+1},
             {7,offset+0x33},{8,offset+0x2f},{9,offset+0x2e},{10,offset+0x2d}});
}
std::uint32_t next_match(std::int16_t index,std::int16_t count) {
    return index < std::int32_t(count)-1 ? extended(std::int16_t(std::uint32_t(std::int32_t(index))+1)) : 0;
}
}
std::uint32_t build_draw(Cpu& c,std::uint32_t address) {
    Frame f(c,0x104bd8c0,0x170,{.first_gpr=13});Stage stage{c,address};Program p{c};
    keep(c,{{28,address},{29,c.toc(slot::doubles)},{30,c.toc(slot::rng_seed)},{31,c.toc(slot::rng_cursor)}});
    auto allocation=p.call(0x10775130,0x104bd8ec,{std::uint32_t(std::int32_t(stage.round_count()))*4});
    compare_logical(c,allocation);c.put32(address+0xac,allocation);
    if(!allocation){reloading_assertion(c,f,0x104bd90c,0xe0,0x58,0x201);return result(c,0xffffffff);}
    keep(c,{{13,0},{14,0},{15,0}});
    for(std::uint32_t round=0;compare(c,round,stage.round_count())<0;++round) {
        const auto count=c.i16(stage.round(round)+0x1c);
        allocation=p.call(0x10775130,0x104bd980,{std::uint32_t(std::int32_t(count))*0x36});
        c.set(4,stage.matches());c.put32(stage.matches()+round*4,allocation);c.set(3,stage.matches());
        if(compare_logical(c,c.u32(stage.matches()+round*4))==0) {
            reloading_assertion(c,f,0x104bd9ac,0xc0,0x50,0x20b);return result(c,0xffffffff);
        }
        keep(c,{{13,(round+1)*0x78},{14,(round+1)*4},{15,round+1}});
    }
    if(compare_logical(c,stage.entrants())!=0) {
        leave(c,{{4,0},{5,0},{6,0}});
        for(std::uint32_t index=0;compare(c,index,stage.entrant_count())<0;++index) {
            c.set(3,stage.entrants());c.put8(stage.entrants()+index*8+5,0);
            leave(c,{{5,(index+1)*8},{6,index+1}});
        }
    }
    if(compare(c,c.i8(address+0x45))==0)return result(c,1);
    c.put8(address+0xa8,0);c.put8(address+0xa9,0);c.put32(f.at(0x118),0);
    keep(c,{{13,0}});c.set(5,0);
    for(std::uint32_t round=0;compare(c,c.u32(f.at(0x118)),stage.round_count())<0;++round) {
        leave(c,{{4,0},{6,0}});
        for(std::uint32_t index=0;;++index) {
            c.set(3,stage.rounds());
            if(compare(c,index,c.i16(stage.round(round)+0x1c))>=0)break;
            clear_match(c,stage,round,index);
        }
        c.set(3,c.u32(f.at(0x118))+1);c.put32(f.at(0x118),c.r(3));
        c.set(5,(round+1)*4);keep(c,{{13,(round+1)*0x78}});
    }
    auto active_rounds=std::int32_t(stage.round_count());c.set(4,active_rounds);
    c.set(3,stage.rounds()+std::uint32_t(active_rounds)*0x78);
    if(compare(c,c.i16(c.r(3)-0x70),0x8c)==0) {--active_rounds;c.set(4,extended(std::int16_t(active_rounds)));}
    keep(c,{{20,0},{18,extended(std::int16_t(active_rounds))},{16,0},{17,0},{13,std::uint32_t(active_rounds)-1}});
    const Stream rng{c,c.r(31),c.toc(slot::rng_end),c.toc(slot::rng_start),c.r(30)};
    const auto draw_index=[&](std::int16_t bound,std::uint32_t refill_back) {
        keep(c,{{14,extended(bound)}});
        std::uint32_t selected=0;
        if(compare(c,bound)!=0) {
            // The bound is loaded with lha, hence the original inlined wide
            // floating-point branch and its call to random_below are unreachable.
            c.set(4,c.u32(rng.cursor));c.set(3,rng.end);rng.advance(refill_back);
            c.set(3,0x10000);compare(c,bound,65535);
            c.set(3,0xffff0000);compare(c,bound,-65535);
            c.set(3,c.u32(rng.cursor));compare(c,bound);
            const auto sum=rng.sample();c.set(3,sum);
            const auto remainder=std::int32_t(std::int64_t(std::int32_t(sum))%bound);
            selected=bound>0?std::uint32_t(remainder):0u-std::uint32_t(remainder);
        }
        keep(c,{{21,selected}});c.set(3,extended(std::int16_t(selected)));return std::int16_t(selected);
    };
    for(std::uint32_t round=0;compare(c,round,active_rounds-1)<0;++round) {
        keep(c,{{19,0},{15,0}});
        for(std::uint32_t match=0;;++match) {
            c.set(3,stage.round(round));
            if(compare(c,match,c.i16(stage.round(round)+0x1c))>=0)break;
            std::int16_t chosen;
            if(compare(c,std::uint32_t(std::int32_t(c.i16(stage.round(round)+0x86)))&0x200)!=0) {
                chosen=std::int16_t(std::int32_t(match)/2);c.set(3,extended(chosen));
            } else chosen=draw_index(c.i16(stage.round(round)+0x94),0x104bde1c);
            const auto try_link=[&](unsigned ahead,std::int16_t first) {
                auto chosen=first;
                do {
                    const auto destination=stage.match(round+ahead,std::uint32_t(std::int32_t(chosen)));
                    const auto offset=std::uint32_t(std::int32_t(chosen))*0x36;
                    const auto base=c.u32(stage.matches()+(round+ahead)*4);
                    if(ahead==1)leave(c,{{4,extended(first)},{5,destination},{6,offset},{7,extended(chosen)}});
                    else leave(c,{{4,base},{5,offset},{6,destination},{7,extended(chosen)}});
                    const auto home=c.i16(destination+0xa);
                    if(ahead==2)c.set(4,extended(home));
                    auto link=[&](bool home) {
                        c.put16(destination+(home?0xa:0xc),match);
                        c.put16(stage.match(round+ahead,std::uint32_t(std::int32_t(chosen)))+(home?0xe:0x10),round);
                        c.put16(stage.match(round,match)+6,std::uint32_t(std::int32_t(chosen)));
                        c.put16(stage.match(round,match)+8,home?1:0);
                        c.put16(stage.match(round,match)+0x12,round+ahead);
                        leave(c,{{3,c.u32(stage.matches()+round*4)},{4,round+ahead},{5,match*0x36+8},
                            {6,home?1u:0u},{7,match*0x36+6},{8,c.u32(stage.matches()+round*4)},
                            {9,c.u32(stage.matches()+(round+ahead)*4)}});
                    };
                    if(compare(c,home,-1)==0){link(true);return true;}
                    const auto away=c.i16(destination+0xc);if(ahead==2)c.set(4,extended(away));
                    if(compare(c,away,-1)==0){link(false);return true;}
                    const auto capacity=c.i16(stage.round(round)+(ahead==1?0x94:0x10c));
                    const auto next=next_match(chosen,capacity);
                    const auto max=std::uint32_t(std::int32_t(capacity))-1;
                    leave(c,{{3,next},{5,max>>31},{6,max},{7,extended(chosen)}});
                    if(ahead==1)c.set(4,extended(first));else c.set(4,next);
                    chosen=std::int16_t(next);
                    // Stop after cycling back to the first candidate.
                    if(compare(c,ahead==1?first:chosen,ahead==1?chosen:first)==0)break;
                }while(true);
                return false;
            };
            if(!try_link(1,chosen)) {
                if(compare(c,round,active_rounds-2)>=0) {
                    reloading_assertion(c,f,0x104be084,0xa0,0x48,0x294);
                    p.call(0x1076f7d0,0x104be0d4,{0xffffffff});
                }
                c.set(3,stage.rounds());chosen=draw_index(c.i16(stage.round(round)+0x10c),0x104be118);
                if(!try_link(2,chosen)) {
                    reloading_assertion(c,f,0x104be370,0x80,0x40,0x2bf);
                    p.call(0x1076f7d0,0x104be3c0,{0xffffffff});return result(c,0xffffffff);
                }
            }
            keep(c,{{15,(match+1)*0x36},{19,match+1}});
        }
        keep(c,{{16,(round+1)*0x78},{17,(round+1)*4},{20,round+1}});
    }
    c.set(3,stage.rounds());
    if(compare(c,c.i16(stage.rounds()+0x1c))==0){c.put8(address+0xb0,1);return result(c,1);}
    leave(c,{{3,0},{4,0}});
    if(compare(c,active_rounds)>0) {
        for(std::uint32_t round=0;round<std::uint32_t(active_rounds);++round) {
            c.set(5,0);
            for(std::uint32_t entrant=0;;++entrant) {
                c.set(6,stage.rounds());
                if(compare(c,entrant,c.i16(stage.round(round)+0x1e))>=0)break;
                auto chosen=std::int16_t(std::int32_t(entrant)/2);
                keep(c,{{13,extended(chosen)}});c.set(6,extended(chosen));
                const auto first=chosen;
                for(;;) {
                    const auto destination=stage.match(round,std::uint32_t(std::int32_t(chosen)));
                    const auto home=c.i16(destination),away=c.i16(destination+2);
                    leave(c,{{7,stage.matches()},{9,destination},{10,extended(home)}});
                    if(compare(c,home,-1)==0 && compare(c,c.i16(destination+0xa),-1)==0) {
                        c.set(6,stage.rounds());c.put16(destination,entrant+std::uint32_t(std::int32_t(c.i16(stage.round(round)+0x20))));break;
                    }
                    c.set(11,extended(away));
                    if(compare(c,away,-1)==0 && compare(c,c.i16(destination+0xc),-1)==0) {
                        c.set(6,stage.rounds());c.put16(destination+2,entrant+std::uint32_t(std::int32_t(c.i16(stage.round(round)+0x20))));break;
                    }
                    const auto host=c.u32(address+0x24);c.set(8,host);
                    if(compare(c,host,-1)!=0 && compare(c,home,-1)!=0 && compare(c,away,-1)!=0) {
                        auto club=c.u32(stage.entrants()+std::uint32_t(std::int32_t(away))*8);c.set(7,club);
                        if(compare_logical(c,club)!=0) {
                            club=c.u32(club+0x3c);c.set(7,club);
                            if(compare_logical(c,club)!=0 && compare(c,host,c.u32(club))==0) {
                                c.put16(destination,std::uint32_t(std::int32_t(away)));c.put16(stage.match(round,std::uint32_t(std::int32_t(chosen)))+2,std::uint32_t(std::int32_t(home)));
                                c.set(7,c.u32(stage.matches()+round*4));
                            }
                        }
                    }
                    const auto count=c.i16(stage.round(round)+0x1c),next=std::int16_t(next_match(chosen,count));
                    leave(c,{{10,extended(chosen)},{7,chosen<std::int32_t(count)-1?0xffffffffu:0},
                        {8,(std::uint32_t(std::int32_t(count))-1)>>31},{9,std::uint32_t(std::int32_t(count))-1}});
                    chosen=next;keep(c,{{13,extended(chosen)}});
                    if(compare(c,first,chosen)==0) {
                        reloading_assertion(c,f,0x104be56c,0x60,0x38,0x310);return result(c,0xffffffff);
                    }
                }
                c.set(5,entrant+1);
            }
            leave(c,{{3,(round+1)*4},{4,(round+1)*0x78}});
        }
    }
    // Assign cumulative display ordinals in reverse round order. The final
    // consolation round (type 140) reserves five places and skips two rounds.
    auto total=std::int32_t(stage.round_count());c.set(5,total);
    bool consolation=false;
    if(compare(c,total)>0) {
        c.set(3,stage.rounds()+std::uint32_t(total)*0x78);
        consolation=compare(c,c.i16(c.r(3)-0x70),0x8c)==0;
    }
    std::uint32_t ordinal;
    std::int32_t last;
    if(consolation) {last=std::int16_t(total-3);ordinal=5;compare(c,last);}
    else {
        const auto last_round=stage.rounds()+std::uint32_t(total)*0x78;
        last=std::int16_t(total-2);compare(c,last);
        c.set(4,extended(c.i16(last_round-0x5e)));ordinal=(c.r(4)+1)&0xff;
    }
    c.set(7,std::uint32_t(last)*0x78);c.set(3,std::uint32_t(last)+1);c.set(8,ordinal);
    if(last<0)return result(c,1);
    auto remaining=std::uint32_t(last)+1;
    compare(c,remaining>>2);
    const auto write_ordinal=[&](std::int32_t round) {
        const auto offset=std::uint32_t(round)*0x78;
        c.put8(stage.round(std::uint32_t(round))+0x68,ordinal);
        c.set(5,stage.rounds());c.set(6,offset+0x68);c.set(4,offset+0x1c);
        ordinal+=c.u16(stage.round(std::uint32_t(round))+0x1c)&0xff;c.set(8,ordinal);
    };
    for(;remaining>=4;remaining-=4) {
        for(unsigned i=0;i<4;++i)write_ordinal(last--);
        c.set(7,std::uint32_t(last)*0x78);
    }
    if(std::uint32_t(total-(consolation?2:1))>=4) {c.set(3,remaining);compare(c,remaining);}
    for(;remaining;--remaining) {write_ordinal(last--);c.set(7,std::uint32_t(last)*0x78);}
    return result(c,1);
}
}
