#include "resource_fork.hpp"
#include <bit>
#include <algorithm>
#include <set>
#include <map>
#include <stdexcept>

namespace cm {
namespace {
class View {
public:
    explicit View(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}
    std::span<const std::uint8_t> slice(std::size_t offset, std::size_t size) const {
        if (offset > bytes_.size() || size > bytes_.size() - offset)
            throw std::runtime_error("Resource fork range is outside its declared section");
        return bytes_.subspan(offset, size);
    }
    std::uint32_t number(std::size_t offset, unsigned size) const {
        std::uint32_t value = 0;
        for (auto b : slice(offset, size)) value = (value << 8) | b;
        return value;
    }
private:
    std::span<const std::uint8_t> bytes_;
};
std::vector<std::uint8_t> copy(std::span<const std::uint8_t> s) { return {s.begin(), s.end()}; }
}
ResourceFork::ResourceFork(std::span<const std::uint8_t> bytes) {
    View file(bytes);
    const auto data_offset = file.number(0, 4), map_offset = file.number(4, 4);
    const auto data_size = file.number(8, 4), map_size = file.number(12, 4);
    if (data_offset < 16 || map_offset < 16)
        throw std::runtime_error("Resource fork sections overlap the header");
    const auto data_end = std::uint64_t(data_offset) + data_size;
    const auto map_end = std::uint64_t(map_offset) + map_size;
    if (data_offset < map_end && map_offset < data_end)
        throw std::runtime_error("Overlapping resource fork data and map");
    View data(file.slice(data_offset, data_size));
    View map(file.slice(map_offset, map_size));
    const auto type_offset = map.number(24, 2), name_offset = map.number(26, 2);
    if (type_offset < 28 || name_offset > map_size)
        throw std::runtime_error("Invalid resource map list offsets");
    const auto type_word = map.number(type_offset, 2);
    const auto type_count = type_word == 0xffff ? 0 : type_word + 1;
    map.slice(type_offset + 2, type_count * 8);
    std::set<std::pair<std::uint32_t, std::int16_t>> identifiers;
    for (std::uint32_t i = 0; i < type_count; ++i) {
        const auto at = type_offset + 2 + i * 8;
        const auto type = map.number(at, 4);
        const auto count_word = map.number(at + 4, 2);
        const auto count = count_word == 0xffff ? 0 : count_word + 1;
        const auto references = type_offset + map.number(at + 6, 2);
        map.slice(references, count * 12);
        for (std::uint32_t j = 0; j < count; ++j) {
            const auto ref = references + j * 12;
            Resource item;
            item.type = type;
            item.id = std::bit_cast<std::int16_t>(std::uint16_t(map.number(ref, 2)));
            item.attributes = map.number(ref + 4, 1);
            const auto name = map.number(ref + 2, 2);
            if (name != 0xffff) {
                const auto start = name_offset + name;
                item.name = copy(map.slice(start + 1, map.number(start, 1)));
            }
            const auto start = map.number(ref + 5, 3);
            item.data = copy(data.slice(start + 4, data.number(start, 4)));
            if (!identifiers.emplace(item.type, item.id).second)
                throw std::runtime_error("Duplicate resource type and ID");
            resources_.push_back(std::move(item));
        }
    }
}
const Resource* ResourceFork::find(std::uint32_t type, std::int16_t id) const {
    for (const auto& resource : resources_)
        if (resource.type == type && resource.id == id) return &resource;
    return nullptr;
}
std::vector<std::uint8_t> encode_resource_fork(std::span<const Resource> resources) {
    auto append = [](std::vector<std::uint8_t>& out, std::uint32_t value, unsigned size) {
        for (unsigned i = size; i > 0; --i) out.push_back((value >> ((i - 1) * 8)) & 255);
    };
    auto put = [](std::vector<std::uint8_t>& out, std::size_t at, std::uint32_t value, unsigned size) {
        for (unsigned i = size; i > 0; --i) out.at(at + size - i) = (value >> ((i - 1) * 8)) & 255;
    };
    std::vector<std::uint32_t> types;
    std::map<std::uint32_t, std::vector<const Resource*>> grouped;
    std::set<std::pair<std::uint32_t, std::int16_t>> identifiers;
    for (const auto& resource : resources) {
        if (!identifiers.emplace(resource.type, resource.id).second)
            throw std::runtime_error("Duplicate resource type and ID");
        if (!grouped.contains(resource.type)) types.push_back(resource.type);
        grouped[resource.type].push_back(&resource);
    }
    const std::size_t references_start = 2 + types.size() * 8;
    const std::size_t names_start = 28 + references_start + resources.size() * 12;
    if (names_start > UINT16_MAX) throw std::runtime_error("Resource map exceeds 16-bit offsets");
    std::vector<std::uint8_t> data, map(names_start), names;
    put(map, 24, 28, 2);
    put(map, 26, names_start, 2);
    put(map, 28, types.empty() ? 0xffff : types.size() - 1, 2);
    std::size_t reference = references_start;
    for (std::size_t i = 0; i < types.size(); ++i) {
        const auto type = types[i];
        const auto& entries = grouped.at(type);
        put(map, 30 + i * 8, type, 4);
        put(map, 34 + i * 8, entries.size() - 1, 2);
        put(map, 36 + i * 8, reference, 2);
        for (const Resource* resource : entries) {
            if (resource->data.size() > UINT32_MAX || data.size() > 0xffffff || resource->name.size() > 255)
                throw std::runtime_error("Resource exceeds classic format bounds");
            const auto at = 28 + reference;
            put(map, at, std::uint16_t(resource->id), 2);
            if (resource->name.empty()) put(map, at + 2, 0xffff, 2);
            else {
                if (names.size() >= UINT16_MAX) throw std::runtime_error("Resource name list too large");
                put(map, at + 2, names.size(), 2);
                append(names, resource->name.size(), 1);
                names.insert(names.end(), resource->name.begin(), resource->name.end());
            }
            put(map, at + 4, resource->attributes, 1);
            put(map, at + 5, data.size(), 3);
            append(data, resource->data.size(), 4);
            data.insert(data.end(), resource->data.begin(), resource->data.end());
            while (data.size() % 4) data.push_back(0);
            reference += 12;
        }
    }
    map.insert(map.end(), names.begin(), names.end());
    if (data.size() > UINT32_MAX - 256 || map.size() > UINT32_MAX)
        throw std::runtime_error("Resource fork too large");
    std::vector<std::uint8_t> output(256);
    put(output, 0, 256, 4);
    put(output, 4, 256 + data.size(), 4);
    put(output, 8, data.size(), 4);
    put(output, 12, map.size(), 4);
    std::copy_n(output.begin(), 16, map.begin());
    output.insert(output.end(), data.begin(), data.end());
    output.insert(output.end(), map.begin(), map.end());
    return output;
}
}
