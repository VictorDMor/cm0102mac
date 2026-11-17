#include "form_layout.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include <algorithm>
#include <array>

namespace cm::aot {
void layout_transfer_proposal(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    // Identify only this builder's declared sections; menu grids and the
    // navigation bar must retain their own positioning and popup links.
    int title=-1, intro=-1, terms=-1, exchange=-1, footer=-1;
    const unsigned grids=std::min<unsigned>(250,c.u16(display+0x129da4));
    const unsigned controls=std::min<unsigned>(1200,c.u16(display+0x129da6));
    for (unsigned i=0;i<grids;++i) {
        const auto box=screen.rect(screen.grid(i));
        if (box==ScreenRect{100,10,790,70}) title=int(i);
        if (box==ScreenRect{100,80,790,125}) intro=int(i);
        if (box==ScreenRect{110,140,780,380} && screen.columns(screen.grid(i))==7) terms=int(i);
        if (box==ScreenRect{110,410,780,545} && screen.columns(screen.grid(i))==3) exchange=int(i);
        if (box==ScreenRect{100,555,790,590} && screen.rows(screen.grid(i))==1) footer=int(i);
    }
    if (title<0 || intro<0 || terms<0 || exchange<0 || footer<0) return;
    unsigned intro_rows=1,term_rows=1,exchange_rows=1;
    for (unsigned i=0;i<controls;++i) {
        const int owner=c.i16(screen.control(i)+0x72);
        const unsigned count=c.u32(screen.control(i)+0x24)+1;
        if (owner==intro) intro_rows=std::max(intro_rows,count);
        if (owner==terms) term_rows=std::max(term_rows,count);
        if (owner==exchange) exchange_rows=std::max(exchange_rows,count);
    }
    // Rich text is already measured by the original font engine. Give any
    // wrapped introduction its own rows instead of changing glyph metrics.
    if (intro_rows>4 || term_rows>30 || exchange_rows>30) return;
    const ScreenLayout shell{canvas};
    auto subtitle=shell.subtitle();
    subtitle.bottom=subtitle.top+std::max(45,int(intro_rows)*30);
    auto body=shell.content(subtitle.bottom+15);
    const int body_width=std::min(960,body.width());
    body.left+=(body.width()-body_width)/2; body.right=body.left+body_width;
    const int term_min=int(term_rows)*21+16,exchange_min=int(exchange_rows)*21+16;
    if (body.height()<term_min+exchange_min+30) return;
    const std::array<FormTrack,5> sections{{
        {term_min,float(term_rows),int(term_rows)*30+16},{5},{20},{5},
        {exchange_min,float(exchange_rows),int(exchange_rows)*30+16}}};
    const auto rows=form_tracks(body,sections,false);
    const std::array<FormTrack,7> fields{{{32},{335,1},{32},{168},{32},{32},{32}}};
    const auto columns=form_tracks(rows[0],fields,true);
    const unsigned button_count=screen.columns(screen.grid(footer));
    if (button_count<2 || button_count>4) return;
    auto buttons=shell.buttons(std::min(body.width(),int(button_count)*190));
    buttons.left-=buttons.right-body.right; buttons.right=body.right;
    const auto history=c.u32(c.toc(-0x7bb0)+0x10);
    const auto base_control=c.u32(history+0x14+15*8);
    if (base_control>=controls) return;
    const unsigned base_row=c.u32(screen.control(base_control)+0x24);
    GuiGuestScope preserve(s);
    c.set(1,c.sp()-0x400);
    for (unsigned i=0;i<controls;++i) {
        const auto at=screen.control(i);
        const int owner=c.i16(at+0x72);
        int left=c.i32(at+0x28),right=c.i32(at+0x2c);
        if (owner==title || owner==intro) {
            if (left && right) {
                const auto moved=DetailsLayout::centered_text({left,0,right,0},screen.rect(screen.grid(owner)),
                                                             owner==title?shell.title():subtitle);
                c.put32(at+0x28,moved.left); c.put32(at+0x2c,moved.right);
            }
        } else if (owner==terms) {
            if (left && right) {
                // Valuation/board paragraphs and linked clause descriptions
                // contain separate text/link controls. Translate their links
                // together; only the full paragraph's available width grows.
                const bool narrative=c.u32(at+0x24)<base_row;
                const int shift=narrative?body.left-110:columns[1].left+4-148;
                c.put32(at+0x28,left+shift);
                c.put32(at+0x2c, !c.u32(at+8) && right==(narrative?780:483)
                                  ? (narrative?body.right:columns[1].right-2):right+shift);
            } else {
                const unsigned col=c.u32(at+0x20);
                if (col<columns.size()) {
                    c.put32(at+0x28,columns[col].left+2);
                    c.put32(at+0x2c,columns[col].right-2);
                }
            }
        } else if (owner==exchange) {
            const unsigned col=c.u32(at+0x20);
            if (col==1 || col==2) {
                c.put32(at+0x28,columns[col==1?1:2].left+2);
                c.put32(at+0x2c,columns[col==1?2:6].left-2);
            }
        } else if (owner<0) {
            const auto before=screen.rect(at+0x10);
            auto after=before;
            if (before.top==385 && before.bottom==405 && before.width()==125) {
                if (before.left==110 || before.left==235) {
                    after.left=body.left+before.left-110; after.right=after.left+125;
                } else if (before.left==530 || before.left==655) {
                    after.right=body.right-(780-before.right); after.left=after.right-125;
                } else continue;
                after.top=rows[2].top; after.bottom=rows[2].bottom;
            } else if (before==ScreenRect{105,15,120,35}) {
                // Other bids stays attached to the title's leading edge.
                after.left=shell.title().left+5; after.right=after.left+15;
            } else continue;
            screen.write(at+0x10,after);
            c.put32(at+0x28,after.left); c.put32(at+0x2c,after.right);
            c.put32(at+0x30,after.top); c.put32(at+0x34,after.bottom);
        }
    }
    screen.set_rows(screen.grid(intro), intro_rows);
    screen.set_rows(screen.grid(terms), term_rows);
    screen.set_rows(screen.grid(exchange), exchange_rows);
    for (unsigned i=0;i<button_count;++i) c.put8(screen.grid(footer)+0xbb2+i,1);
    const std::array<std::pair<int,ScreenRect>,5> panels{{
        {title,shell.title()},{intro,subtitle},{terms,rows[0]},{exchange,rows[4]},{footer,buttons}}};
    for (const auto& [index,box]:panels) {
        screen.write(screen.grid(index),box);
        screen.measure(screen.grid(index));
        screen.publish(screen.grid(index));
    }
}
}
