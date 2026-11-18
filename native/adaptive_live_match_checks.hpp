// Cruzeiro vs Uberlândia at kickoff, loaded from the existing match-tactics capture.
void live_match_builder(const char* snapshot) {
    Temporary temp;
    initialize_new_screens(temp.path/"Screens.plist");
    Clock clock; clock.idle_input = true; clock.step_us = 1000000;
    auto state = std::make_unique<State>();
    auto& s = *state;
    load_match_snapshot(s,snapshot);
    s.code_fragment_observer = &clock;
    guest::Cpu c(s);
    c.set(2,c.u32(c.sp()+0x14));
    const auto manager = c.toc(-0x7bb0), history = c.u32(manager+0x10), display = c.u32(manager+0x24);
    c.put16(manager+0x2c58,0);
    const auto slot = [&](int n) { return c.u32(history+0x14+n*8); };
    const auto fixture = slot(0), stats = c.u32(fixture+0x60), engine = c.u32(fixture+0x58);
    constexpr unsigned builder = 0x1028c430, handler = 0x1028dc80;
    require(fixture && stats && engine && c.u32(c.u32(history))==builder && slot(2)==1, "Expected captured live overview");
    s.platform = std::make_shared<NativePlatform>(temp.path/"game",temp.path/"user",AudioMode::offline);
    const auto scratch = allocate(s,32,true);
    world(s,scratch,scratch+16);
    const GuestScreen screen{c,display};
    const auto call = [&](unsigned entry,std::initializer_list<unsigned> args) {
        unsigned reg = 3;
        for (auto arg : args) c.set(reg++,arg);
        s.budget = s.executed+400000000;
        c.call(entry,return_sentinel);
        return int(c.r(3));
    };
    const auto build = [&] {
        call(0x10719560,{display,1,0});
        call(builder,{});
        const auto nav = s.memory.copy_out(history,0x1f4), record = s.memory.copy_out(stats,0xdfc);
        const auto match = s.memory.copy_out(engine,0xe080), rng = random_state(s);
        const auto regs = s.registers;
        layout_built_screen(s,display,builder);
        require(nav==s.memory.copy_out(history,nav.size()) && record==s.memory.copy_out(stats,record.size()) &&
                match==s.memory.copy_out(engine,match.size()) && rng==random_state(s) && regs==s.registers,
                "Live layout changed match data, clock, RNG or registers");
        for (unsigned i=0; i<c.u16(display+0x129da4); ++i) {
            screen.measure(screen.grid(i)); screen.publish(screen.grid(i));
        }
    };
    const auto bounds = [&](unsigned i) { return screen.rect(screen.control(i)+0x10); };
    const auto label = [&](unsigned i) {
        const auto bytes = s.memory.copy_out(screen.control(i)+0x78,256);
        return std::string(bytes.begin(),std::find(bytes.begin(),bytes.end(),0));
    };
    const auto hit = [&](unsigned index) {
        const auto b = bounds(index);
        c.put32(c.toc(-0x5c10),(b.left+b.right)/2); c.put32(c.toc(-0x5c14),(b.top+b.bottom)/2);
        c.put8(c.toc(-0x7bac),1); c.put8(c.toc(-0x5c0c),0);
        require(std::int16_t(call(c.u8(display)==1 ? 0x1071aa10 : 0x1071a280,{display}))==int(index),
                "Live match control missed its visible bounds");
    };
    const auto press = [&](unsigned index) {
        hit(index);
        require(call(handler,{index})==-11,"Live match command did not rebuild");
    };
    require(s.platform->resize_gui_canvas(s,{801,601}), "Initialize live match surface");
    for (const auto [size,enabled] : {std::pair{GuiCanvas{800,600},true},{{1280,900},false},{{1280,900},true},{{3200,1800},true}}) {
        select_new_screens(enabled);
        require(s.platform->resize_gui_canvas(s,size), "Resize live match");
        build();
        if (!enabled || size.original()) {
            require(bounds(0)==ScreenRect{100,10,444,70} && bounds(slot(33))==ScreenRect{150,415,740,450},
                    "Classic live match layout changed");
            continue;
        }
        require(bounds(0).width()>444 && bounds(2).right==bounds(0).right-5 && bounds(5).right==bounds(3).right-5,
                "Live scoreboard lost alignment");
        if (slot(1)==2) {
            press(slot(4)); build();
            require(label(slot(4)).starts_with("Pausar"), "Kickoff did not start the match");
            // Replay until the first goal is visible, then verify the scoreboard,
            // the scorer's hit target and the commentary control on the same screen.
            for (int ticks=0; ticks<3600 && label(2)=="0" && label(5)=="0"; ++ticks)
                if (call(handler,{0xffffffff})==-11) build();
            require(c.u16(stats+0x4e)>0 && label(2)==std::to_string(c.u8(stats+0x120)) &&
                    label(5)==std::to_string(c.u8(stats+0x784)) && (label(2)!="0" || label(5)!="0"),
                    "Live scoreboard diverged from the match");
            unsigned scorer = 0;
            while (scorer<c.u16(display+0x129da6) && c.u32(screen.control(scorer)+8)!=2007) ++scorer;
            require(scorer<c.u16(display+0x129da6) && !label(scorer).empty(), "Missing live scorer link");
            hit(scorer);
            require(bounds(slot(8)).width()>800, "Live commentary missed its expanded panel");
            for (const auto expected : {"Retomar","Pausar"}) {
                press(slot(4)); build();
                require(label(slot(4)).starts_with(expected), "Start, pause or resume failed");
            }
        }
        hit(slot(5)); // Human club's tactics button.
        // Use the match's original updater to publish a fresh percentage.
        c.put8(stats+0x144,65); c.put8(stats+0x7a8,1);
        c.put8(slot(29)+0x144,50);
        call(0x10291470,{});
        const auto frame = bounds(slot(33)), home = bounds(slot(33)+3), away = bounds(slot(33)+4);
        require(label(slot(33)+3)=="65%" && home.left==frame.left+127 && home.right==away.left &&
                home.top==frame.top+10 && away.right==frame.right-27, "Live overview update lost possession bar bounds");
        for (unsigned tab : {2,3,4,6,7,8,10,9,1}) {
            unsigned i = 0;
            while (i<c.u16(display+0x129da6) &&
                   !(c.u32(screen.control(i)+8)==3 && c.u32(screen.control(i)+0x48)==tab)) ++i;
            require(i<c.u16(display+0x129da6), "Missing live match tab");
            press(i); build();
            require(slot(2)==tab && slot(0)==fixture, "Live tab changed match");
            if (tab==3) {
                const auto first = slot(11);
                const auto a = bounds(first+21), b = bounds(first+22);
                require(a.right==b.left && a.left>212, "Zone bars not centered on adaptive pitch");
                // A new percentage goes through SCRMAN and the real DISPLAY interpreter.
                call(0x10740940,{manager,first+21,212,465,328,480});
                call(0x10740940,{manager,first+22,328,465,678,480});
                require(bounds(first+21).left==a.left && bounds(first+21).top==a.top &&
                        bounds(first+21).right==bounds(first+22).left &&
                        std::abs(bounds(first+21).width()*466-(b.right-a.left)*116)<=466 &&
                        bounds(first+22).right==b.right, "Partial zone repaint reverted to 800x600");
            }
            if (tab==6 || tab==9) {
                const auto grid = screen.grid(c.i16(screen.control(slot(26))+0x72));
                require(screen.rows(grid)==1 && screen.rect(grid).width()>1000, "Live team table not adapted");
            }
        }
    }
    initialize_new_screens();
}
