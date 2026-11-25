#pragma once
#include <chrono>

namespace cm::aot {
// Pacing of the host's presentation while the match screen runs at Turbo. The original
// redraws the screen after every commentary line, and each redraw of the game window is a
// full copy of its pixels plus a round trip to the main thread; at Turbo that cost, and the
// event loop's quiet-screen wait (8 ms per turn), were most of the wall time of a match.
// Only the host's presentation and its waiting change: the guest draws into its own
// pixels exactly as before, and a deferred frame is always presented later (it is a
// snapshot of the whole window, so a later one covers it).
class TurboPresentation {
public:
    using Clock = std::chrono::steady_clock;
    // At most one frame per interval; ~25 per second is as many as a commentary line
    // stays readable for at Turbo's 20 ms pace.
    static constexpr std::chrono::milliseconds interval{40};
    // The screen counts as animating, and not idle, for this long after a frame request.
    static constexpr std::chrono::milliseconds animation{100};

    // A frame is requested at `now`: whether to present it now. Anywhere but on Turbo's
    // match screen, always.
    bool request(Clock::time_point now, bool turbo_screen) {
        if (turbo_screen) {
            requested_ = now;
            if (now - presented_ < interval) { deferred_ = true; return false; }
        }
        presented_ = now;
        deferred_ = false;
        return true;
    }
    // Whether the deferred frame is due: its interval has passed, the screen is no longer
    // Turbo's match screen, or the game is about to wait or be observed (`force`).
    bool due(Clock::time_point now, bool turbo_screen, bool force) const {
        return deferred_ && (force || !turbo_screen || now - presented_ >= interval);
    }
    void presented(Clock::time_point now) { presented_ = now; deferred_ = false; }
    // The deferred frame's window is gone: nothing was presented.
    void discard() { deferred_ = false; }
    bool deferred() const { return deferred_; }
    // Frames are being drawn on Turbo's match screen: the game is working, not idle, so its
    // event loop does not wait for input between turns.
    bool animating(Clock::time_point now, bool turbo_screen) const {
        return turbo_screen && now - requested_ < animation;
    }
private:
    Clock::time_point presented_{}, requested_{};
    bool deferred_ = false;
};
}
