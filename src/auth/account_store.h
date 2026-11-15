// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 RafaelNGP
#pragma once

#include <string>
#include <utility>
#include <vector>

namespace xc::auth {

struct AccountEntry {
    std::string id;       // random local identifier; never a token or password
    std::string gamertag; // display-only metadata
    std::string xuid;     // display/account identity; never used as a path
    std::string gamerpicUrl; // public profile image URL; optional display metadata
};

// A small, non-secret index. Refresh tokens remain in separate AuthManager
// files; this index must never contain credentials.
class AccountStore {
public:
    explicit AccountStore(std::string path) : path_(std::move(path)) {}

    bool load();
    bool save() const;
    bool upsert(const AccountEntry& entry);
    bool setActive(const std::string& id);
    bool remove(const std::string& id);
    bool contains(const std::string& id) const;
    const std::vector<AccountEntry>& accounts() const { return accounts_; }
    const std::string& activeId() const { return activeId_; }
    const AccountEntry* find(const std::string& id) const;

    // The old single-account file remains the legacy slot for safe migration.
    static std::string tokenPath(const std::string& dataDir, const std::string& id);
    static std::string libraryCachePath(const std::string& dataDir, const std::string& id);
    static std::string newId();
    static bool validId(const std::string& id);

private:
    std::string path_;
    std::vector<AccountEntry> accounts_;
    std::string activeId_;
};

} // namespace xc::auth
