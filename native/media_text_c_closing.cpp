// The sentence that closes a long story, and the short story.
#include "media_text_c_detail.hpp"

namespace cm::aot::media::detail::report {
using namespace guest;
using namespace detail;

// The closing sentence of a long story (`closing` 0-4, by kind, or by the league position).
void Report::paragraph_two() {
    if (cmpl(users_manager, other_row) != 0) {
        if (cmp(closing, 2) == 0) {
            cmpl(kind - 0x29, 0xb);  // (the range check of the jump table)
            switch (kind - 0x29) {
            case 0: case 1: case 4: case 5: case 7: {
                return say(paragraph, 0x2aa36, 0x10688168, {twice(0x64), other_full, subject_short});
            }
            case 3: {
                return say(paragraph, 0x2aab2, 0x1068818c, {twice(0x64), other_full, subject_short});
            }
            case 6: {
                return say(paragraph, 0x2a991, 0x10688144, {twice(0x64), other_full, subject_short});
            }
            case 8: {
                return say(paragraph, 0x2ab57, 0x106881b0, {twice(0x64), other_full, subject_short});
            }
            case 9: {
                return say(paragraph, 0x2ac06, 0x106881d4, {twice(0x64), other_full, subject_short});
            }
            case 10: {
                return say(paragraph, 0x2ad56, 0x1068821c, {twice(0x64), other_full, subject_short});
            }
            case 11: {
                return say(paragraph, 0x2acab, 0x106881f8, {twice(0x64), other_full, subject_short});
            }
            default: {
                if ((cmp(second_division, 0) != 0 && cmp(position, 0x24) > 0) || (cmp(second_division, 0) == 0 && cmp(position, 0x22) > 0)) return say(paragraph, 0x2adeb, 0x10688270, {twice(0x64), other_full, subject_short});
                if (cmp(position, 0x18) >= 0) return say(paragraph, 0x2af89, 0x106882c4, {twice(0x64), other_full, subject_short});
                return say(paragraph, 0x2aed7, 0x106882a0, {twice(0x64), other_full, subject_short});
            }
            }
        }
        if (cmp(closing, 2) > 0) {
            if (cmp(closing, 4) >= 0) {
                call(sprintf_entry, 0x106882ec, {paragraph, pool(0x2a3d8)});
                return;
            }
            return say(paragraph, 0x2a3d8, 0x106882d8, {});
        }
        if (cmp(closing, 0) <= 0) {
            call(sprintf_entry, 0x106882ec, {paragraph, pool(0x2a3d8)});
            return;
        }
        cmpl(kind - 0x29, 0xb);  // (the range check of the jump table)
        switch (kind - 0x29) {
        case 0: {
            return say(paragraph, 0x2a4cb, 0x10688010, {other_full});
        }
        case 1: {
            return say(paragraph, 0x2a54c, 0x1068802c, {other_full, subject_short});
        }
        case 3: {
            return say(paragraph, 0x2a5da, 0x10688044, {other_full});
        }
        case 4: case 7: {
            return say(paragraph, 0x2a65d, 0x10688060, {other_full, subject_short});
        }
        case 5: {
            return say(paragraph, 0x2a456, 0x10687ff8, {other_full});
        }
        case 6: {
            return say(paragraph, 0x2a3da, 0x10687fe0, {other_full});
        }
        case 8: {
            return say(paragraph, 0x2a6dd, 0x1068807c, {other_full, subject_short});
        }
        case 9: {
            return say(paragraph, 0x2a74f, 0x1068809c, {0x64, other_full, subject_short});
        }
        case 10: {
            return say(paragraph, 0x2a863, 0x106880dc, {0x64, other_full, subject_short});
        }
        case 11: {
            return say(paragraph, 0x2a7db, 0x106880bc, {0x64, other_full, subject_short});
        }
        default: {
            return say(paragraph, 0x2a8fc, 0x106880fc, {0x64, other_full, subject_short});
        }
        }
    }
    if (cmp(closing, 2) == 0) {
        cmpl(kind - 0x29, 0xb);  // (the range check of the jump table)
        switch (kind - 0x29) {
        case 0: case 1: case 4: case 5: case 7: {
            return say(paragraph, 0x29ef8, 0x10687e4c, {0x64, subject_short});
        }
        case 3: {
            return say(paragraph, 0x29f4c, 0x10687e64, {subject_short});
        }
        case 6: {
            return say(paragraph, 0x29e75, 0x10687e30, {0x64, subject_short});
        }
        case 8: {
            return say(paragraph, 0x29fc2, 0x10687e7c, {subject_short});
        }
        case 9: {
            return say(paragraph, 0x2a047, 0x10687e94, {subject_short});
        }
        case 10: {
            return say(paragraph, 0x2a154, 0x10687ec4, {subject_short});
        }
        case 11: {
            return say(paragraph, 0x2a0cd, 0x10687eac, {subject_short});
        }
        default: {
            if ((cmp(second_division, 0) != 0 && cmp(position, 0x24) > 0) || (cmp(second_division, 0) == 0 && cmp(position, 0x22) > 0)) return say(paragraph, 0x2a1bf, 0x10687f0c, {subject_short});
            if (cmp(position, 0x18) >= 0) return say(paragraph, 0x2a309, 0x10687f48, {subject_short});
            return say(paragraph, 0x2a281, 0x10687f30, {subject_short});
        }
        }
    }
    if (cmp(closing, 2) > 0) {
        if (cmp(closing, 4) >= 0) {
            call(sprintf_entry, 0x10687f70, {paragraph, pool(0x2a3d8)});
            return;
        }
        return say(paragraph, 0x2a3a2, 0x10687f5c, {});
    }
    if (cmp(closing, 0) <= 0) {
        call(sprintf_entry, 0x10687f70, {paragraph, pool(0x2a3d8)});
        return;
    }
    cmpl(kind - 0x29, 0xb);  // (the range check of the jump table)
    switch (kind - 0x29) {
    case 0: {
        return say(paragraph, 0x29ade, 0x10687d24, {});
    }
    case 1: {
        return say(paragraph, 0x29b39, 0x10687d3c, {subject_short});
    }
    case 3: {
        return say(paragraph, 0x29ba1, 0x10687d50, {});
    }
    case 4: case 7: {
        return say(paragraph, 0x29bff, 0x10687d68, {subject_short});
    }
    case 5: {
        return say(paragraph, 0x29a8f, 0x10687d10, {});
    }
    case 6: {
        return say(paragraph, 0x29a39, 0x10687cfc, {});
    }
    case 8: {
        return say(paragraph, 0x29c5f, 0x10687d80, {subject_short});
    }
    case 9: {
        return say(paragraph, 0x29cc9, 0x10687d9c, {0x64, subject_short});
    }
    case 10: {
        return say(paragraph, 0x29d91, 0x10687dd4, {0x64, subject_short});
    }
    case 11: {
        return say(paragraph, 0x29d2f, 0x10687db8, {0x64, subject_short});
    }
    default: {
        return say(paragraph, 0x29e04, 0x10687df0, {0x64, subject_short});
    }
    }
}

// The one-line version of a story.
void Report::short_story() {
    c.put8(score, 0);
    if (cmp(std::uint32_t(std::int32_t(tone) % 8), std::uint32_t(std::int32_t(subject) % 7)) == 0) return say(story, 0x2b047, 0x106883b0, {twice(clubs[club].short_marker()), twice(signed_byte(amount_flag)), clubs[club].short_name(), amount_text});
    cmpl(kind - 0x29, 0xb);  // (the range check of the jump table)
    switch (kind - 0x29) {
    case 0: {
        if (cmp(std::uint32_t(std::int32_t(tone + subject) % 3), 1) == 0) {
            c.set({{6, 0}});  // (left in the argument registers by the arithmetic before)
            say(story, 0x2b192, 0x106884a0, {subject_short});
            return;
        }
        if (cmp(std::uint32_t(std::int32_t(tone + subject) % 3), 1) > 0 || cmp(std::uint32_t(std::int32_t(tone + subject) % 3), 0) < 0) {
            c.set({{6, 0}});  // (left in the argument registers by the arithmetic before)
            say(story, 0x2b1cc, 0x106884b8, {subject_short});
            return;
        }
        c.set({{6, 0}});  // (left in the argument registers by the arithmetic before)
        say(story, 0x2b164, 0x10688488, {subject_short});
        return;
    }
    case 1: {
        if (cmp(std::uint32_t(std::int32_t(tone + subject) % 3), 1) == 0) {
            c.set({{6, 0}});  // (left in the argument registers by the arithmetic before)
            say(story, 0x2b22e, 0x10688538, {subject_short});
            return;
        }
        if (cmp(std::uint32_t(std::int32_t(tone + subject) % 3), 1) > 0 || cmp(std::uint32_t(std::int32_t(tone + subject) % 3), 0) < 0) {
            c.set({{6, 0}});  // (left in the argument registers by the arithmetic before)
            say(story, 0x2b264, 0x10688550, {subject_short});
            return;
        }
        c.set({{6, 0}});  // (left in the argument registers by the arithmetic before)
        say(story, 0x2b1ff, 0x10688520, {subject_short});
        return;
    }
    case 2: {
        c.set({{6, 0}});  // (left in the argument registers by the arithmetic before)
        say(story, 0x2b3c1, 0x1068869c, {subject_short});
        return;
    }
    case 3: {
        c.set({{6, 0}});  // (left in the argument registers by the arithmetic before)
        say(story, 0x2b1cc, 0x106884d0, {subject_short});
        return;
    }
    case 4: {
        c.set({{6, 0}});  // (left in the argument registers by the arithmetic before)
        say(story, 0x2b137, 0x10688438, {subject_short});
        return;
    }
    case 5: {
        c.set({{6, 0}});  // (left in the argument registers by the arithmetic before)
        say(story, 0x2b0cd, 0x10688408, {subject_short});
        return;
    }
    case 6: {
        return say(story, 0x2b099, 0x106883f0, {0x64, subject_short});
    }
    case 7: case 8: {
        c.set({{6, 0}});  // (left in the argument registers by the arithmetic before)
        say(story, 0x2b103, 0x10688420, {subject_short});
        return;
    }
    case 9: {
        return say(story, 0x2b297, 0x106885a0, {twice(clubs[club].short_marker()), subject_short, clubs[club].short_name(), clubs.base()});
    }
    case 10: {
        c.set({{6, 0}});  // (left in the argument registers by the arithmetic before)
        say(story, 0x2b381, 0x10688684, {subject_short});
        return;
    }
    case 11: {
        if (cmp(std::uint32_t(std::int32_t(subject * tone) % 17), 6) <= 0) return say(story, 0x2b335, 0x1068866c, {twice(competitions[competition].marker()), subject_short, competitions[competition].name(), competitions.base()});
        return say(story, 0x2b2e8, 0x1068861c, {twice(competitions[competition].marker()), subject_short, competitions[competition].name(), competitions.base()});
    }
    default: {
        c.set({{6, 0}});  // (left in the argument registers by the arithmetic before)
        say(story, 0x2b3ea, 0x106886b4, {subject_short});
        return;
    }
    }
}
}
