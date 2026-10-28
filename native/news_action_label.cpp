#include "news_action_detail.hpp"
#include <optional>

namespace cm::aot::news {
using namespace detail;
namespace {
struct Descriptor {
    int toc_slot; unsigned kept;
    std::uint32_t resolve(Cpu& c) const { return kept?c.r(kept):c.toc(toc_slot); }
};
constexpr Descriptor slot_callback(int slot) { return {slot,0}; }
constexpr Descriptor kept_callback(unsigned reg) { return {0,reg}; }
struct Label {
    Cpu& c;
    const Frame& frame;
    ActionItem item;
    std::uint32_t manager, output, callback;
    bool in_category(unsigned expected,std::uint32_t back) const {
        enter(c,back,{item.address}); c.set(3,category(c,item.address));
        return compare(c,std::int8_t(c.r(3)),expected)==0;
    }
    std::uint32_t select(unsigned phrase,std::uint32_t back,Descriptor descriptor,unsigned status) const {
        Program{c}.call(0x10727890,back,{output,c.r(30)+phrase});
        c.put32(callback,descriptor.resolve(c)); return status;
    }
    void update(unsigned slot_index,std::uint32_t value,std::uint32_t back) const {
        const auto queue=c.u32(c.toc(-0x7bf0)),id=c.u32(item.address+0xdc);
        enter(c,back,{queue,manager,id,slot_index,value,item.address});
        c.set(3,update_word(c,queue,manager,id,slot_index,value,item.address));
    }
    std::optional<std::uint32_t> manager_message() const {
        const auto type=c.i32(item.address);
        if(compare(c,type,6004)==0) {
            if(compare(c,item.byte(1))==0) return select(0x3f17,0x102c6cd4,slot_callback(-0x6344),2);
            return std::nullopt;
        }
        if(compare(c,type,6022)!=0) return std::nullopt;
        const auto status=item.byte(1),kind=item.byte(2),reply=item.byte(5);
        leave(c,{{3,std::uint32_t(reply)},{4,std::uint32_t(kind)}});
        if(compare(c,status,5)==0 || compare(c,kind)==0 || compare(c,kind,6)==0 || compare(c,reply)!=0) return std::nullopt;
        Program{c}.call(0x10727890,0x102c6d3c,{output,c.r(30)+0x3f1f,c.r(30)+0x3f52});
        c.put32(callback,c.toc(-0x6348)); return 1;
    }
    std::uint32_t staff() const {
        const auto type=c.i32(item.address);
        if(compare(c,type,3007)==0) {
            const auto club=c.u32(manager+0x24),subject=item.value(0); const auto status=item.byte(7);
            leave(c,{{3,subject},{4,std::uint32_t(status)},{5,club}});
            if(compare_logical(c,club)==0 || compare(c,c.u32(club),subject)!=0 || compare(c,status)!=0) return 0;
            return select(0x3f17,0x102c7540,slot_callback(-0x6374),2);
        }
        if(compare(c,type,3048)==0) {
            const auto club=item.club(8,c.r(28)),pending=item.value(15); const auto status=item.byte(18);
            leave(c,{{3,club},{4,std::uint32_t(status)},{5,pending}});
            if(compare_logical(c,c.u32(club+0x90),manager)!=0 || compare(c,status)!=0 || compare(c,pending)!=0) return 0;
            return select(0x3f17,0x102c75a0,kept_callback(23),2);
        }
        if(compare(c,type,3046)==0) {
            const auto club=item.club(9,c.r(28)); const auto status=item.byte(20);
            leave(c,{{3,club},{4,std::uint32_t(status)}});
            if(compare_logical(c,c.u32(club+0x90),manager)!=0 || compare(c,status)!=0) return 0;
            return select(0x3f17,0x102c7600,kept_callback(23),2);
        }
        if(compare(c,type,3033)==0) {
            const auto club=item.club(4,c.r(28)); const auto pending=item.byte(8),status=item.byte(10);
            leave(c,{{3,club},{4,std::uint32_t(status)}});
            if(compare(c,pending)!=0 || compare_logical(c,c.u32(club+0x90),manager)!=0 || compare(c,status)!=0) return 0;
            return select(0x3f17,0x102c766c,kept_callback(23),2);
        }
        if(compare(c,type,3039)==0) {
            const auto status=item.byte(9),pending=item.byte(10); const auto club=item.club(4,c.r(28));
            leave(c,{{3,club},{4,c.u32(c.r(28))},{5,std::uint32_t(pending)}});
            if(compare(c,status,82)>=0) { if(compare(c,status,94)==0) return 0; }
            else {
                const auto lower=compare(c,status,62);
                if(lower==0 || (lower>0 && compare(c,status,80)>=0)) return 0;
            }
            if(compare_logical(c,c.u32(club+0x90),manager)!=0 || compare(c,pending)!=0) return 0;
            return select(0x3f17,0x102c7708,kept_callback(23),2);
        }
        if(compare(c,type,3037)==0) {
            const auto club=item.club(4,c.r(28)); const auto status=item.byte(13);
            leave(c,{{3,club},{4,std::uint32_t(status)}});
            if(compare_logical(c,c.u32(club+0x90),manager)!=0 || compare(c,status)!=0) return 0;
            return select(0x3f17,0x102c7760,kept_callback(23),2);
        }
        if(compare(c,type,3024)==0) {
            const auto club=item.club(4,c.r(28)); const auto status=item.byte(10);
            leave(c,{{3,club},{4,std::uint32_t(status)}});
            if(compare_logical(c,c.u32(club+0x90),manager)!=0 || compare(c,status)!=0) return 0;
            return select(0x3f17,0x102c77b8,kept_callback(23),2);
        }
        if(compare(c,type,3009)==0) {
            const auto club=c.u32(manager+0x24),subject=item.value(0); const auto status=item.byte(6);
            leave(c,{{3,subject},{4,std::uint32_t(status)},{5,club}});
            if(compare_logical(c,club)==0 || compare(c,c.u32(club),subject)!=0 || compare(c,status)!=0) return 0;
            return select(0x3f17,0x102c7810,slot_callback(-0x637c),2);
        }
        if(compare(c,type,3010)==0) {
            const auto club=c.u32(manager+0x24),subject=item.value(0); const auto status=item.byte(7);
            leave(c,{{3,subject},{4,std::uint32_t(status)},{5,club}});
            if(compare_logical(c,club)==0 || compare(c,c.u32(club),subject)!=0 || compare(c,status)!=0) return 0;
            return select(0x3f69,0x102c7864,slot_callback(-0x6380),1);
        }
        if(compare(c,type,3000)!=0) return 0;
        return contract_negotiation();
    }
    std::uint32_t contract_negotiation() const;
    std::uint32_t competition_message() const;
    std::uint32_t appeal() const;
    std::optional<std::uint32_t> transfer() const;
};

std::uint32_t Label::contract_negotiation() const {
    const auto person=item.value(0),club_id=item.value(4),offer_id=item.value(8),key=item.value(9);
    const auto mode=item.byte(10);
    keep(c,{{13,person},{14,club_id},{15,key}}); leave(c,{{4,offer_id},{5,std::uint32_t(mode)}});
    if(compare_logical(c,manager)==0) return 0;
    const auto club=c.u32(manager+0x24);
    if(compare_logical(c,club)==0) return 0;
    c.set(3,club);
    if(compare(c,c.u32(club),club_id)!=0) return 0;
    if(compare(c,offer_id,-1)!=0) {
        const auto slot=Program{c}.call(0x103eb440,0x102c78cc,{c.u32(c.r(25)),offer_id});
        if(compare_logical(c,c.u32(slot))==0) return 0;
        c.set(3,c.u32(slot));
        if(compare(c,c.u32(c.r(3)+0x2c),key)!=0) return 0;
    } else {
        compare_logical(c,manager);
        if(compare(c,mode,5)==0) return 0;
    }
    const auto contract=Program{c}.call(0x1007f720,offer_id==0xffffffff?0x102c794c:0x102c78f8,
                                        {c.u32(c.r(26)),person,club_id});
    if(compare_logical(c,contract)==0 || compare(c,c.u8(contract+0x64),3)!=0) return 0;
    return select(0x3f78,offer_id==0xffffffff?0x102c796c:0x102c7918,kept_callback(22),2);
}

std::uint32_t Label::competition_message() const {
    const auto type=c.i32(item.address); Program program{c};
    if(compare(c,type,6)==0) {
        const auto year=item.value(3),day=item.value(4); const auto request=item.byte(5);
        const auto first=item.value(0),second=item.value(1),requester=item.value(2);
        keep(c,{{13,first},{19,second},{20,year&0xffff},{21,day&0xffff},
                {22,std::uint32_t(request)},{15,requester}});
        leave(c,{{4,day},{5,year}});
        program.call(0x10715880,0x102c6d98,{frame.at(0x84)});
        if(compare(c,request,3)==0 || compare(c,request,4)==0) return 0;
        const auto opposing=compare(c,requester,first)==0?second:first;
        c.set(3,c.u32(c.r(28))); c.set(22,c.r(3)+opposing*0x248);
        if(compare_logical(c,c.u32(manager+0x24),c.r(22))!=0 && compare_logical(c,c.u32(manager+0x20),c.r(22))!=0) return 0;
        program.call(0x107156b0,0x102c6e0c,{frame.at(0x64),extended(std::int16_t(year)),day&0xffff});
        const auto date_year=c.i16(frame.at(0x64)); const auto date_day=c.u16(frame.at(0x66));
        const auto weekday=c.u8(frame.at(0x68));
        c.put16(frame.at(0x84),date_year); c.put16(frame.at(0x86),date_day); c.put8(frame.at(0x88),weekday);
        const auto fixture=program.call(0x10497570,0x102c6e3c,
            {c.u32(c.r(14)),frame.at(0x84),c.u32(c.r(28))+requester*0x248});
        c.put32(frame.at(0x38),0);
        const auto valid=program.call(0x10498f70,0x102c6e78,
            {c.u32(c.r(14)),c.u32(c.r(28))+first*0x248,c.u32(c.r(28))+second*0x248,
             c.r(22),year&0xffff,day&0xffff,fixture,0});
        if(compare(c,std::int8_t(valid))==0) return 0;
        return select(0x3f17,0x102c6e8c,slot_callback(-0x634c),2);
    }
    if(compare(c,type,9)==0) {
        program.call(0x10715880,0x102c6eb0,{frame.at(0x7c)});
        const auto status=item.byte(3),mode=item.byte(2);
        const auto year=item.value(5),day=item.value(6),club_id=item.value(1);
        keep(c,{{15,club_id},{13,std::uint32_t(mode)}});
        leave(c,{{4,year&0xffff},{5,day&0xffff}});
        if(compare(c,status,3)==0 || compare(c,status,4)==0) return 0;
        program.call(0x107156b0,0x102c6efc,{frame.at(0x5c),extended(std::int16_t(year)),day&0xffff});
        const auto date_day=c.u16(frame.at(0x5e)); const auto date_year=c.i16(frame.at(0x5c));
        c.put16(frame.at(0x7e),date_day); c.put16(frame.at(0x7c),date_year);
        c.put8(frame.at(0x80),c.u8(frame.at(0x60)));
        const auto fixture=program.call(0x104976b0,0x102c6f24,{c.u32(c.r(14)),std::uint32_t(mode),frame.at(0x7c)});
        const auto valid=program.call(0x104991b0,0x102c6f44,
            {c.u32(c.r(14)),c.u32(c.r(28))+club_id*0x248,fixture,0,0});
        if(compare(c,std::int8_t(valid))==0) return 0;
        return select(0x3f17,0x102c6f58,slot_callback(-0x6350),2);
    }
    if(compare(c,type,22)==0) {
        const auto state=item.byte(0); const auto club_id=item.value(1),year=item.value(2)&0xffff;
        leave(c,{{4,club_id},{5,std::uint32_t(state)},{6,year}});
        if(compare_logical(c,manager)==0 || compare_logical(c,c.u32(manager+0x24))==0) return 0;
        c.set(3,c.u32(manager+0x24));
        if(compare(c,c.u32(c.r(3)),club_id)!=0) return 0;
        const auto global=c.u32(c.toc(-0x7a48)); c.set(4,global);
        if(compare_logical(c,year,c.u16(global+0x4b2))!=0 || compare(c,state)!=0) return 0;
        return select(0x3f17,0x102c6fe0,slot_callback(-0x6354),2);
    }
    if(compare(c,type,24)==0 || compare(c,type,25)==0) {
        const auto status=item.byte(type==24?1:3); const auto club_id=item.value(0);
        leave(c,{{4,club_id},{5,std::uint32_t(status)}});
        if(compare_logical(c,manager)==0 || compare_logical(c,c.u32(manager+0x24))==0) return 0;
        c.set(3,c.u32(manager+0x24));
        if(compare(c,c.u32(c.r(3)),club_id)!=0 || compare(c,status)!=0) return 0;
        return select(0x3f17,type==24?0x102c7050:0x102c70c0,slot_callback(type==24?-0x6358:-0x635c),2);
    }
    if(compare(c,type,30)==0) {
        if(compare(c,item.byte(2))!=0) return 0;
        return select(0x3f17,0x102c70fc,slot_callback(-0x6360),2);
    }
    if(compare(c,type,34)==0) {
        if(compare(c,item.byte(7))!=0) return 0;
        const auto today=c.r(31); c.put32(frame.at(0x54),c.u32(today)); c.set(5,c.u32(today+4)); c.put32(frame.at(0x58),c.r(5));
        bool current=false; c.set(4,0);
        if(compare_logical(c,c.u16(frame.at(0x56)),c.u16(item.address+0xd2))==0) {
            c.set(3,extended(c.i16(frame.at(0x54))));
            if(compare(c,c.r(3),c.i16(item.address+0xd0))==0) current=true;
        }
        c.set(4,current);
        if(compare(c,current)==0) { update(7,1,0x102c7194); return 0; }
        const auto club_id=item.value(4); c.set(13,club_id);
        const auto club=c.u32(c.r(28))+club_id*0x248;
        const auto selected=program.call(0x102ae9c0,0x102c71c0,{c.u32(c.toc(-0x7a64)),club,std::uint32_t(item.byte(5))});
        const auto valid=program.call(0x102aea20,0x102c71e0,{c.u32(c.toc(-0x7a64)),c.u32(club+0x2c),selected});
        if(compare(c,std::int8_t(valid))==0) return 0;
        return select(0x3f17,0x102c71fc,slot_callback(-0x6364),2);
    }
    if(compare(c,type,39)==0) {
        const auto status=item.value(6),competition_id=item.value(0);
        const auto year=item.short_value(1),round=item.short_value(5); const auto stage=item.byte(4);
        leave(c,{{4,std::uint32_t(stage)},{5,std::uint32_t(round)},{7,competition_id}});
        if(compare(c,status)!=0) return 0;
        const auto table=c.u32(c.toc(-0x7b5c)); c.set(6,table);
        auto competition=c.u32(table+competition_id*4); c.set(3,competition);
        if(compare_logical(c,competition)==0) return 0;
        c.set(6,c.u32(competition+0x30)); c.set(7,std::uint32_t(stage));
        if(compare(c,stage,c.r(6))>0) return 0;
        if(compare(c,stage,-1)==0) {
            c.set(6,c.u8(competition+0x43));
            if(compare(c,c.r(6),1)!=0) return 0;
            c.set(7,extended(c.i16(competition+0x3a))); c.set(6,std::uint32_t(year));
            if(compare(c,c.r(7),year)!=0) return 0;
        } else {
            const auto stage_table=c.u32(competition+0xc);c.set(7,stage_table);
            const auto stage_object=c.u32(stage_table+std::uint32_t(stage)*4);
            c.set(6,c.u8(stage_object+0x43));
            if(compare(c,c.r(6),1)!=0) return 0;
            c.set(7,c.u32(competition+0xc)); c.set(6,stage_object); c.set(8,std::uint32_t(year));
            if(compare(c,year,c.i16(stage_object+0x3a))!=0) return 0;
        }
        if(compare(c,stage,-1)!=0) competition=c.u32(c.u32(competition+0xc)+std::uint32_t(stage)*4);
        c.set(3,competition); c.set(4,std::uint32_t(c.i8(competition+0xa8)));
        if(compare(c,c.i8(competition+0xa8),round)<0) {
            const auto round_header=c.u32(competition+0xa0)+std::uint32_t(round)*0x78;
            const auto ties=c.u32(competition+0xac)+std::uint32_t(round)*4;
            const auto previous=c.u32(ties-4)+std::uint32_t(c.i16(round_header-0x5c))*0x36;
            leave(c,{{4,previous},{6,ties},{7,c.u32(competition+0xac)}});
            if(compare(c,c.u8(previous-2),3)!=0) return 0;
        }
        std::uint32_t tie=0; c.set(7,tie);
        for(;;) {
            const auto round_header=c.u32(competition+0xa0)+std::uint32_t(round)*0x78;
            c.set(4,round_header); c.set(6,std::uint32_t(std::int16_t(tie)));
            if(compare(c,std::int16_t(tie),c.i16(round_header+0x1c))>=0) break;
            const auto table_base=c.u32(competition+0xac); c.set(6,table_base);
            c.set(4,c.u32(table_base+std::uint32_t(round)*4)+std::uint32_t(std::int16_t(tie))*0x36);
            if(compare(c,c.u8(c.r(4)+0x34),3)!=0) return select(0x3f53,0x102c73b4,kept_callback(24),1);
            c.set(7,++tie);
        }
        update(6,1,0x102c7404); return 0;
    }
    if(compare(c,type,40)==0) {
        c.set(4,c.toc(-0x6918));
        const auto competition=c.u32(c.u32(c.toc(-0x7b5c))+c.u32(c.r(4))*4); c.set(3,competition);
        if(compare(c,c.u8(competition+0xb4),1)!=0 || compare(c,item.value(0),1)!=0) return 0;
        return select(0x3f17,0x102c7454,slot_callback(-0x636c),2);
    }
    if(compare(c,type,28)!=0) return 0;
    enter(c,0x102c747c,{item.address,0}); c.set(3,word(c,item.address,0));
    const auto first=c.r(3); c.set(13,first);
    enter(c,0x102c7490,{item.address,1}); c.set(3,word(c,item.address,1));
    const auto second=c.r(3),awards=c.u32(c.toc(-0x7bb4)); c.set(5,awards); c.set(4,first*4);
    const auto nomination=c.u32(c.u32(c.u32(awards+8)+first*4)+4);
    c.set(3,c.u32(nomination+second*4));
    const auto details=c.u32(c.r(3)+0x24);
    if(compare_logical(c,details)==0) return 0;
    c.set(3,details);
    if(compare(c,c.i16(details+4))==0) return 0;
    return select(0x3f5d,0x102c74d8,slot_callback(-0x6370),1);
}

std::uint32_t Label::appeal() const {
    const auto type=c.i32(item.address);
    if(compare(c,type,2003)!=0 && compare(c,type,2002)!=0) return 0;
    Program program{c};
    const auto first_year=item.short_value(4),second_year=item.short_value(15);
    const auto first_day=item.value(5)&0xffff,second_day=item.value(16)&0xffff;
    keep(c,{{13,std::uint32_t(item.byte(0))},{14,std::uint32_t(first_year)},
            {19,first_day},{20,std::uint32_t(second_year)},{21,second_day},
            {22,item.value(10)},{23,std::uint32_t(item.byte(12))}});
    leave(c,{{4,item.value(16)},{5,item.value(15)}});
    program.call(0x10715880,0x102c79e0,{frame.at(0x11c)});
    program.call(0x10715880,0x102c79e8,{frame.at(0xb4)});
    c.put8(frame.at(0x40),0x11);
    program.call(0x1030e2a0,0x102c7a00,{item.address,frame.at(0x40),frame.at(0xa4)});
    const auto decoded_manager=c.i32(frame.at(0xb0));
    if(compare(c,decoded_manager)<0) return 0;
    c.set(3,c.u32(c.toc(slot::first_manager))); c.set(4,decoded_manager);
    if(compare(c,decoded_manager,c.r(3)-1)>0) return 0;
    if(compare(c,item.byte(23))!=0) {
        c.put8(frame.at(0x40),0x18);
        program.call(0x1030dee0,0x102c7a50,{item.address,frame.at(0x40),frame.at(0x11c)});
        const auto club_id=c.i32(frame.at(0x130));
        if(compare(c,club_id)<0) return 0;
        c.set(3,c.u32(c.toc(-0x7b50))); c.set(4,club_id);
        if(compare(c,club_id,c.r(3)-1)>0) return 0;
        const auto competition_id=c.i32(frame.at(0x124));
        if(compare(c,competition_id)<0) return 0;
        c.set(3,c.u32(c.toc(-0x7b64))); c.set(4,competition_id);
        if(compare(c,competition_id,c.r(3)-1)>0) return 0;
        const auto club=c.u32(c.r(28))+std::uint32_t(club_id)*0x248;
        const auto competition_base=c.u32(c.toc(-0x7bbc)),person_base=c.u32(c.r(27));
        keep(c,{{15,competition_base+std::uint32_t(competition_id)*0x6c},
                {24,person_base+std::uint32_t(decoded_manager)*0x70}});
        leave(c,{{4,person_base},{5,std::uint32_t(competition_id)*0x6c},{6,competition_base}});
        const auto special=program.call(0x100c7d00,0x102c7ae0,{club});
        if(compare(c,std::int8_t(special))!=0 || compare_logical(c,c.u32(c.r(15)+8))==0) return 0;
        c.set(3,c.u32(manager+0x24));
        if(compare_logical(c,c.r(3),c.u32(c.r(24)+0x24))!=0) return 0;
    }
    program.call(0x107156b0,0x102c7b24,{frame.at(0x74),c.r(14),c.r(19)});
    program.call(0x107156b0,0x102c7b34,{frame.at(0x6c),c.r(20),c.r(21)});
    std::uint32_t eligible=0;
    if(compare_logical(c,manager)!=0) eligible=program.call(0x100dbbe0,0x102c7b44,{c.u32(manager+0x24)});
    const auto code=c.r(22);
    if(compare(c,code,4)==0 || compare(c,code)==0 || compare(c,code,1)==0) eligible=0;
    if(compare(c,std::int8_t(c.r(13)))!=0 || compare(c,std::int8_t(eligible))==0 || compare(c,std::int8_t(c.r(23)))!=0) return 0;
    // Dates are copied as complete eight-byte images before comparison, so
    // their original padding and the vararg residues remain observable.
    c.put32(frame.at(0x4c),c.u32(frame.at(0x6c))); c.set(5,c.u32(frame.at(0x70)));
    c.put32(frame.at(0x50),c.r(5)); c.set(4,0);
    c.set(3,c.u16(frame.at(0x4e)));
    bool same=false;
    if(compare_logical(c,c.r(3),c.u16(c.r(31)+2))==0) {
        c.set(3,extended(c.i16(frame.at(0x4c))));
        same=compare(c,c.r(3),c.i16(c.r(31)))==0;
    }
    c.set(4,same);
    if(compare(c,same)!=0) return select(0x3f82,0x102c7c34,slot_callback(-0x6388),1);
    c.put32(frame.at(0x44),c.u32(frame.at(0x74))); c.set(4,c.u32(frame.at(0x78)));
    c.put32(frame.at(0x48),c.r(4)); c.set(3,c.u16(frame.at(0x46)));
    bool later=false;
    if(compare_logical(c,c.r(3),c.u16(c.r(31)+2))>0) later=true;
    else if(compare_logical(c,c.u16(frame.at(0x46)),c.u16(c.r(31)+2))>=0) {
        const auto year=c.i16(frame.at(0x44)),today=c.i16(c.r(31));
        const auto difference=std::uint32_t(std::int32_t(year))^std::uint32_t(std::int32_t(today));
        c.set(4,extended(year)); c.set(3,std::uint32_t(std::int32_t(difference)>>1));
        c.s.wr<1>(reg::xer_ca,difference&1); later=year>today;
    }
    if(compare(c,later)==0) return 0;
    return select(0x3f82,0x102c7c34,slot_callback(-0x6388),1);
}

std::optional<std::uint32_t> Label::transfer() const {
    Program program{c};
    const auto is_type=[&](int type){return compare(c,c.i32(item.address),type)==0;};
    if(is_type(4000)) {
        const auto destination=item.value(5),key=item.value(7); const auto status=item.byte(9);
        keep(c,{{14,destination},{13,key},{23,std::uint32_t(status)}});
        const auto slot=program.call(0x103eb440,0x102c7c88,{c.u32(c.r(25)),item.value(6)});
        if(compare_logical(c,c.u32(slot))==0 || compare(c,destination,-1)==0) return std::nullopt;
        c.set(4,c.u32(c.r(28))+destination*0x248);
        if(compare_logical(c,c.u32(c.r(4)+0x90),manager)!=0) return std::nullopt;
        c.set(4,c.u32(slot));
        if(compare(c,c.u32(c.r(4)+0x2c),key)!=0) return std::nullopt;
        if(compare(c,status,1)==0 || compare(c,status,5)==0 || compare(c,status,2)==0 || compare(c,status,17)==0) return std::nullopt;
        c.set(3,c.u32(slot));
        if(compare(c,c.i8(c.r(3)+0x34))!=0) return std::nullopt;
        return select(0x3f17,0x102c7d10,slot_callback(-0x638c),2);
    }
    if(is_type(4028)) {
        const auto club_id=item.value(4); const auto status=item.byte(7);
        leave(c,{{3,club_id},{4,std::uint32_t(status)}});
        if(compare(c,club_id,-1)<=0) return std::nullopt;
        c.set(3,c.u32(c.r(28))+club_id*0x248);
        if(compare_logical(c,c.u32(c.r(3)+0x90),manager)!=0 || compare(c,status)!=0) return std::nullopt;
        return select(0x3f17,0x102c7d6c,kept_callback(23),2);
    }
    if(is_type(4002)) {
        const auto key=item.value(7),secondary_key=item.value(9);
        keep(c,{{24,0},{13,key},{23,secondary_key}});
        const auto slot=program.call(0x103eb440,0x102c7d9c,{c.u32(c.r(25)),item.value(6)}); c.set(14,slot);
        bool linked=false;
        if(compare_logical(c,c.u32(slot))!=0) {
            c.set(3,c.u32(slot));
            if(compare(c,c.u32(c.r(3)+0x2c),key)==0) {
                c.set(3,c.u32(slot));
                if(compare(c,c.u32(c.r(3)+0x30),-1)!=0) {
                    c.set(4,c.u32(slot));
                    const auto secondary=program.call(0x103eb470,0x102c7ddc,{c.u32(c.r(25)),c.u32(c.r(4)+0x30)});
                    c.set(24,1); c.set(19,secondary); linked=true;
                }
            }
        }
        if(compare(c,linked)==0) return std::nullopt;
        c.set(3,c.u32(c.r(19)));
        if(compare(c,secondary_key,c.u32(c.r(3)+0x2c))!=0) return std::nullopt;
        c.set(3,c.u32(slot));
        if(compare(c,c.u8(c.r(3)+0x34),1)!=0) return std::nullopt;
        return select(0x3f17,0x102c7e18,slot_callback(-0x6390),2);
    }
    if(is_type(4012)) {
        const auto status=item.byte(9); const auto person=item.value(0),key=item.value(8),club=c.u32(manager+0x24);
        keep(c,{{24,std::uint32_t(status)},{14,person},{13,key},{23,club}});
        const auto slot=program.call(0x103eb440,0x102c7e54,{c.u32(c.r(25)),item.value(7)}); c.set(19,slot);
        if(compare(c,status,1)==0 || compare(c,status,4)==0) return 0;
        if(compare_logical(c,c.u32(slot))==0) return std::nullopt;
        c.set(3,c.u32(slot));
        if(compare(c,c.u32(c.r(3)+0x2c),key)!=0) return std::nullopt;
        const auto blocked=program.call(0x103fb390,0x102c7ea0,{c.u32(c.r(25)),c.u32(slot)});
        if(compare(c,std::int8_t(blocked))!=0) return std::nullopt;
        c.set(3,c.u32(slot));
        if(compare(c,c.u8(c.r(3)+0x34),1)==0) return std::nullopt;
        if(compare_logical(c,club)!=0) {
            c.set(3,c.u32(slot)); c.set(4,c.u32(club));
            if(compare(c,c.r(4),c.u32(c.r(3)+8))==0) {
                c.set(3,c.u32(slot));
                if(compare(c,c.i8(c.r(3)+0x10))!=0) {
                    c.set(3,c.u32(slot));
                    if(compare(c,c.u8(c.r(3)+0x34),5)!=0) {
                        c.set(3,c.u32(slot));
                        if(compare(c,c.u8(c.r(3)+0x34),6)!=0) return std::nullopt;
                    }
                }
                return select(0x3f69,0x102c7f10,kept_callback(21),2);
            }
        }
        c.set(3,c.u32(slot));
        const auto contract=program.call(0x10082f20,0x102c7f38,
            {c.u32(c.r(26)),c.u32(c.r(27))+c.u32(c.r(3)+0xc)*0x70});
        const auto present=compare_logical(c,contract); c.put32(c.r(20),contract);
        std::uint32_t employer=0;
        if(present!=0) { c.set(3,c.u32(c.r(20))); c.set(4,c.u32(c.r(28))); employer=c.r(4)+c.u32(c.r(3)+4)*0x248; }
        if(compare_logical(c,employer,club)!=0) return std::nullopt;
        c.set(3,c.u32(slot));
        const auto list=c.u32(c.r(3)+4);
        if(compare_logical(c,list)==0) return std::nullopt;
        c.set(3,list); c.put32(list+0xc,c.u32(list+4));
        for(;;) {
            c.set(3,c.u32(slot));
            const auto entry=program.call(0x10738e00,0x102c7fc8,{c.u32(c.r(3)+4)});
            if(compare_logical(c,entry)==0) break;
            if(compare(c,c.u32(entry),person)==0 && compare(c,c.i8(entry+4))==0)
                return select(0x3f69,0x102c7fb0,kept_callback(21),2);
        }
        return std::nullopt;
    }
    if(is_type(4004)) {
        const auto mode=item.byte(6),status=item.byte(9); const auto offer=item.value(7); c.set(13,offer); c.set(3,std::uint32_t(status));
        if(compare(c,mode,3)==0 && compare(c,offer,-1)!=0) {
            const auto key=item.value(8); c.set(14,key);
            const auto slot=program.call(0x103eb440,0x102c8010,{c.u32(c.r(25)),offer});
            if(compare_logical(c,c.u32(slot))!=0) {
                c.set(4,c.u32(slot));
                if(compare(c,c.u32(c.r(4)+0x2c),key)==0) {
                    c.set(4,c.u32(slot));
                    if(compare(c,c.u8(c.r(4)+0x34),7)==0 || (c.set(3,c.u32(slot)),compare(c,c.u8(c.r(3)+0x34),8)==0)) {
                        const auto contract=program.call(0x1007f720,0x102c805c,{c.u32(c.r(26)),item.value(0),item.value(5)});
                        if(compare_logical(c,contract)!=0 && compare(c,c.u8(contract+0x64),3)==0)
                            return select(0x3f78,0x102c807c,kept_callback(22),2);
                    }
                }
            }
        } else if(compare(c,mode,4)==0 && compare(c,offer,-1)!=0 && compare(c,status)==0) {
            const auto key=item.value(8); c.set(14,key);
            const auto slot=program.call(0x103eb440,0x102c80b4,{c.u32(c.r(25)),offer}); c.set(5,item.value(5));
            if(compare_logical(c,c.u32(slot))!=0) {
                c.set(4,c.u32(slot));
                if(compare(c,c.u32(c.r(4)+0x2c),key)==0 && compare_logical(c,c.u32(manager+0x24))!=0) {
                    c.set(4,c.u32(manager+0x24));
                    if(compare(c,c.u32(c.r(4)),c.r(5))==0) {
                        c.set(4,c.u32(slot));
                        if(compare(c,c.u8(c.r(4)+0x34),15)==0) {
                            c.set(3,c.u32(slot));
                            if(compare_logical(c,c.u8(c.r(3)+0x36),3)<0)
                                return select(0x3f78,0x102c811c,kept_callback(22),1);
                        }
                    }
                }
            }
        }
        if(compare(c,offer,-1)!=0) update(7,0xffffffff,0x102c8150);
        return std::nullopt;
    }
    if(is_type(4005)) {
        const auto status=item.byte(9); const auto club=c.u32(manager+0x24),key=item.value(7);
        keep(c,{{19,club},{13,key}}); leave(c,{{3,std::uint32_t(status)},{4,item.value(6)}});
        if(compare_logical(c,std::uint8_t(status-12),2)<=0 || compare_logical(c,std::uint8_t(status-2),1)<=0 ||
           compare(c,status,16)==0 || compare(c,status,18)==0 || compare(c,status,17)==0) return 0;
        const auto slot=program.call(0x103eb440,0x102c81c4,{c.u32(c.r(25)),c.r(4)}); c.set(14,slot);
        if(compare_logical(c,c.u32(slot))==0) return std::nullopt;
        c.set(3,c.u32(slot));
        if(compare(c,c.u32(c.r(3)+0x2c),key)!=0) return std::nullopt;
        if(compare_logical(c,club)!=0) {
            c.set(3,c.u32(slot)); c.set(4,c.u32(club));
            if(compare(c,c.r(4),c.u32(c.r(3)+8))==0) {
                c.set(3,c.u32(slot));
                if(compare(c,c.u8(c.r(3)+0x34),11)!=0) {
                    c.set(3,c.u32(slot)); if(compare(c,c.u8(c.r(3)+0x34),12)!=0) return std::nullopt;
                }
                return select(0x3f89,0x102c822c,slot_callback(-0x6398),2);
            }
        }
        c.set(3,c.u32(slot));
        const auto contract=program.call(0x10082f20,0x102c8258,
            {c.u32(c.r(26)),c.u32(c.r(27))+c.u32(c.r(3)+0xc)*0x70});
        const auto present=compare_logical(c,contract); c.put32(c.r(20),contract);
        std::uint32_t employer=0;
        if(present!=0) { c.set(3,c.u32(c.r(20))); c.set(4,c.u32(c.r(28))); employer=c.r(4)+c.u32(c.r(3)+4)*0x248; }
        if(compare_logical(c,employer,club)!=0) return std::nullopt;
        c.set(3,c.u32(slot));
        if(compare(c,c.i8(c.r(3)+0x37))!=0) return std::nullopt;
        c.set(3,c.u32(slot));
        if(compare(c,c.u8(c.r(3)+0x34),11)!=0) {
            c.set(3,c.u32(slot)); if(compare(c,c.u8(c.r(3)+0x34),13)!=0) return std::nullopt;
        }
        return select(0x3f89,0x102c82c4,slot_callback(-0x6398),2);
    }
    if(is_type(4008)) return 0;
    if(is_type(4011)) {
        const auto club=c.u32(manager+0x24),first=item.value(0),second=item.value(1); const auto status=item.byte(7);
        leave(c,{{3,first},{4,second},{5,std::uint32_t(status)},{6,club}});
        if(compare_logical(c,club)==0) return std::nullopt;
        if(compare(c,c.u32(club),first)!=0 && compare(c,c.u32(club),second)!=0) return std::nullopt;
        if(compare(c,status)!=0) return std::nullopt;
        return select(0x3f17,0x102c833c,slot_callback(-0x639c),2);
    }
    if(is_type(4020)) {
        const auto staff_id=item.value(0),club_id=item.value(5),key=item.value(12);
        keep(c,{{22,std::uint32_t(item.byte(10))},{23,std::uint32_t(item.byte(13))},
                {19,staff_id},{21,club_id},{13,key}});
        const auto slot=program.call(0x103eb440,0x102c8380,{c.u32(c.r(25)),item.value(11)}); c.set(14,slot);
        if(compare(c,slot)==0 || compare_logical(c,c.u32(slot))==0) return 0;
        c.set(3,c.u32(slot)); c.set(4,c.u32(c.r(28)));
        const auto destination=c.r(4)+c.u32(c.r(3)+8)*0x248;
        c.set(3,destination);
        if(compare_logical(c,destination,c.u32(manager+0x24))!=0) return 0;
        const auto employer=[&](std::uint32_t back,bool leave_club) {
            const auto contract=program.call(0x10082f20,back,{c.u32(c.r(26)),c.u32(c.r(27))+staff_id*0x70});
            const auto present=compare_logical(c,contract); c.put32(c.r(20),contract);
            if(present==0) { if(leave_club)c.set(3,0); return 0u; }
            c.set(3,c.u32(c.r(20))); c.set(4,c.u32(c.r(28)));
            const auto club=c.r(4)+c.u32(c.r(3)+4)*0x248;
            if(leave_club) c.set(3,club);
            return club;
        };
        if(compare_logical(c,employer(0x102c83d8,false))==0) return 0;
        const auto current_club=employer(0x102c841c,true);
        // The reference unconditionally dereferences the second lookup result.
        if(compare(c,club_id,c.u32(current_club))!=0) return 0;
        c.set(3,c.u32(slot));
        if(compare(c,c.u32(c.r(3)+0x2c),key)!=0) return std::nullopt;
        c.set(3,c.u32(slot)); if(compare(c,c.i8(c.r(3)+0x37))!=0) return std::nullopt;
        c.set(3,c.u32(slot)); if(compare(c,c.u8(c.r(3)+0x34),1)!=0) return std::nullopt;
        if(compare(c,std::int8_t(c.r(23)))!=0) return std::nullopt;
        c.set(3,c.u32(slot)); c.set(3,std::uint32_t(std::int32_t(c.i8(c.r(3)+0x34))));
        if(compare(c,c.r(3),std::int8_t(c.r(22)))!=0) return std::nullopt;
        return select(0x3f91,0x102c84b4,slot_callback(-0x63a0),2);
    }
    if(is_type(4021)) {
        const auto destination=item.value(5),key=item.value(7); keep(c,{{14,destination},{13,key}});
        const auto slot=program.call(0x103eb440,0x102c84e4,{c.u32(c.r(25)),item.value(6)});
        if(compare_logical(c,c.u32(slot))==0 || compare(c,destination,-1)==0) return std::nullopt;
        c.set(4,c.u32(c.r(28))+destination*0x248);
        if(compare_logical(c,c.u32(c.r(4)+0x90),manager)!=0) return std::nullopt;
        c.set(4,c.u32(slot)); if(compare(c,c.u32(c.r(4)+0x2c),key)!=0) return std::nullopt;
        c.set(3,c.u32(slot)); if(compare(c,c.i8(c.r(3)+0x34))!=0) return std::nullopt;
        return select(0x3f17,0x102c853c,slot_callback(-0x63a4),2);
    }
    if(is_type(4022)) return 0;
    if(is_type(4032)) {
        const auto key=item.value(7),secondary_key=item.value(9); keep(c,{{19,0},{16,key},{14,secondary_key}});
        const auto slot=program.call(0x103eb440,0x102c8584,{c.u32(c.r(25)),item.value(6)}); c.set(13,slot);
        bool linked=false;
        if(compare_logical(c,c.u32(slot))!=0) {
            c.set(3,c.u32(slot));
            if(compare(c,c.u32(c.r(3)+0x2c),key)==0) {
                c.set(3,c.u32(slot));
                if(compare(c,c.u32(c.r(3)+0x30),-1)!=0) {
                    c.set(4,c.u32(slot));
                    const auto secondary=program.call(0x103eb470,0x102c85c4,{c.u32(c.r(25)),c.u32(c.r(4)+0x30)});
                    c.set(19,1); c.put32(frame.at(0x140),secondary); linked=true;
                }
            }
        }
        if(compare(c,linked)==0) return std::nullopt;
        c.set(3,c.u32(c.u32(frame.at(0x140))));
        if(compare(c,secondary_key,c.u32(c.r(3)+0x2c))!=0) return std::nullopt;
        c.set(3,c.u32(slot)); if(compare(c,c.u8(c.r(3)+0x34),1)!=0) return std::nullopt;
        return select(0x3f17,0x102c8604,slot_callback(-0x63a8),2);
    }
    if(is_type(4033)) return select(0x3f9e,0x102c862c,slot_callback(-0x63ac),1);
    return std::nullopt;
}
}

std::uint32_t action_label(Cpu& c,std::uint32_t item,std::uint32_t manager,
                            std::uint32_t output,std::uint32_t capacity,std::uint32_t callback) {
    Frame frame(c,0x102c6b10,0x1a0,{.first_gpr=13});
    keep(c,{{13,c.toc(slot::success)},{14,c.toc(-0x7b54)},{15,item},{16,manager},{17,output},{18,callback},
            {20,c.toc(-0x7b78)},{21,c.toc(-0x6394)},{22,c.toc(-0x6384)},{23,c.toc(-0x6378)},
            {24,c.toc(-0x6368)},{25,c.toc(-0x7b40)},{26,c.toc(-0x7b7c)},{27,c.toc(-0x7be0)},
            {28,c.toc(-0x7be8)},{29,c.toc(slot::message)},{30,c.toc(slot::pool)},{31,c.toc(slot::today)}});
    if(compare(c,manager)==0) { assertion(c,frame,0x102c6b80,0xfc,0x9c,0x123b); return 0; }
    if(compare_logical(c,output)==0 || compare(c,capacity,51)<0) {
        assertion(c,frame,0x102c6be8,0xdc,0x94,0x1243); return 0;
    }
    const auto callback_present=compare_logical(c,callback); c.put8(output,0);
    if(callback_present==0) { assertion(c,frame,0x102c6c50,0xbc,0x8c,0x124e); return 0; }
    c.put32(callback,0);
    Label label{c,frame,{c,item},manager,output,callback};
    if(label.in_category(6,0x102c6ca4)) if(const auto result=label.manager_message()) return *result;
    if(label.in_category(2,0x102c6d54)) return label.competition_message();
    if(label.in_category(4,0x102c74f0)) return label.staff();
    if(label.in_category(3,0x102c7980)) return label.appeal();
    if(label.in_category(5,0x102c7c54)) {
        if(const auto result=label.transfer()) return *result;
        compare(c,c.i32(item),4033); return 0;
    }
    if(label.in_category(1,0x102c8644) && compare(c,c.i32(item),5000)==0)
        return label.select(0x3fa6,0x102c8668,slot_callback(-0x63b0),1);
    return 0;
}
}
