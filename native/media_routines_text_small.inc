MEDIA_ROUTINE(0x10688fd0, 0x106891ac, R3, "ae40b6d662ae2a66",
    []([[maybe_unused]] Cpu& c) { media::club_finance_story(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
MEDIA_ROUTINE(0x10649ad0, 0x10649fac, R3, "3eeedebef5ab1e9b",
    []([[maybe_unused]] Cpu& c) { media::player_apology_story(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
MEDIA_ROUTINE(0x10655030, 0x10655d58, R3, "a9d592ab7e85eaaf",
    []([[maybe_unused]] Cpu& c) { media::manager_exchange_story(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
MEDIA_ROUTINE(0x10657b30, 0x10658370, R3, "bc6ca91cb9b743a9",
    []([[maybe_unused]] Cpu& c) { media::manager_reply_text(c, c.r(3), std::int32_t(c.r(4)), std::int32_t(c.r(5)), c.r(6), std::int32_t(c.r(7))); })
MEDIA_ROUTINE(0x1067d2f0, 0x1067d82c, R3, "c46197f83c1331e8",
    []([[maybe_unused]] Cpu& c) { media::player_request_story(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
MEDIA_ROUTINE(0x1067dc10, 0x1067e210, R3, "fc81f74a43f960d1",
    []([[maybe_unused]] Cpu& c) { media::player_quarrel_story(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
MEDIA_ROUTINE(0x1067b920, 0x1067be98, R3, "89a888399950d230",
    []([[maybe_unused]] Cpu& c) { media::replay_logged_item(c, c.r(3), c.r(4)); })
MEDIA_ROUTINE(0x1067bea0, 0x1067ca24, R3, "aa9682519904498a",
    []([[maybe_unused]] Cpu& c) { media::media_response_dialog(c); })
MEDIA_ROUTINE(0x1067ca30, 0x1067d02c, R3, "a6907411e2fb272e",
    []([[maybe_unused]] Cpu& c) { media::media_response_event(c, c.r(3)); })
MEDIA_ROUTINE(0x10656450, 0x10657b2c, R3, "2ec1ff9cd6fa0e4c",
    []([[maybe_unused]] Cpu& c) { media::player_outburst_story(c, c.r(3), c.r(4), c.r(5), c.r(6)); })
