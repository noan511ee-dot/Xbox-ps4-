// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 RafaelNGP
#include "auth/account_store.h"

#include "platform/platform.h"
#include "util/json.h"

#include <algorithm>
#include <array>

namespace xc::auth {

bool AccountStore::validId(const std::string& id) {
    if (id == "legacy") return true;
    if (id.size() != 32) return false;
    return std::all_of(id.begin(), id.end(), [](unsigned char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}

std::string AccountStore::tokenPath(const std::string& dataDir, const std::string& id) {
    if (!validId(id)) return {};
    if (id == "legacy") return dataDir + "/account.json";
    return dataDir + "/account_" + id + ".json";
}

std::string AccountStore::libraryCachePath(const std::string& dataDir, const std::string& id) {
    if (!validId(id)) return {};
    if (id == "legacy") return dataDir + "/library_cache.json";
    return dataDir + "/library_cache_" + id + ".json";
}

std::string AccountStore::newId() {
    std::array<uint8_t, 16> bytes{};
    if (!platform::randomBytes(bytes.data(), bytes.size())) return {};
    static constexpr char kHex[] = "0123456789abcdef";
    std::string id;
    id.reserve(bytes.size() * 2);
    for (uint8_t b : bytes) {
        id.push_back(kHex[b >> 4]);
        id.push_back(kHex[b & 0x0f]);
    }
    return id;
}

bool AccountStore::load() {
    accounts_.clear();
    activeId_.clear();
    std::string text;
    if (!platform::readFile(path_, text)) return false;
    auto root = json::parse(text);
    if (!root || !(*root).isObject() || (*root)["version"].asInt() != 1 || !(*root)["accounts"].isArray())
        return false;

    for (const auto& item : (*root)["accounts"].items()) {
        AccountEntry entry{item["id"].str(), item["gamertag"].str(), item["xuid"].str(), item["gamerpicUrl"].str()};
        if (!validId(entry.id) || entry.xuid.empty() || contains(entry.id)) continue;
        accounts_.push_back(std::move(entry));
    }
    const std::string requested = (*root)["activeId"].str();
    if (contains(requested)) activeId_ = requested;
    else if (!accounts_.empty()) activeId_ = accounts_.front().id;
    return true;
}

bool AccountStore::save() const {
    json::Value root = json::Value::object();
    root.set("version", 1);
    root.set("activeId", contains(activeId_) ? activeId_ : std::string());
    json::Value list = json::Value::array();
    for (const auto& entry : accounts_) {
        if (!validId(entry.id) || entry.xuid.empty()) continue;
        json::Value item = json::Value::object();
        item.set("id", entry.id);
        item.set("gamertag", entry.gamertag);
        item.set("xuid", entry.xuid);
        item.set("gamerpicUrl", entry.gamerpicUrl);
        list.push(std::move(item));
    }
    root.set("accounts", std::move(list));
    if (!platform::writeFileAtomic(path_, root.dump())) return false;
    platform::makePrivate(path_);
    return true;
}

bool AccountStore::upsert(const AccountEntry& entry) {
    if (!validId(entry.id) || entry.xuid.empty()) return false;
    const auto oldAccounts = accounts_;
    const std::string oldActive = activeId_;
    auto it = std::find_if(accounts_.begin(), accounts_.end(), [&](const AccountEntry& old) { return old.id == entry.id; });
    if (it == accounts_.end()) accounts_.push_back(entry);
    else *it = entry;
    activeId_ = entry.id;
    if (save()) return true;
    accounts_ = oldAccounts;
    activeId_ = oldActive;
    return false;
}

bool AccountStore::setActive(const std::string& id) {
    if (!contains(id)) return false;
    const std::string oldActive = activeId_;
    activeId_ = id;
    if (save()) return true;
    activeId_ = oldActive;
    return false;
}

bool AccountStore::remove(const std::string& id) {
    const auto oldAccounts = accounts_;
    const std::string oldActive = activeId_;
    const auto oldSize = accounts_.size();
    accounts_.erase(std::remove_if(accounts_.begin(), accounts_.end(), [&](const AccountEntry& entry) { return entry.id == id; }),
                    accounts_.end());
    if (accounts_.size() == oldSize) return false;
    if (activeId_ == id) activeId_ = accounts_.empty() ? std::string() : accounts_.front().id;
    if (save()) return true;
    accounts_ = oldAccounts;
    activeId_ = oldActive;
    return false;
}

bool AccountStore::contains(const std::string& id) const { return find(id) != nullptr; }

const AccountEntry* AccountStore::find(const std::string& id) const {
    const auto it = std::find_if(accounts_.begin(), accounts_.end(), [&](const AccountEntry& entry) { return entry.id == id; });
    return it == accounts_.end() ? nullptr : &*it;
}

} // namespace xc::auth
