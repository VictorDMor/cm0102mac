// Stories by kind, the second half.
#include "media_text_c_detail.hpp"

namespace cm::aot::media::detail::report {
using namespace guest;
using namespace detail;

// Kind 0x30: a former star (the legend) gives his opinion.
void Report::kind_30() {
    if (cmp(tone, 1) == 0) {
        if (cmp(legend, -1) > 0 && cmp(previous_club, -1) > 0) {
            const auto job = call(job_title, 0x106810b8, {signed_byte(legend_job), 0, 0, frame.at(0x89)});
            StackWords{c} << clubs[previous_club].short_marker() << signed_byte(byte_at(0x89)) << signed_byte(byte_at(0x89)) << clubs[club].short_name() << amount_text << subject_full << clubs[previous_club].short_name() << job << legend_name;
            say(story, 0x222eb, 0x10681188, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), 0x64, clubs[previous_club].short_marker()}, {{13, club * 0x248}, {14, legend_name}, {22, (previous_club * 0x248) + 0x221}, {23, clubs.base()}});
            return;
        }
        if (cmp(legend, -1) <= 0 || cmp(std::uint32_t(std::int32_t(subject + (club + legend)) % 13), 7) <= 0) {
            StackWords{c} << amount_text << subject_full;
            say(story, 0x22535, 0x1068130c, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), 0x64, clubs[club].short_name()});
            return;
        }
        StackWords{c} << people[legend].nation().marker() << people[legend].nation().marker() << 0x64 << 0x64 << clubs[club].short_name() << amount_text << subject_full << people[legend].nation().name() << legend_name;
        say(story, 0x22412, 0x106812a4, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), twice(0x64)}, {{13, clubs[club].short_name()}, {14, clubs.base()}, {22, 0x64}, {23, people[legend].nation().marker()}, {24, legend_name}});
        return;
    }
    if (cmp(tone, 1) > 0 || cmp(tone, 0) < 0) {
        const auto flag = call(amount_phrase, 0x10681330, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        StackWords{c} << amount_text;
        say(story, 0x22616, 0x10681390, {twice(clubs[club].short_marker()), twice(signed_byte(flag)), clubs[club].short_name(), subject_full}, {{27, flag}});
        return;
    }
    const auto flag2 = call(amount_phrase, 0x10680e44, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
    amount_flag = flag2;
    if (cmp(legend, -1) > 0 && cmp(previous_club, -1) > 0) {
        const auto job2 = call(job_title, 0x10680e70, {signed_byte(legend_job), 0, 0, frame.at(0x8a)});
        StackWords{c} << signed_byte(amount_flag) << signed_byte(amount_flag) << clubs[previous_club].short_name() << job2 << legend_name << clubs[club].short_name() << amount_text << subject_full;
        say(story, 0x21fdd, 0x10680f38, {twice(clubs[previous_club].short_marker()), twice(signed_byte(byte_at(0x8a))), twice(clubs[club].short_marker())});
        return;
    }
    if (cmp(legend, -1) <= 0 || cmp(std::uint32_t(std::int32_t(subject + (club + legend)) % 3), 0) == 0) {
        StackWords{c} << subject_full;
        say(story, 0x22207, 0x1068108c, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), clubs[club].short_name(), amount_text});
        return;
    }
    StackWords{c} << people[legend].nation().name() << legend_name << clubs[club].short_name() << amount_text << subject_full;
    say(story, 0x220f5, 0x1068102c, {twice(people[legend].nation().marker()), twice(clubs[club].short_marker()), twice(signed_byte(amount_flag))});
}

// Kind 0x31: a former star gives his opinion (the other tone of 0x30).
void Report::kind_31() {
    if (cmp(tone, 1) == 0) {
        if (cmp(legend, -1) > 0 && cmp(previous_club, -1) > 0) {
            const auto job = call(job_title, 0x1068164c, {signed_byte(legend_job), 0, 0, frame.at(0x87)});
            StackWords{c} << clubs[previous_club].short_marker() << signed_byte(byte_at(0x87)) << signed_byte(byte_at(0x87)) << clubs[club].short_name() << amount_text << subject_full << clubs[previous_club].short_name() << job << legend_name;
            say(story, 0x22a0d, 0x1068171c, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), 0x64, clubs[previous_club].short_marker()}, {{13, club * 0x248}, {14, legend_name}, {22, (previous_club * 0x248) + 0x221}, {23, clubs.base()}});
            return;
        }
        if (cmp(legend, -1) <= 0 || cmp(std::uint32_t(std::int32_t(subject + (club + legend)) % 11), 6) >= 0) {
            StackWords{c} << amount_text << subject_full;
            say(story, 0x22c35, 0x10681888, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), 0x64, clubs[club].short_name()});
            return;
        }
        StackWords{c} << people[legend].nation().marker() << clubs[club].short_name() << amount_text << subject_full << people[legend].nation().name() << legend_name;
        say(story, 0x22b26, 0x10681820, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), 0x64, people[legend].nation().marker()}, {{13, clubs.base()}});
        return;
    }
    if (cmp(tone, 1) > 0 || cmp(tone, 0) < 0) return say(story, 0x22d08, 0x106818e0, {twice(clubs[club].short_marker()), 0x64, clubs[club].short_name(), subject_full, subject_short});
    const auto flag = call(amount_phrase, 0x106813d0, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
    amount_flag = flag;
    if (cmp(legend, -1) > 0 && cmp(previous_club, -1) > 0) {
        const auto job2 = call(job_title, 0x106813fc, {signed_byte(legend_job), 0, 0, frame.at(0x88)});
        StackWords{c} << signed_byte(amount_flag) << signed_byte(amount_flag) << clubs[previous_club].short_name() << job2 << legend_name << clubs[club].short_name() << amount_text << subject_full;
        say(story, 0x22722, 0x106814c4, {twice(clubs[previous_club].short_marker()), twice(signed_byte(byte_at(0x88))), twice(clubs[club].short_marker())});
        return;
    }
    if (cmp(legend, -1) <= 0 || cmp(std::uint32_t(std::int32_t(subject + (club + legend)) % 11), 6) >= 0) {
        StackWords{c} << subject_full;
        say(story, 0x2292a, 0x10681620, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), clubs[club].short_name(), amount_text});
        return;
    }
    StackWords{c} << people[legend].nation().name() << legend_name << clubs[club].short_name() << amount_text << subject_full;
    say(story, 0x2282b, 0x106815c0, {twice(people[legend].nation().marker()), twice(clubs[club].short_marker()), twice(signed_byte(amount_flag))});
}

// Kind 0x32: an outlet credits his performances with a payoff.
void Report::kind_32() {
    if (cmp(tone, 0) == 0) {
        StackWords{c} << subject_full;
        say(story, 0x244d9, 0x1068287c, {twice(signed_byte(article_marker)), twice(clubs[club].short_marker()), outlet, clubs[club].short_name()});
        return;
    }
    if (cmp(tone, 1) != 0) {
        const auto flag = call(amount_phrase, 0x1068294c, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        StackWords{c} << amount_text << subject_short;
        say(story, 0x24679, 0x106829b4, {twice(clubs[club].short_marker()), twice(signed_byte(flag)), subject_full, clubs[club].short_name()}, {{27, flag}});
        return;
    }
    const auto flag2 = call(amount_phrase, 0x106828a8, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
    StackWords{c} << outlet << amount_text << subject_full << clubs[club].short_name();
    say(story, 0x245a1, 0x10682928, {twice(signed_byte(article_marker)), twice(signed_byte(flag2)), twice(clubs[club].short_marker())}, {{27, flag2}});
}

// Kind 0x33: the outlet praises him; the nations and the competition named in the story vary the wording. Ends in kind_33_after.
void Report::kind_33() {
    if (cmp(tone, 2) == 0) {
        const auto flag = call(amount_phrase, 0x10686088, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        amount_flag = flag;
        if (cmp(legend, -1) == 0) {
            StackWords{c} << legend_name << clubs[club].short_name() << amount_text << subject_full << competitions[competition].name();
            say(story, 0x2762d, 0x1068613c, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), twice(competitions[competition].marker())});
            return kind_33_after();
        }
        if (cmp(nation_a, clubs[club].nation().id()) == 0 || cmp(nation_b, clubs[club].nation().id()) == 0) {
            if (cmp(previous_club, -1) <= 0) {
                StackWords{c} << 0x64 << competitions[competition].marker() << competitions[competition].marker() << people[legend].nation().name() << legend_name << clubs[club].short_name() << amount_text << subject_full << competitions[competition].name();
                say(story, 0x27865, 0x106863b4, {twice(people[legend].nation().marker()), twice(clubs[club].short_marker()), twice(signed_byte(amount_flag))}, {{13, club * 0x248}, {14, (competition * 0x6c) + 0x4d}});
                return kind_33_after();
            }
            const auto job = call(job_title, 0x10686198, {signed_byte(legend_job), 0, 0, frame.at(0x82)});
            StackWords{c} << signed_byte(amount_flag) << signed_byte(amount_flag) << competitions[competition].marker() << competitions[competition].marker() << clubs[previous_club].short_name() << job << legend_name << clubs[club].short_name() << amount_text << subject_full << competitions[competition].name();
            say(story, 0x27719, 0x106862a4, {twice(clubs[previous_club].short_marker()), twice(signed_byte(byte_at(0x82))), twice(clubs[club].short_marker())}, {{13, (previous_club * 0x248) + 0x221}});
            return kind_33_after();
        }
        if (cmp(nation_a, -1) <= 0 || cmp(nation_b, -1) <= 0) {
            if (cmp(previous_club, -1) <= 0) {
                StackWords{c} << competitions[competition].marker() << competitions[competition].marker() << nations[nation_a].marker() << nations[nation_a].marker() << people[legend].nation().name() << legend_name << clubs[club].short_name() << amount_text << subject_full << competitions[competition].name() << nations[nation_a].name();
                say(story, 0x27dda, 0x106869cc, {twice(people[legend].nation().marker()), twice(clubs[club].short_marker()), twice(signed_byte(amount_flag))}, {{13, amount_text}, {14, (club * 0x248) + 0x221}, {22, legend_name}, {23, legend * 0x70}, {24, subject_full}});
                return kind_33_after();
            }
            const auto job2 = call(job_title, 0x10686730, {signed_byte(legend_job), 0, 0, frame.at(0x80)});
            StackWords{c} << signed_byte(amount_flag) << signed_byte(amount_flag) << competitions[competition].marker() << competitions[competition].marker() << nations[nation_a].marker() << nations[nation_a].marker() << clubs[previous_club].short_name() << job2 << legend_name << clubs[club].short_name() << amount_text << subject_full << competitions[competition].name() << nations[nation_a].name();
            say(story, 0x27c8f, 0x10686880, {twice(clubs[previous_club].short_marker()), twice(signed_byte(byte_at(0x80))), twice(clubs[club].short_marker())}, {{13, previous_club * 0x248}, {14, (nation_a * 0x128) + 0xe1}, {22, (competition * 0x6c) + 0x4d}, {23, clubs.base()}});
            return kind_33_after();
        }
        if (cmp(previous_club, -1) <= 0) {
            StackWords{c} << competitions[competition].marker() << competitions[competition].marker() << nations[nation_a].marker() << nations[nation_a].marker() << nations[nation_b].marker() << nations[nation_b].marker() << people[legend].nation().name() << legend_name << clubs[club].short_name() << amount_text << subject_full << competitions[competition].name() << nations[nation_a].name() << nations[nation_b].name();
            say(story, 0x27b21, 0x1068670c, {twice(people[legend].nation().marker()), twice(clubs[club].short_marker()), twice(signed_byte(amount_flag))}, {{13, (competition * 0x6c) + 0x4d}, {14, (club * 0x248) + 0x221}, {22, legend_name}, {23, legend * 0x70}, {24, amount_text}});
            return kind_33_after();
        }
        const auto job3 = call(job_title, 0x106863e8, {signed_byte(legend_job), 0, 0, frame.at(0x81)});
        StackWords{c} << signed_byte(amount_flag) << signed_byte(amount_flag) << competitions[competition].marker() << competitions[competition].marker() << nations[nation_a].marker() << nations[nation_a].marker() << nations[nation_b].marker() << nations[nation_b].marker() << clubs[previous_club].short_name() << job3 << legend_name << clubs[club].short_name() << amount_text << subject_full << competitions[competition].name() << nations[nation_a].name() << nations[nation_b].name();
        say(story, 0x279ad, 0x1068657c, {twice(clubs[previous_club].short_marker()), twice(signed_byte(byte_at(0x81))), twice(clubs[club].short_marker())}, {{13, legend_name}, {14, clubs[previous_club].short_name()}, {22, clubs.base()}, {23, nations.base()}, {24, amount_text}, {27, subject_full}});
        return kind_33_after();
    }
    if (cmp(tone, 2) > 0) {
        call(nationality_of, 0x106869f0, {clubs[club].nation(), nationality, 0x78, 0xc});
        if (cmp(nation_a, clubs[club].nation().id()) == 0 || cmp(nation_b, clubs[club].nation().id()) == 0) {
            StackWords{c} << nationality << amount_text << subject_full << competitions[competition].name();
            say(story, 0x27f1f, 0x10686a9c, {twice(0x64), twice(signed_byte(amount_flag)), twice(competitions[competition].marker())});
            return kind_33_after();
        }
        if (cmp(nation_a, -1) <= 0 || cmp(nation_b, -1) <= 0) {
            StackWords{c} << nations[nation_a].marker() << nations[nation_a].marker() << nationality << amount_text << subject_full << competitions[competition].name() << nations[nation_a].name();
            say(story, 0x2810d, 0x10686c68, {twice(0x64), twice(signed_byte(amount_flag)), twice(competitions[competition].marker())}, {{13, (nation_a * 0x128) + 0xe1}, {14, (competition * 0x6c) + 0x4d}});
            return kind_33_after();
        }
        StackWords{c} << nations[nation_a].marker() << nations[nation_a].marker() << nations[nation_b].marker() << nations[nation_b].marker() << nationality << amount_text << subject_full << competitions[competition].name() << nations[nation_a].name() << nations[nation_b].name();
        say(story, 0x27ff7, 0x10686bac, {twice(0x64), twice(signed_byte(amount_flag)), twice(competitions[competition].marker())}, {{13, competitions.base()}, {14, nationality}, {22, nations[nation_b].marker()}, {23, nations.base()}, {24, subject_full}});
        return kind_33_after();
    }
    if (cmp(tone, 0) == 0) {
        const auto flag2 = call(amount_phrase, 0x1068535c, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
        amount_flag = flag2;
        if (cmp(nation_a, clubs[club].nation().id()) == 0 || cmp(nation_b, clubs[club].nation().id()) == 0) {
            if (cmp(signed_byte(domestic), 0) != 0) {
                StackWords{c} << outlet << amount_text << subject_full << competitions[competition].name();
                say(story, 0x268d9, 0x106854e8, {twice(signed_byte(article_marker)), twice(signed_byte(amount_flag)), twice(competitions[competition].marker())});
                return kind_33_after();
            }
            StackWords{c} << 0x64 << competitions[competition].marker() << competitions[competition].marker() << outlet << clubs[club].short_name() << amount_text << subject_full << competitions[competition].name();
            say(story, 0x26793, 0x10685468, {twice(signed_byte(article_marker)), twice(clubs[club].short_marker()), twice(signed_byte(amount_flag))}, {{13, competitions[competition].marker()}});
            return kind_33_after();
        }
        if (cmp(nation_a, -1) <= 0 || cmp(nation_b, -1) <= 0) {
            if (cmp(signed_byte(domestic), 0) != 0) {
                StackWords{c} << competitions[competition].marker() << nations[nation_a].marker() << nations[nation_a].marker() << outlet << amount_text << subject_full << competitions[competition].name() << nations[nation_a].name();
                say(story, 0x26e3b, 0x10685948, {twice(signed_byte(article_marker)), twice(signed_byte(amount_flag)), 0x64, competitions[competition].marker()}, {{13, nations[nation_a].marker()}, {14, nations.base()}});
                return kind_33_after();
            }
            StackWords{c} << 0x64 << competitions[competition].marker() << competitions[competition].marker() << nations[nation_a].marker() << nations[nation_a].marker() << outlet << clubs[club].short_name() << amount_text << subject_full << competitions[competition].name() << nations[nation_a].name();
            say(story, 0x26cdd, 0x1068587c, {twice(signed_byte(article_marker)), twice(clubs[club].short_marker()), twice(signed_byte(amount_flag))}, {{13, (club * 0x248) + 0x221}, {14, nations[nation_a].marker()}, {22, nations.base()}});
            return kind_33_after();
        }
        if (cmp(signed_byte(domestic), 0) != 0) {
            StackWords{c} << competitions[competition].marker() << nations[nation_a].marker() << nations[nation_a].marker() << nations[nation_b].marker() << nations[nation_b].marker() << outlet << amount_text << subject_full << competitions[competition].name() << nations[nation_a].name() << nations[nation_b].name();
            say(story, 0x26b7d, 0x10685764, {twice(signed_byte(article_marker)), twice(signed_byte(amount_flag)), 0x64, competitions[competition].marker()}, {{13, (competition * 0x6c) + 0x4d}, {14, nations[nation_b].marker()}, {22, nations.base()}, {23, nations.base()}});
            return kind_33_after();
        }
        StackWords{c} << 0x64 << competitions[competition].marker() << competitions[competition].marker() << nations[nation_a].marker() << nations[nation_a].marker() << nations[nation_b].marker() << nations[nation_b].marker() << outlet << clubs[club].short_name() << amount_text << subject_full << competitions[competition].name() << nations[nation_a].name() << nations[nation_b].name();
        say(story, 0x269f6, 0x10685654, {twice(signed_byte(article_marker)), twice(clubs[club].short_marker()), twice(signed_byte(amount_flag))}, {{13, clubs.base()}, {14, nations[nation_b].marker()}, {22, nations.base()}, {23, nations.base()}, {24, subject_full}});
        return kind_33_after();
    }
    if (cmp(tone, 0) <= 0) {
        call(nationality_of, 0x106869f0, {clubs[club].nation(), nationality, 0x78, 0xc});
        if (cmp(nation_a, clubs[club].nation().id()) == 0 || cmp(nation_b, clubs[club].nation().id()) == 0) {
            StackWords{c} << nationality << amount_text << subject_full << competitions[competition].name();
            say(story, 0x27f1f, 0x10686a9c, {twice(0x64), twice(signed_byte(amount_flag)), twice(competitions[competition].marker())});
            return kind_33_after();
        }
        if (cmp(nation_a, -1) <= 0 || cmp(nation_b, -1) <= 0) {
            StackWords{c} << nations[nation_a].marker() << nations[nation_a].marker() << nationality << amount_text << subject_full << competitions[competition].name() << nations[nation_a].name();
            say(story, 0x2810d, 0x10686c68, {twice(0x64), twice(signed_byte(amount_flag)), twice(competitions[competition].marker())}, {{13, (nation_a * 0x128) + 0xe1}, {14, (competition * 0x6c) + 0x4d}});
            return kind_33_after();
        }
        StackWords{c} << nations[nation_a].marker() << nations[nation_a].marker() << nations[nation_b].marker() << nations[nation_b].marker() << nationality << amount_text << subject_full << competitions[competition].name() << nations[nation_a].name() << nations[nation_b].name();
        say(story, 0x27ff7, 0x10686bac, {twice(0x64), twice(signed_byte(amount_flag)), twice(competitions[competition].marker())}, {{13, competitions.base()}, {14, nationality}, {22, nations[nation_b].marker()}, {23, nations.base()}, {24, subject_full}});
        return kind_33_after();
    }
    call(amount_phrase, 0x1068596c, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
    if (cmp(nation_a, clubs[club].nation().id()) == 0 || cmp(nation_b, clubs[club].nation().id()) == 0) {
        if (cmp(previous_club, -1) <= 0) {
            StackWords{c} << clubs[club].short_name() << subject_full << competitions[competition].name();
            say(story, 0x27085, 0x10685b5c, {twice(clubs[club].short_marker()), 0x64, twice(competitions[competition].marker()), legend_name});
            return kind_33_after();
        }
        const auto job4 = call(job_title, 0x106859c4, {signed_byte(legend_job), 0, 0, frame.at(0x85)});
        StackWords{c} << 0x64 << competitions[competition].marker() << competitions[competition].marker() << clubs[previous_club].short_name() << job4 << legend_name << clubs[club].short_name() << subject_full << competitions[competition].name();
        say(story, 0x26f72, 0x10685ac0, {twice(clubs[previous_club].short_marker()), twice(signed_byte(byte_at(0x85))), twice(clubs[club].short_marker())});
        return kind_33_after();
    }
    if (cmp(nation_a, -1) <= 0 || cmp(nation_b, -1) <= 0) {
        if (cmp(previous_club, -1) <= 0) {
            StackWords{c} << legend_name << clubs[club].short_name() << subject_full << competitions[competition].name() << nations[nation_a].name();
            say(story, 0x27524, 0x10686064, {twice(clubs[club].short_marker()), twice(competitions[competition].marker()), twice(nations[nation_a].marker())});
            return kind_33_after();
        }
        const auto job5 = call(job_title, 0x10685e54, {signed_byte(legend_job), 0, 0, frame.at(0x83)});
        StackWords{c} << competitions[competition].marker() << competitions[competition].marker() << nations[nation_a].marker() << nations[nation_a].marker() << clubs[previous_club].short_name() << job5 << legend_name << clubs[club].short_name() << subject_full << competitions[competition].name() << nations[nation_a].name();
        say(story, 0x273fe, 0x10685f8c, {twice(clubs[previous_club].short_marker()), twice(signed_byte(byte_at(0x83))), twice(clubs[club].short_marker())}, {{13, subject_full}, {14, (club * 0x248) + 0x221}, {22, clubs.base()}});
        return kind_33_after();
    }
    if (cmp(previous_club, -1) <= 0) {
        StackWords{c} << nations[nation_a].marker() << nations[nation_b].marker() << nations[nation_b].marker() << legend_name << clubs[club].short_name() << subject_full << competitions[competition].name() << nations[nation_a].name() << nations[nation_b].name();
        say(story, 0x272ca, 0x10685e30, {twice(clubs[club].short_marker()), 0x64, twice(competitions[competition].marker()), nations[nation_a].marker()});
        return kind_33_after();
    }
    const auto job6 = call(job_title, 0x10685b90, {signed_byte(legend_job), 0, 0, frame.at(0x84)});
    StackWords{c} << competitions[competition].marker() << competitions[competition].marker() << nations[nation_a].marker() << nations[nation_a].marker() << nations[nation_b].marker() << nations[nation_b].marker() << clubs[previous_club].short_name() << job6 << legend_name << clubs[club].short_name() << subject_full << competitions[competition].name() << nations[nation_a].name() << nations[nation_b].name();
    say(story, 0x2717b, 0x10685d0c, {twice(clubs[previous_club].short_marker()), twice(signed_byte(byte_at(0x84))), twice(clubs[club].short_marker())}, {{13, previous_club * 0x248}, {14, (nation_b * 0x128) + 0xe1}, {22, (nation_a * 0x128) + 0xe1}, {23, clubs.base()}});
    kind_33_after();
}

// The second half of kind 0x33: the player's next move, when the club that wants him is known.
void Report::kind_33_after() {
    if (cmp(suitor, -1) <= 0) {
        return;
    }
    if (cmp(transfer_agreed, 0) != 0) {
        call(date_of, 0x10686d18, {frame.at(0x90), extended(due_day), due_year});
        call(date_text, 0x10686d2c, {frame.at(0x90), frame.at(0x134), 0x78, 0xff});
        call(copy_entry, 0x10686d38, {paragraph, story});
        StackWords{c} << frame.at(0x134);
        say(story, 0x28286, 0x10686d94, {twice(clubs[suitor].long_marker()), 0x64, paragraph, subject_short, clubs[suitor].long_name()});
        return;
    }
    if (cmp(std::uint32_t(std::int32_t(legend) % 3), 0) == 0) {
        cmp(transfer_agreed, 0);
        c.set({{3, mulhw(0x55555556, legend)}});  // (left in the argument registers by the arithmetic before)
        return;
    }
    call(copy_entry, 0x10686ca8, {paragraph, story});
    say(story, 0x281fa, 0x10686cf8, {twice(clubs[suitor].long_marker()), paragraph, subject_short, clubs[suitor].long_name()});
}
}
