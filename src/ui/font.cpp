// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 RafaelNGP
#include "ui/font.h"

#include <cstdio>

#include "platform/platform.h"
#include "util/log.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

namespace xc::ui {

uint32_t nextCodepoint(std::string_view s, size_t& i);

namespace {
struct ArabicForm {
    uint32_t base, isolated, final, initial, medial;
    bool right, left;
};

// Presentation forms keep the renderer dependency-free: stb_truetype can
// draw the shaped glyphs, while this small table handles the Arabic UI labels.
constexpr ArabicForm kArabicForms[] = {
    {0x0627, 0xFE8D, 0xFE8E, 0, 0, true, false}, {0x0628, 0xFE8F, 0xFE90, 0xFE91, 0xFE92, true, true},
    {0x062A, 0xFE95, 0xFE96, 0xFE97, 0xFE98, true, true}, {0x062B, 0xFE99, 0xFE9A, 0xFE9B, 0xFE9C, true, true},
    {0x062C, 0xFE9D, 0xFE9E, 0xFE9F, 0xFEA0, true, true}, {0x062D, 0xFEA1, 0xFEA2, 0xFEA3, 0xFEA4, true, true},
    {0x062E, 0xFEA5, 0xFEA6, 0xFEA7, 0xFEA8, true, true}, {0x062F, 0xFEA9, 0xFEAA, 0, 0, true, false},
    {0x0630, 0xFEAB, 0xFEAC, 0, 0, true, false}, {0x0631, 0xFEAD, 0xFEAE, 0, 0, true, false},
    {0x0632, 0xFEAF, 0xFEB0, 0, 0, true, false}, {0x0633, 0xFEB1, 0xFEB2, 0xFEB3, 0xFEB4, true, true},
    {0x0634, 0xFEB5, 0xFEB6, 0xFEB7, 0xFEB8, true, true}, {0x0635, 0xFEB9, 0xFEBA, 0xFEBB, 0xFEBC, true, true},
    {0x0636, 0xFEBD, 0xFEBE, 0xFEBF, 0xFEC0, true, true}, {0x0637, 0xFEC1, 0xFEC2, 0xFEC3, 0xFEC4, true, true},
    {0x0638, 0xFEC5, 0xFEC6, 0xFEC7, 0xFEC8, true, true}, {0x0639, 0xFEC9, 0xFECA, 0xFECB, 0xFECC, true, true},
    {0x063A, 0xFECD, 0xFECE, 0xFECF, 0xFED0, true, true}, {0x0641, 0xFED1, 0xFED2, 0xFED3, 0xFED4, true, true},
    {0x0642, 0xFED5, 0xFED6, 0xFED7, 0xFED8, true, true}, {0x0643, 0xFED9, 0xFEDA, 0xFEDB, 0xFEDC, true, true},
    {0x0644, 0xFEDD, 0xFEDE, 0xFEDF, 0xFEE0, true, true}, {0x0645, 0xFEE1, 0xFEE2, 0xFEE3, 0xFEE4, true, true},
    {0x0646, 0xFEE5, 0xFEE6, 0xFEE7, 0xFEE8, true, true}, {0x0647, 0xFEE9, 0xFEEA, 0xFEEB, 0xFEEC, true, true},
    {0x0648, 0xFEED, 0xFEEE, 0, 0, true, false}, {0x064A, 0xFEF1, 0xFEF2, 0xFEF3, 0xFEF4, true, true},
    {0x0629, 0xFE93, 0xFE94, 0, 0, true, false}, {0x0649, 0xFEEF, 0xFEF0, 0, 0, true, false},
};

const ArabicForm* arabicForm(uint32_t cp) {
    for (const auto& f : kArabicForms) if (f.base == cp) return &f;
    return nullptr;
}
bool isArabicMark(uint32_t cp) { return cp >= 0x064B && cp <= 0x065F; }
void appendUtf8(std::string& out, uint32_t cp) {
    if (cp < 0x80) out.push_back(static_cast<char>(cp));
    else if (cp < 0x800) { out.push_back(static_cast<char>(0xC0 | (cp >> 6))); out.push_back(static_cast<char>(0x80 | (cp & 63))); }
    else if (cp < 0x10000) { out.push_back(static_cast<char>(0xE0 | (cp >> 12))); out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 63))); out.push_back(static_cast<char>(0x80 | (cp & 63))); }
    else { out.push_back(static_cast<char>(0xF0 | (cp >> 18))); out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 63))); out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 63))); out.push_back(static_cast<char>(0x80 | (cp & 63))); }
}

std::string shapeArabic(std::string_view text) {
    std::vector<uint32_t> cps;
    for (size_t i = 0; i < text.size();) cps.push_back(nextCodepoint(text, i));
    std::vector<uint32_t> out;
    for (size_t i = 0; i < cps.size();) {
        if (!arabicForm(cps[i])) { out.push_back(cps[i++]); continue; }
        size_t end = i;
        while (end < cps.size() && (arabicForm(cps[end]) || isArabicMark(cps[end]))) ++end;
        std::vector<uint32_t> run;
        for (size_t j = i; j < end; ++j) {
            const ArabicForm* f = arabicForm(cps[j]);
            if (!f) { run.push_back(cps[j]); continue; }
            const ArabicForm* prev = j > i ? arabicForm(cps[j - 1]) : nullptr;
            const ArabicForm* next = j + 1 < end ? arabicForm(cps[j + 1]) : nullptr;
            bool joinPrev = prev && prev->right && f->left;
            bool joinNext = next && f->right && next->left;
            run.push_back(joinPrev && joinNext && f->medial ? f->medial : joinPrev && f->final ? f->final : joinNext && f->initial ? f->initial : f->isolated);
        }
        std::reverse(run.begin(), run.end());
        out.insert(out.end(), run.begin(), run.end());
        i = end;
    }
    std::string result;
    for (uint32_t cp : out) appendUtf8(result, cp);
    return result;
}
}

struct Font::Impl {
    stbtt_fontinfo info{};
    int ascent = 0, descent = 0, lineGap = 0;
    int capY1 = 0;
    bool hasCapBox = false;
};

Font::Font() : impl_(std::make_unique<Impl>()) {}
Font::~Font() = default;

bool Font::load(const std::string& path) {
    loaded_ = false;
    std::string bytes;
    if (!platform::readFile(path, bytes)) {
        XC_LOGE("font %s: cannot read", path.c_str());
        return false;
    }
    data_.assign(bytes.begin(), bytes.end());
    if (!stbtt_InitFont(&impl_->info, data_.data(), stbtt_GetFontOffsetForIndex(data_.data(), 0))) {
        XC_LOGE("font %s: not a TrueType font", path.c_str());
        return false;
    }
    stbtt_GetFontVMetrics(&impl_->info, &impl_->ascent, &impl_->descent, &impl_->lineGap);
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    impl_->hasCapBox = stbtt_GetCodepointBox(&impl_->info, 'H', &x0, &y0, &x1, &y1) != 0;
    impl_->capY1 = y1;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
    }
    loaded_ = true;
    return true;
}

bool Font::usable() const {
    if (loaded_) return true;
    static std::atomic<bool> told{false};
    if (!told.load(std::memory_order_relaxed) && !told.exchange(true)) XC_LOGE("a font is missing: its text isn't drawn");
    return false;
}

int Font::centeredY(int boxY, int boxH, int px) const {
    if (!usable()) return boxY;
    float s = scale(px);
    float capH = impl_->hasCapBox ? impl_->capY1 * s : 0.7f * px;
    float ascent = impl_->ascent * s;  // line top to baseline
    // Cap top = y + ascent - capH; centre it: (boxH - capH) / 2 below boxY.
    return static_cast<int>(std::lround(boxY + (boxH - capH) / 2.0f - (ascent - capH)));
}

float Font::scale(int px) const { return stbtt_ScaleForPixelHeight(&impl_->info, static_cast<float>(px)); }

int Font::lineHeight(int px) const {
    if (!usable()) return px;
    return static_cast<int>(std::ceil((impl_->ascent - impl_->descent) * scale(px)));
}

uint32_t nextCodepoint(std::string_view s, size_t& i) {
    auto c = static_cast<unsigned char>(s[i++]);
    if (c < 0x80) return c;
    int extra = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : -1;
    if (extra < 0) return 0xFFFD;
    uint32_t cp = c & (0x3F >> extra);
    for (int k = 0; k < extra; ++k) {
        if (i >= s.size() || (static_cast<unsigned char>(s[i]) & 0xC0) != 0x80) return 0xFFFD;
        cp = (cp << 6) | (static_cast<unsigned char>(s[i++]) & 0x3F);
    }
    return cp;
}

const Font::Glyph& Font::glyph(uint32_t cp, int px) const {
    static const Glyph kNone;
    if (!usable()) return kNone;
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t key = (static_cast<uint64_t>(cp) << 32) | static_cast<uint32_t>(px);
    auto it = cache_.find(key);
    if (it != cache_.end()) return it->second;
    Glyph g;
    // Characters the font lacks are skipped instead of drawn as boxes.
    if (cp != ' ' && stbtt_FindGlyphIndex(&impl_->info, static_cast<int>(cp)) == 0) {
        if (fallback_) return fallback_->glyph(cp, px);
        return cache_.emplace(key, std::move(g)).first->second;
    }
    float s = scale(px);
    int advance, lsb;
    stbtt_GetCodepointHMetrics(&impl_->info, static_cast<int>(cp), &advance, &lsb);
    g.advance = static_cast<int>(std::lround(advance * s));
    unsigned char* bmp =
        stbtt_GetCodepointBitmap(&impl_->info, 0, s, static_cast<int>(cp), &g.w, &g.h, &g.xoff, &g.yoff);
    if (bmp) {
        g.bitmap.assign(bmp, bmp + static_cast<size_t>(g.w) * g.h);
        stbtt_FreeBitmap(bmp, nullptr);
    }
    return cache_.emplace(key, std::move(g)).first->second;
}

int Font::measure(std::string_view text, int px) const {
    const std::string shaped = shapeArabic(text);
    text = shaped;
    int w = 0;
    for (size_t i = 0; i < text.size();) w += glyph(nextCodepoint(text, i), px).advance;
    return w;
}

int Font::draw(Canvas& c, std::string_view text, int x, int y, int px, Color color) const {
    if (!usable()) return x;
    const std::string shaped = shapeArabic(text);
    text = shaped;
    int baseline = y + static_cast<int>(std::lround(impl_->ascent * scale(px)));
    for (size_t i = 0; i < text.size();) {
        const Glyph& g = glyph(nextCodepoint(text, i), px);
        if (!g.bitmap.empty()) c.drawMask(g.bitmap.data(), g.w, g.h, g.w, x + g.xoff, baseline + g.yoff, color);
        x += g.advance;
    }
    return x;
}

std::string Font::fit(std::string_view text, int px, int maxWidth) const {
    const char* dots = "...";
    int dotsW = measure(dots, px);
    int budget = maxWidth - dotsW;
    int totalW = 0;
    size_t cutIdx = 0;
    bool cutSet = false;
    for (size_t i = 0; i < text.size();) {
        size_t start = i;
        int adv = glyph(nextCodepoint(text, i), px).advance;
        if (!cutSet && totalW + adv > budget) {
            cutIdx = start;
            cutSet = true;
        }
        totalW += adv;
        if (totalW > maxWidth && cutSet) {
            std::string out(text.substr(0, cutIdx));
            while (!out.empty() && out.back() == ' ') out.pop_back();
            return out + dots;
        }
    }
    if (totalW <= maxWidth) return std::string(text);
    std::string out(text.substr(0, cutSet ? cutIdx : 0));
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out + dots;
}

std::vector<std::string> Font::wrap(std::string_view input, int px, int maxWidth, int maxLines) const {
    // Store descriptions use \r\n and the odd tab.
    std::string cleaned;
    cleaned.reserve(input.size());
    for (char ch : input) {
        if (ch == '\r') continue;
        cleaned += ch == '\t' ? ' ' : ch;
    }
    const std::string shaped = shapeArabic(cleaned);
    std::string_view text = shaped;
    std::vector<std::string> lines;
    std::string current;
    size_t i = 0;
    auto flush = [&] {
        while (!current.empty() && current.back() == ' ') current.pop_back();
        lines.push_back(current);
        current.clear();
    };
    while (i < text.size()) {
        if (text[i] == '\n') {
            ++i;
            if (!current.empty()) flush();
            if (static_cast<int>(lines.size()) >= maxLines) break;
            continue;
        }
        size_t end = text.find_first_of(" \n", i);
        if (end == std::string_view::npos) end = text.size();
        std::string word(text.substr(i, end - i));
        std::string candidate = current.empty() ? word : current + " " + word;
        if (!current.empty() && measure(candidate, px) > maxWidth) {
            flush();
            if (static_cast<int>(lines.size()) >= maxLines) break;
            current = word;
        } else {
            current = candidate;
        }
        i = end < text.size() && text[end] == ' ' ? end + 1 : end;
    }
    if (!current.empty() && static_cast<int>(lines.size()) < maxLines) flush();
    bool truncated = i < text.size();
    if (truncated && !lines.empty()) lines.back() = fit(lines.back() + " ...", px, maxWidth);
    for (auto& l : lines) l = fit(l, px, maxWidth);
    return lines;
}

bool Fonts::load(const std::string& dir) {
    bool ok = regular.load(dir + "/Inter-Regular.ttf") && semibold.load(dir + "/Inter-SemiBold.ttf") &&
              bold.load(dir + "/Inter-Bold.ttf") && arabicRegular.load(dir + "/NotoSansArabic-Regular.ttf") &&
           arabicSemibold.load(dir + "/NotoSansArabic-SemiBold.ttf") &&
           arabicBold.load(dir + "/NotoSansArabic-Bold.ttf");
    if (ok) {
        regular.setFallback(&arabicRegular);
        semibold.setFallback(&arabicSemibold);
        bold.setFallback(&arabicBold);
    }
    return ok;
}

bool Fonts::loadIcons(const std::vector<std::string>& paths) {
    for (const auto& p : paths) {
        FILE* f = std::fopen(p.c_str(), "rb");  // a path the system doesn't have here: tried quietly
        if (!f) continue;
        std::fclose(f);
        if (icons.load(p)) return hasIcons = true;
    }
    return false;
}

}  // namespace xc::ui
