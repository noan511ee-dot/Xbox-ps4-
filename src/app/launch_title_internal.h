// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 RafaelNGP
#pragma once

#include "app/library_internal.h"
#include "xcloud/gssv.h"

#include <algorithm>
#include <string>
#include <vector>

namespace xc::app {

// A Store product ID or Xbox Live title ID is not a GSSV launch ID. Resolve
// only to the titleId actually returned by the authenticated GSSV title list.
inline std::string resolveCloudLaunchTitleId(const std::vector<xcloud::Title>& titles,
                                             const std::string& productId) {
    if (productId.empty()) return {};

    for (const auto& title : titles) {
        const bool productMatches = title.productId == productId ||
            std::find(title.alternateProductIds.begin(), title.alternateProductIds.end(), productId) !=
                title.alternateProductIds.end();
        const bool rocketLeagueMatches = productId == kRocketLeagueProductId &&
            title.xboxTitleId == kRocketLeagueXboxTitleId;
        if ((productMatches || rocketLeagueMatches) && !title.titleId.empty() && title.titleId != "None") {
            return title.titleId;
        }
    }
    return {};
}

}  // namespace xc::app
