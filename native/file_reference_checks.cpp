#include "aot_runtime.hpp"
#include "platform.hpp"
#include "stream_io_adapter.hpp"
#include <CoreServices/CoreServices.h>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>

#pragma clang diagnostic ignored "-Wdeprecated-declarations"
using namespace cm::aot;
namespace fs = std::filesystem;
namespace {
constexpr std::uint32_t fixture = 0x30000000, spec = fixture + 0x100, text = fixture + 0x200,
    catalog = fixture + 0x400, name_result = fixture + 0x600, context_pointer = fixture + 0x900,
    counts = fixture + 0xa00, input_offsets = fixture + 0xb00, output_offsets = fixture + 0xc00,
    first_ref = fixture + 0x1000, second_ref = fixture + 0x1100, parent_ref = fixture + 0x1200,
    output_ref = fixture + 0x1300, output = fixture + 0x2000, pb = fixture + 0x3000,
    mode_pointer = fixture + 0x4000, toc = 0x108bba90;
unsigned checks = 0, generated_calls = 0;
std::map<std::string, unsigned> direct_calls;
void require(bool ok, const std::string& message) { ++checks; if (!ok) throw std::runtime_error(message); }
struct Temporary {
    fs::path path;
    Temporary() {
        auto pattern = (fs::temp_directory_path() / "cm-file-reference-XXXXXX").string();
        if (!mkdtemp(pattern.data())) throw std::runtime_error("Cannot create private file-reference fixture");
        path = pattern;
    }
    ~Temporary() { std::error_code error; fs::remove_all(path, error); }
};
std::string bytes(const fs::path& path) {
    std::ifstream stream(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
void pascal_string(State& s, const std::string& value) {
    s.memory.write<1>(text, value.size()); s.memory.copy_in(text + 1, {value.begin(), value.end()});
}
void cstring(State& s, std::uint32_t at, const std::string& value) {
    std::vector<std::uint8_t> data(value.begin(), value.end()); data.push_back(0); s.memory.copy_in(at, data);
}
void unicode(State& s, const std::u16string& value) {
    for (unsigned n = 0; n < value.size(); ++n) s.memory.write<2>(text + n * 2, value[n]);
}
std::int32_t call(State& s, const char* name, std::initializer_list<std::uint32_t> arguments) {
    unsigned n = 3; for (auto value : arguments) s.wr<4>(n++ * 4, value);
    s.wr<4>(4128, return_sentinel); s.pc = 0x1234;
    const auto registers = s.registers;
    const auto unique = s.unique;
    require(s.platform->dispatch(s, name), std::string(name) + " unhandled");
    auto expected = registers; write_be<4>(expected.data() + 12, s.r<4>(12));
    require(s.pc == return_sentinel && s.registers == expected && s.unique == unique,
            std::string(name) + " native return ABI");
    ++direct_calls[name];
    return std::bit_cast<std::int32_t>(std::uint32_t(s.r<4>(12)));
}
void lookup(State& s, const std::string& name, bool game, std::uint32_t result) {
    pascal_string(s, name);
    require(!call(s, "CarbonLib::FSMakeFSSpec", {game ? 0xffffffffu : 0xfffffffeu, game ? 2u : 3u, text, spec}), "Classic lookup");
    s.memory.copy_in(result - 4, std::vector<std::uint8_t>(88, 0xa5));
    require(!call(s, "CarbonLib::FSpMakeFSRef", {spec, result}), "FSSpec to real native identity");
    require(s.memory.read<4>(result - 4) == 0xa5a5a5a5 && s.memory.read<4>(result + 80) == 0xa5a5a5a5, "80-byte FSRef boundary");
}
struct Imports : StreamIoBoundaryObserver {
    std::map<std::string, unsigned> calls;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State&, const char* name) override { ++calls[name]; }
};
std::uint32_t generated(State& s, std::uint32_t entry, std::initializer_list<std::uint32_t> arguments) {
    unsigned n = 3; for (auto value : arguments) s.wr<4>(n++ * 4, value);
    s.wr<4>(4, 0x700fff00); s.wr<4>(8, toc); s.wr<4>(4128, return_sentinel);
    s.pc = entry; s.budget = s.executed + 1000000; run(s);
    require(s.pc == return_sentinel, "Original file controller return"); ++generated_calls;
    return std::uint32_t(s.r<4>(12));
}
void convert_case(State& s, std::uint32_t encoding, const std::vector<std::uint8_t>& source,
                  unsigned capacity, const std::vector<std::uint32_t>& offsets = {}) {
    TextToUnicodeInfo native = nullptr;
    const auto created = ::CreateTextToUnicodeInfoByEncoding(encoding, &native);
    require(!created, "Native conversion fixture context encoding=" + std::to_string(encoding) + " error=" + std::to_string(created));
    require(!call(s, "CarbonLib::CreateTextToUnicodeInfoByEncoding", {encoding, context_pointer}), "Guest conversion context");
    const auto ticket = std::uint32_t(s.memory.read<4>(context_pointer));
    std::vector<UniChar> expected((capacity + 1) / 2 + 1);
    ByteCount source_read = 0, output_bytes = 0; ItemCount offset_count = 0;
    std::vector<ByteOffset> native_offsets(offsets.begin(), offsets.end()), expected_offsets(offsets.size());
    std::uint8_t empty = 0;
    const auto error = ::ConvertFromTextToUnicode(native, source.size(), source.empty() ? &empty : source.data(),
        0, offsets.size(), offsets.empty() ? nullptr : native_offsets.data(), &offset_count,
        offsets.empty() ? nullptr : expected_offsets.data(), capacity, &source_read, &output_bytes, expected.data());
    require(!::DisposeTextToUnicodeInfo(&native), "Native fixture disposal");
    s.memory.copy_in(text, source); s.memory.copy_in(output - 4, std::vector<std::uint8_t>(capacity + 12, 0xa5));
    for (unsigned n = 0; n < offsets.size(); ++n) s.memory.write<4>(input_offsets + n * 4, offsets[n]);
    s.memory.copy_in(output_offsets - 4, std::vector<std::uint8_t>(offsets.size() * 4 + 8, 0xa5));
    s.memory.write<4>(0x700fff00 + 56, capacity); s.memory.write<4>(0x700fff00 + 60, counts);
    s.memory.write<4>(0x700fff00 + 64, counts + 4); s.memory.write<4>(0x700fff00 + 68, output);
    require(call(s, "CarbonLib::ConvertFromTextToUnicode", {ticket, std::uint32_t(source.size()), text, 0,
        std::uint32_t(offsets.size()), offsets.empty() ? 0u : input_offsets, counts + 8, offsets.empty() ? 0u : output_offsets}) == error,
        "Native converter result differed from the direct SDK call");
    require(s.memory.read<4>(counts) == source_read && s.memory.read<4>(counts + 4) == output_bytes &&
            s.memory.read<4>(counts + 8) == offset_count, "Word32 native conversion counts");
    for (unsigned n = 0; n < output_bytes / 2; ++n) require(s.memory.read<2>(output + n * 2) == expected[n], "Big-endian UTF16 unit");
    for (unsigned n = 0; n < offset_count; ++n) require(s.memory.read<4>(output_offsets + n * 4) == expected_offsets[n], "Word32 conversion offset map");
    require(s.memory.read<4>(output - 4) == 0xa5a5a5a5 && s.memory.read<4>(output + output_bytes) == 0xa5a5a5a5 &&
            s.memory.read<4>(output_offsets - 4) == 0xa5a5a5a5 && s.memory.read<4>(output_offsets + offsets.size() * 4) == 0xa5a5a5a5,
            "Conversion wrote beyond the completed units/offsets");
    require(!call(s, "CarbonLib::DisposeTextToUnicodeInfo", {context_pointer}) && !s.memory.read<4>(context_pointer), "Context disposal cleared its ticket");
    require(call(s, "CarbonLib::ConvertFromTextToUnicode", {ticket}) == paramErr, "Disposed context remained live");
    require(call(s, "CarbonLib::DisposeTextToUnicodeInfo", {context_pointer}) == paramErr, "Double context disposal accepted");
}
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    try {
        Temporary temporary;
        const auto game = temporary.path / "Game", user = temporary.path / "User";
        fs::create_directories(game); fs::create_directories(user);
        std::ofstream(game / "media.dat") << "original";
        std::ofstream(user / "small.dat") << "ABCDEFGH";
        std::ofstream(user / "other.dat") << "other";
        std::ofstream(user / "sparse.dat") << "seed";
        fs::resize_file(user / "sparse.dat", (UINT64_C(1) << 32) + 19);
        fs::create_directories(user / "Folder");
        auto state = std::make_unique<State>(); auto& s = *state;
        load_program(s, argv[1]); s.memory.map(fixture, 0x10000);
        s.wr<4>(4, 0x700fff00); s.wr<4>(8, toc);
        s.platform = std::make_shared<NativePlatform>(game, user, AudioMode::offline);
        TextEncoding encoding = 0;
        const auto upgrade_error = ::UpgradeScriptInfoToTextEncoding(-1, -128, -128, nullptr, &encoding);
        require(call(s, "CarbonLib::UpgradeScriptInfoToTextEncoding", {0xffffffff, 0xffffff80, 0xffffff80, 0, counts}) == upgrade_error,
                "System-script encoding upgrade result");
        if (!upgrade_error) require(s.memory.read<4>(counts) == encoding, "Native encoding ID");
        require(call(s, "CarbonLib::UpgradeScriptInfoToTextEncoding", {0x7fff, 0, 0, 0, counts}) != 0, "Invalid script accepted");
        const auto utf8 = ::CreateTextEncoding(kTextEncodingUnicodeDefault, kTextEncodingDefaultVariant, kUnicodeUTF8Format);
        convert_case(s, kTextEncodingMacRoman, {'C','a','f',0x8e,' ',0xde}, 64, {0, 1, 3, 5});
        convert_case(s, kTextEncodingShiftJIS, {'A',0x82,0xa0,'B'}, 64);
        convert_case(s, kTextEncodingShiftJIS, {'A',0x82}, 64);
        convert_case(s, kTextEncodingMacRoman, {'A','B','C'}, 3);
        convert_case(s, kTextEncodingMacRoman, {}, 0);
        // TextToUnicode is the vendor's legacy text converter. Its encoding
        // acceptance must remain native, including an unsupported UTF8 input.
        TextToUnicodeInfo unsupported_context = nullptr;
        const auto unsupported = ::CreateTextToUnicodeInfoByEncoding(utf8, &unsupported_context);
        if (unsupported) {
            s.memory.write<4>(context_pointer, 0xabcdef01);
            require(call(s, "CarbonLib::CreateTextToUnicodeInfoByEncoding", {utf8, context_pointer}) == unsupported &&
                    s.memory.read<4>(context_pointer) == 0xabcdef01, "Unsupported encoding result or output changed");
        } else {
            require(!::DisposeTextToUnicodeInfo(&unsupported_context), "Native UTF8 fixture disposal");
            convert_case(s, utf8, {'A',0xf0,0x9f,0x98,0x80,'B'}, 64);
        }
        s.memory.write<4>(context_pointer, 0xabcdef01);
        require(call(s, "CarbonLib::CreateTextToUnicodeInfoByEncoding", {0xfffffffe, context_pointer}) != 0 &&
            s.memory.read<4>(context_pointer) == 0xabcdef01, "Invalid encoding published a ticket");
        require(!call(s, "CarbonLib::CreateTextToUnicodeInfoByEncoding", {kTextEncodingMacRoman, context_pointer}), "Extent fixture context");
        s.memory.write<4>(0x700fff00 + 56, 32); s.memory.write<4>(0x700fff00 + 68, 0xfffffff0);
        bool fault = false;
        try { call(s, "CarbonLib::ConvertFromTextToUnicode", {std::uint32_t(s.memory.read<4>(context_pointer)), 0, text, 0, 0, 0, 0, 0}); }
        catch (const std::runtime_error& e) { fault = std::string(e.what()).starts_with("Unicode conversion exceeds word32"); }
        require(fault, "Wrapping output extent accepted");
        require(!call(s, "CarbonLib::DisposeTextToUnicodeInfo", {context_pointer}), "Extent fixture disposal");

        lookup(s, "", false, parent_ref); lookup(s, "small.dat", false, first_ref);
        lookup(s, "other.dat", false, second_ref);
        require(!call(s, "CarbonLib::Gestalt", {0x66732020, counts}) && s.memory.read<4>(counts) == 0x1302,
                "Production file capability did not expose the accepted fork family");
        require(!call(s, "CarbonLib::FSCompareFSRefs", {first_ref, first_ref}), "Same native identity differed");
        require(call(s, "CarbonLib::FSCompareFSRefs", {first_ref, second_ref}) == errFSRefsDifferent, "Distinct native identities compared equal");
        s.memory.copy_in(output_ref, std::vector<std::uint8_t>(80, 0));
        for (const char* operation : {"CarbonLib::FSCompareFSRefs", "CarbonLib::FSGetCatalogInfo", "CarbonLib::FSOpenFork", "CarbonLib::FSDeleteObject"})
            require(call(s, operation, {output_ref, 0, 0, 0, 0, 0}) != 0, "Invalid FSRef accepted by " + std::string(operation));
        const auto unchanged = s.memory.copy_out(output_ref, 80);
        unicode(s, u"missing.dat");
        require(call(s, "CarbonLib::FSMakeFSRefUnicode", {parent_ref, 11, text, kTextEncodingUnknown, output_ref}) == fnfErr &&
            s.memory.copy_out(output_ref, 80) == unchanged, "Missing Unicode lookup published an identity");
        unicode(s, u"SMALL.DAT");
        require(!call(s, "CarbonLib::FSMakeFSRefUnicode", {parent_ref, 9, text, kTextEncodingUnknown, output_ref}) &&
            !call(s, "CarbonLib::FSCompareFSRefs", {first_ref, output_ref}), "Unicode lookup lost the Classic case policy");
        require(call(s, "CarbonLib::FSCreateFileUnicode", {parent_ref, 9, text, 0, 0, output_ref, 0}) == dupFNErr, "Unicode creation replaced an existing entry");
        require(call(s, "CarbonLib::FSCreateFileUnicode", {parent_ref, 0, 0, 0, 0, output_ref, 0}) == bdNamErr,
                "Empty Unicode filename accepted");
        const std::u16string supplementary = u"emoji\U0001f600.dat";
        unicode(s, supplementary);
        require(!call(s, "CarbonLib::FSCreateFileUnicode", {parent_ref, std::uint32_t(supplementary.size()), text, 0, 0, output_ref, 0}),
                "Supplementary Unicode filename creation");
        require(!call(s, "CarbonLib::FSGetCatalogInfo", {output_ref, 0, 0, name_result, 0, 0}) &&
                s.memory.read<2>(name_result) == supplementary.size(), "Supplementary native filename units");
        for (unsigned n = 0; n < supplementary.size(); ++n)
            require(s.memory.read<2>(name_result + 2 + n * 2) == supplementary[n], "Supplementary UTF16 unit order");
        require(!call(s, "CarbonLib::FSDeleteObject", {output_ref}), "Supplementary file removal");
        s.memory.copy_in(catalog - 4, std::vector<std::uint8_t>(152, 0xa5));
        require(!call(s, "CarbonLib::FSGetCatalogInfo", {first_ref, kFSCatInfoDataSizes, catalog, 0, 0, 0}), "Selective native catalog query");
        require(s.memory.read<8>(catalog + 104) == 8 && s.memory.read<4>(catalog) == 0xa5a5a5a5 &&
                s.memory.read<4>(catalog + 100) == 0xa5a5a5a5 && s.memory.read<4>(catalog + 120) == 0xa5a5a5a5 &&
                s.memory.read<4>(catalog + 144) == 0xa5a5a5a5, "Catalog selection or 144-byte boundaries");
        auto minimal = std::make_unique<State>(); minimal->platform = s.platform;
        minimal->memory.map(first_ref, 80); minimal->memory.copy_in(first_ref, s.memory.copy_out(first_ref, 80));
        minimal->memory.map(catalog + 104, 16);
        require(!call(*minimal, "CarbonLib::FSGetCatalogInfo", {first_ref, kFSCatInfoDataSizes, catalog, 0, 0, 0}) &&
                minimal->memory.read<8>(catalog + 104) == 8, "Catalog accessed an unselected unmapped field");
        bool catalog_fault = false;
        try { call(s, "CarbonLib::FSGetCatalogInfo", {first_ref, kFSCatInfoDataSizes, 0xffffffa0, 0, 0, 0}); }
        catch (const std::runtime_error& e) { catalog_fault = std::string(e.what()).starts_with("Catalog field exceeds word32"); }
        require(catalog_fault, "Catalog output wrapped into low guest memory");
        require(call(s, "CarbonLib::FSGetCatalogInfo", {first_ref, kFSCatInfoDataSizes, 0, 0, 0, 0}) == paramErr, "Null selected catalog accepted");
        require(call(s, "CarbonLib::FSGetCatalogInfo", {first_ref, kFSCatInfoFSFileSecurityRef, catalog, 0, 0, 0}) == paramErr,
                "Host security pointer was exposed without an owned-object bridge");
        require(!call(s, "CarbonLib::FSGetCatalogInfo", {first_ref, kFSCatInfoPermissions | kFSCatInfoVolume | kFSCatInfoParentDirID | kFSCatInfoNodeID,
            catalog, name_result, spec, output_ref}), "Native catalog outputs and guest identities");
        struct stat info{}; require(!stat((user / "small.dat").c_str(), &info), "Independent file metadata");
        require(s.memory.read<2>(catalog + 2) == 0xfffe && s.memory.read<4>(catalog + 4) == 3 && s.memory.read<4>(catalog + 8) >= 16 &&
            s.memory.read<4>(catalog + 56) == info.st_uid && s.memory.read<4>(catalog + 60) == info.st_gid &&
            (s.memory.read<2>(catalog + 66) & 07777) == (info.st_mode & 07777) && !s.memory.read<4>(catalog + 68), "Catalog word widths, permissions, or host pointer leak");
        require(!call(s, "CarbonLib::FSCompareFSRefs", {parent_ref, output_ref}), "Catalog parent identity");
        require(s.memory.read<2>(spec) == 0xfffe && s.memory.read<4>(spec + 2) == 3 && s.memory.read<1>(spec + 6) == 9, "Catalog FSSpec namespace");
        FSRef native_ref{}; FSCatalogInfo native_info{};
        require(!::FSPathMakeRef(reinterpret_cast<const UInt8*>((user / "small.dat").c_str()), &native_ref, nullptr), "Independent native file identity");
        constexpr auto dates = kFSCatInfoCreateDate | kFSCatInfoContentMod | kFSCatInfoAttrMod | kFSCatInfoAccessDate | kFSCatInfoBackupDate;
        require(!::FSGetCatalogInfo(&native_ref, dates | kFSCatInfoPermissions, &native_info, nullptr, nullptr, nullptr) &&
                !call(s, "CarbonLib::FSGetCatalogInfo", {first_ref, dates | kFSCatInfoPermissions, catalog, 0, 0, 0}), "Independent catalog dates");
        const std::array date_values{native_info.createDate, native_info.contentModDate, native_info.attributeModDate, native_info.accessDate, native_info.backupDate};
        for (unsigned n = 0; n < date_values.size(); ++n) {
            const auto& value = date_values[n];
            require(s.memory.read<2>(catalog + 16 + n * 8) == value.highSeconds && s.memory.read<4>(catalog + 18 + n * 8) == value.lowSeconds &&
                    s.memory.read<2>(catalog + 22 + n * 8) == value.fraction, "Packed two-byte-aligned UTCDateTime");
        }
        require(s.memory.read<2>(catalog + 66) == native_info.permissions.mode, "Exact native permission mode");
        if (native_info.permissions.fileSec) CFRelease(native_info.permissions.fileSec);
        // Ref injection cannot bypass the two configured virtual roots.
        const auto outside = temporary.path / "Outside"; fs::create_directories(outside); std::ofstream(outside / "keep.dat") << "keep";
        require(!::FSPathMakeRef(reinterpret_cast<const UInt8*>((outside / "keep.dat").c_str()), &native_ref, nullptr), "Outside identity fixture");
        s.memory.copy_in(output_ref, {reinterpret_cast<std::uint8_t*>(&native_ref), reinterpret_cast<std::uint8_t*>(&native_ref) + sizeof(native_ref)});
        require(call(s, "CarbonLib::FSOpenFork", {output_ref, 0, 0, 3, counts}) == dirNFErr &&
                call(s, "CarbonLib::FSDeleteObject", {output_ref}) == dirNFErr && bytes(outside / "keep.dat") == "keep", "Injected FSRef escaped configured roots");
        lookup(s, "sparse.dat", false, second_ref);
        require(!call(s, "CarbonLib::FSGetCatalogInfo", {second_ref, kFSCatInfoDataSizes, catalog, 0, 0, 0}) &&
            s.memory.read<8>(catalog + 104) == (UINT64_C(1) << 32) + 19, "Catalog lost the file-size high word");

        unicode(s, u"Caf\u00e9.dat"); s.memory.clear(catalog, 144);
        s.memory.write<4>(catalog + 72, 0x54455854); s.memory.write<4>(catalog + 76, 0x43574945);
        s.memory.write<2>(catalog + 80, 0x0400); s.memory.write<2>(catalog + 82, 12); s.memory.write<2>(catalog + 84, 34);
        s.memory.write<4>(catalog + 140, kTextEncodingMacRoman);
        require(!call(s, "CarbonLib::FSCreateFileUnicode", {parent_ref, 8, text, kFSCatInfoFinderInfo | kFSCatInfoTextEncoding, catalog, second_ref, 0}), "Real Unicode file creation");
        require(!call(s, "CarbonLib::FSGetCatalogInfo", {second_ref, kFSCatInfoFinderInfo, catalog, name_result, 0, 0}) &&
            s.memory.read<4>(catalog + 72) == 0x54455854 && s.memory.read<4>(catalog + 76) == 0x43574945 &&
            s.memory.read<2>(catalog + 80) == 0x0400 && s.memory.read<2>(catalog + 82) == 12 && s.memory.read<2>(catalog + 84) == 34,
            "Finder metadata PowerPC/native conversion");
        const auto accented = user / "Caf\xc3\xa9.dat";
        std::array<std::uint8_t, 32> finder{};
        require(getxattr(accented.c_str(), "com.apple.FinderInfo", finder.data(), finder.size(), 0, 0) == 32 &&
            read_be<4>(finder.data()) == 0x54455854 && read_be<4>(finder.data() + 4) == 0x43574945 &&
            read_be<2>(finder.data() + 8) == 0x0400, "Independent FinderInfo persistence");
        write_be<2>(finder.data() + 16, 0x1122); write_be<2>(finder.data() + 24, 0x3344); write_be<4>(finder.data() + 28, 0x55667788);
        require(!setxattr(accented.c_str(), "com.apple.FinderInfo", finder.data(), finder.size(), 0, 0), "Extended Finder fixture");
        require(!call(s, "CarbonLib::FSGetCatalogInfo", {second_ref, kFSCatInfoFinderXInfo, catalog, 0, 0, 0}) &&
                s.memory.read<2>(catalog + 88) == 0x1122 && s.memory.read<2>(catalog + 96) == 0x3344 &&
                s.memory.read<4>(catalog + 100) == 0x55667788, "Extended FileInfo endian fields");
        lookup(s, "Folder", false, output_ref); finder.fill(0);
        for (unsigned n = 0; n < 4; ++n) write_be<2>(finder.data() + n * 2, n + 1);
        write_be<2>(finder.data() + 16, 5); write_be<2>(finder.data() + 18, 6); write_be<4>(finder.data() + 20, 0x11223344);
        require(!setxattr((user / "Folder").c_str(), "com.apple.FinderInfo", finder.data(), finder.size(), 0, 0), "Folder Finder fixture");
        require(!call(s, "CarbonLib::FSGetCatalogInfo", {output_ref, kFSCatInfoFinderInfo | kFSCatInfoFinderXInfo, catalog, 0, 0, 0}) &&
                s.memory.read<2>(catalog + 72) == 1 && s.memory.read<2>(catalog + 78) == 4 && s.memory.read<2>(catalog + 88) == 5 &&
                s.memory.read<2>(catalog + 90) == 6 && s.memory.read<4>(catalog + 92) == 0x11223344, "FolderInfo and ExtendedFolderInfo field widths");
        unicode(s, u"Caf\u00e9 renamed.dat");
        require(!call(s, "CarbonLib::FSRenameUnicode", {second_ref, 16, text, kTextEncodingMacRoman, output_ref}) &&
            !call(s, "CarbonLib::FSCompareFSRefs", {second_ref, output_ref}), "Rename broke the old native identity");
        unicode(s, u"small.dat");
        require(call(s, "CarbonLib::FSRenameUnicode", {second_ref, 9, text, kTextEncodingUnknown, 0}) == dupFNErr, "Rename replaced an existing file");
        require(!call(s, "CarbonLib::FSDeleteObject", {output_ref}) && call(s, "CarbonLib::FSDeleteObject", {output_ref}) != 0,
            "Delete retained a live identity or accepted a second deletion");
        pascal_string(s, "missing.dat"); require(call(s, "CarbonLib::FSMakeFSSpec", {0xfffffffe, 3, text, spec}) == fnfErr, "Missing FSSpec fixture");
        require(call(s, "CarbonLib::FSpMakeFSRef", {spec, output_ref}) == fnfErr, "Missing FSSpec acquired an identity");

        // An FSRef obtained before private copying resolves to the current
        // merged namespace after a write. No original media file is changed.
        lookup(s, "media.dat", true, first_ref);
        require(!call(s, "CarbonLib::FSOpenFork", {first_ref, 0, 0, 3, counts}), "Writable private fork from an original identity");
        const auto fork = std::uint32_t(std::int16_t(s.memory.read<2>(counts)));
        s.memory.clear(pb, 80); s.memory.write<2>(pb + 24, fork); s.memory.write<4>(pb + 32, output);
        s.memory.write<4>(pb + 36, 4); s.memory.write<2>(pb + 44, 1); s.memory.copy_in(output, {'p','o','r','t'});
        require(!call(s, "CarbonLib::PBWriteForkSync", {pb}) && !call(s, "CarbonLib::FSCloseFork", {fork}), "Private native fork transfer/close");
        require(bytes(game / "media.dat") == "original" && bytes(user / "Files/media.dat") == "portinal", "FSRef write changed original media");
        require(!call(s, "CarbonLib::FSOpenFork", {first_ref, 0, 0, 1, counts}), "Read through a pre-copy identity");
        const auto reopened = std::uint32_t(std::int16_t(s.memory.read<2>(counts)));
        s.memory.write<2>(pb + 24, reopened); s.memory.clear(output, 4);
        require(!call(s, "CarbonLib::PBReadForkSync", {pb}) && s.memory.copy_out(output, 4) == std::vector<std::uint8_t>({'p','o','r','t'}),
            "Pre-copy identity read stale original bytes");
        require(!call(s, "CarbonLib::FSCloseFork", {reopened}), "Reopened private fork close");
        HFSUniStr255 resource_name{}; require(!::FSGetResourceForkName(&resource_name), "Native resource fork name");
        require(!setxattr((user / "small.dat").c_str(), "com.apple.ResourceFork", "resource", 8, 0, 0), "Native resource fork fixture");
        lookup(s, "small.dat", false, first_ref);
        for (unsigned n = 0; n < resource_name.length; ++n) s.memory.write<2>(text + n * 2, resource_name.unicode[n]);
        require(!call(s, "CarbonLib::FSOpenFork", {first_ref, resource_name.length, text, 1, counts}), "Named native fork open");
        const auto resource_fork = std::uint32_t(std::int16_t(s.memory.read<2>(counts)));
        s.memory.write<2>(pb + 24, resource_fork); s.memory.write<4>(pb + 36, 8);
        require(!call(s, "CarbonLib::PBReadForkSync", {pb}) && s.memory.copy_out(output, 8) == std::vector<std::uint8_t>({'r','e','s','o','u','r','c','e'}),
                "Named fork selected the data fork instead");
        require(!call(s, "CarbonLib::FSCloseFork", {resource_fork}) && bytes(user / "small.dat") == "ABCDEFGH", "Named fork damaged the data fork or ownership");

        // Classic metadata setters must also work while other direct clients
        // retain the legacy APIs. A getter populates the complete input record.
        lookup(s, "small.dat", false, first_ref); s.memory.clear(pb, 108);
        s.memory.write<4>(pb + 18, spec + 6); s.memory.write<2>(pb + 22, 0xfffe); s.memory.write<4>(pb + 48, 3);
        require(!call(s, "CarbonLib::PBGetCatInfoSync", {pb}), "Classic metadata getter");
        s.memory.write<4>(pb + 32, 0x42494e41); s.memory.write<4>(pb + 36, 0x43574945); s.memory.write<4>(pb + 48, 3);
        require(!call(s, "CarbonLib::PBSetCatInfoSync", {pb}) && !s.memory.read<2>(pb + 16), "Classic metadata setter");
        require(!call(s, "CarbonLib::FSGetCatalogInfo", {first_ref, kFSCatInfoFinderInfo, catalog, 0, 0, 0}) &&
            s.memory.read<4>(catalog + 72) == 0x42494e41 && s.memory.read<4>(catalog + 76) == 0x43574945, "Classic/modern metadata composition");
        s.memory.write<4>(pb + 48, 0xfefefefe);
        require(call(s, "CarbonLib::PBSetCatInfoSync", {pb}) == dirNFErr && std::int16_t(s.memory.read<2>(pb + 16)) == dirNFErr,
            "Classic setter accepted an unknown directory");

        Imports imports; s.stream_io_observer = &imports;
        // The real generated Mac bodies discover the normal production fork
        // capability themselves. No fixture replaces their Gestalt result.
        s.memory.write<1>(toc - 0x3504, 0); s.memory.write<1>(toc - 0x3503, 0);
        cstring(s, text, "CM User Data:small.dat"); cstring(s, mode_pointer, "rb");
        const auto stream = generated(s, 0x107768f0, {text, mode_pointer});
        require(stream != 0, "Original fopen failed over the real FSRef bridge");
        require(s.memory.read<1>(toc - 0x3504) == 1 && s.memory.read<1>(toc - 0x3503) == 1, "Original capability cache did not select production FSRef services");
        require(generated(s, 0x10775fe0, {output, 1, 8, stream}) == 8 && s.memory.copy_out(output, 8) ==
            std::vector<std::uint8_t>({'A','B','C','D','E','F','G','H'}), "Semantic fread through original modern open");
        require(!generated(s, 0x107766f0, {stream}), "Semantic fclose through original modern close");
        cstring(s, text, "CM User Data:created.dat"); cstring(s, mode_pointer, "wb");
        const auto written_stream = generated(s, 0x107768f0, {text, mode_pointer});
        require(written_stream != 0, "Original modern fopen creation failed");
        s.memory.copy_in(output, {'n','a','t','i','v','e'});
        require(generated(s, 0x10776360, {output, 1, 6, written_stream}) == 6 && !generated(s, 0x107766f0, {written_stream}) &&
            bytes(user / "created.dat") == "native", "Semantic fwrite/flush through original modern creation");
        cstring(s, text, std::string("CM User Data:Caf") + char(0x8e) + " stream.dat"); cstring(s, mode_pointer, "wb");
        const auto accented_stream = generated(s, 0x107768f0, {text, mode_pointer});
        require(accented_stream && generated(s, 0x10776360, {output, 1, 6, accented_stream}) == 6 &&
                !generated(s, 0x107766f0, {accented_stream}) && bytes(user / "Caf\xc3\xa9 stream.dat") == "native",
                "Original MacRoman filename to native Unicode stream composition");
        cstring(s, text, "CM User Data:created.dat"); cstring(s, mode_pointer, "wb");
        const auto truncated_stream = generated(s, 0x107768f0, {text, mode_pointer});
        require(truncated_stream && !generated(s, 0x107766f0, {truncated_stream}) && fs::file_size(user / "created.dat") == 0,
            "Original modern write mode did not truncate");
        cstring(s, text, "CM User Data:missing.dat"); cstring(s, mode_pointer, "rb");
        require(!generated(s, 0x107768f0, {text, mode_pointer}) && !fs::exists(user / "missing.dat"), "Read mode created a missing file");
        cstring(s, text, "CM User Data:created.dat"); cstring(s, mode_pointer, "CM User Data:renamed.dat");
        require(!generated(s, 0x107766c0, {text, mode_pointer}) && !fs::exists(user / "created.dat") && fs::exists(user / "renamed.dat"),
            "Original modern rename chain");
        cstring(s, text, "CM User Data:renamed.dat");
        require(!generated(s, 0x10776690, {text}) && !fs::exists(user / "renamed.dat"), "Original modern remove chain");
        require(generated(s, 0x10776690, {text}) == UINT32_MAX, "Original remove facade failure result");
        // Force Classic only for a final composition witness of its repaired
        // metadata path. Production capability is tested independently below.
        s.memory.write<1>(toc - 0x3503, 0);
        cstring(s, text, "CM User Data:classic-created.dat"); cstring(s, mode_pointer, "wb");
        const auto classic_stream = generated(s, 0x107768f0, {text, mode_pointer});
        require(classic_stream && !generated(s, 0x107766f0, {classic_stream}) && fs::exists(user / "classic-created.dat"),
            "Original Classic create/metadata/close chain");
        s.stream_io_observer = nullptr;
        for (const char* operation : {"CarbonLib::UpgradeScriptInfoToTextEncoding", "CarbonLib::CreateTextToUnicodeInfoByEncoding",
             "CarbonLib::ConvertFromTextToUnicode", "CarbonLib::DisposeTextToUnicodeInfo", "CarbonLib::FSMakeFSRefUnicode",
             "CarbonLib::FSpMakeFSRef", "CarbonLib::FSGetCatalogInfo", "CarbonLib::FSCreateFileUnicode", "CarbonLib::FSOpenFork",
             "CarbonLib::FSCompareFSRefs", "CarbonLib::FSRenameUnicode", "CarbonLib::FSDeleteObject", "CarbonLib::PBSetCatInfoSync"})
            require(imports.calls[operation] != 0, "Original modern chain did not execute " + std::string(operation));
        std::cout << "Native file references passed " << checks << " assertions, " << generated_calls
                  << " generated calls and " << imports.calls.size() << " real imported services\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "Native file references failed: " << error.what() << '\n'; return 1; }
}
