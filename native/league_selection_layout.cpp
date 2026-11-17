#include "form_layout.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include <array>

namespace cm::aot {
void layout_league_selection(State& s, std::uint32_t display, GuiCanvas canvas, bool selector) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    int options=-1, size=-1, countries=-1, footer=-1;
    const ScreenLayout shell{canvas};
    for (unsigned i=0;i<std::min<unsigned>(250,c.u16(display+0x129da4));++i) {
        const auto box=screen.rect(screen.grid(i));
        if (box==ScreenRect{110,145,525,165}) options=int(i);
        if (box==ScreenRect{530,145,780,165}) size=int(i);
        if (box==ScreenRect{110,170,780,535}) countries=int(i);
        if ((box==ScreenRect{100,555,790,590} || box==shell.footer()) && screen.rows(screen.grid(i))==1) footer=int(i);
    }
    if (options<0 || size<0 || countries<0) return;
    auto panel=shell.content(); panel.bottom-=10;
    const int width=std::min(1120,panel.width()); panel.left+=(panel.width()-width)/2; panel.right=panel.left+width;
    const std::array<FormTrack,5> tracks{{{selector?30:0},{selector?10:0},{30},{10},{240,1}}};
    const auto sections=form_tracks(panel,tracks,false);
    const std::array<FormTrack,3> columns{{{415,1},{5},{250,0,320}}};
    const auto config=form_tracks(sections[2],columns,true);
    GuiGuestScope preserve(s); c.set(1,c.sp()-0x400);
    screen.write(screen.grid(options),config[0]); screen.write(screen.grid(size),config[2]); screen.write(screen.grid(countries),sections[4]);
    const int rows=std::clamp(sections[4].height()/25,8,30);
    screen.show_rows(screen.grid(countries), 0, rows);
    screen.set_rows(screen.grid(countries), rows);
    screen.hide_scrollbar_if_fits(screen.grid(countries), rows);
    screen.resize_rows(screen.grid(countries));
    if(footer>=0) {
        screen.write(screen.grid(footer),{panel.right-340,canvas.height-45,panel.right,canvas.height-10});
        for(unsigned col=0;col<screen.columns(screen.grid(footer));++col)c.put8(screen.grid(footer)+0xbb2+col,1);
    }
    for(int i:{options,size,countries,footer}) if(i>=0) {
        screen.measure(screen.grid(i));
        screen.publish(screen.grid(i));
    }
}
}
