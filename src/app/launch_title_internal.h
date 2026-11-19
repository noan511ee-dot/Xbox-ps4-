// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 RafaelNGP
#pragma once

#include "app/library_internal.h"
#include "xcloud/gssv.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <vector>

namespace xc::app {

inline std::string normalizeLaunchLookupId(const std::string& id) {
    std::string normalized;
    normalized.reserve(id.size());
    for (unsigned char ch : id) {
        if (!std::isspace(ch)) normalized.push_back(static_cast<char>(std::toupper(ch)));
    }
    return normalized;
}

// The public catalog's XCloudTitleId is an authoritative launch ID; reject
// absent values and Store IDs, which are not accepted by GSSV startSession.
inline std::string resolveCatalogLaunchTitleId(const std::map<std::string, xcloud::Product>& products,
                                              const std::string& productId) {
    const std::string wanted = normalizeLaunchLookupId(productId);
    for (const auto& [id, product] : products) {
        if (normalizeLaunchLookupId(id) != wanted && normalizeLaunchLookupId(product.productId) != wanted) continue;
        if (product.xcloudTitleId.empty() || product.xcloudTitleId == "None" ||
            normalizeLaunchLookupId(product.xcloudTitleId) == wanted) return {};
        return product.xcloudTitleId;
    }
    return {};
}

// A Store product ID or Xbox Live title ID is not a GSSV launch ID. Resolve
// only to the titleId actually returned by the authenticated GSSV title list.
inline std::string resolveCloudLaunchTitleId(const std::vector<xcloud::Title>& titles,
                                             const std::string& productId) {
    if (productId.empty()) return {};

    const std::string wantedProduct = normalizeLaunchLookupId(productId);
    const std::string wantedXboxId = normalizeLaunchLookupId(kRocketLeagueXboxTitleId);
    const std::string wantedXboxIdAlias = normalizeLaunchLookupId(kRocketLeagueXboxTitleIdAlias);

    for (const auto& title : titles) {
        bool productMatches = normalizeLaunchLookupId(title.productId) == wantedProduct;
        if (!productMatches) {
            productMatches = std::any_of(title.alternateProductIds.begin(), title.alternateProductIds.end(),
                                         [&](const std::string& alternate) {
                                             return normalizeLaunchLookupId(alternate) == wantedProduct;
                                         });
        }
        const std::string xboxId = normalizeLaunchLookupId(title.xboxTitleId);
        const bool rocketLeagueMatches = wantedProduct == normalizeLaunchLookupId(kRocketLeagueProductId) &&
                                         (xboxId == wantedXboxId || xboxId == wantedXboxIdAlias);
        if ((productMatches || rocketLeagueMatches) && !title.titleId.empty() && title.titleId != "None") {
            return title.titleId;
        }
    }
    return {};
}

}  // namespace xc::app
