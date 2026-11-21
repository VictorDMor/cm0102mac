// Supply one input already obtained from the real picker. Stop on the next
// input poll, after the production monitor handles Back and rebuilds the list.
struct ProfileMonitorInput {
    struct Returned {};
    static inline int control = -1;
    decltype(dispatch_probe) previous = dispatch_probe;
    explicit ProfileMonitorInput(int back) { control = back; dispatch_probe = poll; }
    ~ProfileMonitorInput() { dispatch_probe = previous; }
    static bool poll(State& s) {
        if (s.pc != 0x10719350) return false;
        if (control < 0) throw Returned{};
        guest::Cpu c(s);
        c.set(3, control);
        control = -1;
        s.pc = c.lr();
        return true;
    }
};

void check_staff_profiles(State& s, const GuestScreen& screen, unsigned engine,
                          const auto& invoke, const auto& hit, const auto& draw_coaches) {
    auto& c = screen.c;
    constexpr unsigned builder = 0x10389ce0, coaches_handler = 0x103de370;
    const auto display = screen.display, manager = c.toc(-0x7bb0), parent = c.u32(manager + 0x10);
    const auto draft = c.u32(parent + 0x1c);
    const auto assignments = s.memory.copy_out(draft, 0x48);
    const auto find_action = [&](int action, unsigned row = 0) {
        for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
            const auto at = screen.control(i);
            if (c.i32(at + 8) == action && c.u32(at + 0x24) == row) return i;
        }
        throw std::runtime_error("Missing staff profile control");
    };
    for (const unsigned row : {0u, 4u}) {
        draw_coaches({1280,900}, true);
        const auto link = find_action(2007, row);
        const auto staff = c.u32(screen.control(link) + 0x48);
        require(hit(link) == int(link) && invoke(coaches_handler, {link}) == -4, "Open staff profile from coach name");
        const auto page = c.u32(manager + 0x10);
        require(c.u32(c.u32(page)) == builder && c.u32(page + 0x14) == staff && c.u32(page + 0x1f4) == parent,
                "Coach name opened a different profile");
        const auto status = c.u8(staff + 0x62);
        for (unsigned excluded : {1u, 2u}) {
            c.put8(staff + 0x62, excluded);
            require(!adaptive_screen_available(s, builder), "Non-attribute staff view opted in");
        }
        c.put8(staff + 0x62, status);
        c.put32(page + 0x1c, 3);
        require(!adaptive_screen_available(s, builder), "Staff contract opted into profile layout");
        c.put32(page + 0x1c, 1);
        const auto draw = [&](GuiCanvas canvas, bool enabled) {
            select_new_screens(enabled);
            require(adaptive_screen_available(s, builder) == enabled, "Staff profile eligibility");
            require(s.platform->resize_gui_canvas(s, canvas), "Staff profile resize");
            invoke(0x10719560, {display,1,0});
            invoke(builder, {});
            const auto before = s.memory.copy_out(display, 0x129da8);
            const auto person = s.memory.copy_out(staff, 0x70), attributes = s.memory.copy_out(c.u32(staff + 0x1c), 0x40);
            const auto schedules = s.memory.copy_out(engine, 0x86c), history = s.memory.copy_out(page, 0x1fc);
            const auto rng = random_state(s), stack = s.memory.copy_out(c.sp() - 0x8000, 0x8100);
            const auto regs = s.registers; const auto unique = s.unique; const auto pc = s.pc;
            layout_built_screen(s, display, builder);
            require(regs == s.registers && unique == s.unique && pc == s.pc &&
                    stack == s.memory.copy_out(c.sp() - 0x8000, 0x8100), "Staff profile changed guest ABI");
            require(rng == random_state(s) && history == s.memory.copy_out(page, 0x1fc) &&
                    schedules == s.memory.copy_out(engine, 0x86c) && person == s.memory.copy_out(staff, 0x70) &&
                    attributes == s.memory.copy_out(c.u32(staff + 0x1c), 0x40), "Staff profile changed game state");
            if (canvas.original() || !enabled) {
                require(before == s.memory.copy_out(display, 0x129da8), "Classic staff profile changed");
                return;
            }
            for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
                screen.measure(screen.grid(i)); screen.publish(screen.grid(i));
            }
            const auto back = find_action(-2);
            const int pref = c.i16(screen.control(back) + 0x72) - 1;
            const auto preferences = screen.rect(screen.grid(pref));
            const bool wide = canvas.width == 1280;
            unsigned ratings = 0;
            int last_bottom = 0;
            for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
                const auto at = screen.control(i);
                const auto original = before.begin() + (at - display);
                auto unchanged = [&](unsigned off, unsigned len) {
                    const auto bytes = s.memory.copy_out(at + off, len);
                    return std::equal(bytes.begin(), bytes.end(), original + off);
                };
                require(unchanged(8,8) && unchanged(0x48,4) && unchanged(0x78,0x100), "Staff profile text, flags or action changed");
                if (c.i16(at + 0x72) != pref - 1 || c.u32(at + 0x20) % 2 != 1 || !c.u8(at + 0x78)) continue;
                const auto box = screen.rect(at + 0x10);
                if (c.u8(at + 0x78) >= '0' && c.u8(at + 0x78) <= '9') {
                    ++ratings;
                    if (wide) require(box.right < preferences.left && box.top > last_bottom, "Numeric attributes overlap");
                    last_bottom = box.bottom;
                } else if (wide) require(box.left > preferences.left && box.bottom < preferences.top, "Coaching style misplaced");
                if (!wide) require(box.bottom < preferences.top, "Narrow preferences overlap ratings");
            }
            require(ratings == (row == 0 ? 12 : 11), "Wrong number of numeric ratings");
            require(preferences.width() >= 670 && screen.rect(screen.control(back) + 0x10).top > preferences.bottom,
                    "Staff preferences or navigation clipped");
        };
        draw({800,600}, true);
        draw({1280,900}, false);
        draw({801,601}, true);
        draw({1280,900}, true);
        const auto back = find_action(-2);
        require(hit(back) == int(back), "Staff profile Back hit box");
        {
            GuiGuestScope preserve(s, 0x100);
            ProfileMonitorInput input(back);
            try { invoke(0x1073c730, {manager}); }
            catch (const ProfileMonitorInput::Returned&) {}
        }
        require(c.u32(manager + 0x10) == parent && c.u32(parent + 0x1c) == draft &&
                assignments == s.memory.copy_out(draft, 0x48) && find_action(2007, row) == link,
                "Back did not restore the same coaching assignments");
    }
}
