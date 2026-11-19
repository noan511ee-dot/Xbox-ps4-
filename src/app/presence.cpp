// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 RafaelNGP
#include "app/presence.h"

#include "app/app_state.h"
#include "net/http.h"
#include "platform/platform.h"
#include "util/log.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

namespace xc::app {
namespace {

constexpr char kPresenceEndpoint[] = "https://pboxsite-vsgc3x5w.manus.space/api/presence/heartbeat";
constexpr uint64_t kHeartbeatIntervalMs = 30'000;
constexpr uint32_t kSettingsPollMs = 250;
platform::Thread g_presenceThread;
std::atomic<bool> g_stopPresence{false};
std::atomic<bool> g_presenceRunning{false};

bool presenceEnabled() {
    std::lock_guard<std::mutex> lock(g_settingsMutex);
    return g_settings.sharePresence;
}

std::string makeSessionId() {
    std::array<uint8_t, 16> random{};
    if (!platform::randomBytes(random.data(), random.size())) return {};
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(random.size() * 2);
    for (uint8_t byte : random) {
        out.push_back(kHex[byte >> 4]);
        out.push_back(kHex[byte & 0x0F]);
    }
    return out;
}

bool sendHeartbeat(const std::string& sessionId, bool active) {
    net::Request request;
    request.method = "POST";
    request.url = kPresenceEndpoint;
    request.headers = {{"Content-Type", "application/json"}, {"Accept", "application/json"}};
    request.body = std::string("{\"sessionId\":\"") + sessionId + "\",\"active\":" +
                   (active ? "true}" : "false}");
    request.timeoutMs = 4'000;
    const net::Response response = net::perform(request);
    if (!response.ok()) {
        static bool failureLogged = false;
        static uint64_t lastFailureLogAt = 0;
        const uint64_t now = platform::nowMs();
        if (!failureLogged || now - lastFailureLogAt >= 5 * 60 * 1000) {
            XC_LOGW("presence: %s heartbeat failed (HTTP %d)", active ? "active" : "inactive", response.status);
            lastFailureLogAt = now;
            failureLogged = true;
        }
    }
    return response.ok();
}

void presenceLoop() {
    platform::setThreadAffinity(platform::ThreadRole::Worker, "pbox_presence");
    g_presenceRunning.store(true, std::memory_order_release);
    std::string sessionId;
    bool previouslyEnabled = false;
    bool sessionWasRegistered = false;
    uint64_t nextHeartbeatAt = 0;
    while (!g_stopPresence.load(std::memory_order_acquire) && !g_quit.load(std::memory_order_acquire)) {
        const bool enabled = presenceEnabled();
        const uint64_t now = platform::nowMs();
        if (enabled && !previouslyEnabled) nextHeartbeatAt = 0;
        if (!enabled && previouslyEnabled) {
            if (sessionWasRegistered && !sessionId.empty()) sendHeartbeat(sessionId, false);
            sessionWasRegistered = false;
            sessionId.clear();
        }
        previouslyEnabled = enabled;
        if (enabled && now >= nextHeartbeatAt) {
            if (sessionId.empty()) sessionId = makeSessionId();
            if (sessionId.empty()) {
                XC_LOGW("presence: could not create an ephemeral session token; sharing remains inactive");
            } else {
                if (sendHeartbeat(sessionId, true)) {
                    if (!sessionWasRegistered) XC_LOGI("presence: anonymous active session accepted");
                    sessionWasRegistered = true;
                }
            }
            nextHeartbeatAt = platform::nowMs() + kHeartbeatIntervalMs;
        }
        for (uint32_t waited = 0; waited < kHeartbeatIntervalMs &&
                                   !g_stopPresence.load(std::memory_order_acquire) &&
                                   !g_quit.load(std::memory_order_acquire);) {
            const uint32_t step = std::min(kSettingsPollMs, static_cast<uint32_t>(kHeartbeatIntervalMs - waited));
            platform::sleepMs(step);
            waited += step;
            if (presenceEnabled() != previouslyEnabled) break;
        }
    }
    if (sessionWasRegistered && !sessionId.empty()) sendHeartbeat(sessionId, false);
    g_presenceRunning.store(false, std::memory_order_release);
}

}  // namespace

void startPresenceReporter() {
    if (g_presenceThread.joinable()) {
        if (g_presenceRunning.load(std::memory_order_acquire)) return;
        g_presenceThread.join();
    }
    g_stopPresence.store(false, std::memory_order_release);
    g_presenceRunning.store(false, std::memory_order_release);
    if (!platform::startThread(g_presenceThread, presenceLoop, 64u << 10)) {
        XC_LOGW("presence: background reporter could not be started");
    }
}

void stopPresenceReporter() {
    g_stopPresence.store(true, std::memory_order_release);
    if (g_presenceThread.joinable()) g_presenceThread.join();
    g_presenceRunning.store(false, std::memory_order_release);
}

}  // namespace xc::app
