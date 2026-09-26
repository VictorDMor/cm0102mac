#include "floating_point.hpp"
#include "aot_runtime.hpp"
#include <cstdio>
#include <cstdlib>

namespace cm::aot {
void original_fctiwz(State& s, unsigned destination, unsigned source) {
    // PowerPC Programming Environments, Rev. 1, section D.4.2:
    // https://www.nxp.com/docs/en/user-guide/MPCFPE_AD_R1.pdf
    const auto bits = s.r<8>(source);
    const bool nan = (bits & UINT64_C(0x7ff0000000000000)) == UINT64_C(0x7ff0000000000000) &&
                     (bits & UINT64_C(0x000fffffffffffff));
    const bool signaling = nan && !(bits & UINT64_C(0x0008000000000000));
    const auto value = float_value<8>(bits);
    const auto rounded = nan ? 0.0 : std::trunc(value);
    const bool invalid = nan || rounded < -2147483648.0 || rounded >= 2147483648.0;
    const bool inexact = !invalid && rounded != value;
    bool new_exception = false;
    if (invalid) {
        new_exception = !s.r<1>(1303) || (signaling && !s.r<1>(1287));
        s.wr<1>(1303, 1); // VXCVI
        if (signaling) s.wr<1>(1287, 1); // VXSNAN
    } else if (inexact) {
        new_exception = !s.r<1>(1286);
        s.wr<1>(1286, 1); // Sticky XX
    }
    s.wr<1>(1293, 0); // FR: truncation never increments the magnitude.
    s.wr<1>(1294, inexact); // FI is cleared for invalid conversions.
    if (new_exception) s.wr<1>(1280, 1); // Sticky FX
    bool invalid_summary = false;
    for (auto offset : {1287u, 1288u, 1289u, 1290u, 1291u, 1292u, 1301u, 1302u, 1303u})
        invalid_summary |= s.r<1>(offset) != 0;
    s.wr<1>(1282, invalid_summary); // VX
    const bool enabled = (invalid_summary && s.r<1>(1304)) ||
        (s.r<1>(1283) && s.r<1>(1305)) || (s.r<1>(1284) && s.r<1>(1306)) ||
        (s.r<1>(1285) && s.r<1>(1307)) || (s.r<1>(1286) && s.r<1>(1308));
    s.wr<1>(1281, enabled); // FEX
    if (invalid && s.r<1>(1304))
        throw std::runtime_error("Original enabled floating invalid-conversion exception requires a handler");
    const std::uint32_t result = invalid ? (nan || (bits >> 63) ? 0x80000000u : 0x7fffffffu) :
        std::uint32_t(std::int32_t(rounded));
    // The upper word and FPRF are architecturally undefined. Preserve the
    // export's sign-extension convention and leave FPRF unchanged.
    s.wr<8>(destination, std::uint64_t(signed_value<4>(result)));
    if (invalid && std::getenv("CM_AOT_TRACE_FLOAT"))
        std::fprintf(stderr, "Native fctiwz exceptional conversion: pc=%08x input=%016llx result=%08x\n",
            s.pc, static_cast<unsigned long long>(bits), result);
    if (inexact && s.r<1>(1308))
        throw std::runtime_error("Original enabled floating inexact exception requires a handler");
}
}
