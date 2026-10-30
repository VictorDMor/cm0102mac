#pragma once
// DB_FILES.CPP: the database tables on disk. Every table has a loader and a
// conversion from the packed Windows records to the Mac layout, all but oldstaff
// have a saver, and four tables also read their older version field by field.
#include "guest_abi.hpp"

namespace cm::aot::db_files {
using guest::Cpu;

std::uint32_t open_for_reading(Cpu& c, std::uint32_t directory, std::uint32_t name);
std::uint32_t open_for_writing(Cpu& c, std::uint32_t directory, std::uint32_t name);

// The tables, named for their pool strings; the loader of each is at the entry
// listed in db_files.cpp.
enum class Table {
    continents, nations, cities, stadiums, clubs, staff, oldstaff, non_players, players,
    staff_preferences, staff_history, club_comps, club_comp_history, staff_comps,
    staff_comp_history, officials, colours, names,
};
std::uint32_t load(Cpu& c, Table table, std::uint32_t file, std::uint32_t packed,
                   std::uint32_t records, std::uint32_t header);
std::uint32_t save(Cpu& c, std::uint32_t entry, std::uint32_t record_size, std::uint32_t file,
                   std::uint32_t packed, std::uint32_t records, std::uint32_t count);

// Windows record layout to Mac layout, in place of the loaders' raw read.
void convert_continents(Cpu&, std::uint32_t out, std::uint32_t in, std::uint32_t count);
void convert_nations(Cpu&, std::uint32_t out, std::uint32_t in, std::uint32_t count);
void convert_cities(Cpu&, std::uint32_t out, std::uint32_t in, std::uint32_t count);
void convert_stadiums(Cpu&, std::uint32_t out, std::uint32_t in, std::uint32_t count);
void convert_clubs(Cpu&, std::uint32_t out, std::uint32_t in, std::uint32_t count);
void convert_staff(Cpu&, std::uint32_t out, std::uint32_t in, std::uint32_t count);
void convert_oldstaff(Cpu&, std::uint32_t out, std::uint32_t in, std::uint32_t count);
void convert_non_players(Cpu&, std::uint32_t out, std::uint32_t in, std::uint32_t count);
void convert_players(Cpu&, std::uint32_t out, std::uint32_t in, std::uint32_t count);
void convert_staff_preferences(Cpu&, std::uint32_t out, std::uint32_t in, std::uint32_t count);
void convert_staff_history(Cpu&, std::uint32_t out, std::uint32_t in, std::uint32_t count);
void convert_club_comps(Cpu&, std::uint32_t out, std::uint32_t in, std::uint32_t count);
void convert_club_comp_history(Cpu&, std::uint32_t out, std::uint32_t in, std::uint32_t count);
void convert_staff_comps(Cpu&, std::uint32_t out, std::uint32_t in, std::uint32_t count);
void convert_staff_comp_history(Cpu&, std::uint32_t out, std::uint32_t in, std::uint32_t count);
void convert_officials(Cpu&, std::uint32_t out, std::uint32_t in, std::uint32_t count);
void convert_colours(Cpu&, std::uint32_t out, std::uint32_t in, std::uint32_t count);
void convert_names(Cpu&, std::uint32_t out, std::uint32_t in, std::uint32_t count);

// Version 1 files of the tables that changed, read field by field.
std::uint32_t read_old_clubs(Cpu&, std::uint32_t file, std::uint32_t packed, std::uint32_t records, std::uint32_t count);
std::uint32_t read_old_non_players(Cpu&, std::uint32_t file, std::uint32_t packed, std::uint32_t records, std::uint32_t count);
std::uint32_t read_old_players(Cpu&, std::uint32_t file, std::uint32_t packed, std::uint32_t records, std::uint32_t count);
std::uint32_t read_old_club_comps(Cpu&, std::uint32_t file, std::uint32_t packed, std::uint32_t records, std::uint32_t count);
}
