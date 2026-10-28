#include "news_action_detail.hpp"
#include "news_random.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::news {
using namespace detail;
namespace {
struct Effects {
    Cpu& c; const Frame& frame; ActionItem item; std::uint32_t manager;
    bool is_type(int type) const { return compare(c,c.i32(item.address),type)==0; }
    bool ordinary_club() const { return compare(c,std::int8_t(c.r(19)))==0; }
    bool in_category(unsigned expected,std::uint32_t back) const {
        enter(c,back,{item.address}); c.set(3,category(c,item.address));
        return compare(c,std::int8_t(c.r(3)),expected)==0;
    }
    void update(unsigned slot_index,std::uint32_t value,std::uint32_t back,bool current=true) const {
        const auto queue=c.u32(c.r(23)),id=c.u32(item.address+0xdc);
        enter(c,back,{queue,manager,id,slot_index,value,current?item.address:0});
        c.set(3,update_word(c,queue,manager,id,slot_index,value,current?item.address:0));
    }
    bool employment() const {
        Program program{c};
        const auto policy=program.call(0x10147e70,0x102c87b0,{c.u32(c.r(18)),manager});
        bool allowed=compare(c,std::int8_t(policy))!=0;
        if(allowed&&compare_logical(c,c.u32(manager+0x24))!=0)
            allowed=compare(c,std::int8_t(program.call(0x10149df0,0x102c87d4,{c.u32(c.r(18)),1,manager})))!=0;
        if(!allowed) { update(1,2,0x102c8880); return false; }
        const auto target=item.club(0,c.r(29)); c.set(4,c.u32(c.r(29))); c.set(18,target);
        if(compare_logical(c,c.u32(manager+0x24))!=0) {
            const auto club=c.u32(manager+0x24); c.set(4,extended(c.i16(target+0x1ec))); c.set(3,c.r(4));
            if(compare(c,c.i16(target+0x1ec),c.i16(club+0x1ec))<0) { update(1,2,0x102c882c); return true; }
        }
        update(1,1,0x102c884c);
        program.call(0x101b0100,0x102c8860,{c.u32(c.toc(-0x79f0)),target,manager});
        return false;
    }
    void competition_action() const {
        Program program{c};
        if(is_type(6)&&ordinary_club()) {
            const auto year=item.value(3)&0xffff,day=item.value(4)&0xffff,requester=item.value(2);
            keep(c,{{13,requester},{14,year},{15,day}}); c.set(4,item.value(3));
            program.call(0x10715880,0x102c88c4,{frame.at(0x94)});
            program.call(0x107156b0,0x102c88d4,{frame.at(0x7c),extended(std::int16_t(year)),day});
            copy_action_date(c,frame.at(0x94),frame.at(0x7c));
            const auto fixture=program.call(0x10497570,0x102c8904,{c.u32(c.r(28)),frame.at(0x94),c.u32(c.r(29))+requester*0x248});
            c.put8(fixture+0x18,1);
            program.call(0x104942d0,0x102c891c,{c.u32(c.r(28)),fixture,1}); return;
        }
        if(is_type(9)&&ordinary_club()) {
            program.call(0x10715880,0x102c893c,{frame.at(0x8c)});
            const auto club=item.club(1,c.r(29)),year=item.value(5)&0xffff,day=item.value(6)&0xffff;
            const auto mode=item.byte(2); keep(c,{{14,club},{17,std::uint32_t(mode)}});
            leave(c,{{6,item.value(1)*0x248},{7,c.u32(c.r(29))},{8,year}});
            program.call(0x107156b0,0x102c8970,{frame.at(0x74),extended(std::int16_t(year)),day});
            copy_action_date(c,frame.at(0x8c),frame.at(0x74));
            const auto fixture=program.call(0x104976b0,0x102c8998,{c.u32(c.r(28)),std::uint32_t(mode),frame.at(0x8c)});
            c.set(17,fixture); std::uint32_t index=1; c.set(5,index);
            while(compare(c,std::int8_t(index),c.i16(fixture+0x14))<0) {
                if(compare_logical(c,c.u32(fixture+std::uint32_t(std::int8_t(index))*4+0x18),club)==0) {
                    c.set(13,index); c.set(4,1); c.put8(fixture+std::uint32_t(std::int8_t(index))+0x28,1); break;
                }
                c.set(5,++index);
            }
            program.call(0x10493ac0,0x102c89fc,{c.u32(c.r(28)),fixture,0,club});
            // If no membership matched, r13 deliberately remains incoming.
            update(3,std::uint32_t(std::int32_t(c.i8(fixture+std::uint32_t(std::int8_t(c.r(13)))+0x28))),0x102c8a24); return;
        }
        if(is_type(22)&&ordinary_club()) { update(0,1,0x102c8a58); return; }
        if(is_type(24)&&ordinary_club()) {
            const auto club=item.club(0,c.r(29)),table=c.u32(c.toc(-0x7b5c)); c.set(7,table);
            const auto competition=c.u32(table+c.u32(c.u32(club+0x30))*4);
            program.call(0x104acee0,0x102c8aa8,{c.u32(c.r(27)),competition,0,1});
            update(1,1,0x102c8ac4); return;
        }
        if(is_type(25)&&ordinary_club()) {
            program.call(0x104ae160,0x102c8b00,{c.u32(c.r(27)),item.club(0,c.r(29)),std::uint32_t(item.byte(2)),0});
            update(3,1,0x102c8b1c); update(6,0xffffffff,0x102c8b38); return;
        }
        if(is_type(30)&&!ordinary_club()) { national_selection(); return; }
        if(is_type(34)&&!ordinary_club()) {
            const auto nations=c.u32(c.toc(-0x7b98)),people=c.u32(c.r(26));
            program.call(0x102b8850,0x102c8dc8,{c.u32(c.r(30)),nations+item.value(11)*0x128,
                std::uint32_t(item.byte(10)),std::uint32_t(item.byte(9)),people+item.value(0)*0x70});
            update(7,1,0x102c8de4);
        }
        if(is_type(40)) update(0,3,0x102c8e0c);
    }
    void national_selection() const;
    void staff_action() const;
    void transfer_action() const;
};

void Effects::national_selection() const {
    Program program{c};
    program.call(0x10738c00,0x102c8b58,{frame.at(0xa4)});
    const auto club=item.club(0,c.r(29)); const auto mode=item.byte(1);
    c.put16(frame.at(0x6e),item.value(6)); c.put16(frame.at(0x6c),item.value(7)); c.put8(frame.at(0x68),0);
    keep(c,{{13,club},{17,std::uint32_t(mode)},{24,0},{22,0xffffffff},{21,0xffffffff},
            {20,1},{19,frame.at(0xa4)},{18,2}});
    const std::array<std::uint32_t,11> arguments{c.u32(club),0xffffffff,0,0xffffffff,0xffffffff,
        1,frame.at(0xa4),2,0xfffffffe,1,1};
    for(unsigned n=0;n<arguments.size();++n)c.put32(frame.at(0x38+n*4),arguments[n]);
    program.call(0x1048b4e0,0x102c8c08,{c.u32(c.toc(-0x7a60)),frame.at(0x6c),frame.at(0x6e),
        frame.at(0x68),1,0xffffffff,0xffffffff,0xffffffff});
    if(compare(c,c.i16(frame.at(0xa4)))==0) {
        assertion(c,frame,0x102c8c28,0xb4,0x84,0x198b); c.set(4,c.toc(slot::success));
        program.call(0x10738c20,0x102c8c74,{frame.at(0xa4),0xffffffff}); return;
    }
    c.put32(frame.at(0xb0),c.u32(frame.at(0xa8)));
    const auto selected=program.call(0x10738e00,0x102c8c88,{frame.at(0xa4)}); c.set(14,selected);
    bool initialize=compare(c,(c.u8(selected+0x4f)>>6)&3)!=0;
    if(!initialize) initialize=compare(c,std::int8_t(program.call(0x100ce250,0x102c8ca0,{club})))==0;
    const auto initialize_squad=[&](std::uint32_t squad,std::uint32_t check_back,std::uint32_t create_back,std::uint32_t fill_back) {
        const auto set=program.call(0x102af620,check_back,{c.u32(c.r(30)),c.u32(club+0x2c),squad});
        if(compare(c,std::int8_t(set))==0) {
            program.call(0x102acf40,create_back,{c.u32(c.r(30)),c.u32(club+0x2c),squad,c.toc(slot::today),selected,1});
            program.call(0x102acf10,fill_back,{c.u32(c.r(30)),c.u32(club+0x2c),squad});
        }
    };
    if(initialize) initialize_squad(0,0x102c8cb8,0x102c8cdc,0x102c8cec);
    const auto squad=program.call(0x102ae9c0,0x102c8cfc,{c.u32(c.r(30)),club,std::uint32_t(mode)}); c.set(17,squad);
    if(compare(c,std::int8_t(squad))!=0) initialize_squad(squad,0x102c8d18,0x102c8d3c,0x102c8d4c);
    update(2,1,0x102c8d68);
    program.call(0x10738c20,0x102c8d74,{frame.at(0xa4),0xffffffff});
}

void Effects::staff_action() const {
    Program program{c};
    if(is_type(3007)&&ordinary_club()) {
        const auto person=c.u32(c.r(26))+item.value(2)*0x70; c.set(13,person); c.set(3,c.u32(c.r(26)));
        if(compare_logical(c,c.u32(person+0x14))!=0) {
            const auto contract=program.call(0x10082f20,0x102c8e60,{c.u32(c.r(25)),person}); c.set(17,contract);
            if(compare(c,contract)!=0) {
                const auto employer=c.u32(c.r(29))+c.u32(contract+4)*0x248;
                const auto value=program.call(0x10084be0,0x102c8e90,{c.u32(c.r(25)),person,employer,0,0xffffffff,0});
                c.put32(frame.at(0x108),value^0x80000000); c.put32(frame.at(0x104),0x43300000);
                Fpu fp(c);
                c.set_f(2,c.f64(c.r(22)+0x18)); c.set_f(1,c.f64(frame.at(0x104))); c.set_f(0,c.f64(c.r(22)));
                c.set_f(1,fp.fsub(c.f(1),c.f(2))); c.set_f(31,fp.fmul(c.f(0),c.f(1)));
                const auto requested=program.call(0x10085860,0x102c8ee0,
                    {c.u32(c.r(25)),c.u32(c.r(29))+c.u32(contract+4)*0x248,0,person,0,1,1});
                c.put32(frame.at(0xfc),0x43300000); c.set_f(1,c.f64(c.r(22)+0x18));
                c.put32(frame.at(0x100),requested^0x80000000); c.set_f(0,c.f64(frame.at(0xfc)));
                c.set_f(0,fp.fsub(c.f(0),c.f(1))); fp.fcmp(0,c.f(0),c.f(31));
                if(c.cr(0)&8) {
                    update(8,1,0x102c8f20);
                    if(compare(c,c.u8(contract+0x5c)&2)!=0) c.put8(contract+0x5c,c.u8(contract+0x5c)&~2u);
                    c.put8(contract+0x5c,c.u8(contract+0x5c)|8);
                    program.call(0x100d3570,0x102c8f54,{person,12,0});
                    program.call(0x10363ca0,0x102c8f68,{c.u32(c.u32(c.r(24))+0x988),person,1});
                    program.call(0x1008cd20,0x102c8f84,{c.u32(c.r(25)),person,c.u32(c.r(29))+c.u32(contract+4)*0x248});
                } else {
                    update(8,0,0x102c8fa4);
                    // The fivefold player attribute is narrowed AFTER the
                    // subtraction. Signed-byte multiplication is different.
                    const auto adjustment=std::int8_t(c.u8(person+0x65)*5-200);
                    c.set(4,std::uint32_t(std::int32_t(adjustment)));
                    auto reaction=std::int32_t(c.i8(contract+0x4c))+adjustment;
                    if(compare(c,reaction,100)>0) reaction=100;
                    else if(compare(c,reaction,-100)<0) reaction=-100;
                    c.put8(contract+0x4c,reaction);
                    program.call(0x100d3570,0x102c9014,{person,0xfffffff4,0});
                    const auto bound=std::int32_t(c.i8(manager+0x65))*20+c.i8(contract+0x4c)+200;
                    c.set(14,std::uint32_t(bound));
                    const SamplePool pool{c,c.r(20),c.toc(-0x7b8c),c.toc(-0x7b88),c.r(21)};
                    if(compare(c,bound)==0) c.set(13,0);
                    else {
                        // All signed-byte pairs yield [-2488,2867], so the
                        // original's wide FP clone and sampler self-call are
                        // unreachable here. Preserve the small signed draw.
                        pool.advance(0x102c9064,true);
                        compare(c,bound,65535); compare(c,bound,-65535);
                        const auto position=pool.position(); compare(c,bound);
                        const auto bias=pool.bias();
                        c.set(13,oriented_remainder(std::uint32_t(c.u16(position))+bias,bound));
                    }
                    pool.advance(0x102c91c8,true);
                    const auto position=pool.position(),bias=pool.bias();
                    const auto sum=std::uint32_t(c.u16(position))+bias;
                    const auto high=std::int32_t(std::uint64_t(std::int64_t(std::int32_t(sum))*0x51eb851f)>>32);
                    c.s.wr<1>(reg::xer_ca,high<0&&(std::uint32_t(high)&15));
                    c.set(3,std::int32_t(sum)<0?1:0); c.set(4,sum);
                    if(compare(c,c.r(13),remainder_word(sum,50))<0)
                        program.call(0x1037fa80,0x102c921c,{contract,0x10000,0});
                }
            }
        }
        update(7,1,0x102c9238);
    } else if(is_type(3009)&&ordinary_club()) {
        const auto contract=program.call(0x10082f20,0x102c9268,
            {c.u32(c.r(25)),c.u32(c.r(26))+item.value(2)*0x70});
        if(compare_logical(c,contract)!=0) { c.set(4,3); c.put8(contract+0x60,(c.u8(contract+0x60)&0xf0)|3); }
        update(6,1,0x102c929c); update(9,0xffffffff,0x102c92b8);
    } else if(is_type(3010)&&ordinary_club()) update(7,1,0x102c92ec);
    if(!is_type(3000)||!ordinary_club()) return;
    const auto offer_id=item.value(8),person_id=item.value(0),club_id=item.value(4); const auto mode=item.byte(10);
    keep(c,{{13,person_id},{14,club_id}}); c.set(4,offer_id);
    if(compare(c,offer_id,-1)!=0) {
        const auto slot=program.call(0x103eb440,0x102c9324,{c.u32(c.r(24)),offer_id});
        leave(c,{{7,c.u32(c.r(29))}});
        program.call(0x103e6cb0,0x102c9348,{c.u32(c.r(24)),c.u32(c.r(29))+club_id*0x248,
            c.u32(c.r(26))+person_id*0x70,c.u32(slot)}); return;
    }
    if(compare(c,mode,5)==0) {
        program.call(0x104ad840,0x102c9374,{c.u32(c.r(27)),item.club(4,c.r(29)),0}); update(11,1,0x102c9390); return;
    }
    c.set_f(1,c.f64(c.r(22)+0x10)); c.set(10,2);
    program.call(0x10080d30,0x102c93bc,{c.u32(c.r(25)),c.u32(c.r(26))+person_id*0x70,0,0,0xffffffff});
}

void Effects::transfer_action() const {
    Program program{c};
    const auto resolve_attempt=[&](std::uint32_t slot,std::uint32_t back,std::uint32_t reject_back,std::uint32_t accept_back,
                                    std::uint32_t staff,std::uint32_t employer,std::uint32_t destination) {
        c.put32(frame.at(0x38),1);
        const auto accepted=program.call(0x1036f170,back,{c.u32(c.u32(c.r(24))+0x988),employer,staff,0,0,destination,0xffffffff,0xffffffff});
        if(compare(c,std::int8_t(accepted))==0) program.call(0x103e4c90,reject_back,{c.u32(c.r(24)),slot,0});
        else program.call(0x103e4390,accept_back,{c.u32(c.r(24)),slot,0,0});
    };
    if(is_type(4000)&&ordinary_club()) {
        const auto slot=program.call(0x103eb440,0x102c93f4,{c.u32(c.r(24)),item.value(6)}); c.set(13,slot); c.set(3,c.u32(slot));
        if(compare(c,c.u8(c.r(3)+0x24),1)==0) {
            const auto payload=c.u32(slot),staff=c.u32(c.r(26))+c.u32(payload+0xc)*0x70,
                       destination=c.u32(c.r(29))+c.u32(payload+8)*0x248;
            keep(c,{{15,staff},{14,destination}});leave(c,{{5,c.u32(payload+8)*0x248},{6,c.u32(c.r(29))}});
            const auto contract=program.call(0x10082f20,0x102c943c,{c.u32(c.r(25)),staff});
            c.set(4,c.toc(-0x7b78)); const auto present=compare_logical(c,contract); c.put32(c.r(4),contract);
            std::uint32_t employer=0;
            if(present!=0) { c.set(3,c.u32(c.toc(-0x7b78))); employer=c.u32(c.r(29))+c.u32(c.r(3)+4)*0x248; }
            c.set(4,employer);
            resolve_attempt(slot,0x102c9498,0x102c94b0,0x102c94c8,staff,employer,destination);
        } else program.call(0x103ee1b0,0x102c94dc,{c.u32(c.r(24)),slot,0});
        return;
    }
    if(is_type(4002)&&ordinary_club()) {
        const auto slot=program.call(0x103eb440,0x102c9500,{c.u32(c.r(24)),item.value(6)});
        program.call(0x103e4d70,0x102c9510,{c.u32(c.r(24)),slot,0}); return;
    }
    if(is_type(4012)&&ordinary_club()) {
        const auto person_id=item.value(0),club_id=item.value(5); keep(c,{{13,person_id},{14,club_id}});
        const auto slot=program.call(0x103eb440,0x102c953c,{c.u32(c.r(24)),item.value(7)}); c.set(7,c.u32(c.r(29)));
        program.call(0x103e6cb0,0x102c9560,{c.u32(c.r(24)),c.u32(c.r(29))+club_id*0x248,
            c.u32(c.r(26))+person_id*0x70,c.u32(slot)}); return;
    }
    if(is_type(4004)&&ordinary_club()) {
        const auto slot=program.call(0x103eb440,0x102c9584,{c.u32(c.r(24)),item.value(7)});
        c.set(3,c.u32(slot)); c.put8(c.r(3)+0x36,3); return;
    }
    if(is_type(4005)&&ordinary_club()) {
        const auto club_id=item.value(16); c.set(14,club_id);
        const auto slot=program.call(0x103eb440,0x102c95b8,{c.u32(c.r(24)),item.value(6)}); c.set(13,slot);
        const auto result=program.call(0x103f9d60,0x102c95e0,{c.u32(c.r(24)),c.u32(slot),c.u32(c.r(29))+club_id*0x248,frame.at(0x6a),1});
        if(compare(c,std::int8_t(result),1)==0)
            program.call(0x103e8500,0x102c9608,{c.u32(c.r(24)),c.u32(slot),c.u32(c.r(29))+club_id*0x248,extended(c.i16(frame.at(0x6a)))});
        else program.call(0x103e80e0,0x102c9624,{c.u32(c.r(24)),c.u32(slot),c.u32(c.r(29))+club_id*0x248});
        return;
    }
    if(is_type(4008)&&ordinary_club()) {
        program.call(0x104ad840,0x102c964c,{c.u32(c.r(27)),c.r(17),0}); update(7,1,0x102c9668);
    }
    if(is_type(4011)&&ordinary_club()) {
        const auto slot=program.call(0x103eb440,0x102c9688,{c.u32(c.r(24)),item.value(9)}); c.set(13,slot);
        update(7,1,0x102c96ac,false); update(8,0,0x102c96c8,false); c.set(3,c.u32(slot));
        if(compare(c,c.u8(c.r(3)+0x34),17)==0) program.call(0x103e44b0,0x102c96ec,{c.u32(c.r(24)),slot,0,6});
        else program.call(0x103e4130,0x102c9704,{c.u32(c.r(24)),slot,0,6});
    }
    if(is_type(4020)&&ordinary_club()) {
        const auto slot=program.call(0x103eb440,0x102c9724,{c.u32(c.r(24)),item.value(11)});
        program.call(0x103e44b0,0x102c9738,{c.u32(c.r(24)),slot,0,0}); return;
    }
    if(is_type(4021)&&ordinary_club()) {
        const auto club_id=item.value(5); c.set(14,club_id);
        const auto slot=program.call(0x103eb440,0x102c9760,{c.u32(c.r(24)),item.value(6)}); c.set(13,slot);
        const auto payload=c.u32(slot),staff=c.u32(c.r(26))+c.u32(payload+0xc)*0x70,
                   destination=c.u32(c.r(29))+c.u32(payload+8)*0x248;
        resolve_attempt(slot,0x102c97bc,0x102c97d4,0x102c97ec,staff,c.u32(c.r(29))+club_id*0x248,destination); return;
    }
    if(is_type(4022)&&ordinary_club()) {
        program.call(0x104ad840,0x102c9820,{c.u32(c.r(27)),item.club(4,c.r(29)),0}); update(7,1,0x102c983c); return;
    }
    if(is_type(4032)) {
        const auto slot=program.call(0x103eb440,0x102c9858,{c.u32(c.r(24)),item.value(6)}); c.set(4,slot);
        if(compare(c,slot)!=0) program.call(0x103e5810,0x102c9868,{c.u32(c.r(24)),slot});
    }
}
}

void perform_action(Cpu& c,std::uint32_t item,std::uint32_t manager,std::uint32_t club) {
    Frame frame(c,0x102c8690,0x170,{.first_gpr=13,.gpr_offset=-0x5c,.first_fpr=31,.fpr_offset=-8});
    keep(c,{{14,c.toc(slot::message)},{15,item},{16,manager},{17,club},{18,c.toc(-0x7a54)},
            {20,c.toc(-0x7b90)},{21,c.toc(-0x7b84)},{22,c.toc(-0x510c)},{23,c.toc(-0x7bf0)},
            {24,c.toc(-0x7b40)},{25,c.toc(-0x7b7c)},{26,c.toc(-0x7be0)},{27,c.toc(-0x7a98)},
            {28,c.toc(-0x7b54)},{29,c.toc(-0x7be8)},{30,c.toc(-0x7a64)},{31,c.toc(slot::pool)}});
    if(compare(c,club)!=0) Program{c}.call(0x100c7d00,0x102c86f4,{club}); else c.set(3,0);
    const auto present=compare_logical(c,manager); c.set(19,c.r(3));
    if(present==0) { assertion(c,frame,0x102c871c,0xd4,0x9c,0x18d3); c.set(3,c.toc(slot::success)); return; }
    enter(c,0x102c8778,{item,manager,c.toc(-0x7bb8),2000,frame.at(0x70)});
    c.set(3,action_label(c,item,manager,c.toc(-0x7bb8),2000,frame.at(0x70)));
    if(compare(c,std::int8_t(c.r(3)),2)!=0) return;
    Effects action{c,frame,{c,item},manager};
    if(action.in_category(6,0x102c878c)&&action.is_type(6004)&&action.employment()) return;
    if(action.in_category(2,0x102c8888)) {action.competition_action();return;}
    if(action.in_category(4,0x102c8e18)) {action.staff_action();return;}
    if(action.in_category(5,0x102c93c8)) action.transfer_action();
}
}
