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
            auto runtime = installation / "Runtime";
            auto runs = installation / "Runs";
            id validation = [NSBundle.mainBundle objectForInfoDictionaryKey:@"CMValidationRuntime"];
            if (validation) {
                if (![validation isKindOfClass:NSString.class] ||
                    ![NSBundle.mainBundle.bundleIdentifier hasPrefix:@"local.cm0102.validation."])
                    throw std::runtime_error("Validation requires a separate diagnostic app bundle.");
                const std::filesystem::path requested([(NSString*)validation fileSystemRepresentation]);
                if (!requested.is_absolute() || !std::filesystem::is_directory(requested))
                    throw std::runtime_error("Validation requires an existing absolute runtime directory.");
                runtime = std::filesystem::canonical(requested);
                const auto original = std::filesystem::canonical(installation);
                const auto relative = runtime.lexically_relative(original);
                if (relative.empty() || *relative.begin() != "..")
                    throw std::runtime_error("Validation must use a private copy outside the installed game directory.");
                runs = runtime.parent_path() / "Runs";
                id event_pointer = [NSBundle.mainBundle objectForInfoDictionaryKey:@"CMValidationEventPointer"];
                if (event_pointer && ![event_pointer isKindOfClass:NSNumber.class])
                    throw std::runtime_error("Validation pointer selection requires a Boolean.");
                setenv("CM_AOT_VALIDATION_EVENT_POINTER", !event_pointer || [event_pointer boolValue] ? "1" : "0", 1);
                setenv("CM_AOT_PERFORMANCE_TRACE", "1", 1);
                id reference = [NSBundle.mainBundle objectForInfoDictionaryKey:@"CMSeasonReferenceDirectory"];
                if (reference) {
                    if (![reference isKindOfClass:NSString.class])
                        throw std::runtime_error("Season reference requires a private directory string.");
                    const std::filesystem::path fixture([(NSString*)reference fileSystemRepresentation]);
                    if (!fixture.is_absolute() || !std::filesystem::is_directory(fixture) ||
                        !std::filesystem::is_empty(fixture))
                        throw std::runtime_error("Season reference requires a new empty private directory.");
                    const auto relative_fixture = std::filesystem::canonical(fixture).lexically_relative(original);
                    if (relative_fixture.empty() || *relative_fixture.begin() != "..")
                        throw std::runtime_error("Season fixtures must stay outside the installed game directory.");
                    setenv("CM_AOT_SEASON_REFERENCE", fixture.c_str(), 1);
                    id days = [NSBundle.mainBundle objectForInfoDictionaryKey:@"CMSeasonReferenceDays"];
                    if (days) {
                        if (![days isKindOfClass:NSNumber.class] || [days intValue] < 1 || [days intValue] > 7)
                            throw std::runtime_error("Season reference requires 1 through 7 full days.");
                        setenv("CM_AOT_SEASON_REFERENCE_DAYS", [[days stringValue] UTF8String], 1);
                    }
                }
            }
            auto program = bundle / "Contents/Resources/OriginalProgram";
            if (!std::filesystem::is_regular_file(program / "program.json")) program = local / "recomp-input";
            const auto game = installation / "Game";
            if (!std::filesystem::is_regular_file(program / "program.json"))
                throw std::runtime_error("Original program data is missing. Run Install Native Prototype.command again.");
            if (!std::filesystem::is_regular_file(game / "CM 01-02"))
                throw std::runtime_error("Game data is not installed. Run Install Native Prototype.command once to copy your game files.");
            std::filesystem::create_directories(runs);
            auto pattern = (runs / "app.XXXXXX").string();
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
            if (validation) std::fprintf(stderr, "Isolated validation runtime: %s; event-coordinate pointer %s\n", runtime.c_str(),
                std::string(std::getenv("CM_AOT_VALIDATION_EVENT_POINTER")) == "1" ? "enabled" : "disabled (production pointer)");
            std::array<std::string, 5> arguments{"cm-native-prototype", program.string(), "0x10773710", game.string(), runtime.string()};
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
