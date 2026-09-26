#include "aot_integrity.hpp"
#include <CommonCrypto/CommonDigest.h>
#include <fstream>

namespace cm::aot {
void load_verified(Memory& memory, std::uint32_t address,
                   const std::filesystem::path& file, std::uint64_t size,
                   const char* expected_sha256) {
    if (size > UINT32_MAX || std::filesystem::file_size(file) != size)
        throw std::runtime_error("Wrong program segment size: " + file.filename().string());
    std::vector<std::uint8_t> bytes(size);
    std::ifstream stream(file, std::ios::binary);
    if (!stream.read(reinterpret_cast<char*>(bytes.data()), std::streamsize(size)))
        throw std::runtime_error("Cannot read original program segment");
    std::array<unsigned char, CC_SHA256_DIGEST_LENGTH> digest;
    CC_SHA256(bytes.data(), CC_LONG(size), digest.data());
    constexpr char hex[] = "0123456789abcdef";
    std::string actual;
    for (auto b : digest) { actual += hex[b >> 4]; actual += hex[b & 15]; }
    if (actual != expected_sha256)
        throw std::runtime_error("Program segment differs from AOT build input: " + file.filename().string());
    memory.map(address, size);
    memory.copy_in(address, bytes);
}
}
