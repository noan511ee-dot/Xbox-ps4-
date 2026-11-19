// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 RafaelNGP
#pragma once

namespace xc::app {

// Reports an ephemeral active-session heartbeat only when the user has opted in.
// Safe to start after TLS initialization; stop joins the reporter before shutdown.
void startPresenceReporter();
void stopPresenceReporter();

}  // namespace xc::app
