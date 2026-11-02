// The board's reactions: the chairman and the directors on a staff member or on the manager.
#include "media_text_b_detail.hpp"

namespace cm::aot::media::detail::press {
using namespace guest;
using namespace detail;

void Press::short_board() {
    if (cmp(code, 1) == 0) {
        if (cmp(kind, 9) == 0 || cmp(kind, 5) == 0) return say(L(0xfa05, 0x1066cc00, 0x64, subject_short));
        return say(L(0xfa46, 0x1066cc1c, 0x64, subject_short));
    }
    if (transfer(0x1066cc2c) && cmpl(subject, 0) != 0 && !grade_band(0x1066cc48)) {
        if (cmp(kind, 9) == 0 || cmp(kind, 5) == 0 || cmp(kind, 4) == 0) {
            return by_chair(L(0xfacc, 0x1066cc98), L(0xfa84, 0x1066cc84, chairman_short));
        }
        if (cmp(kind, 2) != 0 && cmp(kind, 3) != 0) {
            return;
        }
        if (cmp(tone, 1) == 0) {
            return by_chair(L(0xfb40, 0x1066ccec, 0x64, subject_short), L(0xfafa, 0x1066ccd0, chairman_short));
        }
        if (cmp(tone, 2) != 0) return say(L(0xfbc2, 0x1066cd40));
        return by_chair(L(0xfb84, 0x1066cd2c, subject_short), L(0xfafa, 0x1066cd14, chairman_short));
    }
    if (cmp(kind, 9) == 0) {
        if (cmpl(subject, 0) == 0 || grade_band(0x1066cd60)) {
            if (cmp(tone, 2) == 0 || cmp(tone, 1) == 0) {
                return by_chair(L(0xfd34, 0x1066ce4c), L(0xfcdd, 0x1066ce38, 0x64, chairman_short, manager_short));
            }
            return by_chair(L(0xfdb0, 0x1066ce90, 0x64, manager_short), L(0xfd59, 0x1066ce74, 0x64, chairman_short, manager_short));
        }
        if (cmp(tone, 1) == 0) {
            return by_chair(L(0xfc25, 0x1066cda4, subject_short), L(0xfbf7, 0x1066cd8c, chairman_short));
        }
        if (cmp(tone, 2) != 0) return say(L(0xfc9f, 0x1066ce00, subject_short));
        return by_chair(L(0xfc9f, 0x1066cde8, subject_short), L(0xfc4c, 0x1066cdd0, chairman_short, subject_short));
    }
    if (cmp(kind, 5) == 0) {
        if (cmpl(subject, 0) == 0 || grade_band(0x1066ceb0)) {
            if (cmp(tone, 2) == 0 || cmp(tone, 1) == 0) {
                return by_chair(L(0xff49, 0x1066cfa8), L(0xfef8, 0x1066cf94, 0x64, chairman_short, manager_short));
            }
            return by_chair(L(0xffb8, 0x1066cfec, 0x64, manager_short), L(0xff67, 0x1066cfd0, 0x64, chairman_short, manager_short));
        }
        if (cmp(tone, 1) == 0) {
            return by_chair(L(0xfe1b, 0x1066cef4, subject_short), L(0xfded, 0x1066cedc, chairman_short));
        }
        if (cmp(tone, 2) != 0) return say(L(0xfec7, 0x1066cf5c, 0x64, subject_short));
        return by_chair(L(0xfe92, 0x1066cf40, 0x64, subject_short), L(0xfe42, 0x1066cf24, 0x64, chairman_short, subject_short));
    }
    if (cmp(kind, 4) == 0) {
        if (cmpl(subject, 0) == 0 || grade_band(0x1066d00c)) {
            if (cmp(tone, 2) == 0) {
                return by_chair(L(0x10131, 0x1066d0f8), L(0x100d7, 0x1066d0e4, 0x64, chairman_short, manager_short));
            }
            if (cmp(tone, 1) != 0) return say(L(0x101ae, 0x1066d150));
            return by_chair(L(0x10131, 0x1066d13c), L(0x10157, 0x1066d128, 0x64, chairman_short, manager_short));
        }
        if (cmp(tone, 1) == 0) {
            return by_chair(L(0x1003b, 0x1066d054, subject_short), L(0xffef, 0x1066d03c, chairman_short, manager_short));
        }
        if (cmp(tone, 2) != 0) return say(L(0xfec7, 0x1066d0b4, 0x64, subject_short));
        return by_chair(L(0x100ae, 0x1066d098, subject_short), L(0x1006a, 0x1066d080, chairman_short, subject_short));
    }
    if (cmp(kind, 2) != 0) {
        if (cmpl(subject, 0) != 0 && !grade_band(0x1066d2a8)) {
            if (cmp(tone, 1) != 0) {
                return by_chair(L(0x10428, 0x1066d33c, 0x64, subject_short), L(0x103d5, 0x1066d320, 0x64, 0x64, subject_short, chairman_short));
            }
            return by_chair(L(0x1039f, 0x1066d2f4, 0x64, subject_short), L(0x101d4, 0x1066d2d8, chairman_short, subject_short));
        }
        if (cmp(tone, 2) == 0) {
            return by_chair(L(0x1049d, 0x1066d380, 0x64, manager_short), L(0x10465, 0x1066d364, chairman_short));
        }
        if (cmp(tone, 1) != 0) return say(L(0x10546, 0x1066d3e0, 0x64, manager_short));
        return by_chair(L(0x1050d, 0x1066d3c4, 0x64, manager_short), L(0x104d2, 0x1066d3a8, chairman_short));
    }
    if (cmpl(subject, 0) != 0 && !grade_band(0x1066d170)) {
        if (cmp(tone, 1) != 0) {
            return by_chair(L(0x10260, 0x1066d1f4, subject_short), L(0x101d4, 0x1066d1dc, chairman_short, subject_short));
        }
        return by_chair(L(0x10227, 0x1066d1b8, subject_short), L(0x101d4, 0x1066d1a0, chairman_short, subject_short));
    }
    if (cmp(tone, 2) == 0) {
        return by_chair(L(0x102e8, 0x1066d238), L(0x10293, 0x1066d224, 0x64, chairman_short, manager_short));
    }
    if (cmp(tone, 1) != 0) return say(L(0x10365, 0x1066d290, 0x64, manager_short));
    return by_chair(L(0x10345, 0x1066d274), L(0x1030b, 0x1066d260, chairman_short));
}

void Press::long_board() {
    if (cmp(code, 0x21) == 0) {
        if (cmp(kind, 9) == 0 || cmp(kind, 5) == 0 || cmp(kind, 4) == 0) {
            if (cmp(tone, 2) != 0) {
                return by_chair(L(0x10e56, 0x1066da84, 0x64, subject_short), L(0x10db5, 0x1066da68, 0x64, chairman_full, subject_full));
            }
            return by_chair(L(0x10cbb, 0x1066da40, 0x64, subject_short), L(0x10bc8, 0x1066da24, 0x64, chairman_full, subject_full, chairman_short));
        }
        if (cmp(chair, 0) == 0) {
            if (cmp(tone, 2) != 0) return say(L(0x111be, 0x1066db1c, 0x64, subject_full));
            return say(L(0x110d7, 0x1066db00, 0x64, subject_full));
        }
        if (cmp(tone, 2) != 0) return say(L(0x10ffe, 0x1066dadc, 0x64, chairman_full, subject_full, chairman_short));
        return say(L(0x10ece, 0x1066dab8, 0x64, chairman_full, subject_full, chairman_short));
    }
    if (cmp(code, 1) == 0) {
        if (cmp(kind, 9) != 0 && cmp(kind, 5) != 0 && cmp(kind, 4) != 0) {
            return by_chair(L(0x115a3, 0x1066dc18, 0x64, subject_short), L(0x1149d, 0x1066dbfc, 0x64, chairman_full, subject_full, chairman_short));
        }
        if (cmp(tone, 2) != 0) {
            return by_chair(L(0x113a2, 0x1066dbd0, 0x64, subject_short), L(0x11407, 0x1066dbb4, 0x64, chairman_full, subject_full));
        }
        return by_chair(L(0x113a2, 0x1066db8c, 0x64, subject_short), L(0x1124e, 0x1066db70, 0x64, chairman_full, subject_full, chairman_short));
    }
    if (transfer(0x1066dc28)) {
        if (cmp(kind, 9) == 0 || cmp(kind, 5) == 0 || cmp(kind, 4) == 0) {
            if (cmp(tone, 1) == 0) {
                return by_chair(L(0x1171a, 0x1066dc90, 0x64, subject_short), L(0x11652, 0x1066dc74, 0x64, chairman_full, subject_full));
            }
            if (cmp(tone, 2) != 0) return say(L(0x11928, 0x1066dcf4));
            return by_chair(L(0x1185a, 0x1066dce0, 0x64, subject_short, subject_short), L(0x11781, 0x1066dcc0, 0x64, chairman_full, subject_full));
        }
        if (cmp(kind, 2) != 0 && cmp(kind, 3) != 0) {
            return;
        }
        if (cmp(tone, 1) != 0) {
            if (cmp(tone, 2) != 0) return say(L(0x11dd3, 0x1066de2c));
            return by_chair(
                L(0x11ce4, 0x1066de18, 0x64, pool_text(0x11dc6), subject_short),
                L(0x11ce4, 0x1066ddf4, 0x64, chairman_short, subject_short));
        }
        if (cmp(chair, 0) == 0) return say(L(0x11c4e, 0x1066ddc4, 0x64, subject_short));
        if (cmp(note_a(), 0) != 0 && cmp(extra_a, 0) != 0) return say(L(0x11997, 0x1066dd48, 0x64, chairman_full, subject_full));
        if (cmp(note_b(), 0) == 0 || cmp(note_c(), 0) == 0 || cmp(extra_a, 0) == 0) return say(L(0x11b84, 0x1066dda8, 0x64, chairman_full, subject_full));
        return say(L(0x11a8c, 0x1066dd88, 0x64, chairman_full, subject_full));
    }
    if (cmp(kind, 9) == 0) {
        if (cmpl(subject, 0) == 0 || grade_band(0x1066de4c)) {
            if (cmp(tone, 2) == 0) {
                return by_owner_and_chair(
                    L(0x135eb, 0x1066e554, club_marker, club_marker, 0x64, club_short, manager_full),
                    L(0x13530, 0x1066e524, club_marker, club_marker, 0x64, club_short, chairman_full, manager_full),
                    L(0x134d3, 0x1066e4e8),
                    L(0x133eb, 0x1066e4d4, chairman_full));
            }
            if (cmp(tone, 1) != 0) {
                return by_owner_and_chair(
                    L(0x13ccc, 0x1066e6ac, club_marker, club_marker, 0x64, club_short, manager_full),
                    L(0x13c16, 0x1066e67c, club_marker, club_marker, 0x64, club_short, chairman_full, manager_full),
                    L(0x13b4a, 0x1066e640),
                    L(0x13a70, 0x1066e62c, chairman_full));
            }
            return by_owner_and_chair(
                L(0x13992, 0x1066e604, club_marker, club_marker, 0x64, club_short, manager_full),
                L(0x13898, 0x1066e5d4, club_marker, club_marker, 0x64, club_short, chairman_full, manager_full),
                L(0x137a1, 0x1066e598),
                L(0x1368d, 0x1066e584, chairman_full));
        }
        if (cmp(tone, 1) == 0) {
            if (cmp(own_club, 0) == 0) {
                return by_ordinary_and_chair(
                    0x1066df80,
                    L(0x12622, 0x1066e094, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full),
                    L(0x12575, 0x1066e058, club_marker, club_marker, 0x64, 0x64, club_short, chairman_full, manager_full, subject_full),
                    L(0x124ce, 0x1066e00c, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full),
                    L(0x1241c, 0x1066dfd0, club_marker, club_marker, 0x64, 0x64, club_short, chairman_full, manager_full, subject_full));
            }
            if (ordinary(0x1066de70)) {
                if (cmp(chair, 0) == 0) return say(L(0x1202c, 0x1066dee0, 0x64, subject_full));
                if (cmp(code, 9) != 0) return say(L(0x11f5c, 0x1066dec4, 0x64, chairman_full, subject_full));
                return say(L(0x11e34, 0x1066dea4, 0x64, chairman_full, subject_full));
            }
            if (cmp(chair, 0) == 0) {
                if (cmp(code, 8) != 0) return say(L(0x123a5, 0x1066df70, 0x64, subject_full));
                return say(L(0x122a0, 0x1066df54, 0x64, subject_full));
            }
            if (cmp(code, 8) != 0) return say(L(0x12212, 0x1066df30, 0x64, chairman_full, subject_full));
            return say(L(0x120f6, 0x1066df10, 0x64, chairman_full, subject_full));
        }
        if (cmp(tone, 2) != 0) {
            if (cmp(own_club, 0) == 0) {
                if (!ordinary(0x1066e428)) return say(L(0x1331b, 0x1066e4a4, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
                return say(L(0x13236, 0x1066e468, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            }
            if (!ordinary(0x1066e3dc)) return say(L(0x12c10, 0x1066e418, 0x64, subject_full));
            return say(L(0x12ada, 0x1066e3fc, 0x64, subject_full));
        }
        if (cmp(own_club, 0) == 0) {
            if (ordinary(0x1066e210)) {
                return by_chair(
                    L(0x12d96, 0x1066e29c, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full),
                    L(0x12c9e, 0x1066e260, club_marker, club_marker, 0x64, 0x64, club_short, chairman_full, manager_full, subject_full));
            }
            if (cmp(chair, 0) == 0) {
                if (cmp(code, 3) != 0) return say(L(0x13168, 0x1066e3c4, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
                return say(L(0x13097, 0x1066e388, club_marker, club_marker, 0x64, club_short, manager_full, subject_full));
            }
            if (cmp(code, 3) != 0) return say(L(0x12fb2, 0x1066e34c, club_marker, club_marker, 0x64, 0x64, club_short, chairman_full, manager_full, subject_full));
            return say(L(0x12e6e, 0x1066e308, club_marker, club_marker, 0x64, club_marker, club_marker, club_short, chairman_full, manager_full, subject_full, club_short));
        }
        if (cmp(code, 7) == 0) {
            return by_chair(L(0x12795, 0x1066e0f0, 0x64, subject_full), L(0x126b6, 0x1066e0d4, 0x64, chairman_full, subject_full));
        }
        if (cmp(code, 6) == 0) {
            if (cmp(chair, 0) == 0) return say(L(0x1296e, 0x1066e164, 0x64, subject_full));
            if (cmp(last, 0) == 0) return say(L(0x128e7, 0x1066e148, 0x64, chairman_full, subject_full));
            return say(L(0x12810, 0x1066e128, 0x64, chairman_full, subject_full));
        }
        return by_ordinary_and_chair(
            0x1066e174,
            L(0x12c10, 0x1066e200, 0x64, subject_full),
            L(0x12b72, 0x1066e1e4, 0x64, chairman_full, subject_full),
            L(0x12ada, 0x1066e1bc, 0x64, subject_full),
            L(0x129de, 0x1066e1a0, 0x64, chairman_full, subject_full));
    }
    if (cmp(kind, 5) == 0) {
        if (cmpl(subject, 0) == 0 || grade_band(0x1066e6cc)) {
            if (cmp(tone, 2) == 0) {
                return by_owner_and_chair(
                    L(0x14df5, 0x1066ecc0, club_marker, club_marker, 0x64, club_short, manager_full),
                    L(0x14d36, 0x1066ec90, club_marker, club_marker, 0x64, club_short, chairman_full, manager_full),
                    L(0x14cd8, 0x1066ec54),
                    L(0x14bf3, 0x1066ec40, chairman_full));
            }
            if (cmp(tone, 1) != 0) {
                return by_owner_and_chair(
                    L(0x150f4, 0x1066ee18, club_marker, club_marker, 0x64, club_short, manager_full),
                    L(0x1503d, 0x1066ede8, club_marker, club_marker, 0x64, club_short, chairman_full, manager_full),
                    L(0x14f70, 0x1066edac),
                    L(0x14e98, 0x1066ed98, chairman_full));
            }
            return by_owner_and_chair(
                L(0x13992, 0x1066ed70, club_marker, club_marker, 0x64, club_short, manager_full),
                L(0x13898, 0x1066ed40, club_marker, club_marker, 0x64, club_short, chairman_full, manager_full),
                L(0x137a1, 0x1066ed04),
                L(0x1368d, 0x1066ecf0, chairman_full));
        }
        if (cmp(tone, 1) == 0) {
            if (cmp(own_club, 0) == 0) {
                return by_ordinary_and_chair(
                    0x1066e78c,
                    L(0x1420f, 0x1066e8a0, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full),
                    L(0x14136, 0x1066e864, club_marker, club_marker, 0x64, 0x64, club_short, chairman_full, manager_full, subject_full),
                    L(0x1406f, 0x1066e818, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full),
                    L(0x13f91, 0x1066e7dc, club_marker, club_marker, 0x64, 0x64, club_short, chairman_full, manager_full, subject_full));
            }
            return by_ordinary_and_chair(
                0x1066e6f0,
                L(0x13f18, 0x1066e77c, 0x64, subject_full),
                L(0x13e86, 0x1066e760, 0x64, chairman_full, subject_full),
                L(0x13e08, 0x1066e738, 0x64, subject_full),
                L(0x13d71, 0x1066e71c, 0x64, chairman_full, subject_full));
        }
        if (cmp(tone, 2) != 0) {
            if (cmp(own_club, 0) == 0) {
                if (!ordinary(0x1066eb94)) return say(L(0x14b21, 0x1066ec10, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
                return say(L(0x14a3a, 0x1066ebd4, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            }
            if (!ordinary(0x1066eb48)) return say(L(0x14604, 0x1066eb84, 0x64, subject_full));
            return say(L(0x144cf, 0x1066eb68, 0x64, subject_full));
        }
        if (cmp(own_club, 0) == 0) {
            return by_ordinary_and_chair(
                0x1066ea1c,
                L(0x14961, 0x1066eb30, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full),
                L(0x14871, 0x1066eaf4, club_marker, club_marker, 0x64, 0x64, club_short, chairman_full, manager_full, subject_full),
                L(0x1478e, 0x1066eaa8, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full),
                L(0x14694, 0x1066ea6c, club_marker, club_marker, 0x64, 0x64, club_short, chairman_full, manager_full, subject_full));
        }
        if (cmp(code, 7) == 0) {
            return by_chair(L(0x12795, 0x1066e8fc, 0x64, subject_full), L(0x126b6, 0x1066e8e0, 0x64, chairman_full, subject_full));
        }
        if (cmp(code, 6) == 0) {
            if (cmp(chair, 0) == 0) return say(L(0x143ac, 0x1066e970, 0x64, subject_full));
            if (cmp(last, 0) == 0) return say(L(0x128e7, 0x1066e954, 0x64, chairman_full, subject_full));
            return say(L(0x142d1, 0x1066e934, 0x64, chairman_full, subject_full));
        }
        return by_ordinary_and_chair(
            0x1066e980,
            L(0x14604, 0x1066ea0c, 0x64, subject_full),
            L(0x14569, 0x1066e9f0, 0x64, chairman_full, subject_full),
            L(0x144cf, 0x1066e9c8, 0x64, subject_full),
            L(0x1441c, 0x1066e9ac, 0x64, chairman_full, subject_full));
    }
    if (cmp(kind, 4) == 0) {
        if (cmpl(subject, 0) == 0 || grade_band(0x1066ee38)) {
            if (cmp(tone, 2) == 0) {
                return by_owner_and_chair(
                    L(0x162e1, 0x1066f39c, club_marker, club_marker, 0x64, club_short, manager_full),
                    L(0x1619e, 0x1066f36c, club_marker, club_marker, 0x64, club_short, chairman_full, manager_full, chairman_short),
                    L(0x160d9, 0x1066f328),
                    L(0x16004, 0x1066f314, chairman_full));
            }
            if (cmp(tone, 1) != 0) {
                if (cmp(own_club, 0) == 0) return say(L(0x167ce, 0x1066f498, club_marker, club_marker, 0x64, club_short, manager_full));
                return say(L(0x16769, 0x1066f468));
            }
            return by_owner_and_chair(
                L(0x16682, 0x1066f44c, club_marker, club_marker, 0x64, club_short, manager_full),
                L(0x16586, 0x1066f41c, club_marker, club_marker, 0x64, club_short, chairman_full, manager_full),
                L(0x16494, 0x1066f3e0),
                L(0x16383, 0x1066f3cc, chairman_full));
        }
        if (cmp(tone, 1) == 0) {
            if (cmp(own_club, 0) == 0) {
                return by_ordinary_and_chair(
                    0x1066eef8,
                    L(0x15633, 0x1066f00c, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full),
                    L(0x1555b, 0x1066efd0, club_marker, club_marker, 0x64, 0x64, club_short, chairman_full, manager_full, subject_full),
                    L(0x15493, 0x1066ef84, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full),
                    L(0x153b6, 0x1066ef48, club_marker, club_marker, 0x64, 0x64, club_short, chairman_full, manager_full, subject_full));
            }
            return by_ordinary_and_chair(
                0x1066ee5c,
                L(0x1533c, 0x1066eee8, 0x64, subject_full),
                L(0x152ad, 0x1066eecc, 0x64, chairman_full, subject_full),
                L(0x1522e, 0x1066eea4, 0x64, subject_full),
                L(0x1519a, 0x1066ee88, 0x64, chairman_full, subject_full));
        }
        if (cmp(tone, 2) != 0) {
            if (cmp(own_club, 0) == 0) {
                if (!ordinary(0x1066f268)) return say(L(0x15f2d, 0x1066f2e4, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
                return say(L(0x15e52, 0x1066f2a8, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            }
            if (!ordinary(0x1066f1f4)) return say(L(0x15dcd, 0x1066f258, 0x64, subject_full));
            return by_chair(L(0x15d3e, 0x1066f23c, 0x64, subject_full), L(0x15c9a, 0x1066f220, 0x64, chairman_full, subject_full));
        }
        if (cmp(own_club, 0) == 0) {
            return by_ordinary_and_chair(
                0x1066f0c8,
                L(0x15bd0, 0x1066f1dc, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full),
                L(0x15af1, 0x1066f1a0, club_marker, club_marker, 0x64, 0x64, club_short, chairman_full, manager_full, subject_full),
                L(0x15a1d, 0x1066f154, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full),
                L(0x15930, 0x1066f118, club_marker, club_marker, 0x64, 0x64, club_short, chairman_full, manager_full, subject_full));
        }
        return by_ordinary_and_chair(
            0x1066f02c,
            L(0x158af, 0x1066f0b8, 0x64, subject_full),
            L(0x1581d, 0x1066f09c, 0x64, chairman_full, subject_full),
            L(0x15796, 0x1066f074, 0x64, subject_full),
            L(0x156f6, 0x1066f058, 0x64, chairman_full, subject_full));
    }
    if (cmp(kind, 2) != 0) {
        if (cmpl(subject, 0) == 0 || grade_band(0x1066fbe8)) {
            if (cmp(tone, 2) == 0) {
                if (cmp(own_club, 0) == 0) {
                    return by_chair(
                        L(0x18f66, 0x10670124, club_marker, club_marker, 0x64, club_short, manager_full),
                        L(0x18eb4, 0x106700f4, club_marker, club_marker, 0x64, club_short, chairman_full, manager_full));
                }
                if (cmp(code, 0x60) == 0 || cmp(code, 0x3f) == 0) {
                    return by_chair(L(0x18d2a, 0x10670084), L(0x18c8c, 0x10670070, chairman_full));
                }
                return by_chair(L(0x18e3b, 0x106700b8), L(0x18da6, 0x106700a4, chairman_full));
            }
            if (cmp(tone, 1) != 0) {
                if (cmp(own_club, 0) != 0) return say(L(0x19264, 0x106701f8));
                return by_chair(
                    L(0x193af, 0x10670264, club_marker, club_marker, 0x64, club_short, manager_full),
                    L(0x192d8, 0x10670234, club_marker, club_marker, 0x64, club_short, chairman_full, manager_full));
            }
            if (cmp(own_club, 0) == 0) return say(L(0x191c7, 0x106701dc, club_marker, club_marker, 0x64, club_short, manager_full));
            if (cmp(code, 0x60) == 0 || cmp(code, 0x3f) == 0) {
                return by_chair(L(0x1908a, 0x10670178), L(0x19001, 0x10670164, chairman_full));
            }
            return by_chair(L(0x1916a, 0x106701ac), L(0x190ea, 0x10670198, chairman_full));
        }
        if (cmp(tone, 1) == 0) {
            if (cmp(own_club, 0) == 0) {
                if (!ordinary(0x1066fcb0)) return say(L(0x186c6, 0x1066fdcc, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
                if (cmp(chair, 0) == 0) return say(L(0x1860c, 0x1066fd90, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
                if (cmp(count, 0x1d4c) <= 0) return say(L(0x18538, 0x1066fd54, club_marker, club_marker, 0x64, 0x64, club_short, chairman_full, manager_full, subject_full));
                return say(L(0x183d6, 0x1066fd10, club_marker, club_marker, 0x64, 0x64, club_short, chairman_full, manager_full, subject_full, chairman_short));
            }
            if (!ordinary(0x1066fc0c)) return say(L(0x1836a, 0x1066fca0, 0x64, subject_full));
            if (cmp(chair, 0) != 0) return say(L(0x18105, 0x1066fc38, 0x64, chairman_full, subject_full));
            if (cmp(seed % 2, 0) == 0) return say(L(0x182c6, 0x1066fc84, 0x64, subject_full));
            return say(L(0x18203, 0x1066fc68, 0x64, subject_full));
        }
        if (cmp(tone, 2) != 0) {
            if (cmp(own_club, 0) == 0) {
                if (!ordinary(0x1066ffb4)) return say(L(0x1766b, 0x10670030, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
                return say(L(0x17430, 0x1066fff4, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            }
            if (!ordinary(0x1066ff68)) return say(L(0x18bcd, 0x1066ffa4, 0x64, subject_full));
            return say(L(0x18b3c, 0x1066ff88, 0x64, subject_full));
        }
        if (cmp(own_club, 0) == 0) {
            if (ordinary(0x1066fe88)) return say(L(0x17196, 0x1066fec8, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            return by_chair(
                L(0x17269, 0x1066ff50, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full),
                L(0x18a6b, 0x1066ff14, club_marker, club_marker, 0x64, 0x64, club_short, chairman_full, manager_full, subject_full));
        }
        return by_ordinary_and_chair(
            0x1066fdec,
            L(0x189ec, 0x1066fe78, 0x64, subject_full),
            L(0x188ca, 0x1066fe5c, 0x64, chairman_full, subject_full),
            L(0x18835, 0x1066fe34, 0x64, subject_full),
            L(0x1877b, 0x1066fe18, 0x64, chairman_full, subject_full));
    }
    if (cmpl(subject, 0) == 0 || grade_band(0x1066f4b8)) {
        if (cmp(tone, 2) == 0) {
            if (cmp(own_club, 0) == 0) {
                if (cmp(chair, 0) == 0) return say(L(0x17ba2, 0x1066fa70, club_marker, club_marker, 0x64, club_short, manager_full));
                if (cmp(code, 0x3f) == 0 || cmp(code, 0x60) == 0) return say(L(0x1793d, 0x1066fa04, club_marker, club_marker, 0x64, club_short, chairman_full, manager_full, chairman_short));
                return say(L(0x17a71, 0x1066fa40, club_marker, club_marker, 0x64, club_short, chairman_full, manager_full, chairman_short));
            }
            if (cmp(chair, 0) == 0) return say(L(0x178e6, 0x1066f9b0));
            if (cmp(code, 0x3f) == 0 || cmp(code, 0x60) == 0) return say(L(0x17737, 0x1066f984, chairman_full));
            return say(L(0x17810, 0x1066f99c, chairman_full));
        }
        if (cmp(tone, 1) != 0) {
            if (cmp(own_club, 0) == 0) return say(L(0x18056, 0x1066fbd0, club_marker, club_marker, 0x64, club_short, manager_full));
            return say(L(0x17ff1, 0x1066fba0));
        }
        if (cmp(own_club, 0) != 0) {
            return by_chair(L(0x17cae, 0x1066fab4), L(0x17c42, 0x1066faa0, chairman_full));
        }
        if (cmp(chair, 0) == 0) return say(L(0x17f5f, 0x1066fb84, club_marker, club_marker, 0x64, club_short, manager_full));
        if (cmp(code, 0x3f) == 0 || cmp(code, 0x60) == 0) return say(L(0x17d05, 0x1066fb08, club_marker, club_marker, 0x64, club_short, chairman_full, manager_full, chairman_short));
        return say(L(0x17e29, 0x1066fb54, club_marker, club_marker, 0x64, 0x64, club_short, chairman_full, manager_full, chairman_short, manager_short));
    }
    if (cmp(tone, 1) == 0) {
        if (cmp(own_club, 0) == 0) {
            return by_ordinary_and_chair(
                0x1066f578,
                L(0x16cc9, 0x1066f68c, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full),
                L(0x16bf9, 0x1066f650, club_marker, club_marker, 0x64, 0x64, club_short, chairman_full, manager_full, subject_full),
                L(0x16b3d, 0x1066f604, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full),
                L(0x16a68, 0x1066f5c8, club_marker, club_marker, 0x64, 0x64, club_short, chairman_full, manager_full, subject_full));
        }
        return by_ordinary_and_chair(
            0x1066f4dc,
            L(0x169fa, 0x1066f568, 0x64, subject_full),
            L(0x16977, 0x1066f54c, 0x64, chairman_full, subject_full),
            L(0x16904, 0x1066f524, 0x64, subject_full),
            L(0x1687c, 0x1066f508, 0x64, chairman_full, subject_full));
    }
    if (cmp(tone, 2) != 0) {
        if (cmp(own_club, 0) != 0) {
            if (!ordinary(0x1066f828)) return say(L(0x173aa, 0x1066f864, 0x64, subject_full));
            return say(L(0x1731f, 0x1066f848, 0x64, subject_full));
        }
        if (ordinary(0x1066f874)) return say(L(0x17430, 0x1066f8b4, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
        return by_chair(
            L(0x1766b, 0x1066f944, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full),
            L(0x17506, 0x1066f908, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full, chairman_full, chairman_short));
    }
    if (cmp(own_club, 0) == 0) {
        if (!ordinary(0x1066f748)) return say(L(0x17269, 0x1066f810, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
        return by_chair(
            L(0x17196, 0x1066f7d4, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full),
            L(0x170af, 0x1066f798, club_marker, club_marker, 0x64, 0x64, club_short, chairman_full, manager_full, subject_full));
    }
    return by_ordinary_and_chair(
        0x1066f6ac,
        L(0x16fda, 0x1066f738, 0x64, subject_full),
        L(0x16eeb, 0x1066f71c, 0x64, chairman_full, subject_full),
        L(0x16e47, 0x1066f6f4, 0x64, subject_full),
        L(0x16d80, 0x1066f6d8, 0x64, chairman_full, subject_full));
}
}
