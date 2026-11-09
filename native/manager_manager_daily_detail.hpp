#pragma once
// MANAGER_MANAGER.CPP daily update (manager_manager_daily.hpp): the record fields it reads. Offsets
// are the original's; meanings are named as far as the code shows them (marked "inferred" where
// only the use is known).
#include <cstdint>

namespace cm::aot::manager_manager::daily_detail {
// The game's date: a day number (int16, compared second), a year (uint16, compared first) and a byte.
namespace date {
constexpr std::uint32_t day = 0x0, year = 0x2, extra = 0x4;
}
// The module object (TOC -0x79f0).
namespace object {
constexpr std::uint32_t rows = 0x0, jobs = 0x8;
}
// The board's row of a club (0x4c bytes by the club's id).
namespace row {
constexpr std::uint32_t size = 0x4c, level = 0x1, flags = 0x2, stamp = 0x20;
constexpr std::uint32_t flag_open = 0x80;
}
// The job offer (the list's elements): `person` and `candidate` index the people table.
namespace job {
constexpr std::uint32_t active = 0x0, asked = 0x1, candidate = 0x4, person = 0x8, club = 0x10, decide = 0x18,
                        reminder = 0x20;
}
// A person (0x70 bytes). The last 16 of the table are the human managers. A post is a club link
// with its status byte and since-date: `club_post` for an ordinary club, `side_post` for a club
// the flag routine 0x100c7d00 answers true for (inferred: national sides).
namespace person {
constexpr std::uint32_t size = 0x70, id = 0x0, extra = 0x8, career = 0x14, staff = 0x1c, side_post = 0x20, club_post = 0x24,
                        place_a = 0x28, place_b = 0x2c, place_c = 0x30, side_since = 0x3c, club_since = 0x4c,
                        side_status = 0x60, club_status = 0x62, human = 0x6b;
constexpr std::uint32_t human_slots = 16;
}
// A club (0x248 bytes).
namespace club {
constexpr std::uint32_t size = 0x248, id = 0x0, region = 0x2c, league = 0x30, post_a = 0x80, slots_a = 0x84,
                        manager = 0x90, post_b = 0x94, applicants = 0x98, slots_b = 0x160, slots_c = 0x174,
                        slots_d = 0x190, reputation = 0x1ec;
constexpr unsigned applicant_count = 50;
}
namespace region {
constexpr std::uint32_t scope = 0x68, monthly = 0x74, open_flag = 0x120;
}
// The statuses the posts move through (the byte at +0x60 / +0x62).
namespace status {
constexpr std::uint32_t signed_up = 5, handed_on = 6, kept_on = 8, free = 11, hired = 12;
}
// TOC slots.
namespace toc {
constexpr std::int32_t pool = -0x5368, clubs = -0x7be8, club_count = -0x7b50, people = -0x7be0,
                       people_count = -0x7be4, date = -0x7b80, success = -0x7bac, message = -0x7ba8,
                       deals = -0x7b7c, news = -0x7b40, events = -0x7bf0, leagues = -0x7b5c;
}
}
