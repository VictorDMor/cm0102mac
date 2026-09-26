#include "probe_host.hpp"
#import <AppKit/AppKit.h>
#include <filesystem>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <unistd.h>

int main() {
    @autoreleasepool {
        std::filesystem::path run_directory;
        int result = 1;
        try {
            const std::filesystem::path bundle(NSBundle.mainBundle.bundlePath.fileSystemRepresentation);
            const auto local = bundle.parent_path().parent_path();
            const std::filesystem::path support([[NSFileManager.defaultManager URLsForDirectory:NSApplicationSupportDirectory
                inDomains:NSUserDomainMask] firstObject].fileSystemRepresentation);
            const auto installation = support / "CM 01-02 Native";
            auto program = bundle / "Contents/Resources/OriginalProgram";
            if (!std::filesystem::is_regular_file(program / "program.json")) program = local / "recomp-input";
            const auto game = installation / "Game";
            if (!std::filesystem::is_regular_file(program / "program.json"))
                throw std::runtime_error("Original program data is missing. Run Install Native Prototype.command again.");
            if (!std::filesystem::is_regular_file(game / "CM 01-02"))
                throw std::runtime_error("Game data is not installed. Run Install Native Prototype.command once to copy your game files.");
            std::filesystem::create_directories(installation / "Runs");
            auto pattern = (installation / "Runs" / "app.XXXXXX").string();
            if (!mkdtemp(pattern.data())) throw std::runtime_error("Cannot create the prototype's diagnostic folder.");
            run_directory = pattern;
            if (!std::freopen((run_directory / "startup.log").c_str(), "w", stderr) ||
                dup2(STDERR_FILENO, STDOUT_FILENO) < 0)
                throw std::runtime_error("Cannot write the prototype's diagnostic log.");
            std::setvbuf(stderr, nullptr, _IONBF, 0);
            std::setvbuf(stdout, nullptr, _IOLBF, 0);
            setenv("CM_AOT_INSTRUCTION_BUDGET", "unlimited", 0);
            setenv("CM_AOT_TRACE_AUDIO", "1", 0);
            setenv("CM_AOT_TRACE_FILES", "1", 0);
            setenv("CM_AOT_TRACE_INPUT", "1", 0);
            setenv("CM_AOT_CAPTURE_DIRECTORY", (run_directory / "images").c_str(), 0);
            std::fprintf(stderr, "Native ARM64 development application; original data: %s\nInstalled game files: %s\n", program.c_str(), game.c_str());
            std::array<std::string, 5> arguments{"cm-native-prototype", program.string(), "0x10773710", game.string(), (installation / "Runtime").string()};
            std::array<char*, 5> argv;
            for (unsigned i = 0; i < argv.size(); ++i) argv[i] = arguments[i].data();
            [NSApplication sharedApplication];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            if (@available(macOS 14.0, *)) [NSApp activate];
            result = cm::aot::run_probe(argv.size(), argv.data());
            if (!result) return 0;
        } catch (const std::exception& error) {
            std::fprintf(stderr, "%s\n", error.what());
            [NSApplication sharedApplication];
            NSAlert* alert = [NSAlert new];
            alert.messageText = @"Cannot start the native prototype";
            alert.informativeText = [NSString stringWithUTF8String:error.what()];
            [alert runModal];
            return 1;
        }
        NSAlert* alert = [NSAlert new];
        alert.messageText = @"The native prototype stopped";
        alert.informativeText = [NSString stringWithFormat:@"This development version has reached a part that still needs work. Diagnostic files were saved in:\n%s", run_directory.c_str()];
        [alert runModal];
        return result;
    }
}
