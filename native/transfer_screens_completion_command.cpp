#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
std::uint32_t transfer_completion_command(Cpu& c,std::uint32_t control_value) {
    Frame f(c,0x10424220,0x80,{.first_gpr=19});
    Kept<19> previous_screen_or_reason(c,c.toc(-0x4b00));
    Kept<20> previous_command_or_item(c,c.toc(-0x4b04));
    Kept<21> feedback_screen(c,c.toc(-0x4ae8));
    Kept<22> feedback_command(c,c.toc(-0x4aec));
    Kept<23> news(c,c.toc(-0x7bf0));
    Kept<24> transfers(c,c.toc(slot::transfers));
    Kept<25> reason(c,c.r(25));
    Kept<26> revision(c,c.r(26));
    Kept<27> club(c,c.r(27));
    Kept<28> draft_slot(c,c.r(28));
    Kept<29> control(c,control_value);
    Kept<30> offer_slot(c,c.r(30));
    Kept<31> screens(c,c.toc(slot::screens));
    Program p{c};
    c.set(5,c.toc(-0x7a54));
    draft_slot=p.call(0x10148a90,0x10424260,{c.u32(c.r(5)),0});
    c.set(5,c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+4));
    club=c.u32(c.r(5)+0x24);
    offer_slot=p.call(0x1073ef70,0x10424284,{screens,0});
    reason=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x10424298,{screens,3}))));
    revision=p.call(0x1073ef70,0x104242ac,{screens,4});
    const auto status=std::int8_t(p.call(0x1073ef70,0x104242c0,{screens,6}));
    // This first decoder retains history6 independently of its action result.
    const bool default_control=compare(c,std::int16_t(control),-1)==0;
    c.set(4,c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24));
    if(default_control) c.set(3,0);
    else {
        c.set(4,c.r(4)+0xc0000); c.set(3,c.r(4)+std::uint32_t(std::int16_t(control))*0x17c);
        c.set(3,c.u32(c.r(3)-0x5794));
    }
    c.set(3,std::uint32_t(std::int32_t(std::int16_t(c.r(3)))));
    if(compare(c,c.r(3))==0) c.set(3,std::uint32_t(std::int32_t(c.i16(screens+0x25819e))));
    const auto dispose=[&](std::uint32_t back) {
        p.call(0x10411f10,back,{c.u32(draft_slot),1}); c.put32(draft_slot,0);
    };
    if(compare(c,std::int16_t(c.r(3)),2)==0) {
        if(compare(c,status,2)==0)
            p.call(0x103e9910,0x10424340,{c.u32(transfers),c.u32(offer_slot),club,reason,1});
        dispose(0x1042434c); return std::uint32_t(-9);
    }
    bool completing=compare(c,control_action(c,screens,control),1)==0;
    if(!completing) completing=compare(c,control_action(c,screens,control),4)==0;
    if(!completing) completing=compare(c,control_action(c,screens,control),3)==0;
    if(completing) {
        c.set(3,c.u32(offer_slot));
        bool outdated=compare(c,c.u32(c.r(3)+0x2c),revision)!=0;
        if(!outdated) { c.set(3,c.u32(offer_slot)); outdated=compare(c,c.u8(c.r(3)+0x34),2)==0; }
        if(!outdated) { c.set(3,c.u32(offer_slot)); outdated=compare(c,c.u8(c.r(3)+0x34),3)==0; }
        if(outdated) {
            dispose(0x104244a0);
            const auto screen=std::uint32_t(previous_screen_or_reason);
            previous_screen_or_reason=c.u32(offer_slot);
            if(byte_result(c,p.call(0x1073e4b0,0x104244c8,{screens,screen,previous_command_or_item,1,0,0}))) {
                p.history(0x104244e4,screens,0,previous_screen_or_reason);
                p.history(0x104244f8,screens,1,revision);
            }
            return std::uint32_t(-11);
        }
    }
    if(compare(c,control_action(c,screens,control),1)==0) {
        c.set(3,c.u32(offer_slot)); c.set(4,c.toc(slot::clubs)); c.set(3,c.u32(c.r(4)));
        if(compare_logical(c,club,c.r(3)+c.u32(c.u32(offer_slot)+8)*0x248)==0)
            p.call(0x103e44b0,0x1042458c,{c.u32(transfers),offer_slot,0,reason});
        else {
            c.put16(f.at(0x3a),0);
            if(byte_result(c,p.call(0x103f4d10,0x104245ac,{c.u32(transfers),c.u32(offer_slot),0,f.at(0x3a)}))) {
                previous_screen_or_reason=std::uint32_t(std::int32_t(c.i16(f.at(0x3a))));
                if(byte_result(c,p.call(0x1073e4b0,0x104245d4,{screens,feedback_screen,feedback_command,1,0,0}))) {
                    p.history(0x104245f0,screens,0,offer_slot);
                    p.history(0x10424604,screens,1,0x1b);
                    p.history(0x10424618,screens,2,previous_screen_or_reason);
                }
                return std::uint32_t(-4);
            }
            p.call(0x103e4130,0x10424634,{c.u32(transfers),offer_slot,0,std::uint32_t(std::int32_t(c.i16(f.at(0x3a))))});
        }
        dispose(0x10424640); return std::uint32_t(-9);
    }
    if(compare(c,control_action(c,screens,control),4)==0) {
        previous_screen_or_reason=p.call(0x1073ef70,0x104246b4,{screens,1});
        previous_command_or_item=p.call(0x1073ef70,0x104246c8,{screens,2});
        c.set(4,c.u32(offer_slot)); c.set(5,c.u32(club));
        if(compare(c,c.r(5),c.u32(c.r(4)+8))==0)
            p.call(0x102cd0c0,0x104246fc,{c.u32(news),previous_screen_or_reason,previous_command_or_item,9,0x12,0});
        else {
            if(byte_result(c,p.call(0x103f4d10,0x10424714,{c.u32(transfers),c.u32(offer_slot),0,f.at(0x38)}))) {
                previous_screen_or_reason=std::uint32_t(std::int32_t(c.i16(f.at(0x38))));
                if(byte_result(c,p.call(0x1073e4b0,0x1042473c,{screens,feedback_screen,feedback_command,1,0,0}))) {
                    p.history(0x10424758,screens,0,offer_slot);
                    p.history(0x1042476c,screens,1,0x23);
                    p.history(0x10424780,screens,2,previous_screen_or_reason);
                }
                return std::uint32_t(-4);
            }
            p.call(0x102cd0c0,0x104247a4,{c.u32(news),previous_screen_or_reason,previous_command_or_item,9,0x11,0});
        }
        p.call(0x103e8500,0x104247b8,{c.u32(transfers),c.u32(offer_slot),club,reason});
        dispose(0x104247c4); return std::uint32_t(-9);
    }
    if(compare(c,control_action(c,screens,control),3)==0) {
        p.call(0x103e80e0,0x1042483c,{c.u32(transfers),c.u32(offer_slot),club});
        dispose(0x10424848); return std::uint32_t(-9);
    }
    const auto command=p.call(0x102a54b0,0x10424860,{control});
    if(compare(c,command)!=0) return command;
    const auto fallback=p.call(0x102a83a0,0x10424874,{control});
    c.s.wr<1>(reg::xer_ca,(fallback&0x7fffffff)!=0); return fallback;
}
}
