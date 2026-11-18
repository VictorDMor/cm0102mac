// The December 2001 daily-sort capture includes both Champions League group phases.
void group_stage_builder(const char* snapshot, const std::filesystem::path& user) {
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    Clock clock; clock.idle_input = true;
    auto state = std::make_unique<State>();
    auto& s = *state;
    load_match_snapshot(s, snapshot);
    s.code_fragment_observer = &clock;
    guest::Cpu c(s);
    c.set(2, 0x108bba90);
    const auto manager = c.toc(-0x7bb0);
    c.put16(manager + 0x2c58, 0);
    s.platform = std::make_shared<NativePlatform>(temp.path/"game", user, AudioMode::offline);
    const auto scratch = allocate(s, 32, true);
    world(s, scratch, scratch + 16);
    require(s.platform->resize_gui_canvas(s, {801,601}), "Initialize group screen");
    const auto call = [&](unsigned entry, std::initializer_list<unsigned> args) {
        unsigned reg = 3;
        for (auto arg : args) c.set(reg++, arg);
        s.budget = s.executed + 400000000;
        c.call(entry, return_sentinel);
        return c.r(3);
    };
    const auto cup_id = c.u32(c.toc(-0x7a30));
    call(0x100648e0, {c.u32(c.toc(-0x7bbc)) + cup_id * 0x6c, 0, 0, 0});
    auto history = c.u32(manager + 0x10);
    const auto display = c.u32(manager + 0x24);
    const auto slot = [&history](unsigned n) { return history + 0x14 + 8*n; };
    const auto cup = c.u32(slot(0));
    require(c.u32(cup) == c.toc(-0x6094), "Expected Champions League");
    const GuestScreen screen{c, display};
    const auto find_control = [&](const auto& matches) {
        for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i)
            if (matches(screen.control(i))) return i;
        throw std::runtime_error("Missing Champions League control");
    };
    const auto hit = [&](unsigned index) {
        const auto box = screen.rect(screen.control(index) + 0x10);
        c.put32(c.toc(-0x5c10), (box.left+box.right)/2);
        c.put32(c.toc(-0x5c14), (box.top+box.bottom)/2);
        c.put8(c.toc(-0x7bac),1); c.put8(c.toc(-0x5c0c),0);
        return std::int16_t(call(c.u8(display) == 1 ? 0x1071aa10 : 0x1071a280, {display}));
    };
    int table = -1, games = -1;
    const auto build = [&](GuiCanvas size, bool enabled) {
        select_new_screens(enabled);
        require(s.platform->resize_gui_canvas(s, size), "Resize group canvas");
        call(0x10719560, {display,1,0});
        call(0x10065340, {});
        table = games = -1;
        for (unsigned i = 0; i < c.u16(display+0x129da4); ++i) {
            const auto at = screen.grid(i);
            if (screen.rect(at).left != 110) continue;
            if (screen.columns(at) == 11) table = int(i);
            if (screen.columns(at) == 6) games = int(i);
        }
        require(table >= 0 && games >= 0, "Original paired group grids");
        const auto upper = screen.grid(table), lower = screen.grid(games);
        const auto old = screen.rect(upper);
        const auto original = s.memory.copy_out(display, 0x129da8);
        const auto rng = random_state(s), nav = s.memory.copy_out(history, 0x1f4);
        const auto regs = s.registers;
        const auto stage = c.u32(c.u32(cup+0xc) + c.u32(slot(2))*4);
        const auto standings = c.u32(stage+0xb0);
        const auto records = s.memory.copy_out(standings, c.u16(stage+0x3e) * 0x44);
        layout_built_screen(s, display, 0x10065340);
        require(s.registers == regs && rng == random_state(s) && s.memory.copy_out(history, nav.size()) == nav &&
                s.memory.copy_out(standings, records.size()) == records, "Group layout changed game state");
        if (!enabled || size.original())
            require(s.memory.copy_out(display, original.size()) == original, "Classic group geometry changed");
        const auto panel = screen.rect(upper), fixtures = screen.rect(lower);
        require(panel.top == old.top && panel.bottom == old.bottom && screen.rows(upper) == 6 &&
                fixtures.top == panel.bottom + 5 && fixtures.left == panel.left && fixtures.right == panel.right,
                "Group standings and matches lost alignment");
        if (enabled && !size.original()) {
            require(panel.width() == std::min(size.width-130,1600) && fixtures.bottom <= size.height-100,
                    "Group panels outside content");
            require(screen.rows(lower) >= 12 && screen.rows(lower) <= 30 && !c.u8(lower+0xbef),
                    "Group matches did not expand or retained obsolete scrollbar");
        }
        for (unsigned i = 0; i < c.u16(display+0x129da4); ++i) {
            call(0x10714630, {screen.grid(i)});
            call(0x10714110, {screen.grid(i),0});
        }
        return panel;
    };
    c.put32(slot(2), 0);
    build({800,600}, true);
    build({1280,900}, false);
    const auto panel = build({1280,900}, true);
    unsigned previous_club = 0;
    for (int group = 0; group <= 2; ++group) {
        require(c.i32(slot(2)) == group, "Wrong Champions League group");
        const auto club = find_control([&](unsigned at) {
            return c.i16(at+0x72) == table && c.u32(at+8) == 2005 && c.u32(at+0x20) == 2 && c.u32(at+0x24) == 1;
        });
        require(hit(club) == int(club) && c.u32(screen.control(club)+0x48) != previous_club, "Group table team link");
        previous_club = c.u32(screen.control(club)+0x48);
        const auto last_date = find_control([&](unsigned at) {
            return c.i16(at+0x72) == games && c.u32(at+8) == 8 && c.u32(at+0x24) == 11;
        });
        require(hit(last_date) == int(last_date), "Twelfth group fixture not clickable");
        if (group == 2) break;
        const auto picker = find_control([&](unsigned at) {
            return screen.rect(at+0x10) == ScreenRect{panel.right-252,125,panel.right-127,145} && c.i16(at+0x74) >= 0;
        });
        const auto menu_grid = c.i16(screen.control(picker)+0x74);
        const auto option = find_control([&](unsigned at) {
            return c.i16(at+0x72) == menu_grid && c.u32(at+8) == 5 && c.u32(at+0x24) == unsigned(group+1);
        });
        require(hit(picker) == -1 && hit(option) == int(option) &&
                std::int32_t(call(0x1006d500,{option})) == -11, "Original group switch");
        require(build({1280,900},true) == panel, "Group selector moved between groups");
    }
    // The later phase still has match reports in the captured career.
    call(0x100648e0, {c.u32(c.toc(-0x7bbc)) + cup_id * 0x6c, 8, 0, 0});
    history = c.u32(manager + 0x10);
    require(c.i32(slot(2)) == 8 && !c.u32(slot(5)), "Expected fresh second-phase screen");
    build({3200,1800},true);
    const auto score = find_control([&](unsigned at) { return c.i16(at+0x72) == games && c.u32(at+8) == 2003; });
    require(hit(score) == int(score), "Champions League report link missed its score");
    const auto second_score = find_control([&](unsigned at) {
        return c.i16(at+0x72) == games && c.u32(at+8) == 2003 && c.u32(at+0x24) == 1;
    });
    const auto second_match = c.u32(screen.control(second_score)+0x48);
    const auto key = c.u32(screen.control(score)+0x48);
    require(std::int32_t(call(0x1006d500, {score})) == -4, "Report navigation command");
    history = c.u32(manager+0x10);
    require(c.u32(c.u32(history)) == 0x1028f280 && c.u32(slot(30)) == key, "Wrong match report");
    const auto report = [&](GuiCanvas size, bool enabled) {
        select_new_screens(enabled);
        require(s.platform->resize_gui_canvas(s, size), "Resize archived report");
        call(0x10719560, {display,1,0});
        call(0x1028f280, {});
        const auto original = s.memory.copy_out(display, 0x129da8);
        const auto nav = s.memory.copy_out(history, 0x1f4), rng = random_state(s);
        const auto regs = s.registers;
        layout_built_screen(s, display, 0x1028f280);
        require(s.registers == regs && rng == random_state(s) && nav == s.memory.copy_out(history, nav.size()),
                "Archived report layout changed game state");
        if (!enabled || size.original())
            require(original == s.memory.copy_out(display, original.size()), "Classic report changed");
        for (unsigned i = 0; i < c.u16(display+0x129da4); ++i) {
            call(0x10714630, {screen.grid(i)});
            call(0x10714110, {screen.grid(i),0});
        }
    };
    const auto label = [&](unsigned at) {
        const auto data = s.memory.copy_out(at+0x78, 0x100);
        return std::string(data.begin(), std::find(data.begin(),data.end(),0));
    };
    const auto select_tab = [&](unsigned tab) {
        const auto button = find_control([&](unsigned at) {
            if (c.u32(at+8)!=1 || c.u32(at+0x48)!=tab || c.i16(at+0x72)<0) return false;
            const auto grid = screen.grid(c.i16(at+0x72));
            return screen.rows(grid)==1 && screen.columns(grid)==(tab<=4 ? 4 : 3);
        });
        require(hit(button)==int(button), "Archived tab missed input");
        call(0x1028fa50,{button});
        require(c.u32(slot(2))==tab, "Archived tab navigation");
    };
    report({800,600},true);
    report({1280,900},false);
    for (const GuiCanvas size : {GuiCanvas{1280,900}, GuiCanvas{3200,1800}}) {
        report(size,true);
        const auto home = screen.rect(screen.control(0)+0x10), away = screen.rect(screen.control(3)+0x10);
        require(home.width() > 344 && home.width() <= 710 && away.left == home.right+2 &&
                std::abs(away.width()-home.width()) <= 1 && label(screen.control(0)).ends_with("Juventus") &&
                label(screen.control(3)).ends_with("Real Madrid"), "Archived team headings");
        require(screen.rect(screen.control(2)+0x10).right == home.right-5 &&
                screen.rect(screen.control(5)+0x10).right == away.right-5 &&
                hit(1)==1 && hit(4)==4, "Archived scores or team hit areas");
        const auto buffon = find_control([&](unsigned at) { return c.u32(at+8)==2007 && label(at)=="Gianluigi Buffon"; });
        require(hit(buffon)==int(buffon), "Archived player link");
        for (unsigned tab : {2,1,3,4,5,7,6}) {
            select_tab(tab);
            require(c.u32(slot(30))==key, "Archived tab changed match");
            report(size,true);
            if (tab==1) {
                const auto venue = find_control([&](unsigned at) { return label(at)=="Delle Alpi, Turim"; });
                const auto comp = find_control([&](unsigned at) { return c.u32(at+8)==2021; });
                require(screen.rect(screen.control(venue)+0x10).left==home.left+10 &&
                        screen.rect(screen.control(comp)+0x10).right==away.right-10 && hit(comp)==int(comp),
                        "Archived venue or competition link");
            } else if (tab==2) {
                const auto shots = find_control([&](unsigned at) { return label(at)=="8" && c.u32(at+0x24)==0 &&
                    c.i16(at+0x72)>=0 && screen.columns(screen.grid(c.i16(at+0x72)))==3; });
                const auto box = screen.rect(screen.grid(c.i16(screen.control(shots)+0x72)));
                require(box.left==home.left+10 && box.right==away.right-10 && box.height()>375,
                        "Archived statistics did not expand");
            } else if (tab!=6) {
                const auto msg = find_control([&](unsigned at) { return c.u32(at+0x24)==8 &&
                    c.i16(at+0x72)>=0 && screen.columns(screen.grid(c.i16(at+0x72)))==1 &&
                    screen.rect(screen.grid(c.i16(at+0x72))).top == 125; });
                const auto box = screen.rect(screen.control(msg)+0x10);
                require(std::abs(box.left+box.right-home.left-away.right)<=2, "Archived message off center");
            }
        }
    }
    // The next score in Group A has two goals and clickable scorer rows.
    call(0x1028f070,{second_match});
    history = c.u32(manager+0x10);
    report({1280,900},true);
    select_tab(1);
    report({1280,900},true);
    require(c.u32(slot(30))==second_match && label(screen.control(2))=="2" && label(screen.control(5))=="0",
            "Second archived score");
    for (const auto& [name, minute] : {std::pair{"Pizarro","28"}, std::pair{"Effenberg","56"}}) {
        const auto scorer = find_control([&](unsigned at) { return c.u32(at+8)==2007 && label(at).ends_with(name); });
        const auto at = screen.control(scorer);
        const auto time = find_control([&](unsigned other) {
            return c.i16(other+0x72)==c.i16(at+0x72) && c.u32(other+0x24)==c.u32(at+0x24) &&
                   label(other)==minute;
        });
        require(hit(scorer)==int(scorer) && screen.rect(at+0x10).right < screen.rect(screen.control(time)+0x10).left,
                "Archived goal or scorer link misaligned");
    }
    initialize_new_screens();
}
