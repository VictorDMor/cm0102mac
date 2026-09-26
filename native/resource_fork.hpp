#pragma once
#include <cstdint>
#include <span>
#include <vector>

namespace cm {
struct Resource {
    std::uint32_t type;
    std::int16_t id;
    std::uint8_t attributes;
    // Resource names are original MacRoman bytes without the Pascal length.
    std::vector<std::uint8_t> name;
    std::vector<std::uint8_t> data;
    bool operator==(const Resource&) const = default;
};
class ResourceFork {
public:
    explicit ResourceFork(std::span<const std::uint8_t> bytes);
    const std::vector<Resource>& resources() const { return resources_; }
    const Resource* find(std::uint32_t type, std::int16_t id) const;
private:
    std::vector<Resource> resources_;
};
std::vector<std::uint8_t> encode_resource_fork(std::span<const Resource> resources);
}
