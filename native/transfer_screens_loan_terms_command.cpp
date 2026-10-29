#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
std::uint32_t loan_offer_terms_command(Cpu& c,std::uint32_t control_value) {
    Frame f(c,0x1042e110,0x70,{.first_gpr=18});
    Kept<18> feedback_screen_or_value(c,c.toc(-0x4ad8));
    Kept<19> feedback_command_or_salary(c,c.toc(-0x4adc));
    Kept<20> transfers_or_slot(c,c.toc(slot::transfers));
    Kept<21> previous_screen(c,c.toc(-0x4b00));
    Kept<22> previous_command(c,c.toc(-0x4b04));
    Kept<23> person_or_revision(c,c.r(23));
    Kept<24> cup(c,c.r(24));
    Kept<25> parent(c,c.r(25));
    Kept<26> percentage(c,c.r(26));
    Kept<27> year(c,c.r(27));
    Kept<28> month(c,c.r(28));
    Kept<29> offer(c,c.r(29));
    Kept<30> control(c,control_value);
    Kept<31> screens(c,c.toc(slot::screens));
    Program p{c};
    person_or_revision=p.call(0x1073ef70,0x1042e148,{screens,0});
    offer=p.call(0x1073ef70,0x1042e15c,{screens,7});
    p.call(0x1073ef70,0x1042e170,{screens,10});
    percentage=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x1042e17c,{screens,11}))));
    month=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x1042e190,{screens,12}))));
    year=std::uint16_t(p.call(0x1073ef70,0x1042e1a4,{screens,13}));
    cup=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x1042e1b8,{screens,14}))));
    parent=std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x1042e1cc,{screens,15}))));
    c.set(4,std::uint32_t(c.i16(screens+0x2c58))*0x2c0);
    if(compare(c,control_action(c,screens,control),5)==0)return std::uint32_t(-9);
    const auto confirm=[&](std::uint32_t back,std::uint32_t existing) {
        enter(c,back,{person_or_revision,month,year,percentage,cup,parent,existing});
        open_temporary_offer_terms_confirmation(c,person_or_revision,month,year,percentage,cup,parent,existing);
    };
    if(compare(c,control_action(c,screens,control),6)==0) {confirm(0x1042e2a8,0);return std::uint32_t(-4);}
    if(compare(c,control_action(c,screens,control),1)==0) {
        p.call(0x1073ef70,0x1042e314,{screens,3});p.call(0x1073ef70,0x1042e320,{screens,4});
        p.call(0x1073ef70,0x1042e32c,{screens,0});month=month-1;
        if(compare(c,std::int8_t(month))<0) {month=11;year=year-1;}
        p.history(0x1042e354,screens,12,std::uint32_t(std::int32_t(std::int8_t(month))));
        p.history(0x1042e368,screens,13,std::uint16_t(year));return std::uint32_t(-11);
    }
    if(compare(c,control_action(c,screens,control),2)==0) {
        p.call(0x1073ef70,0x1042e3d4,{screens,5});p.call(0x1073ef70,0x1042e3e0,{screens,6});
        p.call(0x1073ef70,0x1042e3ec,{screens,0});month=month+1;
        if(compare(c,std::int8_t(month),11)>0) {month=0;year=year+1;}
        p.history(0x1042e418,screens,12,std::uint32_t(std::int32_t(std::int8_t(month))));
        p.history(0x1042e42c,screens,13,std::uint16_t(year));return std::uint32_t(-11);
    }
    if(compare(c,control_action(c,screens,control),3)==0) {
        percentage=percentage-10;p.history(0x1042e4a4,screens,11,std::uint32_t(std::int32_t(std::int8_t(percentage))));return std::uint32_t(-11);
    }
    if(compare(c,control_action(c,screens,control),4)==0) {
        percentage=percentage+10;p.history(0x1042e51c,screens,11,std::uint32_t(std::int32_t(std::int8_t(percentage))));return std::uint32_t(-11);
    }
    if(compare(c,control_action(c,screens,control),7)==0) {
        const bool none=compare(c,std::int16_t(control),-1)==0;
        c.set(3,c.u32(screens+std::uint32_t(c.i16(screens+0x2c58))*0x2c0+0x24));
        std::uint32_t selected=0;
        if(!none) {c.set(3,c.r(3)+0xc0000+std::uint32_t(std::int16_t(control))*0x17c);selected=c.u32(c.r(3)-0x5754);}
        feedback_screen_or_value=selected;
        feedback_command_or_salary=p.call(0x1073ef70,0x1042e5c4,{screens,1});
        const auto board=p.call(0x1073ef70,0x1042e5d8,{screens,2});
        const auto cap=compare(c,feedback_command_or_salary,board)<0 ? std::uint32_t(feedback_command_or_salary) : board;
        if(compare(c,feedback_screen_or_value,cap)>=0) {
            if(compare(c,feedback_command_or_salary,board)<0)c.set(3,feedback_command_or_salary);
            feedback_screen_or_value=c.r(3);
        }
        const auto quotient=divide_signed<4>(std::uint32_t(feedback_screen_or_value)*100u,feedback_command_or_salary);
        p.history(0x1042e620,screens,11,std::uint32_t(std::int32_t(std::int8_t(quotient))));return std::uint32_t(-11);
    }
    if(compare(c,control_action(c,screens,control),8)==0) {
        p.history(0x1042e69c,screens,14,std::int8_t(cup)==0 ? 1u : 0u);return std::uint32_t(-11);
    }
    if(compare(c,control_action(c,screens,control),9)==0) {
        p.history(0x1042e718,screens,15,std::int8_t(parent)==0 ? 1u : 0u);return std::uint32_t(-11);
    }
    const auto stale=[&](std::uint32_t revision) {
        if(compare(c,c.u32(offer+0x2c),revision)!=0)return true;
        return compare(c,c.u8(offer+0x34),3)==0;
    };
    const auto previous=[&](std::uint32_t create_back,std::uint32_t offer_back,std::uint32_t revision_back,std::uint32_t revision) {
        if(byte_result(c,p.call(0x1073e4b0,create_back,{screens,previous_screen,previous_command,1,0,0}))) {
            p.history(offer_back,screens,0,offer);p.history(revision_back,screens,1,revision);
        }
    };
    if(compare(c,control_action(c,screens,control),10)==0) {
        feedback_screen_or_value=p.call(0x1073ef70,0x1042e784,{screens,9});
        if(stale(feedback_screen_or_value)) {
            previous(0x1042e7bc,0x1042e7d8,0x1042e7ec,feedback_screen_or_value);return std::uint32_t(-11);
        }
        confirm(0x1042e814,offer);return std::uint32_t(-4);
    }
    const auto feedback=[&](std::uint32_t lookup_back,std::uint32_t create_back,std::uint32_t zero_back,
                            std::uint32_t action_back,std::uint32_t slot_back,unsigned action) {
        transfers_or_slot=p.call(0x103eb440,lookup_back,{c.u32(transfers_or_slot),c.u32(offer+0x28)});
        if(byte_result(c,p.call(0x1073e4b0,create_back,{screens,feedback_screen_or_value,feedback_command_or_salary,1,0,0}))) {
            p.history(zero_back,screens,0,0);p.history(action_back,screens,1,action);p.history(slot_back,screens,2,transfers_or_slot);
        }
    };
    if(compare(c,control_action(c,screens,control),11)==0) {
        person_or_revision=p.call(0x1073ef70,0x1042e880,{screens,9});
        if(stale(person_or_revision)) {previous(0x1042e8b8,0x1042e8d4,0x1042e8e8,person_or_revision);return std::uint32_t(-11);}
        feedback(0x1042e8fc,0x1042e91c,0x1042e938,0x1042e94c,0x1042e960,29);return std::uint32_t(-4);
    }
    if(compare(c,control_action(c,screens,control),12)==0) {
        person_or_revision=p.call(0x1073ef70,0x1042e9cc,{screens,9});
        if(stale(person_or_revision)) {previous(0x1042ea04,0x1042ea20,0x1042ea34,person_or_revision);return std::uint32_t(-11);}
        feedback(0x1042ea48,0x1042ea68,0x1042ea84,0x1042ea98,0x1042eaac,27);return std::uint32_t(-4);
    }
    const auto restore=[&](unsigned snapshot,std::uint32_t read_back,std::uint32_t write_back) {
        for(unsigned i=0;i<5;++i) {
            const auto value=p.call(0x1073ef70,read_back+i*0x20,{screens,snapshot+i});
            p.history(write_back+i*0x20,screens,11+i,value);
        }
    };
    if(compare(c,control_action(c,screens,control),14)==0) {restore(16,0x1042eb18,0x1042eb2c);return std::uint32_t(-11);}
    if(compare(c,control_action(c,screens,control),13)==0) {restore(21,0x1042ec18,0x1042ec2c);return std::uint32_t(-11);}
    if(compare(c,control_action(c,screens,control),15)==0) {
        const auto selected=p.call(0x103eb440,0x1042ed18,{c.u32(transfers_or_slot),c.u32(offer+0x28)});
        p.call(0x103e44b0,0x1042ed2c,{c.u32(transfers_or_slot),selected,0,0});return std::uint32_t(-9);
    }
    const auto command=p.call(0x102a54b0,0x1042ed3c,{control});if(compare(c,command)!=0)return command;
    const auto fallback=p.call(0x102a83a0,0x1042ed50,{control});c.s.wr<1>(reg::xer_ca,(fallback&0x7fffffff)!=0);return fallback;
}
}
