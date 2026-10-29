#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
void open_offer_contract_terms(Cpu& c,std::uint32_t offer_address) {
    Frame f(c,0x1042c6d0,0x90,{.first_gpr=22});
    Kept<22> pool(c,c.toc(slot::pool));
    Kept<23> offer(c,offer_address);
    Kept<24> proposed_contract(c,c.r(24));
    Kept<25> existing_contract(c,c.r(25));
    Kept<26> person(c,indirect(c,slot::people)+c.u32(offer+12)*0x70);
    Kept<27> error_buffer_or_percentage(c,c.toc(-0x7ba8));
    Kept<28> contracts(c,c.toc(slot::contracts));
    Kept<29> current_contract(c,c.toc(slot::current_contract));
    Kept<30> clubs(c,c.toc(slot::clubs));
    Kept<31> screens(c,c.toc(slot::screens));
    Program p{c};
    if (!byte_result(c,p.call(0x1073e4b0,0x1042c72c,{screens,c.toc(-0x4b6c),c.toc(-0x4b70),1,0,0}))) return;
    p.history(0x1042c748,screens,0,person);
    existing_contract=p.call(0x10082f20,0x1042c754,{c.u32(contracts),person});
    proposed_contract=p.call(0x1007f720,0x1042c76c,{c.u32(contracts),c.u32(person),c.u32(offer+8)});
    if (compare(c,proposed_contract)==0) {
        report_assertion(c,{0x1042c788,pool,pool+0x15,pool+0x22,f.at(0x48),f.at(0x40),0x2363});
        c.set(3,c.toc(-0x7bac)); return;
    }
    // The numerator and the rounding bias wrap as original signed 32-bit
    // arithmetic. Round the resulting ratio to a multiple of ten, truncating
    // the biased quotient toward zero (also for negative inputs).
    const auto numerator=c.u32(proposed_contract+12)*100u;
    error_buffer_or_percentage=divide_signed<4>(numerator,c.u32(existing_contract+12));
    const auto biased=std::int32_t(std::uint32_t(error_buffer_or_percentage)+5u);
    const auto high=std::int32_t((std::int64_t(biased)*0x66666667LL)>>32);
    c.s.wr<1>(reg::xer_ca,high<0 && (std::uint32_t(high)&3u)!=0);
    c.set(7,std::uint32_t(high)<0x80000000u ? 0u : 1u);
    error_buffer_or_percentage=std::uint32_t(biased/10)*10u;
    p.history(0x1042c810,screens,1,c.u32(existing_contract+12));
    p.history(0x1042c824,screens,2,c.u32(existing_contract+12));
    const auto record=p.call(0x10082f20,0x1042c830,{c.u32(contracts),person});
    compare_logical(c,record); c.put32(current_contract,record);
    if (record) {
        c.set({{3,c.u32(current_contract)},{4,c.u32(clubs)}});
        c.set(5,c.r(4)+c.u32(c.r(3)+4)*0x248);
    } else c.set(5,0);
    p.call(0x103fa8e0,0x1042c888,{indirect(c,slot::transfers),c.u32(clubs)+c.u32(offer+8)*0x248,c.r(5),person,
        f.at(0x39),f.at(0x3c),f.at(0x38),f.at(0x3a)});
    p.history(0x1042c8a0,screens,3,std::uint32_t(std::int32_t(c.i8(f.at(0x39)))));
    p.history(0x1042c8b4,screens,4,c.u16(f.at(0x3c)));
    p.history(0x1042c8cc,screens,5,std::uint32_t(std::int32_t(c.i8(f.at(0x38)))));
    p.history(0x1042c8e0,screens,6,c.u16(f.at(0x3a)));
    p.history(0x1042c8f4,screens,7,offer);
    p.history(0x1042c908,screens,8,proposed_contract);
    p.history(0x1042c91c,screens,9,c.u32(offer+0x2c));
    p.history(0x1042c930,screens,10,0);
    p.history(0x1042c944,screens,11,error_buffer_or_percentage);
    const auto expiry_month=p.call(0x10718360,0x1042c94c,{proposed_contract+0x30});
    p.history(0x1042c964,screens,12,std::uint32_t(std::int32_t(std::int8_t(expiry_month))));
    p.history(0x1042c978,screens,13,c.u16(proposed_contract+0x32));
    p.history(0x1042c990,screens,14,std::uint32_t(std::int32_t(c.i8(proposed_contract+0x1c))));
    p.history(0x1042c9a8,screens,15,std::uint32_t(std::int32_t(c.i8(proposed_contract+0x1d))));
    p.history(0x1042c9bc,screens,16,error_buffer_or_percentage);
    const auto repeated_month=p.call(0x10718360,0x1042c9c4,{proposed_contract+0x30});
    p.history(0x1042c9dc,screens,17,std::uint32_t(std::int32_t(std::int8_t(repeated_month))));
    p.history(0x1042c9f0,screens,18,c.u16(proposed_contract+0x32));
    p.history(0x1042ca08,screens,19,std::uint32_t(std::int32_t(c.i8(proposed_contract+0x1c))));
    p.history(0x1042ca20,screens,20,std::uint32_t(std::int32_t(c.i8(proposed_contract+0x1d))));
}

void open_offer_terms_from_news(Cpu& c,std::uint32_t manager,std::uint32_t item) {
    Frame f(c,0x10421ee0,0x130,{.first_gpr=30});
    Kept<31> offer_id(c,manager); Kept<30> revision(c,item); Program p{c};
    p.call(0x102bdf90,0x10421f0c,{f.at(0x38),0,0});
    p.call(0x102ccdd0,0x10421f24,{indirect(c,-0x7bf0),offer_id,revision,f.at(0x38)});
    offer_id=p.call(0x102cb570,0x10421f30,{f.at(0x38),6});
    revision=p.call(0x102cb570,0x10421f44,{f.at(0x38),7});
    c.set(5,c.toc(slot::transfers));
    const auto link=p.call(0x103eb440,0x10421f58,{c.u32(c.r(5)),offer_id});
    c.set(4,c.u32(link));
    bool previous=compare(c,c.u32(c.r(4)+0x2c),revision)!=0;
    if (!previous) { c.set(4,c.u32(link)); previous=compare(c,c.u8(c.r(4)+0x34),3)==0; }
    const auto offer=c.u32(link);
    if (previous) { enter(c,0x10421f84,{offer,revision}); open_previous_offer(c,offer,revision); }
    else { enter(c,0x10421f90,{offer}); open_offer_contract_terms(c,offer); }
}
}
