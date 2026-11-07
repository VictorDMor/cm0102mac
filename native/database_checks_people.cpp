// cm-database-checks fixtures for DATABASE.CPP's people block (database_checks.hpp). The routines draw
// from the game's random generator (rng_core 0x1077b770, state word at TOC -0x34c4). A case runs either with
// the draws scripted (every outcome of every draw covered; the number, order and registers of the calls are
// compared) or with the real generator seeded from the case (the draws and the state word compared).
// Each part of the block has its own file with a provider.
#include "database_checks_people_detail.hpp"

namespace cm::aot::database_checks {
std::vector<Fixture> people_names_fixtures();       // database_checks_people_names.cpp
std::vector<Fixture> people_indexes_fixtures();     // database_checks_people_indexes.cpp
std::vector<Fixture> people_attributes_fixtures();  // database_checks_people_attributes.cpp
std::vector<Fixture> people_swaps_fixtures();       // database_checks_people_swaps.cpp
std::vector<Fixture> people_initial_fixtures();     // database_checks_people_initial.cpp
std::vector<Fixture> people_staff_fixtures();       // database_checks_people_staff.cpp
std::vector<Fixture> people_extra_fixtures();       // database_checks_people_extra.cpp

std::vector<Fixture> people_fixtures() {
    std::vector<Fixture> found;
    for (const auto provider : {people_names_fixtures, people_indexes_fixtures, people_attributes_fixtures, people_swaps_fixtures,
                                people_initial_fixtures, people_staff_fixtures, people_extra_fixtures})
        for (auto& f : provider()) found.push_back(std::move(f));
    return found;
}
}
