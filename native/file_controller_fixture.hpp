#pragma once
#include "aot_runtime.hpp"
#include <map>
#include <string>

namespace cm::file_controller_checks {
using namespace cm::aot;
// A dependency model for controller equivalence, not an OS implementation.
// The separately exercised native bridge owns real FSRefs, fork numbers and
// converter pointers; the controller sees explicit, stable guest API outputs.
struct FileModel {
    struct File { std::string name; std::uint64_t size; std::uint32_t type, creator; bool exists; bool operator==(const File&) const = default; };
    std::map<unsigned, File> files{{3, {"", 0, 0, 0, true}}, {7, {"live.dat", 8, 0x54455854, 0x43574945, true}}};
    std::map<std::string, std::int16_t> errors;
    std::uint64_t position = 0;
    unsigned opened = 7, next_context = 0x5b000000, pending_block = 0;
    std::int16_t pending_result = 0;
    bool pending_async = false;
    bool operator==(const FileModel&) const = default;
    static std::uint32_t arg(const State& s, unsigned n) { return std::uint32_t(s.r<4>((n + 3) * 4)); }
    static std::string pascal(const State& s, std::uint32_t p) {
        if (!p) return {};
        const auto size = s.memory.read<1>(p);
        std::string value;
        for (unsigned i = 0; i < size; ++i) value.push_back(char(s.memory.read<1>(p + 1 + i)));
        return value;
    }
    static std::string leaf(std::string name) {
        const auto last = name.find_last_of(':'); return last == std::string::npos ? name : name.substr(last + 1);
    }
    static std::string unicode(const State& s, std::uint32_t p, unsigned length) {
        std::string value; for (unsigned i = 0; i < length; ++i) value.push_back(char(s.memory.read<2>(p + i * 2))); return value;
    }
    static void unicode(State& s, std::uint32_t p, const std::string& value) {
        s.memory.write<2>(p, value.size()); for (unsigned i = 0; i < value.size(); ++i) s.memory.write<2>(p + 2 + i * 2, std::uint8_t(value[i]));
    }
    static void reference(State& s, std::uint32_t p, unsigned id) {
        s.memory.clear(p, 80); s.memory.write<4>(p, id);
    }
    static void spec(State& s, std::uint32_t p, const std::string& name) {
        s.memory.write<2>(p, 0xfffe); s.memory.write<4>(p + 2, 3); s.memory.write<1>(p + 6, name.size());
        s.memory.copy_in(p + 7, {name.begin(), name.end()});
    }
    unsigned find(const std::string& name) const {
        for (const auto& [id, file] : files) if (file.exists && file.name == leaf(name)) return id;
        return 0;
    }
    void complete_pending(State& s) {
        if (pending_block) { s.memory.write<2>(pending_block + 16, std::uint16_t(pending_result)); pending_block = 0; }
    }
    bool dispatch(State& s, std::string name) {
        if (!name.starts_with("CarbonLib::")) return false;
        name.erase(0, 11); const auto a = [&](unsigned n) { return arg(s, n); };
        std::int16_t result = errors.contains(name) ? errors.at(name) : 0;
        const bool injected = result != 0;
        if (name == "Gestalt") { if (!result) s.memory.write<4>(a(1), 0x1302); }
        else if (name == "HGetVol") { if (!result) { s.memory.write<2>(a(1), 0xfffe); s.memory.write<4>(a(2), 3); } }
        else if (name == "FSMakeFSSpec") {
            const auto filename = leaf(pascal(s, a(2))); spec(s, a(3), filename); if (!find(filename) && !result) result = -43;
        } else if (name == "FSpMakeFSRef") {
            const auto id = find(pascal(s, a(0) + 6)); if (!id && !result) result = -43; if (!result) reference(s, a(1), id);
        } else if (name == "FSMakeFSRefUnicode") {
            const auto id = find(unicode(s, a(2), a(1))); if (!id && !result) result = -43; if (!result) reference(s, a(4), id);
        } else if (name == "FSGetCatalogInfo") {
            const auto id = unsigned(s.memory.read<4>(a(0))); if (!files.contains(id) || !files[id].exists) result = -43;
            if (!result) {
                if (a(2)) {
                    if (a(1) & 4) s.memory.write<2>(a(2) + 2, 0xfffe);
                    if (a(1) & 16) s.memory.write<4>(a(2) + 8, id);
                }
                if (a(3)) unicode(s, a(3), files[id].name);
                if (a(4)) spec(s, a(4), files[id].name);
                if (a(5)) reference(s, a(5), 3);
            }
        } else if (name == "ResolveAliasFile") {
            if (!result) { s.memory.write<1>(a(2), 0); s.memory.write<1>(a(3), 0); }
        } else if (name == "UpgradeScriptInfoToTextEncoding") { if (!result) s.memory.write<4>(a(4), 0); }
        else if (name == "CreateTextToUnicodeInfoByEncoding") { if (!result) s.memory.write<4>(a(1), next_context++); }
        else if (name == "DisposeTextToUnicodeInfo") { if (!result) s.memory.write<4>(a(0), 0); }
        else if (name == "ConvertFromTextToUnicode") {
            if (!result) {
                const auto stack = std::uint32_t(s.r<4>(4));
                const auto out = std::uint32_t(s.memory.read<4>(stack + 68));
                for (unsigned i = 0; i < a(1); ++i) s.memory.write<2>(out + i * 2, s.memory.read<1>(a(2) + i));
                s.memory.write<4>(std::uint32_t(s.memory.read<4>(stack + 60)), a(1));
                s.memory.write<4>(std::uint32_t(s.memory.read<4>(stack + 64)), a(1) * 2);
            }
        } else if (name == "FSCreateFileUnicode") {
            const auto filename = unicode(s, a(2), a(1));
            if (find(filename)) result = -48;
            if (!result) {
                files[9] = {filename, 0, std::uint32_t(s.memory.read<4>(a(4) + 72)), std::uint32_t(s.memory.read<4>(a(4) + 76)), true};
                reference(s, a(5), 9); if (a(6)) spec(s, a(6), filename);
            }
        } else if (name == "FSOpenFork") {
            if (!result) { opened = unsigned(s.memory.read<4>(a(0))); position = 0; s.memory.write<2>(a(4), 121); }
        } else if (name == "FSSetForkSize") { if (!result) files[opened].size = (std::uint64_t(a(2)) << 32) | a(3); }
        else if (name == "FSCloseFork") {}
        else if (name == "FSDeleteObject") { if (!result) files[unsigned(s.memory.read<4>(a(0)))].exists = false; }
        else if (name == "FSCompareFSRefs") { if (!result && s.memory.read<4>(a(0)) != s.memory.read<4>(a(1))) result = -1401; }
        else if (name == "FSRenameUnicode") { if (!result) files[unsigned(s.memory.read<4>(a(0)))].name = unicode(s, a(2), a(1)); }
        else if (name.starts_with("PB")) {
            const auto pb = a(0);
            if (name == "PBHGetVInfoSync") {
                if (!result) s.memory.write<2>(pb + 22, 0xfffe);
            } else if (name == "PBHCreateSync") { if (!result) files[9] = {leaf(pascal(s, std::uint32_t(s.memory.read<4>(pb + 18)))), 0, 0, 0, true}; }
            else if (name == "PBHOpenDFSync") { if (!result) { opened = find(pascal(s, std::uint32_t(s.memory.read<4>(pb + 18)))); position = 0; s.memory.write<2>(pb + 24, 121); } }
            else if (name == "PBCloseSync") {}
            else if (name == "PBHDeleteSync") { if (!result) files[find(pascal(s, std::uint32_t(s.memory.read<4>(pb + 18))))].exists = false; }
            else if (name == "PBHRenameSync") {
                if (!result) files[find(pascal(s, std::uint32_t(s.memory.read<4>(pb + 18))))].name = pascal(s, std::uint32_t(s.memory.read<4>(pb + 28)));
            } else if (name == "PBGetCatInfoSync") {
                const auto id = find(pascal(s, std::uint32_t(s.memory.read<4>(pb + 18))));
                if (!id && !result) result = -43;
                if (!result) {
                    s.memory.clear(pb + 32, 16); s.memory.clear(pb + 72, 28);
                    s.memory.write<1>(pb + 30, id == 3 ? 0x10 : 0);
                    s.memory.write<4>(pb + 48, id); s.memory.write<4>(pb + 100, id == 3 ? 1 : 3);
                    s.memory.write<4>(pb + 32, files[id].type); s.memory.write<4>(pb + 36, files[id].creator);
                }
            } else if (name == "PBSetCatInfoSync") {
                if (!result) { const auto id = find(pascal(s, std::uint32_t(s.memory.read<4>(pb + 18)))); files[id].type = s.memory.read<4>(pb + 32); files[id].creator = s.memory.read<4>(pb + 36); }
            } else if (name.starts_with("PBGetForkSize") || name.starts_with("PBGetEOF")) {
                if (!result) { if (name.starts_with("PBGetForkSize")) s.memory.write<8>(pb + 46, files[opened].size); else s.memory.write<4>(pb + 28, files[opened].size); }
            } else if (name.starts_with("PBSetForkSize") || name.starts_with("PBSetEOF")) {
                if (!result) files[opened].size = name.starts_with("PBSetForkSize") ? s.memory.read<8>(pb + 46) : s.memory.read<4>(pb + 28);
            } else if (name.starts_with("PBSetForkPosition") || name.starts_with("PBSetFPos")) {
                if (!result) { position = name.starts_with("PBSetForkPosition") ? s.memory.read<8>(pb + 46) : s.memory.read<4>(pb + 46); if (name.starts_with("PBSetFPos")) s.memory.write<4>(pb + 46, position); }
            } else if (name.starts_with("PBRead") || name.starts_with("PBWrite")) {
                const auto count = std::uint32_t(s.memory.read<4>(pb + 36));
                const bool reading = name.starts_with("PBRead");
                const auto actual = injected ? 0u : reading ? unsigned(std::min(std::uint64_t(count), files[opened].size - std::min(position, files[opened].size))) : count;
                if (reading && actual) s.memory.copy_in(std::uint32_t(s.memory.read<4>(pb + 32)), std::vector<std::uint8_t>(actual, 'x'));
                position += actual; if (!reading) files[opened].size = std::max(files[opened].size, position);
                s.memory.write<4>(pb + 40, actual); if (name.find("Fork") == std::string::npos) s.memory.write<4>(pb + 46, position);
                if (!injected && reading && actual < count) result = -39;
            } else return false;
            if (pending_async && name.ends_with("Async")) {
                pending_block = pb; pending_result = result; s.memory.write<2>(pb + 16, 1);
            } else s.memory.write<2>(pb + 16, std::uint16_t(result));
            if (name.ends_with("Async")) {
                s.wr<4>(12, name.find("Fork") != std::string::npos ? pb : 0); s.pc = std::uint32_t(s.r<4>(4128)); return true;
            }
        } else return false;
        s.wr<4>(12, std::uint32_t(std::int32_t(result))); s.pc = std::uint32_t(s.r<4>(4128)); return true;
    }
};
}
