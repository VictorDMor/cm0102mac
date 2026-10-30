#pragma once
#include "debug_player_attributes.hpp"
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace cm::aot {
// Guest addresses are resolved afresh for each loaded career. The owner is
// responsible for invalidating a registration before a record is reused.
class DebugAttributeLock {
public:
    void add(std::uint32_t staff, std::uint32_t player) {
        remove(staff);
        remove_player(player);
        players_[staff] = player;
        debug_player_attributes::for_each(staff, player, [&](auto address, auto value) {
            bytes_[address] = {staff, player, value};
        });
    }
    void remove(std::uint32_t staff) {
        players_.erase(staff);
        for (auto it = bytes_.begin(); it != bytes_.end();)
            if (it->second.staff == staff) it = bytes_.erase(it); else ++it;
    }
    void clear() { bytes_.clear(); players_.clear(); }
    void remove_player(std::uint32_t player) {
        for (auto it = players_.begin(); it != players_.end();)
            if (it->second == player) it = players_.erase(it); else ++it;
        for (auto it = bytes_.begin(); it != bytes_.end();)
            if (it->second.player == player) it = bytes_.erase(it); else ++it;
    }
    bool empty() const { return bytes_.empty(); }
    std::vector<std::pair<std::uint32_t, std::uint32_t>> players() const {
        return {players_.begin(), players_.end()};
    }
    // Apply after the write and before notifying observers. This also handles
    // multi-byte and partial overlapping stores without touching other bytes.
    bool apply(std::uint32_t address, std::uint8_t* data, std::size_t size) const {
        const auto end = std::uint64_t(address) + size;
        auto it = bytes_.lower_bound(address);
        const bool overlaps = it != bytes_.end() && it->first < end;
        for (; it != bytes_.end() && it->first < end; ++it)
            data[it->first - address] = it->second.value;
        return overlaps;
    }
private:
    struct Byte { std::uint32_t staff, player; std::uint8_t value; };
    std::map<std::uint32_t, Byte> bytes_;
    std::map<std::uint32_t, std::uint32_t> players_;
};
}
