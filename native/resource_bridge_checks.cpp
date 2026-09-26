#include "resource_bridge.hpp"
#include "resource_fork.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <unistd.h>
#include <sys/xattr.h>

namespace {
struct TempDirectory {
    std::filesystem::path path;
    TempDirectory() {
        auto pattern = (std::filesystem::temp_directory_path() / "cm-native-resources-XXXXXX").string();
        if (!mkdtemp(pattern.data())) throw std::runtime_error("Cannot create resource test directory");
        path = pattern;
    }
    ~TempDirectory() { std::error_code error; std::filesystem::remove_all(path, error); }
};
}

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: cm-resource-bridge-checks <original-application>\n"; return 2; }
    try {
        const std::filesystem::path app = argv[1];
        std::ifstream file(app / "..namedfork/rsrc", std::ios::binary);
        if (!file) throw std::runtime_error("Cannot open original resource bytes");
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)), {});
        cm::ResourceFork original(bytes);
        cm::aot::NativeResources native(app);
        std::map<std::uint32_t, unsigned> counts;
        for (const auto& resource : original.resources()) {
            ++counts[resource.type];
            const auto loaded = native.get(resource.type, resource.id, true);
            if (!loaded.ticket || !loaded.loaded || loaded.bytes != resource.data)
                throw std::runtime_error("Native Resource Manager altered original payload " + std::to_string(resource.type));
            native.release(loaded.ticket, false);
            if (native.error()) throw std::runtime_error("Native resource release failed");
        }
        for (const auto& [type, count] : counts)
            if (std::int64_t(native.count(type)) != std::int64_t(count)) throw std::runtime_error("Native resource count differs from original map");
        const auto* strings = original.find(0x53545223, 1000);
        if (!strings) throw std::runtime_error("Original startup string table is missing");
        const auto length = strings->data.at(2);
        std::vector<std::uint8_t> expected(strings->data.begin() + 2, strings->data.begin() + 3 + length);
        if (native.string(1000, 1) != expected) throw std::runtime_error("Original Pascal string changed in native API");
        TempDirectory temporary;
        const auto preferences = temporary.path / "Preferences";
        native.create(preferences, 0x434d3031, 0x50524546);
        if (native.error() || !std::filesystem::is_regular_file(preferences))
            throw std::runtime_error("Native preference resource file creation failed");
        std::array<std::uint8_t, 32> finder{};
        if (getxattr(preferences.c_str(), "com.apple.FinderInfo", finder.data(), finder.size(), 0, 0) != 32 ||
            std::string(finder.begin(), finder.begin() + 8) != "PREFCM01")
            throw std::runtime_error("Native Finder type/creator byte order changed");
        const auto application_ref = native.current();
        const auto application_resource = native.get(0x76657273, 1, true);
        const auto ref = native.open(preferences, 0);
        if (ref == -1 || native.error() || native.current() != ref || native.count(0x76657273) != 0)
            throw std::runtime_error("Native preference resource fork open failed");
        if (!native.get(0x76657273, 1, false).loaded)
            throw std::runtime_error("Preference fork hid original application resources");
        const std::vector<std::uint8_t> payload{0x00, 0xff, 0x81, 0x45, 0x67, 0x89};
        const auto custom = native.add(0x54455354, 1000, {'T', 'e', 's', 't'}, {1, 2});
        if (!custom || native.error()) throw std::runtime_error("Adding a preference resource failed");
        native.write(custom, payload);
        if (native.error() || native.get(0x54455354, 1000, true).bytes != payload)
            throw std::runtime_error("Preference resource growth/write failed");
        const cm::Resource* endian_fixture = nullptr;
        for (const auto& resource : original.resources()) if (resource.type == 0x46524546) { endian_fixture = &resource; break; }
        if (!endian_fixture) throw std::runtime_error("Original FREF fixture is missing");
        const auto typed = native.add(0x46524546, 1000, {}, endian_fixture->data);
        if (!typed || native.error()) throw std::runtime_error("Adding a byte-swapped resource failed");
        native.write(typed, endian_fixture->data);
        if (native.error() || native.get(0x46524546, 1000, true).bytes != endian_fixture->data)
            throw std::runtime_error("Resource write converted byte order incorrectly");
        native.close(ref);
        if (native.error()) throw std::runtime_error("Native preference resource fork close failed");
        native.use(application_ref);
        if (native.get(0x76657273, 1, true).ticket != application_resource.ticket)
            throw std::runtime_error("Closing preference fork invalidated an application resource");
        const auto reopened = native.open(preferences, 3);
        if (reopened == -1 || native.error()) throw std::runtime_error("Created resource fork did not survive close/reopen");
        const auto reloaded = native.get(0x54455354, 1000, true);
        if (reloaded.bytes != payload) throw std::runtime_error("Written resource did not survive reopen");
        std::ifstream persisted_file(preferences / "..namedfork/rsrc", std::ios::binary);
        const std::vector<std::uint8_t> persisted_bytes((std::istreambuf_iterator<char>(persisted_file)), {});
        cm::ResourceFork persisted(persisted_bytes);
        const auto* persisted_typed = persisted.find(0x46524546, 1000);
        if (!persisted_typed || persisted_typed->data != endian_fixture->data)
            throw std::runtime_error("Persisted resource bytes differ from the original byte order");
        native.remove(reloaded.ticket);
        if (native.error()) throw std::runtime_error("Preference resource removal failed");
        native.close(reopened);
        const auto final_ref = native.open(preferences, 1);
        if (final_ref == -1 || native.get(0x54455354, 1000, true).ticket != 0)
            throw std::runtime_error("Resource removal did not survive close/reopen");
        const auto read_only = native.get(0x46524546, 1000, true);
        native.write(read_only.ticket, {1, 2, 3});
        if (native.error() != -54 || native.get(0x46524546, 1000, true).bytes != endian_fixture->data)
            throw std::runtime_error("Read-only resource write changed its cached bytes");
        native.remove(read_only.ticket);
        if (native.error() != -54 || native.get(0x46524546, 1000, true).ticket != read_only.ticket)
            throw std::runtime_error("Read-only resource removal detached its cached handle");
        if (native.add(0x54455354, 1001, {}, {1}) || native.error() != -54)
            throw std::runtime_error("Read-only resource file accepted an added resource");
        native.close(final_ref);
        std::cout << original.resources().size() << " native Resource Manager payloads match the original byte-for-byte\n";
        std::cout << "Native preference resource creation, metadata, chain, writes, byte order, removal, and reopening passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
