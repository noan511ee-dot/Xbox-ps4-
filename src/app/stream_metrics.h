// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 RafaelNGP
#pragma once

#include <cstdint>

namespace xc::app {

// The UI's FPS is a presentation metric: count frames confirmed shown by the
// display, not decoded frames or calls to present().
inline double shownFps(uint64_t currentShown, uint64_t previousShown, double elapsedSeconds) noexcept {
    if (currentShown < previousShown || !(elapsedSeconds >= 0.05)) return 0.0;
    return static_cast<double>(currentShown - previousShown) / elapsedSeconds;
}

}  // namespace xc::app
