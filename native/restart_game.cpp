#include "restart_game.hpp"
#include "guest_abi.hpp"

#include <atomic>
#include <mach-o/dyld.h>
#include <spawn.h>
#include <string>
#include <vector>

extern char** environ;

namespace cm::aot {
namespace {
std::atomic<bool> requested{false};
}
bool restart_game(State& s, std::uint32_t pc) {
    requested.store(true);
    guest::Cpu c{s};
    c.set(3, 0);
    c.call(0x1076f7d0, pc + 4);   // exit(0): ends in ApplicationExit
    return true;
}
void reopen_after_restart() {
    if (!requested.exchange(false)) return;
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string path(size, '\0');
    if (_NSGetExecutablePath(path.data(), &size) != 0) return;
    path.resize(std::char_traits<char>::length(path.c_str()));
    // The bundle of .../Name.app/Contents/MacOS/name; a bare executable (checkers) is not reopened.
    const auto contents = path.rfind("/Contents/MacOS/");
    if (contents == std::string::npos) return;
    path.resize(contents);
    // A new instance, once this one is gone: the delay lets it release its files first.
    std::string script = "sleep 1; exec /usr/bin/open -n \"$0\"";
    std::vector<char*> arguments{const_cast<char*>("/bin/sh"), const_cast<char*>("-c"), script.data(), path.data(), nullptr};
    pid_t child = 0;
    posix_spawn(&child, "/bin/sh", nullptr, nullptr, arguments.data(), environ);
}
}
