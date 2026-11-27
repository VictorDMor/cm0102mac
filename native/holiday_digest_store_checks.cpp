#include "holiday_digest_store.hpp"
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace cm::aot::holiday_digest;
namespace fs = std::filesystem;
namespace {
void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
struct Directory {
    fs::path path;
    Directory() {
        auto pattern = (fs::temp_directory_path() / "cm-holiday-store-XXXXXX").string();
        if (!mkdtemp(pattern.data())) throw std::runtime_error("Cannot create test directory");
        path = pattern;
    }
    ~Directory() { std::error_code ignored; fs::remove_all(path, ignored); }
};
}

int main() {
    try {
        Directory directory;
        const auto member = directory.path / Store::member_name;
        Store original;
        original.next_id = 38;
        Report away{14, 7, 30, true, "9/8/2002", "3", {"Away game"}, {"New arrival"}, {"Departure"}, {}};
        Report completed{37, 12, 90, false, "4/12/2002", "", {}, {}, {}, {}};
        completed.lines.emplace_back("A\0B\nC", 5);
        completed.lines.insert(completed.lines.end(), 3000, std::string(400, 'x'));
        original.reports.emplace(away.id, away);
        original.reports.emplace(completed.id, completed);
        original.active_ids = {away.id};
        original.save(member);

        Store restored;
        restored.load(member);
        require(restored.reports == original.reports && restored.next_id == original.next_id &&
                restored.active_ids == original.active_ids, "Failed to restore complete holiday news");

        Store malformed = original;
        malformed.next_id = completed.id;
        bool rejected = false;
        try { malformed.save(member); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Invalid sequence was saved");
        restored.load(member);
        require(restored.reports == original.reports, "Failed save replaced the working member");

        fs::resize_file(member, fs::file_size(member) - 1);
        rejected = false;
        try { restored.load(member); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected && restored.reports == original.reports, "Partial report leaked into running store");

        original.reports.clear();
        original.active_ids.clear();
        original.save(member);
        restored.load(member);
        require(restored.next_id == 38 && restored.reports.empty() && restored.active_ids.empty(),
                "Deleting reports reset the archive sequence");
        Store{}.save(member);
        require(!fs::exists(member), "Fresh career kept a previous career's member");
        restored.load(member);
        require(restored.next_id == 1 && restored.reports.empty() && restored.active_ids.empty(),
                "Legacy career inherited another career's holiday news");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "Holiday news persistence checks passed\n";
}
