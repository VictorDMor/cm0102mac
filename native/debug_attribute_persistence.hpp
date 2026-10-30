#pragma once
#include "aot_runtime.hpp"
#include <CommonCrypto/CommonDigest.h>
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace cm::aot::debug_attribute_persistence {
class Digest {
    CC_SHA256_CTX context_{};
public:
    Digest() { CC_SHA256_Init(&context_); }
    void add(const void* data, std::size_t length) {
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        while (length) {
            const auto amount = std::min<std::size_t>(length, 1u << 20);
            CC_SHA256_Update(&context_, bytes, CC_LONG(amount));
            bytes += amount; length -= amount;
        }
    }
    std::string finish() {
        std::array<unsigned char, CC_SHA256_DIGEST_LENGTH> result{};
        CC_SHA256_Final(result.data(), &context_);
        std::ostringstream out;
        for (auto byte : result) out << std::hex << std::setw(2) << std::setfill('0') << unsigned(byte);
        return out.str();
    }
};

inline std::string save_key(const std::filesystem::path& save) {
    Digest digest;
    const auto path = std::filesystem::weakly_canonical(save).string();
    digest.add(path.data(), path.size());
    const std::uint8_t separator = 0;
    digest.add(&separator, 1);
    std::ifstream input(save, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot read saved career for attribute locks");
    std::array<char, 65536> bytes{};
    while (input) {
        input.read(bytes.data(), bytes.size());
        digest.add(bytes.data(), std::size_t(input.gcount()));
    }
    if (!input.eof()) throw std::runtime_error("Cannot hash saved career for attribute locks");
    return digest.finish();
}

inline std::string identity(const State& s, std::uint32_t staff) {
    Digest digest;
    const auto birthday = s.memory.copy_out(staff + 0x34, 5);
    digest.add(birthday.data(), birthday.size());
    for (unsigned offset : {0x28u, 0x2cu, 0x30u}) {
        const auto name = std::uint32_t(s.memory.read<4>(staff + offset));
        bool ended = false;
        for (unsigned i = 0; i < 256; ++i) {
            const auto byte = std::uint8_t(name ? s.memory.read<1>(name + i) : 0);
            digest.add(&byte, 1);
            if (!byte) { ended = true; break; }
        }
        if (!ended) throw std::runtime_error("Invalid player name in attribute lock");
    }
    return digest.finish();
}

struct Tables { std::uint32_t staff, staff_count, players, player_count; };
inline Tables tables(const State& s) {
    const auto toc = std::uint32_t(s.r<4>(8));
    const auto value = [&](int offset) { return std::uint32_t(s.memory.read<4>(s.memory.read<4>(toc + offset))); };
    return {value(-0x7be0), value(-0x7be4), value(-0x7b38), value(-0x74d4)};
}
inline std::uint32_t resolve(const State& s, const Tables& t, std::uint32_t id) {
    if (!t.staff || !t.players || id >= t.staff_count || std::uint64_t(t.staff) + std::uint64_t(id) * 0x70 + 0x70 > (UINT64_C(1) << 32)) return 0;
    const auto staff = std::uint32_t(t.staff + id * 0x70);
    if (s.memory.read<4>(staff) != id) return 0;
    const auto player = std::uint32_t(s.memory.read<4>(staff + 0x14));
    if (player < t.players || (player - t.players) % 0x48 || (player - t.players) / 0x48 >= t.player_count) return 0;
    return staff;
}

inline void save(const State& s, const std::filesystem::path& directory, const std::filesystem::path& career) {
    const auto records = s.memory.debug_locked_players();
    const auto target = directory / save_key(career);
    std::ostringstream data;
    data << "CMDEBUGATTR 1\n" << records.size() << '\n';
    const auto t = tables(s);
    for (const auto& [staff, player] : records) {
        const auto id = std::uint32_t(s.memory.read<4>(staff));
        if (resolve(s, t, id) != staff || s.memory.read<4>(staff + 0x14) != player)
            throw std::runtime_error("Invalid locked player identity during save");
        data << id << ' ' << identity(s, staff) << '\n';
    }
    std::filesystem::create_directories(directory);
    auto pattern = (directory / ".locks-XXXXXX").string();
    const int fd = mkstemp(pattern.data());
    if (fd < 0) throw std::runtime_error("Cannot create attribute lock metadata");
    const auto content = data.str();
    std::size_t done = 0;
    while (done < content.size()) {
        const auto n = ::write(fd, content.data() + done, content.size() - done);
        if (n <= 0) { ::close(fd); std::filesystem::remove(pattern); throw std::runtime_error("Cannot write attribute lock metadata"); }
        done += std::size_t(n);
    }
    const bool synced = ::fsync(fd) == 0;
    const bool closed = ::close(fd) == 0;
    if (!synced || !closed) { std::filesystem::remove(pattern); throw std::runtime_error("Cannot sync attribute lock metadata"); }
    std::filesystem::rename(pattern, target);
}

inline void load(State& s, const std::filesystem::path& directory, const std::filesystem::path& career) {
    if (!std::filesystem::exists(directory)) return;
    const auto target = directory / save_key(career);
    if (!std::filesystem::exists(target)) return;
    std::ifstream data(target);
    std::string magic; unsigned version = 0; std::size_t count = 0;
    if (!(data >> magic >> version >> count) || magic != "CMDEBUGATTR" || version != 1 || count > 100000)
        throw std::runtime_error("Invalid attribute lock metadata");
    const auto t = tables(s);
    std::vector<std::pair<std::uint32_t, std::uint32_t>> selected;
    std::set<std::uint32_t> seen_staff, seen_players;
    for (std::size_t i = 0; i < count; ++i) {
        std::uint32_t id; std::string fingerprint;
        if (!(data >> id >> fingerprint) || fingerprint.size() != 64) throw std::runtime_error("Incomplete attribute lock metadata");
        const auto staff = resolve(s, t, id);
        if (!staff || identity(s, staff) != fingerprint) throw std::runtime_error("Saved player identity changed");
        const auto player = std::uint32_t(s.memory.read<4>(staff + 0x14));
        if (!seen_staff.insert(staff).second || !seen_players.insert(player).second)
            throw std::runtime_error("Duplicate player in attribute lock metadata");
        selected.emplace_back(staff, player);
    }
    data >> std::ws;
    if (!data.eof()) throw std::runtime_error("Unexpected attribute lock metadata");
    for (const auto& [staff, player] : selected) s.memory.lock_debug_player_attributes(staff, player);
}
}
