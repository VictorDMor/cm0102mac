#pragma once

#include <cstdint>

namespace cm::aot {
struct State;
namespace guest { class Cpu; }

namespace match_pl {
inline constexpr std::uint32_t shot_event_entry = 0x102576e0;

void execute_shot_event(guest::Cpu&);
bool shot_event_uses_cpp(const State&);
void run_shot_event_until(State&, std::uint32_t stop);

// Paired checks can select the original for one State without changing
// the route of any other MATCH_PL routine or of the comparison State.
class OriginalShotEventScope {
public:
    explicit OriginalShotEventScope(const State&) noexcept;
    ~OriginalShotEventScope();
    OriginalShotEventScope(const OriginalShotEventScope&) = delete;
    OriginalShotEventScope& operator=(const OriginalShotEventScope&) = delete;
private:
    const State* previous_;
};
}
}
