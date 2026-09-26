#include "resource_fork.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {
void require(bool b, const char* message) { if (!b) throw std::runtime_error(message); }
void put(std::vector<std::uint8_t>& b, unsigned pos, unsigned size, std::uint32_t v) {
    for (unsigned i = 0; i < size; ++i) b.at(pos + size - 1 - i) = (v >> (8 * i)) & 255;
}
std::vector<std::uint8_t> fixture() {
    std::vector<std::uint8_t> b(86);
    put(b, 0, 4, 16); put(b, 4, 4, 24); put(b, 8, 4, 8); put(b, 12, 4, 62);
    put(b, 16, 4, 4); put(b, 20, 4, 0x12345678);
    put(b, 48, 2, 28); put(b, 50, 2, 50); // map list offsets
    put(b, 52, 2, 0); put(b, 54, 4, 0x54455354); // one TEST type
    put(b, 58, 2, 0); put(b, 60, 2, 10); // one resource, ref at type+10
    put(b, 62, 2, 0xffff); put(b, 64, 2, 0xffff); // signed ID -1, unnamed
    return b;
}
}
int main(int argc, char** argv) {
    try {
        auto bytes = fixture();
        cm::ResourceFork valid(bytes);
        const auto* item = valid.find(0x54455354, -1);
        require(item && item->data == std::vector<std::uint8_t>({0x12, 0x34, 0x56, 0x78}), "Resource payload/ID");
        require(!valid.find(0x54455354, 1), "Unexpected resource ID");
        require(cm::ResourceFork(cm::encode_resource_fork(valid.resources())).resources() == valid.resources(),
                "Resource encoding round-trip");
        require(cm::ResourceFork(cm::encode_resource_fork({})).resources().empty(), "Empty resource file encoding");
        for (unsigned mode = 0; mode < 5; ++mode) {
            auto broken = bytes;
            if (mode == 0) broken.resize(12);
            if (mode == 1) put(broken, 16, 4, 0xffffffff);
            if (mode == 2) put(broken, 48, 2, 0xffff);
            if (mode == 3) put(broken, 4, 4, 18);
            if (mode == 4) put(broken, 64, 2, 0xfffe);
            bool rejected = false;
            try { cm::ResourceFork invalid(broken); }
            catch (const std::runtime_error&) { rejected = true; }
            require(rejected, "Damaged resource fork accepted");
        }
        std::cout << "Resource parser checks passed\n";
        if (argc == 2) {
            std::ifstream stream(argv[1], std::ios::binary);
            if (!stream) throw std::runtime_error("Cannot open original resource fork");
            std::vector<std::uint8_t> original((std::istreambuf_iterator<char>(stream)), {});
            cm::ResourceFork fork(original);
            require(fork.find(0x4d454e55, 100), "Missing original MENU 100");
            require(fork.find(0x444c4f47, 128), "Missing original DLOG 128");
            require(cm::ResourceFork(cm::encode_resource_fork(fork.resources())).resources() == fork.resources(),
                    "Original resource payloads changed during serialization");
            std::cout << "Read " << fork.resources().size() << " resources from the actual Mac executable\n";
        }
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
