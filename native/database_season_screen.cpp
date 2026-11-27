#include "database_seasons.hpp"
#include "adaptive_gui.hpp"
#include "form_layout.hpp"
#include "gui_guest_scope.hpp"
#include "platform.hpp"

namespace cm::aot {
namespace {
constexpr unsigned league_builder = 0x10335500, league_handler = 0x103366f0;
bool local_setup(guest::Cpu& c) {
    const auto manager = c.toc(-0x7bb0);
    return manager && !c.u16(manager + 0x2c58) && !c.u32(manager + 0x2c64) &&
           !c.u32(manager + 4) && !c.u8(c.toc(-0x79e4));
}
unsigned invoke(guest::Cpu& c, unsigned entry, std::initializer_list<unsigned> args = {}) {
    unsigned reg = 3;
    for (const auto arg : args) c.set(reg++, arg);
    try { c.call(entry, 0xfffffff0); }
    catch (const std::exception& e) {
        throw std::runtime_error("Season setup " + hex_address(entry) + " at " + hex_address(c.s.pc) + ": " + e.what());
    }
    return c.r(3);
}
void write_text(guest::Cpu& c, unsigned at, std::string_view value) {
    for (std::size_t i = 0; i <= value.size(); ++i) c.put8(at + i, i == value.size() ? 0 : value[i]);
}
void load_selection(guest::Cpu& c, DatabaseSeasons& seasons, std::size_t index) {
    GuiGuestScope preserve(c.s);
    c.set(1, c.sp() - 0x400);
    seasons.activate(index);
    // Actual current date, not DATABASE's historical contract-law cutoff.
    invoke(c, 0x107158a0, {c.toc(-0x7b80), 1, 0, seasons.year(), 0xffffffff});
    const auto path = c.sp() + 0x100;
    write_text(c, path, *seasons.data_path("SI_DATA", ""));
    if (!invoke(c, 0x1009ef90, {path, 1})) throw std::runtime_error("Database load failed");
    invoke(c, 0x1013feb0, {c.u32(c.toc(-0x7ac8))});
    invoke(c, 0x10343200);
    if (!invoke(c, 0x10190180)) throw std::runtime_error("League initialization failed");
    database_season_limit_leagues(c.s);
}
}
void database_season_before_build(State& s, std::uint32_t builder) {
    if (!s.platform) return;
    auto& seasons = s.platform->database_seasons();
    if (!seasons.enabled()) return;
    guest::Cpu c(s);
    if (!local_setup(c)) return;
    if (builder == 0x10334550) { seasons.end(); return; }
    if (builder != league_builder) return;
    seasons.previous_control = seasons.next_control = seasons.historical_control = -1;
    if (!seasons.active()) seasons.begin(c.u32(c.toc(-0x7b98)) != 0);
    const auto pending = seasons.pending();
    if (!pending) { database_season_limit_leagues(s); return; }
    const auto previous = seasons.loaded().value_or(seasons.selected());
    try { seasons.preflight(*pending); seasons.preflight(previous); }
    catch (const std::exception& e) {
        seasons.activate(previous);
        seasons.complete(); seasons.error = e.what(); return;
    }
    try {
        load_selection(c, seasons, *pending);
        seasons.error.clear();
    } catch (const std::exception& e) {
        const std::string failure = e.what();
        // The original loader releases its old tables before reading. Merely
        // restoring a label would leave dangling pointers; reload the old pack.
        seasons.preflight(previous);
        load_selection(c, seasons, previous);
        seasons.error = failure;
    }
    seasons.complete();
}
bool database_season_event(State& s, std::uint32_t handler, std::int16_t control) {
    if (!s.platform) return false;
    auto& seasons = s.platform->database_seasons();
    if (!seasons.enabled()) return false;
    guest::Cpu c(s);
    if (handler == 0x10334940 && local_setup(c)) {
        const auto manager = c.toc(-0x7bb0), display = c.u32(manager + 0x24);
        auto category = control < 0 ? 0 : c.i16(display + 0xba86e + unsigned(control) * 0x17c);
        if (!category) category = c.i16(manager + 0x25819e);
        if (category == 1) {
            // Route BEFORE the original opener reads tables/language. Its
            // ordinary cold load and RNG sequence remain the only load.
            try { seasons.preflight(seasons.default_index()); }
            catch (const std::exception& e) { seasons.error = e.what(); return true; }
            seasons.begin(c.u32(c.toc(-0x7b98)) != 0);
            if (c.u16(c.toc(-0x7b80) + 2) != 2001) {
                GuiGuestScope preserve(s); c.set(1, c.sp() - 0x400);
                invoke(c, 0x107158a0, {c.toc(-0x7b80), 1, 0, 2001, 0xffffffff});
            }
        }
        return false;
    }
    if (handler != league_handler || !local_setup(c)) return false;
    if (control >= 0 && control == seasons.historical_control) {
        seasons.historical_rules = !seasons.historical_rules;
        return true;
    }
    if (control < 0 || (control != seasons.previous_control && control != seasons.next_control)) return false;
    // Selection is deferred until the monitor has destroyed the old controls.
    seasons.request(control == seasons.previous_control ? -1 : 1);
    return true;
}
void database_season_after_build(State& s, std::uint32_t builder) {
    if (!s.platform || (builder != league_builder && builder != 0x10334550)) return;
    auto& seasons = s.platform->database_seasons();
    guest::Cpu c(s);
    if (!local_setup(c)) return;
    const auto manager = c.toc(-0x7bb0), display = c.u32(manager + 0x24);
    const bool selector = builder == league_builder && seasons.enabled() && seasons.active();
    const bool adaptive = adaptive_screen_available(s, builder);
    if (builder == league_builder && (adaptive || selector)) layout_league_selection(s, display, gui_canvas(s), selector);
    if (!selector && seasons.error.empty()) return;
    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    auto panel = ScreenLayout{gui_canvas(s)}.content();
    const int width = std::min(1120, panel.width());
    panel.left += (panel.width() - width) / 2; panel.right = panel.left + width;
    const int center = (panel.left + panel.right) / 2;
    const auto ink = c.u16(c.toc(-0x7bc8)), background = c.u16(c.toc(-0x7bc4));
    const auto control = [&](int left, int right, std::string_view label, bool button, bool enabled, unsigned action) {
        write_text(c, c.sp() + 0x100, label);
        c.put32(c.sp() + 0x38, 1);
        c.put32(c.sp() + 0x3c, ink);
        c.put32(c.sp() + 0x40, c.sp() + 0x100);
        c.put32(c.sp() + 0x44, action);
        c.put32(c.sp() + 0x48, 0);
        return std::int16_t(invoke(c, button ? 0x10744430 : 0x10744390,
            {manager, unsigned(left), unsigned(panel.top), unsigned(right), unsigned(panel.top + 30),
             button ? (enabled ? 0x30u : 0x830u) : 1u, button ? background : 0u, 12}));
    };
    if (selector) {
        seasons.previous_control = control(center - 135, center - 101, "<", true, seasons.selected() > 0, 0x7000);
        control(center - 96, center + 96, seasons.packs()[seasons.selected()].label, false, true, 0);
        seasons.next_control = control(center + 101, center + 135, ">", true, seasons.selected() + 1 < seasons.packs().size(), 0x7001);
        const bool portuguese = c.u8(c.r(2) - 0x35ac) == 9;
        const auto label = portuguese ? (seasons.historical_rules ? "Regras realistas: Sim" : "Regras realistas: N\x8bo")
                                      : (seasons.historical_rules ? "Historical rules: On" : "Historical rules: Off");
        seasons.historical_control = control(center + 145, panel.right - 8, label, true, true, 0x7002);
    }
    // Keep file failures visible; the previous complete database remains selected.
    if (!seasons.error.empty()) {
        const bool portuguese = c.u8(c.r(2) - 0x35ac) == 9;
        write_text(c, c.sp() + 0x100, portuguese ? "Nao foi possivel carregar esta base." : "This database could not be loaded.");
        c.put32(c.sp() + 0x38, 1); c.put32(c.sp() + 0x3c, ink); c.put32(c.sp() + 0x40, c.sp() + 0x100);
        invoke(c, 0x10744390, {manager, unsigned(panel.left), 128, unsigned(panel.right), 143, 1, 0, 12});
    }
}
}
