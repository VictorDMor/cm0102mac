#include "adaptive_gui.hpp"
#include "guest_abi.hpp"
#include "platform.hpp"
#include "new_screens.hpp"
#include "screen_layout.hpp"
#include "transfer_screens_adapter.hpp"
#include "club_control.hpp"
#include <algorithm>
#include <vector>

namespace cm::aot {
bool adaptive_screen_enabled(std::uint32_t builder) {
    return new_screens_enabled() && screen_template(builder) != ScreenTemplate::legacy;
}
namespace {
// The local, non-modal screen whose history node was opened by `builder`:
// its manager and history, or zeros. Network screens share the original
// protocol's 800x600 geometry and modal groups keep theirs.
struct OpenScreen { std::uint32_t manager = 0, history = 0; };
OpenScreen open_screen(const State& s, std::uint32_t builder) {
    const auto read = [&](std::uint32_t at) { return std::uint32_t(s.memory.read<4>(at)); };
    const auto manager = read(std::uint32_t(s.r<4>(8)) - 0x7bb0);
    if (!manager || s.memory.read<2>(manager + 0x2c58) || read(manager + 0x2c64)) return {};
    const auto history = read(manager + 0x10);
    if (!history) return {};
    const auto descriptor = read(history);
    if (!descriptor || read(descriptor) != builder) return {};
    return {manager, history};
}
bool club_tables_available(const State& s) {
    switch (adaptive_club_section(s)) {
    case 1: case 2: case 3: case 4: return true;
    case 5: {
        const auto toc=std::uint32_t(s.r<4>(8));
        const auto read=[&](std::uint32_t at){return std::uint32_t(s.memory.read<4>(at));};
        const auto manager=read(toc-0x7bb0),history=read(manager+0x10);
        const auto view=read(history+0x14+27*8)&31;
        // Public information and statistics exist for foreign clubs and
        // national teams too; only financial ledgers require human control.
        if(view==1 || view==16)return read(history+0x14)!=0;
        if(view!=2 && view!=4 && view!=8)return false;
        const auto club=read(history+0x14),instance=read(manager+4);
        const auto clubs=read(toc-0x7b50),nations=read(toc-0x7b04);
        if(!club || !instance || !clubs || !nations || read(club+0x90)!=instance)return false;
        const auto total=read(clubs),national=read(nations);
        if(national>total/2 || read(club)>=total-2*national)return false;
        // The own normal club uses the same absence/holiday veto as the
        // original finance view. Unavailable/foreign/national views stay old.
        return club_control::human_controlled(s,club,0,1,instance,0).value_or(false);
    }
    default: return false;
    }
}
}
bool adaptive_screen_available(const State& s, std::uint32_t builder) {
    if (!adaptive_screen_enabled(builder)) return false;
    const auto kind = screen_template(builder);
    if (kind == ScreenTemplate::tactics) return true;
    const auto toc = std::uint32_t(s.r<4>(8));
    const auto read = [&](std::uint32_t at) { return std::uint32_t(s.memory.read<4>(at)); };
    if (kind == ScreenTemplate::club_tables) return club_tables_available(s);
    const auto [manager, history] = open_screen(s, builder);
    if (!history) return false;
    const auto value = [&](unsigned index) { return read(history + 0x14 + index * 8); };
    const auto instance = read(manager + 4);
    switch (kind) {
    case ScreenTemplate::league_selection:
        return !instance && !s.memory.read<1>(read(toc - 0x79e4));
    case ScreenTemplate::load_browser: {
        const auto extension = value(4), filter = value(5);
        // FILE_SCREENS also serves quick-start, tactics and folder choosers. Opt in only to the
        // saved-career workflows: Load (mode 0), Save (1) and Delete (2), from the start screen or
        // inside a career, with or without the Info button. The layout itself declines any other
        // arrangement of grids.
        if ((value(2) & 0xff) > 2 || value(13) || value(17) || value(18) > 1 ||
            !value(3) || !value(6) || !value(7) || !value(8) || !extension || !filter || read(filter) != 0x10355820) return false;
        constexpr char suffix[] = ".sav";
        for (unsigned i = 0; i < sizeof(suffix); ++i)
            if (s.memory.read<1>(extension + i) != std::uint8_t(suffix[i])) return false;
        return true;
    }
    case ScreenTemplate::startup:
        // First-run setup places a covering grid over the menu before opening
        // mandatory settings. Let that original transition keep its geometry.
        return !value(0);
    case ScreenTemplate::match_report: {
        const auto tab = value(2);
        if (builder == 0x1028c430) {
            // Mode 1 draws only the transition. The live builder has no tab 5.
            return value(0) && value(1) != 1 && tab >= 1 && tab <= 10 && tab != 5;
        }
        return !value(0) && std::int32_t(value(30)) >= 0 && tab >= 1 && tab <= 7;
    }
    case ScreenTemplate::daily_fixtures: {
        const auto queue_slot = read(toc - 0x7420);
        const auto queue = queue_slot ? read(queue_slot) : 0;
        if (!queue) return false;
        const auto group = value(1), groups = read(queue + 0x10), count = read(queue + 0x18);
        if (!group || !groups || group < groups || (group - groups) % 0x20 || (group - groups) / 0x20 >= count) return false;
        const auto first = std::int32_t(read(group + 4)), last = std::int32_t(read(group + 8));
        const auto entries = read(queue + 0x1c);
        return first >= 0 && last >= first && std::uint32_t(last) < read(queue + 0x24) &&
               entries && read(entries + std::uint32_t(first) * 0x6c);
    }
    case ScreenTemplate::transfer_history:
        return transfer_browser_native_enabled();
    case ScreenTemplate::training_schedule:
    case ScreenTemplate::training_coaches:
        return value(0) && value(1) && value(2);
    case ScreenTemplate::training:
        return value(0) && s.memory.read<1>(value(0) + 0x860) < 10;
    case ScreenTemplate::nations_and_clubs: {
        const auto tab = value(0), mode = value(19);
        // The single-list nation picker selects tab 1 during construction.
        if (mode == 2) return value(1) != 0;
        if (mode > 1 || tab < 1 || tab > 6) return false;
        constexpr unsigned selection_slot[]{1, 4, 7, 10, 13, 16};
        return value(selection_slot[tab - 1]) != 0;
    }
    case ScreenTemplate::competition_history:
        return value(0) && value(1) >= 1 && value(1) <= 7;
    case ScreenTemplate::competition: {
        const auto competition = value(0);
        if (!competition) return false;
        const auto section = value(1);
        // Results, Calendar and Match Dates list games for the competition as
        // a whole, including competitions with knockout stages.
        if (section >= 2 && section <= 4) return true;
        if (section != 1 || value(4) > 2) return false;
        const auto last = std::int32_t(read(competition + 0x30));
        const auto selected = std::int32_t(value(2));
        if (last < -1 || last > 127 || selected < -2 || selected > last) return false;
        const auto stages = read(competition + 0xc);
        if (last >= 0 && !stages) return false;
        const auto supported = [&](std::uint32_t stage) {
            if (!stage) return false;
            const auto kind = s.memory.read<1>(stage + 0x43);
            if (kind == 1) return true; // CUP_STAGE draw, including its pending-draw message
            if (kind != 2) return false;
            return std::int16_t(s.memory.read<2>(stage + 0x3e)) > 0;
        };
        if (selected >= -1) return supported(selected == -1 ? competition : read(stages + selected * 4));
        // The original current-stage query can finalize competition records.
        // Never call it from presentation. Before it resolves history2, opt in
        // only when every possible stage belongs to this supported family.
        if (!supported(competition)) return false;
        for (int index = 0; index <= last; ++index)
            if (!supported(read(stages + index * 4))) return false;
        return true;
    }
    case ScreenTemplate::news_inbox:
        return instance && value(1) < 8 && value(8);
    case ScreenTemplate::message_screen:
        // The layout moves the grids of the shared shape and declines any other arrangement.
        return instance && read(instance + 0x24);
    case ScreenTemplate::board_confidence: {
        // The opener stores the manager's own club; the builder reads its record.
        const auto clubs_slot = read(toc - 0x7be8), total_slot = read(toc - 0x7b50);
        if (!clubs_slot || !total_slot) return false;
        const auto clubs = read(clubs_slot), total = read(total_slot), club = value(0);
        return instance && read(instance + 0x24) && clubs && total && club >= clubs &&
               club < clubs + total * 0x248 && (club - clubs) % 0x248 == 0;
    }
    case ScreenTemplate::manager_statistics:
    case ScreenTemplate::world_rankings:
    case ScreenTemplate::uefa_coefficients:
    case ScreenTemplate::hall_of_fame:
    case ScreenTemplate::manager_status:
    case ScreenTemplate::job_information:
    case ScreenTemplate::holiday:
        // Tables of the whole game's managers or nations and the holiday dialog: the layout reads no record of the history.
        return instance && read(instance + 0x24);
    case ScreenTemplate::friendly_countries:
    case ScreenTemplate::friendly_dates:
        // The layout reads no record of the history; a friendly is arranged by a managed club.
        return instance && read(instance + 0x24);
    case ScreenTemplate::text_entry:
        // The layout reads no record; the dialogs name a person of the career.
        return instance && read(instance + 0x24);
    case ScreenTemplate::credits:
    case ScreenTemplate::web_sites:
        // Plain text pages of the Game menu, also reached from the start menu before any career exists.
        return true;
    case ScreenTemplate::bid_rejection: {
        // The builder takes the offer from history 0, or from history 1 when that is empty, and reads
        // the player it names and the manager's club.
        const auto count = read(toc - 0x7be4);
        const auto link = value(0) ? value(0) : value(1);
        const auto offer = link ? read(link) : 0;
        return instance && read(instance + 0x24) && count && offer && read(offer + 12) < read(count);
    }
    case ScreenTemplate::transfer_status:
    case ScreenTemplate::previous_offers: {
        // History 0 is a person ID for the status screen and the offer record for previous offers.
        // The builders read the person (and the bidder club) from it, and the manager's club.
        const auto count = read(toc - 0x7be4), clubs = read(toc - 0x7b50);
        if (!instance || !read(instance + 0x24) || !count || !clubs) return false;
        if (kind == ScreenTemplate::transfer_status) return value(0) < read(count);
        const auto offer = value(0);
        return offer && read(offer + 12) < read(count) && read(offer + 8) < read(clubs);
    }
    case ScreenTemplate::loan_terms:
    case ScreenTemplate::negotiation_confirmation: {
        if (!instance || !read(instance + 0x24)) return false;
        if (builder == 0x1009dc00) {
            // The confirmation reads the current human's embedded contract
            // draft. Keep staff roles outside the player negotiation recipe.
            const auto role = value(4);
            return (role >= 11 && role <= 13) || role == 15;
        }
        const auto count = read(toc - 0x7be4);
        if (!count) return false;
        auto subject = value(0);
        if (builder == 0x1041eb10) {
            // Proposal confirmation owns a link; loan screens own a person
            // pointer. Neither is the person ID stored by proposal editing.
            if (!subject) subject = value(2);
            if (!subject) return false;
            const auto offer = read(subject);
            return offer && read(offer + 12) < read(count);
        }
        return subject && read(subject) < read(count) && read(subject + 0x14);
    }
    case ScreenTemplate::transfer_proposal:
    case ScreenTemplate::contract_negotiation: {
        const auto count = read(toc - 0x7be4);
        // The proposal stores a person ID, unlike the player-details screen.
        // Its original builder creates the transient draft on first entry.
        if (!count || value(0) >= read(count) || !instance) return false;
        if (kind == ScreenTemplate::contract_negotiation) {
            const auto tab = value(1), role = value(8);
            // Match the original playing-role predicate; staff negotiations
            // have a different, tabless form and retain their legacy layout.
            return tab >= 1 && tab <= 3 && ((role >= 11 && role <= 13) || role == 15) &&
                   read(instance + 0x24);
        }
        return true;
    }
    case ScreenTemplate::search_tables:
    case ScreenTemplate::search_filters: {
        if (kind == ScreenTemplate::search_filters) {
            if (!value(1)) return false;
            const auto tab = value(2);
            if (tab != 1 && tab != 2) return false;
        }
        const auto engine = value(0);
        if (!engine) return false;
        const auto preset = std::int8_t(s.memory.read<1>(engine));
        if (preset < 0 || preset >= 10) return false;
        const auto mode = s.memory.read<1>(engine + 4 + std::uint32_t(preset) * 0x2b0 + 0xd);
        return mode >= 1 && mode <= 4;
    }
    case ScreenTemplate::player_details: {
        const auto section = value(1);
        if (section < 1 || section > 5) return false;
        if (section == 5) {
            switch (value(5)) {
            case 1: case 2: case 4: case 8: case 16: break;
            default: return false;
            }
        }
        const auto staff = value(0);
        const auto count = read(toc - 0x7be4);
        // The final database entries are sentinels. Staff-only profiles also
        // need the non-playing attributes table emitted for their status.
        if (!staff || !count ||
            std::int32_t(read(staff)) >= std::int32_t(read(count) - 16)) return false;
        const auto status = s.memory.read<1>(staff + 0x62);
        if (!read(staff + 0x14))
            return section == 1 && value(4) == 1 && read(staff + 0x1c) && status != 1 && status != 2;
        if (section == 1 && value(4) != 0) return false;
        if (section != 2 && (status == 2 || (section != 1 && status == 1))) return false;
        return section != 3 || s.memory.read<1>(staff + 0x60) != 5 ||
            (read(staff + 0x24) && status == 11);
    }
    default: return true;
    }
}
std::uint32_t adaptive_club_section(const State& s) {
    const auto toc = std::uint32_t(s.r<4>(8));
    const auto manager = std::uint32_t(s.memory.read<4>(toc - 0x7bb0));
    if (!manager || s.memory.read<2>(manager + 0x2c58) != 0 || s.memory.read<4>(manager + 0x2c64)) return 0;
    const auto history = std::uint32_t(s.memory.read<4>(manager + 0x10));
    return history ? s.memory.read<4>(history + 0x14 + 4 * 8) : 0;
}
GuiCanvas gui_canvas(const State& s) {
    return s.platform ? s.platform->gui_canvas_size() : GuiCanvas{};
}
namespace {
void draw_sidebar_background(State& s, std::uint32_t control, GuiCanvas canvas) {
    guest::Cpu c(s);
    // GUIO's bitmap restore ignores the control's right/bottom bounds. Extend
    // only the career menubar artwork; leave every other image and all glyphs
    // to the original renderer. Its source header/pixels are never modified.
    if(c.u32(control+0xc)!=1024 || c.i32(control+0x10)!=0 || c.i32(control+0x14)!=0 ||
       c.i32(control+0x18)!=89 || c.i32(control+0x1c)!=canvas.height-1)return;
    const auto image=c.u32(control+0x5c);
    if(!image)return;
    const auto width=c.i32(image),height=c.i32(image+4);
    const auto pixels=c.u32(image+12),destination=c.u32(c.r(2)-0x3564);
    const auto stride=c.u16(c.r(2)-0x3566);
    if(width<=0 || height<=0 || width>3200 || height>1800 || !pixels || !destination ||
       stride<canvas.width*2 || (width==90 && height==canvas.height))return;
    const auto source=s.memory.copy_out(pixels,std::uint64_t(width)*height*2);
    std::vector<std::uint8_t> row(180);
    for(int y=0;y<canvas.height;++y) {
        const int sy=y*height/canvas.height;
        for(int x=0;x<90;++x) {
            const auto from=(std::size_t(sy)*width+x*width/90)*2;
            row[x*2]=source[from]; row[x*2+1]=source[from+1];
        }
        s.memory.copy_in(destination+y*stride,row);
    }
}
}
bool adaptive_gui_instruction(State& s, std::uint32_t pc) {
    const auto canvas = gui_canvas(s);
    if (canvas.original()) return false;
    s.step(pc);
    guest::Cpu c(s);
    const auto right = std::uint32_t(canvas.width - 1), bottom = std::uint32_t(canvas.height - 1);
    const auto subtract = [&](unsigned reg, unsigned from, std::uint32_t limit) {
        const auto result = std::uint64_t(~c.r(from)) + limit + 1;
        c.set(reg, std::uint32_t(result));
        s.wr<1>(guest::reg::xer_ca, result >> 32);
    };
    switch (pc) {
    case 0x1001f064: {
        // Information and Next Match build fixed-capacity panels. Keep their
        // original construction bounds; resize the actual branch afterward.
        const auto section=adaptive_club_section(s);
        c.set(0, section==3 || section==5 ? 545 : ScreenLayout{canvas}.content().bottom); break;
    }
    case 0x10726050:
        draw_sidebar_background(s,c.r(31),canvas);
        c.set(0,c.u8(c.r(31)+0x17a)); break;
    // Career sidebar background, grids and bottom Continue/status controls.
    case 0x102a1188: c.set(7, canvas.height - 1); break;
    case 0x102a1244: case 0x102a12ec: case 0x102a18b0:
    case 0x102a47c8: c.set(7, canvas.height - 10); break;
    case 0x102a4b6c: case 0x102a4bec: case 0x102a4ce8:
    case 0x102a4d68: case 0x102a4e70: case 0x102a4eec:
    case 0x102a4f8c: case 0x102a5004: c.set(6, canvas.height - 50); break;
    case 0x102a4b78: case 0x102a4bf8: case 0x102a4cf4:
    case 0x102a4d74: case 0x102a4e7c: case 0x102a4ef8:
    case 0x102a4f98: case 0x102a5010: c.set(8, canvas.height - 10); break;
    // Processing bar and its partial repaint use the committed canvas, even after history is cleared.
    case 0x1076e6c0: case 0x1076e798: c.set(4, canvas.height - 45); break;
    case 0x1076e6c4: case 0x1076e79c: c.set(5, canvas.width - 10); break;
    case 0x1076e6c8: case 0x1076e7a0: c.set(6, canvas.height - 10); break;
    case 0x1076e6f8: case 0x1076e71c: case 0x1076e778: c.set(4, canvas.height - 35); break;
    case 0x1076e704: case 0x1076e724: case 0x1076e784: c.set(6, canvas.height - 20); break;
    case 0x1076e720: case 0x1076e77c: c.set(5, canvas.width - 30); break;
    case 0x1076e738: c.set(0, canvas.width - 32); break;
    case 0x1076e748: c.set(4, canvas.height - 33); break;
    case 0x1076e750: c.set(6, canvas.height - 22); break;
    case 0x1031bdcc: c.set(3, ScreenLayout{canvas}.content().bottom); break;
    // Search uses a separate one-row heading: total capacity 31 leaves at
    // most 30 body rows, and its original pagination reads this same value.
    case 0x1031d200: c.set(0, std::min(31u, c.r(5) & 0xffu)); break;
    // AutoGrid supports at most 30 visible rows (the detailed views reserve
    // one for the heading). Keep its original scrollable player collection.
    case 0x100240d0: c.set(16, std::min(30u, c.r(3) & 0xffu)); break;
    case 0x10024190: c.set(16, std::min(30u, c.r(8) & 0xffu)); break;
    case 0x10029b4c: c.set(22, std::min(30u, c.r(0) & 0xffu)); break;
    case 0x1002e48c: c.set(10, std::min(30u, c.r(3) & 0xffu)); break;
    case 0x10718f8c: case 0x10718fa0: case 0x10719074:
    case 0x1071929c: case 0x107192c8: case 0x10719318: c.set(5, right); break;
    case 0x10718f90: case 0x10718fa4: case 0x10719078:
    case 0x107192a0: case 0x107192cc: case 0x1071931c: c.set(6, bottom); break;
    case 0x10713e1c: case 0x10714f88:
    case 0x10725b04: case 0x1071b074: c.compare(0, std::int32_t(c.r(0)), std::int32_t(right)); break;
    case 0x10713e28: case 0x10725b10: c.set(0, right); break;
    case 0x10713e34: case 0x10715054:
    case 0x10725b1c: case 0x1071b0a0: c.compare(0, std::int32_t(c.r(0)), std::int32_t(bottom)); break;
    case 0x10713e40: case 0x10725b28: c.set(0, bottom); break;
    case 0x10714d00: case 0x10714dbc: case 0x10714e34:
        c.compare(0, std::int32_t(c.r(0)), canvas.height); break;
    case 0x10714df4: c.compare(0, std::int32_t(c.r(0)), canvas.width / 2); break;
    case 0x10714e84: c.compare(0, std::int32_t(c.r(6)), canvas.width / 2); break;
    case 0x10714fe4: subtract(28, 0, right); break;
    case 0x107150b0: subtract(27, 0, bottom); break;
    case 0x1071b07c: subtract(27, 4, right); break;
    case 0x1071b0a8: subtract(30, 5, bottom); break;
    case 0x1076add4: c.set(3, canvas.width); break;
    case 0x1076ae34: c.set(4, canvas.width); break;
    case 0x10769c40: subtract(3, 4, canvas.width); break;
    case 0x1074b0c4: c.set(5, canvas.height - 45); break;
    case 0x1074b0cc: c.set(7, canvas.height - 10); break;
    case 0x1074b100: c.set(6, canvas.width - 10); break;
    default: return false;
    }
    return true;
}
void draw_adaptive_background(State& s) {
    const auto canvas = gui_canvas(s);
    if (canvas.original()) return;
    guest::Cpu c(s);
    const auto image = c.u32(c.r(24) + 0x12a928);
    if (!image) return;
    const auto width = c.i32(image), height = c.i32(image + 4);
    if (width <= 0 || height <= 0 || width > 3200 || height > 1800) return;
    // Full-canvas captures already carry the observer's original vector
    // glyphs. Repainting their bitmap would flatten those contours.
    if (width == canvas.width && height == canvas.height) return;
    const auto pixels = c.u32(image + 12), destination = c.u32(c.r(2) - 0x3564);
    const auto stride = c.u16(c.r(2) - 0x3566);
    if (!pixels || !destination || stride < canvas.width * 2) return;
    const auto source = s.memory.copy_out(pixels, std::uint64_t(width) * height * 2);
    std::vector<std::uint8_t> row(std::size_t(canvas.width) * 2);
    // Cover the extra canvas with the selected original image, keeping its
    // aspect ratio. Only the background is sampled; text/control geometry
    // and original font contours are rendered afterwards at their own size.
    const double scale = std::max(double(canvas.width) / width, double(canvas.height) / height);
    const double offset_x = (width * scale - canvas.width) / 2;
    const double offset_y = (height * scale - canvas.height) / 2;
    for (int y = 0; y < canvas.height; ++y) {
        const auto sy = std::clamp(int((y + offset_y) / scale), 0, height - 1);
        for (int x = 0; x < canvas.width; ++x) {
            const auto sx = std::clamp(int((x + offset_x) / scale), 0, width - 1);
            const auto from = (std::size_t(sy) * width + sx) * 2;
            row[x * 2] = source[from]; row[x * 2 + 1] = source[from + 1];
        }
        s.memory.copy_in(destination + y * stride, row);
    }
}
}
