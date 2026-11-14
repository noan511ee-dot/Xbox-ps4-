// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/intro_video.h"

#include <algorithm>
#include <cstdio>
#include <mutex>

#include "display/display.h"
#include "media/audio_out.h"
#include "platform/platform.h"
#include "util/log.h"

namespace xc::app {

namespace {

constexpr double kFrameMs = 1001.0 / 30.0;  // 29.97 fps
constexpr size_t kSampleRate = 48000;
constexpr size_t kAudioLeadFrames = 1920;   // 40 ms queued ahead of the picture
constexpr uint64_t kStartTimeoutMs = 4000;  // no picture by then: give up
constexpr size_t kMaxStreamBytes = 8u << 20;
constexpr size_t kMaxPcmBytes = 16u << 20;

bool readFile(const std::string& path, std::vector<uint8_t>& out, size_t limit) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (size <= 0 || static_cast<size_t>(size) > limit) {
        std::fclose(f);
        return false;
    }
    out.resize(static_cast<size_t>(size));
    size_t got = std::fread(out.data(), 1, out.size(), f);
    std::fclose(f);
    return got == out.size();
}

}  // namespace

IntroVideo::~IntroVideo() {
    if (active_) finish("destroyed");
}

bool IntroVideo::start(const std::string& assetDir) {
    const std::string base = assetDir + "/intro/";
    if (!readFile(base + "intro.h264", stream_, kMaxStreamBytes)) {
        XC_LOGI("intro: no clip in %s", base.c_str());
        return false;
    }
    units_ = media::splitAccessUnits(stream_.data(), stream_.size());
    if (units_.size() < 2) {
        XC_LOGW("intro: clip has no frames");
        return false;
    }
    std::vector<uint8_t> raw;
    if (readFile(base + "intro.pcm", raw, kMaxPcmBytes)) {
        pcm_.resize(raw.size() / sizeof(int16_t));
        std::copy(raw.begin(), raw.begin() + static_cast<long>(pcm_.size() * sizeof(int16_t)),
                  reinterpret_cast<uint8_t*>(pcm_.data()));
    }
    decoder_ = std::make_unique<media::VideoDecoder>();
    // Software decoding: a 720p clip is light for any PS4 (Slim, Pro or the
    // original), and it leaves the hardware decoder untouched for the game stream.
    if (!decoder_->init(1, false)) {
        XC_LOGW("intro: decoder not available");
        decoder_.reset();
        return false;
    }
    if (!pcm_.empty()) {
        audioOn_ = media::audioStart();
        if (!audioOn_) XC_LOGW("intro: no audio output, playing silent");
    }
    active_ = true;
    startMs_ = 0;
    XC_LOGI("intro: %zu frames, %zu audio frames", units_.size(), pcm_.size() / 2);
    return true;
}

void IntroVideo::skip() {
    if (active_) finish("skipped");
}

void IntroVideo::finish(const char* why) {
    XC_LOGI("intro: end (%s), %d frames shown, %d dropped", why, shown_, dropped_);
    active_ = false;
    if (audioOn_) {
        media::audioStop();
        audioOn_ = false;
    }
    decoder_.reset();
    std::vector<uint8_t>().swap(stream_);
    std::vector<std::pair<size_t, size_t>>().swap(units_);
    std::vector<int16_t>().swap(pcm_);
}

bool IntroVideo::step(uint64_t nowMs) {
    if (!active_) return false;
    if (!startMs_) startMs_ = nowMs;
    const uint64_t elapsed = nowMs - startMs_;

    // Sound first: it is the clock the ear follows. Queue only a little ahead,
    // the audio queue trims anything beyond ~60 ms.
    if (audioOn_) {
        const size_t totalFrames = pcm_.size() / 2;
        size_t want = std::min(totalFrames, static_cast<size_t>(elapsed * kSampleRate / 1000) + kAudioLeadFrames);
        while (audioPos_ < want) {
            size_t n = std::min<size_t>(want - audioPos_, 960);
            float buf[960 * 2];
            for (size_t i = 0; i < n * 2; ++i) buf[i] = static_cast<float>(pcm_[audioPos_ * 2 + i]) / 32768.0f;
            media::audioPush(buf, n);
            audioPos_ += n;
        }
    }

    // Every frame that is due is decoded (the P frames need their references);
    // only the newest one is shown, so a slow pass drops pictures, never stalls.
    const size_t due = std::min(units_.size(), static_cast<size_t>(static_cast<double>(elapsed) / kFrameMs) + 1);
    media::Picture last;
    bool haveLast = false;
    int decodedNow = 0;
    while (next_ < due) {
        media::Picture pic;
        const auto& u = units_[next_];
        bool got = decoder_->decode(stream_.data() + u.first, u.second, pic, static_cast<uint32_t>(next_));
        ++next_;
        ++decodedNow;
        if (got && pic.y) {
            if (haveLast) ++dropped_;
            last = pic;
            haveLast = true;
        }
        // The last picture's planes stay valid only until the next decode().
        if (haveLast && next_ < due) {
            // More to decode in this pass: show this one now, keep the pace.
            std::lock_guard<std::recursive_mutex> lock(display::frameMutex());
            bool ok = last.format == media::Picture::Format::Nv12
                          ? display::drawNv12(last.y, last.u, last.strideY, last.strideU, last.width, last.height, false)
                          : display::drawYuv420(last.y, last.u, last.v, last.strideY, last.strideU, last.strideV,
                                                last.width, last.height, false);
            if (ok) {
                display::present();
                ++shown_;
                lastShownMs_ = nowMs;
            }
            haveLast = false;
        }
        if (decodedNow >= 4) break;  // never hold the loop for long
    }
    if (haveLast) {
        std::lock_guard<std::recursive_mutex> lock(display::frameMutex());
        bool ok = last.format == media::Picture::Format::Nv12
                      ? display::drawNv12(last.y, last.u, last.strideY, last.strideU, last.width, last.height, true)
                      : display::drawYuv420(last.y, last.u, last.v, last.strideY, last.strideU, last.strideV,
                                            last.width, last.height, true);
        if (ok) {
            display::present();
            ++shown_;
            lastShownMs_ = nowMs;
        }
    }

    if (shown_ == 0 && elapsed > kStartTimeoutMs) {
        finish("no picture");
        return false;
    }
    if (next_ >= units_.size() && elapsed >= static_cast<uint64_t>(units_.size() * kFrameMs) + 150) {
        finish("done");
        return false;
    }
    if (decodedNow == 0) display::idleWait(4);
    return true;
}

}  // namespace xc::app
