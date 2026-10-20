#pragma once
#include <algorithm>
#include <string>

namespace cm::aot {
// A host (POSIX) name cannot contain '/'; a Mac/HFS name cannot contain ':'.
// macOS stores an HFS '/' as ':' on disk, as Carbon did. Convert at every
// boundary between a host path and a MacRoman or Carbon Unicode name.
inline std::string hfs_name(std::string host) {
    std::ranges::replace(host, ':', '/');
    return host;
}
inline std::string host_name(std::string hfs) {
    std::ranges::replace(hfs, '/', ':');
    return hfs;
}
}
