// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 RafaelNGP
#include <cstdint>
#include "xcloud/gssv.h"

#include "auth/msa.h"
#include "net/http.h"
#include "util/json.h"
#include "platform/platform.h"
#include "util/log.h"
#include "xcloud/catalog.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <map>
#include <optional>

namespace xc::xcloud {

namespace {
constexpr const char* kClientHeader = "XboxComBrowser";
constexpr int kLoginTries = 6;  // xCloud login: tries on a 5xx / no answer

// Sent like the Windows Xbox app does: "Windows" plus a 1080p display is what
// gets a 1080p stream; an Android device with a 720p display gets 720p.
const std::string& deviceInfo(Resolution res, bool preview = true) {
    auto build = [](bool hd, bool prev, int w = 1920, int h = 1080) {
        json::Value env = json::Value::object();
        env.set("clientAppId", prev ? "play.xbox.com" : "Microsoft.GamingApp");
        env.set("clientAppType", prev ? "browser" : "native");
        env.set("clientAppVersion", prev ? "26.1.97" : "2203.1001.4.0");
        env.set("clientSdkVersion", prev ? "10.3.7" : "8.5.2");
        env.set("httpEnvironment", "prod");
        env.set("sdkInstallId", "");
        json::Value app = json::Value::object();
        app.set("env", env);
        json::Value hw = json::Value::object();
        hw.set("make", hd ? "Microsoft" : "Google");
        hw.set("model", hd ? "Surface Pro" : "Pixel");
        hw.set("sdktype", prev ? "web" : "native");
        json::Value os = json::Value::object();
        os.set("name", hd ? "Windows 11" : "Android");
        os.set("ver", hd ? "22631.2715" : "14");
        os.set("platform", hd ? "desktop" : "phone");
        json::Value dims = json::Value::object();
        dims.set("widthInPixels", hd ? w : 1280);
        dims.set("heightInPixels", hd ? h : 720);
        json::Value dpi = json::Value::object();
        dpi.set("dpiX", 1);
        dpi.set("dpiY", 1);
        json::Value display = json::Value::object();
        display.set("dimensions", dims);
        display.set("pixelDensity", dpi);
        json::Value dev = json::Value::object();
        dev.set("hw", hw);
        dev.set("os", os);
        dev.set("displayInfo", display);
        json::Value v = json::Value::object();
        v.set("appInfo", app);
        v.set("dev", dev);
        return v.dump();
    };
    // 1440p: the xbox.com web client (SDK 10.6.62) in Edge on a 1440p display,
    // field for field as its X-MS-Device-Info interceptor builds it.
    auto makeQhd = [](bool prev) {
        json::Value env = json::Value::object();
        env.set("clientAppId", prev ? "play.xbox.com" : "www.xbox.com");
        env.set("clientAppType", "browser");
        env.set("clientAppVersion", "1.0.2610.0201");
        env.set("clientSdkVersion", "10.6.65");
        env.set("httpEnvironment", "prod");
        env.set("sdkInstallId", [] {  // filled in by the web client; random, per run (nothing of the console)
            uint8_t b[16];
            platform::randomBytes(b, sizeof b);
            b[6] = static_cast<uint8_t>((b[6] & 0x0f) | 0x40);
            b[8] = static_cast<uint8_t>((b[8] & 0x3f) | 0x80);
            char id[37];
            std::snprintf(id, sizeof id, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", b[0], b[1],
                          b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
            return std::string(id);
        }());
        json::Value app = json::Value::object();
        app.set("env", env);
        json::Value dims = json::Value::object();
        dims.set("heightInPixels", 1440);
        dims.set("widthInPixels", 2560);
        json::Value dpi = json::Value::object();
        dpi.set("dpiX", 1);
        dpi.set("dpiY", 1);
        json::Value display = json::Value::object();
        display.set("dimensions", dims);
        display.set("pixelDensity", dpi);
        json::Value browser = json::Value::object();
        browser.set("browserName", "edge");
        browser.set("browserVersion", "141.0.0.0");
        json::Value hw = json::Value::object();
        hw.set("make", "Microsoft");
        hw.set("model", "Windows");
        hw.set("platformType", "desktop");
        hw.set("sdkType", "web");
        json::Value os = json::Value::object();
        os.set("name", "windows");
        os.set("ver", "10.0");
        os.set("platform", "desktop");
        json::Value dev = json::Value::object();
        dev.set("displayInfo", display);
        dev.set("browser", browser);
        dev.set("hw", hw);
        dev.set("os", os);
        json::Value v = json::Value::object();
        v.set("appInfo", app);
        v.set("dev", dev);
        return v.dump();
    };
    // 1080p HQ: the web client on a Tizen TV with a 4K panel, as Better
    // xCloud sends it.
    auto makeTv = [](bool prev) {
        json::Value env = json::Value::object();
        env.set("clientAppId", prev ? "play.xbox.com" : "www.xbox.com");
        env.set("clientAppType", "browser");
        env.set("clientAppVersion", "26.1.97");
        env.set("clientSdkVersion", "10.3.7");
        env.set("httpEnvironment", "prod");
        env.set("sdkInstallId", "");
        json::Value app = json::Value::object();
        app.set("env", env);
        json::Value os = json::Value::object();
        os.set("name", "tizen");
        os.set("ver", "22631.2715");
        os.set("platform", "desktop");
        json::Value hw = json::Value::object();
        hw.set("make", "Microsoft");
        hw.set("model", "unknown");
        hw.set("sdktype", "web");
        json::Value browser = json::Value::object();
        browser.set("browserName", "chrome");
        browser.set("browserVersion", "140.0.3485.54");
        json::Value dims = json::Value::object();
        dims.set("widthInPixels", 4096);
        dims.set("heightInPixels", 2160);
        json::Value dpi = json::Value::object();
        dpi.set("dpiX", 1);
        dpi.set("dpiY", 1);
        json::Value display = json::Value::object();
        display.set("dimensions", dims);
        display.set("pixelDensity", dpi);
        json::Value dev = json::Value::object();
        dev.set("os", os);
        dev.set("hw", hw);
        dev.set("browser", browser);
        dev.set("displayInfo", display);
        json::Value v = json::Value::object();
        v.set("appInfo", app);
        v.set("dev", dev);
        return v.dump();
    };
    static const std::string qhd_p = makeQhd(true), qhd_np = makeQhd(false);
    static const std::string tv_p = makeTv(true), tv_np = makeTv(false);
    static const std::string hd_p = build(true, true), hd_np = build(true, false);
    static const std::string sd_p = build(false, true), sd_np = build(false, false);
    switch (res) {
    case Resolution::P720: return preview ? sd_p : sd_np;
    case Resolution::P1440: return preview ? qhd_p : qhd_np;
    case Resolution::P1080HQ: return preview ? tv_p : tv_np;
    default: return preview ? hd_p : hd_np;
    }
}

std::string describe(const net::Response& r) {
    if (r.status == 0) return r.error;
    std::string s = "HTTP " + std::to_string(r.status);
    std::string fields = errorFields(r.body);
    if (!fields.empty()) s += ": " + fields;
    return s;
}

SessionState parseState(const std::string& s) {
    if (s == "Provisioning") return SessionState::Provisioning;
    if (s == "WaitingForResources") return SessionState::WaitingForResources;
    if (s == "ReadyToConnect") return SessionState::ReadyToConnect;
    if (s == "Provisioned") return SessionState::Provisioned;
    if (s == "Failed") return SessionState::Failed;
    return SessionState::Unknown;
}

// The SDP/ICE endpoints wrap their payload as a JSON *string* inside
// "exchangeResponse"; unwrap it.
std::optional<json::Value> exchangePayload(const json::Value& v) {
    const auto& ex = v["exchangeResponse"];
    if (ex.isString()) return json::parse(ex.asString());
    if (ex.isObject() || ex.isArray()) return ex;
    return std::nullopt;
}
}  // namespace

const char* toString(SessionState s) {
    switch (s) {
        case SessionState::Provisioning: return "Provisioning";
        case SessionState::WaitingForResources: return "WaitingForResources";
        case SessionState::ReadyToConnect: return "ReadyToConnect";
        case SessionState::Provisioned: return "Provisioned";
        case SessionState::Failed: return "Failed";
        default: return "Unknown";
    }
}

const Region* GssvLogin::defaultRegion() const {
    for (const auto& r : regions)
        if (r.isDefault) return &r;
    return regions.empty() ? nullptr : &regions.front();
}

std::string GssvClient::url(const std::string& path) const { return region_.baseUri + path; }

std::string GssvClient::sessionUrl(const std::string& suffix) const {
    std::string p = sessionPath_;
    if (!p.empty() && p.front() != '/') p.insert(p.begin(), '/');
    return region_.baseUri + p + suffix;
}

static net::Request authed(Resolution res, bool preview, const GssvLogin& l, std::string method, std::string url,
                           std::string body = {}) {
    net::Request req;
    req.method = std::move(method);
    req.url = std::move(url);
    req.body = std::move(body);
    req.headers = {{"Authorization", "Bearer " + l.gsToken},
                   {"x-gssv-client", kClientHeader},
                   {"X-MS-Device-Info", deviceInfo(res, preview)}};
    if (!req.body.empty() || req.method == "POST") req.headers.emplace_back("Content-Type", "application/json");
    return req;
}

std::string errorFields(const std::string& body) {
    auto j = json::parse(body);
    if (!j || !j->isObject()) return {};
    std::string out;
    auto add = [&](const json::Value& v) {
        for (const char* key : {"status", "code", "errorCode", "message", "errorMessage", "reason"}) {
            const auto& f = v[key];
            std::string text = f.isString() ? f.str() : f.isNumber() ? std::to_string(f.asInt()) : std::string();
            if (!text.empty()) out += (out.empty() ? "" : ", ") + std::string(key) + "=" + text.substr(0, 200);
        }
    };
    add(*j);
    add((*j)["errorDetails"]);
    return out;
}

const char* loginCountryAddress(const std::string& country) {
    static const struct {
        const char* country;
        const char* address;
    } kAddresses[] = {{"br", "169.150.198.66"},
                      {"jp", "138.199.21.239"},
                      {"kr", "121.125.60.151"},
                      {"pl", "45.134.212.66"},
                      {"us", "143.244.47.65"}};
    for (const auto& a : kAddresses)
        if (country == a.country) return a.address;
    return nullptr;
}

bool GssvClient::login(const auth::XblToken& gssvXsts, std::string& err) {
    json::Value body = json::Value::object();
    body.set("token", gssvXsts.token);
    body.set("offeringId", offering_);

    net::Request req;
    req.method = "POST";
    req.url = "https://" + offering_ + ".gssv-play-prod.xboxlive.com/v2/login/user";
    req.headers = {{"Content-Type", "application/json"}, {"x-gssv-client", kClientHeader}};
    req.body = body.dump();
    if (const char* address = loginCountryAddress(loginCountry_)) {
        req.headers.emplace_back("X-Forwarded-For", address);
        XC_LOGI("xCloud login as country %s", loginCountry_.c_str());  // not the address
    }
    // The service fails now and then with a 5xx (or is slow to answer) and
    // works a little later: up to six tries, 1, 2, 4, 8 and 8 s apart. A refusal
    // (4xx) is an answer, not tried again.
    net::Response r;
    std::optional<json::Value> j;
    for (int attempt = 1;; ++attempt) {
        r = net::perform(req);
        j = json::parse(r.body);
        if (r.ok() && j && (*j)["gsToken"].isString()) break;
        bool transient = r.status == 0 || r.status == 408 || r.status == 429 || r.status >= 500;
        if (!transient || attempt == kLoginTries) break;
        XC_LOGW("xCloud login: %s, trying again (%d/%d)", describe(r).c_str(), attempt + 1, kLoginTries);
        platform::sleepMs(1000u << std::min(attempt - 1, 3));
    }
    loginStatus_ = r.status;
    if (!r.ok() || !j || !(*j)["gsToken"].isString()) {
        err = "xCloud login failed: " + describe(r);
        if (r.status == 403) err += " (no Game Pass plan with cloud gaming, or xCloud is not available in this region)";
        return false;
    }
    if (std::getenv("XC_DUMP_LOGIN")) {
        // Diagnostics: everything except the token itself.
        json::Value copy = *j;
        copy.set("gsToken", "<redacted>");
        XC_LOGI("login response: %s", copy.dump().substr(0, 6000).c_str());
    }
    login_ = {};
    login_.gsToken = (*j)["gsToken"].str();
    login_.market = (*j)["market"].str();
    login_.expiresAt = auth::unixNow() + (*j)["durationInSeconds"].asInt(3600);
    for (const auto& r : (*j)["offeringSettings"]["regions"].items()) {
        Region reg;
        reg.name = r["name"].str();
        reg.baseUri = r["baseUri"].str();
        reg.isDefault = r["isDefault"].asBool();
        if (!reg.baseUri.empty()) login_.regions.push_back(std::move(reg));
    }
    if (const Region* def = login_.defaultRegion()) {
        region_ = *def;
    } else {
        err = "xCloud login returned no regions";
        return false;
    }
    XC_LOGI("xCloud login ok: market=%s region=%s (%zu regions)", login_.market.c_str(), region_.name.c_str(),
            login_.regions.size());
    return true;
}

bool GssvClient::listTitles(std::vector<Title>& out, std::string& err, bool recentOnly) {
    out.clear();
    uint64_t started = platform::nowMs();
    std::string continuation;
    int pages = 0;
    std::map<std::string, int> inputTypes;  // for the log: how many titles take each
    do {
        std::string path = recentOnly ? "/v2/titles/mru?mr=25" : "/v2/titles?mr=200";
        if (!continuation.empty()) path += "&ct=" + net::urlEncode(continuation);
        // A page that fails for a moment (5xx, a timeout, "slow down") is asked again with the same
        // token: one bad answer in a list of fifteen pages no longer throws the whole list away.
        net::Response r;
        std::optional<json::Value> j;
        constexpr int kTries = 3;
        for (int attempt = 1; attempt <= kTries; ++attempt) {
            r = net::perform(authed(resolution_, previewFeatures_, login_, "GET", url(path)));
            j = json::parse(r.body);
            bool transient = r.status == 0 || r.status == 408 || r.status == 429 || r.status >= 500 || (r.ok() && !j);
            if ((r.ok() && j) || !transient || attempt == kTries) break;
            XC_LOGW("title list: page %d: %s, asking again (%d/%d)", pages + 1, describe(r).c_str(), attempt + 1, kTries);
            platform::sleepMs(1000u << (attempt - 1));
        }
        if (!r.ok() || !j) {
            err = "title list failed: " + describe(r);
            return false;
        }
        std::string().swap(r.body);
        // Debugging: XC_DUMP_TITLES="ID1,ID2" prints those entries as sent.
        static const char* dump = std::getenv("XC_DUMP_TITLES");
        const auto& results = (*j)["results"].items();
        out.reserve(out.size() + results.size());
        for (const auto& t : results) {
            if (dump && ("," + std::string(dump) + ",").find("," + t["titleId"].str() + ",") != std::string::npos)
                std::printf("%s\n", t.dump().c_str());
            Title title;
            title.titleId = t["titleId"].str();
            title.productId = t["details"]["productId"].str();
            title.hasEntitlement = t["details"]["hasEntitlement"].asBool();
            title.xboxTitleId = t["details"]["xboxTitleId"].str();
            title.isFreeInStore = t["details"]["isFreeInStore"].asBool();
            for (const auto& alt : t["details"]["alternateProductIds"].items()) {
                if (alt.isString() && !alt.str().empty()) title.alternateProductIds.push_back(alt.str());
            }
            if (title.productId.empty() && !title.alternateProductIds.empty()) {
                title.productId = title.alternateProductIds[0];
            }
            for (const auto& input : t["details"]["supportedInputTypes"].items()) {
                ++inputTypes[input.str()];
                if (input.str() == "MKB") title.mouseKeyboard = true;
            }
            for (const auto& d : t["details"]["allowedDeviceTypes"].items())
                if (d.isString() && !d.str().empty()) title.allowedDeviceTypes.push_back(d.str());
            // What lets this account play it: a subscription's program
            // (CALLISTO for Game Pass in the cloud), F2P, or FERDINAND, the
            // free tier with ads (an account with no subscription has it:
            // counting it as one showed the whole Game Pass catalog as
            // playable to a free account); none when bought.
            for (const auto& program : t["details"]["userPrograms"].items()) {
                if (program.str() == "FERDINAND") title.freeTier = true;
                else if (program.str() != "F2P") title.viaSubscription = true;
            }
            title.maxSessionSeconds = static_cast<int>(t["details"]["maxSessionLengthInSeconds"].asInt(0));
            if (!title.titleId.empty() && !isNonGameAddon(title.titleId)) out.push_back(std::move(title));
        }
        // The next page; a token that repeats, an empty page or too many
        // pages end the list (a server that always says "more" can't loop).
        std::string next = recentOnly ? std::string() : (*j)["continuationToken"].str();
        ++pages;
        if (!next.empty() && (next == continuation || (*j)["results"].items().empty() || pages >= 50)) {
            XC_LOGW("title list: stopped paging after %d page(s), %zu titles", pages, out.size());
            next.clear();
        }
        continuation = next;
    } while (!continuation.empty() && out.size() < 10000);
    XC_LOGI("title list%s: %zu titles in %llu ms", recentOnly ? " (recent)" : "", out.size(),
            static_cast<unsigned long long>(platform::nowMs() - started));
    if (!inputTypes.empty()) {
        std::string types;
        for (const auto& [name, n] : inputTypes) types += (types.empty() ? "" : ", ") + name + " " + std::to_string(n);
        XC_LOGI("title list: input types: %s", types.c_str());
    }
    return true;
}

bool GssvClient::hydrateTitles(std::vector<Title>& titles, const std::string& market, const std::string& lang,
                               std::string& err) {
    std::map<std::string, std::vector<Title*>> byProduct;
    for (auto& t : titles)
        if (!t.productId.empty()) byProduct[t.productId].push_back(&t);

    std::vector<std::string> ids;
    for (const auto& [id, _] : byProduct) ids.push_back(id);

    constexpr size_t kBatch = 20;
    for (size_t i = 0; i < ids.size(); i += kBatch) {
        std::string list;
        for (size_t k = i; k < ids.size() && k < i + kBatch; ++k) {
            if (!list.empty()) list += ',';
            list += ids[k];
        }
        net::Request req;
        req.url = "https://displaycatalog.mp.microsoft.com/v7.0/products?bigIds=" + list +
                  "&market=" + market + "&languages=" + lang;
        auto r = net::perform(req);
        auto j = json::parse(r.body);
        if (!r.ok() || !j) {
            err = "catalog lookup failed: " + describe(r);
            return false;
        }
        for (const auto& p : (*j)["Products"].items()) {
            auto it = byProduct.find(p["ProductId"].str());
            if (it == byProduct.end()) continue;
            const auto& loc = p["LocalizedProperties"][0];
            std::string name = loc["ProductTitle"].str();
            std::string image;
            for (const char* purpose : {"Poster", "BoxArt", "Tile"}) {
                for (const auto& img : loc["Images"].items()) {
                    if (img["ImagePurpose"].st
