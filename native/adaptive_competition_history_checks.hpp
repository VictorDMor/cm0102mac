// Replay the competition's Historial action and its own seven-tab browser.
void competition_history_builder(const char* path) {
    Temporary temp;
    initialize_new_screens(temp.path/"Screens.plist");
    Clock clock; clock.idle_input = true;
    auto state = std::make_unique<State>(); auto& s = *state;
    load_match_snapshot(s,path); s.code_fragment_observer = &clock;
    guest::Cpu c(s); c.set(2,c.u32(c.sp()+0x14));
    const auto manager = c.toc(-0x7bb0), display = c.u32(manager+0x24);
    c.put16(manager+0x2c58,0);
    s.platform = std::make_shared<NativePlatform>(temp.path/"game",temp.path/"user",AudioMode::offline);
    const auto scratch = allocate(s,32,true);
    world(s,scratch,scratch+16);
    require(s.platform->resize_gui_canvas(s,{801,601}), "Initialize competition history");
    const GuestScreen screen{c,display};
    constexpr unsigned builder = 0x1006ff50, handler = 0x10072e50;
    const auto call = [&](unsigned entry,std::initializer_list<unsigned> args) {
        unsigned reg=3;
        for (auto arg : args) c.set(reg++,arg);
        s.budget = s.executed+400000000;
        c.call(entry,return_sentinel);
        return std::int32_t(c.r(3));
    };
    const auto history = [&] { return c.u32(manager+0x10); };
    const auto slot = [&](unsigned n) { return c.u32(history()+0x14+n*8); };
    const auto label = [&](unsigned at) {
        const auto data = s.memory.copy_out(at+0x78,256);
        return std::string(data.begin(),std::find(data.begin(),data.end(),0));
    };
    const auto find = [&](auto predicate) {
        for (unsigned i=0; i<c.u16(display+0x129da6); ++i)
            if (predicate(screen.control(i))) return i;
        throw std::runtime_error("Missing history control");
    };
    const auto hit = [&](unsigned index) {
        const auto b = screen.rect(screen.control(index)+0x10);
        c.put32(c.toc(-0x5c10),(b.left+b.right)/2); c.put32(c.toc(-0x5c14),(b.top+b.bottom)/2);
        c.put8(c.toc(-0x7bac),1); c.put8(c.toc(-0x5c0c),0);
        return std::int16_t(call(c.u8(display)==1 ? 0x1071aa10 : 0x1071a280,{display}));
    };
    const auto build = [&](unsigned entry) {
        call(0x10719560,{display,1,0}); call(entry,{});
        const auto nav = s.memory.copy_out(history(),0x1f4), rng = random_state(s);
        const auto regs = s.registers;
        const auto original = s.memory.copy_out(display,0x129da8);
        layout_built_screen(s,display,entry);
        require(nav==s.memory.copy_out(history(),nav.size()) && rng==random_state(s) && regs==s.registers,
                "Competition history layout changed game state");
        if (gui_canvas(s).original() || !new_screens_enabled())
            require(original==s.memory.copy_out(display,original.size()), "Classic history layout changed");
        for (unsigned i=0; i<c.u16(display+0x129da6); ++i) {
            const auto at = screen.control(i);
            for (const auto [offset,length] : {std::pair{8u,4u},{0x20u,8u},{0x48u,4u},{0x68u,0x110u}}) {
                const auto field = s.memory.copy_out(at+offset,length);
                require(std::equal(field.begin(),field.end(),original.begin()+(at-display)+offset),
                        "Competition history changed a row, label, style or link");
            }
        }
        for (unsigned i=0; i<c.u16(display+0x129da4); ++i) {
            screen.measure(screen.grid(i)); screen.publish(screen.grid(i));
        }
    };
    const auto tab = [&](unsigned number) {
        const auto button = find([&](unsigned at) { return c.u32(at+8)==2 && c.u32(at+0x48)==number; });
        require(hit(button)==int(button) && call(handler,{button})==-11 && slot(1)==number, "History tab navigation");
        build(builder);
    };
    const auto definitions = c.u32(c.toc(-0x7bbc));
    call(0x100648e0,{definitions+262*0x6c,0xfffffffe,0xffffffff,0});
    build(0x10065340);
    const auto open = find([&](unsigned at) { return c.u32(at+8)==4 && label(at)=="Historial"; });
    require(hit(open)==int(open) && call(0x1006d500,{open})==-4 && c.u32(c.u32(history()))==builder,
            "Competition did not open its history");
    for (const auto [size,enabled] : {std::pair{GuiCanvas{800,600},true},{{1280,900},false},
                                      {{1280,900},true},{{3200,1800},true}}) {
        select_new_screens(enabled);
        require(s.platform->resize_gui_canvas(s,size), "History canvas resize");
        build(builder);
        if (size.original() || !enabled) continue;
        const auto grid = slot(5), body = screen.grid(grid);
        const auto b = screen.rect(body);
        require(screen.rows(body)==30 && screen.columns(body)==3 && c.i32(body+0x14)==89 &&
                b.width()==std::min(1600,size.width-130) && b.bottom<size.height-90, "Mineiro winners geometry");
        c.put32(c.toc(-0x5c10),b.right-10);
        c.put32(c.toc(-0x5c14),c.i32(body+0xb90)); c.put8(c.toc(-0x7bac),1);
        for (int i=0; i<4 && c.i32(body+0xb98)<60; ++i)
            require(call(0x1071bd00,{display,grid})==1, "Winners scrollbar did not page");
        require(c.i32(body+0xb98)==60, "Missing earliest championship");
        const auto last = find([&](unsigned at) { return c.i16(at+0x72)==int(grid) && c.u32(at+0x24)==89 && c.u32(at+0x20)==1; });
        require(label(screen.control(last)).ends_with("(MG)") && c.u32(screen.control(last)+8)==2005 &&
                hit(last)==int(last), "Earliest winner lost its club link");
        tab(2);
        const auto club = find([&](unsigned at) { return c.u32(at+8)==2005 && c.i16(at+0x72)==int(slot(8)); });
        const auto link = screen.rect(screen.control(club)+0x10);
        require(link.left>b.left+300 && hit(club)==int(club), "Record description lost its embedded club link");
        for (unsigned number : {3,4,5,6,7}) {
            tab(number);
            const auto list = screen.grid(slot(13+3*(number-3)));
            require(screen.rows(list)==30 && screen.rect(list).width()==b.width(), "Record list failed to expand");
        }
        tab(1);
    }
    // A short international table also includes third place and host. An empty
    // qualification history has just headings; neither should get a scrollbar.
    for (unsigned id : {404,392}) {
        call(0x1006fc10,{definitions+id*0x6c,0}); build(builder);
        const auto body = screen.grid(slot(5));
        require(!c.u8(body+0xbef) && screen.columns(body)==(id==404 ? 5 : 3) &&
                c.i32(body+0x14)==(id==404 ? 16 : 0), "International winners columns or scroll range");
        if (id==404) {
            const auto brazil = find([&](unsigned at) { return c.i16(at+0x72)==int(slot(5)) &&
                c.u32(at+0x24)==1 && c.u32(at+0x20)==2; });
            require(label(screen.control(brazil)).ends_with("Brasil") && hit(brazil)==int(brazil), "1998 finalist link");
        }
    }
    initialize_new_screens();
}
