#include "snapshot_stream.hpp"
#include <zstd.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <vector>
#include <unistd.h>

namespace cm::aot {
namespace {
void check(std::size_t result) {
    if (ZSTD_isError(result))
        throw std::runtime_error(std::string("Private snapshot compression: ") + ZSTD_getErrorName(result));
}

class Encoder final : public std::streambuf {
    std::unique_ptr<ZSTD_CCtx, decltype(&ZSTD_freeCCtx)> ctx_{ZSTD_createCCtx(), ZSTD_freeCCtx};
    std::vector<char> input_, output_;
    std::FILE* file_;

    void drain(ZSTD_EndDirective directive) {
        ZSTD_inBuffer input{pbase(), std::size_t(pptr() - pbase()), 0};
        std::size_t remaining;
        do {
            ZSTD_outBuffer output{output_.data(), output_.size(), 0};
            remaining = ZSTD_compressStream2(ctx_.get(), &output, &input, directive);
            check(remaining);
            if (std::fwrite(output.dst, 1, output.pos, file_) != output.pos)
                throw std::runtime_error("Cannot write private snapshot");
        } while (input.pos < input.size || (directive == ZSTD_e_end && remaining));
        setp(input_.data(), input_.data() + input_.size());
    }
    int_type overflow(int_type c) override {
        drain(ZSTD_e_continue);
        if (!traits_type::eq_int_type(c, traits_type::eof())) sputc(traits_type::to_char_type(c));
        return traits_type::not_eof(c);
    }
public:
    explicit Encoder(std::FILE* file) : input_(ZSTD_CStreamInSize()), output_(ZSTD_CStreamOutSize()), file_(file) {
        if (!ctx_) throw std::bad_alloc{};
        check(ZSTD_CCtx_setParameter(ctx_.get(), ZSTD_c_compressionLevel, 1));
        check(ZSTD_CCtx_setParameter(ctx_.get(), ZSTD_c_checksumFlag, 1));
        setp(input_.data(), input_.data() + input_.size());
    }
    void finish() { drain(ZSTD_e_end); }
};

class Decoder final : public std::streambuf {
    std::unique_ptr<ZSTD_DCtx, decltype(&ZSTD_freeDCtx)> ctx_{ZSTD_createDCtx(), ZSTD_freeDCtx};
    std::ifstream& file_;
    std::vector<char> compressed_, decoded_;
    ZSTD_inBuffer input_{};
    bool finished_ = false;

    int_type underflow() override {
        while (!finished_) {
            if (input_.pos == input_.size && !file_.eof()) {
                file_.read(compressed_.data(), compressed_.size());
                if (file_.bad()) throw std::runtime_error("Cannot read private snapshot");
                input_ = {compressed_.data(), std::size_t(file_.gcount()), 0};
            }
            ZSTD_outBuffer output{decoded_.data(), decoded_.size(), 0};
            const auto remaining = ZSTD_decompressStream(ctx_.get(), &output, &input_);
            check(remaining);
            if (!remaining) {
                finished_ = true;
                if (input_.pos != input_.size || file_.peek() != traits_type::eof() || file_.bad())
                    throw std::runtime_error("Trailing data after private snapshot frame");
            } else if (!output.pos && input_.pos == input_.size && file_.eof()) {
                throw std::runtime_error("Truncated private snapshot frame");
            }
            if (output.pos) {
                setg(decoded_.data(), decoded_.data(), decoded_.data() + output.pos);
                return traits_type::to_int_type(*gptr());
            }
        }
        return traits_type::eof();
    }
public:
    explicit Decoder(std::ifstream& file)
        : file_(file), compressed_(ZSTD_DStreamInSize()), decoded_(ZSTD_DStreamOutSize()) {
        if (!ctx_) throw std::bad_alloc{};
    }
};
}

void write_snapshot_file(const std::filesystem::path& path, const std::function<void(std::ostream&)>& write) {
    auto temporary = path.string() + ".XXXXXX";
    const int fd = mkstemp(temporary.data());
    if (fd < 0) throw std::runtime_error("Cannot create private snapshot beside " + path.string());
    try {
        std::unique_ptr<std::FILE, decltype(&std::fclose)> file(fdopen(fd, "wb"), std::fclose);
        if (!file) {
            close(fd);
            throw std::runtime_error("Cannot open private snapshot output");
        }
        Encoder buffer(file.get());
        std::ostream out(&buffer);
        out.exceptions(std::ios::badbit | std::ios::failbit);
        write(out);
        buffer.finish();
        if (std::fclose(file.release()) != 0) throw std::runtime_error("Cannot finish private snapshot");
        std::filesystem::rename(temporary, path);
    } catch (...) {
        unlink(temporary.c_str());
        throw;
    }
}

void read_snapshot_file(const std::filesystem::path& path, const std::function<void(std::istream&)>& read) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open private match snapshot");
    constexpr std::array<unsigned char, 4> zstd_magic{0x28, 0xb5, 0x2f, 0xfd};
    std::array<unsigned char, 4> prefix{};
    if (!file.read(reinterpret_cast<char*>(prefix.data()), prefix.size()))
        throw std::runtime_error("Truncated private snapshot header");
    file.seekg(0);
    file.exceptions(std::ios::badbit);
    if (prefix != zstd_magic) {
        read(file);
        return;
    }
    Decoder buffer(file);
    std::istream in(&buffer);
    in.exceptions(std::ios::badbit);
    read(in);
}
}
