#include "transfer_screens_detail.hpp"

namespace cm::aot::transfer_screens {
using namespace detail;
namespace {
struct TransferFeedback {
    Cpu& c;
    Frame f;
    Program p;
    Kept<16> reason_or_valid;
    Kept<17> kind;
    Kept<18> recipient;
    Kept<19> selected_nation_or_owner;
    Kept<20> other_nation_or_owner;
    Kept<21> nations;
    Kept<22> transfers;
    Kept<23> contracts;
    Kept<24> current_contract;
    Kept<25> clubs;
    Kept<26> screens;
    Kept<27> rows;
    Kept<28> person;
    Kept<29> foreground;
    Kept<30> text;
    Kept<31> pool;

    explicit TransferFeedback(Cpu& cpu)
        : c(cpu), f(c,0x10421070,0x4f0,{.first_gpr=16}), p{c},
          reason_or_valid(c,c.r(16)), kind(c,c.r(17)), recipient(c,c.r(18)),
          selected_nation_or_owner(c,c.toc(-0x7a18)), other_nation_or_owner(c,c.toc(-0x6678)),
          nations(c,c.toc(-0x7b98)), transfers(c,c.toc(slot::transfers)),
          contracts(c,c.toc(slot::contracts)), current_contract(c,c.toc(slot::current_contract)),
          clubs(c,c.toc(slot::clubs)), screens(c,c.toc(slot::screens)), rows(c,c.r(27)),
          person(c,c.r(28)), foreground(c,c.toc(-0x7bc8)), text(c,c.toc(-0x7bb8)),
          pool(c,c.toc(slot::pool)) {
        c.put16(f.at(0x50),c.u16(foreground)); c.put16(f.at(0x52),c.u16(c.toc(-0x7b14)));
        person = p.call(0x1073ef70,0x104210cc,{screens,0});
        recipient = p.call(0x1073ef70,0x104210e0,{screens,1});
        reason_or_valid = extended(std::int16_t(p.call(0x1073ef70,0x104210f4,{screens,2})));
        kind = std::uint32_t(std::int32_t(std::int8_t(p.call(0x1073ef70,0x10421108,{screens,4}))));
    }
    int kind_index() {
        const auto value = std::int8_t(std::uint32_t(kind));
        if (compare(c,value) == 0) return 0;
        if (compare(c,value,1) == 0) return 1;
        if (compare(c,value,2) == 0) return 2;
        return 3;
    }
    void heading() {
        p.call(0x107136d0,0x10421120,{person,0,0});
        p.call(0x100c7a50,0x10421134,{recipient,1,f.at(0x52),f.at(0x50)});
        p.call(0x10744250,0x10421148,{screens,c.u16(f.at(0x50)),c.u16(f.at(0x52)),recipient+0x1ee});
        p.call(0x100caeb0,0x1042115c,{person,f.at(0x378),300,1});
        struct Heading { unsigned caption; std::uint32_t back; };
        static constexpr Heading headings[]{{0xe0e,0x10421174},{0xe44,0x10421194},
            {0xe74,0x104211b4},{0xea5,0x104211c8}};
        const auto& h = headings[kind_index()];
        p.call(0x10727890,h.back,{text,pool+h.caption,f.at(0x378)});
        p.call(0x107442f0,0x104211d4,{screens,text});
        c.set(3,divide_signed<4>(405,p.call(0x1076d470,0x104211dc,{3})));
        rows = std::uint32_t(std::int32_t(std::int8_t(c.r(3))));
    }
    std::uint32_t owner(std::uint32_t back) {
        reason_or_valid = 0;
        const auto record = p.call(0x10082f20,back,{c.u32(contracts),person});
        compare_logical(c,record); c.put32(current_contract,record);
        if (record) {
            c.set(3,c.u32(current_contract));
            if (compare(c,c.i8(c.r(3)+0x38)>>2,5) != 0) reason_or_valid=1;
        }
        if (compare(c,reason_or_valid) == 0) return 0;
        c.set({{3,c.u32(current_contract)},{4,c.u32(clubs)}});
        return c.r(4)+c.u32(c.r(3)+4)*0x248;
    }
    void owner_message() {
        if (compare_logical(c,owner(0x104212e4)) == 0) {
            p.call(0x10727890,0x10421494,{text,pool+0x1051,f.at(0x378)}); return;
        }
        // These are separate observable queries in the original, even though
        // the contract manager normally returns the same record each time.
        selected_nation_or_owner=owner(0x1042134c);
        other_nation_or_owner=owner(0x104213ac);
        c.set(3,owner(0x1042140c));
        p.call(0x10727890,0x10421480,{text,pool+0xfe9,
            std::uint32_t(std::int32_t(c.i8(c.r(3)+0x23b))),
            std::uint32_t(std::int32_t(c.i8(other_nation_or_owner+0x23b))),
            f.at(0x378),selected_nation_or_owner+0x221});
    }
    void mark_response() {
        p.call(0x103ea290,0x10421240,{c.u32(transfers),person,f.at(0x54)});
        c.set(4,p.call(0x1007f720,0x10421254,{c.u32(contracts),c.u32(person),c.u32(c.u32(f.at(0x54)))}));
        if (compare(c,c.r(4)) != 0 && compare(c,c.i32(c.r(4)+0x70),-1) != 0) {
            const auto link = p.call(0x103eb440,0x10421274,{c.u32(transfers),c.u32(c.r(4)+0x70)});
            if (compare_logical(c,link) != 0 && compare_logical(c,c.u32(link)) != 0) {
                c.set(3,c.u32(link)); c.put8(c.r(3)+0x35,1);
            }
        }
        const auto club=c.u32(f.at(0x54));
        const auto gender=std::uint32_t(std::int32_t(c.i8(club+0x23b)));
        p.call(0x10727890,0x104212c4,{text,pool+0xf77,gender,gender,f.at(0x378),club+0x221});
    }
    struct NationMessage {
        unsigned scratch, caption;
        std::uint32_t nation_back, club_nation_back, format_back;
    };
    void nation_message(bool selected) {
        static constexpr NationMessage other[] {
            {0x314,0x1143,0x10421588,0x104215a0,0x104215cc},
            {0x2b0,0x11ac,0x10421608,0x10421620,0x1042164c},
            {0x24c,0x1216,0x10421688,0x104216a0,0x104216cc},
            {0x1e8,0x1281,0x104216fc,0x10421714,0x10421740}};
        static constexpr NationMessage home[] {
            {0x184,0x12eb,0x10421778,0,0x104217a4},
            {0x120,0x1350,0x104217d4,0,0x10421800},
            {0xbc,0x13b6,0x10421830,0,0x1042185c},
            {0x58,0x141d,0x10421880,0,0x104218ac}};
        const auto index=kind_index();
        const auto& message=selected ? home[index] : other[index];
        const bool use_registry=selected || compare(c,std::int16_t(std::uint32_t(reason_or_valid)),25)==0;
        const auto nation=use_registry ? c.u32(nations)+c.u32(selected ? std::uint32_t(selected_nation_or_owner) : std::uint32_t(other_nation_or_owner))*0x128
                                       : c.u32(recipient+0x2c);
        p.call(0x10168a00,use_registry ? message.nation_back : message.club_nation_back,
            {nation,f.at(message.scratch),100,13});
        const auto gender=std::uint32_t(std::int32_t(c.i8(recipient+0x23b)));
        p.call(0x10727890,message.format_back,{text,pool+message.caption,gender,gender,100,100,
            recipient+0x221,f.at(message.scratch)});
    }
    void message() {
        const auto reason=std::int16_t(std::uint32_t(reason_or_valid));
        if (compare(c,reason,7)==0) p.call(0x10727890,0x10421204,{text,pool+0xed5,f.at(0x378)});
        else if (compare(c,reason,39)==0) p.call(0x10727890,0x10421220,{text,pool+0xf31});
        else if (compare(c,reason,33)==0) mark_response();
        else if (compare(c,reason,1)==0) owner_message();
        else if (compare(c,reason,18)==0 || compare(c,reason,32)==0) {
            c.set(10,p.call(0x1073ef70,0x104214bc,{screens,3}));
            c.set({{8,c.u32(clubs)},{9,c.u32(clubs)}});
            const auto club=c.u32(clubs)+c.r(10)*0x248;
            const auto gender=std::uint32_t(std::int32_t(c.i8(club+0x23b)));
            p.call(0x10727890,0x10421508,{text,pool+0x1096,gender,gender,f.at(0x378),club+0x221});
        } else if (compare(c,reason,41)==0) {
            const auto gender=std::uint32_t(std::int32_t(c.i8(recipient+0x23b)));
            p.call(0x10727890,0x10421538,{text,pool+0x10f9,gender,gender,recipient+0x221});
        } else if (compare(c,reason,25)==0 || compare(c,reason,40)==0) nation_message(false);
        else if (compare(c,reason,26)==0) nation_message(true);
    }
    void footer() {
        c.put32(f.at(0x38),c.u16(foreground)); c.put32(f.at(0x3c),text); c.put32(f.at(0x40),0);
        p.call(0x1074a350,0x104218e4,{110,140,780,545,2,0,
            std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(rows)))),3});
        p.call(0x102a1070,0x104218f0,{4,0});
        p.call(0x10727890,0x104218fc,{text,pool+0x5ed});
        c.put32(f.at(0x38),3); c.put32(f.at(0x3c),c.u16(foreground)); c.put32(f.at(0x40),text);
        c.put32(f.at(0x44),1); c.put32(f.at(0x48),0);
        p.call(0x10744430,0x10421948,{screens,100,555,790,590,0x30,c.u16(c.toc(-0x7bc4)),12});
    }
};
}
void render_transfer_feedback(Cpu& c) {
    TransferFeedback screen(c);
    screen.heading(); screen.message(); screen.footer();
}
}
