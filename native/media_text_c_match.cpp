// The match report (kind 0x34) and how its story ends.
#include "media_text_c_detail.hpp"

namespace cm::aot::media::detail::report {
using namespace guest;
using namespace detail;

// Kind 0x34: a match report. The record must carry a match (otherwise the assertion fires); a friendly and a reserve match get a lower-case first letter.
void Report::kind_34() {
    if (cmp(signed_byte(with_match), 0) == 0) {
        say(story, 0x9f58, 0x106829d4, {});
        assertion(c, frame, 0x106829e8, 0xbc, 0xa8, 0x46f7);
        c.set({{3, c.toc(-0x7bac)}});  // (left in the argument registers by the arithmetic before)
        return;
    }
    const auto flag = call(amount_phrase, 0x10682a4c, {subject, amount, flags, amount_text, 0xc8, amount_flag, 1});
    amount_flag = flag;
    {
            // The score has the higher number first: the original orders the two goal counts with two compares of the same pair.
            const std::int32_t first = std::int8_t(first_goals), second = std::int8_t(second_goals);
            cmp(first, second);
            call(sprintf_entry, 0x10682aac, {score, pool(0x1cc7), std::uint32_t(std::max(first, second)), std::uint32_t(std::min(first, second))});
        }
    call(competition_title, 0x10682ad4, {competitions[match_competition].at, extended(match_day), extended(match_year), 1, competition_name, 0xc8});
    const auto same = call(compare_entry, 0x10682ae4, {competition_name, pool(0x247bc)});
    if (cmp(same, 0) == 0) {
        c.put8(competition_name, 0x66);
        return match_story();
    }
    const auto same2 = call(compare_entry, 0x10682b08, {competition_name, pool(0x247c5)});
    if (cmp(same2, 0) != 0) {
        return match_story();
    }
    c.put8(competition_name, 0x72);
    match_story();
}

// The match report once the competition's name is known: by tone, then by whether the first side scored more, and the club that wants him.
void Report::match_story() {
    const auto label = call(competition_article_of, 0x10682b38, {competitions[match_competition].at, extended(match_day), extended(match_year), 1});
    competition_article = label;
    if (cmp(tone, 2) == 0) {
        if (cmp(signed_byte(first_goals), signed_byte(second_goals)) <= 0) {
            if (cmp(suitor, -1) <= 0) {
                StackWords{c} << 0x64 << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name() << subject_full;
                say(story, 0x26183, 0x106844f8, {twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), twice(clubs[second_club].long_marker())});
                return outcome_story(2);
            }
            if (cmp(transfer_agreed, 0) == 0) {
                StackWords{c} << clubs[suitor].short_marker() << clubs[suitor].short_marker() << 0x64 << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name() << clubs[suitor].short_name() << subject_full;
                say(story, 0x26045, 0x10684438, {twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), twice(clubs[second_club].long_marker())}, {{22, (club * 0x248) + 0x221}});
                return outcome_story(2);
            }
            StackWords{c} << 0x64 << 0x64 << clubs[suitor].short_marker() << clubs[suitor].short_marker() << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name() << subject_full << subject_short << clubs[suitor].short_name();
            say(story, 0x25ed0, 0x10684334, {twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), twice(clubs[second_club].long_marker())}, {{22, score}, {23, (club * 0x248) + 0x221}});
            return outcome_story(2);
        }
        if (cmp(suitor, -1) <= 0) {
            StackWords{c} << 0x64 << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name() << subject_full;
            say(story, 0x25db6, 0x1068420c, {twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), twice(clubs[second_club].long_marker())});
            return;
        }
        if (cmp(transfer_agreed, 0) == 0) {
            StackWords{c} << 0x64 << clubs[suitor].short_marker() << clubs[suitor].short_marker() << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name() << clubs[suitor].short_name() << subject_full;
            say(story, 0x25c79, 0x1068414c, {twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), twice(clubs[second_club].long_marker())}, {{13, clubs.base()}});
            return;
        }
        StackWords{c} << 0x64 << 0x64 << clubs[suitor].short_marker() << clubs[suitor].short_marker() << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name() << subject_full << subject_short << clubs[suitor].short_name();
        say(story, 0x25b05, 0x10684048, {twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), twice(clubs[second_club].long_marker())}, {{13, score}, {22, (club * 0x248) + 0x221}});
        return;
    }
    if (cmp(tone, 2) > 0) {
        return match_story_other();
    }
    if (cmp(tone, 0) != 0) {
        if (cmp(tone, 0) <= 0) {
            return match_story_other();
        }
        if (cmp(signed_byte(first_goals), signed_byte(second_goals)) <= 0) {
            if (cmp(suitor, -1) <= 0) {
                StackWords{c} << clubs[second_club].long_marker() << subject_full << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name();
                say(story, 0x259d8, 0x10683d9c, {0x64, twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), clubs[second_club].long_marker()});
                return outcome_story(1);
            }
            if (cmp(transfer_agreed, 0) != 0) {
                StackWords{c} << clubs[second_club].long_marker() << clubs[suitor].short_marker() << clubs[suitor].short_marker() << subject_full << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name() << clubs[suitor].short_name();
                say(story, 0x2554a, 0x10683ab0, {0x64, twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), clubs[second_club].long_marker()}, {{14, clubs[suitor].short_marker()}, {22, (club * 0x248) + 0x221}, {23, suitor * 0x248}});
                return outcome_story(1);
            }
            if (cmp(signed_byte(transfer_rumour), 0) == 0) {
                StackWords{c} << clubs[second_club].long_marker() << clubs[suitor].short_marker() << clubs[suitor].short_marker() << subject_full << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name() << clubs[suitor].short_name();
                say(story, 0x25889, 0x10683cdc, {0x64, twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), clubs[second_club].long_marker()}, {{14, clubs[suitor].short_marker()}, {22, (club * 0x248) + 0x221}, {23, suitor * 0x248}});
                return outcome_story(1);
            }
            StackWords{c} << clubs[second_club].long_marker() << clubs[suitor].short_marker() << clubs[suitor].short_marker() << signed_byte(amount_flag) << signed_byte(amount_flag) << subject_full << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name() << clubs[suitor].short_name() << amount_text;
            say(story, 0x2569e, 0x10683bd8, {0x64, twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), clubs[second_club].long_marker()}, {{14, clubs[suitor].short_marker()}, {22, signed_byte(amount_flag)}, {23, suitor * 0x248}, {24, score}, {27, competition_name}});
            return outcome_story(1);
        }
        if (cmp(suitor, -1) <= 0) {
            StackWords{c} << clubs[second_club].long_marker() << subject_full << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name();
            say(story, 0x2541e, 0x10683998, {0x64, twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), clubs[second_club].long_marker()});
            return;
        }
        if (cmp(transfer_agreed, 0) != 0) {
            StackWords{c} << clubs[second_club].long_marker() << clubs[suitor].short_marker() << clubs[suitor].short_marker() << subject_full << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name() << clubs[suitor].short_name();
            say(story, 0x24fa5, 0x106836b4, {0x64, twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), clubs[second_club].long_marker()}, {{13, clubs.base()}, {14, clubs[suitor].short_marker()}, {22, suitor * 0x248}});
            return;
        }
        if (cmp(signed_byte(transfer_rumour), 0) == 0) {
            StackWords{c} << clubs[second_club].long_marker() << clubs[suitor].short_marker() << clubs[suitor].short_marker() << subject_full << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name() << clubs[suitor].short_name();
            say(story, 0x252d0, 0x106838d8, {0x64, twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), clubs[second_club].long_marker()}, {{13, clubs.base()}, {14, clubs[suitor].short_marker()}, {22, suitor * 0x248}});
            return;
        }
        StackWords{c} << clubs[second_club].long_marker() << clubs[suitor].short_marker() << clubs[suitor].short_marker() << 0x64 << subject_full << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name() << clubs[suitor].short_name() << subject_short;
        say(story, 0x250f8, 0x106837d4, {0x64, twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), clubs[second_club].long_marker()}, {{13, clubs.base()}, {14, clubs[suitor].short_marker()}, {22, 0x64}, {23, suitor * 0x248}, {24, subject_short}});
        return;
    }
    if (cmp(signed_byte(first_goals), signed_byte(second_goals)) <= 0) {
        if (cmp(suitor, -1) <= 0) {
            StackWords{c} << signed_byte(amount_flag) << signed_byte(amount_flag) << subject_full << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name() << amount_text << subject_short;
            say(story, 0x24e5b, 0x10683418, {twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), twice(clubs[second_club].long_marker())});
            return outcome_story(0);
        }
        if (cmp(transfer_agreed, 0) != 0) {
            StackWords{c} << subject_full << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name();
            say(story, 0x24c32, 0x10683094, {twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), twice(clubs[second_club].long_marker())});
            StackWords{c} << clubs[suitor].short_name();
            say(paragraph, 0x24cf7, 0x106830f0, {twice(signed_byte(amount_flag)), twice(clubs[suitor].short_marker()), amount_text, subject_short});
            call(append_entry, 0x106830fc, {story, paragraph});
            return outcome_story(0);
        }
        if (cmp(signed_byte(transfer_rumour), 0) == 0) {
            StackWords{c} << subject_full << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name();
            say(story, 0x24c32, 0x106832d8, {twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), twice(clubs[second_club].long_marker())});
            StackWords{c} << clubs[suitor].short_name();
            say(paragraph, 0x24a31, 0x10683334, {twice(signed_byte(amount_flag)), twice(clubs[suitor].short_marker()), amount_text, subject_short});
            call(append_entry, 0x10683340, {story, paragraph});
            return outcome_story(0);
        }
        StackWords{c} << subject_full << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name();
        say(story, 0x24c32, 0x106831c0, {twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), twice(clubs[second_club].long_marker())});
        say(paragraph, 0x24db4, 0x10683214, {twice(signed_byte(amount_flag)), twice(clubs[suitor].short_marker()), amount_text, clubs[suitor].short_name()});
        call(append_entry, 0x10683220, {story, paragraph});
        return outcome_story(0);
    }
    if (cmp(suitor, -1) <= 0) {
        StackWords{c} << signed_byte(amount_flag) << signed_byte(amount_flag) << subject_full << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name() << amount_text << subject_short;
        say(story, 0x24ae9, 0x10682fc8, {twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), twice(clubs[second_club].long_marker())});
        return;
    }
    if (cmp(transfer_agreed, 0) != 0) {
        StackWords{c} << subject_full << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name();
        say(story, 0x247cd, 0x10682c3c, {twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), twice(clubs[second_club].long_marker())});
        StackWords{c} << clubs[suitor].short_name();
        say(paragraph, 0x24891, 0x10682c98, {twice(signed_byte(amount_flag)), twice(clubs[suitor].short_marker()), amount_text, subject_short});
        call(append_entry, 0x10682ca4, {story, paragraph});
        return;
    }
    if (cmp(signed_byte(transfer_rumour), 0) == 0) {
        StackWords{c} << subject_full << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name();
        say(story, 0x247cd, 0x10682e88, {twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), twice(clubs[second_club].long_marker())});
        StackWords{c} << clubs[suitor].short_name();
        say(paragraph, 0x24a31, 0x10682ee4, {twice(signed_byte(amount_flag)), twice(clubs[suitor].short_marker()), amount_text, subject_short});
        call(append_entry, 0x10682ef0, {story, paragraph});
        return;
    }
    StackWords{c} << subject_full << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name();
    say(story, 0x247cd, 0x10682d68, {twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), twice(clubs[second_club].long_marker())});
    StackWords{c} << clubs[suitor].short_name();
    say(paragraph, 0x2494d, 0x10682dc4, {twice(signed_byte(amount_flag)), twice(clubs[suitor].short_marker()), amount_text, subject_short});
    call(append_entry, 0x10682dd0, {story, paragraph});
}

// The match report for the tones other than 0, 1 and 2.
void Report::match_story_other() {
    if (cmp(signed_byte(first_goals), signed_byte(second_goals)) <= 0) {
        if (cmp(suitor, -1) <= 0) {
            StackWords{c} << signed_byte(amount_flag) << signed_byte(amount_flag) << 0x64 << 0x64 << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name() << amount_text << subject_full << legend_name;
            say(story, 0x2659e, 0x106851ac, {twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), twice(clubs[second_club].long_marker())});
            return outcome_story(3);
        }
        if (cmp(transfer_agreed, 0) != 0) {
            call(date_of, 0x10684c40, {date_short, extended(due_day), due_year});
            call(date_text, 0x10684c54, {date_short, due_date_short, 0x78, 0xff});
            StackWords{c} << signed_byte(amount_flag) << signed_byte(amount_flag) << 0x64 << 0x64 << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name() << amount_text << subject_full << legend_name;
            say(story, 0x2659e, 0x10684d38, {twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), twice(clubs[second_club].long_marker())});
            StackWords{c} << due_date_short;
            say(paragraph, 0x263fa, 0x10684d94, {0x64, twice(clubs[suitor].short_marker()), 0x64, subject_short, clubs[suitor].short_name()});
            call(append_entry, 0x10684da0, {story, paragraph});
            return outcome_story(3);
        }
        if (cmp(signed_byte(transfer_rumour), 0) == 0) {
            StackWords{c} << signed_byte(amount_flag) << signed_byte(amount_flag) << 0x64 << 0x64 << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name() << amount_text << subject_full << legend_name;
            say(story, 0x2659e, 0x1068501c, {twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), twice(clubs[second_club].long_marker())});
            StackWords{c} << clubs[suitor].nation().name() << clubs[suitor].short_name();
            say(paragraph, 0x26520, 0x106850b8, {0x64, twice(clubs[suitor].nation().marker()), twice(clubs[suitor].short_marker()), subject_short});
            call(append_entry, 0x106850c4, {story, paragraph});
            return outcome_story(3);
        }
        StackWords{c} << signed_byte(amount_flag) << signed_byte(amount_flag) << 0x64 << 0x64 << clubs[club].short_name() << score << competition_name << clubs[second_club].long_name() << amount_text << subject_full << legend_name;
        say(story, 0x2659e, 0x10684e94, {twice(clubs[club].short_marker()), twice(signed_byte(competition_article)), twice(clubs[second_club].long_marker())});
        StackWords{c} << clubs[suitor].short_name();
        say(paragraph, 0x266ef, 0x10684f28, {twice(clubs[suitor].nation().marker()), twice(clubs[suitor].short_marker()), subject_short, clubs[suitor].nation().name()});
        call(append_entry, 0x10684f34, {story, paragraph});
        return outcome_story(3);
    }
    if (cmp(suitor, -1) <= 0) {
        StackWords{c} << signed_byte(amount_flag) << signed_byte(amount_flag) << 0x64 << 0x64 << clubs[club].short_name() << clubs[second_club].long_name() << score << competition_name << amount_text << subject_full << legend_name;
        say(story, 0x2629e, 0x10684c18, {twice(clubs[club].short_marker()), twice(clubs[second_club].long_marker()), twice(signed_byte(competition_article))}, {{13, score}, {14, (second_club * 0x248) + 0x1ee}, {22, second_club}});
        return;
    }
    if (cmp(transfer_agreed, 0) != 0) {
        call(date_of, 0x106846a4, {date_long, extended(due_day), due_year});
        call(date_text, 0x106846b8, {date_long, due_date, 0x78, 0xff});
        StackWords{c} << signed_byte(amount_flag) << signed_byte(amount_flag) << 0x64 << 0x64 << clubs[club].short_name() << clubs[second_club].long_name() << score << competition_name << amount_text << subject_full << legend_name;
        say(story, 0x2629e, 0x1068479c, {twice(clubs[club].short_marker()), twice(clubs[second_club].long_marker()), twice(signed_byte(competition_article))}, {{13, score}, {14, (second_club * 0x248) + 0x1ee}, {22, second_club}});
        StackWords{c} << due_date;
        say(paragraph, 0x263fa, 0x106847f8, {0x64, twice(clubs[suitor].short_marker()), 0x64, subject_short, clubs[suitor].short_name()});
        call(append_entry, 0x10684804, {story, paragraph});
        return;
    }
    if (cmp(signed_byte(transfer_rumour), 0) == 0) {
        StackWords{c} << signed_byte(amount_flag) << signed_byte(amount_flag) << 0x64 << 0x64 << clubs[club].short_name() << clubs[second_club].long_name() << score << competition_name << amount_text << subject_full << legend_name;
        say(story, 0x2629e, 0x10684a88, {twice(clubs[club].short_marker()), twice(clubs[second_club].long_marker()), twice(signed_byte(competition_article))}, {{13, score}, {14, (second_club * 0x248) + 0x1ee}, {22, second_club}});
        StackWords{c} << clubs[suitor].nation().name() << clubs[suitor].short_name();
        say(paragraph, 0x26520, 0x10684b24, {0x64, twice(clubs[suitor].nation().marker()), twice(clubs[suitor].short_marker()), subject_short});
        call(append_entry, 0x10684b30, {story, paragraph});
        return;
    }
    StackWords{c} << signed_byte(amount_flag) << signed_byte(amount_flag) << 0x64 << 0x64 << clubs[club].short_name() << clubs[second_club].long_name() << score << competition_name << amount_text << subject_full << legend_name;
    say(story, 0x2629e, 0x106848f8, {twice(clubs[club].short_marker()), twice(clubs[second_club].long_marker()), twice(signed_byte(competition_article))}, {{13, score}, {14, (second_club * 0x248) + 0x1ee}, {22, second_club}});
    StackWords{c} << clubs[suitor].nation().name() << clubs[suitor].short_name();
    say(paragraph, 0x26475, 0x10684994, {0x64, twice(clubs[suitor].nation().marker()), twice(clubs[suitor].short_marker()), subject_short});
    call(append_entry, 0x106849a0, {story, paragraph});
}

// How a match story ends (outcome 1-4: won, lost, won, lost). Four places end a match story this way, each
// with its own return addresses.
void Report::outcome_story(unsigned place) {
    static constexpr std::uint32_t first_backs[4]{0x10683464, 0x10683de8, 0x10684544, 0x106851f8};
    match_ending(c, std::int32_t(std::uint32_t(outcome)), first_backs[place], paragraph, story, [&] {
        return std::array<std::uint32_t, 4>{clubs[club].short_marker(), clubs[club].short_marker(), clubs[club].short_name(), clubs.base()};
    });
}
}
