// The fans' reactions to a staff member or the manager.
#include "media_text_b_detail.hpp"

namespace cm::aot::media::detail::press {
using namespace guest;
using namespace detail;

void Press::short_fans() {
    if (transfer(0x1066d3f0) && cmpl(subject, 0) != 0) {
        if (cmp(kind, 9) == 0 || cmp(kind, 5) == 0 || cmp(kind, 4) == 0) {
            if (cmp(tone, 1) != 0) return say(L(0x105c0, 0x1066d448));
            return say(L(0x1057e, 0x1066d434, subject_short));
        }
        if (cmp(tone, 1) == 0) return say(L(0x105ed, 0x1066d46c, 0x64, subject_short));
        if (cmp(tone, 2) != 0) return say(L(0x10677, 0x1066d4a4));
        return say(L(0x10631, 0x1066d490, 0x64, subject_short));
    }
    if (cmp(code, 1) == 0) {
        if (cmp(kind, 9) == 0 || cmp(kind, 5) == 0) return say(L(0x106a2, 0x1066d4d8, 0x64, subject_short));
        return say(L(0x106d3, 0x1066d4f4, 0x64, subject_short));
    }
    if (cmp(code, 0x3c) == 0 || cmp(code, 0x3d) == 0) {
        if (cmp(kind, 9) == 0 || cmp(kind, 5) == 0 || cmp(kind, 4) == 0) {
            if (cmp(tone, 1) == 0) return say(L(0x10702, 0x1066d538));
            if (cmp(tone, 2) != 0) return say(L(0x10739, 0x1066d568));
            return say(L(0x1071a, 0x1066d554));
        }
        if (cmp(tone, 1) == 0) return say(L(0x1075d, 0x1066d584));
        if (cmp(tone, 2) != 0) return say(L(0x10739, 0x1066d5b4));
        return say(L(0x10777, 0x1066d5a0));
    }
    if (cmp(kind, 9) == 0) {
        if (cmpl(subject, 0) == 0 || grade_band(0x1066d5d4)) {
            if (cmp(tone, 2) == 0) return say(L(0x10814, 0x1066d658));
            if (cmp(tone, 1) != 0) return say(L(0x10860, 0x1066d688));
            return say(L(0x1083e, 0x1066d674));
        }
        if (cmp(tone, 1) == 0) return say(L(0x10789, 0x1066d5fc, 0x64, subject_short));
        if (cmp(tone, 2) != 0) return say(L(0x106a2, 0x1066d63c, 0x64, subject_short));
        return say(L(0x107ce, 0x1066d620, 0x64, subject_short));
    }
    if (cmp(kind, 5) == 0) {
        if (cmpl(subject, 0) == 0 || grade_band(0x1066d6a8)) {
            if (cmp(tone, 2) == 0) return say(L(0x108c8, 0x1066d728));
            if (cmp(tone, 1) != 0) return say(L(0x10912, 0x1066d758));
            return say(L(0x108f0, 0x1066d744));
        }
        if (cmp(tone, 1) == 0) return say(L(0x1086e, 0x1066d6cc, subject_short));
        if (cmp(tone, 2) != 0) return say(L(0x106a2, 0x1066d70c, 0x64, subject_short));
        return say(L(0x10894, 0x1066d6f0, 0x64, subject_short));
    }
    if (cmp(kind, 4) == 0) {
        if (cmpl(subject, 0) == 0 || grade_band(0x1066d778)) {
            if (cmp(tone, 2) == 0) return say(L(0x10981, 0x1066d7fc));
            if (cmp(tone, 1) != 0) return say(L(0x109ce, 0x1066d82c));
            return say(L(0x109ac, 0x1066d818));
        }
        if (cmp(tone, 1) == 0) return say(L(0x10927, 0x1066d7a0, 0x64, subject_short));
        if (cmp(tone, 2) != 0) return say(L(0x106a2, 0x1066d7e0, 0x64, subject_short));
        return say(L(0x10957, 0x1066d7c4, 0x64, subject_short));
    }
    if (cmp(kind, 2) == 0) {
        if (cmpl(subject, 0) == 0 || grade_band(0x1066d84c)) {
            if (cmp(tone, 2) != 0) return say(L(0x10aba, 0x1066d8e4));
            return say(L(0x10a98, 0x1066d8d0));
        }
        if (cmp(code, 0x21) == 0) return say(L(0x109f1, 0x1066d874, 0x64, subject_short));
        if (cmp(tone, 1) != 0) return say(L(0x10a5e, 0x1066d8b4, 0x64, subject_short));
        return say(L(0x10a2c, 0x1066d898, 0x64, subject_short));
    }
    if (cmpl(subject, 0) == 0 || grade_band(0x1066d8fc)) {
        if (cmp(tone, 2) != 0) return say(L(0x10baa, 0x1066d9b8));
        return say(L(0x10b88, 0x1066d9a4));
    }
    if (cmp(tone, 1) != 0) return say(L(0x10b4f, 0x1066d988, 0x64, subject_short));
    if (cmp(code, 0x52) == 0) return say(L(0x10adf, 0x1066d92c, 0x64, subject_short));
    if (cmp(code, 0x21) != 0) return say(L(0x10b1a, 0x1066d96c, 0x64, subject_short));
    say(L(0x109f1, 0x1066d950, 0x64, subject_short));
}

void Press::long_fans() {
    if (cmp(code, 3) == 0) {
        if (cmp(kind, 9) == 0 || cmp(kind, 5) == 0 || cmp(kind, 4) == 0) {
            if (cmp(tone, 1) == 0) return say(L(0x1946b, 0x106702b8, club_marker, club_marker, club_short, subject_full));
            if (cmp(tone, 2) != 0) return say(L(0x19618, 0x10670318, club_marker, club_marker, club_short, manager_full));
            return say(L(0x1952f, 0x106702ec, club_marker, club_marker, club_short, subject_full));
        }
        if (cmp(tone, 1) == 0) return say(L(0x19752, 0x1067034c, club_marker, club_marker, club_short, subject_full));
        if (cmp(tone, 2) != 0) return say(L(0x198f0, 0x106703b0, club_marker, club_marker, club_short, manager_full, subject_full));
        return say(L(0x197fb, 0x10670380, club_marker, club_marker, club_short, subject_full));
    }
    if (cmp(code, 0x21) == 0) {
        if (cmp(kind, 9) != 0 && cmp(kind, 5) != 0 && cmp(kind, 4) != 0) return say(L(0x19b4d, 0x1067048c, club_marker, club_marker, 0x64, club_short, subject_full));
        if (cmpl(subject, 0) == 0 || (cmpl(c.u32(club + 0x68), subject) != 0 && cmpl(c.u32(club + 0x6c), subject) != 0 && cmpl(c.u32(club + 0x70), subject) != 0)) return say(L(0x19ac3, 0x1067045c, club_marker, club_marker, 0x64, club_short, subject_full));
        return say(L(0x19a40, 0x1067042c, club_marker, club_marker, 0x64, club_short, subject_full));
    }
    if (cmp(code, 0x3c) == 0 || cmp(code, 0x3d) == 0) {
        if (cmp(kind, 9) == 0 || cmp(kind, 5) == 0 || cmp(kind, 4) == 0) {
            if (cmp(tone, 1) == 0) return say(L(0x19c44, 0x106704e4, club_marker, club_marker, club_short));
            if (cmp(tone, 2) != 0) return say(L(0x19e01, 0x1067053c, club_marker, club_marker, club_short));
            return say(L(0x19d28, 0x10670514, club_marker, club_marker, club_short));
        }
        if (cmp(tone, 1) == 0) return say(L(0x19ec1, 0x10670574, club_marker, club_marker, 0x64, club_short, manager_full));
        if (cmp(tone, 2) != 0) return say(L(0x1a0a7, 0x106705d8, club_marker, club_marker, club_short, manager_full));
        return say(L(0x19fa1, 0x106705ac, club_marker, club_marker, 0x64, club_short, manager_full));
    }
    if (transfer(0x106705e8) && cmpl(subject, 0) != 0) {
        if (cmp(kind, 9) == 0 || cmp(kind, 5) == 0 || cmp(kind, 4) == 0) {
            if (cmp(tone, 1) != 0) {
                if (cmpl(subject, c.u32(club + 0x5c)) == 0 || cmpl(subject, c.u32(club + 0x60)) == 0) {
                    if (cmp(extra_b, 0) != 0) return say(L(0x1a60a, 0x106708ac, club_marker, club_marker, 0x64, club_short, subject_short));
                    if (cmp(std::int8_t(c.u8(c.u32(subject + 0x14) + 0x44)), 0xf) <= 0 || cmp(std::int8_t(c.u8(subject + 0x67)), 0xa) <= 0) return say(L(0x1a7b8, 0x10670930, club_marker, club_marker, 0x64, club_short, subject_short));
                    return say(L(0x1a6cb, 0x10670900, club_marker, club_marker, 0x64, club_short, subject_short));
                }
                if (cmpl(subject, c.u32(club + 0x64)) == 0 && cmp(extra_b, 0) != 0) return say(L(0x1a60a, 0x106708ac, club_marker, club_marker, 0x64, club_short, subject_short));
                if (cmp(std::int8_t(c.u8(c.u32(subject + 0x14) + 0x44)), 0xf) <= 0 || cmp(std::int8_t(c.u8(subject + 0x67)), 0xa) <= 0) return say(L(0x1a7b8, 0x10670930, club_marker, club_marker, 0x64, club_short, subject_short));
                return say(L(0x1a6cb, 0x10670900, club_marker, club_marker, 0x64, club_short, subject_short));
            }
            if (cmpl(c.u32(club + 0x68), subject) != 0 && cmpl(c.u32(club + 0x6c), subject) != 0 && cmpl(c.u32(club + 0x70), subject) != 0) {
                if (cmp(c.i16(club + 0x1ec), 0x109a) < 0 && cmpl(c.u32(subject + 0x14), 0) != 0 && cmp(std::int8_t(c.u8(c.u32(subject + 0x14) + 0x44)), 0xf) > 0) return say(L(0x1a56a, 0x106707d4, club_marker, club_marker, club_short, subject_short));
                if (cmp(std::int8_t(c.u8(c.u32(subject + 0x14) + 0x44)), 0xa) >= 0 || cmp(std::int8_t(c.u8(subject + 0x67)), 6) >= 0) return say(L(0x1a4d0, 0x10670850, club_marker, club_marker, club_short, subject_short));
                return say(L(0x1a4d0, 0x10670824, club_marker, club_marker, club_short, subject_short));
            }
            if (cmp(std::int8_t(c.u8(c.u32(subject + 0x14) + 0x44)), 6) < 0) return say(L(0x1a188, 0x10670678, club_marker, club_marker, club_short, subject_short));
            // One block of the original (0x106706dc) serves both ways in.
            if ((cmp(std::int8_t(c.u8(subject + 0x66)), 0xa) < 0 || cmp(std::int8_t(c.u8(subject + 0x67)), 0xa) < 0) &&
                cmp(std::int8_t(c.u8(c.u32(subject + 0x14) + 0x1c)), 0xf) > 0)
                return say(L(0x1a248, 0x106706d8, club_marker, club_marker, club_short, subject_short));
            if (cmp(c.u8(subject + 0x67), 1) == 0) return say(L(0x1a336, 0x10670710, club_marker, club_marker, club_short, subject_short));
            // The byte at +0x44 was found to be 6 or more above, so the original's leaf for it being less (phrase 0x1a430, 0x10670750) never runs; its compare stays.
            cmp(std::int8_t(c.u8(c.u32(subject + 0x14) + 0x44)), 6);
            return say(L(0x1a4d0, 0x1067077c, club_marker, club_marker, club_short, subject_short));
        }
        if (cmp(tone, 1) == 0) {
            if (cmp(code, 0x30) == 0 || cmp(code, 0x2d) == 0) return say(L(0x1a85d, 0x10670978, club_marker, club_marker, 0x64, club_short, subject_short));
            if (cmp(code, 0x2e) == 0) return say(L(0x1a948, 0x106709cc, club_marker, club_marker, 0x64, club_marker, club_marker, club_short, subject_short, club_short));
            if (cmp(code, 0x31) == 0) return say(L(0x1aa50, 0x10670a20, club_marker, club_marker, 0x64, club_marker, club_marker, club_short, subject_short, club_short));
            if (cmp(code, 0x32) == 0) return say(L(0x1ab43, 0x10670a74, club_marker, club_marker, 0x64, club_marker, club_marker, club_short, subject_short, club_short));
            if (cmp(code, 0x2a) == 0) return say(L(0x1ac59, 0x10670aac, club_marker, club_marker, 0x64, club_short, subject_short));
            if (cmp(code, 0x29) == 0 || cmp(code, 0x2c) == 0) return say(L(0x1ad47, 0x10670aec, club_marker, club_marker, 0x64, club_short, subject_short));
            return say(L(0x1ae43, 0x10670b1c, club_marker, club_marker, 0x64, club_short, subject_short));
        }
        if (cmp(tone, 2) != 0) return say(L(0x1b3f1, 0x10670cf4, club_marker, club_marker, 0x64, club_short, subject_short));
        if (cmp(code, 0x2b) == 0) {
            if (cmpl(c.u32(club + 0x68), subject) == 0 || cmpl(c.u32(club + 0x6c), subject) == 0 || cmpl(c.u32(club + 0x70), subject) == 0) return say(L(0x1b0bd, 0x10670c04, club_marker, club_marker, 0x64, club_short, subject_short));
            if (cmp(std::int8_t(c.u8(subject + 0x63)), 0xf) > 0 && cmp(std::int8_t(c.u8(subject + 0x65)), 6) < 0) return say(L(0x1b19a, 0x10670c54, club_marker, club_marker, 0x64, club_short, subject_short));
            if (cmp(std::int8_t(c.u8(subject + 0x66)), 6) >= 0) return say(L(0x1b363, 0x10670cc4, club_marker, club_marker, 0x64, club_short, subject_short));
            return say(L(0x1b283, 0x10670c94, club_marker, club_marker, 0x64, club_short, subject_short));
        }
        if (cmp(code, 0x2b) >= 0) {
            if (cmp(code, 0x35) < 0 && cmp(code, 0x2d) < 0) return say(L(0x1aede, 0x10670b80, club_marker, club_marker, 0x64, club_short, subject_short));
            if (cmpl(c.u32(club + 0x68), subject) == 0 || cmpl(c.u32(club + 0x6c), subject) == 0 || cmpl(c.u32(club + 0x70), subject) == 0) return say(L(0x1b0bd, 0x10670c04, club_marker, club_marker, 0x64, club_short, subject_short));
            if (cmp(std::int8_t(c.u8(subject + 0x63)), 0xf) > 0 && cmp(std::int8_t(c.u8(subject + 0x65)), 6) < 0) return say(L(0x1b19a, 0x10670c54, club_marker, club_marker, 0x64, club_short, subject_short));
            if (cmp(std::int8_t(c.u8(subject + 0x66)), 6) >= 0) return say(L(0x1b363, 0x10670cc4, club_marker, club_marker, 0x64, club_short, subject_short));
            return say(L(0x1b283, 0x10670c94, club_marker, club_marker, 0x64, club_short, subject_short));
        }
        if (cmp(code, 0x29) == 0) return say(L(0x1aede, 0x10670b80, club_marker, club_marker, 0x64, club_short, subject_short));
        if (cmp(code, 0x29) >= 0) return say(L(0x1afc4, 0x10670bb0, club_marker, club_marker, 0x64, club_short, subject_short));
        if (cmpl(c.u32(club + 0x68), subject) == 0 || cmpl(c.u32(club + 0x6c), subject) == 0 || cmpl(c.u32(club + 0x70), subject) == 0) return say(L(0x1b0bd, 0x10670c04, club_marker, club_marker, 0x64, club_short, subject_short));
        if (cmp(std::int8_t(c.u8(subject + 0x63)), 0xf) > 0 && cmp(std::int8_t(c.u8(subject + 0x65)), 6) < 0) return say(L(0x1b19a, 0x10670c54, club_marker, club_marker, 0x64, club_short, subject_short));
        if (cmp(std::int8_t(c.u8(subject + 0x66)), 6) >= 0) return say(L(0x1b363, 0x10670cc4, club_marker, club_marker, 0x64, club_short, subject_short));
        return say(L(0x1b283, 0x10670c94, club_marker, club_marker, 0x64, club_short, subject_short));
    }
    if (cmp(kind, 9) == 0) {
        if (cmpl(subject, 0) == 0 || grade_band(0x10670d14)) {
            if (cmp(tone, 2) == 0) return say(L(0x1bc78, 0x106710f4, club_marker, club_marker, 0x64, club_short, manager_full));
            if (cmp(tone, 1) != 0) return say(L(0x1be12, 0x10671160, club_marker, club_marker, 0x64, club_short, manager_full));
            return say(L(0x1bcfb, 0x10671130, club_marker, club_marker, 0x64, club_short, manager_full, manager_short));
        }
        if (cmp(tone, 1) != 0) {
            if (cmp(tone, 2) != 0) {
                if (ordinary(0x10671008)) return say(L(0x1bb14, 0x10671048, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
                if (cmp(code, 1) != 0) return say(L(0x1bbc1, 0x106710bc, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
                return say(L(0x1b52a, 0x10671080, club_marker, club_marker, 0x64, club_short, subject_full));
            }
            if (ordinary(0x10670f3c)) return say(L(0x1b8f7, 0x10670f84, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full, manager_short));
            if (cmp(code, 1) != 0) return say(L(0x1ba59, 0x10670ff8, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            return say(L(0x1b52a, 0x10670fbc, club_marker, club_marker, 0x64, club_short, subject_full));
        }
        if (cmp(seed % 2, 0) != 0) {
            if (ordinary(0x10670d44)) return say(L(0x1b47a, 0x10670d84, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            if (cmp(code, 1) != 0) return say(L(0x1b617, 0x10670df8, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            return say(L(0x1b52a, 0x10670dbc, club_marker, club_marker, 0x64, club_short, subject_full));
        }
        if (!ordinary(0x10670e08)) {
            if (cmp(code, 1) != 0) return say(L(0x1b835, 0x10670f24, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            return say(L(0x1b52a, 0x10670ee8, club_marker, club_marker, 0x64, club_short, subject_full));
        }
        if (cmp(std::int32_t(c.u32(club)) % 7, 0) == 0) return say(L(0x1b77f, 0x10670eb0, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
        return say(L(0x1b6cb, 0x10670e74, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
    }
    if (cmp(kind, 5) == 0) {
        if (cmpl(subject, 0) == 0 || grade_band(0x10671180)) {
            if (cmp(tone, 2) == 0) return say(L(0x1c5d6, 0x1067152c, club_marker, club_marker, 0x64, club_short, manager_full, manager_short));
            if (cmp(tone, 1) != 0) return say(L(0x1c7d6, 0x10671598, club_marker, club_marker, 0x64, club_short, manager_full));
            return say(L(0x1c6d4, 0x10671568, club_marker, club_marker, 0x64, club_short, manager_full, manager_short));
        }
        if (cmp(tone, 1) != 0) {
            if (cmp(tone, 2) != 0) {
                if (!ordinary(0x10671474)) return say(L(0x1c475, 0x106714f0, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
                return say(L(0x1c529, 0x106714b4, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            }
            if (ordinary(0x106713a8)) return say(L(0x1c315, 0x106713f0, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full, manager_short));
            if (cmp(code, 1) != 0) return say(L(0x1c475, 0x10671464, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            return say(L(0x1bf5c, 0x10671428, club_marker, club_marker, 0x64, club_short, subject_full));
        }
        if (cmp(seed % 2, 0) != 0) {
            if (ordinary(0x106711b0)) return say(L(0x1bea1, 0x106711f0, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            if (cmp(code, 1) != 0) return say(L(0x1c035, 0x10671264, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            return say(L(0x1bf5c, 0x10671228, club_marker, club_marker, 0x64, club_short, subject_full));
        }
        if (!ordinary(0x10671274)) {
            if (cmp(code, 1) != 0) return say(L(0x1c25f, 0x10671390, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            return say(L(0x1bf5c, 0x10671354, club_marker, club_marker, 0x64, club_short, subject_full));
        }
        if (cmp(std::int32_t(c.u32(club)) % 7, 0) == 0) return say(L(0x1c1a4, 0x1067131c, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
        return say(L(0x1c0eb, 0x106712e0, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
    }
    if (cmp(kind, 4) == 0) {
        if (cmpl(subject, 0) == 0 || grade_band(0x106715b8)) {
            if (cmp(tone, 2) == 0) return say(L(0x1cdc8, 0x10671964, club_marker, club_marker, 0x64, club_short, manager_full, manager_short));
            if (cmp(tone, 1) == 0) return say(L(0x1ceda, 0x1067199c, club_marker, club_marker, 0x64, club_short, manager_full));
            if (cmp(code, 0x60) == 0 || cmp(code, 0x3f) == 0) return say(L(0x1cfa8, 0x106719dc, club_marker, club_marker, 0x64, club_short, manager_full));
            return say(L(0x1d04d, 0x10671a0c, club_marker, club_marker, 0x64, club_short, manager_full));
        }
        if (cmp(tone, 1) != 0) {
            if (cmp(tone, 2) != 0) {
                if (ordinary(0x10671874)) return say(L(0x1bb14, 0x106718b4, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
                if (cmp(code, 1) != 0) return say(L(0x1bbc1, 0x10671928, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
                return say(L(0x1bf5c, 0x106718ec, club_marker, club_marker, 0x64, club_short, subject_full));
            }
            if (ordinary(0x106717a8)) return say(L(0x1cbf7, 0x106717f0, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full, manager_short));
            if (cmp(code, 1) != 0) return say(L(0x1cd19, 0x10671864, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            return say(L(0x1bf5c, 0x10671828, club_marker, club_marker, 0x64, club_short, subject_full));
        }
        if (cmp(seed % 2, 0) != 0) {
            if (!ordinary(0x106715e8)) return say(L(0x1c91f, 0x10671664, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            return say(L(0x1c863, 0x10671628, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
        }
        if (!ordinary(0x10671674)) {
            if (cmp(code, 1) != 0) return say(L(0x1cb4e, 0x10671790, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            return say(L(0x1bf5c, 0x10671754, club_marker, club_marker, 0x64, club_short, subject_full));
        }
        if (cmp(std::int32_t(c.u32(club)) % 7, 0) == 0) return say(L(0x1ca88, 0x1067171c, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
        return say(L(0x1c9ce, 0x106716e0, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
    }
    if (cmp(kind, 2) == 0) {
        if (cmpl(subject, 0) == 0 || grade_band(0x10671a2c)) {
            if (cmp(tone, 2) == 0) return say(L(0x1d890, 0x10671cd0, club_marker, club_marker, 0x64, club_short, manager_full));
            if (cmp(tone, 1) != 0) return say(L(0x1db28, 0x10671d78, club_marker, club_marker, 0x64, club_short, manager_full));
            if (cmp(code, 0x60) == 0 || cmp(code, 0x3f) == 0) return say(L(0x1d96f, 0x10671d18, club_marker, club_marker, 0x64, club_short, manager_full));
            return say(L(0x1da55, 0x10671d48, club_marker, club_marker, 0x64, club_short, manager_full));
        }
        if (cmp(tone, 1) == 0) {
            if (ordinary(0x10671a48)) return say(L(0x1d0f1, 0x10671a88, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            if (cmp(code, 0x52) == 0) return say(L(0x1d19b, 0x10671acc, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            if (cmp(code, 1) != 0) return say(L(0x1d396, 0x10671b40, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            return say(L(0x1d264, 0x10671b04, club_marker, club_marker, 0x64, club_short, subject_full));
        }
        if (cmp(tone, 2) != 0) {
            if (!ordinary(0x10671c1c)) return say(L(0x1d7d1, 0x10671c98, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
            return say(L(0x1d70d, 0x10671c5c, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
        }
        if (ordinary(0x10671b58)) return say(L(0x1d45a, 0x10671b98, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
        if (cmp(code, 1) != 0) return say(L(0x1d654, 0x10671c0c, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
        return say(L(0x1d526, 0x10671bd0, club_marker, club_marker, 0x64, club_short, subject_full));
    }
    if (cmpl(subject, 0) == 0 || grade_band(0x10671d90)) {
        if (cmp(tone, 2) == 0) return say(L(0x1e086, 0x10672094, club_marker, club_marker, 0x64, club_short, manager_full));
        if (cmp(tone, 1) != 0) return say(L(0x1e317, 0x1067213c, club_marker, club_marker, 0x64, club_short, manager_full));
        if (cmp(code, 0x60) == 0 || cmp(code, 0x3f) == 0) return say(L(0x1e150, 0x106720dc, club_marker, club_marker, 0x64, club_short, manager_full));
        return say(L(0x1e233, 0x1067210c, club_marker, club_marker, 0x64, club_short, manager_full));
    }
    if (cmp(tone, 1) == 0) {
        if (ordinary(0x10671dac)) return say(L(0x1dbc5, 0x10671dec, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
        if (cmp(code, 1) == 0) return say(L(0x1dc6d, 0x10671e24, club_marker, club_marker, 0x64, club_short, subject_full));
        if (cmp(code, 0x52) != 0) return say(L(0x1d396, 0x10671ea4, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
        return say(L(0x1d19b, 0x10671e68, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
    }
    if (cmp(tone, 2) != 0) {
        if (ordinary(0x10671fa8)) return say(L(0x1d70d, 0x10671fe8, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
        if (cmp(code, 1) != 0) return say(L(0x1d7d1, 0x1067205c, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
        return say(L(0x1de72, 0x10672020, club_marker, club_marker, 0x64, club_short, subject_full));
    }
    if (ordinary(0x10671ebc)) return say(L(0x1dda8, 0x10671efc, club_marker, club_marker, 0x64, 0x64, club_short, manager_full, subject_full));
    if (cmp(code, 1) != 0) return say(L(0x1dfae, 0x10671f98, club_marker, club_marker, 0x64, 0x64, poll_percent, club_short, manager_full, subject_full));
    say(L(0x1de72, 0x10671f34, club_marker, club_marker, 0x64, club_short, subject_full));
}
}
