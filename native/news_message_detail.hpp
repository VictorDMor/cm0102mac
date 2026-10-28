#pragma once
#include "news_detail.hpp"
#include <span>

namespace cm::aot::news::detail {
// One rendering invocation. The manager and quote byte live in the original
// argument homes; nested quoted messages must read those guest homes too.
struct Message {
    Cpu& c;
    const Frame& frame;
    std::uint32_t item, output, detail;
    std::uint32_t value(unsigned n) const {return c.u32(Item{c,item}.payload(n));}
    std::int8_t byte(unsigned n) const {return std::int8_t(value(n));}
    std::int16_t short_value(unsigned n) const {return std::int16_t(value(n));}
    std::uint32_t club(unsigned n) const {return c.u32(c.r(28))+value(n)*0x248;}
    std::uint32_t competition_name(std::uint32_t id) const {return c.u32(c.r(30))+id*0x6c;}
    std::uint32_t competition(std::uint32_t id) const {return c.u32(c.u32(c.r(24))+id*4);}
    std::uint32_t manager() const {return c.u32(frame.at(0x25dc));}
    std::uint32_t pool(unsigned offset) const {return c.r(31)+offset;}
    std::uint32_t scratch() const {return c.r(29);}
    void assign(std::uint32_t text,std::uint32_t back) const {Program{c}.call(0x1074b830,back,{output,text});}
    void append(std::uint32_t text,std::uint32_t back) const {Program{c}.call(0x1074b960,back,{output,text});}
    void link(std::uint32_t token,unsigned kind,std::uint32_t object,std::uint32_t back) const {
        Program{c}.call(0x1074bab0,back,{output,token,kind,object});
    }
    void format(std::uint32_t destination,unsigned phrase,std::uint32_t back,
                 std::initializer_list<std::uint32_t> arguments={}) const {
        format_values(destination,phrase,back,{arguments.begin(),arguments.size()});
    }
    void format_values(std::uint32_t destination,unsigned phrase,std::uint32_t back,
                       std::span<const std::uint32_t> arguments) const {
        c.set(3,destination);c.set(4,pool(phrase));unsigned index=0;
        for(auto argument:arguments) {
            if(index<6)c.set(5+index,argument);
            else c.put32(frame.at(0x38+(index-6)*4),argument);
            ++index;
        }
        c.call(0x10727890,back);
    }
    std::uint32_t local(unsigned type) const;
    std::uint32_t rearranged_fixture() const;
    std::uint32_t television_fixture() const;
    std::uint32_t hosting_bids() const;
    std::uint32_t host_selected() const;
    std::uint32_t friendly_proposal() const;
    std::uint32_t friendly_reply() const;
    std::uint32_t friendly_cancelled() const;
    std::uint32_t tournament_invitation() const;
    std::uint32_t tournament_reply() const;
    std::uint32_t tournament_cancelled() const;
    std::uint32_t virtual_message() const;
    std::uint32_t fixture_list() const;
    std::uint32_t decoded_virtual_message() const;
    std::uint32_t playoff_place() const;
    std::uint32_t geographical_reshuffle() const;
    std::uint32_t cup_draw() const;
    std::uint32_t pending_cup_draw() const;
    std::uint32_t draw_today() const;
    std::uint32_t group_draw() const;
    std::uint32_t intertoto_application() const;
    std::uint32_t intertoto_selection() const;
    std::uint32_t conditional_competition_message() const;
    std::uint32_t cup_bye() const;
    std::uint32_t competition_record() const;
    std::uint32_t injury_article() const;
    std::uint32_t manager_message() const;
};
}
