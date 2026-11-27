#pragma once
#include <mutex>

namespace cm::aot::shortlist_workers {
// One batch owns the locks. Workers share the guest records, and the game
// thread waits for the batch before walking or mutating those records itself.
struct SharedRecords {
    std::mutex manager_list;
    std::mutex transfers;
};

inline thread_local SharedRecords* active_records = nullptr;

class RecordScope {
public:
    explicit RecordScope(SharedRecords& records) : previous_(active_records) { active_records = &records; }
    ~RecordScope() { active_records = previous_; }
    RecordScope(const RecordScope&) = delete;
    RecordScope& operator=(const RecordScope&) = delete;
private:
    SharedRecords* previous_;
};

inline std::unique_lock<std::mutex> lock(std::mutex SharedRecords::* domain) {
    if (active_records) return std::unique_lock(active_records->*domain);
    return {};
}
}
