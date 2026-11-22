#include "paged_screen_layout.hpp"

namespace cm::aot {
// Screens that are one table at the classic content rectangle (110,145 or 110,170 to 780,535) under the subtitle, with
// an optional menu above it, and the Back/Next footer: the awards of the game (AWARD_SCREENS.CPP: the list of an award across
// the seasons, builder 0x10000180, opened with an award of a competition from the Competitions menu or the competition
// screen, with the Ver menu under the subtitle, a table of the winners per season or, while nobody has won it, one centered
// message; and the nominees of an award, builder 0x100026d0, opened from an award's row: a three-column table of players with
// their clubs) and the friendly tour's invited clubs (builder 0x10048450, opener 0x10048330: a four-column table with an
// "invite another" button above its right edge). This pass reads how many columns the builder gave the table (the kind of
// award decides it) and hands the screen to the shared table recipe, which moves the table and the footer, carries the Ver menu to its side of the
// panel, keeps a scrollbar's gutter and lets only the widest column take the extra width. The original builders keep
// every text, link and command. The list's kinds with several columns were not exercised by a capture (nobody has won
// an award in the January 2002 world); a table the recipe cannot read (more than eight columns, an empty column) stays
// as built.
void layout_paged_table(State& s, std::uint32_t display, GuiCanvas canvas) {
    guest::Cpu c(s);
    const GuestScreen screen{c, display};
    const unsigned grids = std::min<unsigned>(250, c.u16(display + 0x129da4));
    for (const int top : {145, 170}) {
        const ScreenRect panel_box{110, top, 780, 535};
        unsigned columns = 0, found = 0;
        for (unsigned i = 0; i < grids; ++i)
            if (screen.rect(screen.grid(i)) == panel_box) { columns = screen.columns(screen.grid(i)); ++found; }
        if (found != 1) continue;
        paged_screen::layout_table_screen(s, display, canvas, panel_box, columns, columns == 1);
        return;
    }
}
}
