#include "aot_runtime.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <tuple>

using namespace cm::aot;
int main(int argc, char** argv) {
    if (argc != 3) { std::cerr << "Usage: cm-aot-checks <segments> <oracle.tsv>\n"; return 2; }
    try {
        std::ifstream input(argv[2]);
        std::string command, identity;
        if (!(input >> command >> identity) || command != "SOURCE" ||
            identity != "91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b")
            throw std::runtime_error("Missing or mismatched original-code oracle");
        unsigned passed = 0;
        while (input >> command) {
            if (command != "CASE") throw std::runtime_error("Malformed oracle case");
            std::string name;
            std::uint32_t entry;
            std::uint64_t steps;
            if (!(input >> name >> entry >> steps)) throw std::runtime_error("Truncated case header");
            auto s = std::make_unique<State>();
            load_program(*s, argv[1]);
            s->pc = entry;
            using Check = std::tuple<std::size_t, unsigned, std::uint64_t>;
            std::vector<Check> registers;
            std::vector<std::pair<std::uint32_t, std::vector<std::uint8_t>>> memory;
            bool ended = false;
            while (input >> command) {
                if (command == "END") { ended = true; break; }
                if (command == "I" || command == "E") {
                    std::size_t offset;
                    unsigned size;
                    std::string value;
                    if (!(input >> offset >> size >> value) || !size || size > 8 ||
                        offset + size > s->registers.size()) throw std::runtime_error("Invalid oracle register");
                    const auto v = std::stoull(value, nullptr, 16);
                    if (command == "I") {
                        for (unsigned i = 0; i < size; ++i)
                            s->registers[offset + size - 1 - i] = (v >> (8 * i)) & 255;
                    } else registers.emplace_back(offset, size, v);
                } else if (command == "M") {
                    std::uint32_t address;
                    std::string hex;
                    if (!(input >> address >> hex) || hex.size() % 2) throw std::runtime_error("Invalid oracle memory");
                    std::vector<std::uint8_t> bytes;
                    for (std::size_t i = 0; i < hex.size(); i += 2)
                        bytes.push_back(std::stoul(hex.substr(i, 2), nullptr, 16));
                    memory.emplace_back(address, std::move(bytes));
                } else throw std::runtime_error("Unknown oracle field");
            }
            if (!ended) throw std::runtime_error("Truncated oracle case");
            try { run(*s); }
            catch (const std::exception& error) { throw std::runtime_error(name + ": " + error.what()); }
            if (s->executed != steps) throw std::runtime_error(name + ": instruction-count mismatch");
            for (const auto& [offset, size, expected] : registers) {
                std::uint64_t actual = 0;
                for (unsigned i = 0; i < size; ++i) actual = (actual << 8) | s->registers[offset + i];
                if (actual != expected) throw std::runtime_error(name + ": register mismatch at " + std::to_string(offset));
            }
            for (const auto& [address, expected] : memory)
                if (s->memory.copy_out(address, expected.size()) != expected)
                    throw std::runtime_error(name + ": stack-memory mismatch");
            ++passed;
        }
        if (!passed) throw std::runtime_error("Empty oracle");
        std::cout << passed << " original routines/cases match the PowerPC oracle on ARM64\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
