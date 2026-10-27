#include "screen_manager_packets.hpp"
#include <array>

namespace cm::aot::screen_manager {
using namespace detail;
namespace {
struct PropertySite {
    std::uint32_t entry, first_diagnostic, value_diagnostic, length_call, copy_call;
    SendSite send;
    unsigned tag, width, value_line;
    bool skip_missing;
};
constexpr std::array property_sites{
    PropertySite{0x10740620,0x10740684,0x10740808,0,0,{0x10740880,0x107408b4,0x107408c8,0x107408e8,0xe0,0x58,0xa2c},0,4,0x88,true},
    PropertySite{}, // Rectangle has four independent short values.
    PropertySite{0x10740e10,0x10740e74,0x10740ff8,0,0,{0x10741070,0x107410a4,0x107410b8,0x107410d8,0xe0,0x58,0xa75},2,4,0x88,true},
    PropertySite{0x10741130,0x10741194,0x10741318,0,0,{0x10741398,0x107413cc,0x107413e0,0x10741400,0xe0,0x58,0xa98},3,2,0x8b,true},
    PropertySite{0x10741450,0x107414b4,0x10741638,0,0,{0x107416b0,0x107416e4,0x107416f8,0x10741718,0xe0,0x58,0xade},5,4,0x88,true},
    PropertySite{0x10741770,0x107417c8,0x1074194c,0,0,{0x107419c4,0x107419f8,0x10741a0c,0x10741a2c,0xe0,0x58,0xaff},6,1,0x85,false},
    PropertySite{0x10741a80,0x10741ae4,0x10741c68,0,0,{0x10741ce8,0x10741d1c,0x10741d30,0x10741d50,0xe0,0x58,0xb22},7,2,0x8b,true},
    PropertySite{0x10741da0,0x10741e04,0x10741f98,0x10741f64,0x10742008,{0x1074203c,0x10742070,0x10742084,0x107420a4,0xe0,0x58,0xb45},8,2,0x86,true},
    PropertySite{0x10742100,0x10742164,0x107422e8,0,0,{0x10742360,0x10742394,0x107423a8,0x107423c8,0xe0,0x58,0xb68},9,4,0x88,true},
    PropertySite{0x10742420,0x10742484,0x10742608,0,0,{0x10742680,0x107426b4,0x107426c8,0x107426e8,0xe0,0x58,0xb8b},10,4,0x89,true}
};
}
std::uint32_t send_property(Cpu& c, std::uint32_t address, std::uint32_t field,
                            std::uint32_t value, Property type) {
    const auto& site = property_sites[unsigned(type)];
    Frame frame(c, site.entry, 0x120, {.first_gpr=26});
    const bool text = type == Property::Text;
    keep(c, {{26,address},{27,text?value:field},{28,text?field:value},
             {29,c.toc(slot::success)},{30,c.toc(slot::message)},{31,c.toc(slot::pool)}});
    if (site.skip_missing && compare(c,std::int16_t(field),-1)==0) return c.r(3);
    auto out=packet(c,frame,address);
    c.put32(out.position,0);
    out.integer(1,8,site.first_diagnostic,0xc0,0x50,0x85);
    out.integer(2,field,site.first_diagnostic+0x80,0xa0,0x48,0x86);
    out.integer(1,site.tag,site.first_diagnostic+0x104,0x80,0x40,0x85);
    if(text) {
        c.set(28,extended(std::int16_t(Program{c}.call(0x1077dbd0,site.length_call+4,{c.r(27)})+1)));
        out.integer(2,c.r(28),site.value_diagnostic,0x60,0x38,0x86);
        out.string(c.r(27),c.r(28),site.copy_call+4);
    } else out.integer(site.width,c.r(28),site.value_diagnostic,0x60,0x38,site.value_line);
    send_current(c,frame,address,site.send);
    return c.r(3);
}
std::uint32_t send_rectangle(Cpu& c,std::uint32_t address,std::uint32_t field,
    std::uint32_t x,std::uint32_t y,std::uint32_t width,std::uint32_t height) {
    Frame frame(c,0x10740940,0x1a0,{.first_gpr=23});
    keep(c,{{23,field},{30,c.toc(slot::success)},{31,c.toc(slot::message)},{29,address},
            {28,c.toc(slot::pool)},{24,x},{25,y},{26,width},{27,height}});
    if(compare(c,std::int16_t(field),-1)==0)return c.r(3);
    auto out=packet(c,frame,address);c.put32(out.position,0);
    out.integer(1,8,0x107409b0,0x138,0x68,0x85);
    out.integer(2,c.r(23),0x10740a30,0x118,0x60,0x86);
    out.integer(1,1,0x10740ab4,0xf8,0x58,0x85);
    constexpr std::array diagnostics{0x10740b38u,0x10740bc0u,0x10740c48u,0x10740cd0u};
    for(unsigned i=0;i<4;++i) {
        c.set(4,extended(std::int16_t(c.r(24+i))));
        out.integer(2,c.r(4),diagnostics[i],0xd8-i*0x20,0x50-i*8,0x86);
    }
    send_current(c,frame,address,{0x10740d50,0x10740d84,0x10740d98,0x10740db8,0x158,0x70,0xa52});
    return c.r(3);
}
std::uint32_t send_command(Cpu& c,std::uint32_t address,unsigned command) {
    struct Site {std::uint32_t entry,diagnostic;SendSite send;unsigned tag;};
    constexpr std::array sites{
        Site{0x10740070,0x107400cc,{0x10740148,0,0x1074015c,0x1074017c,0x68,0x40,0x9ae},0x11},
        Site{0x10743840,0x1074389c,{0x10743918,0,0x1074392c,0x1074394c,0x68,0x40,0xce5},0x18},
        Site{0x10743bd0,0x10743c2c,{0x10743ca8,0,0x10743cbc,0x10743cdc,0x68,0x40,0xd17},0x1a},
        Site{0x10743d40,0x10743d9c,{0x10743e18,0,0x10743e2c,0x10743e4c,0x68,0x40,0xd2e},0x16},
        Site{0x10743eb0,0x10743f0c,{0x10743f88,0,0x10743f9c,0x10743fbc,0x68,0x40,0xd45},0x15}
    };
    const auto& site=sites[command];
    Frame frame(c,site.entry,0xa0,{.first_gpr=28});
    keep(c,{{31,c.toc(slot::message)},{30,c.toc(slot::success)},{29,address},{28,c.toc(slot::pool)}});
    auto out=packet(c,frame,address);c.put32(out.position,0);
    out.integer(1,site.tag,site.diagnostic,0x48,0x38,0x85);
    send_current(c,frame,address,site.send);return c.r(3);
}
std::uint32_t send_short_command(Cpu& c,std::uint32_t address,std::uint32_t value,bool last) {
    const auto entry=last?0x10743660u:0x107401e0u;
    const auto first=last?0x107436b4u:0x10740234u;
    Frame frame(c,entry,0xd0,{.first_gpr=27});
    keep(c,{{28,value},{27,address},{29,c.toc(slot::success)},{30,c.toc(slot::message)},{31,c.toc(slot::pool)}});
    auto out=packet(c,frame,address);c.put32(out.position,0);
    out.integer(1,last?0x17:0x13,first,0x70,0x40,0x85);
    out.integer(2,c.r(28),first+0x80,0x50,0x38,0x86);
    send_current(c,frame,address,last?SendSite{0x107437b4,0,0x107437c8,0x107437e8,0x90,0x48,0xcce}:
                                      SendSite{0x10740334,0,0x10740348,0x10740368,0x90,0x48,0x9dd});
    return c.r(3);
}
}
