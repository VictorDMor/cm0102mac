// TRANSFER_MANAGER.CPP, block "text_b": news message 4001's text (0x104001e0): a signing, with its fee, reasons and clubs.
#include "transfer_manager_native_text_b.hpp"
#include "transfer_manager_native_text_b_4001.hpp"

namespace cm::aot::transfer_manager_native {
using namespace text_b;
using guest::Cpu;
using guest::Frame;

namespace text_b {
// The message's sentence: which format the kind, the reason, the amount and the clubs pick, and what it is given
// (each club's name length twice and its name, the cash text, the date text, and the reasons text on the stack).
void write_signing(Signing& s) {
    const auto say = [&](std::uint32_t back, std::uint32_t format, std::initializer_list<std::uint32_t> values) {
        s.say(back, format, values);
    };
    const auto len_from = [&] { return s.len(s.from); };
    const auto len_to = [&] { return s.len(s.to); };
    const auto name_from = [&] { return s.name(s.from); };
    const auto name_to = [&] { return s.name(s.to); };
    const auto date = s.date(), cash = s.cash(), reasons = s.reasons();
    if (s.kind == 2) {
        build_reasons(s);
        if (s.flagged) {
            if (s.reason == 0) {
                if (s.amount == 0) {
                    if (s.extra == 0) say(0x10400d7c, 0x5c01, {len_from(), len_from(), len_to(), len_to(), name_from(), name_to(), date});
                    else say(0x10400d2c, 0x5b6a, {len_from(), len_from(), len_to(), len_to(), name_from(), name_to(), date});
                } else if (s.extra == 0) {
                    say(0x10400cd4, 0x5abe, {len_from(), len_from(), len_to(), len_to(), name_from(), cash, name_to(), date});
                } else {
                    say(0x10400c7c, 0x5a10, {len_from(), len_from(), len_to(), len_to(), name_from(), cash, name_to(), date});
                }
            } else if (s.reason == 5) {
                if (s.amount == 0) say(0x10400e34, 0x5d93, {len_from(), len_from(), len_to(), len_to(), name_from(), name_to(), date});
                else say(0x10400de4, 0x5c96, {len_from(), len_from(), len_to(), len_to(), name_from(), cash, name_to(), date});
            } else if (s.reason == 0x23) {
                if (s.amount == 0) say(0x10400eec, 0x5f58, {len_from(), len_from(), len_to(), len_to(), name_from(), name_to(), date});
                else say(0x10400e9c, 0x5e79, {len_from(), len_from(), len_to(), len_to(), name_from(), cash, name_to(), date});
            } else if (s.reason == 6) {
                say(0x10400f44, 0x6020, {len_from(), len_from(), len_to(), len_to(), name_from(), name_to(), date});
            } else if (s.reason == 10) {
                if (s.amount == 0) {
                    if (s.extra == 0) say(0x104010b4, 0x6334, {len_from(), len_from(), len_to(), len_to(), name_from(), name_to(), date});
                    else say(0x10401064, 0x6284, {len_from(), len_from(), len_to(), len_to(), name_from(), name_to(), date});
                } else if (s.extra == 0) {
                    say(0x1040100c, 0x61bf, {len_from(), len_from(), len_to(), len_to(), name_from(), cash, name_to(), date});
                } else {
                    say(0x10400fb4, 0x60f8, {len_from(), len_from(), len_to(), len_to(), name_from(), cash, name_to(), date});
                }
            } else if (s.amount == 0) {
                if (s.extra == 0) say(0x1040123c, 0x663f, {len_from(), len_from(), len_to(), len_to(), name_from(), name_to(), date, reasons});
                else say(0x104011e4, 0x6584, {len_from(), len_from(), len_to(), len_to(), name_from(), name_to(), date, reasons});
            } else if (s.extra == 0) {
                say(0x10401184, 0x64b4, {len_from(), len_from(), len_to(), len_to(), name_from(), cash, name_to(), date, reasons});
            } else {
                say(0x10401124, 0x63e2, {len_from(), len_from(), len_to(), len_to(), name_from(), cash, name_to(), date, reasons});
            }
        } else if (s.reason == 0) {
            if (s.marker == 1) {
                if (s.extra == 0) say(0x104005a0, 0x4cce, {len_from(), len_from(), len_to(), len_to(), name_from(), name_to(), date});
                else say(0x10400550, 0x4c40, {len_from(), len_from(), len_to(), len_to(), name_from(), name_to(), date});
            } else if (s.amount != 0) {
                if (s.extra == 0) say(0x10400660, 0x4dfa, {len_from(), len_from(), len_to(), len_to(), name_from(), cash, name_to(), date});
                else say(0x10400608, 0x4d5a, {len_from(), len_from(), len_to(), len_to(), name_from(), cash, name_to(), date});
            } else if (s.extra == 0) {
                say(0x10400708, 0x4f21, {len_from(), len_from(), len_to(), len_to(), name_from(), name_to(), date});
            } else {
                say(0x104006b8, 0x4e98, {len_from(), len_from(), len_to(), len_to(), name_from(), name_to(), date});
            }
        } else if (s.reason == 5) {
            if (s.amount == 0) say(0x104007c0, 0x50a1, {len_from(), len_from(), len_to(), len_to(), name_from(), date, name_to()});
            else say(0x10400770, 0x4fa9, {len_from(), len_from(), len_to(), len_to(), name_from(), cash, date, name_to()});
        } else if (s.reason == 0x23) {
            if (s.amount == 0) say(0x10400878, 0x525c, {len_from(), len_from(), len_to(), len_to(), name_from(), date, name_to()});
            else say(0x10400828, 0x5182, {len_from(), len_from(), len_to(), len_to(), name_from(), cash, date, name_to()});
        } else if (s.reason == 6) {
            if (s.from == -1) say(0x10400914, 0x53eb, {len_to(), len_to(), date, name_to()});
            else say(0x104008e0, 0x531f, {len_from(), len_from(), len_to(), len_to(), 0x64, name_from(), name_to(), date});
        } else if (s.reason == 10) {
            if (s.amount == 0) {
                if (s.extra == 0) say(0x10400a84, 0x5690, {len_from(), len_from(), len_to(), len_to(), name_from(), name_to(), date});
                else say(0x10400a34, 0x55ee, {len_from(), len_from(), len_to(), len_to(), name_from(), name_to(), date});
            } else if (s.extra == 0) {
                say(0x104009dc, 0x5537, {len_from(), len_from(), len_to(), len_to(), name_from(), cash, name_to(), date});
            } else {
                say(0x10400984, 0x547e, {len_from(), len_from(), len_to(), len_to(), name_from(), cash, name_to(), date});
            }
        } else if (s.amount == 0) {
            if (s.extra == 0) say(0x10400c0c, 0x5964, {len_from(), len_from(), len_to(), len_to(), name_from(), name_to(), date, reasons});
            else say(0x10400bb4, 0x58b7, {len_from(), len_from(), len_to(), len_to(), name_from(), name_to(), date, reasons});
        } else if (s.extra == 0) {
            say(0x10400b54, 0x57f5, {len_from(), len_from(), len_to(), len_to(), name_from(), cash, name_to(), date, reasons});
        } else {
            say(0x10400af4, 0x5731, {len_from(), len_from(), len_to(), len_to(), name_from(), cash, name_to(), date, reasons});
        }
    } else {
        if (s.flagged) {
            if (s.reason == 0) say(0x10401568, 0x6c29, {len_to(), len_to(), len_from(), len_from(), name_to(), name_from(), date});
            else if (s.reason == 3) say(0x104015c0, 0x6cbe, {len_to(), len_to(), len_from(), len_from(), name_to(), name_from(), date});
            else if (s.reason == 4) say(0x10401618, 0x6d7a, {len_to(), len_to(), len_from(), len_from(), name_to(), name_from(), date});
            else if (s.reason == 0x22) say(0x10401670, 0x6e5d, {len_to(), len_to(), len_from(), len_from(), name_to(), name_from(), date});
            else if (s.reason == 6) say(0x104016c8, 0x6020, {len_from(), len_from(), len_to(), len_to(), name_to(), name_from(), date});
        } else if (s.reason == 0) {
            if (s.from == -1) say(0x104012d8, 0x6780, {len_to(), len_to(), name_to(), date});
            else say(0x104012a4, 0x66f8, {len_to(), len_to(), len_from(), len_from(), name_to(), name_from(), date});
        } else if (s.reason == 0x16) {
            say(0x10401330, 0x67e6, {len_to(), len_to(), len_from(), len_from(), name_to(), name_from(), date});
        } else if (s.reason == 3) {
            say(0x10401388, 0x68a2, {len_to(), len_to(), len_from(), len_from(), name_to(), name_from(), date});
        } else if (s.reason == 4) {
            if (s.from == -1) say(0x1040141c, 0x6a27, {len_to(), len_to(), name_to(), date});
            else say(0x104013e8, 0x6951, {len_to(), len_to(), len_from(), len_from(), name_to(), name_from(), date});
        } else if (s.reason == 0x22) {
            if (s.from == -1) say(0x104014b0, 0x6b93, {len_to(), len_to(), name_to(), date});
            else say(0x1040147c, 0x6adb, {len_to(), len_to(), len_from(), len_from(), name_to(), name_from(), date});
        } else if (s.reason == 6) {
            say(0x10401510, 0x531f, {len_from(), len_from(), len_to(), len_to(), 0x64, name_to(), name_from(), date});
        }
        if (s.reason == 2) say(0x1040171c, 0x6f22, {len_to(), len_to(), len_from(), len_from(), name_to(), name_from(), date});
    }
}
}
}
