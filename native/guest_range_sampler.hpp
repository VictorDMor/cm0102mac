#pragma once

#include "guest_values.hpp"
#include "guest_saved_registers.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::guest {

template<std::uint32_t Entry, std::int32_t ConstantsSlot, unsigned SignedBias>
class RandomRangeSampler {
    static constexpr std::uint32_t reseed = 0x1077b770;
    static void block(Cpu& c, std::uint32_t offset, unsigned instructions) {
        c.s.step(Entry + offset);
        c.s.charge(instructions - 1);
    }
    static constexpr std::uint32_t word_range = 65535, double_header = 0x43300000;
    static constexpr std::int32_t reciprocal_word_range = std::int32_t(0x80008001u);
    static constexpr unsigned frame_size = 0x80;

    static void record_signed(Cpu& c, std::int32_t value) {
        const auto condition = value < 0 ? 8u : value > 0 ? 4u : 2u;
        const auto overflow = c.s.r<1>(guest::reg::xer_so) & 1;
        c.set_cr(0, std::uint8_t((c.cr(0) & 0xf0) | condition | overflow));
    }

    struct SamplePool {
        Cpu& c;

        std::uint32_t constants() const { return c.r(25); }
        std::uint32_t salt() const { return c.r(26); }
        std::uint32_t base() const { return c.r(27); }
        std::uint32_t end() const { return c.r(28); }
        std::uint32_t cursor() const { return c.r(29); }

        void advance(bool correction) const {
            std::uint32_t current = c.r(3);
            if (!correction) {
                block(c, 0x34, 6);
                current = c.u32(cursor());
                c.set(3, current);
            } else {
                block(c, 0x124, 5);
            }
            const auto last = c.u32(end());
            c.set(0, last);
            const auto next = current + 2;
            c.set(3, next);
            if (correction) c.put32(cursor(), next);
            const bool wrap = compare_logical(c, next, last) > 0;
            if (!correction) c.put32(cursor(), next);
            if (!wrap) return;

            const auto pc = correction ? 0x138u : 0x4cu;
            block(c, pc, 2);
            c.put32(cursor(), base());
            c.call(reseed, Entry + pc + 8);
            block(c, pc + 8, 2);
            const auto value = c.r(3) & 0xffffu;
            c.set(0, value);
            c.put32(salt(), value);
        }

        void small_draw(std::int32_t bound, bool correction) const {
            const auto pc = correction ? 0x4d4u : 0x538u;
            block(c, pc, 10);
            const auto position = c.u32(cursor());
            c.set(3, position);
            const auto sign = compare(c, bound);
            const auto bias = c.u32(salt());
            c.set(0, bias);
            const auto word = c.u16(position);
            const auto biased_word = word + bias;
            c.set(3, biased_word);
            const auto divisor = std::uint32_t(bound);
            const auto quotient = std::uint32_t(divide_signed<4>(biased_word, divisor));
            const auto remainder = biased_word - quotient * divisor;
            const auto result_register = correction ? 24u : 3u;
            c.set({{0, remainder}, {result_register, 0u - remainder}});
            if (sign > 0) {
                block(c, pc + 40, 1);
                c.set(result_register, remainder);
            }
        }
    };

    struct Reduction {
        std::int32_t floor, quotient;
        bool carry;
    };

    static Reduction word_reduction(std::uint32_t bits) {
        const auto product_high = (std::int64_t(reciprocal_word_range) * std::int32_t(bits)) >> 32;
        const auto sum = std::int32_t(std::uint32_t(product_high) + bits);
        return {sum >> 15, std::int32_t(bits) / std::int32_t(word_range), sum < 0 && (std::uint32_t(sum) & 0x7fff)};
    }

    static void wide_draw(Cpu& c, const SamplePool& pool, std::int32_t bound, std::uint32_t frame) {
        block(c, 0x7c, 40);
        guest::Fpu fpu(c);
        const auto input_bits = std::uint32_t(bound) ^ 0x80000000u;
        c.set({{9, double_header}, {0, input_bits}});
        c.put32(frame + 0x4c, input_bits);
        c.set(5, 0x80010000);
        const auto position = c.u32(pool.cursor());
        c.set({{3, position}, {4, 0x10000}});
        c.put32(frame + 0x48, double_header);
        c.set(6, std::uint32_t(reciprocal_word_range));
        const auto bias = c.f64(pool.constants() + SignedBias);
        c.set_f(2, bias);
        const auto tenfold = std::uint32_t(bound) * 10;
        c.set(0, tenfold);
        const auto wide_bound = c.f64(frame + 0x48);
        c.set_f(1, wide_bound);
        c.set(5, word_range);
        const auto scale = c.f64(pool.constants());
        c.set_f(0, scale);
        const auto unbiased_bound = fpu.fsub(wide_bound, bias);
        c.set_f(1, unbiased_bound);
        c.set(4, std::uint32_t((std::int64_t(reciprocal_word_range) * std::int32_t(tenfold)) >> 32));
        const auto word = c.u16(position);
        c.set(8, word);
        const auto salt = c.u32(pool.salt());
        c.set(7, salt);
        const auto scaled_bound = fpu.fdiv(unbiased_bound, scale);
        c.set_f(1, scaled_bound);
        c.put32(frame + 0x40, double_header);

        const auto biased_word = word + salt;
        const auto word_info = word_reduction(biased_word);
        const auto bound_info = word_reduction(tenfold);
        const auto multiple = std::uint32_t(word_info.quotient) * word_range;
        const auto remainder = biased_word - multiple;
        const auto reduced_word = remainder ^ 0x80000000u;
        c.set({{7, biased_word}, {6, word_info.floor < 0}, {5, multiple}, {4, bound_info.floor < 0},
               {22, std::uint32_t(bound_info.quotient)}, {0, reduced_word}});
        c.s.wr<1>(guest::reg::xer_ca, bound_info.carry);
        record_signed(c, bound_info.quotient);
        c.put32(frame + 0x44, reduced_word);
        const auto scaled_word = fpu.fsub(c.f64(frame + 0x40), bias);
        c.set_f(0, fpu.fmul(scaled_bound, scaled_word));
        c.put64(frame + 0x38, fpu.fctiwz(0, c.f(0)));
        c.set(31, c.u32(frame + 0x3c));

        if (!bound_info.quotient) {
            block(c, 0x11c, 2);
            c.set(24, 0);
        } else {
            // A wrapped signed word divided by 65535 is at most 32768 in
            // magnitude. The wider correction arms and recursive call cannot run.
            pool.advance(true);
            block(c, 0x148, 4);
            c.set({{3, 0x10000}, {0, word_range}});
            compare(c, bound_info.quotient, std::int32_t(word_range));
            block(c, 0x158, 4);
            c.set({{3, 0xffff0000}, {0, std::uint32_t(-65535)}});
            compare(c, bound_info.quotient, -65535);
            pool.small_draw(bound_info.quotient, true);
        }

        block(c, 0x500, 11);
        const auto numerator = std::int32_t(c.r(24) + 5);
        const auto shift_source = (INT64_C(0x66666667) * numerator) >> 32;
        const auto capped_at = std::uint32_t(bound) - 1;
        const auto sample = c.r(31) + std::uint32_t(numerator / 10);
        c.set({{0, capped_at}, {3, std::uint32_t(numerator / 10)}, {4, shift_source < 0}, {31, sample}});
        c.s.wr<1>(guest::reg::xer_ca, shift_source < 0 && (shift_source & 3));
        if (compare(c, std::int32_t(sample), std::int32_t(capped_at)) > 0) {
            block(c, 0x52c, 1);
            c.set(31, capped_at);
        }
        block(c, 0x530, 2);
        c.set(3, c.r(31));
    }

public:
    static std::uint32_t sample(Cpu& c, std::int32_t bound) {
        block(c, 0, 11);
        const auto stack = c.sp();
        const auto link = c.lr();
        c.set(0, link);
        transfer_saved_registers<20>(c, stack);
        c.set(30, std::uint32_t(bound));
        record_signed(c, bound);
        c.set(25, c.toc(ConstantsSlot));
        c.set(26, c.toc(-0x7b90));
        c.set(27, c.toc(-0x7b8c));
        c.set(28, c.toc(-0x7b88));
        c.set(29, c.toc(-0x7b84));
        c.put32(stack + 8, link);
        const auto frame = stack - frame_size;
        c.put32(frame, stack);
        c.set(1, frame);
        const SamplePool pool{c};
        if (!bound) {
            block(c, 0x2c, 2);
            c.set(3, 0);
        } else {
            pool.advance(false);
            block(c, 0x5c, 4);
            c.set({{3, 0x10000}, {0, word_range}});
            if (compare(c, bound, std::int32_t(word_range)) > 0) {
                wide_draw(c, pool, bound, frame);
            } else {
                block(c, 0x6c, 4);
                c.set({{3, 0xffff0000}, {0, std::uint32_t(-65535)}});
                if (compare(c, bound, -65535) >= 0) pool.small_draw(bound, false);
                else wide_draw(c, pool, bound, frame);
            }
        }
        block(c, 0x564, 5);
        const auto restored_stack = c.sp() + frame_size;
        const auto restored_link = c.u32(restored_stack + 8);
        c.set({{0, restored_link}, {1, restored_stack}});
        c.set_lr(restored_link);
        transfer_saved_registers<20, true>(c, restored_stack);
        return c.r(3);
    }
};
}
