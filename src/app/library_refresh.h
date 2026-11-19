// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 RafaelNGP
#pragma once

#include <cstdint>

namespace xc::app {

// No catalog push notification is available to the client; check periodically
// while the signed-in user is back at the library, rather than during a stream.
constexpr uint64_t kLibraryAutoRefreshIntervalMs = 5 * 60 * 1000;

inline uint64_t libraryRefreshDeadline(uint64_t nowMs) noexcept {
    return nowMs + kLibraryAutoRefreshIntervalMs;
}

inline bool shouldAutoRefreshLibrary(uint64_t nowMs, uint64_t dueAtMs, bool signedIn,
                                     bool onHomeScreen, bool hasLibrary, bool commandIdle) noexcept {
    return dueAtMs != 0 && nowMs >= dueAtMs && signedIn && onHomeScreen && hasLibrary && commandIdle;
}

}  // namespace xc::app
