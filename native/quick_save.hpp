#pragma once

#include <optional>
#include <string>
#include <utility>

namespace cm::aot {
// These are original-game path bytes, not host filesystem paths. The original
// preferences' directory and basename can change before a save succeeds, so
// they cannot identify the last completed operation on their own.
class QuickSaveDestination {
public:
    void begin_load(std::optional<std::string> path) { loading_ = std::move(path); }
    // A failed load may have replaced part of the world, so the career that
    // was active no longer identifies what is in memory: nothing can be saved
    // over it until a load or an ordinary save succeeds.
    void finish_load(bool success) {
        if (success) last_ = std::move(loading_);
        else last_.reset();
        loading_.reset();
    }
    void begin_save(std::optional<std::string> path) { saving_ = std::move(path); }
    void finish_save(bool success) {
        if (success) last_ = std::move(saving_);
        saving_.reset();
    }
    const std::optional<std::string>& last() const { return last_; }

private:
    std::optional<std::string> last_, loading_, saving_;
};
}
