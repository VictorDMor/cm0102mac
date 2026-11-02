// Stories by kind, the first half: a pundit's praise, a report, a former star's opinion.
#include "media_text_c_detail.hpp"

namespace cm::aot::media::detail::report {
using namespace guest;
using namespace detail;

// Kind 0x29: an outlet praises him, a first cap is in the offing, or a former star has joined the club.
void Report::kind_29() {
    if (cmp(tone, 2) == 0) {
        if (cmp(legend, -1) <= 0 || cmp(nation, people[legend].nation().id()) != 0) {
            StackWords{c} << clubs[club].short_name() << amount_text << subject_full << nations[nation].short_name();
            say(story, 0x23a99, 0x10682244, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), twice(nations[nation].short_marker())});
            return;
        }
        if (cmp(previous_club, -1) <= 0) {
            StackWords{c} << legend_name << clubs[club].short_name() << amount_text << subject_full << nations[nation].short_name();
            say(story, 0x2398a, 0x10682194, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), twice(nations[nation].short_marker())});
            return;
        }
        const auto job = call(job_title, 0x10681fc4, {signed_byte(legend_job), 0, 0, frame.at(0x86)});
        StackWords{c} << signed_byte(amount_flag) << signed_byte(amount_flag) << nations[nation].short_marker() << nations[nation].short_marker() << clubs[previous_club].short_name() << job << legend_name << clubs[club].short_name() << amount_text << subject_full << nations[nation].short_name();
        say(story, 0x23852, 0x106820dc, {twice(clubs[previous_club].short_marker()), twice(signed_byte(byte_at(0x86))), twice(clubs[club].short_marker())}, {{13, (previous_club * 0x248) + 0x221}});
        return;
    }
    if (cmp(tone, 2) > 0) {
        if (cmp(signed_byte(domestic), 0) == 0) {
            const auto flag = call(amount_phrase, 0x106822f4, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
            StackWords{c} << clubs[club].short_name() << amount_text << subject_full << clubs[club].nation().name();
            say(story, 0x23c92, 0x106823a4, {twice(clubs[club].short_marker()), twice(signed_byte(flag)), twice(clubs[club].nation().marker())}, {{13, clubs.base()}, {27, flag}});
            return;
        }
        const auto flag2 = call(amount_phrase, 0x10682270, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        StackWords{c} << subject_full;
        say(story, 0x23b76, 0x106822d0, {twice(clubs[club].short_marker()), twice(signed_byte(flag2)), clubs[club].short_name(), amount_text}, {{27, flag2}});
        return;
    }
    if (cmp(tone, 0) == 0) {
        const auto flag3 = call(amount_phrase, 0x10681c6c, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        amount_flag = flag3;
        if (cmp(signed_byte(domestic), 0) == 0) {
            StackWords{c} << 0x64 << outlet << clubs[club].short_name() << amount_text << subject_full;
            say(story, 0x2352f, 0x10681dd4, {twice(signed_byte(article_marker)), twice(clubs[club].short_marker()), twice(signed_byte(amount_flag))});
            return;
        }
        StackWords{c} << 0x64 << nations[nation].short_marker() << nations[nation].short_marker() << outlet << clubs[club].short_name() << amount_text << subject_full << nations[nation].short_name();
        say(story, 0x233f5, 0x10681d4c, {twice(signed_byte(article_marker)), twice(clubs[club].short_marker()), twice(signed_byte(amount_flag))}, {{13, nations[nation].short_marker()}});
        return;
    }
    if (cmp(tone, 0) > 0) {
        if (cmp(signed_byte(domestic), 0) != 0) {
            StackWords{c} << clubs[club].short_name() << amount_text << subject_full << nations[nation].short_name();
            say(story, 0x23644, 0x10681e8c, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), twice(nations[nation].short_marker())});
            return;
        }
        call(nationality_of, 0x10681eb0, {clubs[club].nation(), nationality, 0x78, 0x1c});
        StackWords{c} << nations[nation].short_marker() << nations[nation].short_marker() << clubs[club].short_name() << amount_text << subject_full << nationality << nations[nation].short_name();
        say(story, 0x2373a, 0x10681f74, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), twice(0x64)}, {{13, club * 0x248}, {14, (nation * 0x128) + 0xae}, {22, clubs.base()}});
        return;
    }
    if (cmp(signed_byte(domestic), 0) == 0) {
        const auto flag = call(amount_phrase, 0x106822f4, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        StackWords{c} << clubs[club].short_name() << amount_text << subject_full << clubs[club].nation().name();
        say(story, 0x23c92, 0x106823a4, {twice(clubs[club].short_marker()), twice(signed_byte(flag)), twice(clubs[club].nation().marker())}, {{13, clubs.base()}, {27, flag}});
        return;
    }
    const auto flag2 = call(amount_phrase, 0x10682270, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
    StackWords{c} << subject_full;
    say(story, 0x23b76, 0x106822d0, {twice(clubs[club].short_marker()), twice(signed_byte(flag2)), clubs[club].short_name(), amount_text}, {{27, flag2}});
}

// Kind 0x2a: an outlet praises him; an exciting future lies ahead for the club.
void Report::kind_2a() {
    if (cmp(tone, 2) == 0) {
        if (cmp(signed_byte(domestic), 0) == 0) {
            const auto flag = call(amount_phrase, 0x106825fc, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
            StackWords{c} << clubs[club].short_name() << amount_text << subject_full << clubs[club].nation().name();
            say(story, 0x24177, 0x106826ac, {twice(clubs[club].short_marker()), twice(signed_byte(flag)), twice(clubs[club].nation().marker())}, {{13, clubs.base()}, {27, flag}});
            return;
        }
        const auto flag2 = call(amount_phrase, 0x10682578, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        StackWords{c} << subject_full;
        say(story, 0x24068, 0x106825d8, {twice(clubs[club].short_marker()), twice(signed_byte(flag2)), clubs[club].short_name(), amount_text}, {{27, flag2}});
        return;
    }
    if (cmp(tone, 2) > 0) {
        if (cmp(signed_byte(domestic), 0) == 0) {
            const auto flag3 = call(amount_phrase, 0x1068275c, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
            StackWords{c} << amount_text << subject_full << clubs[club].nation().name() << clubs[club].short_name();
            say(story, 0x243af, 0x1068280c, {twice(signed_byte(flag3)), twice(clubs[club].nation().marker()), twice(clubs[club].short_marker())}, {{27, flag3}});
            return;
        }
        const auto flag4 = call(amount_phrase, 0x106826d8, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        StackWords{c} << clubs[club].short_name();
        say(story, 0x2429c, 0x10682738, {twice(signed_byte(flag4)), twice(clubs[club].short_marker()), amount_text, subject_full}, {{27, flag4}});
        return;
    }
    if (cmp(tone, 0) == 0) {
        StackWords{c} << 0x64 << outlet << clubs[club].short_name() << amount_text << subject_full;
        say(story, 0x23dc4, 0x1068244c, {twice(signed_byte(article_marker)), twice(clubs[club].short_marker()), twice(signed_byte(amount_flag))});
        return;
    }
    if (cmp(tone, 0) > 0) {
        if (cmp(signed_byte(domestic), 0) != 0) {
            StackWords{c} << subject_full;
            say(story, 0x23ed8, 0x106824b4, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), clubs[club].short_name(), amount_text});
            return;
        }
        call(nationality_of, 0x106824d8, {clubs[club].nation(), nationality, 0x78, 0x1c});
        StackWords{c} << clubs[club].short_name() << amount_text << subject_full << nationality;
        say(story, 0x23f8f, 0x1068254c, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), twice(0x64)});
        return;
    }
    if (cmp(signed_byte(domestic), 0) == 0) {
        const auto flag3 = call(amount_phrase, 0x1068275c, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        StackWords{c} << amount_text << subject_full << clubs[club].nation().name() << clubs[club].short_name();
        say(story, 0x243af, 0x1068280c, {twice(signed_byte(flag3)), twice(clubs[club].nation().marker()), twice(clubs[club].short_marker())}, {{27, flag3}});
        return;
    }
    const auto flag4 = call(amount_phrase, 0x106826d8, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
    StackWords{c} << clubs[club].short_name();
    say(story, 0x2429c, 0x10682738, {twice(signed_byte(flag4)), twice(clubs[club].short_marker()), amount_text, subject_full}, {{27, flag4}});
}

// Kind 0x2b: journalists hail his performances; `flags` pick the angle. When none fits, the tone is replaced by the club id's remainder and the story starts again.
void Report::kind_2b() {
    if (cmp(tone, 0) == 0 && cmp(flags & 0x1000000, 0) != 0) {
        StackWords{c} << outlet << clubs[club].short_name() << subject_full << subject_short;
        say(story, 0x28314, 0x10686e24, {twice(signed_byte(article_marker)), twice(clubs[club].short_marker()), twice(0x64)});
        return;
    }
    if (cmp(tone, 0) == 0) {
        if (cmp(signed_byte(domestic), 0) == 0) {
            StackWords{c} << 0x64 << clubs[club].nation().name() << clubs[club].short_name() << amount_text << subject_full;
            say(story, 0x2851b, 0x10686f54, {twice(clubs[club].nation().marker()), twice(clubs[club].short_marker()), twice(signed_byte(amount_flag))});
            return;
        }
        StackWords{c} << amount_text << subject_full;
        say(story, 0x2841d, 0x10686e9c, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), 0x64, clubs[club].short_name()});
        return;
    }
    if (cmp(tone, 1) == 0) {
        if (cmp(signed_byte(domestic), 0) == 0) {
            StackWords{c} << clubs[club].nation().name();
            say(story, 0x286ee, 0x1068704c, {twice(clubs[club].short_marker()), twice(clubs[club].nation().marker()), clubs[club].short_name(), subject_full});
            return;
        }
        return say(story, 0x28637, 0x10686fb4, {twice(clubs[club].short_marker()), clubs[club].short_name(), subject_full, clubs.base()});
    }
    if (cmp(tone, 2) == 0) {
        if (cmp(signed_byte(domestic), 0) != 0) {
            StackWords{c} << subject_full;
            say(story, 0x287a5, 0x106870c4, {twice(signed_byte(article_marker)), twice(clubs[club].short_marker()), outlet, clubs[club].short_name()});
            return;
        }
        call(nationality_of, 0x106870e8, {clubs[club].nation(), nationality, 0x78, 0x1f});
        say(story, 0x2886a, 0x1068713c, {0x64, twice(clubs[club].short_marker()), nationality, clubs[club].short_name(), subject_full});
        return;
    }
    if (cmp(tone, 3) == 0) {
        StackWords{c} << subject_full;
        say(story, 0x28927, 0x106871ac, {twice(signed_byte(article_marker)), twice(clubs[club].short_marker()), outlet, clubs[club].short_name()});
        return;
    }
    if (cmpl((flags & 0x2c0040) - 0x2c0000, 0x40) == 0 && cmp(std::uint32_t(std::int32_t(tone) % 4), 0) != 0) {
        StackWords{c} << subject_full << subject_short << clubs[club].nation().name();
        say(story, 0x28a02, 0x10687284, {twice(clubs[club].short_marker()), 0x64, twice(clubs[club].nation().marker()), clubs[club].short_name()});
        return;
    }
    if (cmpl((flags & 0x240040) - 0x240000, 0x40) == 0 && cmp(std::uint32_t(std::int32_t(tone) % 4), 0) != 0) {
        StackWords{c} << subject_full << subject_short << competitions[competition].short_name();
        say(story, 0x28b36, 0x10687350, {twice(clubs[club].short_marker()), 0x64, twice(competitions[competition].short_marker()), clubs[club].short_name()});
        return;
    }
    if (cmpl((flags & 0xc0040) - 0xc0000, 0x40) == 0 && cmp(std::uint32_t(std::int32_t(tone) % 4), 0) != 0) {
        if (cmp(signed_byte(domestic), 0) != 0) return say(story, 0x28c86, 0x106873d8, {twice(clubs[club].short_marker()), clubs[club].short_name(), subject_full, clubs.base()});
        call(nationality_of, 0x106873fc, {clubs[club].nation(), nationality, 0x78, 0x16});
        say(story, 0x28d6f, 0x10687450, {twice(clubs[club].short_marker()), 0x64, clubs[club].short_name(), subject_full, nationality});
        return;
    }
    if (cmpl((flags & 0x40040) - 0x40000, 0x40) == 0 && cmp(std::uint32_t(std::int32_t(tone) % 4), 0) != 0) {
        StackWords{c} << competitions[competition].short_name();
        say(story, 0x28e67, 0x1068750c, {twice(clubs[club].short_marker()), twice(competitions[competition].short_marker()), clubs[club].short_name(), subject_full});
        return;
    }
    if (cmpl((flags & 0x102040) - 0x100000, 0x2040) == 0 && cmp(std::uint32_t(std::int32_t(tone) % 4), 0) != 0) {
        if (cmp(signed_byte(domestic), 0) == 0) {
            StackWords{c} << clubs[club].nation().name();
            say(story, 0x29000, 0x1068762c, {twice(clubs[club].short_marker()), twice(clubs[club].nation().marker()), clubs[club].short_name(), subject_full});
            return;
        }
        return say(story, 0x28f51, 0x10687594, {twice(clubs[club].short_marker()), clubs[club].short_name(), subject_full, clubs.base()});
    }
    if (cmpl((flags & 0x200840) - 0x200000, 0x840) == 0) {
        if (cmp(std::uint32_t(std::int32_t(tone) % 4), 0) != 0) {
            if (cmp(signed_byte(domestic), 0) == 0) {
                StackWords{c} << clubs[club].nation().name();
                say(story, 0x2917d, 0x1068774c, {twice(clubs[club].short_marker()), twice(clubs[club].nation().marker()), clubs[club].short_name(), subject_full});
                return;
            }
            return say(story, 0x290cd, 0x106876b4, {twice(clubs[club].short_marker()), clubs[club].short_name(), subject_full, clubs.base()});
        }
        // (flags has 0x840 here and the tone's remainder is 0, so the original's next test for both fails; its compares remain.)
        cmp(flags & 0x840, 0x840);
        cmp(std::uint32_t(std::int32_t(tone) % 4), 0);
        const auto flag = call(amount_phrase, 0x10687834, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        amount_flag = flag;
        if (cmp(flags & 0x1000000, 0) == 0) {
            tone = std::uint32_t(std::int32_t(club) % 4);
            c.set({{3, club >> 0x1f}});  // (left in the argument registers by the arithmetic before)
            return kind_2b();
        }
        if (cmp(signed_byte(domestic), 0) != 0) {
            StackWords{c} << amount_text << subject_full;
            say(story, 0x2938a, 0x106878ac, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), 0x64, clubs[club].short_name()});
            return;
        }
        call(nationality_of, 0x106878d0, {clubs[club].nation(), nationality, 0x78, 0x1c});
        StackWords{c} << 0x64 << nationality << clubs[club].short_name() << amount_text << subject_full;
        say(story, 0x2948b, 0x1068794c, {twice(0x64), twice(clubs[club].short_marker()), twice(signed_byte(amount_flag))});
        return;
    }
    if (cmp(flags & 0x840, 0x840) != 0) {
        const auto flag2 = call(amount_phrase, 0x10687834, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        amount_flag = flag2;
        if (cmp(flags & 0x1000000, 0) == 0) {
            tone = std::uint32_t(std::int32_t(club) % 4);
            c.set({{3, club >> 0x1f}});  // (left in the argument registers by the arithmetic before)
            return kind_2b();
        }
        if (cmp(signed_byte(domestic), 0) != 0) {
            StackWords{c} << amount_text << subject_full;
            say(story, 0x2938a, 0x106878ac, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), 0x64, clubs[club].short_name()});
            return;
        }
        call(nationality_of, 0x106878d0, {clubs[club].nation(), nationality, 0x78, 0x1c});
        StackWords{c} << 0x64 << nationality << clubs[club].short_name() << amount_text << subject_full;
        say(story, 0x2948b, 0x1068794c, {twice(0x64), twice(clubs[club].short_marker()), twice(signed_byte(amount_flag))});
        return;
    }
    if (cmp(std::uint32_t(std::int32_t(tone) % 4), 0) != 0) {
        StackWords{c} << subject_short << clubs[club].nation().name();
        say(story, 0x2924b, 0x10687810, {twice(clubs[club].short_marker()), twice(clubs[club].nation().marker()), clubs[club].short_name(), subject_full});
        return;
    }
    const auto flag = call(amount_phrase, 0x10687834, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
    amount_flag = flag;
    if (cmp(flags & 0x1000000, 0) == 0) {
        tone = std::uint32_t(std::int32_t(club) % 4);
        c.set({{3, club >> 0x1f}});  // (left in the argument registers by the arithmetic before)
        return kind_2b();
    }
    if (cmp(signed_byte(domestic), 0) != 0) {
        StackWords{c} << amount_text << subject_full;
        say(story, 0x2938a, 0x106878ac, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), 0x64, clubs[club].short_name()});
        return;
    }
    call(nationality_of, 0x106878d0, {clubs[club].nation(), nationality, 0x78, 0x1c});
    StackWords{c} << 0x64 << nationality << clubs[club].short_name() << amount_text << subject_full;
    say(story, 0x2948b, 0x1068794c, {twice(0x64), twice(clubs[club].short_marker()), twice(signed_byte(amount_flag))});
}

// Kind 0x2c (and any kind outside the table): national papers lavish praise on him.
void Report::kind_2c() {
    if (cmp(signed_byte(domestic), 0) == 0) {
        const auto flag = call(amount_phrase, 0x10687b84, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        StackWords{c} << nations[nation].short_marker() << nations[nation].short_marker() << clubs[club].nation().name() << clubs[club].short_name() << subject_full << amount_text << nations[nation].short_name();
        say(story, 0x298fc, 0x10687c84, {twice(clubs[club].nation().marker()), twice(clubs[club].short_marker()), twice(signed_byte(flag))}, {{13, clubs[club].short_name()}, {14, clubs.base()}, {27, flag}});
        return;
    }
    const auto flag2 = call(amount_phrase, 0x10687ab0, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
    StackWords{c} << clubs[club].short_name() << subject_full << amount_text << nations[nation].short_name();
    say(story, 0x297dd, 0x10687b60, {twice(clubs[club].short_marker()), twice(signed_byte(flag2)), twice(nations[nation].short_marker())}, {{27, flag2}});
}

// Kind 0x2d: a pundit or a national paper reports on the club.
void Report::kind_2d() {
    if (cmp(tone, 1) == 0) {
        if (cmp(signed_byte(domestic), 0) != 0) {
            const auto flag = call(amount_phrase, 0x106819ac, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
            StackWords{c} << subject_full << clubs[club].long_name();
            say(story, 0x22f27, 0x10681a14, {twice(signed_byte(flag)), 0x64, twice(clubs[club].long_marker()), amount_text}, {{27, flag}});
            return;
        }
        call(nationality_of, 0x10681a38, {clubs[club].nation(), nationality, 0x78, 0x16});
        const auto flag2 = call(amount_phrase, 0x10681a58, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        StackWords{c} << nationality << amount_text << subject_full << clubs[club].long_name();
        say(story, 0x23049, 0x10681ad0, {0x64, twice(signed_byte(flag2)), 0x64, twice(clubs[club].long_marker())}, {{27, flag2}});
        return;
    }
    if (cmp(tone, 1) <= 0 && cmp(tone, 0) >= 0) {
        const auto flag3 = call(amount_phrase, 0x10681920, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        StackWords{c} << subject_full;
        say(story, 0x22e36, 0x10681980, {twice(clubs[club].short_marker()), twice(signed_byte(flag3)), clubs[club].short_name(), amount_text}, {{27, flag3}});
        return;
    }
    if (cmp(signed_byte(domestic), 0) != 0) {
        const auto flag4 = call(amount_phrase, 0x10681afc, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        StackWords{c} << subject_short << amount_text;
        say(story, 0x2318b, 0x10681b64, {twice(clubs[club].short_marker()), twice(signed_byte(flag4)), clubs[club].short_name(), subject_full}, {{27, flag4}});
        return;
    }
    call(nationality_of, 0x10681b88, {clubs[club].nation(), nationality, 0x78, 0x1c});
    const auto flag5 = call(amount_phrase, 0x10681ba8, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
    StackWords{c} << clubs[club].short_name() << subject_full << subject_short << amount_text << nationality;
    say(story, 0x232b2, 0x10681c28, {twice(clubs[club].short_marker()), twice(signed_byte(flag5)), twice(0x64)}, {{27, flag5}});
}

// Kind 0x2e: a pundit praises him, with a former star's view or the sum he is worth.
void Report::kind_2e() {
    if (cmp(tone, 2) == 0) {
        if (cmp(signed_byte(domestic), 0) != 0) {
            const auto flag = call(amount_phrase, 0x10680b04, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
            StackWords{c} << clubs[club].short_name();
            say(story, 0x21a35, 0x10680b64, {twice(signed_byte(flag)), twice(clubs[club].short_marker()), amount_text, subject_full}, {{27, flag}});
            return;
        }
        call(nationality_of, 0x10680b88, {clubs[club].nation(), nationality, 0x78, 0x16});
        const auto flag2 = call(amount_phrase, 0x10680ba8, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        StackWords{c} << amount_text << subject_full << clubs[club].short_name();
        say(story, 0x21b3f, 0x10680c18, {0x64, twice(signed_byte(flag2)), twice(clubs[club].short_marker()), nationality}, {{27, flag2}});
        return;
    }
    if (cmp(tone, 2) > 0) {
        if (cmp(tone, 4) < 0) {
            const auto flag3 = call(amount_phrase, 0x10680c3c, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
            StackWords{c} << amount_text;
            say(story, 0x21c69, 0x10680c9c, {twice(clubs[club].short_marker()), twice(signed_byte(flag3)), clubs[club].short_name(), subject_full}, {{27, flag3}});
            return;
        }
        if (cmp(signed_byte(domestic), 0) != 0) {
            const auto flag4 = call(amount_phrase, 0x10680cc8, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
            StackWords{c} << clubs[club].short_name() << subject_full << subject_short << amount_text;
            say(story, 0x21d65, 0x10680d40, {twice(clubs[club].short_marker()), twice(0x64), twice(signed_byte(flag4))}, {{27, flag4}});
            return;
        }
        call(nationality_of, 0x10680d64, {clubs[club].nation(), nationality, 0x78, 0x1f});
        const auto flag5 = call(amount_phrase, 0x10680d84, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        StackWords{c} << nationality << clubs[club].short_name() << subject_full << subject_short << amount_text;
        say(story, 0x21e92, 0x10680e04, {0x64, twice(clubs[club].short_marker()), 0x64, twice(signed_byte(flag5))}, {{27, flag5}});
        return;
    }
    if (cmp(tone, 0) == 0) {
        StackWords{c} << subject_full;
        say(story, 0x216f9, 0x10680980, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), clubs[club].short_name(), amount_text});
        return;
    }
    if (cmp(tone, 0) <= 0) {
        if (cmp(signed_byte(domestic), 0) != 0) {
            const auto flag4 = call(amount_phrase, 0x10680cc8, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
            StackWords{c} << clubs[club].short_name() << subject_full << subject_short << amount_text;
            say(story, 0x21d65, 0x10680d40, {twice(clubs[club].short_marker()), twice(0x64), twice(signed_byte(flag4))}, {{27, flag4}});
            return;
        }
        call(nationality_of, 0x10680d64, {clubs[club].nation(), nationality, 0x78, 0x1f});
        const auto flag5 = call(amount_phrase, 0x10680d84, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        StackWords{c} << nationality << clubs[club].short_name() << subject_full << subject_short << amount_text;
        say(story, 0x21e92, 0x10680e04, {0x64, twice(clubs[club].short_marker()), 0x64, twice(signed_byte(flag5))}, {{27, flag5}});
        return;
    }
    if (cmp(signed_byte(domestic), 0) != 0) {
        const auto flag6 = call(amount_phrase, 0x106809ac, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        StackWords{c} << clubs[club].short_name() << amount_text;
        say(story, 0x217ef, 0x10680a14, {0x64, twice(clubs[club].short_marker()), twice(signed_byte(flag6)), subject_full}, {{27, flag6}});
        return;
    }
    const auto flag7 = call(amount_phrase, 0x10680a38, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
    amount_flag = flag7;
    call(nationality_of, 0x10680a5c, {clubs[club].nation(), nationality, 0x78, 0x1c});
    StackWords{c} << signed_byte(amount_flag) << subject_full << nationality << clubs[club].short_name() << amount_text;
    say(story, 0x21901, 0x10680ad8, {twice(0x64), 0x64, twice(clubs[club].short_marker()), signed_byte(amount_flag)});
}

// Kind 0x2f: a pundit or columnist believes in him.
void Report::kind_2f() {
    if (cmp(tone, 1) >= 0 || cmp(tone, 0) < 0) {
        const auto flag = call(amount_phrase, 0x10687a24, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        StackWords{c} << amount_text;
        say(story, 0x296c2, 0x10687a84, {twice(clubs[club].short_marker()), twice(signed_byte(flag)), subject_full, clubs[club].short_name()}, {{27, flag}});
        return;
    }
    const auto flag2 = call(amount_phrase, 0x106879a0, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
    StackWords{c} << clubs[club].short_name();
    say(story, 0x295ae, 0x10687a00, {twice(signed_byte(flag2)), twice(clubs[club].short_marker()), amount_text, subject_full}, {{27, flag2}});
}
}
