#include "contract_manager_random.hpp"

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
bool review_byte(Cpu& c, std::uint32_t value) {
    const auto byte=std::int8_t(value);c.set(0,std::uint32_t(std::int32_t(byte)));return nonzero(c,byte);
}
std::int32_t pending_role(Cpu& c,std::uint32_t record) {
    const auto flags=c.u8(record+0x38);const auto role=std::int32_t(std::int8_t(flags))>>2;
    c.set(0,std::uint32_t(role));c.s.wr<1>(guest::reg::xer_ca,role<0&&(flags&2));return role;
}
double pending_encoded(Cpu& c,const Frame& frame,unsigned slot,std::uint32_t value) {
    c.put32(frame.at(slot),0x43300000);c.put32(frame.at(slot+4),value^0x80000000u);return c.f64(frame.at(slot));
}
std::uint32_t pending_employer(Cpu& c,Program& p,std::uint32_t managers,std::uint32_t person,
                                std::uint32_t scratch,std::uint32_t clubs,std::uint32_t back,unsigned out) {
    const auto contract=p.call(0x10082f20,back,{c.u32(managers),person});
    const bool exists=nonzero_logical(c,contract);c.put32(scratch,contract);std::uint32_t employer=0;
    if(exists){const auto id=c.u32(contract+4),base=c.u32(clubs);c.set(0,id*0x248);c.set(3,base);employer=base+id*0x248;}
    c.set(out,employer);return employer;
}
void decrease_pending_count(Cpu& c,std::uint32_t person,std::uint32_t indices,bool last=false) {
    const auto id=c.u32(person),base=c.u32(indices),offset=id*100+8;
    c.set(0,id);c.set(last?6:5,base);c.set(3,id*100);c.set(last?5:4,offset);
    const auto count=c.u8(base+offset);c.set(3,count);c.set(0,std::uint32_t(count)-1);c.put8(base+offset,std::uint32_t(count)-1);
}
}

void review_pending_contracts(Cpu& c,std::uint32_t manager_address,std::uint32_t person_address) {
    Frame frame(c,0x10081bd0,0x250,{.first_gpr=13,.first_fpr=28});c.set(11,frame.caller());
    Kept<27> constants(c,c.toc(-0x571c));Kept<15> object(c,manager_address);Kept<22> transfers(c,c.toc(-0x7b40));
    Kept<16> person(c,person_address);Kept<23> national(c,c.toc(-0x7b28));Kept<24> managers(c,c.toc(-0x7b7c));
    Kept<20> best(c,0);Kept<25> scratch(c,c.toc(-0x7b78));Kept<26> clubs(c,c.toc(-0x7be8));
    Kept<14> cursor_or_node(c,c.toc(-0x7b84));Kept<28> indices(c,c.toc(-0x7b9c));Kept<29> errors(c,c.toc(-0x7ba8));
    Kept<30> today(c,c.toc(-0x7b80));Kept<31> pool(c,c.toc(slot::pool));
    Kept<19> fallback(c,c.r(19));Kept<13> refused(c,c.r(13));Kept<17> decision(c,c.r(17));
    Kept<18> current_or_node(c,c.r(18));Kept<21> squad_role(c,c.r(21));Program p(c);guest::Fpu fp(c);
    c.set_f(31,c.f64(constants+0x140));
    p.call(0x103721f0,0x10081c2c,{frame.at(0x124)});p.call(0x10715880,0x10081c34,{frame.at(0x18c)});
    fallback=0;p.call(0x10738c00,0x10081c40,{frame.at(0x90)});p.call(0x10738c00,0x10081c48,{frame.at(0x80)});
    p.call(0x103721f0,0x10081c50,{frame.at(0xc0)});refused=0;p.call(0x10738c00,0x10081c5c,{frame.at(0x70)});
    const auto temperament=c.i8(person+0x67),adaptability=c.i8(person+0x63),ambition=c.i8(person+0x65);
    c.set(6,std::uint32_t(std::int32_t(temperament)));c.set(5,std::uint32_t(std::int32_t(adaptability)));c.set(3,std::uint32_t(std::int32_t(ambition)));c.set(4,person);
    const auto negative=std::uint32_t(-9)-std::uint32_t(std::int32_t(adaptability));
    c.set(5,negative);c.s.wr<1>(guest::reg::xer_ca,std::uint32_t(-9)>=std::uint32_t(std::int32_t(adaptability)));
    const auto traits=negative+std::uint32_t(std::int32_t(temperament)+ambition);c.set(0,traits^0x80000000u);
    c.put32(frame.at(0x1d4),0x43300000);const auto bias=c.f64(constants+8),divisor=c.f64(constants+0x148);
    c.set_f(2,bias);c.set_f(0,divisor);c.put32(frame.at(0x1d8),traits^0x80000000u);
    const auto converted=fp.fsub(c.f64(frame.at(0x1d4)),bias);c.set_f(1,converted);c.set_f(28,fp.fdiv(converted,divisor));decision=0;
    const auto cleanup=[&](std::uint32_t first) {
        for(unsigned n=0;n<3;++n)p.call(0x10738c20,first+n*12,{frame.at(0x70+n*16),0xffffffff});
        c.set(11,frame.caller());
    };
    if(review_byte(c,p.call(0x103f4c00,0x10081cb4,{c.u32(transfers),person}))) {
        p.call(0x10717890,0x10081ccc,{frame.at(0x50),today,2});
        const auto phase=c.u8(c.toc(-0x79ec));const auto day=c.i16(frame.at(0x50));
        const auto year=c.u16(frame.at(0x52)),leap=std::uint16_t(c.u8(frame.at(0x54)));
        c.set(0,phase);c.set(10,extended(day));c.set(9,year);c.set(8,leap);
        c.put16(frame.at(0x60),extended(day));c.put16(frame.at(0x62),year);c.put8(frame.at(0x64),leap);
        p.call(0x100808d0,0x10081d04,{object,person,frame.at(0x60),std::uint32_t(std::int32_t(std::int8_t(phase))),3});
        cleanup(0x10081d10);return;
    }
    const auto list=c.u32(object+0x34);c.set(3,list);
    if(nonzero_logical(c,list)) {
        const auto head=c.u32(list+4);c.set(0,head);c.put32(list+12,head);
        for(;;) {
            const auto row=p.call(0x10738e00,0x10082314,{c.u32(object+0x34)});
            const bool present=nonzero_logical(c,row);c.put32(frame.at(0x44),row);if(!present)break;
            const auto id=c.i32(person),row_id=c.i32(row);c.set(4,row);c.set(0,std::uint32_t(id));c.set(3,std::uint32_t(row_id));
            if(compare(c,row_id,id)!=0)continue;
            const auto active=c.i8(row+0x79);c.set(0,std::uint32_t(std::int32_t(active)));if(!nonzero(c,active))continue;
            const auto origin=c.i32(row+0x70);c.set(0,std::uint32_t(origin));
            if(compare(c,origin,-1)==0) {
                const auto employer=pending_employer(c,p,managers,person,scratch,clubs,0x10081d7c,5);
                const auto current=c.u32(frame.at(0x44)),base=c.u32(clubs),club_id=c.u32(current+4);
                c.set(4,current);c.set(3,base);c.set(0,club_id*0x248);const auto target=base+club_id*0x248;c.set(0,target);
                if(compare_logical(c,employer,target)!=0&&compare(c,pending_role(c,current),5)!=0) {
                    p.call(0x10738e60,0x10081ddc,{c.u32(object+0x34)});decrease_pending_count(c,person,indices);continue;
                }
            }
            const auto current=c.u32(frame.at(0x44));c.set(3,current);const auto state=c.i8(current+0x64);c.set(4,c.u8(current+0x64));c.set(0,std::uint32_t(std::int32_t(state)));
            if(nonzero(c,state)){c.set(0,std::uint32_t(std::int32_t(state)));if(compare(c,state,2)==0)decision=1;continue;}
            const auto role=pending_role(c,current);
            const auto target_club=[&] {return c.u32(clubs)+c.u32(c.u32(frame.at(0x44))+4)*0x248;};
            if(compare(c,role,5)==0||compare(c,role,6)==0) {
                std::uint32_t wanted_role=8;
                if(role==6)wanted_role=p.call(0x1036d510,0x10081eb8,{c.u32(c.u32(transfers)+0x988),person,target_club(),0});
                p.call(0x10369080,role==5?0x10081e54:0x10081eec,
                       {c.u32(c.u32(transfers)+0x988),person,target_club(),0,1,wanted_role,0});
                const auto score=c.f(1);fp.fcmp(0,score,c.f(31));c.put_f64(frame.at(0x68),score);
                bool chosen=score>c.f(31);
                if(chosen){const auto threshold=c.f64(constants+0x150);c.set_f(0,threshold);fp.fcmp(0,score,threshold);c.set_cr(0,c.cr(0)|((c.cr(0)&4)?2:0));chosen=score>=threshold;}
                if(chosen){c.set_f(31,score);best=c.u32(frame.at(0x44));}
                else {const auto zero=c.f64(constants+0x60);c.set_f(0,zero);c.put_f64(frame.at(0x68),zero);}
                c.set_f(30,c.f64(constants+0x60));
            } else {
                const auto source=c.u32(current+0x70);c.set(4,source);squad_role=c.u8(current+0x60)&15u;
                if(compare(c,std::int32_t(source),-1)!=0) {
                    const auto offer=p.call(0x103eb440,0x10081f40,{c.u32(transfers),source});
                    const auto record=c.u32(offer);const auto item=p.call(0x10417260,0x10081f4c,{record,person});
                    if(nonzero_logical(c,item))c.put8(item+5,squad_role);
                }
                const auto kind=c.u8(c.u32(frame.at(0x44))+0x78);
                p.call(0x10374f50,0x10081f8c,{frame.at(0xc0),person,target_club(),0xffffffff,squad_role,kind,0,0});
                const auto base=p.call(0x1037e660,0x10081f9c,{frame.at(0xc0),person,frame.at(0xc0)});
                c.set(3,base^0x80000000u);c.set(0,0x43300000);const auto raw=pending_encoded(c,frame,0x1d4,base);
                const auto conversion_bias=c.f64(constants+8),zero=c.f64(constants+0x60);c.set_f(2,conversion_bias);c.set_f(0,zero);c.set_f(1,raw);
                const auto baseline=fp.fsub(raw,conversion_bias);c.set_f(30,baseline);fp.fcmp(0,baseline,zero);
                if(!(baseline>zero))c.put_f64(frame.at(0x68),zero);
                else {
                    const auto assessed=p.call(0x1037e660,0x10081fd4,{c.u32(frame.at(0x44)),person,frame.at(0xc0)});
                    c.set(3,assessed^0x80000000u);c.set(0,0x43300000);const auto amount=pending_encoded(c,frame,0x1d4,assessed),local_bias=c.f64(constants+8);c.set_f(1,local_bias);c.set_f(0,amount);
                    const auto valuation=fp.fsub(amount,local_bias);c.set_f(0,valuation);const auto ratio=fp.fdiv(valuation,c.f(30));c.set_f(0,ratio);
                    const auto pressure=fp.fsub(ratio,c.f(28));c.set_f(0,pressure);c.put_f64(frame.at(0x68),pressure);
                    const auto profile=c.u32(person+0x1c);c.set(0,profile);bool mixed=false;
                    if(nonzero_logical(c,profile)){const auto k=c.u8(c.u32(frame.at(0x44))+0x78);c.set(7,k);mixed=compare(c,k,11)!=0;}
                    if(mixed) {
                        const auto k=c.u8(c.u32(frame.at(0x44))+0x78);
                        p.call(0x10366470,0x10082040,{c.u32(c.u32(transfers)+0x988),person,target_club(),0,k,0});c.set_f(29,c.f(1));
                        if(review_byte(c,p.call(0x100c7a20,0x10082050,{c.u8(c.u32(frame.at(0x44))+0x78)}))) {
                            p.call(0x10369080,0x1008208c,{c.u32(c.u32(transfers)+0x988),person,target_club(),0,0,squad_role,0});
                            const auto sum=fp.fadd(c.f(29),c.f(1));c.set_f(29,sum);const auto half=c.f64(constants+0xb8);c.set_f(0,half);c.set_f(29,fp.fmul(sum,half));
                        }
                    } else {p.call(0x10369080,0x100820d0,{c.u32(c.u32(transfers)+0x988),person,target_club(),0,0,squad_role,0});c.set_f(29,c.f(1));}
                    const auto employer=pending_employer(c,p,managers,person,scratch,clubs,0x100820e0,0);
                    bool same=false;
                    if(nonzero_logical(c,employer)) {
                        const auto repeated=pending_employer(c,p,managers,person,scratch,clubs,0x10082118,4);
                        const auto recorded=c.i32(c.u32(frame.at(0x44))+4),current_club=c.i32(repeated);c.set(3,std::uint32_t(recorded));c.set(0,std::uint32_t(current_club));same=compare(c,recorded,current_club)==0;
                    }
                    if(same) {
                        current_or_node=1;const auto score=c.f64(frame.at(0x68)),threshold=c.f64(constants+0x158);c.set_f(1,score);c.set_f(0,threshold);fp.fcmp(0,score,threshold);c.set_cr(0,c.cr(0)|((c.cr(0)&4)?2:0));
                        if(score>=threshold){const auto minimum=c.f64(constants+0x150);c.set_f(0,minimum);fp.fcmp(0,c.f(29),minimum);if(!(c.f(29)>minimum))c.set_f(29,minimum);}
                    } else current_or_node=0;
                    const auto score=c.f64(frame.at(0x68)),minimum=c.f64(constants+0x160);c.set_f(1,score);c.set_f(0,minimum);fp.fcmp(0,score,minimum);
                    if(score<minimum){const auto z=c.f64(constants+0x60);c.set_f(0,z);c.put_f64(frame.at(0x68),z);}
                }
            }
            if(review_byte(c,p.call(0x100c7a00,0x100821b0,{c.u8(c.u32(frame.at(0x44))+0x78)}))) {
                const auto profile=c.u32(person+0x1c);c.set(0,profile);
                if(!nonzero_logical(c,profile)&&!review_byte(c,p.call(0x102dbe90,0x100821cc,{c.u32(national)}))){const auto z=c.f64(constants+0x60);c.set_f(0,z);c.put_f64(frame.at(0x68),z);}
            }
            const auto score=c.f64(frame.at(0x68)),half=c.f64(constants+0xb8);c.set_f(1,score);c.set_f(0,half);fp.fcmp(0,score,half);
            if(score>half&&review_byte(c,p.call(0x103ecef0,0x10082210,{c.u32(transfers),person,target_club(),0}))) {
                const auto days=p.call(0x10718130,0x1008222c,{c.u32(frame.at(0x44))+0x30,c.u32(today),c.u32(today+4)});
                if(compare(c,std::int32_t(days),90)<0){const auto z=c.f64(constants+0x60);c.set_f(0,z);c.put_f64(frame.at(0x68),z);}
            }
            p.call(0x10738d10,0x1008224c,{frame.at(0x90),frame.at(0x44),4});p.call(0x10738d10,0x1008225c,{frame.at(0x80),frame.at(0x68),8});
            const auto zero=c.f64(constants+0x60);c.set_f(0,zero);fp.fcmp(0,zero,c.f(30));if(zero==c.f(30))continue;
            const auto candidate=c.f64(frame.at(0x68)),upper=c.f64(constants+0x168);c.set_f(1,candidate);c.set_f(0,upper);fp.fcmp(0,candidate,upper);c.set_cr(0,c.cr(0)|((c.cr(0)&4)?2:0));
            bool eligible=candidate>=upper;
            if(!eligible){const auto middle=c.f64(constants+0x170);c.set_f(0,middle);fp.fcmp(0,candidate,middle);if(candidate>middle){const auto level=c.f64(constants+0x150);c.set_f(0,level);fp.fcmp(0,c.f(29),level);eligible=c.f(29)>level;}}
            if(!eligible){const auto lower=c.f64(constants+0x178);c.set_f(0,lower);fp.fcmp(0,candidate,lower);if(candidate>lower){const auto level=c.f64(constants+0x180);c.set_f(0,level);fp.fcmp(0,c.f(29),level);eligible=c.f(29)>level;}}
            if(eligible) {
                const auto minimum=c.f64(constants+0x150);c.set_f(0,minimum);fp.fcmp(0,c.f(29),minimum);if(!(c.f(29)>minimum))c.set_f(29,minimum);
                const auto attraction=fp.frsp(c.f(29));c.set_f(29,attraction);const auto combined=fp.fmul(candidate,attraction);c.set_f(0,combined);fp.fcmp(0,combined,c.f(31));
                if(combined>c.f(31)){c.set_f(31,combined);best=c.u32(frame.at(0x44));continue;}
            }
            c.set(0,std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(current_or_node)))));if(nonzero(c,std::int8_t(std::uint32_t(current_or_node))))fallback=c.u32(frame.at(0x44));
        }
    }
    c.set(0,std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(decision)))));
    if(nonzero(c,std::int8_t(std::uint32_t(decision))))best=0;
    else {const auto profile=c.u32(person+0x1c);c.set(0,profile);if(nonzero_logical(c,profile)&&review_byte(c,p.call(0x101b1620,0x1008234c,{c.u32(c.toc(-0x79f0)),person})))best=0;}
    c.set(3,c.u32(frame.at(0x94)));c.set(0,c.u32(frame.at(0x84)));c.put32(frame.at(0x9c),c.r(3));c.put32(frame.at(0x8c),c.r(0));
    for(;;) {
        current_or_node=p.call(0x10738e00,0x10082428,{frame.at(0x90)});if(!nonzero(c,std::int32_t(std::uint32_t(current_or_node))))break;
        const auto cell=p.call(0x10738e00,0x10082374,{frame.at(0x80)});const auto value=c.f64(cell);c.set_f(0,value);c.put_f64(frame.at(0x68),value);
        const auto row=c.u32(current_or_node);c.set(3,row);const bool chosen=compare_logical(c,row,best)==0;
        c.set(0,std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(decision)))));
        if(chosen&&!nonzero(c,std::int8_t(std::uint32_t(decision)))) {
            c.set(0,2);c.put8(row+0x64,2);p.call(0x10698e80,0x100823a8,{c.u32(c.toc(-0x7b94)),person});p.call(0x103ea250,0x100823b4,{c.u32(transfers),person});
        } else {
            c.set(6,1);c.put8(row+0x64,1);const auto again=c.u32(current_or_node);const auto origin=c.i32(again+0x70);c.set(4,again);c.set(5,std::uint32_t(origin));
            if(compare(c,origin,-1)!=0)p.call(0x103e76c0,0x100823e0,{c.u32(transfers),person,std::uint32_t(origin),best});
            else {
                const auto order=compare_logical(c,fallback,again);c.set(3,object);c.set(5,0);std::uint32_t notify=0;
                if(!order){const auto attributes=c.u32(person+0x14);c.set(0,attributes);if(nonzero_logical(c,attributes)){const auto role=c.u8(fallback+0x78);c.set(0,role);if(compare(c,role,11)==0&&!nonzero_logical(c,best))notify=1;}}
                c.set(5,notify);p.call(0x1008a350,0x10082420,{object,again,notify});
            }
        }
    }
    if(nonzero_logical(c,best)) {
        p.call(0x10082c50,0x10082444,{frame.at(0x124),best});const auto state=c.u8(best+0x64);c.set(0,state);c.put8(frame.at(0x188),state);
        p.call(0x10082c10,0x1008245c,{frame.at(0x48),frame.at(0x18c),best+0x68});
        const auto origin=c.u32(best+0x70),offer=c.u32(best+0x74);c.set(0,origin);c.put32(frame.at(0x194),origin);c.set(0,offer);c.put32(frame.at(0x198),offer);
        for(unsigned offset:{0x78u,0x79u}){const auto value=c.u8(best+offset);c.set(0,value);c.put8(frame.at(0x19c+offset-0x78),value);}
        p.call(0x10082bf0,0x10082488,{frame.at(0x1a0),best+0x7c});
        const auto role=pending_role(c,frame.at(0x124));
        bool consider=compare(c,role,5)!=0;
        if(consider)consider=compare(c,role,6)!=0;
        if(consider){const auto threshold=c.f64(constants+0xc8);c.set_f(0,threshold);fp.fcmp(0,c.f(31),threshold);consider=c.f(31)>threshold;}
        if(consider) {
            const auto upper=c.f64(constants+0x190),maximum=c.f64(constants+0x188);c.set_f(0,upper);c.set_f(28,maximum);
            const auto difference=fp.fsub(upper,c.f(31));c.set_f(0,difference);fp.fcmp(0,maximum,difference);if(!(maximum>difference))c.set_f(28,difference);
            const auto zero=c.f64(constants+0x60);c.set_f(0,zero);fp.fcmp(0,c.f(28),zero);
            if(c.f(28)==zero)decision=0;
            else {
                const RandomTable random{c,c.toc(-0x7b90),c.toc(-0x7b8c),c.toc(-0x7b88),cursor_or_node};random.advance(0x1008250c);
                const auto bits=fp.fctiwz(0,c.f(28));c.put64(frame.at(0x1d4),bits);auto bound=c.i32(frame.at(0x1d8));c.set(3,std::uint32_t(bound));c.set(0,65535);
                bool wide=compare(c,bound,65535)>0;
                if(!wide){c.put64(frame.at(0x1d4),bits);c.set(3,0xffff0000);c.set(0,0xffff0001);wide=compare(c,bound,-65535)<0;}
                if(wide) {
                    c.put64(frame.at(0x1d4),fp.fctiwz(0,c.f(28)));bound=c.i32(frame.at(0x1d8));c.put64(frame.at(0x1b4),c.s.r<8>(guest::reg::fpr(0)));
                    c.set(3,std::uint32_t(bound)^0x80000000u);c.set(5,0x43300000);c.set(6,65535);c.set(0,0x80008001);
                    const auto position=random.position();c.set(7,position);const auto conversion_bias=c.f64(constants+8);c.set_f(2,conversion_bias);
                    const auto raw=pending_encoded(c,frame,0x1cc,std::uint32_t(bound)),divisor=c.f64(constants);c.set_f(0,divisor);c.set_f(1,raw);
                    const auto converted=fp.fsub(raw,conversion_bias);c.set_f(1,converted);const auto draw=random.value();c.set(7,draw);c.set(3,c.u32(random.salt));c.set(4,std::uint32_t(bound)*10);
                    const auto share=fp.fdiv(converted,divisor);c.set_f(1,share);const auto whole=divide_65535(std::int32_t(draw));const auto tenfold=std::int32_t(std::uint32_t(bound)*10);const auto coarse=divide_65535(tenfold);
                    c.set(5,whole.sign);c.set(3,(draw-std::uint32_t(whole.value)*65535)^0x80000000u);c.set(0,std::uint32_t(coarse.floor));c.s.wr<1>(guest::reg::xer_ca,coarse.floor<0&&((std::uint32_t(tenfold)+std::uint32_t(multiply_high(std::int32_t(0x80008001),tenfold)))&0x7fff));
                    const auto encoded_draw=pending_encoded(c,frame,0x1c4,draw-std::uint32_t(whole.value)*65535);c.set_f(0,encoded_draw);const auto remainder=fp.fsub(encoded_draw,conversion_bias);c.set_f(0,remainder);
                    const auto scaled=fp.fmul(share,remainder);c.set_f(0,scaled);c.put64(frame.at(0x1bc),fp.fctiwz(0,scaled));decision=c.u32(frame.at(0x1c0));c.set(3,std::uint32_t(coarse.value));
                    const auto refined=p.call(0x100788f0,0x10082604);
                    c.put64(frame.at(0x1ac),fp.fctiwz(0,c.f(28)));const auto current_bound=c.i32(frame.at(0x1b0));const auto dividend=std::int32_t(refined+5),high=multiply_high(0x66666667,dividend),floor=high>>2;
                    const auto rounded=std::uint32_t(floor)+(std::uint32_t(floor)>>31);c.set(4,std::uint32_t(current_bound)-1);c.set(3,std::uint32_t(floor)>>31);c.set(0,rounded);c.s.wr<1>(guest::reg::xer_ca,high<0&&(std::uint32_t(high)&3));
                    decision=std::uint32_t(decision)+rounded;if(compare(c,std::int32_t(std::uint32_t(decision)),std::int32_t(std::uint32_t(current_bound)-1))>0)decision=std::uint32_t(current_bound)-1;
                } else {
                    const auto position=random.position();c.set(3,position);c.put64(frame.at(0x1ac),bits);const auto sample=c.u16(position);const auto salt=c.u32(random.salt);c.set(4,std::uint32_t(bound));c.set(0,salt);c.set(5,std::uint32_t(sample)+salt);c.put64(frame.at(0x1b4),bits);
                    const auto draw=std::uint32_t(sample)+salt;const auto quotient=std::uint32_t(divide_signed<4>(draw,std::uint32_t(bound))),remainder=draw-quotient*std::uint32_t(bound);c.set(3,quotient);c.set(0,remainder);
                    decision=0u-remainder;if(compare(c,bound)>0)decision=remainder;
                }
            }
            const auto ambition=c.i8(person+0x65);c.set(0,std::uint32_t(std::int32_t(ambition)));
            if(compare(c,ambition,std::int32_t(std::uint32_t(decision)))>0) {
                const auto club_id=c.i32(frame.at(0x128));c.set(0,std::uint32_t(club_id));
                if(compare(c,club_id,-1)>0) {
                    const auto target=c.u32(clubs)+std::uint32_t(club_id)*0x248;c.set(3,target);const auto owner=c.u32(target+0x90);c.set(0,owner);
                    if(nonzero_logical(c,owner)) {
                        const auto reputation=c.i16(target+0x1ec);c.set(0,extended(reputation));bool allowed=compare(c,reputation,4500)>0;
                        const RandomTable random{c,c.toc(-0x7b90),c.toc(-0x7b8c),c.toc(-0x7b88),cursor_or_node};
                        if(!allowed){random.advance(0x100826ec);const auto draw=random.value();c.set(3,draw);const auto sign=draw>>31;const auto rotated=((draw<<30)|(draw>>2))&0xc0000000u;const auto value=std::rotl(rotated-sign,2)+sign;c.set(0,value);allowed=!nonzero(c,std::int32_t(value));}
                        if(allowed){random.advance(0x1008274c);const auto draw=random.value();const auto sign=draw>>31;const auto parity=((draw&1)^sign)-sign;c.set(0,parity);c.set(3,sign);
                            c.set(0,c.u32(frame.at(0x128))*0x248);c.set(4,c.u32(clubs));
                            const auto target_club=c.u32(clubs)+c.u32(frame.at(0x128))*0x248;
                            if(nonzero(c,std::int32_t(parity)))p.call(0x100d9960,0x100827a0,{person,c.u32(target_club+0x90),1});
                            else p.call(0x100da110,0x100827c0,{person,target_club,1});
                        }
                    }
                }
            }
        }
    } else if(nonzero_logical(c,fallback)) {
        const auto offset=c.u32(person)*100+12,base=c.u32(indices);c.set(3,offset);c.set(4,base);c.set(5,1);const auto flags=c.u8(base+offset);c.set(0,flags|1u);c.put8(base+offset,flags|1u);
    }
    c.set(0,c.u32(frame.at(0x94)));c.put32(frame.at(0x9c),c.r(0));
    for(;;) {
        cursor_or_node=p.call(0x10738e00,0x10082adc,{frame.at(0x90)});if(!nonzero(c,std::int32_t(std::uint32_t(cursor_or_node))))break;
        if(nonzero_logical(c,best)) {
            const auto row=c.u32(cursor_or_node);const auto club_id=c.i32(row+4),chosen=c.i32(frame.at(0x128));c.set(3,std::uint32_t(club_id));c.set(0,std::uint32_t(chosen));
            if(compare(c,club_id,chosen)!=0) {
                const auto origin=c.i32(frame.at(0x194));c.set(0,std::uint32_t(origin));
                if(compare(c,origin,-1)!=0) {
                    const auto deferred=c.u32(object+0x38);c.set(0,deferred);
                    if(!nonzero_logical(c,deferred)){decision=p.call(0x10773520,0x10082834,{16});if(nonzero(c,std::int32_t(std::uint32_t(decision))))p.call(0x10738c00,0x10082840);c.put32(object+0x38,decision);}
                    p.call(0x10738d10,0x10082854,{c.u32(object+0x38),c.u32(cursor_or_node),136});
                }
                p.call(0x10738d10,0x10082868,{frame.at(0x70),c.u32(cursor_or_node)+4,4});p.call(0x10738e60,0x10082874,{c.u32(object+0x34),c.u32(cursor_or_node)});decrease_pending_count(c,person,indices);
            } else {
                const auto origin=c.i32(frame.at(0x194));c.set(0,std::uint32_t(origin));if(compare(c,origin,-1)!=0)continue;
                const auto accepted=p.call(0x103e8bb0,0x100828ac,{c.u32(transfers),frame.at(0x124)});c.set(0,std::uint32_t(std::int32_t(std::int8_t(accepted))));refused=std::int8_t(accepted)==0;c.set(0,refused);
                if(!nonzero(c,std::int8_t(std::uint32_t(refused)))) {
                    const auto kind=c.u8(frame.at(0x19c));
                    if(review_byte(c,p.call(0x100c7a00,0x100828c8,{kind}))) {
                        const auto profile=c.u32(person+0x1c);c.set(0,profile);
                        if(!nonzero_logical(c,profile)) {
                            p.call(0x102dbeb0,0x100828f0,{c.u32(national),frame.at(0x40),frame.at(0x3c),frame.at(0x38)});
                            const auto nation=c.i32(frame.at(0x40));c.set(4,std::uint32_t(nation));bool valid=compare(c,nation)>=0;
                            if(valid){const auto count=c.u32(c.toc(-0x79f4));c.set(0,count);valid=compare(c,nation,std::int32_t(count))<=0;}
                            if(!valid){p.call(0x10739890,0x10082924,{pool+0x37,0,0,frame.at(0xa0),frame.at(0x58)});const auto version=p.call(0x10768810,0x10082928);p.call(0x1077b720,0x10082944,{errors,pool+0x23b,version,frame.at(0xa0),frame.at(0x58),0x139f});p.call(0x10769b60,0x10082954,{pool+0x248,errors,0});const auto enabled=c.toc(-0x7bac);c.set(3,enabled);c.set(0,0);c.put8(enabled,0);}
                            else {const auto base=c.u32(c.toc(-0x79f8)),address=base+std::uint32_t(nation)*68;c.set(3,base);c.set(0,address);c.put32(person+0x1c,address);}
                        }
                    }
                    decision=pending_employer(c,p,managers,person,scratch,clubs,0x10082984,17);bool human=false;
                    if(nonzero_logical(c,decision))human=review_byte(c,p.call(0x1014c2e0,0x100829c8,{decision,1,1,0,0}));
                    if(human) {
                        p.call(0x103d2b30,0x100829e4,{c.u32(c.toc(-0x7a7c)),c.u32(person+0x24),0});p.call(0x1030f950,0x100829f4,{c.u32(c.toc(-0x7b1c)),c.u32(decision+0x90)});
                        const auto flags=c.u8(frame.at(0x180))&4u;c.set(0,flags);
                        if(nonzero(c,std::int32_t(flags))){const auto attributes=c.u32(person+0x14);c.set(0,attributes);if(nonzero_logical(c,attributes)){const auto value=p.call(0x10084be0,0x10082a28,{object,person,decision,0,0xffffffff,1});c.put32(person+8,value);}}
                    } else {
                        const auto attributes=c.u32(person+0x14);c.set(0,attributes);
                        if(nonzero_logical(c,attributes)){const auto flags=c.u8(frame.at(0x180))|4u;c.set(0,flags);c.put8(frame.at(0x180),flags);const auto value=p.call(0x10084be0,0x10082a64,{object,person,decision,0,0xffffffff,0});c.put32(person+8,value);}
                    }
                }
                p.call(0x10738e60,0x10082a74,{c.u32(object+0x34),c.u32(cursor_or_node)});decrease_pending_count(c,person,indices,true);p.call(0x103ea270,0x10082a9c,{c.u32(transfers),person});
            }
        } else {
            const auto row=c.u32(cursor_or_node);c.set(4,row);const auto state=c.u8(row+0x64);c.set(0,state);
            if(compare(c,state,3)!=0){p.call(0x10738e60,0x10082ab8,{c.u32(object+0x34),row});decrease_pending_count(c,person,indices);}
        }
    }
    c.set(0,std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(refused)))));
    if(nonzero(c,std::int8_t(std::uint32_t(refused)))){cleanup(0x10082af8);return;}
    if(nonzero_logical(c,best)) {
        const auto origin=c.i32(frame.at(0x194));c.set(0,std::uint32_t(origin));
        if(compare(c,origin,-1)!=0) {
            if(review_byte(c,p.call(0x100c7a00,0x10082b30,{c.u8(frame.at(0x19c))}))){const auto profile=c.u32(person+0x1c);c.set(0,profile);if(!nonzero_logical(c,profile))p.call(0x102dbeb0,0x10082b58,{c.u32(national),best+0x7c,best+0x80,best+0x84});}
            p.call(0x103e70f0,0x10082b74,{c.u32(transfers),person,c.u32(frame.at(0x194)),frame.at(0x124),c.u8(frame.at(0x19c)),frame.at(0x70)});
        } else p.call(0x1008a350,0x10082b88,{object,frame.at(0x124),0});
        const auto offset=c.u32(person)*100+12,base=c.u32(indices);c.set(5,0);c.set(4,base);c.set(3,offset);const auto flags=c.u8(base+offset)&~1u;c.set(0,flags);c.put8(base+offset,flags);
    }
    cleanup(0x10082bb4);
}
}
