// cm-competition-values: prints the plain values of every competition of a world snapshot as TOML.
//   cm-competition-values SNAPSHOT [--out FILE] [--check]
// --check also verifies the snapshot against the facts the tool relies on (counts, unique keys, the round trip,
// known values) and exits 1 on a failure.
#include "aot_runtime.hpp"
#include "competition_values.hpp"
#include "match_replay.hpp"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>

using namespace cm::aot;
using namespace cm::aot::competition_values;

namespace {
int failures = 0;
void expect(bool ok, const std::string& what) {
    std::fprintf(stderr, "%s: %s\n", ok ? "ok  " : "FAIL", what.c_str());
    if (!ok) ++failures;
}
// The game's texts are localized (this career is the Portuguese edition: "Primeira Liga Inglesa" is the Premier
// Division), so a league is found by its nation code and its club count, not by name.
const Entry* find_league(const std::vector<Entry>& all, const std::string& nation, int clubs) {
    for (const auto& e : all)
        if (e.kind == Kind::league && e.identity.stage < 0 && e.identity.nation == nation && e.values.at("league.structure.clubs") == clubs) return &e;
    return nullptr;
}

void check(const std::vector<Entry>& all, const std::string& text, unsigned skipped) {
    std::size_t leagues = 0, cups = 0, stages = 0;
    std::set<std::string> keys;
    for (const auto& e : all) {
        (e.kind == Kind::league ? leagues : cups) += 1;
        stages += e.identity.stage >= 0;
        keys.insert(std::string(e.kind == Kind::league ? "league." : "cup.") + key(e.identity));
    }
    std::fprintf(stderr, "leagues %zu, cups %zu (stage objects %zu), skipped objects of another kind %u\n", leagues, cups, stages, skipped);
    expect(keys.size() == all.size(), "every identifier is unique");
    expect(!all.empty() && leagues > 0 && cups > 0, "leagues and cups were found");
    const auto parsed = parse(text);
    expect(parsed.has_value(), parsed ? "the text parses" : "the text parses (line " + std::to_string(parsed.error().line) + ": " + parsed.error().message + ")");
    if (parsed) expect(write(*parsed) == text, "write(parse(write(x))) == write(x)");
    // Strictness: a corrupted text must be rejected with a line number.
    {
        auto bad = text;
        bad.insert(text.find("bench = "), "no_such_key = 1\n");
        const auto r = parse(bad);
        expect(!r && r.error().line > 1, "an unknown key is rejected with its line");
        const auto r2 = parse("[league.1]\nid = x\n");
        expect(!r2 && r2.error().line == 2, "a non-integer value is rejected with its line");
    }
    const auto v = [](const Entry& e, const char* id) { return e.values.at(id); };
    if (const auto* p = find_league(all, "ING", 20)) {
        expect(v(*p, "league.points.win") == 3 && v(*p, "league.points.draw") == 1, "Premier: 3 points for a win, 1 for a draw");
        expect(v(*p, "league.relegation.places") == 3 && v(*p, "league.structure.clubs") == 20 && v(*p, "league.structure.rounds") == 38,
               "Premier: relegation 3, 20 clubs, 38 rounds");
    } else expect(false, "English Premier Division found (nation ING, 20 clubs)");
    if (const auto* f = find_league(all, "ING", 24); f && v(*f, "league.structure.rounds") == 46) {
        expect(v(*f, "league.promotion.automatic") == 2 && v(*f, "league.promotion.playoff") == 4, "English First Division: automatic 2, playoff 4");
    } else expect(false, "English First Division found (nation ING, 24 clubs, 46 rounds)");
    bool state_bench = false;
    for (const auto& e : all)
        if (e.kind == Kind::league && e.identity.name.starts_with("Campeonato Estadual") && v(e, "league.bench") == 7 && v(e, "league.substitutions") == 3) state_bench = true;
    expect(state_bench, "a Brazilian state championship has bench 7, substitutions 3");
    // The registry: ids are unique and every key is distinct inside its table.
    std::set<std::string> ids, table_keys;
    bool unique = true;
    for (const auto& p : parameters()) {
        unique &= ids.insert(std::string(p.id)).second;
        unique &= table_keys.insert(std::to_string(int(p.scope)) + (p.read_only ? "r" : "w") + toml_key(p)).second;
    }
    expect(unique, "registry ids and TOML keys are unique");
    std::map<std::string, std::size_t> problems;
    for (const auto& e : all) {
        for (const auto& [id, value] : e.values) if (!range_problem(*find_parameter(id), value).empty()) ++problems[range_problem(*find_parameter(id), value)];
        for (const auto& row : e.rounds)
            for (const auto& [id, value] : row) if (!range_problem(*find_parameter(id), value).empty()) ++problems[range_problem(*find_parameter(id), value)];
    }
    for (const auto& [text, count] : problems) std::fprintf(stderr, "note: %zu values: %s\n", count, text.c_str());
}
}  // namespace

int main(int argc, char** argv) {
    std::string snapshot, out;
    bool do_check = false;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--check") do_check = true;
        else if (a == "--out" && i + 1 < argc) out = argv[++i];
        else if (snapshot.empty() && a[0] != '-') snapshot = a;
        else { std::fprintf(stderr, "usage: cm-competition-values SNAPSHOT [--out FILE] [--check]\n"); return 2; }
    }
    if (snapshot.empty()) { std::fprintf(stderr, "usage: cm-competition-values SNAPSHOT [--out FILE] [--check]\n"); return 2; }
    try {
        auto state = std::make_unique<State>();
        load_match_snapshot(*state, snapshot);
        unsigned skipped = 0;
        const auto all = read_competitions(*state, &skipped);
        const auto text = write(all);
        if (out.empty()) std::cout << text;
        else std::ofstream(out, std::ios::binary) << text;
        if (do_check) check(all, text, skipped);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    return failures ? 1 : 0;
}
