#include "competition_defaults.hpp"
#include "career_rules.hpp"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace cm::aot {
namespace fs = std::filesystem;

fs::path CompetitionDefaults::file(const fs::path& directory, CompetitionEdition edition) {
    return directory / (std::string(edition_name(edition)) + ".toml");
}

std::vector<competition_values::Entry> read_competition_defaults(const fs::path& file, unsigned* skipped) {
    std::vector<competition_values::Entry> out;
    if (skipped) *skipped = 0;
    std::ifstream input(file, std::ios::binary);
    if (!input) return out;
    const std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    // Each record was appended as a dump of its own, starting with the dump's comment line.
    std::set<std::tuple<competition_values::Kind, std::int32_t, std::int32_t>> seen;
    for (std::size_t at = 0; at < text.size();) {
        auto next = text.find("\n# Competition values", at);
        next = next == std::string::npos ? text.size() : next + 1;
        const auto parsed = competition_values::parse(std::string_view(text).substr(at, next - at));
        at = next;
        if (!parsed) { if (skipped) ++*skipped; continue; }
        for (const auto& e : *parsed)
            if (seen.emplace(e.kind, e.identity.id, e.identity.stage).second) out.push_back(e);
    }
    return out;
}

std::set<CompetitionDefaults::Key>* CompetitionDefaults::seen(CompetitionEdition edition) {
    auto& known = seen_.at(std::size_t(edition));
    if (known) return &*known;
    unsigned skipped = 0;
    const auto entries = read_competition_defaults(file(directory_, edition), &skipped);
    if (skipped) std::fprintf(stderr, "Competition defaults %s: %u unreadable record(s) will be recorded again\n",
                              file(directory_, edition).string().c_str(), skipped);
    known.emplace();
    for (const auto& e : entries) known->emplace(e.kind, e.identity.id, e.identity.stage);
    return &*known;
}

void CompetitionDefaults::record(const State& s, std::uint32_t object, const competition_overrides::Target& target,
                                 CompetitionEdition edition) {
    if (failed_) return;
    auto* known = seen(edition);
    const Key key{target.kind, target.id, target.stage};
    if (known->contains(key)) return;
    try {
        auto entry = competition_values::read_competition(s, object, target.kind);
        // A stage's own record names the stage; its key is the owning competition's id and the stage index.
        entry.identity.id = target.id;
        entry.identity.stage = target.stage;
        entry.identity.parent = target.stage >= 0 ? target.id : -1;
        std::error_code error;
        fs::create_directories(directory_, error);
        std::ofstream output(file(directory_, edition), std::ios::binary | std::ios::app);
        const auto text = competition_values::write({entry});
        if (!output.write(text.data(), std::streamsize(text.size())).flush()) throw std::runtime_error("cannot write");
        known->insert(key);
    } catch (const std::runtime_error& error) {
        failed_ = true;
        std::fprintf(stderr, "Competition defaults not recorded (%s): %s\n", file(directory_, edition).string().c_str(), error.what());
    }
}
}
